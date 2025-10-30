// Link layer protocol implementation

#include "../include/link_layer.h"
#include "../include/serial_port.h"
#include "../include/state_machine.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

extern int alarmEnabled;
extern int alarmCount;
extern void alarmHandler(int sig);

LinkLayer currentParams;
int linkFd = -1;
int FRAME_MAX_SIZE = 4096;

////////////////////////////////////////////////
/////////            LLOPEN              ///////
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);
    if (fd < 0) {
        perror("Error opening serial port!");
        return -1;
    }

    linkFd = fd;
    currentParams = connectionParameters;

    struct sigaction act = {0};
    act.sa_handler = alarmHandler;
    if (sigaction(SIGALRM, &act, NULL) < 0)
    {
        perror("Error configuring alarm handler");
        closeSerialPort();
        exit(1);
    }

    int TxRx;
    if(connectionParameters.role == LlTx){
        TxRx = transmissorLLopen();
    }
    else{
        TxRx = receptorLLopen();
    }

    return TxRx;
}

////////////////////////////////////////////////
/////////            TX_LLOPEN           ///////
////////////////////////////////////////////////
int transmissorLLopen(){

    unsigned char SET[5] = {FLAG, A_TX, C_SET, 0x00, FLAG};
    SET[3] = SET[1] ^ SET[2]; //BCC = A xor C

    unsigned char byte;
    int tries = 0;
    
    while(tries < currentParams.nRetransmissions){
        if(writeBytesSerialPort(SET, 5) != 5){
            perror("Failed to send SET");
            tries++;
        }
        printf("SET SENT\n");

        alarmEnabled = 1;
        alarm(currentParams.timeout);

        while(alarmEnabled == 1){
            int res = readByteSerialPort(&byte);
            if(res > 0){
                int state = state_machine(byte);
                if(state == 2){
                    printf("UA received\n");
                    alarm(0);
                    return linkFd;
                }
            }
        }

        tries++;
        printf("Timeout - tries: %d\n", tries);
    }

    printf("Fail establishing connection\n");
    return -1;
}

////////////////////////////////////////////////
/////////            RX_LLOPEN           ///////
////////////////////////////////////////////////
int receptorLLopen(){
    unsigned char byte;
    int tries = 0;

    while (tries < currentParams.nRetransmissions){
        alarmEnabled = 1;
        alarm(currentParams.timeout);

        while (alarmEnabled == 1){
            int res = readByteSerialPort(&byte);

            if (res > 0){
                int state = state_machine(byte);
                if (state == 1){
                    printf("SET RECEIVED\n");
                    alarm(0);

                    unsigned char UA[5] = {FLAG, A_RX, C_UA, 0x00, FLAG};
                    UA[3] = UA[1] ^ UA[2];

                    writeBytesSerialPort(UA, 5);
                    printf("UA SENT\n");
                    return linkFd;
                }
            }
        }

        tries++;
        printf("Timeout waiting for SET - tries: %d\n", tries);
    }

    printf("Receiver failed to establish connection\n");
    return -1;
}

//_____________________________________________________________________________________________________________________
////////////////////////////////////////////////
/////////           LLWRITE              ///////
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    if (linkFd < 0) {
        fprintf(stderr, "llwrite: link not open\n");
        return -1;
    }

    int tries = 0;
    static int sequenceNumber = 0; // 0 or 1

    unsigned char frame[2 * (bufSize + 6)];
    FRAME_MAX_SIZE = 2 * (bufSize + 6);
    int frameSize = buildIFrame(frame, buf, bufSize, sequenceNumber);

    while (tries < currentParams.nRetransmissions) {
        if (writeBytesSerialPort(frame, frameSize) != frameSize) {
            tries++;
            continue;
        }

        // Wait for RR/REJ
        alarmEnabled = TRUE;
        alarm(currentParams.timeout);

        int control = readControlField(); // returns -1 of timeout

        if (control == -1) {
            // timeout: reesend
            tries++;
            printf("llwrite: timeout waiting for RR/REJ — try %d\n", tries);
            continue;
        }

        if (control == C_RR0 || control == C_RR1) {
            sequenceNumber ^= 1;
            alarm(0);
            return bufSize; // success
        }
        else if (control == C_REJ0 || control == C_REJ1) {
            // REJ received — retransmit
            printf("llwrite: REJ received — retransmitting\n");
            tries++;
            continue;
        }
        else {
            // another frame, ignore
            printf("llwrite: unexpected control 0x%02X — ignoring\n", control);
            tries++;
            continue;
        }
    }

    printf("llwrite: failed after %d tries\n", tries);
    return -1;
}

int readControlField() {
    unsigned char buf[5];
    int i = 0;

    while (readByteSerialPort(&buf[i]) > 0) {
        if (buf[i] == FLAG && i >= 4) {
            unsigned char A = buf[1];
            unsigned char C = buf[2];
            unsigned char BCC = buf[3];
            if ((A ^ C) == BCC) {
                return C; // control
            }
        }
        i++;
    }
    return -1;
}

////////////////////////////////////////////////
/////////      BYTE STUFFING HELPERS     ///////
////////////////////////////////////////////////

unsigned char calculateBCC2(const unsigned char *buf, int bufSize)
{
    unsigned char bcc2 = 0x00;
    for (int i = 0; i < bufSize; i++)
        bcc2 ^= buf[i];
    return bcc2;
}

int applyByteStuffing(const unsigned char *input, int inputSize, unsigned char *output)
{
    int outIndex = 0;

    for (int i = 0; i < inputSize; i++)
    {
        if (input[i] == FLAG)
        {
            output[outIndex++] = ESC;
            output[outIndex++] = FLAG_ESC;
        }
        else if (input[i] == ESC)
        {
            output[outIndex++] = ESC;
            output[outIndex++] = ESC_ESC;
        }
        else
        {
            output[outIndex++] = input[i];
        }
    }

    return outIndex; // size after stuffing
}

////////////////////////////////////////////////
/////////       I-FRAME BUILDER          ///////
////////////////////////////////////////////////

int buildIFrame(unsigned char *frame, const unsigned char *buf, int bufSize, int sequenceNumber)
{
    int index = 0;
    unsigned char A = A_TX;
    unsigned char C = (sequenceNumber << 6); // 0x00 or 0x40
    unsigned char BCC1 = A ^ C;
    unsigned char BCC2 = calculateBCC2(buf, bufSize);

    frame[index++] = FLAG;
    frame[index++] = A;
    frame[index++] = C;
    frame[index++] = BCC1;

    // Data with stuffing
    unsigned char *stuffedData = malloc(2 * bufSize + 2);
    int stuffedSize = applyByteStuffing(buf, bufSize, stuffedData);
    for (int i = 0; i < stuffedSize; i++)
        frame[index++] = stuffedData[i];

    // BCC2 with stuffing
    unsigned char bcc2Stuffed[2];
    int bcc2Size = applyByteStuffing(&BCC2, 1, bcc2Stuffed);
    for (int i = 0; i < bcc2Size; i++){
        frame[index++] = bcc2Stuffed[i];
    }
    frame[index++] = FLAG;

    free(stuffedData);
    return index; // total size
}

//_____________________________________________________________________________________________________________________
////////////////////////////////////////////////
/////////            LLREAD              ///////
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    unsigned char frame[FRAME_MAX_SIZE];
    int frameSize = 0;
    int tries = 0;

    while (tries < currentParams.nRetransmissions) {

        alarmEnabled = 1;
        alarm(currentParams.timeout);

        while (alarmEnabled == 1) {
            frameSize = readFrame(frame, FRAME_MAX_SIZE);
            if (frameSize > 0) {
                alarm(0);              // read with success - alarm disabled
                alarmEnabled = 0;
                break;
            }
        }

        if (frameSize > 0) break; // frame read with success

        tries++;
        printf("llread: timeout waiting for frame — try %d\n", tries);
    }

    // Max number of tries
    if (tries >= currentParams.nRetransmissions && frameSize <= 0) {
        printf("llread: failed after %d tries — closing connection\n", tries);
        return -3; // fail reading
    }

    int seqNum;
    int payloadSize = validateIFrame(frame, frameSize, packet, &seqNum, FRAME_MAX_SIZE);

    if (payloadSize < 0) {
        printf("llread: invalid frame, sending REJ\n");
        sendREJ(seqNum);
        return -1;
    }

    sendRR(seqNum);
    return payloadSize;
}


int readFrame(unsigned char *frame, int maxSize)
{
    unsigned char byte;
    int i = 0;
    int start = 0;

    while (alarmEnabled == 1) {  // read only when alarm is enabled
        int res = readByteSerialPort(&byte);
        if (res <= 0) continue;

        if (byte == FLAG) {
            if (start == 0) {
                start = 1;
                if (i < maxSize) frame[i++] = byte;
                else return -1; // overflow
            } else {
                if (i < maxSize) frame[i++] = byte;
                else return -1; // overflow
                break;
            }
        } else if (start) {
            if (i < maxSize) frame[i++] = byte;
            else return -1; // overflow
        }
    }

    // When the alarm expired and didn´t received the full frame
    if (alarmEnabled == 0 && i == 0) {
        printf("readFrame: timeout (no data)\n");
        return -1;
    }

    return i;
}

int validateIFrame(const unsigned char *frame, int frameSize, unsigned char *packet, int *sequenceNumber, int maxPacketSize){
    if(frameSize < 6) return -1;

    unsigned char A = frame[1];
    unsigned char C = frame[2];
    unsigned char BCC1 = frame[3];

    *sequenceNumber = (C >> 6) & 0x01;

    if((A ^ C) != BCC1){
        printf("llread: BCC1 error\n");
        return -1;
    }

    //Remove FLAGS, header, trailer
    int dataSize = frameSize - 5;

    if (dataSize <= 0) {
        return -1;
    }

    unsigned char *destuffed = malloc(dataSize);
    if (!destuffed) return -1;
    int destuffedSize = destuffing(frame + 4, dataSize, destuffed, FRAME_MAX_SIZE);

    if (destuffedSize <= 0) {
        free(destuffed);
        printf("llread: destuffing failed\n");
        return -1;
    }

    unsigned char receivedBCC2 = destuffed[destuffedSize - 1];
    unsigned char calculatedBCC2 = calculateBCC2(destuffed, destuffedSize - 1);

    if(receivedBCC2 != calculatedBCC2){
        printf("llread: BCC2 error\n");
        free(destuffed);
        return -1;
    }

    memcpy(packet, destuffed, destuffedSize - 1);
    free(destuffed);
    return destuffedSize - 1;
}

int destuffing(const unsigned char *input, int inputSize, unsigned char *output, int maxOutput){
    int Index = 0;

    for(int i = 0; i < inputSize; i++){
        if(input[i] == ESC){
            if (i + 1 >= inputSize) {
                return -1;
            }
            if(input[i+1] == FLAG_ESC){
                output[Index++] = FLAG;
            }
            else if(input[i+1] == ESC_ESC){
                output[Index++] = ESC;
            }
            else {
                return -1;
            }
            i++;
        }
        else{
            if (Index >= maxOutput) return -1;
            output[Index++] = input[i];
        }
    }
    return Index;
}

void sendRR(int sequenceNumber){
    unsigned char RR[5] = {FLAG, A_RX, 0x00, 0x00, FLAG};
    if(sequenceNumber == 0){
        RR[2] = C_RR1;
    }
    else{
        RR[2] = C_RR0;
    }
    RR[3] = RR[1] ^ RR[2];
    writeBytesSerialPort(RR, 5);
    printf("RR sent (seq=%d)\n", sequenceNumber);
}

void sendREJ(int sequenceNumber){
    unsigned char REJ[5] = {FLAG, A_RX, 0x00, 0x00, FLAG};
    if(sequenceNumber == 0){
        REJ[2] = C_REJ1;
    }
    else{
        REJ[2] = C_REJ0;
    }
    REJ[3] = REJ[1] ^ REJ[2];
    writeBytesSerialPort(REJ, 5);
    printf("REJ sent (seq=%d)\n", sequenceNumber);
}

//_____________________________________________________________________________________________________________________
////////////////////////////////////////////////
/////////         TX_LLCLOSE             ///////
////////////////////////////////////////////////
int transmissorLLclose()
{
    unsigned char DISCONNECT[5] = {FLAG, A_TX, DISC, 0x00, FLAG};
    DISCONNECT[3] = DISCONNECT[1] ^ DISCONNECT[2];

    unsigned char byte;
    int tries = 0;

    while (tries < currentParams.nRetransmissions) {
        if (writeBytesSerialPort(DISCONNECT, 5) != 5) {
            tries++;
            continue;
        }
        printf("DISC SENT\n");

        alarmEnabled = 1;
        alarm(currentParams.timeout);

        while (alarmEnabled == 1) {
            int res = readByteSerialPort(&byte);
            if (res > 0) {
                int state = state_machine(byte);
                if (state == 3) {
                    printf("DISC RECEIVED\n");

                    unsigned char UA[5] = {FLAG, A_RX, C_UA, 0x00, FLAG};
                    UA[3] = UA[1] ^ UA[2];
                    writeBytesSerialPort(UA, 5);
                    printf("UA SENT\n");

                    alarm(0);
                    closeSerialPort();
                    printf("Connection terminated by transmitter.\n");
                    return 0;
                }
            }
        }

        tries++;
        printf("Timeout - tries: %d\n", tries);
    }

    printf("Failed to close connection (Tx)\n");
    closeSerialPort();
    return -1;
}

////////////////////////////////////////////////
/////////         RX_LLCLOSE             ///////
////////////////////////////////////////////////
int receptorLLclose()
{
    unsigned char byte;
    int tries = 0;

    // Wait for DISC for Tx
    while (tries < currentParams.nRetransmissions) {
        alarmEnabled = 1;
        alarm(currentParams.timeout);

        while (alarmEnabled == 1) {
            int res = readByteSerialPort(&byte);
            if (res > 0) {
                int state = state_machine(byte);
                if (state == 3) { // DISC received
                    printf("DISC RECEIVED\n");
                    alarm(0);
                    alarmEnabled = 0;

                    // Send DISC back
                    unsigned char DISCONNECT[5] = {FLAG, A_RX, DISC, 0x00, FLAG};
                    DISCONNECT[3] = DISCONNECT[1] ^ DISCONNECT[2];
                    writeBytesSerialPort(DISCONNECT, 5);
                    printf("DISC SENT\n");

                    // Wait for UA
                    int uaTries = 0;
                    while (uaTries < currentParams.nRetransmissions) {
                        alarmEnabled = 1;
                        alarm(currentParams.timeout);

                        while (alarmEnabled == 1) {
                            res = readByteSerialPort(&byte);
                            if (res > 0) {
                                int stateUA = state_machine(byte);
                                if (stateUA == 2) { // UA received
                                    printf("UA RECEIVED\n");
                                    alarm(0);
                                    closeSerialPort();
                                    printf("Connection terminated by receiver.\n");
                                    return 0;
                                }
                            }
                        }
                        uaTries++;
                        printf("Timeout waiting for UA - tries: %d\n", uaTries);
                    }

                    printf("Failed to receive UA after DISC exchange\n");
                    closeSerialPort();
                    return -1;
                }
            }
        }

        tries++;
        printf("Timeout waiting for DISC - tries: %d\n", tries);
    }

    printf("Receiver failed to close connection after %d tries\n", currentParams.nRetransmissions);
    closeSerialPort();
    return -1;
}

////////////////////////////////////////////////
/////////            LLCLOSE             ///////
////////////////////////////////////////////////
int llclose()
{
    if (linkFd < 0) return -1;
    if (currentParams.role == LlTx) {
        return transmissorLLclose();
    } else {
        return receptorLLclose();
    }
}

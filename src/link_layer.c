// Link layer protocol implementation

#include "../include/link_layer.h"
#include "../include/serial_port.h"
#include "../include/state_machine.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

extern int alarmEnabled;
extern int alarmCount;
extern void alarmHandler(int sig);

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
        TxRx = transmissorLLopen(connectionParameters, fd);
    }
    else{
        TxRx = receptorLLopen(connectionParameters, fd);
    }

    return TxRx;
}

////////////////////////////////////////////////
/////////            TX_LLOPEN           ///////
////////////////////////////////////////////////
int transmissorLLopen(LinkLayer connectionParameters, int fd){

    unsigned char SET[5] = {FLAG, A_TX, C_SET, 0x00, FLAG};
    SET[3] = SET[1] ^ SET[2]; //BCC = A xor C

    unsigned char byte;
    int tries = 0;
    
    while(tries < connectionParameters.nRetransmissions){
        if(writeBytesSerialPort(SET, 5) != 5){
            continue;
        }
        printf("SET SENT\n");

        alarmEnabled = 1;
        alarm(connectionParameters.timeout);

        while(alarmEnabled == 1){
            int res = readByteSerialPort(&byte);
            if(res > 0){
                int state = state_machine(fd);
                if(state == 2){
                    printf("UA recebido\n");
                    alarm(0);
                    return fd;
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
int receptorLLopen(LinkLayer connectionParameters, int fd){
    
    unsigned char byte;
    
    while(1){
        int res = readByteSerialPort(&byte);
        if(res > 0){
            int state = state_machine(fd);
            if(state == 1){
                printf("SET RECEIVED\n");
                break;
            }
        }
    }

    unsigned char UA[5] = {FLAG, A_RX, C_UA, 0x00, FLAG};
    UA[3] = UA[1] ^ UA[2]; //BCC = A xor C

    writeBytesSerialPort(UA, 5);
    printf("UA SENT\n");

    return fd;
}

////////////////////////////////////////////////
/////////           LLWRITE              ///////
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{

    return 0;
}

////////////////////////////////////////////////
/////////            LLREAD              ///////
////////////////////////////////////////////////
int llread(unsigned char *packet)
{

    return 0;
}

////////////////////////////////////////////////
/////////            LLCLOSE             ///////
////////////////////////////////////////////////
int llclose(LinkLayer connectionParameters, int fd)
{
    if (connectionParameters.role == LlTx) {
        return transmissorLLclose(connectionParameters, fd);
    } else {
        return receptorLLclose(connectionParameters, fd);
    }
}

////////////////////////////////////////////////
/////////         TX_LLCLOSE             ///////
////////////////////////////////////////////////
int transmissorLLclose(LinkLayer connectionParameters, int fd)
{
    unsigned char DISCONNECT[5] = {FLAG, A_TX, DISC, 0x00, FLAG};
    DISCONNECT[3] = DISCONNECT[1] ^ DISCONNECT[2];

    unsigned char byte;
    int tries = 0;

    while (tries < connectionParameters.nRetransmissions) {
        if (writeBytesSerialPort(DISCONNECT, 5) != 5) {
            continue;
        }
        printf("DISC SENT\n");

        alarmEnabled = 1;
        alarm(connectionParameters.timeout);

        while (alarmEnabled == 1) {
            int res = readByteSerialPort(&byte);
            if (res > 0) {
                int state = state_machine(fd);
                if (state == 3) {
                    printf("DISC RECEIVED\n");

                    unsigned char UA[5] = {FLAG, A_TX, C_UA, 0x00, FLAG};
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
int receptorLLclose(LinkLayer connectionParameters, int fd)
{
    unsigned char byte;

    while (1) {
        int res = readByteSerialPort(&byte);
        if (res > 0) {
            int state = state_machine(fd);
            if (state == 3) {
                printf("DISC RECEIVED\n");
                break;
            }
        }
    }

    unsigned char DISCONNECT[5] = {FLAG, A_RX, DISC, 0x00, FLAG};
    DISCONNECT[3] = DISCONNECT[1] ^ DISCONNECT[2];
    writeBytesSerialPort(DISCONNECT, 5);
    printf("DISC SENT\n");

    while (1) {
        int res = readByteSerialPort(&byte);
        if (res > 0) {
            int state = state_machine(fd);
            if (state == 2) {
                printf("UA RECEIVED\n");
                closeSerialPort();
                printf("Connection terminated by receiver.\n");
                return 0;
            }
        }
    }

    return -1;
}
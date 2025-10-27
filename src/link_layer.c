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
        perror("Erro ao abrir porta série");
        return -1;
    }

    struct sigaction act;
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
int llclose()
{

    return 0;
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
        printf("Timeout - trie %d\n", tries);
    }

    printf("Fail establishing connection\n");
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

    return;
}
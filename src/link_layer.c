// Link layer protocol implementation

#include "../include/link_layer.h"
#include "../include/serial_port.h"
#include "../include/state_machine.h"

#include <signal.h>
#include <stdio.h>
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

    closeSerialPort();
    return -1;
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

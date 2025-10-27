#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/state_machine.h"
#include "../include/link_layer.h"

int state_machine(int file)
{
    State state = START;
    unsigned char byte, A = 0, C = 0, BCC = 0;

    while (state != STOP)
    {
        int t = read(file, &byte, 1);
        if (t <= 0)
            return 0; // timeout ou erro

        switch (state)
        {
        case START:
            if (byte == FLAG)
                state = FLAG_RCV;
            break;

        case FLAG_RCV:
            if (byte == FLAG)
                state = FLAG_RCV; // FLAG repetido, continua
            else if (byte == A_TX || byte == A_RX)
            {
                A = byte;
                state = A_RCV;
            }
            else
                state = START;
            break;

        case A_RCV:
            if (byte == FLAG)
                state = FLAG_RCV;
            else if (byte == C_SET || byte == C_UA || byte == DISC)
            {
                C = byte;
                state = C_RCV;
            }
            else
                state = START;
            break;

        case C_RCV:
            BCC = A ^ C;
            if (byte == FLAG)
                state = FLAG_RCV;
            else if (byte == BCC)
                state = BCC_OK;
            else
                state = START;
            break;

        case BCC_OK:
            if (byte == FLAG)
                state = STOP;
            else
                state = START;
            break;

        default:
            state = START;
            break;
        }
    }

    switch (C){
        case C_SET:
            return 1;
        case C_UA:
            return 2;
        case DISC:
            return 3;
        default:
            return 0;   //erro
    }
}

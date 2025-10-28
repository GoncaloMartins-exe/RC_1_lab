#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/state_machine.h"
#include "../include/link_layer.h"

State currentState = START; //global state
unsigned char A = 0, C = 0, BCC = 0;

void reset_state_machine() {
    currentState = START;
}

int state_machine(unsigned char byte)
{
    switch (currentState){
        case START:
            if (byte == FLAG)
                currentState = FLAG_RCV;
            break;

        case FLAG_RCV:
            if (byte == FLAG)
                currentState = FLAG_RCV;
            else if (byte == A_TX || byte == A_RX)
            {
                A = byte;
                currentState = A_RCV;
            }
            else
                currentState = START;
            break;

        case A_RCV:
            if (byte == FLAG)
                currentState = FLAG_RCV;
            else if (byte == C_SET || byte == C_UA || byte == DISC)
            {
                C = byte;
                currentState = C_RCV;
            }
            else
                currentState = START;
            break;

        case C_RCV:
            BCC = A ^ C;
            if (byte == FLAG)
                currentState = FLAG_RCV;
            else if (byte == BCC)
                currentState = BCC_OK;
            else
                currentState = START;
            break;

        case BCC_OK:
            if (byte == FLAG)
                currentState = STOP;
            else
                currentState = START;
            break;

        default:
            currentState = START;
            break;
    }

    if (currentState == STOP){
        currentState = START;
        switch (C)
        {
        case C_SET:
            return 1;
        case C_UA:
            return 2;
        case DISC:
            return 3;
        default:
            return 0;
        }
    }

    return 0;
}

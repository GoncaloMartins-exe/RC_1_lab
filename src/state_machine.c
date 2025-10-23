#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/state_machine.h"

int state_machine(int file){
    enum State state = START;
    unsigned char byte, A, C, BCC;
    while(state != STOP){
        int t = read(file, &byte, 1);
        if(t <= 0){
            return 0;   // error by timeout which means it was not received
        }
        switch (state){
            case START:
                if(byte == 0x7E){
                    state = FLAG_RCV;
                }
            case FLAG_RCV:
                if(byte == 0x7E){
                    state = FLAG_RCV;
                }
                else{
                    A = byte;
                    state = A_RCV; 
                }
                break;
            case A_RCV:
                if(byte == 0x7E){
                    state = FLAG_RCV;
                }
                else{
                    C = byte;
                    state = C_RCV; 
                }
            case C_RCV:
                BCC = A ^ C;
                if(byte == 0x7E){
                    state = FLAG_RCV;
                }
                else if(byte == BCC){
                    state = BCC_OK;
                }
                else state = START;
                break;
            case BCC_OK:
                if(byte == 0x7E){
                    state = STOP;
                }
                else{
                    state = START;
                }
                break;
            default:
                state = START;
        }
    }

    if(C == 0x03){
        return 1;
    }
    if(C == 0x07){
        return 2;
    }
    return 0;
}
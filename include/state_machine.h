#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP} State;

typedef struct {
    State state;
    unsigned char expectedAddress;
    unsigned char expectedControl;
    unsigned char receivedAddress;
    unsigned char receivedControl;
    unsigned char receivedBCC;
} StateMachine;

void reset_state_machine();

int state_machine(unsigned char byte);

#endif
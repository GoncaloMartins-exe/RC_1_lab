#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

enum State {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP};

int state_machine(int file);

#endif
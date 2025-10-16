// Alarm example using sigaction.
// This example shows how to configure an alarm using the sigaction function.
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]
//              Rui Prior [rcprior@fc.up.pt]

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define FALSE 0
#define TRUE 1
#define MAX_RETRANSMISSIONS 3
#define TIMEOUT 3

volatile sig_atomic_t alarmEnabled = FALSE;
volatile sig_atomic_t alarmCount = 0;
volatile sig_atomic_t uaReceived = FALSE;

void sendSET();
int receiveUA();
void alarmHandler(int signal);

//State Machine

enum State {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP};

int receiveSET_UA(int file){
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



// Alarm function handler.
// This function will run whenever the signal SIGALRM is received.
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    
    printf("Alarm #%d received\n", alarmCount);
}

int main()
{
    // Set alarm function handler.
    // Install the function signal to be automatically invoked when the timer expires,
    // invoking in its turn the user function alarmHandler
    struct sigaction act = {0};
    act.sa_handler = &alarmHandler;
    if (sigaction(SIGALRM, &act, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    printf("Alarm configured\n");

    while (alarmCount < MAX_RETRANSMISSIONS && !uaReceived)
    {
        if (alarmEnabled == FALSE)
        {
            sendSET();
            alarm(TIMEOUT);
            alarmEnabled = TRUE;
        }

        sleep(0.1);
        uaReceived = receiveUA();

        if(uaReceived){
            alarm(0);
            printf("UA frame received, connection established\n");
            break;
        }
    }

    if(!uaReceived){
        printf("No UA after %d retransmissions. Connection failed.\n");
    }

    printf("Ending program\n");

    return 0;
}

void sendSET(){
    printf("-> Sending SET frame\n");
}

int receiveUA(){
    if(alarmCount == 1){
        printf("<- UA frame received\n");
        return TRUE;
    }
    return FALSE;
}
#include <signal.h>
#include <stdio.h>

int alarmEnabled = 0;
int alarmCount = 0;

void alarmHandler(int sig)
{
    alarmEnabled = 0;
    alarmCount++;
    printf("Alarm #%d received\n", alarmCount);
}

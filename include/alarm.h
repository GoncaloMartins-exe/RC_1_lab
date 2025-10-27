#ifndef _ALARM_H_
#define _ALARM_H_

extern int alarmEnabled;
extern int alarmCount;

void alarmHandler(int sig);

#endif

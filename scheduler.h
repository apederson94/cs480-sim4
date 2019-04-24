#include "dataStructures.h"

#ifndef SCHEDULER
#define SCHEDULER

enum
{
    NO_APPS_READY = -999
};

int scheduleNext(struct PCB **pcbList, char *scheduler, int numProcesses, double *interrupts);
int checkAllDone(struct PCB **pcbList, int numProcesses);

#endif
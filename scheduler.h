#include "dataStructures.h"
#include "booleans.h"

#ifndef SCHEDULER
#define SCHEDULER

enum
{
    NO_APPS_READY = -999,
    APP_NOT_READY = -255
};

int scheduleNext(struct PCB **pcbList, int schedCode, int numProcesses, double *interrupts);

int checkAllDone(struct PCB **pcbList, int numProcesses);

bool isPreemptive(int schedCode);

#endif
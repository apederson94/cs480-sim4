#include "dataStructures.h"
#include "booleans.h"

#ifndef SCHEDULER
#define SCHEDULER

//error codes pertaining to the scheduler
enum
{
    NO_APPS_READY = -999,
    APP_NOT_READY = -255
};

//returns the PID of the next process to be run
int scheduleNext(struct PCB **pcbList, int schedCode, int numProcesses, double *interrupts, bool freeQueue);

//checks to see if all processes are done running
int checkAllDone(struct PCB **pcbList, int numProcesses);

//returns TRUE if the scheduler code is preemptive
bool isPreemptive(int schedCode);

#endif
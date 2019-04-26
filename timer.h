#include <sys/time.h>
#include "dataStructures.h"

#define USEC_PER_SEC 1000000
#define MS_PER_SEC 1000
#define USEC_PER_MS 1000

struct runForArgs
{
    struct timeval runtime;
    struct PCB **pcbList;
    double *interrupts;
    char cmdLtr;
    int pid;
    double cpuCycleTime;
    int elapsedCycles;
    int quantum;
    char *cpuSched;
    int numApps;
    long int cyclesToRun;
    int schedCode;
};

struct timerArgs
{
    struct timeval runtime;
    double *interrupts;
    double cpuCycleTime;
    int elapsedCycles;
    int quantum;
    int numApps;
    int pid;
    long int cyclesToRun;
};

//returns time of execution local to program
struct timeval
execTime(struct timeval start);

//converts timeval to double
double tv2double(struct timeval tv);

//creates a threaded timer
void *threadTimer(void *args);

//creates a threaded timer for a run command
void *threadTimerRun(void *args);

//runs a program for an amount of time
void *runFor(void *args);

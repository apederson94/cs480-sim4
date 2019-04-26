#include <sys/time.h>

#ifndef INTERRUPTS
#define INTERRUPTS

//defines all structs and functions tied to interrupts

//error codes regarding to interrupts
enum
{
    NO_INTERRUPTS = -900,
    WAS_INTERRUPTED = -480
};

//checks for interrupts and returns process id for earliest interrupt
int checkForInterrupt(double *interrupts, int numApps);

#endif
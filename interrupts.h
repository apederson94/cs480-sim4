#include <sys/time.h>

#ifndef INTERRUPTS
#define INTERRUPTS

//defines all structs and functions tied to interrupts
enum
{
    NO_INTERRUPTS = -900
};

//checks for interrupts
int checkForInterrupt(double *interrupts, int numApps);

#endif
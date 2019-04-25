#include <sys/time.h>
#include <stdio.h>
#include <math.h>
#include "interrupts.h"

/*
    * checks for any interrupts:
       * iterates over interrupts array and finds interrupts
       * selects interrupt that happened first
       * returns selected interrupt's corresponding process number
*/
int checkForInterrupt(double *interrupts, int numApps)
{
    int intIter, soonest;
    double soonestTime, currTime;
    soonest = NO_INTERRUPTS;
    soonestTime = INFINITY;

    for (intIter = 0; intIter < numApps; intIter++)
    {
        currTime = interrupts[intIter];

        if (currTime == WAS_INTERRUPTED)
        {
            return intIter;
        }

        if (currTime != 0.0 && currTime <= soonestTime)
        {
            soonest = intIter;
            soonestTime = currTime;
        }
    }

    return soonest;
}

//TODO: FIX ERROR WITH NEXT INSTRUCTION BEING EXECUTED AFTER INTERRUPT
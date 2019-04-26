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

    //iterates over all PCBs
    for (intIter = 0; intIter < numApps; intIter++)
    {
        //time to compare
        currTime = interrupts[intIter];

        //checks to see if error code is present that this application was interrupted
        if (currTime == WAS_INTERRUPTED)
        {
            return intIter;
        }

        //ensures time is not actually an error code and checks that it is earlier than current earliest
        if (currTime > 0.0 && currTime <= soonestTime)
        {
            soonest = intIter;
            soonestTime = currTime;
        }
    }

    //returns earliest interrupt's process ID
    return soonest;
}
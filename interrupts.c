#include <sys/time.h>
#include <stdio.h>
#include <math.h>
#include "interrupts.h"


int checkForInterrupt(double *interrupts, int numApps)
{
    int intIter, soonest;
    double soonestTime, currTime;
    soonest = NO_INTERRUPTS;
    soonestTime = INFINITY;

    for (intIter = 0; intIter < numApps; intIter++)
    {
        currTime = interrupts[intIter];

        if(currTime > 0.0)
        {
            printf("%d CURRENT TIME: %lf\n", intIter, currTime);
        }

        if (currTime != 0.0 && currTime <= soonestTime)
        {
            soonest = intIter;
            soonestTime = currTime;
        }
    }

    return soonest;
}

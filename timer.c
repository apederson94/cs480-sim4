#include "timer.h"
#include "dataStructures.h"
#include "interrupts.h"
#include "booleans.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

/*
    gets execution time of this function:
        * creates a timeval struct to return
        * checks timeval difference to ensure correctness
        * returns correct timeval
*/
struct timeval execTime(struct timeval start)
{
    struct timeval end;
    double secDiff;
    double usecDiff;

    //gets current time
    gettimeofday(&end, NULL);

    //subtracts seconds and useconds
    secDiff = end.tv_sec - start.tv_sec;
    usecDiff = end.tv_usec - start.tv_usec;

    //if the useconds are negative, add 1 second in USEC to usecDiff and decrement secDiff
    if (usecDiff < 0)
    {
        usecDiff += USEC_PER_SEC;
        secDiff--;
    }

    //send end values to secDiff and usecDiff
    end.tv_sec = secDiff;
    end.tv_usec = usecDiff;

    return end;
}

/*
    creates a pthread that runs a timer for timeval amount of time
*/
void *runFor(void *arguments)
{
    pthread_t threadId;
    struct runForArgs args = *((struct runForArgs *)arguments);
    int pid = args.pid;
    void *cyclesRun;
    struct timeval runtime = args.runtime;
    struct timeval currTime;
    char cmdLtr = args.cmdLtr;
    static struct timerArgs *targs;
    static bool notSet = TRUE;

    if (notSet)
    {
        targs = (struct timerArgs *)calloc(args.numApps, sizeof(struct timerArgs));
        notSet = FALSE;
    }

    targs[pid].runtime = runtime;
    targs[pid].cpuCycleTime = args.cpuCycleTime;
    targs[pid].elapsedCycles = args.elapsedCycles;
    targs[pid].numApps = args.numApps;
    targs[pid].quantum = args.quantum;
    targs[pid].interrupts = args.interrupts;

    //pass in time to run for, and clock start time for program
    if (args.cmdLtr == 'P')
    {
        pthread_create(&threadId, NULL, threadTimerRun, &targs[pid]);
        pthread_join(threadId, &cyclesRun);

        return cyclesRun;
    }
    else
    {
        pthread_create(&threadId, NULL, threadTimer, &targs[pid]);
        pthread_join(threadId, NULL);
        gettimeofday(&currTime, NULL);
        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ INTERRUPT SENT %d @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n", pid);
        args.interrupts[pid] = tv2double(currTime);

        return NULL;
    }
}

/*
    function for thread timer:
        * gets times for start and current
        * checks difference between those times:
            * when time difference is equal to the runtime values, return
*/
void *threadTimerRun(void *args)
{

    //convert argument to correct type
    struct timerArgs targs = *((struct timerArgs *)args);
    struct timeval time;
    struct timeval start;
    struct timeval diff;
    int secDiff, usecDiff, cyclesRun;
    int cyclesToRun = tv2double(targs.runtime) / (targs.cpuCycleTime / MS_PER_SEC);

    cyclesRun = 0;

    //gets start time and current time
    gettimeofday(&start, NULL);
    gettimeofday(&time, NULL);

    //calculate the difference between start and current time
    secDiff = time.tv_sec - start.tv_sec;
    usecDiff = time.tv_sec - start.tv_sec;
    diff.tv_sec = secDiff;
    diff.tv_usec = usecDiff;

    //while seconds or useconds are less than runtime values, keep running
    while (cyclesRun + targs.elapsedCycles < targs.quantum && cyclesRun < cyclesToRun)
    {
        while (tv2double(diff) < (targs.cpuCycleTime / MS_PER_SEC))
        {
            //gets the current time and then updates secDiff and usecDiff
            gettimeofday(&time, NULL);
            secDiff = time.tv_sec - start.tv_sec;
            usecDiff = time.tv_usec - start.tv_usec;
            diff.tv_sec = secDiff;
            diff.tv_usec = usecDiff;

            //if usecDiff is negative, add USEC_PER_SEC to it and decrement secDiff
            if (usecDiff < 0)
            {
                usecDiff += USEC_PER_SEC;
                secDiff--;
            }
        }
        gettimeofday(&start, NULL);

        diff.tv_sec = 0;
        diff.tv_usec = 0;
        cyclesRun += 1;

        if (checkForInterrupt(targs.interrupts, targs.numApps) >= 0)
        {
            printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ INTERRUPTED BY %d @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n", checkForInterrupt(targs.interrupts, targs.numApps));
            return (void *)cyclesRun;
        }
        //TODO: MAKE P(RUN) ON INTERRUPT WORK BETTER
    }

    return (void *)cyclesRun;
}

/*
    function for thread timer:
        * gets times for start and current
        * checks difference between those times:
            * when time difference is equal to the runtime values, return
*/
void *threadTimer(void *args)
{

    //convert argument to correct type
    struct timerArgs targs = *((struct timerArgs *)args);
    struct timeval runtime = targs.runtime;
    struct timeval time;
    struct timeval start;
    int secDiff, usecDiff;

    //gets start time and current time
    gettimeofday(&start, NULL);
    gettimeofday(&time, NULL);

    //calculate the difference between start and current time
    secDiff = time.tv_sec - start.tv_sec;
    usecDiff = time.tv_sec - start.tv_sec;

        //while seconds or useconds are less than runtime values, keep running
        while (secDiff < runtime.tv_sec || usecDiff < runtime.tv_usec)
    {

        //gets the current time and then updates secDiff and usecDiff
        gettimeofday(&time, NULL);
        secDiff = time.tv_sec - start.tv_sec;
        usecDiff = time.tv_usec - start.tv_usec;

            //if usecDiff is negative, add USEC_PER_SEC to it and decrement secDiff
            if (usecDiff < 0)
        {
            usecDiff += USEC_PER_SEC;
            secDiff--;
        }
    }

    return NULL;
}

//converts a timeval to double and returns it
double tv2double(struct timeval tv)
{
    return ((double)tv.tv_sec) + (((double)tv.tv_usec) / USEC_PER_SEC);
}
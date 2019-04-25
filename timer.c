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
<<<<<<< HEAD
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
    }
    else
    {
        pthread_create(&threadId, NULL, threadTimer, &targs[pid]);
    }
=======
    struct timerArgs targs;

    targs.runtime = runtime;
    targs.cpuCycleTime = args.cpuCycleTime;

    //pass in time to run for, and clock start time for program
    pthread_create(&threadId, NULL, threadTimer, &targs);
>>>>>>> b702d9cbce78e7e6f545ec4ef94b2432b4c07c70

    //wait for thread to return before deciding whether to cause an interrupt
    if (cmdLtr == 'P')
    {
        pthread_join(threadId, &cyclesRun);

        return cyclesRun;
    }
    else
    {
        pthread_join(threadId, NULL);
        gettimeofday(&currTime, NULL);
        args.interrupts[pid] = tv2double(currTime);
<<<<<<< HEAD

        return NULL;
=======
>>>>>>> b702d9cbce78e7e6f545ec4ef94b2432b4c07c70
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
    while (cyclesRun + targs.elapsedCycles < targs.quantum)
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

        if (checkForInterrupt(targs.interrupts, targs.numApps))
        {
            break;
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

<<<<<<< HEAD
    //convert argument to correct type
    struct timerArgs targs = *((struct timerArgs *)args);
=======
    //convert argument to correct type 
    struct targs = *((struct timerArgs*) args);
>>>>>>> b702d9cbce78e7e6f545ec4ef94b2432b4c07c70
    struct timeval runtime = targs.runtime;
    struct timeval time;
    struct timeval start;
    struct timeval diff;
    int secDiff, usecDiff;

    //gets start time and current time
    gettimeofday(&start, NULL);
    gettimeofday(&time, NULL);

    //calculate the difference between start and current time
    secDiff = time.tv_sec - start.tv_sec;
    usecDiff = time.tv_sec - start.tv_sec;
    diff.tv_sec = secDiff
    diff.tv_usec = usecDiff

    //while seconds or useconds are less than runtime values, keep running
<<<<<<< HEAD
    while (secDiff < runtime.tv_sec || usecDiff < runtime.tv_usec)
=======
    while(secDiff < runtime.tv_sec || usecDiff < runtime.tv_usec)
>>>>>>> b702d9cbce78e7e6f545ec4ef94b2432b4c07c70
    {
        //TODO: MAKE P(RUN) ON INTERRUPT WORK BETTER
        if (tv2double(diff) >= targs.cpuCycleTime)
        {
            if (checkForInterrupts(targs.interrupts) >= 0)
            {
                return elapsedCycles;
            }
        }

        //gets the current time and then updates secDiff and usecDiff
        gettimeofday(&time, NULL);
        secDiff = time.tv_sec - start.tv_sec;
        usecDiff = time.tv_usec - start.tv_usec;
        diff.tv_sec = secDiff
        diff.tv_usec = usecDiff

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
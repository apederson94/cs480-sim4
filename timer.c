#include "timer.h"
#include "dataStructures.h"
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
        usecDiff +=  USEC_PER_SEC;
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
void runFor(void *arguments)
{
    pthread_t threadId; 
    struct runForArgs *args = (struct runForArgs*) arguments;
    int pid = args->pid;
    struct timeval runtime = args->runtime;
    struct timeval currTime;
    char cmdLtr = args->cmdLtr;

    //pass in time to run for, and clock start time for program
    pthread_create(&threadId, NULL, threadTimer, &runtime);

    //TODO: this may be getting subtracted twice when I run through again on I/O ops. I'll need to double check...

    //TODO: Process 3 is chosen way too frequently. What's up with that??

    //wait for thread to return
    pthread_join(threadId, NULL);

    if (cmdLtr == 'I' || cmdLtr == 'O')
    {
        //TODO: CHECK THE DOUBLE SELECTED PROCESS LOG OUTPUT
        //TODO: THESE VALUES AREN'T TRANSLATING TO THE OUTSIDE WORLD. THAT'S WHERE THE DISCONNECT IS HAPPENING, I THINK
        gettimeofday(&currTime, NULL);
        *(args->interrupts[pid]) = tv2double(currTime);
        printf("PID: %d\n", pid);
        args->pcbList[pid]->state = READY_STATE;
    }
}

/*
    function for thread timer:
        * gets times for start and current
        * checks difference between those times:
            * when time difference is equal to the runtime values, return
*/
void * threadTimer(void *args)
{

    //convert argument to correct type 
    struct timeval *runtime = (struct timeval*) args;
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
    while(secDiff < runtime->tv_sec || usecDiff < runtime->tv_usec)
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

    return 0;
}

//converts a timeval to double and returns it
double tv2double(struct timeval tv)
{
    return ((double)tv.tv_sec) + (((double)tv.tv_usec)/USEC_PER_SEC);
}
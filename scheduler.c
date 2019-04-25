#include "dataStructures.h"
#include "strUtils.h"
#include "errors.h"
#include "scheduler.h"
#include "interrupts.h"
#include "booleans.h"
#include <stdio.h>
#include <stdlib.h>

/*
    * basic scheduler that supports:
        * FCFS-N
        * SJF-N
        * FCFS-P
        * SRTF-P
        * RR-P
*/
int scheduleNext(struct PCB **pcbList, int schedCode, int numProcesses, double *interrupts)
{
    int pcbIter, nextJob, interrupt;
    static int tmp;
    static int *queue;
    static int queueIter = 0;

    if (queueIter == 0)
    {
        queue = (int*) calloc(numProcesses, sizeof(int));
        for (queueIter = 0; queueIter < numProcesses; queueIter++)
        {
            queue[queueIter] = queueIter;
        }
    }

    nextJob = NO_APPS_READY;

    //logic for dealing with "FCFS-N" scheduling type
    if (schedCode == FCFS_N)
    {

        //iterates PCB array
        for (pcbIter = 0; pcbIter < numProcesses; pcbIter++)
        {

            //if time remaining isn't 0, select the first process in the array
            if (pcbList[pcbIter]->timeRemaining != 0 
            && (pcbList[pcbIter]-> state == READY_STATE || pcbList[pcbIter]-> state == RUNNING_STATE))
            { 
                nextJob = pcbList[pcbIter]->processNum;
                return nextJob;
            }
        }
    }

    //logic for dealing with "SJF-N" scheduling type
    else if (schedCode == SJF_N)
    {

        //iterates PCB array
        for (pcbIter = 0; pcbIter < numProcesses; pcbIter++)
        {

            //if time remaining isn't 0, select the shortest job
            if (pcbList[pcbIter]->timeRemaining != 0)
            {
                if (nextJob == NO_APPS_READY
                && (pcbList[pcbIter]->state == READY_STATE || pcbList[pcbIter]->state == RUNNING_STATE))
                {
                    nextJob = pcbIter;
                }
                else if (nextJob != NO_APPS_READY && (pcbList[nextJob]->timeRemaining > pcbList[pcbIter]->timeRemaining))
                {
                    nextJob = pcbIter;
                }
            }
        }
    }

    //logic for dealing with "SJF-P" scheduling type
    else if (schedCode == FCFS_P)
    {

        //process number of interrupt or error code
        interrupt = checkForInterrupt(interrupts, numProcesses);

        //error codes are less than zero so this will only ever return a process number
        if (interrupt >= 0)
        {
            if (interrupts[interrupt] == WAS_INTERRUPTED)
            {
                interrupts[interrupt] = 0.0;
            }
            return interrupt;
        }

        //iterates PCB array
        for (pcbIter = 0; pcbIter < numProcesses; pcbIter++)
        {

            //if time remaining isn't 0, select the first process in the array
            if (pcbList[pcbIter]->timeRemaining != 0 
            && (pcbList[pcbIter]-> state == READY_STATE || pcbList[pcbIter]-> state == RUNNING_STATE))
            { 
                nextJob = pcbList[pcbIter]->processNum;
                return nextJob;
            }
        }
    }

    //logic for dealing with "SRTF-P" scheduling type
    else if (schedCode == SRTF_P)
    {

        interrupt = checkForInterrupt(interrupts, numProcesses);

        if (interrupt >= 0)
        {
            return interrupt;
        }

        //iterates PCB array
        for (pcbIter = 0; pcbIter < numProcesses; pcbIter++)
        {

            //if time remaining isn't 0, select the shortest job
            if (pcbList[pcbIter]->timeRemaining != 0)
            {
                if (pcbList[pcbIter]->state == READY_STATE || pcbList[pcbIter]->state == RUNNING_STATE)
                {
                    if (nextJob == NO_APPS_READY)
                    {
                        nextJob = pcbIter;
                    }
                    else if (nextJob != NO_APPS_READY && (pcbList[nextJob]->timeRemaining > pcbList[pcbIter]->timeRemaining))
                    {
                        nextJob = pcbIter;
                    }
                }
            }
        }
    }

    //logic for dealing with "SJF-P" scheduling type
    else if (schedCode == RR_P)
    {

        //process number of interrupt or error code
        interrupt = checkForInterrupt(interrupts, numProcesses);

        //error codes are less than zero so this will only ever return a process number
        if (interrupt >= 0)
        {
            printf("HINT HINT %d, %lf\n", interrupt, interrupts[interrupt]);
            return interrupt;
        }

        //logic for returning next in the rr list
        /* figure out what to do if process is not ready yet...
        ask michael tomorrow basically
        wait for next avail or just keep looping and wait for the next one to be ready??
        while (pcbList[next]->timeRemaining == 0)
        {
            
        } */

        //iterates PCB array
        for (pcbIter = 0; pcbIter < numProcesses; pcbIter++)
        {

            if (pcbList[queue[pcbIter]]->timeRemaining != 0
            && (pcbList[queue[pcbIter]]->state == READY_STATE || pcbList[queue[pcbIter]]->state == RUNNING_STATE))
            {
                nextJob = pcbList[queue[pcbIter]]->processNum;
                
                for (queueIter = pcbIter; queueIter < numProcesses-1; queueIter++)
                {
                    tmp = queue[queueIter];
                    queue[queueIter] = queue[queueIter+1];
                    queue[queueIter+1] = tmp;
                }
                
                return nextJob;
            }
        }
    }

    if (checkAllDone(pcbList, numProcesses))
    {
        return ALL_PROGRAMS_DONE;
    }

    return nextJob;
}

/*
    * checks to see if all processes are done running:
        * checks to see if all processes have no time remaining
        * checks to see if all processes are set in the EXIT_STATE
*/
int checkAllDone(struct PCB **pcbList, int numProcesses)
{
    struct PCB *controlBlock;
    int iter;

    for (iter = 0; iter < numProcesses; iter++)
    {
        controlBlock = pcbList[iter];
        if (controlBlock->timeRemaining != 0 || controlBlock->state != EXIT_STATE)
        {
            return FALSE;
        }
    }

    return TRUE;
}

bool isPreemptive(int schedCode)
{
    return schedCode >= 2;
}
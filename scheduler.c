#include "dataStructures.h"
#include "strUtils.h"
#include "errors.h"
#include "scheduler.h"
#include "interrupts.h"
#include <stdio.h>

/*
    * basic scheduler that supports:
        * FCFS-N
        * SJF-N
*/
int scheduleNext(struct PCB **pcbList, char *scheduler, int numProcesses, double *interrupts[])
{
    int pcbIter, nextJob, interrupt;

    nextJob = NO_APPS_READY;

    //logic for dealing with "FCFS-N" scheduling type
    if (strCmp(scheduler, "FCFS-N"))
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

        //if no program has time remaining, return ALL_PROGRAMS_DONE code
        return ALL_PROGRAMS_DONE;
    }

    //logic for dealing with "SJF-N" scheduling type
    else if (strCmp(scheduler, "SJF-N"))
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

        return nextJob;
    }

    //logic for dealing with "SJF-P" scheduling type
    else if (strCmp(scheduler, "FCFS-P"))
    {

        interrupt = checkForInterrupt(interrupts, numProcesses);

        if (interrupt >= 0)
        {
            return interrupt;
        }

        //iterates PCB array
        for (pcbIter = 0; pcbIter < numProcesses; pcbIter++)
        {

            printf("%d STATE: %d\n", pcbIter, pcbList[pcbIter]->state);

            //if time remaining isn't 0, select the first process in the array
            if (pcbList[pcbIter]->timeRemaining != 0 
            && (pcbList[pcbIter]-> state == READY_STATE || pcbList[pcbIter]-> state == RUNNING_STATE))
            { 
                nextJob = pcbList[pcbIter]->processNum;
                return nextJob;
            }
        }
        
        return nextJob;
    }

    return SCHEDULER_NOT_AVAILABLE;
}
#include "dataStructures.h"
#include "strUtils.h"
#include "errors.h"
#include <stdio.h>

/*
    * basic scheduler that supports:
        * FCFS-N
        * SJF-N
*/
int scheduleNext(struct PCB **pcbList, char *scheduler, int numProcesses)
{
    int pcbIter, shortestJob;

    //logic for dealing with "FCFS-N" scheduling type
    if (strCmp(scheduler, "FCFS-N"))
    {

        //iterates PCB array
        for (pcbIter = 0; pcbIter < numProcesses; pcbIter++)
        {

            //if time remaining isn't 0, select the first process in the array
            if (pcbList[pcbIter]->timeRemaining != 0)
            {
                return pcbList[pcbIter]->processNum;
            }
        }

        //if no program has time remaining, return ALL_PROGRAMS_DONE code
        return ALL_PROGRAMS_DONE;
    }

    //logic for dealing with "SJF-N" scheduling type
    if (strCmp(scheduler, "SJF-N"))
    {

        shortestJob = ALL_PROGRAMS_DONE;

        //iterates PCB array
        for (pcbIter = 0; pcbIter < numProcesses; pcbIter++)
        {

            //if time remaining isn't 0, select the shortest job
            if (pcbList[pcbIter]->timeRemaining != 0)
            {
                if (shortestJob == ALL_PROGRAMS_DONE)
                {
                    shortestJob = pcbIter;
                }
                else if (pcbList[shortestJob]->timeRemaining > pcbList[pcbIter]->timeRemaining)
                {
                    shortestJob = pcbIter;
                }
            }
        }

        return shortestJob;
    }

    return SCHEDULER_NOT_AVAILABLE;
}
#include "dataStructures.h"
#include "timer.h"
#include "scheduler.h"
#include "booleans.h"
#include "strUtils.h"
#include "logger.h"
#include "errors.h"
#include "memoryManagement.h"
#include "interrupts.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

int simulate(struct simAction *actionsList, struct configValues *settings, struct logEvent *logList)
{
    int numApps, runningApp, timeSec, timeUsec, totalTime, cycleTime, memoryReturn, intIter;
    int memoryValues[3];
    char *type, *line;
    struct runForArgs args;
    struct PCB *controlBlock;
    struct MMU *mmu = calloc(1, sizeof(struct MMU));
    struct timeval startTime;
    struct timeval runtime;
    bool programsToRun = TRUE;
    bool logToMon = FALSE;
    bool logToFile = FALSE;

    //initializes MMU
    initializeMMU(mmu);

    //allocating memory for line
    line = calloc(100, sizeof(char));

    //settings flags for logging
    if (strCmp(settings->logTo, "Both"))
    {
        logToMon = TRUE;
        logToFile = TRUE;
    }
    else if (strCmp(settings->logTo, "Monitor"))
    {
        logToMon = TRUE;
    }
    else if (strCmp(settings->logTo, "File"))
    {
        logToFile = TRUE;
    }

    logIt("===============\nSimulator Start\n===============\n\n", logList, logToMon, logToFile);

    //getting start time of OS
    gettimeofday(&startTime, NULL);

    sprintf(line, "[%lf] OS: System Start\n", (double)0);
    logIt(line, logList, logToMon, logToFile);

    sprintf(line, "[%lf] OS: Creating Process Control Blocks\n", tv2double(execTime(startTime)));
    logIt(line, logList, logToMon, logToFile);

    //counting number of applications in actions list
    numApps = countApplications(actionsList);

    //allocating memory for PCB array
    struct PCB *pcbList[numApps];

    //allocating memory for interrupts array
    double *interrupts[numApps];

    //initializing interrupts array data
    for (intIter = 0; intIter < numApps; intIter++)
    {
        interrupts[intIter] = calloc(1, sizeof(double));
    }

    //initializing pthreads
    pthread_t threadIds[numApps];
    
    //creates PCB array
    createPCBList(pcbList, actionsList, settings);

    sprintf(line, "[%lf] OS: All processes initialized in \"new\" state\n", tv2double(execTime(startTime)));
    logIt(line, logList, logToMon, logToFile);

    //setting state to ready for all PCBs in PCB array
    setStatesReady(pcbList, numApps);

    sprintf(line, "[%lf] OS: All processes now set to state \"ready\"\n", tv2double(execTime(startTime)));
    logIt(line, logList, logToMon, logToFile);

    //schedules the next app to be run
    runningApp = scheduleNext(pcbList, settings->cpuSched, numApps, interrupts);

    pcbList[runningApp]->state = RUNNING_STATE;

    //error checking
    if (runningApp < 0)
    {
        return runningApp;
    }

    sprintf(line, "[%lf] OS: Selected process %d with %dms remaining\n\n", tv2double(execTime(startTime)), runningApp, pcbList[runningApp]->timeRemaining);
    logIt(line, logList, logToMon, logToFile);

    sprintf(line, "[%lf] OS: Process %d set in RUNNING state\n\n", tv2double(execTime(startTime)), runningApp);
    logIt(line, logList, logToMon, logToFile);

    while (programsToRun)
    {

        //sets current PCB and current program counter
        controlBlock = pcbList[runningApp];

        //iterates until A(end) occurs
        while (!strCmp(controlBlock->pc->operationString, "end") && controlBlock->timeRemaining != 0)
        {
            memoryReturn = 0;

            //if program counter's command letter is not 'A'
            if (controlBlock->pc->commandLetter != 'A')
            {

                switch (controlBlock->pc->commandLetter)
                {
                //if program counter's command letter is 'P
                case 'P':
                    cycleTime = settings->cpuCycleTime;
                    type = "operation";
                    break;
                //if program counter's command letter is 'I'
                case 'I':
                    cycleTime = settings->ioCycleTime;
                    type = "input";
                    break;
                //if program counter's command letter is 'O'
                case 'O':
                    cycleTime = settings->ioCycleTime;
                    type = "output";
                    break;
                //if program counter's command letter is 'M'
                case 'M':
                    cycleTime = settings->ioCycleTime;
                    type = "memory";
                    break;
                //should never be reached by this point in the program
                default:
                    break;
                }

                //creating time time values and setting timeval data
                totalTime = controlBlock->pc->assocVal * cycleTime;
                timeSec = totalTime / MS_PER_SEC;
                timeUsec = (totalTime % MS_PER_SEC) * USEC_PER_MS;
                runtime.tv_sec = timeSec;
                runtime.tv_usec = timeUsec;

                //memory operation logic
                if (controlBlock->pc->commandLetter == 'M')
                {

                    //puts SSBBBAAA values into memoryValues array (SS being 0, BBB being 1, AAA being 2)
                    stripMemoryValues(controlBlock->pc->assocVal, memoryValues);

                    //memory allocation logic
                    if (strCmp(controlBlock->pc->operationString, "allocate"))
                    {
                        sprintf(line, "[%lf] Process: %d, MMU attempt to allocate %d/%d/%d\n", tv2double(execTime(startTime)), controlBlock->processNum, memoryValues[0], memoryValues[1], memoryValues[2]);
                        logIt(line, logList, logToMon, logToFile);

                        //attempts to allocate memory
                        memoryReturn = allocate(mmu, memoryValues[0], memoryValues[1], memoryValues[2], settings->memoryAvailable, controlBlock);

                        //error handling for memory allocation
                        if (memoryReturn)
                        {
                            if (memoryReturn == MEMORY_ALREADY_ALLOCATED_ERROR)
                            {
                                sprintf(line, "[%lf] Process: %d, MMU failed to allocate. Memory already allocated. \n\n", tv2double(execTime(startTime)), controlBlock->processNum);
                            }
                            else if (memoryReturn == CANNOT_ALLOCATE_MEMORY_AMOUNT_ERROR)
                            {
                                sprintf(line, "[%lf] Process: %d, MMU failed to allocate. System memory full. \n\n", tv2double(execTime(startTime)), controlBlock->processNum);
                            }
                            else if (memoryReturn == DUPLICATE_MEMORY_ACCESS_ID_ERROR)
                            {
                                sprintf(line, "[%lf] Process: %d, MMU failed to allocate. Duplicate memory segment ID used. \n\n", tv2double(execTime(startTime)), controlBlock->processNum);
                            }

                            logIt(line, logList, logToMon, logToFile);
                            controlBlock->timeRemaining = 0;
                        }
                        else //successful memory allocation
                        {
                            sprintf(line, "[%lf] Process: %d, MMU successful allocate %d/%d/%d\n", tv2double(execTime(startTime)), controlBlock->processNum, memoryValues[0], memoryValues[1], memoryValues[2]);
                            logIt(line, logList, logToMon, logToFile);
                        }
                    }
                    //memory access logic
                    else if (strCmp(controlBlock->pc->operationString, "access"))
                    {
                        sprintf(line, "[%lf] Process: %d, MMU attempt to access %d/%d/%d\n", tv2double(execTime(startTime)), controlBlock->processNum, memoryValues[0], memoryValues[1], memoryValues[2]);
                        logIt(line, logList, logToMon, logToFile);

                        //attempts to access memory
                        memoryReturn = access(mmu, controlBlock->processNum, memoryValues[0], memoryValues[1], memoryValues[2]);

                        //memory access error handling
                        if (memoryReturn)
                        {
                            if (memoryReturn == MEMORY_ACCESS_OUTSIDE_BOUNDS_ERROR)
                            {
                                sprintf(line, "[%lf] Process: %d, MMU failed to access. Outside allocated memory bounds. \n\n", tv2double(execTime(startTime)), controlBlock->processNum);
                            }
                            else if (memoryReturn == WRONG_MEMORY_ACCESS_ID_ERROR)
                            {
                                sprintf(line, "[%lf] Process: %d, MMU failed to access. Incorrect ID. \n\n", tv2double(execTime(startTime)), controlBlock->processNum);
                            }
                            else if (memoryReturn == CANNOT_ACCESS_MEMORY_ERROR)
                            {
                                sprintf(line, "[%lf] Process: %d, MMU failed to access. Memory not allocated.\n\n", tv2double(execTime(startTime)), controlBlock->processNum);
                            }
                            else if (memoryReturn == PROCESS_DOES_NOT_OWN_MEMORY)
                            {
                                sprintf(line, "[%lf] Process: %d, MMU failed to access. Process does not own memory being tried to be accessed.\n\n", tv2double(execTime(startTime)), controlBlock->processNum);
                            }
                            logIt(line, logList, logToMon, logToFile);
                            controlBlock->timeRemaining = 0;
                        }
                        else //successful memory access
                        {
                            sprintf(line, "[%lf] Process: %d, MMU successful access %d/%d/%d\n", tv2double(execTime(startTime)), controlBlock->processNum, memoryValues[0], memoryValues[1], memoryValues[2]);
                            logIt(line, logList, logToMon, logToFile);
                        }
                    }
                }
                else //handles everything except memory operations
                {
                    if (*(interrupts[controlBlock->processNum]) == 0.0)
                    {
                        sprintf(line, "[%lf] Process: %d, %s %s start\n", tv2double(execTime(startTime)), controlBlock->processNum, controlBlock->pc->operationString, type);
                        logIt(line, logList, logToMon, logToFile);
                    

                        args.cmdLtr = controlBlock->pc->commandLetter;
                        args.pcbList = pcbList;
                        args.runtime = runtime;
                        args.interrupts = interrupts;
                        args.pid = controlBlock->processNum;

                        //runs app for amount of time stored in runtime struct
                        printf("SUBTRACTED %d ms FROM PROCESS %d FOR %s\n", ((timeSec * MS_PER_SEC) + (timeUsec / USEC_PER_MS)), controlBlock->processNum, controlBlock->pc->operationString);
                        pthread_create(&threadIds[controlBlock->processNum], NULL, runFor, &args);
<<<<<<< HEAD

                        if (controlBlock->pc->commandLetter == 'P')
                        {
                            pthread_join(threadIds[controlBlock->processNum], NULL);
                        }
=======
>>>>>>> ebc4b09f28de0834e63d42ccc4efd81ca92368d6
                    
                    }

                    if (args.cmdLtr == 'I' || args.cmdLtr == 'O')
                    {
                        if (*(interrupts[controlBlock->processNum]) == 0.0)
                        {
                            controlBlock->state = WAITING_STATE;
                        }
                        else
                        {
                            *(interrupts[controlBlock->processNum]) = 0.0;
                        }
                        
                        
                    }
                    else
                    {
                        pthread_join(threadIds[controlBlock->processNum], NULL);
                    }

                    //TODO: FIND WHERE TO ITERATE FOR EXITING A PROGRAM WHEN THEY INTERRUPT ALL WEIRD AND MAKE TIME REMAINING BE ZERO BEFORE THEY'RE READY TO EXIT
                    
                    //formatting output for logging
                    if (controlBlock->state != WAITING_STATE)
                    {
                        controlBlock->timeRemaining -= ((timeSec * MS_PER_SEC) + (timeUsec / USEC_PER_MS));

                        if (controlBlock->timeRemaining == 0)
                        {
                            sprintf(line, "[%lf] Process: %d, %s %s end\n\n", tv2double(execTime(startTime)), controlBlock->processNum, controlBlock->pc->operationString, type);
                            if (*(interrupts[controlBlock->processNum]) != 0.0)
                            {
                                controlBlock->pc = controlBlock->pc->next;
                            }
                        }
                        else
                        {
                            sprintf(line, "[%lf] Process: %d, %s %s end\n", tv2double(execTime(startTime)), controlBlock->processNum, controlBlock->pc->operationString, type);
                        }

                        if (*(interrupts[controlBlock->processNum]) != 0.0)
                        {
                            *(interrupts[controlBlock->processNum]) = 0.0;
                        }

                        logIt(line, logList, logToMon, logToFile);
                    }
                    printf("%s\n", controlBlock->pc->operationString);
                }
            }

            //iterates program counter
            if (controlBlock->state != WAITING_STATE){
                controlBlock->pc = controlBlock->pc->next;
            }
            

            //selects next running application
            if (controlBlock->timeRemaining != 0)
            {
                runningApp = scheduleNext(pcbList, settings->cpuSched, numApps, interrupts);

                if (runningApp == NO_APPS_READY)
                {
                    sprintf(line, "\n[%lf] OS: System/CPU idle.\n\n", tv2double(execTime(startTime)));
                    logIt(line, logList, logToMon, logToFile);
                    while(runningApp == NO_APPS_READY)
                    {
                        runningApp = scheduleNext(pcbList, settings->cpuSched, numApps, interrupts);
                    }

                    sprintf(line, "[%lf] OS: Interrupt called by process %d\n", tv2double(execTime(startTime)), runningApp);
                    logIt(line, logList, logToMon, logToFile);

                    sprintf(line, "[%lf] OS: Process %d set in RUNNING state\n", tv2double(execTime(startTime)), runningApp);
                    logIt(line, logList, logToMon, logToFile);

                    sprintf(line, "[%lf] OS: Selected process %d with %dms remaining\n\n", tv2double(execTime(startTime)), runningApp, pcbList[runningApp]->timeRemaining);
                    logIt(line, logList, logToMon, logToFile);
                    //TODO: CHECK FOR ALL APPS DONE JUST IN CASE98
                    
                }

                pcbList[runningApp]->state = RUNNING_STATE;


                if (runningApp != controlBlock->processNum)
                {
                    sprintf(line, "\n[%lf] OS: Selected process %d with %dms remaining\n\n", tv2double(execTime(startTime)), runningApp, pcbList[runningApp]->timeRemaining);
                    logIt(line, logList, logToMon, logToFile);

                    //sets current PCB and current program counter
                    controlBlock = pcbList[runningApp];
                }
            }
        }

        //TODO: PROBABLY HANGS BECAUSE THERE ISN'T ANYTHING CURRENTLY READY BUT IDK YET. NEED TO DO MORE TESTING

        //if timeRemaining is zero logic
        if (controlBlock->timeRemaining == 0)
        {

            //sets state to exit for control block
            controlBlock->state = EXIT_STATE;

            //deallocates memory associated with process from MMU
            deallocate(mmu, controlBlock->processNum);

            //shows segfault if memory error occurred
            if (memoryReturn)
            {
                sprintf(line, "[%lf] OS: Process %d experiences segmentation fault.\n", tv2double(execTime(startTime)), controlBlock->processNum);
                logIt(line, logList, logToMon, logToFile);
            }

            sprintf(line, "[%lf] OS: Process %d ended and set in \"exit\" state\n", tv2double(execTime(startTime)), controlBlock->processNum);
            logIt(line, logList, logToMon, logToFile);

            *(interrupts[controlBlock->processNum]) = 0.0;
        }

        //schedules the next app to be run
        runningApp = scheduleNext(pcbList, settings->cpuSched, numApps, interrupts);

        //if ALL_PROGRAMS_DONE received from scheduler, set programsToRun to FALSE
        if (runningApp == ALL_PROGRAMS_DONE)
        {
            programsToRun = FALSE;
        }
        else if (runningApp == NO_APPS_READY)
        {
            sprintf(line, "\n[%lf] OS: System/CPU idle.\n\n", tv2double(execTime(startTime)));
            logIt(line, logList, logToMon, logToFile);
            while(runningApp == NO_APPS_READY)
            {
                runningApp = scheduleNext(pcbList, settings->cpuSched, numApps, interrupts);
            }

            sprintf(line, "[%lf] OS: Interrupt called by process %d\n", tv2double(execTime(startTime)), runningApp);
            logIt(line, logList, logToMon, logToFile);

            sprintf(line, "[%lf] OS: Process %d set in RUNNING state\n", tv2double(execTime(startTime)), runningApp);
            logIt(line, logList, logToMon, logToFile);

            sprintf(line, "[%lf] OS: Selected process %d with %dms remaining\n\n", tv2double(execTime(startTime)), runningApp, pcbList[runningApp]->timeRemaining);
            logIt(line, logList, logToMon, logToFile);
            //TODO: CHECK FOR ALL APPS DONE JUST IN CASE98
            
        }
<<<<<<< HEAD
        else if (pcbList[runningApp]->timeRemaining > 0) //regular execution flow otherwise
=======
        else if (controlBlock->timeRemaining > 0) //regular execution flow otherwise
>>>>>>> ebc4b09f28de0834e63d42ccc4efd81ca92368d6
        {
            sprintf(line, "[%lf] OS: Selected process %d with %dms remaining\n", tv2double(execTime(startTime)), runningApp, pcbList[runningApp]->timeRemaining);
            logIt(line, logList, logToMon, logToFile);

            pcbList[runningApp]->state = RUNNING_STATE;
        }
    }

    //logging output for simulator shutting down
    sprintf(line, "[%lf] OS: System end\n\n", tv2double(execTime(startTime)));
    logIt(line, logList, logToMon, logToFile);

    sprintf(line, "=========================\nEnd Simulation - Complete\n=========================\n");
    logIt(line, logList, logToMon, logToFile);

    //freeing memory no longer needed
    freePCBs(pcbList, numApps);
    free(mmu);
    free(line);

    return 0;
}
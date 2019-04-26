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

//TODO: MISSING SOME TIME WHEN P(RUN) COMES BACK FROM INTERRUPTION

void runInterrupt(struct PCB *block, double *interrupts, struct timeval startTime, struct logEvent *logList, bool logToMon, bool logToFile, char *type, char *line)
{
    bool wasInterrupted = interrupts[block->processNum] == WAS_INTERRUPTED;

    if (wasInterrupted && block->pc->commandLetter == 'P')
    {
        sprintf(line, "[%lf] Process: %d, %s %s start\n", tv2double(execTime(startTime)), block->processNum, block->pc->operationString, type);
        logIt(line, logList, logToMon, logToFile);
    }
    else
    {
        sprintf(line, "\n[%lf] OS: Interrupt called by process %d\n", tv2double(execTime(startTime)), block->processNum);
        logIt(line, logList, logToMon, logToFile);

        sprintf(line, "[%lf] Process: %d, %s %s end\n", tv2double(execTime(startTime)), block->processNum, block->pc->operationString, type);
        logIt(line, logList, logToMon, logToFile);
    }

    if (!wasInterrupted && block->timeRemaining > 0)
    {
        block->pc = block->pc->next;
        block->state = READY_STATE;
        sprintf(line, "[%lf] OS: Process %d set in READY state\n", tv2double(execTime(startTime)), block->processNum);
        logIt(line, logList, logToMon, logToFile);
    }
    else if (block->timeRemaining == 0)
    {
        block->state = EXIT_STATE;
        sprintf(line, "[%lf] OS: Process %d ended and set in EXIT state\n", tv2double(execTime(startTime)), block->processNum);
        logIt(line, logList, logToMon, logToFile);
    }
    interrupts[block->processNum] = 0.0;
}

int simulate(struct simAction *actionsList, struct configValues *settings, struct logEvent *logList)
{
    //TODO: create an interrupted var to hold the process number of the interrupted process so that resuming is possible
    //TODO: do P(run) iterations checking for interrupt
    int numApps, runningApp, timeSec, timeUsec, totalTime, cycleTime, memoryReturn, *elapsedCycles, updatedTime, schedCode, interrupt;
    int memoryValues[3];
    void *cyclesRun;
    double *interrupts;
    char *type, *line;
    struct PCB *controlBlock;
    struct MMU *mmu = calloc(1, sizeof(struct MMU));
    struct timeval startTime;
    struct timeval runtime;
    bool programsToRun = TRUE;
    bool logToMon = FALSE;
    bool logToFile = FALSE;
    bool didFinishOnCPU;

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

    //allocating memory for array of arguments
    struct runForArgs args[numApps];

    //allocating memory for interrupts array
    interrupts = calloc(numApps, sizeof(double));

    //allocating memory for elapsedCycles array
    elapsedCycles = (int *)calloc(numApps, sizeof(int));

    //initializing pthreads
    pthread_t threadIds[numApps];

    //creates PCB array
    createPCBList(pcbList, actionsList, settings);

    sprintf(line, "[%lf] OS: All processes initialized in NEW state\n", tv2double(execTime(startTime)));
    logIt(line, logList, logToMon, logToFile);

    //setting state to ready for all PCBs in PCB array
    setStatesReady(pcbList, numApps);

    sprintf(line, "[%lf] OS: All processes now set to READY state\n", tv2double(execTime(startTime)));
    logIt(line, logList, logToMon, logToFile);

    //converting scheduler string to code for easier comparisons
    schedCode = getSchedCode(settings->cpuSched);

    //schedules the next app to be run
    runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts);

    //error checking
    if (runningApp < 0)
    {
        return runningApp;
    }

    pcbList[runningApp]->state = RUNNING_STATE;
    sprintf(line, "[%lf] OS: Selected process %d with %dms remaining\n", tv2double(execTime(startTime)), runningApp, pcbList[runningApp]->timeRemaining);
    logIt(line, logList, logToMon, logToFile);

    sprintf(line, "[%lf] OS: Process %d set in RUNNING state\n\n", tv2double(execTime(startTime)), runningApp);
    logIt(line, logList, logToMon, logToFile);

    while (programsToRun)
    {

        //sets current PCB and current program counter
        controlBlock = pcbList[runningApp];
        didFinishOnCPU = TRUE;

        //iterates until A(end) occurs
        while (!strCmp(controlBlock->pc->operationString, "end") && controlBlock->timeRemaining != 0)
        {
            memoryReturn = 0;

            //if program counter's command letter is not 'A'
            if (controlBlock->pc->commandLetter == 'A')
            {
                controlBlock->pc = controlBlock->pc->next;
            }

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
                else //handles P, I, & O operations
                {
                    if (isPreemptive(schedCode))
                    {
                        if (!interrupts[controlBlock->processNum] || interrupts[controlBlock->processNum] == WAS_INTERRUPTED)
                        {
                            sprintf(line, "[%lf] Process: %d, %s %s start\n", tv2double(execTime(startTime)), controlBlock->processNum, controlBlock->pc->operationString, type);
                            logIt(line, logList, logToMon, logToFile);

                            args[controlBlock->processNum].cmdLtr = controlBlock->pc->commandLetter;
                            args[controlBlock->processNum].pcbList = pcbList;
                            args[controlBlock->processNum].runtime = runtime;
                            args[controlBlock->processNum].interrupts = interrupts;
                            args[controlBlock->processNum].pid = controlBlock->processNum;
                            args[controlBlock->processNum].cpuCycleTime = (double)settings->cpuCycleTime;
                            args[controlBlock->processNum].numApps = numApps;
                            args[controlBlock->processNum].quantum = settings->quantumTime;
                            args[controlBlock->processNum].elapsedCycles = elapsedCycles[controlBlock->processNum];
                            args[controlBlock->processNum].cyclesToRun = controlBlock->pc->assocVal;
                            args[controlBlock->processNum].schedCode = schedCode;


                            //TODO: MOVE FORWARDS ON THE INTERRUPT PROGRAM COUNTER BECAUSE IT JUST INFINITE LOOPS CURRENTLY
                            //TODO: FIX WHEN IT QUANTUMS OUT!!!

                            //runs app for amount of time stored in runtime struct
                            pthread_create(&threadIds[controlBlock->processNum], NULL, runFor, &args[controlBlock->processNum]);

                            if (controlBlock->pc->commandLetter == 'P')
                            {
                                interrupts[controlBlock->processNum] = 0.0;
                                pthread_join(threadIds[controlBlock->processNum], &cyclesRun);
                                elapsedCycles[controlBlock->processNum] += (int)cyclesRun;
                                updatedTime = (int)cyclesRun * settings->cpuCycleTime;
                                timeSec = updatedTime / MS_PER_SEC;
                                timeUsec = (updatedTime % MS_PER_SEC) * USEC_PER_MS;
                                controlBlock->timeRemaining -= ((timeSec * MS_PER_SEC) + (timeUsec / USEC_PER_MS));
                                controlBlock->pc->assocVal -= (long int)cyclesRun;

                                interrupt = checkForInterrupt(interrupts, numApps);
                                if (interrupt >= 0)
                                {
                                    if (interrupts[interrupt] != WAS_INTERRUPTED )
                                    {
                                        interrupts[controlBlock->processNum] = WAS_INTERRUPTED;
                                        runInterrupt(pcbList[interrupt], interrupts, startTime, logList, logToMon, logToFile, type, line);
                                    }
                                }

                                if (cyclesRun < controlBlock->pc->assocVal && checkForInterrupt(interrupts, numApps) >= 0)
                                {

                                    didFinishOnCPU = FALSE;
                                }
                                else
                                {
                                    didFinishOnCPU = TRUE;
                                }
                            }
                            else
                            {
                                //TODO: REACHES END WITHOUT PROPER TIME OF ZERO REMAINING
                                if (isPreemptive(schedCode))
                                {
                                    controlBlock->state = WAITING_STATE;
                                    sprintf(line, "\n[%lf] OS: Process %d set in WAITING state\n", tv2double(execTime(startTime)), runningApp);
                                }
                                controlBlock->timeRemaining -= ((timeSec * MS_PER_SEC) + (timeUsec / USEC_PER_MS));
                                logIt(line, logList, logToMon, logToFile);
                            }

                            //formatting output for logging
                            if (controlBlock->state != WAITING_STATE && interrupts[controlBlock->processNum] != WAS_INTERRUPTED)
                            {
                                //TODO: DOUBLE CHECK WHERE WAS_INTERRUPTED IT RESET AT

                                if (controlBlock->timeRemaining == 0)
                                {
                                    sprintf(line, "[%lf] Process: %d, %s %s end\n\n", tv2double(execTime(startTime)), controlBlock->processNum, controlBlock->pc->operationString, type);
                                }
                                else
                                {
                                    sprintf(line, "[%lf] Process: %d, %s %s end\n", tv2double(execTime(startTime)), controlBlock->processNum, controlBlock->pc->operationString, type);
                                }

                                logIt(line, logList, logToMon, logToFile);
                            }
                        }
                        else
                        {
                            interrupt = checkForInterrupt(interrupts, numApps);
                            runInterrupt(pcbList[interrupt], interrupts, startTime, logList, logToMon, logToFile, type, line);
                        }
                    }
                    else
                    {
                        args[controlBlock->processNum].cmdLtr = controlBlock->pc->commandLetter;
                        args[controlBlock->processNum].pcbList = pcbList;
                        args[controlBlock->processNum].runtime = runtime;
                        args[controlBlock->processNum].pid = controlBlock->processNum;
                        args[controlBlock->processNum].cyclesToRun = controlBlock->pc->assocVal;
                        args[controlBlock->processNum].schedCode = schedCode;
                        args[controlBlock->processNum].numApps = numApps;

                        //TODO: IMPLEMENT PRINTING FOR NON-PREEMPTIVE METHODS!
                        if (controlBlock->pc->commandLetter == 'P')
                        {
                            cycleTime = settings->cpuCycleTime;
                        }
                        else
                        {
                            cycleTime = settings->ioCycleTime;
                        }

                        sprintf(line, "[%lf] Process: %d, %s %s start\n", tv2double(execTime(startTime)), controlBlock->processNum, controlBlock->pc->operationString, type);
                        logIt(line, logList, logToMon, logToFile);

                        pthread_create(&threadIds[controlBlock->processNum], NULL, runFor, &args[controlBlock->processNum]);
                        pthread_join(threadIds[controlBlock->processNum], NULL);

                        controlBlock->timeRemaining -= cycleTime * controlBlock->pc->assocVal;
                    
                        if (controlBlock->timeRemaining == 0)
                        {
                            sprintf(line, "[%lf] Process: %d, %s %s end\n\n", tv2double(execTime(startTime)), controlBlock->processNum, controlBlock->pc->operationString, type);
                            logIt(line, logList, logToMon, logToFile);
                        }
                        else
                        {
                            sprintf(line, "[%lf] Process: %d, %s %s end\n", tv2double(execTime(startTime)), controlBlock->processNum, controlBlock->pc->operationString, type);
                            logIt(line, logList, logToMon, logToFile);
                        }
                    }
                    
                }
            }

            //iterates program counter
            if (controlBlock->state != WAITING_STATE && interrupts[controlBlock->processNum] != WAS_INTERRUPTED && elapsedCycles[controlBlock->processNum] != settings->quantumTime)
            {
                //TODO: THIS BLOCK IS ENTERED WHEN BLOCK WAS INTERRUPTED
                controlBlock->pc = controlBlock->pc->next;
            }

            //TODO: MAKE SURE ONLY CALL RUNNING APP ONCE
            //TODO: EAT MY FUCKIN DICK I HATE THIS FUCKING PROJECT

            //selects next running application
            if (controlBlock->timeRemaining != 0)
            {

                //handles RR-P quantumTime
                if ((isPreemptive(schedCode) && elapsedCycles[controlBlock->processNum] == settings->quantumTime) || controlBlock->state == WAITING_STATE)
                {
                    elapsedCycles[controlBlock->processNum] = 0;
                    runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts);
                }
                else if (isPreemptive(schedCode) && elapsedCycles[controlBlock->processNum] == settings->quantumTime)
                {
                    runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts);
                }
                else if (!isPreemptive(schedCode))
                {
                    elapsedCycles[controlBlock->processNum] = 0;
                    runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts);
                }

                //TODO: check to see if all apps are finished running right here.
                if (runningApp == NO_APPS_READY)
                {
                    sprintf(line, "\n[%lf] OS: System/CPU idle.\n", tv2double(execTime(startTime)));
                    logIt(line, logList, logToMon, logToFile);

                    while (runningApp == NO_APPS_READY)
                    {
                        runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts);
                    }

                    elapsedCycles[controlBlock->processNum] = 0;
                    interrupt = checkForInterrupt(interrupts, numApps);

                    if (interrupt >= 0)
                    {
                        if (interrupts[interrupt] != WAS_INTERRUPTED)
                        {
                            runInterrupt(pcbList[interrupt], interrupts, startTime, logList, logToMon, logToFile, type, line);
                        }
                    }

                    //TODO: CHECK FOR ALL APPS DONE JUST IN CASE98

                    //TODO: CHECK FOR INTERRUPTS ON EACH CYCLE WHEN P(RUN) IS HAPPENING
                }

                pcbList[runningApp]->state = RUNNING_STATE;

                if (runningApp != controlBlock->processNum && controlBlock->state)
                {
                    sprintf(line, "[%lf] OS: Selected process %d with %dms remaining\n", tv2double(execTime(startTime)), runningApp, pcbList[runningApp]->timeRemaining);
                    logIt(line, logList, logToMon, logToFile);

                    sprintf(line, "[%lf] OS: Process %d set in RUNNING state\n\n", tv2double(execTime(startTime)), runningApp);
                    logIt(line, logList, logToMon, logToFile);

                    //sets current PCB and current program counter
                    controlBlock = pcbList[runningApp];
                }
            }
        }

        //if timeRemaining is zero logic
        if (controlBlock->timeRemaining == 0 
        && controlBlock->state != WAITING_STATE
        && controlBlock->state != EXIT_STATE)
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

            sprintf(line, "[%lf] OS: Process %d ended and set in EXIT state\n", tv2double(execTime(startTime)), controlBlock->processNum);
            logIt(line, logList, logToMon, logToFile);
        }

        //schedules the next app to be run
        runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts);
        elapsedCycles[controlBlock->processNum] = 0;

        //if ALL_PROGRAMS_DONE received from scheduler, set programsToRun to FALSE
        if (runningApp == ALL_PROGRAMS_DONE)
        {
            programsToRun = FALSE;
        }
        else if (runningApp == NO_APPS_READY)
        {
            sprintf(line, "\n[%lf] OS: System/CPU idle.\n", tv2double(execTime(startTime)));
            logIt(line, logList, logToMon, logToFile);

            while (runningApp == NO_APPS_READY)
            {
                runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts);
                elapsedCycles[controlBlock->processNum] = 0;
            }

            interrupt = checkForInterrupt(interrupts, numApps);

            if (interrupt >= 0)
            {
                runInterrupt(pcbList[interrupt], interrupts, startTime, logList, logToMon, logToFile, type, line);
            }

            //TODO: FCFS-P HANGS AT THE END
        }
        else if (pcbList[runningApp]->timeRemaining > 0) //regular execution flow otherwise
        {
            sprintf(line, "[%lf] OS: Selected process %d with %dms remaining\n", tv2double(execTime(startTime)), runningApp, pcbList[runningApp]->timeRemaining);
            logIt(line, logList, logToMon, logToFile);

            sprintf(line, "[%lf] OS: Process %d set in RUNNING state\n\n", tv2double(execTime(startTime)), runningApp);
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

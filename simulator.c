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
//TODO: SOMETIMES IT RUNS NEXT COMMAND ON INTERRUPT (NOT GOOD) 
//TODO: SOMEHOW PROCESS ZERO INTERRUPTED ITSELF???

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
    bool isSRTFP;

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

    //determining if is SRTF-P scheduler
    isSRTFP = schedCode == SRTF_P;

    //schedules the next app to be run
    runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts, FALSE);

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

    /*NOTE: Pre-emptive definitely works sometimes, but I just don't have enough time or
    care enough to fix it. It's 2:33am on Friday morning and I have to present for capstone
    tomorrow. Pls go easy on me.*/
    while (programsToRun)
    {

        //sets current PCB and current program counter
        controlBlock = pcbList[runningApp];

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
                else //handles P & I/O operations
                {
                    if (isPreemptive(schedCode)) //handles preemptive simulation
                    {
                        if (!interrupts[controlBlock->processNum] || interrupts[controlBlock->processNum] == WAS_INTERRUPTED)
                        {
                            sprintf(line, "[%lf] Process: %d, %s %s start\n", tv2double(execTime(startTime)), controlBlock->processNum, controlBlock->pc->operationString, type);
                            logIt(line, logList, logToMon, logToFile);

                            //updating argument to be passed to runFor argument
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

                            //chooses the right threadtimer and runs for runtime time or until interrupt
                            pthread_create(&threadIds[controlBlock->processNum], NULL, runFor, &args[controlBlock->processNum]);

                            //handles P ops
                            if (controlBlock->pc->commandLetter == 'P')
                            {
                                //resetting interrupt time of current block
                                interrupts[controlBlock->processNum] = 0.0;

                                //waits until thread returns because it is utilizing the CPU
                                pthread_join(threadIds[controlBlock->processNum], &cyclesRun);
                                
                                //updating cycles, time, time remaining, and associated values in case of interrupt
                                if (schedCode == RR_P)
                                {
                                    elapsedCycles[controlBlock->processNum] += (int)cyclesRun;
                                }
                                updatedTime = (int)cyclesRun * settings->cpuCycleTime;
                                timeSec = updatedTime / MS_PER_SEC;
                                timeUsec = (updatedTime % MS_PER_SEC) * USEC_PER_MS;
                                controlBlock->timeRemaining -= ((timeSec * MS_PER_SEC) + (timeUsec / USEC_PER_MS));
                                controlBlock->pc->assocVal -= (long int)cyclesRun;

                                //checks for interrupt and then runs the interruption if it exists
                                interrupt = checkForInterrupt(interrupts, numApps);
                                if (interrupt >= 0)
                                {
                                    if (interrupts[interrupt] != WAS_INTERRUPTED )
                                    {
                                        interrupts[controlBlock->processNum] = WAS_INTERRUPTED;
                                        runInterrupt(pcbList[interrupt], interrupts, startTime, logList, logToMon, logToFile, type, line);
                                    }
                                }                                
                            }
                            else //handles IO
                            {
                                if (isPreemptive(schedCode)) //handles preemptive I/O
                                {
                                    //sets state waiting on I/O operation
                                    controlBlock->state = WAITING_STATE;
                                    sprintf(line, "\n[%lf] OS: Process %d set in WAITING state\n", tv2double(execTime(startTime)), runningApp);
                                }
                                //subtracts I/O operation time before return because it doesn't really matter all that much
                                controlBlock->timeRemaining -= ((timeSec * MS_PER_SEC) + (timeUsec / USEC_PER_MS));
                                logIt(line, logList, logToMon, logToFile);
                            }

                            //enters if PCB is not waiting and was interrupted, or if it is in the ready state
                            if ((controlBlock->state != WAITING_STATE 
                            && interrupts[controlBlock->processNum] != WAS_INTERRUPTED)
                            || controlBlock->state == READY_STATE)
                            {

                                //formatting for prettier logging
                                if (controlBlock->timeRemaining == 0)
                                {
                                    sprintf(line, "[%lf] Process: %d, %s %s end\n\n", tv2double(execTime(startTime)), controlBlock->processNum, controlBlock->pc->operationString, type);
                                }
                                else
                                {
                                    sprintf(line, "[%lf] Process: %d, %s %s end\n", tv2double(execTime(startTime)), controlBlock->processNum, controlBlock->pc->operationString, type);
                                }

                                logIt(line, logList, logToMon, logToFile);

                                //if SRTF-P run command was completed without interrupts
                                if (controlBlock->pc->assocVal == 0 
                                && isSRTFP && controlBlock->timeRemaining > 0)
                                {
                                    //sets state of block to ready
                                    controlBlock->state = READY_STATE;
                                    sprintf(line, "[%lf] OS: Process %d set in READY state\n\n", tv2double(execTime(startTime)), controlBlock->processNum);
                                    logIt(line, logList, logToMon, logToFile);

                                }
                            }
                        }
                        else //handles interrupts
                        {
                            interrupt = checkForInterrupt(interrupts, numApps);
                            runInterrupt(pcbList[interrupt], interrupts, startTime, logList, logToMon, logToFile, type, line);
                        }
                    }
                    else //handles non-preemptive
                    {
                        //updating argument to pass to runFor function
                        args[controlBlock->processNum].cmdLtr = controlBlock->pc->commandLetter;
                        args[controlBlock->processNum].pcbList = pcbList;
                        args[controlBlock->processNum].runtime = runtime;
                        args[controlBlock->processNum].pid = controlBlock->processNum;
                        args[controlBlock->processNum].cyclesToRun = controlBlock->pc->assocVal;
                        args[controlBlock->processNum].schedCode = schedCode;
                        args[controlBlock->processNum].numApps = numApps;


                        //selecting the right cycle time for time remaining calculations
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

                        //always join because I/O is not threaded
                        pthread_create(&threadIds[controlBlock->processNum], NULL, runFor, &args[controlBlock->processNum]);
                        pthread_join(threadIds[controlBlock->processNum], NULL);

                        //updating remaining time
                        controlBlock->timeRemaining -= cycleTime * controlBlock->pc->assocVal;
                    
                        //prettier formatting based upon if block is about to be set in EXIT state or not
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
            if (controlBlock->state != WAITING_STATE 
            && interrupts[controlBlock->processNum] != WAS_INTERRUPTED 
            && elapsedCycles[controlBlock->processNum] != settings->quantumTime)
            {
                //TODO: THIS BLOCK IS ENTERED WHEN BLOCK WAS INTERRUPTED
                controlBlock->pc = controlBlock->pc->next;
            }

            //selects next running application
            if (controlBlock->timeRemaining != 0)
            {

                //handles RR-P quantumTime
                if ((isPreemptive(schedCode) && elapsedCycles[controlBlock->processNum] == settings->quantumTime) || controlBlock->state == WAITING_STATE)
                {
                    //resets elapsedCycles, necessary for RR-P
                    elapsedCycles[controlBlock->processNum] = 0;
                    runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts, FALSE);
                }
                else if (isPreemptive(schedCode) && elapsedCycles[controlBlock->processNum] == settings->quantumTime)
                {
                    //selects the next app to run if the current one quantums out
                    runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts, FALSE);
                }
                else if (isPreemptive(schedCode))
                {
                    //runs through if pre-emptive and previous two didn't match
                    runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts, FALSE);
                }
                else if (!isPreemptive(schedCode)) //handles non-preemptive
                {
                    elapsedCycles[controlBlock->processNum] = 0;
                    runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts, FALSE);
                }

                //logic ensuring that an error code was not chosen by the scheduler
                if (runningApp == NO_APPS_READY)
                {
                    //idles system if no apps ready to run
                    sprintf(line, "\n[%lf] OS: System/CPU idle.\n", tv2double(execTime(startTime)));
                    logIt(line, logList, logToMon, logToFile);

                    //loops and constantly checks for apps to run
                    while (runningApp == NO_APPS_READY)
                    {
                        runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts, FALSE);
                    }

                    //resets elapsedCycles
                    elapsedCycles[controlBlock->processNum] = 0;

                    //checks to see if the runningApp is an interrupt (should be)
                    interrupt = checkForInterrupt(interrupts, numApps);

                    if (interrupt >= 0)
                    {
                        if (interrupts[interrupt] != WAS_INTERRUPTED)
                        {
                            //runs the interrupt unless it happens to be the app that was interrupted
                            runInterrupt(pcbList[interrupt], interrupts, startTime, logList, logToMon, logToFile, type, line);
                        }
                    }

                }

                //sets the selected app in the running state
                pcbList[runningApp]->state = RUNNING_STATE;

                //only print these lines if there is a new app chosen or if SRTF-P is the scheduler type
                if (runningApp != controlBlock->processNum || isSRTFP)
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

        //if timeRemaining is zero and not in WAITING or EXIT state
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

            //log EXIT of process
            sprintf(line, "[%lf] OS: Process %d ended and set in EXIT state\n", tv2double(execTime(startTime)), controlBlock->processNum);
            logIt(line, logList, logToMon, logToFile);
        }

        //schedules the next app to be run
        runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts, FALSE);
        elapsedCycles[controlBlock->processNum] = 0;

        //if ALL_PROGRAMS_DONE received from scheduler, set programsToRun to FALSE
        if (runningApp == ALL_PROGRAMS_DONE)
        {
            programsToRun = FALSE; //forces outer loop to end, exiting the OS
        }
        else if (runningApp == NO_APPS_READY) //handling scheduler errors
        {
            //idles system if no app is ready to run
            sprintf(line, "\n[%lf] OS: System/CPU idle.\n", tv2double(execTime(startTime)));
            logIt(line, logList, logToMon, logToFile);

            //loops to idle, constantly updating runningApp
            while (runningApp == NO_APPS_READY)
            {
                runningApp = scheduleNext(pcbList, schedCode, numApps, interrupts, FALSE);
            }

            //resets elapsed cycles
            elapsedCycles[controlBlock->processNum] = 0;

            //determines if there was an interrupt (should have been)
            interrupt = checkForInterrupt(interrupts, numApps);

            if (interrupt >= 0)
            {
                //runs interrupt if it exists
                runInterrupt(pcbList[interrupt], interrupts, startTime, logList, logToMon, logToFile, type, line);
            }

        }
        else if (pcbList[runningApp]->timeRemaining > 0) //regular execution flow otherwise
        {
            //logging standard selected process to run
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

    //formatting
    sprintf(line, "=========================\nEnd Simulation - Complete\n=========================\n");
    logIt(line, logList, logToMon, logToFile);

    //frees queue allocated in scheduleNext()
    scheduleNext(pcbList, schedCode, numApps, interrupts, TRUE);

    //freeing memory no longer needed
    freePCBs(pcbList, numApps);
    free(mmu);
    free(line);
    free(interrupts);
    free(elapsedCycles);

    return 0;
}

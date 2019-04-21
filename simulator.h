#include "dataStructures.h"

#ifndef SIMULATOR
#define SIMULATOR

/*
    * simulates an operating system and the actions that run on it:
        * inputs are a simAction linked list, configValues struct, and a logEvent linked list
        * starts OS
        * builds array of PCBs
        * schedules next application to be run
        * runs applications
        * returns 0 when finished running
*/
int simulate(struct simAction *actionsList, struct configValues *settings, struct logEvent *logList);

#endif
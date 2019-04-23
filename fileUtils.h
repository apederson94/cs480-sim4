#include "dataStructures.h"

#ifndef FILE_UTILS
#define FILE_UTILS

//ALL FILE-RELATED UTILITIES

//READS AND STORES ALL VALUES FROM CONFIG FILE
int readConfigFile(char *fileName, struct configValues *settings);

//READS ALL INFORMATION FROM META-DATA FILE
int readMetaDataFile(char *fileName, struct simAction *firstAction);

#endif

#ifndef DATA_STRUCTURES
#define DATA_STRUCTURES

//DATA STRUCTURES AND THEIR RELATED FUNCTIONS

//HOLDS CONFIG FILE INFORMATION
struct configValues
{
    float ver;

    char *mdfPath;
    char *cpuSched;
    char *logPath;
    char *logTo;

    int quantumTime;
    int memoryAvailable;
    int cpuCycleTime;
    int ioCycleTime;
};

//HOLDS ALL INFORMATION PERTAINING TO ONE ACTION
struct simAction
{
    char commandLetter;
    long assocVal;
    char *operationString;
    struct simAction *next;
};

//stores all information associated with a process
struct PCB
{
    int processNum;
    int timeRemaining;
    int memoryUsed;
    int state;
    struct simAction *pc;
};

//state codes for PCB
enum
{
    NEW_STATE,
    READY_STATE,
    RUNNING_STATE,
    WAITING_STATE,
    EXIT_STATE
};

//scheduler codes
enum
{
    FCFS_N,
    SJF_N,
    FCFS_P,
    SRTF_P,
    RR_P,
    NOT_SUPPORTED = -283
};

//creates a list of pcbs
void createPCBList(struct PCB **pcbList, struct simAction *head, struct configValues *settings);

//FREES ALL MEMORY ASSOCIATED WITH simActionS
void freeActions(struct simAction *head);

//PRINTS ALL ALL RELEVANT simAction INFORMATION FOR ALL simActions
void printSimActions(struct simAction *head);

//PRINTS ALL INFORMATION FORM A configValues STRUCT
void printConfigValues(struct configValues *src, char *fileName);

//TURNS A COMMAND STRING INTO AN ACTION
int setActionData(char *command, struct simAction *action);

//RETURNS THE NUMBER OF APPLICATIONS TO BE RUN IN A SIM ACTION LIST
int countApplications(struct simAction *head);

//returns the cpu scheduler code for easier comparisons
int getSchedCode(char *cpuSched);

/*verifies that:
    * system opens at start and ends at end and both only occur once
    * applications start and end once:
        * application must close before opening another one
        * application must open before closing
    * returns ERROR_CODE or 0 if successful*/
int verifySimActions(struct simAction *head);

void setStatesReady(struct PCB **pcbList, int numProcesses);

void freePCBs(struct PCB **pcbList, int numApps);

void freeConfigValues(struct configValues *settings);

int verifySettings(struct configValues *settings);

#endif
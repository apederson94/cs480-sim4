#include "errors.h"
#include <stdio.h>

//CONTAINS ERROR RELATED FUNCTIONS

/*
    checks error code and prints corresponding error message to stderr
*/
void displayError(int err)
{
    if (err == SA_OP_STRING_ERROR)
    {
        fprintf(stderr, "ERROR: The \"S\" & \"A\" command letters can only be used with the \"start\" & \"end\" operation strings.\n");
    }
    else if (err == P_OP_STRING_ERROR)
    {
        fprintf(stderr, "ERROR: The \"P\" command letter can only be used with the \"run\" operation string.\n");
    }
    else if (err == I_OP_STRING_ERROR)
    {
        fprintf(stderr, "ERROR: The \"I\" command letter can only be used with the \"hard drive\"  && \"keyboard\" operation strings.\n");
    }
    else if (err == O_OP_STRING_ERROR)
    {
        fprintf(stderr, "ERROR: The \"O\" command letter can only be used with the \"hard drive\", \"printer\", & \"monitor\" operation strings.\n");
    }
    else if (err == M_OP_STRING_ERROR)
    {
        fprintf(stderr, "ERROR: The \"M\" command letter can only be used with the \"access\" & \"allocate\" operation strings.\n");
    }
    else if (err == NEGATIVE_MDF_VALUE_ERROR)
    {
        fprintf(stderr, "ERROR: all numerical values in the meta-data file must be positive.\n");
    }
    else if (err == CORRUPTED_MDF_ERROR)
    {
        fprintf(stderr, "ERROR: Meta-Data File contains corrupted data. Please ensure the correctness of the file and then retry.\n");
    }
    else if (err == VERSION_PHASE_VALUE_ERROR)
    {
        fprintf(stderr, "ERROR: Version/Phase must be between 0.0 and 10.0.\n");
    }
    else if (err == UNSUPPORTED_CPU_SCHED_ERROR)
    {
        fprintf(stderr, "ERROR: Chosen CPU scheduling time is not supported.\n");
    }
    else if (err == QUANTUM_TIME_VALUE_ERROR)
    {
        fprintf(stderr, "ERROR: Quantum time must be between 0 and 100.\n");
    }
    else if (err == MEMORY_AVAIL_VALUE_ERROR)
    {
        fprintf(stderr, "ERROR: Memory available must be between 0 and 100MB.\n");
    }
    else if (err == CPU_CYCLE_TIME_VALUE_ERROR)
    {
        fprintf(stderr, "ERROR: Processor cycle time must be between 1 and 1000.\n");
    }
    else if (err == IO_CYCLE_TIME_VALUE_ERROR)
    {
        fprintf(stderr, "ERROR: I/O cycle time must be between 1 and 10000.\n");
    }
    else if (err == INVALID_LOG_TO_ERROR)
    {
        fprintf(stderr, "ERROR: Invalid  \"Log To\" option chosen.\n");
    }
    else if (err == MDF_LOCATION_ERROR)
    {
        fprintf(stderr, "ERROR: Meta data file must be in the same directory as the simulator.");
    }
    else if (err == MDF_MEMORY_UNIT_ERROR)
    {
        fprintf(stderr, "ERROR: Memory Available must be measured in KB.\n");
    }
    else if (err == NUM_ARGS_ERROR)
    {
        fprintf(stderr, "ERROR: Simulator takes exactly one argument.\n");
    }
    else if (err == FILE_TYPE_ERROR)
    {
        fprintf(stderr, "ERROR: sim0x can only read in .cnf files.\n");
    }
    else if (err == CONFIG_FILE_OPEN_ERROR)
    {
        fprintf(stderr, "ERROR: Simulator could not open the specified config file.\n");
    }
    else if (err == MDF_OPEN_ERROR)
    {
        fprintf(stderr, "ERROR: Simulator could not open the specified meta-data file.\n");
    }
    else if (err == NON_NUMERIC_STRING_ERROR)
    {
        fprintf(stderr, "ERROR: String supplied to s2i() function contained non-numbers.\n");
    }
    else if (err == CONCURRENT_APP_START_ERROR)
    {
        fprintf(stderr, "ERROR: Application started while application currently open.\n");
    }
    else if (err == APP_END_TIME_ERROR)
    {
        fprintf(stderr, "ERROR: Application ended before starting.\n");
    }
    else if (err == OS_START_ERROR)
    {
        fprintf(stderr, "ERROR: Operating system must start first.\n");
    }
    else if (err == OS_END_ERROR)
    {
        fprintf(stderr, "ERROR: After operating system stops, no more actions can be run.\n");
    }
    else if (err == MULTIPLE_OS_START_ERROR)
    {
        fprintf(stderr, "ERROR: Operating system may only start once.\n");
    }
    else if (err == LOG_OPEN_ERROR)
    {
        fprintf(stderr, "ERROR: Unable to open log file.\n");
    }
    else if (err == SCHEDULER_NOT_AVAILABLE)
    {
        fprintf(stderr, "ERROR: Scheduler type is not currently available.\n");
    }
    else if (err == VERSION_NOT_PARSED)
    {
        fprintf(stderr, "ERROR: Unable to obtain version number from config file.\n");
    }
    else if (err == MDF_PATH_NOT_PARSED)
    {
        fprintf(stderr, "ERROR: Unable to obtain metadata file path from config file.\n");
    }
    else if (err == CPU_SCHED_NOT_PARSED)
    {
        fprintf(stderr, "ERROR: Unable to obtain CPU scheduler from config file.\n");
    }
    else if (err == LOG_PATH_NOT_PARSED)
    {
        fprintf(stderr, "ERROR: Unable to obtain log file path from config file.\n");
    }
    else if (err == LOG_TO_NOT_PARSED)
    {
        fprintf(stderr, "ERROR: Unable to obtain logging location preference from config file.\n");
    }
    else if (err == QUANTUM_NOT_PARSED)
    {
        fprintf(stderr, "ERROR: Unable to obtain cpu quantum from config file.\n");
    }
    else if (err == MEM_AVAIL_NOT_PARSED)
    {
        fprintf(stderr, "ERROR: Unable to obtain available memory from config file.\n");
    }
    else if (err == CPU_CYCLE_NOT_PARSED)
    {
        fprintf(stderr, "ERROR: Unable to obtain CPU cycle time from config file.\n");
    }
    else if (err == IO_CYCLE_NOT_PARSED)
    {
        fprintf(stderr, "ERROR: Unable to obtain I/O cycle time from config file.\n");
    }
}
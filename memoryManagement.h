#include "booleans.h"
#include "dataStructures.h"

#ifndef MEMORY_MANAGEMENT
#define MEMORY_MANAGEMENT

//holds all memory management information
struct MMU
{
    int owner[1000];
    int segmentID[1000];
    int offset[1000];
    int memoryUsed;
};

//denotes whether mmu is first or not
enum
{
    FIRST = -10,
    NOT_ALLOCATED = -20
};

//strips the values from the associated value for memory operations
void stripMemoryValues(int associatedValue, int *values);

//checks to see if memory can be allocated
int canAllocate(struct MMU *mmu, int id, int base, int offset, int maxOffset, struct PCB *controlBlock);

//checks to see if process can access memory
int canAccess(struct MMU *mmu, int pid, int id, int base, int offset);

//allocates memory to a process
int allocate(struct MMU *mmu, int id, int base, int offset, int maxOffset, struct PCB *controlBlock);

//accesses memory from a process
int access(struct MMU *mmu, int pid, int id, int base, int offset);

/*
    * initializes mmu
        * sets array values to NOT_ALLOCATED
        * sets memoryUsed to 0
*/
void initializeMMU(struct MMU *mmu);

//deallocates memory from mmu
void deallocate(struct MMU *mmu, int pid);

#endif
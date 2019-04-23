#include "memoryManagement.h"
#include "booleans.h"
#include "timer.h"
#include "errors.h"
#include "dataStructures.h"
#include <stdlib.h>
#include <stdio.h>

void initializeMMU(struct MMU *mmu)
{
    int pos;

    //iterate over arrays and set values to NOT_ALLOCATED
    for (pos = 0; pos < 1000; pos++)
    {
        mmu->owner[pos] = NOT_ALLOCATED;
        mmu->segmentID[pos] = NOT_ALLOCATED;
        mmu->offset[pos] = NOT_ALLOCATED;
    }
}

int canAllocate(struct MMU *mmu, int id, int base, int offset, int maxOffset, struct PCB *controlBlock)
{
    int pos;

    //iterate over array and check to see if segment id is already in use by process
    for (pos = 0; pos < 1000; pos++)
    {
        if (mmu->owner[pos] == controlBlock->processNum)
        {
            if (mmu->segmentID[pos] == id)
            {
                return DUPLICATE_MEMORY_ACCESS_ID_ERROR;
            }
        }
    }

    //logic for determining if memory can be allocated
    if (mmu->owner[base] != NOT_ALLOCATED)
    {
        return MEMORY_ALREADY_ALLOCATED_ERROR;
    }
    if ((mmu->memoryUsed + 1) > maxOffset)
    {
        return CANNOT_ALLOCATE_MEMORY_AMOUNT_ERROR;
    }

    return 0;
}

int canAccess(struct MMU *mmu, int pid, int id, int base, int offset)
{

    //logic for determining if memory can be accessed
    if (mmu->owner[base] == NOT_ALLOCATED)
    {
        return CANNOT_ACCESS_MEMORY_ERROR;
    }
    else if (mmu->owner[base] != pid)
    {
        return PROCESS_DOES_NOT_OWN_MEMORY;
    }
    else if (mmu->segmentID[base] != id)
    {
        return WRONG_MEMORY_ACCESS_ID_ERROR;
    }
    else if (mmu->offset[base] < offset)
    {
        return MEMORY_ACCESS_OUTSIDE_BOUNDS_ERROR;
    }

    return 0;
}

int allocate(struct MMU *mmu, int id, int base, int offset, int maxOffset, struct PCB *controlBlock)
{
    int error;

    error = canAllocate(mmu, id, base, offset, maxOffset, controlBlock);

    if (error)
    {
        return error;
    }

    //allocating arrays to show who owns process, segmentID needed to access, and memory allocated in block
    mmu->owner[base] = controlBlock->processNum;
    mmu->segmentID[base] = id;
    mmu->offset[base] = offset;

    //increasing memory used by 1MB
    mmu->memoryUsed += 1;

    return 0;
}

int access(struct MMU *mmu, int pid, int id, int base, int offset)
{
    int error;

    error = canAccess(mmu, pid, id, base, offset);

    if (error)
    {
        return error;
    }

    return 0;
}

void deallocate(struct MMU *mmu, int pid)
{
    int pos;

    //loops over array and looks for memory owned by process
    for (pos = 0; pos < 1000; pos++)
    {
        if (mmu->owner[pos] == pid)
        {
            //removes all mmu references to this specific memory that was allocated
            mmu->owner[pos] = NOT_ALLOCATED;
            mmu->segmentID[pos] = NOT_ALLOCATED;
            mmu->offset[pos] = NOT_ALLOCATED;

            //subtracts 1MB from system memory used
            mmu->memoryUsed -= 1;
        }
    }
}

void stripMemoryValues(int associatedValue, int *values)
{
    /*
        * getting first 2 numbers from SSBBBAAA value (SS)
            * works by integer division, removing the last 5 digits from value
            * SSBBBAAA / 1000000 = SS
    */
    values[0] = associatedValue / 1000000;

    /*
        * getting middle 3 numbers from SSBBBAAA value (BBB)
            * works by integer division, removing last 3 digits from value and subtracing previous value
            * SSBBBAAA / 1000 = SSBBB
            * SS * 1000 = SS000
            * SSBBB - SS000 = BBB
    */
    values[1] = (associatedValue / 1000) - (values[0] * 1000);

    /*
        * getting last 3 numbers from SSBBBAAA value (AAA)
            * works by taking previous values and adding them together and subtracting them from SSBBBAAA
            * SS * 1000000 = SS000000
            * BBB * 1000 = BBB000
            * SS000000 + BBB000 = SSBBB000
            * SSBBBAAA - SSBBB000 = AAA
    */
    values[2] = associatedValue - ((values[0] * 1000000) + (values[1] * 1000));
}
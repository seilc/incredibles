#ifndef XMEMORYMANAGERFIXED_H
#define XMEMORYMANAGERFIXED_H

#include "xMemoryManager.h"

class xMemoryManagerFixed : public xMemoryManager
{
    xMemoryManagerFixed();

    U32 GetElementSize() const;

    void Init(void* start, U32 elements, U32 elementSize);

    void* DoAllocate(U32, U32);
    void DoFree(void* pointer);
    void* DoReallocate(void* pointer, U32, U32);
    U32 DoGetBlockSize(void*) const;

private:
    struct FixedHeader
    {
        FixedHeader* next;
    };

    FixedHeader* freeList;
    U32 elements;
    U32 elementSize;

    void InitMemory();
};

#endif
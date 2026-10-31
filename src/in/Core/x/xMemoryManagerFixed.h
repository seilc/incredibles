#ifndef XMEMORYMANAGERFIXED_H
#define XMEMORYMANAGERFIXED_H

#include "xMemoryManager.h"

class xMemoryManagerFixed : public xMemoryManager
{
public:
    void Init(void* start, U32 elements, U32 elementSize);

protected:
    virtual void* DoAllocate(U32 size, U32 options);
    virtual void DoFree(void* pointer);
    virtual void* DoReallocate(void* pointer, U32 size, U32);
    virtual U32 DoGetBlockSize(void*) const;

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

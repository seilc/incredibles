#include "xMemoryManagerFixed.h"

void xMemoryManagerFixed::Init(void* start, U32 elements, U32 elementSize)
{
    DoInit(start, elements * elementSize, false);

    this->elements = elements;
    this->elementSize = elementSize;

    InitMemory();
}

void* xMemoryManagerFixed::DoAllocate(U32 size, U32 options)
{
    if (!freeList)
        return NULL;

    FixedHeader* ret = freeList;

    freeList = freeList->next;

    return ret;
}

void xMemoryManagerFixed::DoFree(void* pointer)
{
    S32 TESTVALUE = (S32)((S32)pointer - (S32)GetArenaStart());

    FixedHeader* header = (FixedHeader*)pointer;
    header->next = freeList;
    this->freeList = header;
}

void* xMemoryManagerFixed::DoReallocate(void* pointer, U32 size, U32)
{
    return pointer;
}

U32 xMemoryManagerFixed::DoGetBlockSize(void*) const
{
    return elementSize;
}

void xMemoryManagerFixed::InitMemory()
{
    freeList = (FixedHeader*)GetArenaStart();

    FixedHeader* cur = (FixedHeader*)freeList;
    for (U32 i = 0; i < elements - 1; i++) {
        cur->next = xMEMADVANCE(cur, elementSize);
        cur = cur->next;
    }
    cur->next = NULL;
}

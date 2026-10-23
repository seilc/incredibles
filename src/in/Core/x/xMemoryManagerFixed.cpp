#include "xMemoryManagerFixed.h"

void xMemoryManagerFixed::Init(void* start, U32 elements, U32 elementSize)
{
    DoInit(start, elements * elementSize, false);

    this->elements = elements;
    this->elementSize = elementSize;

    InitMemory();
}

void* xMemoryManagerFixed::DoAllocate(U32, U32)
{
    FixedHeader* header = freeList;

    if (!header)
    {
        return NULL;
    }

    freeList = header->next;

    return header;
}

void xMemoryManagerFixed::DoFree(void* pointer)
{
    FixedHeader* header = (FixedHeader*)pointer;

    GetArenaStart();

    header->next = freeList;
    freeList = header;
}

void* xMemoryManagerFixed::DoReallocate(void* pointer, U32, U32)
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

    FixedHeader* header = freeList;

    for (U32 i = 0; i < elements - 1; i++)
    {
        header->next = xMEMADVANCE(header, elementSize);
        header = header->next;
    }

    header->next = NULL;
}
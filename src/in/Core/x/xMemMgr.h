#ifndef XMEMMGR_H
#define XMEMMGR_H

#include "iMemMgr.h"

#include <stddef.h>
#include <new>

struct xMemPool
{
    void* FreeList;
    U16 NextOffset;
    U16 Flags;
    void* UsedList;
    void (*InitCB)(xMemPool*, void*);
    void* Buffer;
    U16 Size;
    U16 NumRealloc;
    U32 Total;
};

enum xMemStaticType
{
    eMemStaticTypeUnk0
};

extern U32 gActiveHeap;

void* xMemAlloc(U32 heapID, U32 size, S32 align);
void* xMemGrowAlloc(U32 heapID, U32 size);
void* xMemPushTemp(U32 size);
S32 xMemPushBase();
S32 xMemPopBase(S32 depth);
S32 xMemGetBase();

void* operator new(size_t, xMemStaticType, U32);
void* operator new[](size_t, xMemStaticType, U32);

#define xMEMALLOC(size) xMemAlloc(gActiveHeap, (size), 0)
#define xMEMALLOCALIGN(size, align) xMemAlloc(gActiveHeap, (size), (align))
#define xMEMGROWALLOC(size) xMemGrowAlloc(gActiveHeap, (size))
#define xMEMPUSHTEMP_L(amt, assetID, line) xMemPushTemp((amt))
#define xMEMPUSHTEMP(amt, assetID) xMEMPUSHTEMP_L(amt, assetID, __LINE__)
#define xMEMPOPTEMP(pointer) iMemPopTemp((pointer))

#endif
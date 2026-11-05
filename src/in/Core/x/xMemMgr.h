#ifndef XMEMMGR_H
#define XMEMMGR_H

#include "iMemMgr.h"

#include <stddef.h>
#include <new>

struct xMemPool;
typedef void(*xMemPoolInitCallback)(xMemPool*, void*);

struct xMemPool
{
    void* FreeList;
    U16 NextOffset;
    U16 Flags;
    void* UsedList;
    xMemPoolInitCallback InitCB;
    void* Buffer;
    U16 Size;
    U16 NumRealloc;
    U32 Total;
};

enum xMemStaticType
{
    eMemStaticType_0, // unknown
};

struct xMemArea
{
    U32 addr;
    U32 size;
    U32 flags;
};

struct xMemInfo
{
    xMemArea system;
    xMemArea stack;
    xMemArea DRAM;
};

#define XMEMMAXSTACK 12

static const S32 XMEM_MAX_HEAPS = 3;

typedef void(*xMemFreeFunc)(void*);
typedef void(*xMemBaseNotifyFunc)();

struct xMemFreeLink
{
    xMemFreeLink* next;
    xMemFreeFunc func;
    void* param;
};

struct xHeapState
{
    U32 curr;
    U16 blk_ct;
    U16 pad;
    U32 used;
    U32 wasted;
    xMemFreeLink* free_funcs;
};

struct xMemBlock
{
    U32 addr;
    U32 size;
    S32 align;
};

struct xMemHeap
{
    U32 flags;
    U32 hard_base;
    U32 size;
    S16 opp_heap[2];
    xHeapState state[XMEMMAXSTACK];
    U16 state_idx;
    U16 max_blks;
    xMemBlock* blk;
    xMemBlock* lastblk;
};

extern xMemInfo gMemInfo;
extern U32 gActiveHeap;
extern xMemHeap gxHeap[XMEM_MAX_HEAPS];
extern U32 scratch_mem_used;

void xMemDebug_SoakLog(const char*);
void xMemInit();
void xMemExit();
void xMemInitHeap(xMemHeap* heap, U32 base, U32 size, U32 flags);

S32 xMemPushBase(U32 heapID);
S32 xMemPushBase();
void* xMemAllocFreeable(void* alloced_memory, S32 asked_for_size, xMemFreeFunc func, void* param);
void xMemFreeDefault(void* addr_of_ptr);
S32 xMemPopBase(U32 heapID, S32 depth);
S32 xMemPopBase(S32 depth);
S32 xMemGetBase(U32 heapID);
void xMemRegisterBaseNotifyFunc(xMemBaseNotifyFunc func);
S32 xMemGetBase();
void xMemPoolSetup(xMemPool* pool, void* buffer, U32 nextOffset, U32 flags, xMemPoolInitCallback initCB, U32 size, U32 count, U32 numRealloc);
void* xMemPoolAlloc(xMemPool* pool);
void xMemPoolFree(xMemPool* pool, void* data);
void* xMemScratchPush(U32 size, U32 align);
void xMemScratchPop(void* mem);

void* xMemGrowAlloc(U32 heapID, U32 size);

void* xMemAlloc(U32 heapID, U32 size, S32 align);

void* xMemPushTemp(U32 size);

void* operator new[](size_t, xMemStaticType, U32);

#define xMEMALLOC(size, align) xMemAlloc(gActiveHeap, (size), (align))
#define xMEMGROWALLOC(size) xMemGrowAlloc(gActiveHeap, (size))
#define xMEMPUSHTEMP(amt) xMemPushTemp((amt))
#define xMEMPOPTEMP(pointer) iMemPopTemp((pointer))

inline void* operator new(size_t size, xMemStaticType, U32)
{
    return xMEMALLOC(size, 0);
}

#endif

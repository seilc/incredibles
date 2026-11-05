#include "xMemMgr.h"

#include "xMath.h"
#include "xutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
    eMemHeapGrowsDown = 0x100,
    eMemHeapHeaderInBlock = 0x8000,
    eMemHeapBlockList = 0x10000,
};

struct xMemBlkInfo
{
    xMemBlock* header;
    U32 pre;
    U32 block;
    U32 post;
    U32 curr;
    U32 waste;
    U32 total;
};

xMemInfo gMemInfo;
U32 gActiveHeap;
xMemHeap gxHeap[XMEM_MAX_HEAPS];
xMemBaseNotifyFunc sMemBaseNotifyFunc;

void xMemDebug_SoakLog(const char*)
{
    printf("MEM FREE = %d K\n", iMemMgrGetMinFreeSpace() / 1024 - 24 * 1024);
}

void xMemInit()
{
    iMemInit();

    xMemInitHeap(&gxHeap[0], gMemInfo.DRAM.addr, gMemInfo.DRAM.size, 0x8826);
    xMemInitHeap(&gxHeap[1], gMemInfo.DRAM.addr + gMemInfo.DRAM.size, gMemInfo.DRAM.size, 0x8925);
    xMemInitHeap(&gxHeap[2], gMemInfo.DRAM.addr + gMemInfo.DRAM.size, gMemInfo.DRAM.size, 0x892A);

    gxHeap[0].opp_heap[0] = 1;
    gxHeap[0].opp_heap[1] = 2;
    gxHeap[1].opp_heap[0] = 0;
    gxHeap[1].opp_heap[1] = 2;
    gxHeap[2].opp_heap[0] = 0;
    gxHeap[2].opp_heap[1] = 1;

    gActiveHeap = 0;
}

void xMemExit()
{
    iMemExit();
}

void xMemInitHeap(xMemHeap* heap, U32 base, U32 size, U32 flags)
{
    U32 old_base = base;

    S32 align = 1 << ((flags >> 9) & 0x1F);
    if (flags & eMemHeapGrowsDown) {
        base = old_base & -align;
        size -= old_base - base;
    } else {
        base = -align & (old_base + align - 1);
        size -= base - old_base;
    }

    heap->state_idx = 0;
    heap->hard_base = base;
    heap->size = size;
    heap->flags = flags;
    heap->lastblk = NULL;

    xHeapState* sp = &heap->state[heap->state_idx];
    sp->curr = base;
    sp->blk_ct = 0;
    sp->free_funcs = NULL;

    if (flags & eMemHeapBlockList) {
        heap->max_blks = 1 << ((flags >> 14) & 1);
        heap->blk = (xMemBlock*)malloc(heap->max_blks * sizeof(xMemBlock));
        memset(heap->blk, 0, heap->max_blks * sizeof(xMemBlock));
    } else {
        heap->max_blks = 0xFFFF;
        heap->blk = NULL;
    }

    heap->opp_heap[0] = -1;
    heap->opp_heap[1] = -1;
}

static U32 xMemGetBlockInfo(xMemHeap* heap, U32 size, S32 align, xMemBlkInfo* info)
{
    S32 header_size, total, hdr, pre, block, post, dir;
    xHeapState* sp;

    sp = &heap->state[heap->state_idx];
    dir = (heap->flags & eMemHeapGrowsDown) ? -1 : 1;
    header_size = (heap->flags & eMemHeapHeaderInBlock) ? (S32)sizeof(xMemBlock) : 0;

    if (heap->flags & eMemHeapGrowsDown) {
        S32 remainder = (sp->curr - header_size - size) & (align - 1);
        hdr = -header_size;
        block = -(header_size + size + remainder);
        pre = block;
        post = block + size;
        total = -pre;
    } else {
        hdr = 0;
        S32 remainder = (sp->curr + header_size) & (align - 1);
        if (remainder == 0) remainder = align;
        block = align + header_size - remainder;
        pre = block;
        post = block + size;
        total = post;
    }

    total = (total + 3) & ~3;

    if (heap->flags & eMemHeapBlockList) {
        info->header = &heap->blk[sp->blk_ct];
    } else {
        info->header = (xMemBlock*)(sp->curr + hdr);
    }

    info->pre = sp->curr + pre;
    info->block = sp->curr + block;
    info->post = sp->curr + post;
    info->curr = sp->curr + dir * total;
    info->waste = total - (header_size + size);
    info->total = total;

    return total;
}

void* xMemGrowAlloc(U32 heapID, U32 size)
{
    U32 oldalignsize, newalignsize;
    xMemHeap* heap;
    xMemBlock* hdr;
    xHeapState* sp;
    void* memptr;

    size = (size + 3) & ~3;

    heap = &gxHeap[heapID];
    sp = &heap->state[heap->state_idx];
    hdr = heap->lastblk;

    oldalignsize = hdr->size;
    newalignsize = hdr->size + size;

    if (sp->used + (newalignsize - oldalignsize) > heap->size) {
        return NULL;
    }

    if (heap->flags & eMemHeapGrowsDown) {
        memptr = (void*)(hdr->addr - size);
        hdr->addr -= size;
        sp->curr -= newalignsize - oldalignsize;
    } else {
        memptr = (void*)(hdr->addr + hdr->size);
        sp->curr += newalignsize - oldalignsize;
    }

    sp->used += newalignsize - oldalignsize;
    hdr->size += size;
    memset(memptr, 0, size);
    return memptr;
}

void* xMemAlloc(U32 heapID, U32 size, S32 align)
{
    xMemHeap* heap;
    xMemBlock* hdr;
    xHeapState* sp;
    xMemBlkInfo info;
    U32 i, total;

    heap = &gxHeap[heapID];
    sp = &heap->state[heap->state_idx];

    align = MAX(align, 1 << ((heap->flags >> 9) & 0x1F));

    if (size == 0) {
        return (void*)0xDEADBEEF;
    }

    total = xMemGetBlockInfo(heap, size, align, &info);
    hdr = info.header;

    if (sp->used + total > heap->size) {
        return NULL;
    }

    for (i = 0; i < ARRAY_LENGTH(heap->opp_heap); i++) {
    }

    sp->curr = info.curr;
    sp->blk_ct++;
    if (heap->flags & eMemHeapBlockList) {
        return NULL;
    }

    hdr->addr = info.block;
    hdr->size = size;
    hdr->align = align;

    if (iMemMgrCheckCollisionWithMallocHeap((void*)hdr->addr, hdr->size)) {
        printf("Out of memory crash, Loc=xMemAlloc Size=%d, heap overlaps malloc pool\n", hdr->size);
    }

    sp->used += total;
    sp->wasted += info.waste;

    memset((void*)hdr->addr, 0, size);
    heap->lastblk = hdr;

    return (void*)hdr->addr;
}

S32 xMemPushBase(U32 heapID)
{
    xMemHeap* heap = &gxHeap[heapID];

    heap->state_idx++;
    xHeapState* sp = &heap->state[heap->state_idx];
    *sp = *(sp - 1);
    sp->free_funcs = NULL;

    if (sMemBaseNotifyFunc) {
        sMemBaseNotifyFunc();
    }

    return heap->state_idx - 1;
}

S32 xMemPushBase()
{
    return xMemPushBase(gActiveHeap);
}

void* xMemAllocFreeable(void* alloced_memory, S32 asked_for_size, xMemFreeFunc func, void* param)
{
    xMemFreeLink* link = (xMemFreeLink*)((U8*)alloced_memory + asked_for_size);
    link->func = func;
    link->param = param;

    xMemHeap* heap = &gxHeap[gActiveHeap];
    link->next = heap->state[heap->state_idx].free_funcs;
    heap->state[heap->state_idx].free_funcs = link;

    return alloced_memory;
}

S32 xMemPopBase(U32 heapID, S32 depth)
{
    xMemHeap* heap;
    xHeapState* sp;
    S32 new_idx;

    heap = &gxHeap[heapID];

    if (depth < 0) {
        new_idx = heap->state_idx + depth;
    } else {
        new_idx = depth;
    }

    for (S32 popped = heap->state_idx; popped > new_idx; popped--) {
        xMemFreeLink* list = heap->state[popped].free_funcs;
        while (list) {
            list->func(list->param);
            list = list->next;
        }
    }

    heap->state_idx = new_idx;
    sp = &heap->state[heap->state_idx];

    if (sMemBaseNotifyFunc) {
        sMemBaseNotifyFunc();
    }

    return heap->state_idx;
}

S32 xMemPopBase(S32 depth)
{
    return xMemPopBase(gActiveHeap, depth);
}

S32 xMemGetBase(U32 heapID)
{
    return gxHeap[heapID].state_idx;
}

void xMemRegisterBaseNotifyFunc(xMemBaseNotifyFunc func)
{
    sMemBaseNotifyFunc = func;
}

S32 xMemGetBase()
{
    return xMemGetBase(gActiveHeap);
}

static void xMemPoolAddElements(xMemPool* pool, void* buffer, U32 count)
{
    S32 i;
    void* curr = buffer;
    xMemPoolInitCallback initCB = pool->InitCB;
    U32 next = pool->NextOffset;
    U32 size = pool->Size;

    for (i = 0; i < (S32)count - 1; i++) {
        *(void**)((U8*)curr + next) = (U8*)curr + size;
        if (initCB) initCB(pool, curr);
        curr = (U8*)curr + size;
    }
    *(void**)((U8*)curr + next) = pool->FreeList;
    if (initCB) initCB(pool, curr);

    pool->FreeList = buffer;
    pool->Total += count;
}

void xMemPoolSetup(xMemPool* pool, void* buffer, U32 nextOffset, U32 flags, xMemPoolInitCallback initCB, U32 size, U32 count, U32 numRealloc)
{
    pool->FreeList = NULL;
    pool->NextOffset = nextOffset;
    pool->Flags = flags;
    pool->UsedList = NULL;
    pool->InitCB = initCB;
    pool->Buffer = buffer;
    pool->Size = size;
    pool->NumRealloc = numRealloc;
    pool->Total = 0;

    xMemPoolAddElements(pool, buffer, count);
}

void* xMemPoolAlloc(xMemPool* pool)
{
    void* retval = pool->FreeList;
    U32 next = pool->NextOffset;
    U32 flags = pool->Flags;

    if (!retval) {
        xMemPoolAddElements(pool, xMEMALLOC(pool->NumRealloc * pool->Size, 0), pool->NumRealloc);
        retval = pool->FreeList;
    }

    pool->FreeList = *(void**)((U8*)retval + next);

    if (flags & 0x1) {
        *(void**)((U8*)retval + next) = pool->UsedList;
        pool->UsedList = retval;
    }

    return retval;
}

void xMemPoolFree(xMemPool* pool, void* data)
{
    if (data) {
        void* freeList = pool->FreeList;
        U32 next = pool->NextOffset;
        U32 flags = pool->Flags;

        if (flags & 0x1) {
            void** prev = &pool->UsedList;
            void* curr = pool->UsedList;
            while (curr && curr != data) {
                prev = (void**)((U8*)curr + next);
                curr = *(void**)((U8*)curr + next);
            }
            if (curr) {
                *prev = *(void**)((U8*)curr + next);
            }
        }

        *(void**)((U8*)data + next) = freeList;
        pool->FreeList = data;
    }
}

U32 scratch_mem_used;

namespace
{
static const S32 MAX_SCRATCH_ALLOC = 128;

static U16 scratch_stack[MAX_SCRATCH_ALLOC];
static U8 scratch_buffer[0x4000] ALIGN(16);
static S32 scratch_allocations;
}

void* xMemScratchPush(U32 size, U32 align)
{
    U8* scratch = scratch_buffer;

    if (align < 4)
        align = 4;
    U32 align_mask = align - 1;

    U8* mem = (U8*)((U32)(scratch + scratch_mem_used + align_mask) & ~align_mask);
    U32 used = size + (mem - scratch);
    if (used > sizeof(scratch_buffer))
        return NULL;

    scratch_stack[scratch_allocations] = used - scratch_mem_used;
    scratch_mem_used = used;
    scratch_allocations++;

    return mem;
}

void xMemScratchPop(void* mem)
{
    scratch_allocations--;
    scratch_mem_used -= scratch_stack[scratch_allocations];
}

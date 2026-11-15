#include "xCommon.h"
#include "iMemMgr.h"

#include "xMemMgr.h"
#include "xMemoryManagerFixed.h"
#include "xMemoryManagerGeneral.h"
#include "iException.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEMP 32

namespace {

struct
{
    void* data;
    U32 size;
} tempStack[MAX_TEMP];

U32 tempCurrent;
U32 StackBase;
U32 StackSize;

}

inline U32 GetStackPointer()
{ register U32 result; asm { mr result, r1 } return result; }

namespace {

inline void* GetHeapTop()
{
    xMemHeap* heap = &gxHeap[gActiveHeap];
    xHeapState* sp = &heap->state[heap->state_idx];
    return tempCurrent == 0 ? (void*)sp->curr : xMEMADVANCE(tempStack[tempCurrent - 1].data, tempStack[tempCurrent - 1].size);
}

void iMemClearStack()
{
    U32 sp = GetStackPointer();
    S32 ei = OSDisableInterrupts();
    memset((void*)(StackBase + 4), 0x77, sp - StackBase);
    *(U32*)StackBase = 0xDEADBABE;
    OSRestoreInterrupts(ei);
    OSCheckActiveThreads();
}

class MemoryManagerMulti : public xMemoryManager
{
public:
    MemoryManagerMulti() {}

    void Init(void* start, U32 size, U32, U32 resourceArenaSize, U32 byte4Count, U32 byte8Count, U32 byte32Count, U32 byte48Count, bool debugging);
    void* GetHeapCurrentEnd() const { return heap.GetCurrentEnd(); }

    void* GetHeapArenaStart() const { return heap.GetArenaStart(); }
    void* GetHeapArenaEnd() const { return heap.GetArenaEnd(); }

    S32 GetMinFreeSpace() const;

protected:
    virtual void* DoAllocate(U32 size, U32 options);
    virtual void DoFree(void* pointer);
    virtual void* DoReallocate(void* pointer, U32 size, U32 options);
    virtual U32 DoGetBlockSize(void* pointer) const;
    virtual void HandleOutOfMemory(U32 size, U32 options);

private:
    void* resourceArena;
    bool resourceArenaUsed;
    U32 resourceArenaSize;
    xMemoryManagerFixed fixed4;
    xMemoryManagerFixed fixed8;
    xMemoryManagerFixed fixed32;
    xMemoryManagerFixed fixed48;
    xMemoryManagerGeneral heap;
};

void MemoryManagerMulti::Init(void* start, U32 size, U32, U32 resourceArenaSize, U32 byte4Count, U32 byte8Count, U32 byte32Count, U32 byte48Count, bool debugging)
{
    DoInit(start, size, debugging);

    U8* startByte = (U8*)start;

    resourceArena = xMEMALIGN(startByte, 32);
    resourceArenaUsed = false;
    this->resourceArenaSize = resourceArenaSize + GetDebugDataSize();
    startByte += this->resourceArenaSize;

    fixed4.Init(xMEMALIGN(startByte, 4), byte4Count, 4 + GetDebugDataSize());
    startByte = (U8*)fixed4.GetArenaEnd();

    fixed8.Init(xMEMALIGN(startByte, 8), byte8Count, 8 + GetDebugDataSize());
    startByte = (U8*)fixed8.GetArenaEnd();

    fixed32.Init(xMEMALIGN(startByte, 32), byte32Count, 32 + GetDebugDataSize());
    startByte = (U8*)fixed32.GetArenaEnd();

    fixed48.Init(xMEMALIGN(startByte, 48), byte48Count, 48 + GetDebugDataSize());
    startByte = (U8*)fixed48.GetArenaEnd();

    startByte = xMEMALIGN(startByte, 32);

    size -= startByte - (U8*)start;
    heap.Init(startByte, size - 16, xMemoryManagerGeneral::Down, xMemoryManagerGeneral::FirstFit, false);
}

void* MemoryManagerMulti::DoAllocate(U32 size, U32 options)
{
    if (size == resourceArenaSize && !resourceArenaUsed) {
        resourceArenaUsed = true;
        return resourceArena;
    }

    if (size <= fixed4.GetElementSize()) {
        void* test = fixed4.Allocate(size, options);
        if (test) {
            return test;
        }
    }

    if (size <= fixed8.GetElementSize()) {
        void* test = fixed8.Allocate(size, options);
        if (test) {
            return test;
        }
    }

    if (size <= fixed32.GetElementSize()) {
        void* test = fixed32.Allocate(size, options);
        if (test) {
            return test;
        }
    }

    if (size <= fixed48.GetElementSize()) {
        void* test = fixed48.Allocate(size, options);
        if (test) {
            return test;
        }
    }

    if (size <= 48) {
        size = (size + 47) & ~31;
        size -= 16;
    }

    void* raw = heap.Allocate(size, options);

    if (raw < GetHeapTop()) {
        char buffer[128];

        sprintf(buffer, "Heap overlaps malloc pool\nTop: %08X; Ptr: %08X\nOptions: %08X", GetHeapTop(), raw, options);
        iExceptionMemCrash("MemoryManagerMulti", size, buffer);
        OSReport("### Over by %d bytes (%6.2f KB)\n", (U32)raw - (U32)GetHeapTop(), ((U32)raw - (U32)GetHeapTop()) / 1024.0f);
    }

    return raw;
}

void* MemoryManagerMulti::DoReallocate(void* pointer, U32 size, U32 options)
{
    if (fixed4.Owns(pointer) && size <= fixed4.GetElementSize())
        return pointer;
    if (fixed8.Owns(pointer) && size > fixed4.GetElementSize() && size <= fixed8.GetElementSize())
        return pointer;
    if (fixed32.Owns(pointer) && size > fixed8.GetElementSize() && size <= fixed32.GetElementSize())
        return pointer;
    if (fixed48.Owns(pointer) && size > fixed32.GetElementSize() && size <= fixed48.GetElementSize())
        return pointer;

    if (heap.Owns(pointer))
        return heap.Reallocate(pointer, size, options);

    return xMemoryManager::DoReallocate(pointer, size, options);
}

void MemoryManagerMulti::DoFree(void* pointer)
{
    if (pointer < fixed4.GetArenaStart()) {
        resourceArenaUsed = false;
    }
    else if (pointer < fixed4.GetArenaEnd())
        fixed4.Free(pointer);
    else if (pointer < fixed8.GetArenaEnd()) {
        fixed8.Free(pointer);
    }
    else if (pointer < fixed32.GetArenaEnd()) {
        fixed32.Free(pointer);
    }
    else if (pointer < fixed48.GetArenaEnd()) {
        fixed48.Free(pointer);
    } else {
        heap.Free(pointer);
    }
}

U32 MemoryManagerMulti::DoGetBlockSize(void* pointer) const
{
    if (pointer < fixed4.GetArenaStart()) {
        return resourceArenaSize;
    }

    if (pointer < fixed4.GetArenaEnd())
        return fixed4.GetBlockSize(pointer);
    if (pointer < fixed8.GetArenaEnd())
        return fixed8.GetBlockSize(pointer);
    if (pointer < fixed32.GetArenaEnd())
        return fixed32.GetBlockSize(pointer);
    if (pointer < fixed48.GetArenaEnd())
        return fixed48.GetBlockSize(pointer);

    return heap.GetBlockSize(pointer);
}

S32 MemoryManagerMulti::GetMinFreeSpace() const
{
    xMemHeap* hp = &gxHeap[gActiveHeap];
    xHeapState* sp = &hp->state[hp->state_idx];

    return (U32)heap.GetCurrentEnd() - sp->curr;
}

void MemoryManagerMulti::HandleOutOfMemory(U32 size, U32 options)
{
    char optional[256];

    sprintf(optional, "options=%08X", options);

    iExceptionMemCrash("xMemoryManager::Allocate", size, optional);
}

MemoryManagerMulti memoryManager;

}

static bool memInited;
S32 the_heap;
void* hiheap_start;
void* hiheap_end;
U32 hiheap_size;

extern char _stack_end[];
extern char _stack_addr[];

void iMemInit()
{
    if (memInited) {
        return;
    }

    void* rhs, *heap_end;
    U32 HeapBase, HeapSize;

    new (&memoryManager) MemoryManagerMulti;

    tempCurrent = 0;

    hiheap_start = (void*)(((U32)OSGetArenaLo() + 31) & ~31);
    hiheap_end = (void*)((U32)OSGetArenaHi() & ~31);

    heap_end = (void*)((U32)hiheap_start + 150 * 1024);
    rhs = OSInitAlloc(hiheap_start, heap_end, 1);
    the_heap = OSCreateHeap(rhs, heap_end);
    if (the_heap >= 0) {
        OSSetCurrentHeap(the_heap);
    } else {
        exit(-5);
    }

    hiheap_start = (void*)((U32)hiheap_start + 150 * 1024);
    hiheap_size = (U32)hiheap_end - (U32)hiheap_start;

    memoryManager.Init(hiheap_start, hiheap_size, 0, 512 * 1024, 768, 768, 512, 850, false);

    HeapBase = (U32)memoryManager.GetHeapArenaStart();
    HeapSize = (U32)memoryManager.GetHeapArenaEnd() - (U32)memoryManager.GetHeapArenaStart();

    StackBase = (U32)_stack_end;
    StackSize = (U32)_stack_addr - (U32)_stack_end;

    iMemClearStack();

    gMemInfo.system.addr = 0;
    gMemInfo.system.size = 1024 * 1024;
    gMemInfo.system.flags = 0x20;

    gMemInfo.stack.addr = (U32)_stack_addr;
    gMemInfo.stack.size = (U32)_stack_end - (U32)_stack_addr;
    gMemInfo.stack.flags = 0x820;

    gMemInfo.DRAM.addr = HeapBase;
    gMemInfo.DRAM.size = HeapSize;
    gMemInfo.DRAM.flags = 0x820;

    memInited = true;
}

void iMemExit()
{
    free((void*)gMemInfo.DRAM.addr);
    gMemInfo.DRAM.addr = NULL;
}

static bool sMemMgrEnableRenderWareMemoryHack;

void iMemMgrEnableRenderWareMemoryHack(bool hackEnabled)
{
    sMemMgrEnableRenderWareMemoryHack = hackEnabled;
}

void* iMemMgrMalloc(unsigned long size, U32)
{
    if (size == 200 && sMemMgrEnableRenderWareMemoryHack) {
        return NULL;
    }

    if (size > 48) {
        size = (size + 47) & ~31;
        size -= 16;
    }

    void* ret = memoryManager.Allocate(size, 1);

    return ret;
}

void* iMemMgrRealloc(void* mem, unsigned long newSize, U32)
{
    void* ret;

    if (newSize > 48) {
        newSize = (newSize + 47) & ~31;
        newSize -= 16;
    }

    ret = memoryManager.Reallocate(mem, newSize, 0);

    return ret;
}

void iMemMgrFree(void* mem)
{
    memoryManager.Free(mem);
}

void* iMemMgrCalloc(unsigned long numObj, unsigned long sizeObj, U32)
{
    return memoryManager.Allocate(numObj * sizeObj, 1);
}

bool iMemMgrCheckCollisionWithMallocHeap(void* mem, U32 size)
{
    bool ret = xMEMADVANCE(mem, size) >= memoryManager.GetHeapCurrentEnd() && mem < memoryManager.GetArenaEnd();

    if (ret) {
        return true;
    }

    return false;
}

void* iMemPushTemp(U32 size)
{
    size = (size + 15) & ~15;

    if (tempCurrent) {
        if (tempCurrent >= MAX_TEMP) {
            return NULL;
        }

        tempStack[tempCurrent].data = xMEMADVANCE(tempStack[tempCurrent - 1].data, tempStack[tempCurrent - 1].size);
    } else {
        xMemHeap* heap = &gxHeap[gActiveHeap];
        xHeapState* sp = &heap->state[heap->state_idx];
        tempStack[0].data = xMEMALIGN((void*)sp->curr, 16);
    }

    tempStack[tempCurrent].size = size;

    void* end = xMEMADVANCE(tempStack[tempCurrent].data, tempStack[tempCurrent].size);

    if (end > memoryManager.GetHeapCurrentEnd()) {
        tempStack[tempCurrent].size = 0;
        return NULL;
    }

    return tempStack[tempCurrent++].data;
}

void iMemPopTemp(void* pointer)
{
    for (S32 i = 0; i < tempCurrent; i++)
        if (tempStack[i].data == pointer) {
            tempStack[i].data = NULL;
            tempStack[i].size = 0;

            if (i + 1 == tempCurrent) {
                tempCurrent--;
                while (tempCurrent && !tempStack[tempCurrent - 1].data)
                    tempCurrent--;
            }
        }
}

S32 iMemMgrGetMinFreeSpace()
{
    return memoryManager.GetMinFreeSpace();
}

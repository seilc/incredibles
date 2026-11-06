#include "xMemoryManager.h"

namespace {
    S32 zeroMemoryAddress = 0;

    U32 MEM_CLEAR_VALUE = 0xD00D00D5;
    U32 MEM_DEALLOC_VALUE = 0x1A13D00D;
}

void* xMemoryManager::Allocate(U32 size, U32 options)
{
    if (size == 0)
        return &zeroMemoryAddress;

    size = xALIGN(size, 4);

    void* memory = DoAllocate(size + debugDataSize, options);
    if (!memory) {
        HandleOutOfMemory(size, options);
        return NULL;
    }

    if (IsDebugging()) {
        memory = SetupDebugBlock(memory, size, NULL, NULL, 0);
        if (!(options & 0x1))
            xMemorySetV32A32(memory, MEM_CLEAR_VALUE, size);
    }

    if (options & 0x1)
        xMemorySetV32A32(memory, 0, size);

    return memory;
}

void xMemoryManager::Free(void* pointer)
{
    if (!pointer || pointer == &zeroMemoryAddress)
        return;

    if (IsDebugging()) {
        U32 size;
        pointer = RemoveDebugBlock(pointer, &size);
        xMemorySetV32A32(pointer, MEM_DEALLOC_VALUE, size);
    }

    DoFree(pointer);
}

void* xMemoryManager::Reallocate(void* pointer, U32 size, U32 options)
{
    if (!pointer)
        return Allocate(size, options);

    if (size == 0) {
        if (pointer != &zeroMemoryAddress)
            Free(pointer);
        return &zeroMemoryAddress;
    }

    if (IsDebugging())
        pointer = RemoveDebugBlock(pointer, NULL);

    size = xALIGN(size, 4);

    void* newBlock = DoReallocate(pointer, size + debugDataSize, options);
    if (!newBlock) {
        HandleOutOfMemory(size, options);
        return NULL;
    }

    if (IsDebugging())
        newBlock = SetupDebugBlock(newBlock, size, NULL, NULL, 0);

    return newBlock;
}

U32 xMemoryManager::GetBlockSize(void* pointer) const
{
    if (pointer == &zeroMemoryAddress)
        return 0;

    return DoGetBlockSize(pointer) - debugDataSize;
}

void xMemoryManager::DoInit(void* start, U32 size, bool debugging)
{
    arenaStart = start;
    arenaEnd = (void*)((U32)start + size);
    this->size = size;
    debugDataSize = debugging ? (sizeof(DebugAllocationHeader) + sizeof(DebugAllocationTrailer)) : 0;
    activeList = NULL;
}

void xMemoryManager::HandleOutOfMemory(U32 size, U32 options)
{
}

void* xMemoryManager::DoReallocate(void* pointer, U32 size, U32 options)
{
    U32 copySize = DoGetBlockSize(pointer);
    if (copySize > size)
        copySize = size;

    void* newBlock = DoAllocate(size, options);
    if (!newBlock)
        return NULL;

    xMemoryCopyUpA32(newBlock, pointer, copySize);
    DoFree(pointer);

    return newBlock;
}

void* xMemoryManager::SetupDebugBlock(void* memory, U32 size, const char* file, const char* function, S32 line)
{
    DebugAllocationHeader* header = (DebugAllocationHeader*)memory;
    DebugAllocationTrailer* trailer = (DebugAllocationTrailer*)xMEMADVANCE(header+1, size);

    header->file = file;
    header->function = function;
    header->line = line;
    header->size = size;
    header->next = activeList;
    header->prev = NULL;
    header->manager = this;

    if (activeList)
        activeList->prev = header;

    activeList = header;

    for (S32 i = 0; i < header->NUM_MAGIC; i++)
        header->magic[i] = header->MAGIC;

    for (S32 i = 0; i < trailer->NUM_MAGIC; i++)
        trailer->magic[i] = trailer->MAGIC;

    return (void*)(header+1);
}

void* xMemoryManager::RemoveDebugBlock(void* memory, U32* size)
{
    DebugAllocationHeader* header = (DebugAllocationHeader*)((U32)memory - sizeof(DebugAllocationHeader));
    DebugAllocationTrailer* trailer = (DebugAllocationTrailer*)xMEMADVANCE(memory, header->size);

    for (S32 i = 0; i < header->NUM_MAGIC; i++) {
    }

    for (S32 i = 0; i < trailer->NUM_MAGIC; i++) {
    }

    if (header->prev)
        header->prev->next = header->next;
    else
        activeList = header->next;

    if (header->next)
        header->next->prev = header->prev;

    if (size)
        *size = header->size + debugDataSize;

    return (void*)header;
}

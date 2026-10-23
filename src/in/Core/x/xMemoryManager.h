#ifndef XMEMORYMANAGER_H
#define XMEMORYMANAGER_H

#include <types.h>

class xMemoryManager
{
public:
    xMemoryManager();

    void* GetArenaStart() const;
    void* GetArenaEnd() const;
    U32 GetDebugDataSize() const;
    bool IsDebugging() const;
    bool Owns(const void*) const;

    void* Allocate(U32 size, U32 options);
    void Free(void* pointer);
    void* Reallocate(void* pointer, U32 size, U32 options);
    U32 GetBlockSize(void* pointer) const;

    virtual void* DoAllocate(U32 size, U32 options) = 0;
    virtual void DoFree(void* pointer) = 0;
    virtual void* DoReallocate(void* pointer, U32 size, U32 options);
    virtual U32 DoGetBlockSize(void* pointer) const = 0;
    void DoInit(void* start, U32 size, bool debugging);
    virtual void HandleOutOfMemory(U32 size, U32 options);

    void* SetupDebugBlock(void* memory, U32 size, const char* file, const char* function,
                          S32 line);
    void* RemoveDebugBlock(void* memory, U32* size);

private:
    struct DebugAllocationHeader
    {
        const char* file;
        S32 line;
        const char* function;
        U32 size;
        DebugAllocationHeader* prev;
        DebugAllocationHeader* next;
        xMemoryManager* manager;
        //U32 magic[5];
        // This is an array of 1 in the GC version
        // This must be an array and not just "U32 magic;"
        // or else xMemoryManager::SetupDebugBlock won't match
        U32 magic[1];
    };

    struct DebugAllocationTrailer
    {
        //U32 magic[4];
        // This is an array of 8 in the GC version
        U32 magic[8];
    };

    void* arenaStart;
    void* arenaEnd;
    U32 size;
    const char* lastFile;
    const char* lastFunction;
    S32 lastLine;
    U32 debugDataSize;
    DebugAllocationHeader* activeList;
};

#define XMEMORYMANAGER_CLEAR 0x1

U32 xALIGN(U32 size, U32 align);
void xMemoryCopyUpA32(void* dst, const void* src, U32 size);
void xMemorySetV32A32(void* dst, U32 val, U32 size);

template <class T> inline static T* xMEMADVANCE(T* ptr, U32 size)
{
    return (T*)((char*)ptr + size);
}

#endif
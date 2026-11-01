#ifndef XMEMORYMANAGER_H
#define XMEMORYMANAGER_H

#include "xMemory.h"

class xMemoryManager
{
public:
    bool Owns(const void* pointer) const;
    void* GetArenaStart() const;
    void* GetArenaEnd() const;

    bool IsDebugging() const
    {
        return debugDataSize != 0;
    }

    U32 GetBlockSize(void* pointer) const;

    void* Allocate(U32 size, U32 options);

    void Free(void* pointer);

    void* Reallocate(void* pointer, U32 size, U32 options);

protected:
    void DoInit(void* start, U32 size, bool debugging);

    virtual void* DoAllocate(U32 size, U32 options) = 0;

    virtual void DoFree(void* pointer) = 0;

    virtual void* DoReallocate(void* pointer, U32 size, U32 options);

    virtual U32 DoGetBlockSize(void* pointer) const = 0;

    virtual void HandleOutOfMemory(U32 size, U32 options);

private:
    struct DebugAllocationHeader
    {
        static const U32 MAGIC = 0xDEADBEEF;
        static const U32 NUM_MAGIC = 1;

        const char* file;
        S32 line;
        const char* function;
        U32 size;
        DebugAllocationHeader* prev;
        DebugAllocationHeader* next;
        xMemoryManager* manager;
        U32 magic[NUM_MAGIC];
    };

    struct DebugAllocationTrailer
    {
        static const U32 MAGIC = 0x31173D0D;
        static const U32 NUM_MAGIC = 8;

        U32 magic[NUM_MAGIC];
    };

    void* arenaStart;
    void* arenaEnd;
    U32 size;
    const char* lastFile;
    const char* lastFunction;
    S32 lastLine;
    U32 debugDataSize;
    DebugAllocationHeader* activeList;

    void* SetupDebugBlock(void* memory, U32 size, const char* file, const char* function, S32 line);
    void* RemoveDebugBlock(void* memory, U32* size);
};

#endif

#ifndef XMEMORYMANAGER_H
#define XMEMORYMANAGER_H

#include "xMemory.h"

#include <dolphin.h>

#if DEBUG || RELEASE
#define xMEMORYMANAGERASSERT(line, cond)                                                          \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        OSReport("assert: %d (%s#%d)\n", #cond, __FILE__, (line));                                \
        while (1);                                                                                \
    }                                                                                             \
} while (0)
#else
#define xMEMORYMANAGERASSERT(line, cond)
#endif

static const U32 xMEMORYOPT_ALIGN_MASK = 0x3C;
static const U32 xMEMORYOPT_ALIGN_SHIFT = 2;

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
    bool InActiveList(const void* pointer, U32) const;
    bool IsValidPointer(const void* pointer) const;
    void DumpActiveList() const;

#if DEBUG || RELEASE
    void* Allocate(U32 size, U32 options, const char* file, const char* function, S32 line);
#else
    void* Allocate(U32 size, U32 options);
#endif

    void Free(void* pointer);

#if DEBUG || RELEASE
    void* Reallocate(void* pointer, U32 size, U32 options, const char* file, const char* function, S32 line);
#else
    void* Reallocate(void* pointer, U32 size, U32 options);
#endif

protected:
    void DoInit(void* start, U32 size, bool debugging);

#if DEBUG || RELEASE
    virtual void* DoAllocate(U32 size, U32 options, const char*, const char*, S32) = 0;
#else
    virtual void* DoAllocate(U32 size, U32 options) = 0;
#endif

    virtual void DoFree(void* pointer) = 0;

#if DEBUG || RELEASE
    virtual void* DoReallocate(void* pointer, U32 size, U32 options, const char* file, const char* function, S32 line);
#else
    virtual void* DoReallocate(void* pointer, U32 size, U32 options);
#endif

    virtual U32 DoGetBlockSize(void* pointer) const = 0;

#if DEBUG || RELEASE
    virtual void HandleOutOfMemory(U32 size, U32 options, const char* file, const char* function, S32 line);
#else
    virtual void HandleOutOfMemory(U32 size, U32 options);
#endif

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
#if DEBUG || RELEASE
    U32 callsToAllocate;
    U32 callsToReallocate;
    U32 callsToFree;
#endif
    const char* lastFile;
    const char* lastFunction;
    S32 lastLine;
    U32 debugDataSize;
    DebugAllocationHeader* activeList;
#if DEBUG || RELEASE
    U32 debugOverhead;
#endif

    void* SetupDebugBlock(void* memory, U32 size, const char* file, const char* function, S32 line);
    void* RemoveDebugBlock(void* memory, U32* size);
};

#endif

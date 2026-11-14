#include "xCommon.h"
#include "iARAMTmp.h"

#include "iSystem.h"
#include "iTime.h"
#include "iTRC.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zScene.h"

#include <string.h>

#define MAX_MEMBLOCKS 32

struct memBlock
{
    U32 base;
    U32 size;
    volatile U32 addr;
    void* aram;
    ARQRequest xfer;
};

struct tempTransfer
{
    memBlock blockList[MAX_MEMBLOCKS];
    U32 numBlocks;
    U32 type;
};

static tempTransfer activeTransfers[2];

static void transferDoneCB(u32 pointerToARQRequest)
{
    ARQRequest* arq = (ARQRequest*)pointerToARQRequest;
    memBlock* bp = (memBlock*)arq->owner;
    if (bp->addr == 0) {
        bp->addr = OSRoundUp32B(bp->base);
    } else {
        bp->addr = 0;
    }
}

U32 iARAMTmpBegin(U32 type, U32 max, bool doTRCCheck)
{
    if (!xSTIsLoaded() || zSceneIsLoadingPackFile()) {
        return 0;
    }

    S32 which;
    for (which = 0; which < ARRAY_LENGTH(activeTransfers); which++) {
        if (activeTransfers[which].type == type) {
            return 0;
        }

        if (activeTransfers[which].type == 0) {
            break;
        }
    }

    if (which == ARRAY_LENGTH(activeTransfers)) {
        return 0;
    }

    activeTransfers[which].type = type;

    iTime t0 = iTimeGet();

    ARQInit();

    memBlock* blockList = activeTransfers[which].blockList;
    U32& numBlocks = activeTransfers[which].numBlocks;

    // BUG: clears only the first 4 bytes
    memset(blockList, 0, sizeof(blockList));
    numBlocks = 0;
    S32 num = xSTAssetCountByType(type);

    S32 total = 0;
    S32 i, j;
    S32 totalSize = 0;
    for (i = 0; i < num && totalSize < max; i++) {
        PKRAssetTOCInfo ainfo;
        xSTGetAssetInfoByType(type, i, &ainfo);

        U32 base = (U32)ainfo.mempos;
        U32 size = ainfo.size;
        U32 alignedBase = base & ~31;
        size += base - alignedBase;
        base = alignedBase;
        size = OSRoundUp32B(size);
        totalSize += size;

        memBlock* bp = &blockList[numBlocks];
        for (j = numBlocks; --bp, j--;) {
            if (base > bp->base && base - (bp->base + bp->size) < 16) {
                bp->size = base + size - bp->base;
                break;
            }

            if (base > bp->base && base < bp->base + bp->size) {
                bp->size = base + size - bp->base;
                break;
            }

            if (bp->base > base && bp->base - (base + size) < 16) {
                bp->size = bp->base + bp->size - base;
                bp->base = base;
                break;
            }

            if (bp->base > base && bp->base <= base + size) {
                bp->size = bp->base + bp->size - base;
                bp->base = base;
                break;
            }
        }

        if (j < 0) {
            if (numBlocks < MAX_MEMBLOCKS) {
                bp = &blockList[numBlocks];
                bp->base = base;
                bp->size = size;
                numBlocks++;
            } else {
                totalSize -= size;
            }
        }
    }

    memBlock* bp = blockList;
    for (i = 0; i < numBlocks; i++, bp++) {
        total += bp->size;
        S32 size = OSRoundUp32B(bp->size);

        bp->aram = iSystemARAMAlloc(size);

        DCFlushRange((void*)bp->base, size);

        U32 owner = (U32)bp;
        ARQPostRequest(&bp->xfer, owner, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH, bp->base, (U32)bp->aram, size,
                       transferDoneCB);

        while (bp->addr == 0) {
            if (doTRCCheck) {
                iTRC_CHECK();
            }
        }
    }

    iTime t1 = iTimeGet();

    if (total == 0) {
        activeTransfers[which].type = 0;
    }

    return total;
}

void* iARAMTmpMalloc(U32 type, U32 size)
{
    S32 which;
    for (which = 0; which < ARRAY_LENGTH(activeTransfers); which++) {
        if (activeTransfers[which].type == type) {
            break;
        }
    }

    if (which == ARRAY_LENGTH(activeTransfers)) {
        return (void*)0xBEEEEEEF;
    }

    size = OSRoundUp32B(size);
    memBlock* bp = activeTransfers[which].blockList;
    for (S32 i = 0; i < activeTransfers[which].numBlocks; i++, bp++) {
        if (bp->addr + size <= bp->base + bp->size) {
            (void)bp->addr;
            U32 addr = bp->addr;
            bp->addr += size;
            return (void*)addr;
        }
    }

    return (void*)0xBEEEEEEF;
}

void iARAMTmpFree(U32, void*)
{
}

S32 iARAMTmpInARAMTemp(U32 type, void* ptr)
{
    S32 which;
    for (which = 0; which < ARRAY_LENGTH(activeTransfers); which++) {
        if (activeTransfers[which].type == type) {
            break;
        }
    }

    if (which == ARRAY_LENGTH(activeTransfers)) {
        return FALSE;
    }

    memBlock* bp = activeTransfers[which].blockList;
    for (S32 i = 0; i < activeTransfers[which].numBlocks; i++, bp++) {
        if ((U32)ptr >= bp->base && (U32)ptr < bp->base + bp->size)
            return TRUE;
    }

    return FALSE;
}

void iARAMTmpEnd(U32 type, bool doTRCCheck)
{
    S32 which;
    for (which = 0; which < ARRAY_LENGTH(activeTransfers); which++) {
        if (activeTransfers[which].type == type) {
            break;
        }
    }

    if (which == ARRAY_LENGTH(activeTransfers)) {
        return;
    }

    iTime t0 = iTimeGet();

    activeTransfers[which].type = 0;
    memBlock* bp = activeTransfers[which].blockList;
    S32 total = 0;
    for (S32 i = 0; i < activeTransfers[which].numBlocks; i++, bp++) {
        DCInvalidateRange((void*)bp->base, bp->size);

        U32 owner = (U32)bp;
        ARQPostRequest(&bp->xfer, owner, ARQ_TYPE_ARAM_TO_MRAM, ARQ_PRIORITY_HIGH, (U32)bp->aram, bp->base, bp->size,
                       transferDoneCB);

        total += bp->size;

        while (bp->addr != 0) {
            if (doTRCCheck) {
                iTRC_CHECK();
            }
        }

        iSystemARAMFree(bp->aram);
    }

    iTime t1 = iTimeGet();
}

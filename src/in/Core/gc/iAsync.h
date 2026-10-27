#ifndef IASYNC_H
#define IASYNC_H

#include "types.h"

#include <dolphin.h>

extern OSSemaphore asyncSema;
extern bool needToLock;

inline void iAsyncLockRW()
{
    if (needToLock) {
        OSWaitSemaphore(&asyncSema);
    }
}

inline void iAsyncUnlockRW()
{
    if (needToLock) {
        OSSignalSemaphore(&asyncSema);
    }
}

struct iAsyncLockRWSentry
{
    iAsyncLockRWSentry()
    {
        iAsyncLockRW();
    }

    ~iAsyncLockRWSentry()
    {
        iAsyncUnlockRW();
    }
};

#endif

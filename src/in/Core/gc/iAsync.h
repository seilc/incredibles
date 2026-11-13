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

void iAsyncStart(void* (*func)(void*));
S32 iAsyncIsDone();
void iAsyncTerminate();
void iAsyncBeginUpdate();
void iAsyncEndUpdate();
void iAsyncSuspend();
void iAsyncResume();
void iAsyncEnd();

struct iAsyncLockRWSentry
{
    iAsyncLockRWSentry() { iAsyncLockRW(); }

    ~iAsyncLockRWSentry() { iAsyncUnlockRW(); }
};

#endif

#include "xCommon.h"
#include "iAsync.h"

#include "iSystem.h"
#include "xPad.h"
#include "xutil.h"

#include <rwcore.h>

RwLinkList asyncDirtyFrameList;
bool asyncThreadDone = true;
OSThread asyncThread;
static U8 ThreadStack[8192];
OSSemaphore asyncSema;
bool needToLock;

void iAsyncStart(void* (*func)(void*))
{
    rwLinkListInitialize(&asyncDirtyFrameList);
    asyncThreadDone = false;
    OSInitSemaphore(&asyncSema, 1);
    needToLock = true;

    OSCreateThread(&asyncThread, func, NULL, ThreadStack + sizeof(ThreadStack), sizeof(ThreadStack), 10,
                   OS_THREAD_ATTR_DETACH);

    OSResumeThread(&asyncThread);
    OSCheckActiveThreads();
}

S32 iAsyncIsDone()
{
    return asyncThreadDone;
}

void iAsyncTerminate()
{
    OSCancelAlarms(IDTAG('A','S','Y','N'));
    asyncThreadDone = true;
    needToLock = false;

    OSCheckActiveThreads();
}

void iAsyncBeginUpdate()
{
    OSCancelAlarms(IDTAG('A','S','Y','N'));
    xPadUpdate(0, 0.0f);
}

void iAsyncEndUpdate()
{
    iSystemSleep(50, IDTAG('A','S','Y','N'));
}

void iAsyncSuspend()
{
    OSCancelAlarms(IDTAG('A','S','Y','N'));
    if (!asyncThreadDone && !OSIsThreadSuspended(&asyncThread)) {
        OSSuspendThread(&asyncThread);
    }
}

void iAsyncResume()
{
    if (!asyncThreadDone && OSIsThreadSuspended(&asyncThread)) {
        OSResumeThread(&asyncThread);
    }
}

void iAsyncEnd()
{
    OSCancelAlarms(IDTAG('A','S','Y','N'));

    if (asyncThreadDone && OSIsThreadSuspended(&asyncThread)) {
        OSResumeThread(&asyncThread);
    }
}

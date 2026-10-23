#ifndef XTIMER_H
#define XTIMER_H

#include "xBase.h"

struct xTimerAsset : xBaseAsset
{
    F32 seconds;
    F32 randomRange;
};

struct xTimer : xBase
{
    xTimerAsset* tasset;
    U8 state;
    U8 runsInPause;
    U16 flags;
    F32 secondsLeft;
};

#define XTIMER_STATE_STOPPED 0
#define XTIMER_STATE_RUNNING 1

#define XTIMER_FLAGS_0x1 0x1

#define xTimerIsUnk0x1(t) ((t)->flags & XTIMER_FLAGS_0x1)
#define xTimerIsRunning(t) ((t)->state == XTIMER_STATE_RUNNING)

struct xScene;

void xTimerInit(void* b, void* tasset);
void xTimerInit(xBase* b, xTimerAsset* tasset);
void xTimerReset(xTimer* t);
void xTimerSave(xTimer* ent, xSerial* s);
void xTimerLoad(xTimer* ent, xSerial* s);
void xTimerEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase*, U32);
void xTimerUpdate(xBase* to, xScene*, F32 dt);

#endif
#include "xCommon.h"
#include "xTimer.h"

#include "xEvent.h"
#include "xRand.h"
#include "zHashes.h"
#include "zMain.h"
#include "zTalkBox.h"

static U32 sPauseTimerHash[] = {
    HASH_PAUSE_TIMER00, HASH_PAUSE_TIMER09,
    HASH_PAUSE_TIMER10, HASH_PAUSE_TIMER19,
    HASH_PAUSE_TIMER20, HASH_PAUSE_TIMER29,
    HASH_PAUSE_TIMER30, HASH_PAUSE_TIMER39,
    HASH_PAUSE_TIMER40, HASH_PAUSE_TIMER49,
    HASH_PAUSE_TIMER50, HASH_PAUSE_TIMER59,
    HASH_PAUSE_TIMER60, HASH_PAUSE_TIMER69,
    HASH_PAUSE_TIMER70, HASH_PAUSE_TIMER79,
    HASH_PAUSE_TIMER80, HASH_PAUSE_TIMER89,
    HASH_PAUSE_TIMER90, HASH_PAUSE_TIMER99,
};

static F32 GetRandomizedTime(xTimerAsset* tasset)
{
    U32 halfRangeMilli = 1000.0f * tasset->randomRange;
    if (halfRangeMilli == 0) {
        return tasset->seconds;
    }

    S32 offset = xrand_RandomInt32() % (halfRangeMilli * 2) - halfRangeMilli;
    F32 time = tasset->seconds + offset / 1000.0f;
    return time;
}

void xTimerInit(void* b, void* tasset)
{
    xTimerInit((xBase*)b, (xTimerAsset*)tasset);
}

static S32 xTimer_ObjIDIsPauseTimer(U32 id)
{
    if (id == HASH_PAUSE_TIMER)
        return TRUE;
    if (id >= HASH_PAUSE_TIMER0 && id <= HASH_PAUSE_TIMER9)
        return TRUE;

    S32 foo = (id >= HASH_PAUSE_TIMER00);
    S32 bar = (id <= HASH_PAUSE_TIMER99);
    if (foo && bar) {
        for (S32 i = 0; i < 10; i++) {
            if (id >= sPauseTimerHash[i*2] && id <= sPauseTimerHash[i*2+1]) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

void xTimerInit(xBase* b, xTimerAsset* tasset)
{
    xTimer* t = (xTimer*)b;

    xBaseInit(t, tasset);

    t->eventFunc = xTimerEventCB;
    t->tasset = tasset;

    if (t->linkCount)
        t->link = (xLinkAsset*)(t->tasset + 1);
    else
        t->link = NULL;

    t->state = 0;
    t->secondsLeft = GetRandomizedTime(tasset);
    t->runsInPause = xTimer_ObjIDIsPauseTimer(t->id);
    t->flags = 0;
}

void xTimerReset(xTimer* t)
{
    xBaseReset(t, t->tasset);

    t->state = 0;
    t->secondsLeft = GetRandomizedTime(t->tasset);
    t->flags = 0;
}

void xTimerSave(xTimer* ent, xSerial* s)
{
    xBaseSave(ent, s);

    s->Write(ent->state);
    s->Write(ent->secondsLeft);
}

void xTimerLoad(xTimer* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    s->Read(&ent->state);
    s->Read(&ent->secondsLeft);
}

void xTimerEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase*, U32)
{
    xTimer* t = (xTimer*)to;

    switch (toEvent) {
    case eEventRun:
    {
        t->state = 1;
        break;
    }
    case eEventStop:
    {
        if (t->state == 1) {
            t->state = 0;
        }

        break;
    }
    case eEventReset:
    {
        xTimerReset(t);
        break;
    }
    case eEventExpired:
    {
        t->state = 0;
        break;
    }
    case eEventTimerSet:
    {
        t->secondsLeft = toParam[0];
        break;
    }
    case eEventTimerAdd:
    {
        t->secondsLeft += toParam[0];
        break;
    }
    }
}

void xTimerUpdate(xBase* to, xScene*, F32 dt)
{
    xTimer* t = (xTimer*)to;

    if (t->state == 1) {
        if (!(t->flags & 0x1) || !globals.player.ControlOff || !ztalkbox::get_active()) {
            t->secondsLeft -= dt;

            if (t->secondsLeft <= 0.0f) {
                zEntEvent(t, t, eEventExpired);
            }
        }
    }
}
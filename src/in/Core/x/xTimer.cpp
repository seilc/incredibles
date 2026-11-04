#include "xTimer.h"

#include "xEvent.h"
#include "xRand.h"
#include "zMain.h"
#include "zTalkBox.h"

#define PAUSE_TIMER_ID 0xCB3F6340 // xStrHash("PAUSE_TIMER")
#define PAUSE_TIMER0_ID 0x016FC9F0 // xStrHash("PAUSE_TIMER0")
#define PAUSE_TIMER9_ID 0x016FC9F9 // xStrHash("PAUSE_TIMER9")
#define PAUSE_TIMER00_ID 0xBC345600 // xStrHash("PAUSE_TIMER00")
#define PAUSE_TIMER09_ID 0xBC345609 // xStrHash("PAUSE_TIMER09")
#define PAUSE_TIMER10_ID 0xBC345683 // xStrHash("PAUSE_TIMER10")
#define PAUSE_TIMER19_ID 0xBC34568C // xStrHash("PAUSE_TIMER19")
#define PAUSE_TIMER20_ID 0xBC345706 // xStrHash("PAUSE_TIMER20")
#define PAUSE_TIMER29_ID 0xBC34570F // xStrHash("PAUSE_TIMER29")
#define PAUSE_TIMER30_ID 0xBC345789 // xStrHash("PAUSE_TIMER30")
#define PAUSE_TIMER39_ID 0xBC345792 // xStrHash("PAUSE_TIMER39")
#define PAUSE_TIMER40_ID 0xBC34580C // xStrHash("PAUSE_TIMER40")
#define PAUSE_TIMER49_ID 0xBC345815 // xStrHash("PAUSE_TIMER49")
#define PAUSE_TIMER50_ID 0xBC34588F // xStrHash("PAUSE_TIMER50")
#define PAUSE_TIMER59_ID 0xBC345898 // xStrHash("PAUSE_TIMER59")
#define PAUSE_TIMER60_ID 0xBC345912 // xStrHash("PAUSE_TIMER60")
#define PAUSE_TIMER69_ID 0xBC34591B // xStrHash("PAUSE_TIMER69")
#define PAUSE_TIMER70_ID 0xBC345995 // xStrHash("PAUSE_TIMER70")
#define PAUSE_TIMER79_ID 0xBC34599E // xStrHash("PAUSE_TIMER79")
#define PAUSE_TIMER80_ID 0xBC345A18 // xStrHash("PAUSE_TIMER80")
#define PAUSE_TIMER89_ID 0xBC345A21 // xStrHash("PAUSE_TIMER89")
#define PAUSE_TIMER90_ID 0xBC345A9B // xStrHash("PAUSE_TIMER90")
#define PAUSE_TIMER99_ID 0xBC345AA4 // xStrHash("PAUSE_TIMER99")

static U32 sPauseTimerHash[] = {
    PAUSE_TIMER00_ID, PAUSE_TIMER09_ID,
    PAUSE_TIMER10_ID, PAUSE_TIMER19_ID,
    PAUSE_TIMER20_ID, PAUSE_TIMER29_ID,
    PAUSE_TIMER30_ID, PAUSE_TIMER39_ID,
    PAUSE_TIMER40_ID, PAUSE_TIMER49_ID,
    PAUSE_TIMER50_ID, PAUSE_TIMER59_ID,
    PAUSE_TIMER60_ID, PAUSE_TIMER69_ID,
    PAUSE_TIMER70_ID, PAUSE_TIMER79_ID,
    PAUSE_TIMER80_ID, PAUSE_TIMER89_ID,
    PAUSE_TIMER90_ID, PAUSE_TIMER99_ID,
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
    if (id == PAUSE_TIMER_ID) return TRUE;
    if (id >= PAUSE_TIMER0_ID && id <= PAUSE_TIMER9_ID) return TRUE;

    S32 foo = (id >= PAUSE_TIMER00_ID);
    S32 bar = (id <= PAUSE_TIMER99_ID);
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
    {
        t->link = (xLinkAsset*)(t->tasset + 1);
    }
    else
    {
        t->link = NULL;
    }

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

    switch (toEvent)
    {
    case eEventRun:
    {
        t->state = 1;
        break;
    }
    case eEventStop:
    {
        if (t->state == 1)
        {
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

    if (t->state == 1)
    {
        if (!(t->flags & 0x1) || !globals.player.ControlOff || !ztalkbox::get_active())
        {
            t->secondsLeft -= dt;

            if (t->secondsLeft <= 0.0f)
            {
                zEntEvent(t, t, eEventExpired);
            }
        }
    }
}
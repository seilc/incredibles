#include "xTimer.h"

#include "xEvent.h"
#include "xRand.h"
#include "zMain.h"
#include "zTalkBox.h"

// clang-format off
static U32 sPauseTimerHash[] =
{
    0xBC345600, 0xBC345609,
    0xBC345683, 0xBC34568C,
    0xBC345706, 0xBC34570F,
    0xBC345789, 0xBC345792,
    0xBC34580C, 0xBC345815,
    0xBC34588F, 0xBC345898,
    0xBC345912, 0xBC34591B,
    0xBC345995, 0xBC34599E,
    0xBC345A18, 0xBC345A21,
    0xBC345A9B, 0xBC345AA4
};
// clang-format on

static F32 GetRandomizedTime(xTimerAsset* tasset)
{
    U32 r31 = 1000.0f * tasset->randomRange;

    if (r31 == 0)
    {
        return tasset->seconds;
    }

    S32 r0 = (xrand_RandomInt32() % (r31 * 2)) - r31;

    return tasset->seconds + r0 / 1000.0f;
}

void xTimerInit(void* b, void* tasset)
{
    xTimerInit((xBase*)b, (xTimerAsset*)tasset);
}

static S32 xTimer_ObjIDIsPauseTimer(U32 id)
{
    if (id == 0xCB3F6340)
    {
        return TRUE;
    }

    if (id >= 0x016FC9F0 && id <= 0x016FC9F9)
    {
        return TRUE;
    }

    S32 bar = (id <= 0xBC345AA4);

    if (id >= 0xBC345600 && bar)
    {
        for (S32 i = 0; i < 10; i++)
        {
            if (id >= sPauseTimerHash[i * 2] && id <= sPauseTimerHash[i * 2 + 1])
            {
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
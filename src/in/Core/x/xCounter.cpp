#include "xCounter.h"

#include "xDebug.h"
#include "xDebugTweak.h"
#include "xEvent.h"
#include "xFont.h"

#include "zBase.h"

namespace
{
    void add_tweaks(_xCounter&)
    {
    }
} // namespace

void xCounterInit()
{
    xDebugRemoveTweak("Widgets|Counters");
}

void xCounterInit(void* b, void* asset)
{
    xCounterInit((xBase*)b, (xCounterAsset*)asset);
}

void xCounterInit(xBase* b, xCounterAsset* asset)
{
    _xCounter* t = (_xCounter*)b;

    xBaseInit(t, asset);

    t->eventFunc = xCounterEventCB;
    t->asset = asset;

    if (t->linkCount)
    {
        t->link = (xLinkAsset*)(t->asset + 1);
    }
    else
    {
        t->link = NULL;
    }

    t->state = 0;
    t->count = asset->count;

    add_tweaks(*t);
}

void xCounterReset(xBase* b)
{
    _xCounter* t = (_xCounter*)b;

    xBaseInit(t, t->asset);

    if (t->linkCount)
    {
        t->link = (xLinkAsset*)(t->asset + 1);
    }
    else
    {
        t->link = NULL;
    }

    t->count = t->asset->count;
    t->state = 0;
}

void xCounterSave(_xCounter* ent, xSerial* s)
{
    xBaseSave(ent, s);

    s->Write(ent->state);
    s->Write(ent->count);
}

void xCounterLoad(_xCounter* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    s->Read(&ent->state);
    s->Read(&ent->count);
}

void xCounterEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam,
                     xBase* toParamWidget, U32)
{
    _xCounter* t = (_xCounter*)to;

    switch (toEvent)
    {
    case eEventDecrement:
    {
        if (t->state != 1)
        {
            t->count--;

            if (t->count <= 0)
            {
                zEntEvent(t, t, eEventExpired);
            }
            else if (t->count > 0 && t->count <= 20)
            {
                zEntEvent(t, t, eEventCount1 + (U32)(t->count - 1));
            }
        }

        break;
    }
    case eEventIncrement:
    {
        if (t->state != 1)
        {
            t->count++;

            if (t->count <= 0)
            {
                zEntEvent(t, t, eEventExpired);
            }
            else if (t->count > 0 && t->count <= 20)
            {
                zEntEvent(t, t, eEventCount1 + (U32)(t->count - 1));
            }
        }

        break;
    }
    case eEventReset:
    {
        t->state = 0;
        t->count = t->asset->count;
        break;
    }
    case eEventExpired:
    {
        t->count = 0;
        t->state = 1;
        break;
    }
    case eEventSetCount:
    {
        if (t->state != 1)
        {
            if (toParamWidget && toParamWidget->baseType == eBaseTypeCounter)
            {
                t->count = ((_xCounter*)toParamWidget)->count;
            }
            else
            {
                t->count = toParam[0];
            }
        }

        break;
    }
    default:
    {
        if (t->state != 1)
        {
            if (toEvent >= eEventCount1 && toEvent <= eEventCount20)
            {
                t->count = toEvent - (eEventCount1 - 1);
            }
            else if (toEvent == eEventCount0)
            {
                t->count = 0;
            }
        }

        break;
    }
    }
}

#ifndef NO_HACKS
#pragma push
#pragma force_active off
static const char* unused_string_1 = "Counter";
static const char* unused_string_2 = "%d";
static const char* unused_string_3 = "%.*s";
static void hack_unused_function()
{
    xtextbox::jot j;
    j.reset_flags();
}
#pragma pop
#endif
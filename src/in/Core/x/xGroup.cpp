#include "xGroup.h"

#include "xMemMgr.h"
#include "xstransvc.h"
#include "xEvent.h"
#include "xRand.h"
#include "xEnt.h"
#include "zScene.h"

void xGroupInit(void* b, void* asset)
{
    xGroupInit((xBase*)b, (xGroupAsset*)asset);
}

void xGroupInit(xBase* b, xGroupAsset* asset)
{
    xGroup* g = (xGroup*)b;

    xBaseInit(g, asset);

    g->eventFunc = xGroupEventCB;
    g->asset = asset;

    if (g->linkCount)
    {
        g->link = (xLinkAsset*)((U8*)g->asset + asset->itemCount * sizeof(U32) +
                                sizeof(xGroupAsset));
    }
    else
    {
        g->link = NULL;
    }

    U32 count = xGroupGetCount(g);

    g->item = (count) ? (xBase**)xMEMALLOC(count * sizeof(xBase*)) : NULL;
    g->last_index = 0;
    g->flg_group = 0;
}

void xGroupSetup(xGroup* g)
{
    if (!(g->flg_group & 0x1))
    {
        U32 count = xGroupGetCount(g);

        for (U32 i = 0; i < count; i++)
        {
            U32 id = xGroupGetItem(g, i);

            g->item[i] = zSceneFindObject(id);

            if (g->item[i])
            {
                g->flg_group |= XGRP_FLG_HASWIDGET;
            }
            else
            {
                g->item[i] = (xBase*)xSTFindAsset(id, NULL);

                if (g->item[i])
                {
                    g->flg_group |= XGRP_FLG_HASASSET;
                }
            }
        }

        g->flg_group |= 0x1;
    }
}

void xGroupSave(xGroup* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void xGroupLoad(xGroup* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

void xGroupReset(xGroup* g)
{
    xBaseReset(g, g->asset);
}

void xGroupEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                   U32 toParamWidgetID)
{
    xGroup* g = (xGroup*)to;

    switch (toEvent)
    {
    case eEventReset:
        xGroupReset(g);
        break;
    case eEventDisableGroupContents:
        toEvent = eEventDisable;
        break;
    }

    S32 rand = -1;

    if (g->asset->groupFlags & 0x1)
    {
        rand = xrand_RandomChoice(g->asset->itemCount);
    }

    switch (toEvent)
    {
    case eEventFastVisible:
    case eEventFastInvisible:
    case eEventVisibilityCullOn:
    case eEventVisibilityCullOff:
    {
        xEntVisHandler entVisEventHandler;

        switch (toEvent)
        {
        case eEventFastVisible:
            entVisEventHandler = xEntShow;
            break;
        case eEventFastInvisible:
            entVisEventHandler = xEntHide;
            break;
        case eEventVisibilityCullOn:
            entVisEventHandler = xEntVisibilityCullOn;
            break;
        case eEventVisibilityCullOff:
            entVisEventHandler = xEntVisibilityCullOff;
            break;
        }

        for (S32 i = 0; i < g->asset->itemCount; i++)
        {
            if (rand == -1 || rand == i)
            {
                xBase* b = g->item[i];

                if (b)
                {
                    if (b->baseFlags & k_XBASE_IS_ENTITY)
                    {
                        if (xBaseIsEnabled(b))
                        {
                            entVisEventHandler((xEnt*)b);
                        }
                    }
                    else
                    {
                        zEntEvent(b, toEvent, toParam, toParamWidget, toParamWidgetID);
                    }
                }
            }
        }

        break;
    }
    default:
    {
        for (S32 i = 0; i < g->asset->itemCount; i++)
        {
            if (rand == -1 || rand == i)
            {
                xBase* b = g->item[i];

                if (b)
                {
                    zEntEvent(b, toEvent, toParam, toParamWidget, toParamWidgetID);
                }
            }
        }
    }
    }
}

U32 xGroupGetCount(xGroup* g)
{
    return g->asset->itemCount;
}

xBase* xGroupGetItemPtr(xGroup* g, U32 index)
{
    if (!(g->flg_group & 0x1))
    {
        xGroupSetup(g);
    }

    if (g->item)
    {
        return g->item[index];
    }

    return NULL;
}

U32 xGroupGetItem(xGroup* g, U32 index)
{
    return ((U32*)(g->asset + 1))[index];
}

U32 xGroup::get_any()
{
    if (!asset->itemCount)
    {
        return 0;
    }

    U32 id = ((U32*)(asset + 1))[last_index];

    last_index = (last_index + 1) % asset->itemCount;

    return id;
}
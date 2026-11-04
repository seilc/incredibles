#include "zPortal.h"

#include "zMain.h"
#include "xEvent.h"

void zPortalInit(void* portal, void* passet)
{
    zPortalInit((zPortal*)portal, (xPortalAsset*)passet);
}

void zPortalInit(zPortal* portal, xPortalAsset* passet)
{
    xBaseInit(portal, passet);

    portal->passet = passet;
    portal->eventFunc = zPortalEventCB;

    if (portal->linkCount)
    {
        portal->link = (xLinkAsset*)(portal->passet + 1);
    }
}

void zPortalReset(zPortal* portal)
{
    xBaseReset(portal, portal->passet);
}

void zPortalSave(zPortal* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zPortalLoad(zPortal* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

void zPortalEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase*, U32)
{
    switch (toEvent)
    {
    case eEventReset:
    {
        zPortalReset((zPortal*)to);
        break;
    }
    case eEventTeleportPlayer:
    {
        if (ZPLAYER->IsDead())
        {
            break;
        }

        if ((globals.demoType != zDT_PUBLICITY && globals.demoType != zDT_E3) ||
            ((zPortal*)to)->passet->sceneID == globals.sceneCur->sceneID)
        {
            zSceneSwitch((zPortal*)to, toParam[0] != 0.0f);
        }

        break;
    }
    }
}
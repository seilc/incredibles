#ifndef ZPORTAL_H
#define ZPORTAL_H

#include "xBase.h"
#include "xPortalAsset.h"

typedef struct _zPortal : xBase
{
    xPortalAsset* passet;
} zPortal;

void zPortalInit(void* portal, void* passet);
void zPortalInit(zPortal* portal, xPortalAsset* passet);
void zPortalReset(zPortal* portal);
void zPortalSave(zPortal* ent, xSerial* s);
void zPortalLoad(zPortal* ent, xSerial* s);
void zPortalEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif
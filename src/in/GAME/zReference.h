#ifndef ZREFERENCE_H
#define ZREFERENCE_H

#include "xDynAsset.h"

struct zReferenceAsset : xDynAsset
{
    U32 initial;
};

struct zReference : xBase
{
    xBase* current;
};

void zReferenceInit(xBase& data, xDynAsset& asset, ulong32);
void zReferenceInit(zReference* ref, zReferenceAsset* asset);
void zReferenceEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                       xBase* toParamWidget, U32 toParamWidgetID);

#endif
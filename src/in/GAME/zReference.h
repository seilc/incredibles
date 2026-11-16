#ifndef ZREFERENCE_H
#define ZREFERENCE_H

#include "xDynAsset.h"
#include "zReferenceAsset.h"

struct zReference : xBase
{
    xBase* current;
};

void zReferenceInit(xBase& data, xDynAsset& asset, size_t);
void zReferenceInit(zReference* ref, zReferenceAsset* asset);
void zReferenceEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                       xBase* toParamWidget, U32 toParamWidgetID);

#endif
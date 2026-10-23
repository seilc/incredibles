#ifndef XGROUP_H
#define XGROUP_H

#include "xBase.h"

struct xGroupAsset : xBaseAsset
{
    U16 itemCount;
    U16 groupFlags;
};

#define XGROUPASSET_RANDOM 0x1

struct xGroup : xBase
{
    xGroupAsset* asset;
    xBase** item;
    U32 last_index;
    S32 flg_group;

    U32 get_any();
};

#define XGROUP_IS_SETUP 0x1
#define XGROUP_HAS_BASES 0x2
#define XGROUP_HAS_ASSETS 0x4

void xGroupInit(void* b, void* asset);
void xGroupInit(xBase* b, xGroupAsset* asset);
void xGroupSetup(xGroup* g);
void xGroupSave(xGroup* ent, xSerial* s);
void xGroupLoad(xGroup* ent, xSerial* s);
void xGroupReset(xGroup* g);
void xGroupEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                   U32 toParamWidgetID);
U32 xGroupGetCount(xGroup* g);
xBase* xGroupGetItemPtr(xGroup* g, U32 index);
U32 xGroupGetItem(xGroup* g, U32 index);

#endif
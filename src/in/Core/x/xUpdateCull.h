#ifndef XUPDATECULL_H
#define XUPDATECULL_H

#include "xGroup.h"

typedef U32(*xUpdateCullEntCallback)(void*, void*);

struct xUpdateCullEnt
{
    U16 index;
    S16 groupIndex;
    xUpdateCullEntCallback update_cull_cb;
    void* cbdata;
    xUpdateCullEnt* nextInGroup;
};

struct xUpdateCullGroup
{
    U32 active;
    U16 startIndex;
    U16 endIndex;
    xGroup* groupObject;
};

typedef void(*xUpdateCullMgrActivateCallback)(void*);
typedef void(*xUpdateCullMgrDeactivateCallback)(void*);

struct xUpdateCullMgr
{
    U32 entCount;
    U32 entActive;
    void** ent;
    xUpdateCullEnt** mgr;
    U32 mgrCount;
    U32 mgrCurr;
    xUpdateCullEnt* mgrList;
    U32 grpCount;
    xUpdateCullGroup* grpList;
    xUpdateCullMgrActivateCallback activateCB;
    xUpdateCullMgrDeactivateCallback deactivateCB;
};

union FloatAndVoid
{
    F32 f;
    void* v;
};

U32 xUpdateCull_AlwaysTrueCB(void*, void*);
U32 xUpdateCull_DistanceSquaredCB(void* ent, void* cbdata);
xUpdateCullMgr* xUpdateCull_Init(void** ent, U32 entCount, xGroup** group, U32 groupCount);
void xUpdateCull_Update(xUpdateCullMgr* m, U32 percent_update);
void xUpdateCull_SetCB(xUpdateCullMgr* m, void* entity, xUpdateCullEntCallback cb, void* cbdata);
void xUpdateCull_Reset(xUpdateCullMgr* m);

#endif

#ifndef ZMOVEPOINT_H
#define ZMOVEPOINT_H

#include "xMovePoint.h"

struct zScene;

struct zMovePoint : xMovePoint
{
};

zMovePoint* zMovePoint_GetMemPool(S32 cnt);
void zMovePointInit(zMovePoint* m, xMovePointAsset* asset);
zMovePoint* zMovePoint_GetInst(S32 n);
void zMovePointSetup(zMovePoint* mvpt, zScene* scn);
void zMovePointSave(zMovePoint* ent, xSerial* s);
void zMovePointLoad(zMovePoint* ent, xSerial* s);
void zMovePointReset(zMovePoint* m);
void zMovePointEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);
xVec3* zMovePointGetPos(const zMovePoint* m);

#endif
#ifndef ZCONDITIONAL_H
#define ZCONDITIONAL_H

#include "xBase.h"
#include "zCondAsset.h"

struct zVarEntry;

typedef struct _zConditional : xBase
{
    zCondAsset* asset;
    zVarEntry* varEntry;
} zConditional;

void zConditionalInit(void* b, void* asset);
void zConditionalInit(xBase* b, zCondAsset* asset);
void zConditionalReset(zConditional* c);
void zConditionalSave(zConditional* ent, xSerial* s);
void zConditionalLoad(zConditional* ent, xSerial* s);
U32 zConditional_GetCount(zConditional* c);
U32 zConditional_Evaluate(zConditional* c);
void zConditionalEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif
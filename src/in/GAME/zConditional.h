#ifndef ZCONDITIONAL_H
#define ZCONDITIONAL_H

#include "xBase.h"

struct zVarEntry;

struct zCondAsset : xBaseAsset
{
    U32 constNum;
    U32 expr1;
    U32 op;
    U32 value_asset;
};

typedef struct _zConditional : xBase
{
    zCondAsset* asset;
    zVarEntry* varEntry;
} zConditional;

// zCond operators
#define ZCOND_EQ 0 // equal
#define ZCOND_GT 1 // greater than
#define ZCOND_LT 2 // less than
#define ZCOND_GE 3 // greater than or equal
#define ZCOND_LE 4 // less than or equal
#define ZCOND_NE 5 // not equal

void zConditionalInit(void* b, void* asset);
void zConditionalInit(xBase* b, zCondAsset* asset);
void zConditionalReset(zConditional* c);
void zConditionalSave(zConditional* ent, xSerial* s);
void zConditionalLoad(zConditional* ent, xSerial* s);
U32 zConditional_GetCount(zConditional* c);
U32 zConditional_Evaluate(zConditional* c);
void zConditionalEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif
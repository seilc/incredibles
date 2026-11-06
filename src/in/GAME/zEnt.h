#ifndef ZENT_H
#define ZENT_H

#include "xEnt.h"

struct xModelAssetParam;

struct zEnt : xEnt
{
    xAnimTable* atbl;
};

void zEntEventAllOfType(U32 toEvent, U32 type);
void zEntParseModelInfo(xEnt* ent, U32 assetID);
xModelInstance* zEntRecurseModelInfo(void* info, xEnt* ent);
xModelAssetParam* zEntGetModelParams(U32 assetID, U16* size);
const char* zParamGetString(xModelAssetParam* param, U32 size, const char* tok, const char* def);

#endif
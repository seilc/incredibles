#ifndef ZENT_H
#define ZENT_H

#include "xEnt.h"

struct zEnt : xEnt
{
    xAnimTable* atbl;
};

void zEntEventAllOfType(U32 toEvent, U32 type);
void zEntParseModelInfo(xEnt* ent, U32 assetID);
xModelAssetParam* zEntGetModelParams(U32 assetID, U16* size);
const char* zParamGetString(xModelAssetParam* param, U32 size, const char* tok, const char* def);

#endif
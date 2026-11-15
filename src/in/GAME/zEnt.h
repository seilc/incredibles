#ifndef ZENT_H
#define ZENT_H

#include "xEnt.h"

struct xModelAssetParam;
struct xSerial;

struct zEnt : xEnt
{
    xAnimTable* atbl;
};

void zEntInit(zEnt* ent, xEntAsset* asset, U32 type);
void zEntSave(zEnt* ent, xSerial* s);
void zEntLoad(zEnt* ent, xSerial* s);
void zEntEventAllOfType(U32 toEvent, U32 type);
void zEntParseModelInfo(xEnt* ent, U32 assetID);
xModelInstance* zEntRecurseModelInfo(void* info, xEnt* ent);
xModelAssetParam* zEntGetModelParams(U32 assetID, U16* size);
const char* zParamGetString(xModelAssetParam* param, U32 size, const char* tok, const char* def);
void zEntGetShadowParams(xEnt* ent, xVec3* center, F32* radius, F32* strength, xEntShadow::radius_enum rtype);

#endif
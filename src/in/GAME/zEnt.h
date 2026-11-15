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
S32 zParamGetInt(xModelAssetParam* param, U32 size, const char* tok, S32 def);
F32 zParamGetFloat(xModelAssetParam* param, U32 size, const char* tok, F32 def);
S32 zParamGetVector(xModelAssetParam* param, U32 size, const char* tok, xVec3 def, xVec3* result);
void zEntGetShadowParams(xEnt* ent, xVec3* center, F32* radius, F32* strength, xEntShadow::radius_enum rtype);

#endif
#ifndef ZENV_H
#define ZENV_H

#include "xBase.h"
#include "xEnv.h"
#include "xMath3.h"

struct xEnvAsset : xBaseAsset
{
    U32 bspAssetID;
    U32 startCameraAssetID;
    U32 climateFlags;
    F32 climateStrengthMin;
    F32 climateStrengthMax;
    U32 bspLightKit;
    U32 objectLightKit;
    U32 flags;
    U32 bspCollisionAssetID;
    U32 bspFXAssetID;
    U32 bspCameraAssetID;
    U32 bspMapperID;
    U32 bspMapperCollisionID;
    U32 bspMapperFXID;
    F32 loldHeight;
    xVec3 minBounds;
    xVec3 maxBounds;
};

#define XENVASSET_FLAGS_0x1 0x1

typedef struct _zEnv : xBase
{
    xEnvAsset* easset;
} zEnv;

void zEnvInit(void* env, void* easset);
void zEnvInit(zEnv* env, xEnvAsset* easset);
void zEnvSetup(zEnv* env);
void zEnvStartingCamera(zEnv*);
void zEnvRender(xEnv* env);
void zEnvSave(zEnv* ent, xSerial* s);
void zEnvLoad(zEnv* ent, xSerial* s);
void zEnvReset(zEnv* env);
void zEnvEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase* toParamWidget, U32);

#endif
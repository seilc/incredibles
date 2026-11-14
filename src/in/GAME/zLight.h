#ifndef ZLIGHT_H
#define ZLIGHT_H

#include "xBase.h"
#include "iLight.h"

struct zLightAsset : xBaseAsset
{
    U8 lightType;
    U8 lightEffect;
    U8 lightPad[2];
    U32 lightFlags;
    F32 lightColor[4];
    xVec3 lightDir;
    F32 lightConeAngle;
    xSphere lightSphere;
    U32 attachID;
};

typedef struct _zLight : xBase
{
    U32 flags;
    zLightAsset* tasset;
    iLight light;
    xBase* attached_to;
    S32 true_idx;
    F32* reg;
    S32 effect_idx;
} zLight;

struct xEnv;
struct xEnt;

void zLightResetAll(xEnv* env);
void zLightAddLocal(xEnt* ent);
void zLightOn(zLight* zl, S32 on);

#endif

#ifndef ZLIGHTASSET_H
#define ZLIGHTASSET_H

#include "xBaseAsset.h"
#include "xMath3.h"

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

#endif

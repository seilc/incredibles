#ifndef XSCRFXLENSFLAREASSET_H
#define XSCRFXLENSFLAREASSET_H

#include "xDynAsset.h"

#include <rwcore.h>

struct xScrFxLensFlareElement : xDynAsset
{
    U32 textureID;
    RwTexture* texture;
    F32 position;
    F32 width;
    F32 height;
    F32 uv[2][2];
    U32 color;
    U32 mode;
};

struct xScrFxLensFlareSourceAsset : xDynAsset
{
    U32 object;
    U32 elementID;
    xScrFxLensFlareElement** elementArray;
    U32 numElements;
    F32 size;
    F32 intensity;
    F32 coneAngle;
    U32 distanceAttenuationMode;
};

struct xScrFxLensFlareParams
{
    F32 near_dist;
    F32 far_dist;
    F32 near_dist2;
    F32 far_dist2;
};

#endif

#ifndef XSHADOW_H
#define XSHADOW_H

#include "xMath3.h"

struct xEnt;

struct xShadowPoly
{
    xVec4 vert[3];
};

struct xShadowCache
{
    xVec3 pos;
    F32 radius;
    S32 entCount;
    S32 polyCount;
    F32 polyRayDepth[5];
    U16 castOnEnt;
    U16 castOnPoly;
    U32 pad;
    xEnt* ent[16];
    xShadowPoly* poly;
};

extern S32 SHADOW_CACHE_POLY_MAX;

typedef void (*xShadowRenderCallback)(void*);

void xShadowSetLight(xVec3* target_pos, xVec3* in_vec, F32);
void xShadowCameraUpdate(void* model, xShadowRenderCallback renderCB, xVec3* center, F32 radius, S32 shadowMode);
void xShadowVertical_FillCache(xShadowCache* cache, xVec3* pos, F32 r, F32 depth, F32 minNormY);
void xShadowVertical_DrawCache(xShadowCache* cache, F32 shadowFactor, F32 fadeDist, S32 shadowMode, RwMatrix* shadowMat, RwRaster* shadowRaster);

#endif

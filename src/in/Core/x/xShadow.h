#ifndef XSHADOW_H
#define XSHADOW_H

#include "xMath3.h"

#include <rwcore.h>
#include <rpworld.h>

struct xEnt;

#define SHADOW_CACHE_ENT_MAX 16

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
    xEnt* ent[SHADOW_CACHE_ENT_MAX];
    xShadowPoly* poly;
};

extern RwCamera* ShadowCamera;
extern S32 SHADOW_CACHE_POLY_MAX;

void xShadowInit();
void xShadowManager_Init(S32 numEnts, S32 maxPolys, S32 maxCaches);
void xShadowManager_Add(xEnt* ent);
void xShadowManager_Remove(xEnt* ent); // UNUSED
void xShadowManager_Render();
void xShadowSetWorld(RpWorld* world);
void xShadowSetLight(xVec3* target_pos, xVec3* in_vec, F32);
S32 xShadowCameraCreate();
void xShadowCameraUpdate(void* model, void (*renderCB)(void*), xVec3* center, F32 radius, S32 shadowMode);
U32 xShadowReceiveShadowSetup(xEnt* ent);
void xShadowReceiveShadow(xEnt* ent, F32 shadowFactor, S32 shadowMode, RwMatrix* shadowMat, RwRaster* shadowRast);
void xShadow_ListAdd(xEnt* ent);
S32 Im2DRenderQuad(F32 x1, F32 y1, F32 x2, F32 y2, F32 z, F32 recipCamZ, F32 uvOffset);
void GCSaveFrameBuffer();
void GCRestoreFrameBuffer();
void xShadowVertical_FillCache(xShadowCache* cache, xVec3* pos, F32 r, F32 depth, F32 minNormY);
void xShadowVertical_DrawCache(xShadowCache* cache, F32 shadowFactor, F32 fadeDist, S32 shadowMode, RwMatrix* shadowMat, RwRaster* shadowRast);

#endif

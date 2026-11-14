#ifndef XSHADOWSIMPLE_H
#define XSHADOWSIMPLE_H

#include "xMath3.h"

#include <rwcore.h>

struct xEnt;

struct xShadowSimplePoly
{
    xVec3 vert[3];
    xVec3 norm;
};

struct xShadowSimpleCache
{
    U16 flags;
    U8 alpha;
    U8 pad;
    U32 collPriority;
    xVec3 pos;
    xVec3 at;
    F32 tol_movement;
    F32 radiusOptional;
    xEnt* castOnEnt;
    xShadowSimplePoly poly;
    F32 envHeight;
    F32 shadowHeight;
    union
    {
        U32 raster;
        RwRaster* ptr_raster;
    };
    F32 dydx;
    F32 dydz;
    xVec3 corner[4];
    void* collSkipsItem;
};

void xShadowSimple_Init();
void xShadowSimple_Reset();
void xShadowSimple_CacheInit(xShadowSimpleCache* cache, xEnt* ent, U8 alpha);
void xShadowSimple_Add(xShadowSimpleCache* cache, xEnt* ent, F32 radius, F32 ecc);
void xShadowSimple_Render();
void xShadowSimple_CacheInit_Expert(xShadowSimpleCache* cache, RwRaster* rast, U8 alpha);
void xShadowSimple_Add_Expert(xShadowSimpleCache* cache, const xMat4x3* mat_owner, U8 alpha, F32 radius, F32 height, F32 ecc);

#endif

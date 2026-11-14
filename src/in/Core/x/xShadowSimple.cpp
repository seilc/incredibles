#include "xCommon.h"
#include "xShadowSimple.h"

#include "xClumpColl.h"
#include "xEnt.h"
#include "xEnv.h"
#include "xGlobals.h"
#include "xIM.h"
#include "xJSP.h"
#include "xScene.h"
#include "xstransvc.h"
#include "zGrid.h"

#include <string.h>

#define SHADOW_ROUND_ID 0x1A2A0413 // xStrHash("shadow_round")

struct xShadowSimpleQueue
{
    xShadowSimpleCache* cache;
    U32 priority;
    const xMat4x3* modelMatrix;
    F32 radius;
    F32 height;
    F32 ecc;
};

struct zSimpleShadowTableHeader
{
    U32 num;
};

struct zSimpleShadow
{
    U32 modelID;
    U32 shadowID;
    U32 flags;
};

#define MAX_SHAD_QUADS 64
#define MAX_SHAD_VERTS (MAX_SHAD_QUADS * 6)
#define MAX_SHAD_RASTERS 64
#define MAX_COLL_QUEUE 2

static xShadowSimpleQueue sCollQueue[MAX_COLL_QUEUE];
static RxRenderStateVector xrsv;
static RwIm3DVertex sShadVert[MAX_SHAD_VERTS];
static RwRaster* sShadRasters[MAX_SHAD_RASTERS];
static RwRaster* sShadRaster;
static U32 sShadVertCount;
static RwMatrix* sModelMat;

static RpCollisionTriangle* shadowRayCB(RpIntersection*, RpWorldSector*, RpCollisionTriangle* tri, RwReal dist, void* data)
{
    xVec3 xformnorm;
    xVec3* norm;
    F32* testdist;
    xShadowSimpleCache* cache = (xShadowSimpleCache*)data;

    if (sModelMat) {
        testdist = &cache->shadowHeight;
        xMat3x3RMulVec(&xformnorm, (xMat3x3*)sModelMat, (xVec3*)&tri->normal);
        xVec3Normalize(&xformnorm, &xformnorm);
        norm = &xformnorm;
    } else {
        testdist = &cache->envHeight;
        norm = (xVec3*)&tri->normal;
    }

    // 0.0871557f = cos(85 degrees)
    if (dist >= *testdist || norm->y < 0.0871557f) {
        return tri;
    }

    *testdist = dist;

    cache->poly.vert[0] = *(xVec3*)tri->vertices[0];
    cache->poly.vert[1] = *(xVec3*)tri->vertices[1];
    cache->poly.vert[2] = *(xVec3*)tri->vertices[2];
    cache->poly.norm = *(xVec3*)&tri->normal;

    return tri;
}

static RpCollisionTriangle* shadowRayModelCB(RpIntersection* isx, RpCollisionTriangle* tri, RwReal dist, void* data)
{
    return shadowRayCB(isx, NULL, tri, dist, data);
}

struct ShadowSimpleCBParam
{
    xShadowSimpleCache* cache;
    RpIntersection* isx;
};

static S32 shadowRayEntCB(xEnt* ent, void* cbdata)
{
    if (!(ent->baseFlags & k_XBASE_RECEIVES_SHADOWS)) {
        return TRUE;
    }

    ShadowSimpleCBParam* cbparam = (ShadowSimpleCBParam*)cbdata;

    if (cbparam->cache->collSkipsItem == ent) {
        return TRUE;
    }

    xModelInstance* m = ent->model;

    if (iModelNumBones(m->Data) > 1) {
        return TRUE;
    }

    F32 oldHeight = cbparam->cache->shadowHeight;

    RwFrame* frame = RpAtomicGetFrame(m->Data);
    RwFrameTransform(frame, m->Mat, rwCOMBINEREPLACE);

    sModelMat = m->Mat;
    RpAtomicForAllIntersections(m->Data, cbparam->isx, shadowRayModelCB, cbparam->cache);
    sModelMat = NULL;

    if (cbparam->cache->shadowHeight != oldHeight) {
        cbparam->cache->castOnEnt = ent;
    }

    return TRUE;
}

static void xShadowSimple_SceneCollide(xShadowSimpleCache* cache, const xVec3* pos, F32 depth)
{
    cache->envHeight = HUGE;
    cache->shadowHeight = HUGE;
    cache->castOnEnt = NULL;
    cache->collPriority = 0;

    xEnv* env = ((xScene*)xglobals->sceneCur)->env;

    RpIntersection isx;
    isx.type = rpINTERSECTLINE;
    isx.t.line.start.x = pos->x;
    isx.t.line.start.y = 0.1f + pos->y;
    isx.t.line.start.z = pos->z;
    isx.t.line.end.x = pos->x;
    isx.t.line.end.y = pos->y - depth;
    isx.t.line.end.z = pos->z;

    for (S32 i = 0; i < env->geom->jsp_count; i++) {
        if (xEnvIsJSPActive(env, i)) {
            xClumpColl_ForAllIntersections(env->geom->jsp_list[i]->colltree, &isx, shadowRayCB, cache);
        }
    }

    if (cache->envHeight != HUGE) {
        cache->envHeight = cache->envHeight * (isx.t.line.end.y - isx.t.line.start.y) + isx.t.line.start.y;
        isx.t.line.end.y = cache->envHeight;
    }

    if (!(cache->flags & 0x10)) {
        ShadowSimpleCBParam cbparam;

        xQCData qcd;
        xQuickCullForLine(&qcd, (xLine3*)&isx.t.line);

        cbparam.cache = cache;
        cbparam.isx = &isx;

        xGridCheckPosition(&colls_grid, (xVec3*)&isx.t.line.start, &qcd, shadowRayEntCB, &cbparam);
        xGridCheckPosition(&colls_oso_grid, (xVec3*)&isx.t.line.start, &qcd, shadowRayEntCB, &cbparam);
    }

    if (cache->shadowHeight != HUGE) {
        cache->shadowHeight = cache->shadowHeight * (isx.t.line.end.y - isx.t.line.start.y) + isx.t.line.start.y;
    } else {
        cache->shadowHeight = cache->envHeight;
    }

    if (cache->shadowHeight != HUGE && !cache->castOnEnt) {
        cache->dydx = -cache->poly.norm.x / cache->poly.norm.y;
        cache->dydz = -cache->poly.norm.z / cache->poly.norm.y;
    }
}

static void xShadowSimple_CalcCorners(xShadowSimpleCache* cache, const xMat4x3* mat, F32 radius, F32 ecc)
{
    if (cache->shadowHeight == HUGE) {
        return;
    }

    if (cache->castOnEnt) {
        xVec3 tempnorm;
        xMat3x3RMulVec(&tempnorm, (xMat3x3*)cache->castOnEnt->model->Mat, &cache->poly.norm);
        xVec3Normalize(&tempnorm, &tempnorm);

        cache->dydx = -tempnorm.x / tempnorm.y;
        cache->dydz = -tempnorm.z / tempnorm.y;
    }

    xVec3 shadowRight, shadowAt;
    if (cache->flags & 0x2) {
        shadowRight.x = mat->right.x * radius * ecc;
        shadowRight.z = mat->right.z * radius * ecc;
        shadowRight.y = shadowRight.x * cache->dydx + shadowRight.z * cache->dydz;
        shadowAt.x = mat->at.x * radius;
        shadowAt.z = mat->at.z * radius;
        shadowAt.y = shadowAt.x * cache->dydx + shadowAt.z * cache->dydz;
    } else {
        shadowRight.x = radius * ecc;
        shadowRight.z = 0.0f;
        shadowRight.y = shadowRight.x * cache->dydx;
        shadowAt.x = 0.0f;
        shadowAt.z = radius;
        shadowAt.y = shadowAt.z * cache->dydz;
    }

    cache->corner[0].x = mat->pos.x + shadowRight.x;
    cache->corner[0].y = cache->shadowHeight + shadowRight.y + 0.02f;
    cache->corner[0].z = mat->pos.z + shadowRight.z;
    cache->corner[1].x = mat->pos.x + shadowAt.x;
    cache->corner[1].y = cache->shadowHeight + shadowAt.y + 0.02f;
    cache->corner[1].z = mat->pos.z + shadowAt.z;
    cache->corner[2].x = mat->pos.x - shadowAt.x;
    cache->corner[2].y = cache->shadowHeight - shadowAt.y + 0.02f;
    cache->corner[2].z = mat->pos.z - shadowAt.z;
    cache->corner[3].x = mat->pos.x - shadowRight.x;
    cache->corner[3].y = cache->shadowHeight - shadowRight.y + 0.02f;
    cache->corner[3].z = mat->pos.z - shadowRight.z;
}

static void xShadowSimple_AddVerts(xShadowSimpleCache* cache)
{
    if (cache->shadowHeight == HUGE || sShadVertCount >= MAX_SHAD_VERTS) {
        return;
    }

    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 0], cache->corner[0].x, cache->corner[0].y, cache->corner[0].z);
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 1], cache->corner[1].x, cache->corner[1].y, cache->corner[1].z);
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 2], cache->corner[2].x, cache->corner[2].y, cache->corner[2].z);
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 3], cache->corner[1].x, cache->corner[1].y, cache->corner[1].z);
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 4], cache->corner[2].x, cache->corner[2].y, cache->corner[2].z);
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 5], cache->corner[3].x, cache->corner[3].y, cache->corner[3].z);

    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 0], 0, 0, 0, cache->alpha);
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 1], 0, 0, 0, cache->alpha);
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 2], 0, 0, 0, cache->alpha);
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 3], 0, 0, 0, cache->alpha);
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 4], 0, 0, 0, cache->alpha);
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 5], 0, 0, 0, cache->alpha);

    sShadRasters[sShadVertCount / 6] = cache->ptr_raster;
    sShadVertCount += 6;
}

void xShadowSimple_Init()
{
    memset(sCollQueue, 0, sizeof(sCollQueue));

    RwTexture* tex = (RwTexture*)xSTFindAsset(SHADOW_ROUND_ID, NULL);
    if (tex) {
        sShadRaster = RwTextureGetRaster(tex);
    } else
        sShadRaster = NULL;

    memset(sShadVert, 0, sizeof(sShadVert));
    for (U32 i = 0; i < MAX_SHAD_QUADS; i++) {
        sShadVert[i * 6 + 1].u = 1.0f;
        sShadVert[i * 6 + 2].v = 1.0f;
        sShadVert[i * 6 + 3].u = 1.0f;
        sShadVert[i * 6 + 4].v = 1.0f;
        sShadVert[i * 6 + 5].u = 1.0f;
        sShadVert[i * 6 + 5].v = 1.0f;

        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 0], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 1], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 2], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 3], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 4], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 5], 0.0f, 1.0f, 0.0f);
    }
}

void xShadowSimple_Reset()
{
    memset(sCollQueue, 0, sizeof(sCollQueue));
}

void xShadowSimple_CacheInit(xShadowSimpleCache* cache, xEnt* ent, U8 alpha)
{
    memset(cache, 0, sizeof(xShadowSimpleCache));

    cache->envHeight = HUGE;
    cache->shadowHeight = HUGE;
    cache->flags = 0x1;
    cache->alpha = alpha;
    cache->collSkipsItem = ent;

    if (!ent->model || ent->model->shadowID != 0xDEADBEEF) {
        return;
    }

    S32 i, n;
    U32 j;
    zSimpleShadowTableHeader* sst;
    RwRaster* raster = NULL;
    U32 flags = 0;

    n = xSTAssetCountByType('SHDW');
    for (i = 0; i < n; i++) {
        U32 size;
        sst = (zSimpleShadowTableHeader*)xSTFindAssetByType('SHDW', i, &size);

        for (j = 0; j < sst->num; j++) {
            zSimpleShadow* table = (zSimpleShadow*)(sst + 1);
            if (table[j].modelID == ent->model->modelID) {
                RwTexture* tex = (RwTexture*)xSTFindAsset(table[j].shadowID, NULL);
                if (tex) {
                    raster = RwTextureGetRaster(tex);
                    flags = table[j].flags;
                } else {
                    raster = (RwRaster*)0xDEADBEEF;
                }
            }

            if (raster)
                break;
        }

        if (raster)
            break;
    }
    
    if (!raster || raster == (RwRaster*)0xDEADBEEF) {
        raster = sShadRaster;
    }

    cache->ptr_raster = raster;
    cache->flags |= (U16)flags;

    ent->model->shadowID = (U32)raster;
}

void xShadowSimple_Add(xShadowSimpleCache* cache, xEnt* ent, F32 radius, F32 ecc)
{
    xShadowSimple_Add_Expert(cache, (xMat4x3*)ent->model->Mat, cache->alpha, radius, 10.0f, ecc);
}

void xShadowSimple_Render()
{
    for (S32 qnum = 0; qnum < MAX_COLL_QUEUE; qnum++) {
        if (sCollQueue[qnum].cache) {
            xShadowSimpleCache* cache = sCollQueue[qnum].cache;

            xShadowSimple_SceneCollide(cache, &sCollQueue[qnum].modelMatrix->pos, sCollQueue[qnum].height);

            cache->pos = sCollQueue[qnum].modelMatrix->pos;
            cache->at = sCollQueue[qnum].modelMatrix->at;

            xShadowSimple_CalcCorners(cache, sCollQueue[qnum].modelMatrix, sCollQueue[qnum].radius, sCollQueue[qnum].ecc);
            xShadowSimple_AddVerts(cache);
        }

        sCollQueue[qnum].cache = NULL;
    }

    if (sShadVertCount == 0) {
        return;
    }

    RxRenderStateVectorLoadDriverState(&xrsv);

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCCOLOR);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDDESTCOLOR);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    
    for (U32 i = 0, j; i < sShadVertCount; i = j) {
        RwRaster* raster = sShadRasters[i / 6];

        j = i;
        while (j < sShadVertCount && sShadRasters[j / 6] == raster) j += 6;

        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)raster);
        xIMRenderLegacy(sShadVert + i, j - i, NULL, rwPRIMTYPETRILIST);
    }
    
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)xrsv.SrcBlend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)xrsv.DestBlend);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)xrsv.ShadeMode);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)((xrsv.Flags & rxRENDERSTATEFLAG_ZWRITEENABLE) ? TRUE : FALSE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)((xrsv.Flags & rxRENDERSTATEFLAG_VERTEXALPHAENABLE) ? TRUE : FALSE));

    sShadVertCount = 0;
}

void xShadowSimple_CacheInit_Expert(xShadowSimpleCache* cache, RwRaster* rast, U8 alpha)
{
    memset(cache, 0, sizeof(xShadowSimpleCache));

    cache->envHeight = HUGE;
    cache->shadowHeight = HUGE;
    cache->flags = 0x3;
    cache->alpha = alpha;
    cache->collSkipsItem = NULL;
    cache->radiusOptional = -1.0f;
    cache->tol_movement = 0.1f;

    if (rast) {
        cache->ptr_raster = rast;
    } else {
        cache->ptr_raster = sShadRaster;
    }
}

inline void XSSC_Add_NudgeItAndFudgeIt(xShadowSimpleCache* cache, const xMat4x3* mat_owner)
{
    xVec3* vert;
    xVec3 xformvert[3];

    if (cache->castOnEnt) {
        xVec3 xformnorm;
        xEnt* castOnEnt = cache->castOnEnt;

        xMat4x3Toworld(&xformvert[0], (xMat4x3*)castOnEnt->model->Mat, &cache->poly.vert[0]);
        xMat4x3Toworld(&xformvert[1], (xMat4x3*)castOnEnt->model->Mat, &cache->poly.vert[1]);
        xMat4x3Toworld(&xformvert[2], (xMat4x3*)castOnEnt->model->Mat, &cache->poly.vert[2]);

        xMat3x3RMulVec(&xformnorm, (xMat4x3*)castOnEnt->model->Mat, &cache->poly.norm);
        xVec3Normalize(&xformnorm, &xformnorm);

        // BUG: Should be xabs(xformnorm.y) > EPSILON
        if (xabs(xformnorm.y > EPSILON)) {
            cache->dydx = -xformnorm.x / xformnorm.y;
            cache->dydz = -xformnorm.z / xformnorm.y;
        } else {
            F32 poo = (xformnorm.y > 0.0f) ? EPSILON : -EPSILON;
            cache->dydx = -xformnorm.x / poo;
            cache->dydz = -xformnorm.z / poo;
        }

        vert = &xformvert[0];
    } else {
        vert = &cache->poly.vert[0];
        if (vert->x == HUGE) {
            return;
        }
    }

    F32 nx, nz, pdot;
    for (S32 i = 0; i < 3; i++) {
        xVec3* v0 = &vert[i];
        xVec3* v1 = &vert[i == 2 ? 0 : i + 1];
        nx = v0->z - v1->z;
        nz = v1->x - v0->x;
        pdot = nx * (mat_owner->pos.x - v0->x) + nz * (mat_owner->pos.z - v0->z);
        if (pdot > EPSILON) {
            cache->collPriority += 20;
            break;
        }
    }
    
    cache->shadowHeight =
        cache->dydx * (mat_owner->pos.x - vert->x) +
        vert->y +
        cache->dydz * (mat_owner->pos.z - vert->z);
}

inline S32 XSSC_Add_ShoveIntoQueue(xShadowSimpleCache* cache, const xMat4x3* mat_owner, F32 radius, F32 height, F32 ecc)
{
    for (S32 i = 0; i < MAX_COLL_QUEUE; i++) {
        if (!sCollQueue[i].cache) {
            sCollQueue[i].cache = cache;
            sCollQueue[i].priority = sCollQueue[i].cache->collPriority;
            sCollQueue[i].modelMatrix = mat_owner;
            sCollQueue[i].radius = radius;
            sCollQueue[i].height = height;
            sCollQueue[i].ecc = ecc;
            return TRUE;
        }

        if (cache->collPriority > sCollQueue[i].cache->collPriority) {
            if (sCollQueue[1].cache) {
                xShadowSimple_CalcCorners(sCollQueue[1].cache, sCollQueue[1].modelMatrix, sCollQueue[1].radius, sCollQueue[1].ecc);
                xShadowSimple_AddVerts(sCollQueue[1].cache);
            }
            for (S32 j = 0; j >= i; j--) {
                sCollQueue[j + 1] = sCollQueue[j];
            }
            sCollQueue[i].cache = cache;
            sCollQueue[i].priority = sCollQueue[i].cache->collPriority;
            sCollQueue[i].modelMatrix = mat_owner;
            sCollQueue[i].radius = radius;
            sCollQueue[i].height = height;
            sCollQueue[i].ecc = ecc;
            return TRUE;
        }
    }
    
    return FALSE;
}

void xShadowSimple_Add_Expert(xShadowSimpleCache* cache, const xMat4x3* mat_owner, U8 alpha, F32 radius, F32 height, F32 ecc)
{
    cache->alpha = alpha;

    if (radius >= 0.0f) {
        cache->radiusOptional = radius;
    }
    if (radius < 0.0f && cache->radiusOptional >= 0.0f)
        radius = cache->radiusOptional;

    if (cache->flags & 0x1) {
        cache->flags &= (U16)~0x1;
        cache->poly.vert[0].x = HUGE;

        xShadowSimple_SceneCollide(cache, &mat_owner->pos, height);

        cache->pos = mat_owner->pos;
        cache->at = mat_owner->at;

        if (cache->shadowHeight != HUGE) {
            xShadowSimple_CalcCorners(cache, mat_owner, radius, ecc);
            xShadowSimple_AddVerts(cache);
        }
    } else {
        U32 ownerMoved;
        if (cache->flags & 0x8) {
            xVec3 delta = cache->pos - mat_owner->pos;
            F32 ds2_moved = delta.length2();
            ownerMoved = (ds2_moved > xsqr(cache->tol_movement));
        } else {
            ownerMoved = (cache->pos.x != mat_owner->pos.x || cache->pos.y != mat_owner->pos.y || cache->pos.z != mat_owner->pos.z);
        }

        if ((cache->flags & 0x8) && (ownerMoved || cache->castOnEnt)) {
            XSSC_Add_NudgeItAndFudgeIt(cache, mat_owner);
        }
        else if (cache->shadowHeight != HUGE && (ownerMoved || cache->castOnEnt)) {
            XSSC_Add_NudgeItAndFudgeIt(cache, mat_owner);
            if (ownerMoved) {
                cache->envHeight = HUGE;
            }
        }

        if (ownerMoved)
            cache->collPriority += 6;
        else if (cache->castOnEnt)
            cache->collPriority += 3;
        else
            cache->collPriority += 2;

        S32 added = XSSC_Add_ShoveIntoQueue(cache, mat_owner, radius, height, ecc);
        if (!added) {
            xShadowSimple_CalcCorners(cache, mat_owner, radius, ecc);
            xShadowSimple_AddVerts(cache);
        }
    }
}

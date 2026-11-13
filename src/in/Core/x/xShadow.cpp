#include "xCommon.h"
#include "xShadow.h"

#include "xClumpColl.h"
#include "xCollide.h"
#include "xCam.h"
#include "xCollideSweptSphere.h"
#include "xDebugTweak.h"
#include "xEnv.h"
#include "xEnt.h"
#include "xGlobals.h"
#include "xGrid.h"
#include "xJSP.h"
#include "xMath.h"
#include "xutil.h"
#include "xIM.h"
#include "xLightKit.h"
#include "xMemMgr.h"
#include "xModel.h"
#include "xRenderBuffer.h"
#include "xShadowSimple.h"
#include "zBase.h"
#include "zEnt.h"
#include "zGrid.h"
#include "iCamera.h"
#include "iModel.h"

#include "decomp.h"

#include <stdlib.h>
#include <string.h>

xTWEAKABLE F32 SHADOW_BIAS_AMT = 0.008f;
xTWEAKABLE F32 SHADOW_MINNORMY = 0.0871557f;
xTWEAKABLE U32 rrs = 1;
RpLight* ShadowLight;
RwRGBAReal ShadowLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
static xTWEAKABLE S32 enableNPCShadowOneEnt = 1;
static xTWEAKABLE S32 enableNPCShadowAllEnt = 0;
static xTWEAKABLE F32 DefaultShadowDepth = 10.0f;
S32 SHADOW_CACHE_POLY_MAX = 256;
static F32 SHADOW_BOTH;
static RwIm3DVertex* Im3DBuffer;
static U32 Im3DBufferPos;
RwCamera* ShadowCamera;
RwRaster* ShadowCameraRaster;
static RwRaster* ShadowRenderRaster;
F32 gShadowObjectRadius;
static U32 shadow_ent_count;
static S32 sShadowCollJSP;
static RwRaster* gc_saveraster;

typedef struct _ProjectionParam
{
    RwV3d at;
    RwMatrix invMatrix;
    U8 shadowValue;
    S32 fade;
    U32 numIm3DBatch;
    U32 shadowWord;
} ProjectionParam;

static S32 SetupShadow();
static RwCamera* ShadowCameraCreatePersp(S32 size);
static RwRaster* ShadowRasterCreate(S32 size);
static RwCamera* ShadowCameraUpdate(RwCamera* shadowCamera, void* model, void (*renderCB)(void*), xVec3* center, F32 radius, S32 shadowMode);
static void InvertRaster(RwCamera* camera);
static RwCamera* ShadowCameraSetSpherePersp(RwCamera* camera, RwV3d* center, F32 radius);

void xShadowInit()
{
    S32 size;
    RwFrame* frame;

    xShadowCameraCreate();

    size = 128;

    gc_saveraster = RwRasterCreate(size, size, 32, 0x504);

    shadow_ent_count = 0;

    ShadowLight = RpLightCreate(rpLIGHTDIRECTIONAL);
    RpLightSetColor(ShadowLight, &ShadowLightColor);
    frame = RwFrameCreate();
    RpLightSetFrame(ShadowLight, frame);
}

static S32 SetupShadow()
{
    S32 res;

    res = 128;

    while (res > (S32)FB_XRES || res > (S32)FB_YRES) {
        res >>= 1;
    }

    ShadowCamera = ShadowCameraCreatePersp(res);

    if (!ShadowCamera) {
        return FALSE;
    }

    ShadowCameraRaster = ShadowRasterCreate(res);
    if (!ShadowCameraRaster) {
        return FALSE;
    }

    RwCameraSetRaster(ShadowCamera, ShadowCameraRaster);
    return TRUE;
}

void xShadowSetWorld(RpWorld* world)
{
    RpWorldAddCamera(world, ShadowCamera);

    SHADOW_BOTH = 2.0f;
}

void xShadowSetLight(xVec3* target_pos, xVec3* in_vec, F32)
{
    xVec3 zvec;
    xMat4x3 matrix;

    xVec3Normalize(&zvec, in_vec);

    xMat3x3LookVec(&matrix, &zvec);

    matrix.pos = *target_pos;

    RwFrame* camFrame = RwCameraGetFrame(ShadowCamera);
    RwMatrix* camMatrix = RwFrameGetMatrix(camFrame);
    xMat4x3Copy((xMat4x3*)camMatrix, &matrix);

    RwFrameOrthoNormalize(camFrame);

    RwMatrixUpdate(camMatrix);

    RwFrameUpdateObjects(camFrame);
}

S32 xShadowCameraCreate()
{
    if (!SetupShadow()) {
        return FALSE;
    }

    return TRUE;
}

void xShadowCameraUpdate(void* model, void (*renderCB)(void*), xVec3* center, F32 radius, S32 shadowMode)
{
    ShadowCameraSetSpherePersp(ShadowCamera, (RwV3d*)center, radius);

    ShadowCameraUpdate(ShadowCamera, model, renderCB, center, radius, shadowMode);

    ShadowRenderRaster = ShadowCameraRaster;
}

static void modelRenderCB(void* model); // UNUSED

DECOMP_FORCEFLOAT(0.0f)
DECOMP_FORCEFLOAT(0.5f)
DECOMP_FORCEFLOAT(1.0f)
DECOMP_FORCEFLOAT(0.00001f)

U32 xShadowReceiveShadowSetup(xEnt* ent)
{
    if (ent->model && xEntIsVisible(ent)) {
        if (!rrs || (ent->baseFlags & k_XBASE_RECEIVES_SHADOWS))
            if (!xModelCullGroup(ent->model)) {
                return TRUE;
            }
    }

    return FALSE;
}

void xShadowReceiveShadow(xEnt* ent, F32 shadowFactor, S32 shadowMode, RwMatrix* shadowMat, RwRaster* shadowRast)
{
    RwMatrix oldroot = { 0 };
    if (ent->model->Scale.x) {
        oldroot = *ent->model->Mat;
        ent->model->Mat->right.x *= ent->model->Scale.x,
        ent->model->Mat->right.y *= ent->model->Scale.x,
        ent->model->Mat->right.z *= ent->model->Scale.x,
        ent->model->Mat->up.x *= ent->model->Scale.y,
        ent->model->Mat->up.y *= ent->model->Scale.y,
        ent->model->Mat->up.z *= ent->model->Scale.y,
        ent->model->Mat->at.x *= ent->model->Scale.z,
        ent->model->Mat->at.y *= ent->model->Scale.z,
        ent->model->Mat->at.z *= ent->model->Scale.z;
    }

    RwCamera* shadowCamera = ShadowCamera;
    RwMatrix* shadowMatrix;
    RwMatrix invMatrix;
    RwV3d at, scl, tr;
    F32 fadeDist = 0.0f;
    U32 i, num_verts;
    xVec3* xvert;
    RpTriangle* tri;
    RpGeometry* geom;
    RwMatrix* mat;
    F32 radius;

    S32 fogstate;

    if (shadowRast) {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, shadowRast);
    } else {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwCameraGetRaster(ShadowCamera));
    }

    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogstate);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);

    switch (shadowMode) {
    case 1:
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        break;
    case 0:
    default:
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDZERO);
        break;
    }

    if (shadowFactor < 0.0f) {
        shadowFactor = -shadowFactor;

        switch (shadowMode) {
        case 1:
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCALPHA);
            break;
        case 0:
        default:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCCOLOR);
            break;
        }
    } else {
        switch (shadowMode) {
        case 1:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            break;
        case 0:
        default:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCCOLOR);
            break;
        }
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);

    if (shadowMat) {
        shadowMatrix = shadowMat;
    } else {
        shadowMatrix = RwFrameGetMatrix(RwCameraGetFrame(shadowCamera));
    }

    at = *RwMatrixGetAt(shadowMatrix);
    radius = gShadowObjectRadius;

    RwMatrixInvert(&invMatrix, shadowMatrix);

    scl.x = scl.y = -0.5f / radius;
    scl.z = 1.0f / (fadeDist + radius);
    RwMatrixScale(&invMatrix, &scl, rwCOMBINEPOSTCONCAT);

    tr.x = tr.y = 0.5f;
    tr.z = 0.0f;
    RwMatrixTranslate(&invMatrix, &tr, rwCOMBINEPOSTCONCAT);

    xModelInstance* model = ent->model;
    U32 max_verts = 0;
    while (model) {
        if (model->Flags & 0x1) {
            geom = RpAtomicGetGeometry(model->Data);
            mat = model->Mat;

            num_verts = RpGeometryGetNumVertices(geom);
            if (num_verts > max_verts)
                max_verts = num_verts;
        }
        if (ent->pflags & 0x40) break;
        model = model->Next;
    }

    if (max_verts) {
        xvert = (xVec3*)xMEMPUSHTEMP(max_verts * sizeof(xVec3));
        if (xvert) {
            Im3DBuffer = gRenderBuffer.m_vertex;

            model = ent->model;
            while (model) {
                if (model->Flags & 0x1) {
                    xVec3* vert = xvert;

                    geom = RpAtomicGetGeometry(model->Data);
                    mat = model->Mat;

                    num_verts = RpGeometryGetNumVertices(geom);

                    iModelVertEval(model->Data, 0, num_verts, mat, NULL, xvert);

                    RwIm3DVertex* imv;
                    U8 val = 255.0f * shadowFactor;

                    tri = geom->triangles;
                    for (i = 0; i < geom->numTriangles; i++, tri++) {
                        if (Im3DBufferPos > 477) {
                            xIMRenderLegacy(Im3DBuffer, Im3DBufferPos, NULL, rwPRIMTYPETRILIST);
                            Im3DBufferPos = 0;
                        }

                        RwV3d vShad[3];
                        imv = &Im3DBuffer[Im3DBufferPos];
                        xVec3* v0 = &vert[tri->vertIndex[0]];
                        xVec3* v1 = &vert[tri->vertIndex[1]];
                        xVec3* v2 = &vert[tri->vertIndex[2]];
                        RwV3d vIn[3];

                        vIn[0] = *(RwV3d*)v0;
                        vIn[1] = *(RwV3d*)v1;
                        vIn[2] = *(RwV3d*)v2;

                        RwV3dTransformPoints(vShad, vIn, ARRAY_LENGTH(vIn), &invMatrix);

                        if ((vShad[0].z < 0.0f && vShad[1].z < 0.0f && vShad[2].z < 0.0f) || (vShad[0].x < 0.0f && vShad[1].x < 0.0f && vShad[2].x < 0.0f) || (vShad[0].x > 1.0f && vShad[1].x > 1.0f && vShad[2].x > 1.0f) || (vShad[0].y < 0.0f && vShad[1].y < 0.0f && vShad[2].y < 0.0f) || (vShad[0].y > 1.0f && vShad[1].y > 1.0f && vShad[2].y > 1.0f)) {
                            continue;
                        }

                        RwV3d a, b, c;
                        RwV3dSub(&a, (RwV3d*)v1, (RwV3d*)v0);
                        RwV3dSub(&b, (RwV3d*)v2, (RwV3d*)v0);
                        RwV3dCrossProduct(&c, &a, &b);
                        RwReal len = RwV3dLength(&c);
                        if (xabs(len) < 0.00001f) {
                            if (len < 0.0f) {
                                len = -0.00001f;
                            } else {
                                len = 0.00001f;
                            }
                        }

                        len = SHADOW_BIAS_AMT / len;
                        RwV3dScale(&c, &c, len);

                        if (RwV3dDotProduct(&c, &at) > -SHADOW_BIAS_AMT * SHADOW_MINNORMY) {
                            continue;
                        }

                        RwIm3DVertexSetPos(&imv[0], v0->x + c.x, v0->y + c.y, v0->z + c.z);
                        RwIm3DVertexSetPos(&imv[1], v1->x + c.x, v1->y + c.y, v1->z + c.z);
                        RwIm3DVertexSetPos(&imv[2], v2->x + c.x, v2->y + c.y, v2->z + c.z);

                        imv[0].u = vShad[0].x;
                        imv[1].u = vShad[1].x;
                        imv[2].u = vShad[2].x;
                        imv[0].v = vShad[0].y;
                        imv[1].v = vShad[1].y;
                        imv[2].v = vShad[2].y;

                        RwIm3DVertexSetRGBA(&imv[0], val, val, val, val);
                        RwIm3DVertexSetRGBA(&imv[1], val, val, val, val);
                        RwIm3DVertexSetRGBA(&imv[2], val, val, val, val);

                        Im3DBufferPos += 3;
                    }

                    if (Im3DBufferPos) {
                        xIMRenderLegacy(Im3DBuffer, Im3DBufferPos, NULL, rwPRIMTYPETRILIST);
                        Im3DBufferPos = 0;
                    }
                }
                if (ent->pflags & 0x40) break;
                model = model->Next;
            }

            xMEMPOPTEMP(xvert);

            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);

            RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fogstate);

            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
        }
    }

    if (ent->model->Scale.x)
        *ent->model->Mat = oldroot;
}

DECOMP_FORCEFLOAT(10.0f)

void xShadow_ListAdd(xEnt* ent)
{
    xShadowManager_Add(ent);
}

S32 Im2DRenderQuad(F32 x1, F32 y1, F32 x2, F32 y2, F32 z, F32 recipCamZ, F32 uvOffset)
{
    RwIm2DVertex vx[4];

    RwIm2DVertexSetScreenX(&vx[0], x1);
    RwIm2DVertexSetScreenY(&vx[0], y1);
    RwIm2DVertexSetScreenZ(&vx[0], z);
    RwIm2DVertexSetIntRGBA(&vx[0], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[0], uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[0], uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[1], x1);
    RwIm2DVertexSetScreenY(&vx[1], y2);
    RwIm2DVertexSetScreenZ(&vx[1], z);
    RwIm2DVertexSetIntRGBA(&vx[1], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[1], uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[1], 1.0f + uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[2], x2);
    RwIm2DVertexSetScreenY(&vx[2], y1);
    RwIm2DVertexSetScreenZ(&vx[2], z);
    RwIm2DVertexSetIntRGBA(&vx[2], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[2], 1.0f + uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[2], uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[3], x2);
    RwIm2DVertexSetScreenY(&vx[3], y2);
    RwIm2DVertexSetScreenZ(&vx[3], z);
    RwIm2DVertexSetIntRGBA(&vx[3], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[3], 1.0f + uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[3], 1.0f + uvOffset, recipCamZ);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vx, ARRAY_LENGTH(vx));

    return TRUE;
}

static void InvertRaster(RwCamera* camera)
{
    RwIm2DVertex vx[4];
    F32 crw, crh;
    RwRaster* camRas;
    F32 recipZ;

    camRas = RwCameraGetRaster(camera);
    crw = RwRasterGetWidth(camRas);
    crh = RwRasterGetHeight(camRas);

    recipZ = 1.0f / RwCameraGetNearClipPlane(camera);

    RwIm2DVertexSetScreenX(&vx[0], 0.0f);
    RwIm2DVertexSetScreenY(&vx[0], 0.0f);
    RwIm2DVertexSetScreenZ(&vx[0], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetIntRGBA(&vx[0], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[0], 0.0f, recipZ);
    RwIm2DVertexSetV(&vx[0], 0.0f, recipZ);

    RwIm2DVertexSetScreenX(&vx[1], 0.0f);
    RwIm2DVertexSetScreenY(&vx[1], crh);
    RwIm2DVertexSetScreenZ(&vx[1], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetIntRGBA(&vx[1], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[1], 0.0f, recipZ);
    RwIm2DVertexSetV(&vx[1], 1.0f, recipZ);

    RwIm2DVertexSetScreenX(&vx[2], crw);
    RwIm2DVertexSetScreenY(&vx[2], 0.0f);
    RwIm2DVertexSetScreenZ(&vx[2], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetIntRGBA(&vx[2], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[2], 1.0f, recipZ);
    RwIm2DVertexSetV(&vx[2], 0.0f, recipZ);

    RwIm2DVertexSetScreenX(&vx[3], crw);
    RwIm2DVertexSetScreenY(&vx[3], crh);
    RwIm2DVertexSetScreenZ(&vx[3], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetIntRGBA(&vx[3], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[3], 1.0f, recipZ);
    RwIm2DVertexSetV(&vx[3], 1.0f, recipZ);

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVDESTCOLOR);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vx, ARRAY_LENGTH(vx));

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
}

static void ShadowCameraDestroy(RwCamera* shadowCamera)
{
    if (shadowCamera) {
        RwRaster* raster;

        _rwFrameSyncDirty();

        RwFrame* frame = RwCameraGetFrame(shadowCamera);
        if (frame) {
            RwCameraSetFrame(shadowCamera, NULL);
            RwFrameDestroy(frame);
        }

        raster = RwCameraGetZRaster(shadowCamera);
        if (raster) {
            RwCameraSetZRaster(shadowCamera, NULL);
            RwRasterDestroy(raster);
        }

        raster = RwCameraGetRaster(shadowCamera);
        if (raster) {
            RwCameraSetRaster(shadowCamera, NULL);
            RwRasterDestroy(raster);
        }

        RwCameraDestroy(shadowCamera);
    }
}

DECOMP_FORCEFLOAT(0.001f)

static RwCamera* ShadowCameraUpdate(RwCamera* shadowCamera, void* model, void (*renderCB)(void*), xVec3*, F32, S32 shadowMode)
{
    RwRGBA bgColor = { 255, 255, 255, 0 };

    RwCamera* camera = RwCameraGetCurrentCamera();

    S32 fogstate;
    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogstate);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);

    if (camera) {
        RwCameraEndUpdate(camera);
    }

    GCSaveFrameBuffer();

    shadowCamera->frameBuffer->width--;
    shadowCamera->frameBuffer->height--;
    RwCameraClear(shadowCamera, &bgColor, rwCAMERACLEARIMAGE);
    shadowCamera->frameBuffer->width++;
    shadowCamera->frameBuffer->height++;

    RwFrameOrthoNormalize(RwCameraGetFrame(shadowCamera));

    if (RwCameraBeginUpdateWrapper(shadowCamera)) {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

        renderCB(model);

        if (!shadowMode) {
            InvertRaster(shadowCamera);
        }

        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
        RwCameraEndUpdate(shadowCamera);

        RwGameCubeCameraTextureFlush(RwCameraGetRaster(shadowCamera), FALSE);
    }

    if (camera) {
        RwCameraBeginUpdateWrapper(camera);
    }

    GCRestoreFrameBuffer();

    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fogstate);

    return shadowCamera;
}

static RwRaster* ShadowRasterCreate(S32 size)
{
    RwRaster* raster;

    raster = RwRasterCreate(size, size, 0, rwRASTERTYPECAMERATEXTURE);

    return raster;
}

static RpCollisionTriangle* ShadowRenderTriangleCB(RpIntersection*, RpWorldSector*, RpCollisionTriangle* collTriangle, RwReal, void* data); // UNUSED

static void ShadowRenderCachePolys(ProjectionParam* param, xShadowPoly* poly, S32 count)
{
    for (S32 i = 0; i < count; i++, poly++) {
        RwIm3DVertex* imv;
        RwV3d vShad[3];
        RwV3d vIn[3];
        F32 cx, cy, cz;

        vIn[0] = *(RwV3d*)&poly->vert[0];
        vIn[1] = *(RwV3d*)&poly->vert[1];
        vIn[2] = *(RwV3d*)&poly->vert[2];

        RwV3dTransformPoints(vShad, vIn, ARRAY_LENGTH(vIn), &param->invMatrix);

        if ((vShad[0].z < 0.0f && vShad[1].z < 0.0f && vShad[2].z < 0.0f) || (vShad[0].x < 0.0f && vShad[1].x < 0.0f && vShad[2].x < 0.0f) || (vShad[0].x > 1.0f && vShad[1].x > 1.0f && vShad[2].x > 1.0f) || (vShad[0].y < 0.0f && vShad[1].y < 0.0f && vShad[2].y < 0.0f) || (vShad[0].y > 1.0f && vShad[1].y > 1.0f && vShad[2].y > 1.0f)) {
            continue;
        }

        if (Im3DBufferPos > 477) {
            xIMRenderLegacy(Im3DBuffer, Im3DBufferPos, NULL, rwPRIMTYPETRILIST);
            param->numIm3DBatch++;
            Im3DBufferPos = 0;
        }

        imv = &Im3DBuffer[Im3DBufferPos];

        cx = poly->vert[0].w * SHADOW_BIAS_AMT;
        cy = poly->vert[1].w * SHADOW_BIAS_AMT;
        cz = poly->vert[2].w * SHADOW_BIAS_AMT;
        RwIm3DVertexSetPos(&imv[0], poly->vert[0].x + cx, poly->vert[0].y + cy, poly->vert[0].z + cz);
        RwIm3DVertexSetPos(&imv[1], poly->vert[1].x + cx, poly->vert[1].y + cy, poly->vert[1].z + cz);
        RwIm3DVertexSetPos(&imv[2], poly->vert[2].x + cx, poly->vert[2].y + cy, poly->vert[2].z + cz);

        imv[0].u = vShad[0].x;
        imv[1].u = vShad[1].x;
        imv[2].u = vShad[2].x;

        imv[0].v = vShad[0].y;
        imv[1].v = vShad[1].y;
        imv[2].v = vShad[2].y;

        U8 val = param->shadowValue;
        RwIm3DVertexSetRGBA(&imv[0], val, val, val, val);
        RwIm3DVertexSetRGBA(&imv[1], val, val, val, val);
        RwIm3DVertexSetRGBA(&imv[2], val, val, val, val);

        Im3DBufferPos += 3;
    }
}

DECOMP_FORCEBLOCK((const iEnv* env) {
    iEnvIsJSPVisible(env, 0);
})

void GCSaveFrameBuffer()
{
    RwGameCubeCameraTextureFlush(gc_saveraster, FALSE);
}

void GCRestoreFrameBuffer()
{
    F32 size = 128.0f;
    F32 uvOffset = 0.5f / size;
    F32 recipCamZ = 1.0f / RwCameraGetFarClipPlane(RwCameraGetCurrentCamera());
    void* oldAlphaFunction;
    void* oldTextureFilter;

    RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &oldAlphaFunction);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);

    RwRenderStateGet(rwRENDERSTATETEXTUREFILTER, &oldTextureFilter);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, gc_saveraster);

    Im2DRenderQuad(0.0f, 0.0f, size, size, RwIm2DGetFarScreenZ(), recipCamZ, uvOffset);

    if ((S32)oldAlphaFunction >= 1)
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, oldAlphaFunction);
    else
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATEREQUAL);

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

    if ((S32)oldTextureFilter >= 1) {
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, oldTextureFilter);
    }
}

static RwCamera* ShadowCameraCreatePersp(S32 size)
{
    RwCamera* shadowCamera;

    shadowCamera = RwCameraCreateWrapper();
    if (shadowCamera) {
        RwCameraSetFrame(shadowCamera, RwFrameCreate());

        RwV2d vw;
        vw.x = 0.001745331f;
        vw.y = 0.001745331f;
        RwCameraSetViewWindow(shadowCamera, &vw);

        if (RwCameraGetFrame(shadowCamera)) {
            RwRaster* zRaster = RwRasterCreate(size, size, 0, rwRASTERTYPEZBUFFER);
            if (zRaster) {
                RwCameraSetZRaster(shadowCamera, zRaster);

                return shadowCamera;
            }
        }
    }

    ShadowCameraDestroy(shadowCamera);

    return NULL;
}

F32 rscale = 1.0f;
RwV3d gCamPos = { 0.0f, 0.0f, 0.0f };

static RwCamera* ShadowCameraSetSpherePersp(RwCamera* camera, RwV3d* center, F32 radius)
{
    RwFrame* camFrame = RwCameraGetFrame(camera);
    RwMatrix* camMatrix = RwFrameGetMatrix(camFrame);
    RwV3d* camPos = RwMatrixGetPos(camMatrix);

    F32 objDepth = 572.95807f * radius;

    F32 nearZ = objDepth - rscale * radius;
    F32 farZ = objDepth + rscale * radius;

    camera->nearPlane = nearZ;
    camera->farPlane = farZ;

    RwCameraSetNearClipPlane(camera, nearZ);
    RwCameraSetFarClipPlane(camera, farZ);

    *camPos = *center;

    RwV3dIncrementScaled(camPos, RwMatrixGetAt(camMatrix), -objDepth);

    gCamPos = *camPos;

    RwMatrixUpdate(camMatrix);
    RwFrameUpdateObjects(camFrame);

    gShadowObjectRadius = radius;

    return camera;
}

static U8 ShadowInsideBoxAdjust(xVec3* volume); // UNUSED
static void DrawAlphaBox(xVec3* volume, S32 frontface, U8 alpha); // UNUSED
static void xShadowSetVolume(RwCamera* shadowCamera, xVec3* pos, F32 depth); // UNUSED

struct ShadowCacheContext
{
    xShadowCache* cache;
    F32 minNormY;
};

static RpCollisionTriangle* shadowCacheEnvCB(RpIntersection*, RpWorldSector*, RpCollisionTriangle* tri, RwReal, void* data)
{
    ShadowCacheContext* context = (ShadowCacheContext*)data;
    xShadowCache* cache = context->cache;

    if (cache->polyCount >= SHADOW_CACHE_POLY_MAX) {
        return NULL;
    }

    if (tri->normal.y < context->minNormY) {
        return tri;
    }

    if (sShadowCollJSP && !(((xClumpCollBSPTriangle*)tri->index)->flags & 0x8)) {
        return tri;
    }

    xShadowPoly* poly = &cache->poly[cache->polyCount];
    cache->polyCount++;
    *(xVec3*)&poly->vert[0] = *(xVec3*)tri->vertices[0];
    *(xVec3*)&poly->vert[1] = *(xVec3*)tri->vertices[1];
    *(xVec3*)&poly->vert[2] = *(xVec3*)tri->vertices[2];
    poly->vert[0].w = tri->normal.x;
    poly->vert[1].w = tri->normal.y;
    poly->vert[2].w = tri->normal.z;

    F32 dydx = -tri->normal.x / tri->normal.y;
    F32 dydz = -tri->normal.z / tri->normal.y;
    F32 depth0 = poly->vert[0].y + (dydx * (cache->pos.x - poly->vert[0].x) + dydz * (cache->pos.z - poly->vert[0].z));

    F32 n0x = poly->vert[0].z - poly->vert[1].z;
    F32 n0z = poly->vert[1].x - poly->vert[0].x;
    F32 n0d = n0x * (cache->pos.x - poly->vert[0].x) + n0z * (cache->pos.z - poly->vert[0].z);

    F32 n1x = poly->vert[1].z - poly->vert[2].z;
    F32 n1z = poly->vert[2].x - poly->vert[1].x;
    F32 n1d = n1x * (cache->pos.x - poly->vert[1].x) + n1z * (cache->pos.z - poly->vert[1].z);

    F32 n2x = poly->vert[2].z - poly->vert[0].z;
    F32 n2z = poly->vert[0].x - poly->vert[2].x;
    F32 n2d = n2x * (cache->pos.x - poly->vert[2].x) + n2z * (cache->pos.z - poly->vert[2].z);

    if (n0d <= 0.00001f && n1d <= 0.00001f && n2d <= 0.00001f) {
        cache->polyRayDepth[0] = MAX(depth0, cache->polyRayDepth[0]);
    }

    if (n0d + 0.5f * n0x * cache->radius <= 0.00001f && n1d + 0.5f * n1x * cache->radius <= 0.00001f && n2d + 0.5f * n2x * cache->radius <= 0.00001f) {
        cache->polyRayDepth[1] = MAX(depth0 + 0.5f * dydx * cache->radius, cache->polyRayDepth[1]);
    }

    if (n0d - 0.5f * n0x * cache->radius <= 0.00001f && n1d - 0.5f * n1x * cache->radius <= 0.00001f && n2d - 0.5f * n2x * cache->radius <= 0.00001f) {
        cache->polyRayDepth[2] = MAX(depth0 - 0.5f * dydx * cache->radius, cache->polyRayDepth[2]);
    }

    if (n0d + 0.5f * n0z * cache->radius <= 0.00001f && n1d + 0.5f * n1z * cache->radius <= 0.00001f && n2d + 0.5f * n2z * cache->radius <= 0.00001f) {
        cache->polyRayDepth[3] = MAX(depth0 + 0.5f * dydz * cache->radius, cache->polyRayDepth[3]);
    }

    if (n0d - 0.5f * n0z * cache->radius <= 0.00001f && n1d - 0.5f * n1z * cache->radius <= 0.00001f && n2d - 0.5f * n2z * cache->radius <= 0.00001f) {
        cache->polyRayDepth[4] = MAX(depth0 - 0.5f * dydz * cache->radius, cache->polyRayDepth[4]);
    }

    return tri;
}

struct ShadowCBParam
{
    xShadowCache* cache;
    RpIntersection* isx;
    xVec3 capsuleStart;
    xVec3 capsuleEnd;
    F32 capsuleRadius;
    xEnt* ent;
    RwLine localLine;
    RwV3d localDelta;
    F32 localRadius;
    RwMatrix* modelMat;
    RpGeometry* geom;
    U32 polyFound;
    xEnt* rayCloser[5];
};

static S32 shadowCacheLeafCB(S32 numTriangles, S32 triOffset, void* data)
{
    ShadowCBParam* cbparam = (ShadowCBParam*)data;
    xShadowCache* cache = cbparam->cache;
    RpGeometry* geometry = cbparam->geom;
    RwV3d* vertices = geometry->morphTarget->verts;
    RpTriangle* triangles = geometry->triangles;
    S32 triSlot;
    U16* triIndex = _rpCollisionGeometryGetDataMacro(geometry)->tree->map;

    if (triIndex) {
        triIndex += triOffset;
        triSlot = 0;
    } else {
        triSlot = triOffset;
    }

    while (numTriangles--) {
        S32 i;

        if (triIndex) {
            triSlot = *triIndex;
            triIndex++;
        } else {
            triSlot++;
        }

        const RpTriangle* const tri = &triangles[triSlot];
        S32 vertIndex0 = tri->vertIndex[0];
        S32 vertIndex1 = tri->vertIndex[1];
        S32 vertIndex2 = tri->vertIndex[2];
        RwV3d* v0 = &vertices[vertIndex0];
        RwV3d* v1 = &vertices[vertIndex1];
        RwV3d* v2 = &vertices[vertIndex2];
        xVec3 worldV[3];

        xMat4x3Toworld(&worldV[0], (xMat4x3*)cbparam->modelMat, (xVec3*)v0);
        xMat4x3Toworld(&worldV[1], (xMat4x3*)cbparam->modelMat, (xVec3*)v1);
        xMat4x3Toworld(&worldV[2], (xMat4x3*)cbparam->modelMat, (xVec3*)v2);

        F32 posX = cbparam->capsuleStart.x, posZ = cbparam->capsuleStart.z;

        for (i = 0; i < 3; i++) {
            F32 nx, nz, nmag2, pdot;
            xVec3* vert0 = &worldV[i];
            xVec3* vert1 = &worldV[i == 2 ? 0 : i + 1];

            nx = vert0->z - vert1->z;
            nz = vert1->x - vert0->x;
            nmag2 = xsqr(nx) + xsqr(nz);
            pdot = nx * (posX - vert0->x) + nz * (posZ - vert0->z);
            if (pdot > 0.0f && xsqr(pdot) >= nmag2 * xsqr(cbparam->capsuleRadius)) {
                goto next;
            }
        }

        for (i = 0; i < 3; i++) {
            F32 dotA, dotB;
            xVec3* vert0 = &worldV[i];
            xVec3* vert1 = &worldV[(i + 1) % 3];
            xVec3* vert2 = &worldV[(i + 2) % 3];

            dotA = (vert1->x - vert0->x) * (posX - vert0->x) + (vert1->z - vert0->z) * (posZ - vert0->z);
            dotB = (vert2->x - vert0->x) * (posX - vert0->x) + (vert2->z - vert0->z) * (posZ - vert0->z);
            if (dotA < 0.0f && dotB < 0.0f && xsqr(posX - vert0->x) + xsqr(posZ - vert0->z) > xsqr(cbparam->capsuleRadius)) {
                goto next;
            }
        }

        cbparam->polyFound++;

        xVec3 aa, bb, trinorm;
        F32 depthtest, dydx, dydz, depth0;
        F32 n0x, n0z, n0d, n1x, n1z, n1d, n2x, n2z, n2d, denom;

        xVec3Sub(&aa, (xVec3*)v1, (xVec3*)v0);
        xVec3Sub(&bb, (xVec3*)v2, (xVec3*)v0);
        xVec3Cross(&trinorm, &aa, &bb);
        xVec3Normalize(&trinorm, &trinorm);

        denom = xabs(trinorm.y) > 0.00001f ? trinorm.y : 0.00001f;

        dydx = -trinorm.x / denom;
        dydz = -trinorm.z / denom;
        depth0 = worldV[0].y + (dydx * (cache->pos.x - worldV[0].x) + dydz * (cache->pos.z - worldV[0].z));

        n0x = worldV[0].z - worldV[1].z;
        n0z = worldV[1].x - worldV[0].x;
        n0d = n0x * (cache->pos.x - worldV[0].x) + n0z * (cache->pos.z - worldV[0].z);

        n1x = worldV[1].z - worldV[2].z;
        n1z = worldV[2].x - worldV[1].x;
        n1d = n1x * (cache->pos.x - worldV[1].x) + n1z * (cache->pos.z - worldV[1].z);

        n2x = worldV[2].z - worldV[0].z;
        n2z = worldV[0].x - worldV[2].x;
        n2d = n2x * (cache->pos.x - worldV[2].x) + n2z * (cache->pos.z - worldV[2].z);

        if (n0d <= 0.00001f && n1d <= 0.00001f && n2d <= 0.00001f) {
            depthtest = depth0;
            if (depthtest > cache->polyRayDepth[0]) {
                cbparam->rayCloser[0] = cbparam->ent;
                cache->polyRayDepth[0] = depthtest;
            }
        }

        if (n0d + 0.5f * n0x * cache->radius <= 0.00001f && n1d + 0.5f * n1x * cache->radius <= 0.00001f && n2d + 0.5f * n2x * cache->radius <= 0.00001f) {
            depthtest = depth0 + 0.5f * dydx * cache->radius;
            if (depthtest > cache->polyRayDepth[1]) {
                cbparam->rayCloser[1] = cbparam->ent;
                cache->polyRayDepth[1] = depthtest;
            }
        }

        if (n0d - 0.5f * n0x * cache->radius <= 0.00001f && n1d - 0.5f * n1x * cache->radius <= 0.00001f && n2d - 0.5f * n2x * cache->radius <= 0.00001f) {
            depthtest = depth0 - 0.5f * dydx * cache->radius;
            if (depthtest > cache->polyRayDepth[2]) {
                cbparam->rayCloser[2] = cbparam->ent;
                cache->polyRayDepth[2] = depthtest;
            }
        }

        if (n0d + 0.5f * n0z * cache->radius <= 0.00001f && n1d + 0.5f * n1z * cache->radius <= 0.00001f && n2d + 0.5f * n2z * cache->radius <= 0.00001f) {
            depthtest = depth0 + 0.5f * dydz * cache->radius;
            if (depthtest > cache->polyRayDepth[3]) {
                cbparam->rayCloser[3] = cbparam->ent;
                cache->polyRayDepth[3] = depthtest;
            }
        }

        if (n0d - 0.5f * n0z * cache->radius <= 0.00001f && n1d - 0.5f * n1z * cache->radius <= 0.00001f && n2d - 0.5f * n2z * cache->radius <= 0.00001f) {
            depthtest = depth0 - 0.5f * dydz * cache->radius;
            if (depthtest > cache->polyRayDepth[4]) {
                cbparam->rayCloser[4] = cbparam->ent;
                cache->polyRayDepth[4] = depthtest;
            }
        }

    next:;
    }

    return 1;
}

static const xEnt* sEntSelf;

static S32 shadowCacheEntityCB(xEnt* ent, void* cbdata)
{
    if (!(ent->baseFlags & k_XBASE_RECEIVES_SHADOWS)) {
        return 1;
    }

    ShadowCBParam* cbparam = (ShadowCBParam*)cbdata;
    if (cbparam->cache->entCount >= SHADOW_CACHE_ENT_MAX) {
        return 0;
    }

    if (ent == sEntSelf) {
        return 1;
    }

    if (ent->baseType == eBaseTypeNPC) {
        return 1;
    }

    xCollis coll;
    coll.flags = 0;
    if (ent->bound.type == k_XBOUNDTYPE_SPHERE) {
        xBoxHitsSphere((xBox*)&cbparam->isx->t.box, &ent->bound.sph, &coll);
    } else if (ent->bound.type == k_XBOUNDTYPE_OBB) {
        xBoxHitsObb((xBox*)&cbparam->isx->t.box, &ent->bound.box.box, ent->bound.mat, &coll);
    } else if (ent->bound.type == k_XBOUNDTYPE_BOX) {
        if (((xBox*)&cbparam->isx->t.box)->upper.x > ent->bound.box.box.lower.x &&
            ((xBox*)&cbparam->isx->t.box)->upper.y > ent->bound.box.box.lower.y &&
            ((xBox*)&cbparam->isx->t.box)->upper.z > ent->bound.box.box.lower.z &&
            ((xBox*)&cbparam->isx->t.box)->lower.x < ent->bound.box.box.upper.x &&
            ((xBox*)&cbparam->isx->t.box)->lower.y < ent->bound.box.box.upper.y &&
            ((xBox*)&cbparam->isx->t.box)->lower.z < ent->bound.box.box.upper.z) {
            coll.flags |= 0x1;
        }
    }

    if (coll.flags & k_HIT_IT) {
        xModelInstance* m = ent->collModel ? ent->collModel : ent->model;
        RpCollisionData* colldata = _rpCollisionGeometryGetDataMacro(m->Data->geometry);
        F32 invScale;
        RwMatrix inverseLTM;

        if (m->Data->boundingSphere.radius > 2.0f && colldata && colldata->tree) {
            RwMatrixInvert(&inverseLTM, m->Mat);
            RwV3dTransformPoints(&cbparam->localLine.start, (RwV3d*)&cbparam->capsuleStart, 2, &inverseLTM);
            RwV3dSub(&cbparam->localDelta, &cbparam->localLine.end, &cbparam->localLine.start);
            invScale = xVec3Length((xVec3*)&inverseLTM.right);
            cbparam->localRadius = cbparam->capsuleRadius * invScale;
            cbparam->geom = m->Data->geometry;
            cbparam->modelMat = m->Mat;
            cbparam->polyFound = 0;
            cbparam->ent = ent;
            memset(cbparam->rayCloser, 0, sizeof(cbparam->rayCloser));

            RpCollTreeForAllCapsuleIntsec(colldata->tree, &cbparam->localLine, cbparam->localRadius, shadowCacheLeafCB, cbdata);

            if (cbparam->polyFound) {
                cbparam->cache->ent[cbparam->cache->entCount++] = ent;
            }
        } else {
            cbparam->cache->ent[cbparam->cache->entCount++] = ent;
        }
    }

    return 1;
}

static bool sCastEnvironment = true;

void xShadowVertical_FillCache(xShadowCache* cache, xVec3* pos, F32 r, F32 depth, F32 minNormY)
{
    S32 i, j;

    cache->pos = *pos;
    cache->radius = r;
    cache->entCount = 0;
    cache->polyCount = 0;
    for (i = 0; i < 5; i++) {
        cache->polyRayDepth[i] = -1e38f;
    }

    xEnv* env = xglobals->sceneCur->env;

    RpIntersection isx;
    isx.type = rpINTERSECTBOX;
    isx.t.box.sup.x = pos->x + r;
    isx.t.box.sup.y = pos->y + r;
    isx.t.box.sup.z = pos->z + r;
    isx.t.box.inf.x = pos->x - r;
    isx.t.box.inf.y = pos->y - r - depth;
    isx.t.box.inf.z = pos->z - r;

    ShadowCacheContext context;
    context.cache = cache;
    context.minNormY = minNormY;

    if (sCastEnvironment) {
        sShadowCollJSP = TRUE;
        for (S32 i = 0; i < env->geom->jsp_count; i++) {
            if (iEnvIsJSPVisible(env->geom, i)) {
                xClumpColl_ForAllIntersections(env->geom->jsp_list[i]->colltree, &isx, shadowCacheEnvCB, &context);
            }
        }
        sShadowCollJSP = FALSE;
    }

    F32 sortRayDepth[5];
    F32 tmpswap, objMaxDepth;
    xQCData qcd;
    ShadowCBParam cbparam;

    memcpy(sortRayDepth, cache->polyRayDepth, sizeof(sortRayDepth));
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 4; j++) {
            if (sortRayDepth[j] > sortRayDepth[j + 1]) {
                tmpswap = sortRayDepth[j];
                sortRayDepth[j] = sortRayDepth[j + 1];
                sortRayDepth[j + 1] = tmpswap;
            }
        }
    }

    objMaxDepth = sortRayDepth[2];
    if (objMaxDepth == -1e38f) {
        objMaxDepth = pos->y - depth;
    }
    if (objMaxDepth > pos->y - 0.001f) {
        objMaxDepth = pos->y - 0.001f;
    }

    if (objMaxDepth - 1.0f > isx.t.box.inf.y) {
        isx.t.box.inf.y = objMaxDepth - 1.0f;
    }

    cbparam.cache = cache;
    cbparam.isx = &isx;
    cbparam.capsuleStart.x = pos->x;
    cbparam.capsuleStart.y = pos->y;
    cbparam.capsuleStart.z = pos->z;
    cbparam.capsuleEnd.x = pos->x;
    cbparam.capsuleEnd.y = objMaxDepth;
    cbparam.capsuleEnd.z = pos->z;
    cbparam.capsuleRadius = r;

    xQuickCullForBox(&qcd, (xBox*)&isx.t.box);
    xGridCheckPosition(&colls_grid, (xVec3*)&isx.t.box, &qcd, shadowCacheEntityCB, &cbparam);
    xGridCheckPosition(&colls_oso_grid, (xVec3*)&isx.t.box, &qcd, shadowCacheEntityCB, &cbparam);

    cache->castOnEnt = (cache->entCount != 0);
    cache->castOnPoly = (cache->polyCount != 0);
}

void xShadowVertical_DrawCache(xShadowCache* cache, F32 shadowFactor, F32 fadeDist, S32 shadowMode, RwMatrix* shadowMat, RwRaster* shadowRast)
{
    ProjectionParam param;
    RwMatrix* shadowMatrix;
    F32 radius;
    RwV3d scl, tr;
    xVec3 A, B;
    S32 old_zwrite, fogstate;

    if (shadowRast) {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, shadowRast);
    } else {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwCameraGetRaster(ShadowCamera));
    }

    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

    RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &old_zwrite);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);

    switch (shadowMode) {
    case 1:
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        break;
    case 0:
    default:
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDZERO);
        break;
    }

    if (shadowFactor < 0.0f) {
        shadowFactor = -shadowFactor;

        switch (shadowMode) {
        case 1:
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCALPHA);
            break;
        case 0:
        default:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCCOLOR);
            break;
        }
    } else {
        switch (shadowMode) {
        case 1:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            break;
        case 0:
        default:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCCOLOR);
            break;
        }
    }

    if (shadowMat) {
        shadowMatrix = shadowMat;
    } else {
        shadowMatrix = RwFrameGetMatrix(RwCameraGetFrame(ShadowCamera));
    }

    param.at = *RwMatrixGetAt(shadowMatrix);
    radius = gShadowObjectRadius;

    RwMatrixInvert(&param.invMatrix, shadowMatrix);

    scl.x = scl.y = -0.5f / radius;
    scl.z = 1.0f / (fadeDist + radius);
    RwMatrixScale(&param.invMatrix, &scl, rwCOMBINEPOSTCONCAT);

    param.fade = fadeDist > 0.0f ? 1 : 0;
    param.shadowValue = 255.0f * shadowFactor;
    param.shadowWord = RWRGBALONG(param.shadowValue, param.shadowValue, param.shadowValue, param.shadowValue);
    param.numIm3DBatch = 0;

    Im3DBuffer = gRenderBuffer.m_vertex;
    Im3DBufferPos = 0;

    xVec3Add(&A, (xVec3*)&shadowMatrix->pos, (xVec3*)&shadowMatrix->at);
    RwV3dTransformPoints((RwV3d*)&B, (RwV3d*)&A, 1, &param.invMatrix);

    tr.x = tr.y = 0.5f;
    tr.z = 0.0f;
    RwMatrixTranslate(&param.invMatrix, &tr, rwCOMBINEPOSTCONCAT);

    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogstate);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);

    ShadowRenderCachePolys(&param, cache->poly, cache->polyCount);

    if (Im3DBufferPos) {
        xIMRenderLegacy(Im3DBuffer, Im3DBufferPos, NULL, rwPRIMTYPETRILIST);
        Im3DBufferPos = 0;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)old_zwrite);

    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fogstate);

    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
}

struct xShadowMgr
{
    xEnt* ent;
    xShadowCache* cache;
    S32 priority;
    S32 cacheReady;
};

static S32 SHADOW_CACHE_MAX = 6;
static xShadowMgr* sMgrList;
static S32 sMgrCount;
static S32 sMgrTotal;
static xShadowCache* sCacheList;

void xShadowManager_Init(S32 numEnts, S32 maxPolys, S32 maxCaches)
{
    SHADOW_CACHE_MAX = maxCaches;
    sMgrList = (xShadowMgr*)xMEMALLOC(numEnts * sizeof(xShadowMgr), 0);
    sMgrTotal = numEnts;
    sMgrCount = 0;

    sCacheList = new (eMemStaticType_0, 0) xShadowCache[SHADOW_CACHE_MAX];

    SHADOW_CACHE_POLY_MAX = maxPolys;

    S32 polys_size = SHADOW_CACHE_POLY_MAX * SHADOW_CACHE_MAX;
    xShadowPoly* polys = new (eMemStaticType_0, 0) xShadowPoly[polys_size];
    for (S32 i = 0; i < SHADOW_CACHE_MAX; i++) {
        sCacheList[i].poly = polys;
        polys += SHADOW_CACHE_POLY_MAX;
    }
}

void xShadowManager_Add(xEnt* ent)
{
    for (S32 i = 0; i < sMgrCount; i++) {
        if (sMgrList[i].ent == ent) {
            return;
        }
    }

    if (sMgrCount >= sMgrTotal) {
        return;
    }

    sMgrList[sMgrCount].ent = ent;
    sMgrList[sMgrCount].cache = NULL;
    sMgrList[sMgrCount].priority = 1000;
    sMgrList[sMgrCount].cacheReady = FALSE;
    sMgrCount++;
}

// UNUSED: needed to match (code from TSSM GC, BFBB GC)
void xShadowManager_Remove(xEnt* ent)
{
    S32 i;

    for (i = SHADOW_CACHE_MAX; i < sMgrCount; i++) {
        sMgrList[i].cache = NULL;
    }

    for (i = 0; i < sMgrCount;) {
        if (sMgrList[i].ent == ent) {
            sMgrList[i] = sMgrList[sMgrCount - 1];
            sMgrCount--;
        } else {
            i++;
        }
    }
}

static S32 CmpShadowMgr(const void* a, const void* b)
{
    xEnt* entA = ((xShadowMgr*)a)->ent;
    xEnt* entB = ((xShadowMgr*)b)->ent;
    S32 isPlayerA = (entA->baseType == eBaseTypePlayer || entA->baseType == eBaseTypeBoulder);
    S32 isPlayerB = (entB->baseType == eBaseTypePlayer || entB->baseType == eBaseTypeBoulder);
    F32 distA, distB;

    if (isPlayerA && !isPlayerB) return -1;
    if (isPlayerB && !isPlayerA) return 1;

    xVec3* campos = &xglobals->cam->mat.pos;
    xVec3Dist2Macro(campos, &entA->model->Mat->pos, &distA);
    xVec3Dist2Macro(campos, &entB->model->Mat->pos, &distB);
    if (distA < distB) return -1;
    if (distA > distB) return 1;
    return 0;
}

static void xShadowModelRender(xModelInstance* minst)
{
    xMat3x3 tmpmat;
    S32 i;

    while (minst) {
        if ((minst->Flags & 0x401) == 0x1) {
            if (minst->Scale.x) {
                xMat3x3Copy(&tmpmat, (xMat3x3*)minst->Mat);

                minst->Mat->right.x *= minst->Scale.x;
                minst->Mat->right.y *= minst->Scale.x;
                minst->Mat->right.z *= minst->Scale.x;

                minst->Mat->up.x *= minst->Scale.y;
                minst->Mat->up.y *= minst->Scale.y;
                minst->Mat->up.z *= minst->Scale.y;

                minst->Mat->at.x *= minst->Scale.z;
                minst->Mat->at.y *= minst->Scale.z;
                minst->Mat->at.z *= minst->Scale.z;
            }

            RpMaterialList* matList = &minst->Data->geometry->matList;
            RwTexture* oldtex[8];

            if (matList->numMaterials <= 8) {
                for (i = 0; i < matList->numMaterials; i++) {
                    oldtex[i] = matList->materials[i]->texture;
                    matList->materials[i]->texture = NULL;
                }
            }

            if (minst->BoneCount) {
                iModelSetWorldBoundFromBox(minst->Data, &minst->animBound);
            } else {
                iModelWorldBoundSphere(minst->Data, minst->Mat, NULL);
            }

            iModelRender(minst->Data, minst->Mat);

            if (matList->numMaterials <= 8) {
                for (i = 0; i < matList->numMaterials; i++) {
                    matList->materials[i]->texture = oldtex[i];
                }
            }

            if (minst->Scale.x) {
                xMat3x3Copy((xMat3x3*)minst->Mat, &tmpmat);
            }
        }

        minst = minst->Next;
    }
}

static void xShadow_PickEntForNPC(xShadowMgr* mgr);

void xShadowManager_Render()
{
    S32 i;
    S32 cacheUsed[10];
    S32 j;
    F32 shadow_strength;
    U8 old_xClumpColl_FilterFlags;

    old_xClumpColl_FilterFlags = xClumpColl_FilterFlags;
    xClumpColl_FilterFlags |= 0x20;

    for (i = SHADOW_CACHE_MAX; i < sMgrCount; i++) {
        sMgrList[i].cache = NULL;
    }

    for (i = 0; i < sMgrCount;) {
        xEnt* ent_ref = sMgrList[i].ent;
        if (!xEntIsVisible(ent_ref) || !ent_ref->entShadow || (ent_ref->model->Flags & 0x400)) {
            sMgrList[i] = sMgrList[sMgrCount - 1];
            sMgrCount--;
        } else {
            i++;
        }
    }

    qsort(sMgrList, sMgrCount, sizeof(xShadowMgr), CmpShadowMgr);

    memset(cacheUsed, 0, sizeof(cacheUsed));
    for (i = 0; i < SHADOW_CACHE_MAX && i < sMgrCount; i++) {
        if (sMgrList[i].cache) {
            cacheUsed[sMgrList[i].cache - sCacheList] = TRUE;
        }
    }
    for (i = 0; i < SHADOW_CACHE_MAX && i < sMgrCount; i++) {
        if (!sMgrList[i].cache) {
            for (j = 0; j < SHADOW_CACHE_MAX; j++) {
                if (!cacheUsed[j]) {
                    sMgrList[i].cache = &sCacheList[j];
                    sMgrList[i].cacheReady = FALSE;
                    sMgrList[i].priority = 1000;
                    sCacheList[j].entCount = 0;
                    sCacheList[j].polyCount = 0;
                    cacheUsed[j] = TRUE;
                    break;
                }
            }
        }
    }
    for (i = SHADOW_CACHE_MAX; i < sMgrCount; i++) {
        sMgrList[i].cache = NULL;
    }

    S32 bestIndex = -1;
    S32 foundPriority = -1;
    for (i = 0; i < SHADOW_CACHE_MAX && i < sMgrCount; i++) {
        sMgrList[i].priority++;
        if (sMgrList[i].priority > foundPriority) {
            bestIndex = i;
            foundPriority = sMgrList[i].priority;
        }
    }

    if (bestIndex != -1) {
        xVec3 center;
        F32 radius;

        zEntGetShadowParams(sMgrList[bestIndex].ent, &center, &radius, &shadow_strength, xEntShadow::RADIUS_CACHE);

        xShadowMgr* mgr_best = &sMgrList[bestIndex];

        F32 dst_depth = DefaultShadowDepth;
        const xEnt* ep = mgr_best->ent;
        if (ep->entShadow->dst_cast > 0.0f) {
            dst_depth = ep->entShadow->dst_cast;
        }

        sEntSelf = ep;
        xShadowVertical_FillCache(mgr_best->cache, &center, radius, dst_depth, 0.0871557f);
        sEntSelf = NULL;

        sMgrList[bestIndex].priority = 0;
        sMgrList[bestIndex].cacheReady = TRUE;

        if (enableNPCShadowOneEnt) {
            xShadow_PickEntForNPC(mgr_best);
        }
    }

    for (i = 0; i < sMgrCount; i++) {
        xEnt* ent = sMgrList[i].ent;
        if (ent->flags & 0x40) {
            continue;
        }

        F32 dst_depth = DefaultShadowDepth;
        if (ent->entShadow->dst_cast > 0.0f) {
            dst_depth = ent->entShadow->dst_cast;
        }

        S32 shadowOutside;
        xModelCullGroupPlusShadow(ent->model, dst_depth, &shadowOutside);
        if (!shadowOutside) {
            if (sMgrList[i].cache && sMgrList[i].cacheReady) {
                xVec3 center;
                F32 radius;

                ent->entShadow->pos.x = ent->model->Mat->pos.x;
                ent->entShadow->pos.y = ent->model->Mat->pos.y + 1.0f;
                ent->entShadow->pos.z = ent->model->Mat->pos.z;
                ent->entShadow->vec.x = 0.0f;
                ent->entShadow->vec.y = -1.0f;
                ent->entShadow->vec.z = 0.0f;
                xShadowSetLight(&ent->entShadow->pos, &ent->entShadow->vec, 1.0f);

                zEntGetShadowParams(ent, &center, &radius, &shadow_strength, xEntShadow::RADIUS_RASTER);

                if (radius <= 0.0f) {
                    continue;
                }

                RpAtomic* old_model = NULL;
                xModelInstance* old_mnext = ent->model->Next;
                if (ent->entShadow->shadowModel) {
                    RpAtomic* shadModel = ent->entShadow->shadowModel;
                    old_model = ent->model->Data;
                    ent->model->Data = shadModel;
                    ent->model->Next = NULL;
                }

                xLightKit_Enable(NULL, xglobals->currWorld);

                xShadowCameraUpdate(ent->model, (void (*)(void*))xShadowModelRender, &center, radius, 0);

                if (old_model) {
                    ent->model->Data = old_model;
                    ent->model->Next = old_mnext;
                }

                xShadowVertical_DrawCache(sMgrList[i].cache, shadow_strength, 0.0f, 0, NULL, NULL);

                if (i == 0 || sMgrList[i].ent->baseType == eBaseTypeBoulder) {
                    for (j = 0; j < sMgrList[i].cache->entCount; j++) {
                        xEnt* ep = sMgrList[i].cache->ent[j];
                        if (xShadowReceiveShadowSetup(ep))
                            xShadowReceiveShadow(ep, 0.3f, 0, NULL, NULL);
                    }
                }
                else if (enableNPCShadowOneEnt || enableNPCShadowAllEnt) {
                    xShadowMgr* mgr = &sMgrList[i];

                    if (mgr->ent->baseType == eBaseTypeNPC) {
                        if (mgr->cache->entCount >= 1) {
                            const xEntShadow* npcshad = mgr->ent->entShadow;
                            if (npcshad) {
                                S32 num;

                                if (npcshad->flg_castOnAllDFF)
                                    num = mgr->cache->entCount;
                                else if (npcshad->flg_castOnOneDFF)
                                    num = 1;
                                else
                                    continue;

                                for (S32 a = 0; a < num; a++) {
                                    xEnt* ep = mgr->cache->ent[a];
                                    if (xShadowReceiveShadowSetup(ep)) {
                                        xShadowReceiveShadow(ep, 0.3f, 0, NULL, NULL);
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (ent && ent->simpShadow) {
                F32 rad;
                if (ent->baseType == eBaseTypeBoulder) {
                    rad = ent->model->Data->boundingSphere.radius;
                    if (rad > 0.75f) rad = 0.75f;
                    rad *= 2.0f;
                }
                else if (ent->bound.type == k_XBOUNDTYPE_SPHERE) {
                    rad = ent->bound.sph.r;
                } else {
                    xVec3* lower = &ent->bound.box.box.lower;
                    xVec3* upper = &ent->bound.box.box.upper;
                    rad = 0.167f * (upper->x + upper->y + upper->z - lower->x - lower->y - lower->z);
                }

                xShadowSimple_Add(ent->simpShadow, ent, rad, 1.0f);
            }
        }
    }

    xClumpColl_FilterFlags = old_xClumpColl_FilterFlags;
}

static void xShadow_PickByRayCast(xShadowMgr* mgr)
{
    xEnt* ent_best = NULL;
    S32 idx_best = -1;
    F32 dst_best = 21.7f;
    xCollis colrec;
    xRay3 ray;

    memset(&colrec, 0, sizeof(xCollis));

    ray.dir = g_NY3;
    ray.min_t = 0.0f;
    ray.max_t = 10.5f;
    ray.flags = XRAY3_USE_MIN | XRAY3_USE_MAX;

    S32 num = mgr->cache->entCount;
    for (S32 i = 0; i < num; i++) {
        xEnt* ep = mgr->cache->ent[i];

        colrec.flags = 0;
        colrec.flags |= 0x1F00;
        ray.origin.x = ep->model->Mat->pos.x;
        ray.origin.y = ep->model->Mat->pos.y;
        ray.origin.z = ep->model->Mat->pos.z;
        xRayHitsModel(&ray, ep->model, &colrec);

        if (!(colrec.flags & k_HIT_IT)) continue;

        if (colrec.dist > dst_best) {
            continue;
        } else {
            ent_best = ep;
            idx_best = i;
        }
    }

    if (idx_best > 0) {
        mgr->cache->ent[idx_best] = mgr->cache->ent[0];
        mgr->cache->ent[0] = ent_best;
    }
}

static void xShadow_PickEntForNPC(xShadowMgr* mgr)
{
    if (mgr->ent->baseType != eBaseTypeNPC) return;
    if (mgr->cache->entCount < 2) {
        return;
    }

    xShadow_PickByRayCast(mgr);
}

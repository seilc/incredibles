#include "iCamera.h"

#include "iAsync.h"
#include "iScrFX.h"
#include "iMemMgr.h"
#include "xGlobals.h"
#include "xShadow.h"

#include <string.h>

// Taken from baworobj.c
typedef struct RpWorldCameraExt RpWorldCameraExt;
struct RpWorldCameraExt
{
    RpWorldSector** frustumSectors;
    RwInt32 space;
    RwInt32 position;
    RpWorld* world;
    RwCameraBeginUpdateFunc oldBeginUpdate;
    RwCameraEndUpdateFunc oldEndUpdate;
    RwObjectHasFrameSyncFunction oldSync;
};
#define rwCAMERADISPLAYSECTORSINCREASE 50
#define WORLDCAMERAEXTFROMCAMERA(camera) ((RpWorldCameraExt *)(((RwUInt8 *)(camera)) + cameraExtOffset))

F32 sCameraNearClip = 0.2f;
F32 sCameraFarClip = 400.0f;
RwCamera* globalCamera = NULL;
static RwCamera* sMainGameCamera = NULL;

void iCameraSetOrthoProjection(RwCamera* cam, bool ortho)
{
    RwCamera* camera = RwCameraGetCurrentCamera();

    if (cam == camera) {
        RwCameraEndUpdate(camera);
    }

    RwCameraSetProjection(cam, ortho ? rwPARALLEL : rwPERSPECTIVE);

    if (cam == camera) {
        RwCameraBeginUpdateWrapper(camera);
    }
}

RwCamera* iCameraCreate(S32 width, S32 height, S32 mainGameCamera)
{
    iAsyncLockRWSentry rw_sentry;
    RwV2d vw;
    RwCamera* camera;
    RwFrame* cameraFrame;
    RwRaster* cameraRaster, *zbuffer;

    camera = RwCameraCreate();

    cameraFrame = RwFrameCreate();
    RwCameraSetFrame(camera, cameraFrame);

    cameraRaster = RwRasterCreate(width, height, 0, rwRASTERTYPECAMERA);
    RwCameraSetRaster(camera, cameraRaster);

    zbuffer = RwRasterCreate(width, height, 0, rwRASTERTYPEZBUFFER);
    RwCameraSetZRaster(camera, zbuffer);

    RwCameraSetFarClipPlane(camera, sCameraFarClip);
    RwCameraSetNearClipPlane(camera, sCameraNearClip);

    vw.x = 1.0f;
    vw.y = 1.0f;
    RwCameraSetViewWindow(camera, &vw);

    if (mainGameCamera) {
        iScrFxCameraCreated(camera);
        sMainGameCamera = camera;
    }

    return camera;
}

void iCameraDestroy(RwCamera* camera)
{
    iAsyncLockRWSentry rw_sentry;
    RpWorld* pWorld;

    iMemMgrEnableRenderWareMemoryHack(true);
    _rwFrameSyncDirty();
    iMemMgrEnableRenderWareMemoryHack(false);

    pWorld = RwCameraGetWorld(camera);
    if (pWorld) {
        RpWorldRemoveCamera(pWorld, camera);
    }

    if (camera == sMainGameCamera) {
        iScrFxCameraDestroyed(camera);
        sMainGameCamera = 0;
    }

    if (camera) {
        RwRaster* raster;

        RwFrame* frame = RwCameraGetFrame(camera);
        if (frame) {
            RwCameraSetFrame(camera, NULL);
            RwFrameDestroy(frame);
        }

        raster = RwCameraGetRaster(camera);
        if (raster) {
            RwRasterDestroy(raster);
            RwCameraSetRaster(camera, NULL);
        }

        raster = RwCameraGetZRaster(camera);
        if (raster) {
            RwRasterDestroy(raster);
            RwCameraSetZRaster(camera, NULL);
        }

        RwCameraDestroy(camera);
    }
}

void iCameraBegin(RwCamera* cam, S32 clear)
{
    if (clear) {
        if (xglobals->fog.type != rwFOGTYPENAFOGTYPE) {
            RwCameraClear(cam, &xglobals->fog.bgcolor, rwCAMERACLEARIMAGE | rwCAMERACLEARZ | rwCAMERACLEARSTENCIL);
        } else
            RwCameraClear(cam, NULL, rwCAMERACLEARZ);
    }

    RwCameraSetNearClipPlane(cam, sCameraNearClip);
    RwCameraBeginUpdateWrapper(cam);
}

void iCameraEnd(RwCamera* cam)
{
    iScrFxCameraEndScene(cam);
    RwCameraEndUpdate(cam);
}

void iCameraShowRaster(RwCamera* cam)
{
    RwCameraShowRaster(cam, NULL, 0);
}

void iCameraUpdatePos(RwCamera* cam, xMat4x3* pos)
{
    RwFrame* f;

    f = RwCameraGetFrame(cam);

    RwMatrix* m = RwFrameGetMatrix(f);
    xMat4x3Copy((xMat4x3*)m, pos);

    m = RwFrameGetLTM(f);
    xMat4x3Copy((xMat4x3*)m, pos);
    
    RwFrameOrthoNormalize(f);
    RwFrameUpdateObjects(f);
}

F32 iCameraGetAspectRatio()
{
    return 3/4.f;
}

void iCameraSetFOV(RwCamera* cam, F32 fov)
{
    F32 view_window = itan(0.5f * fov * (PI/180.0f));
    F32 fAspectRatio = iCameraGetAspectRatio();
    RwV2d vw;

    vw.x = view_window;
    vw.y = view_window * fAspectRatio;
    RwCameraSetViewWindow(cam, &vw);
}

void iCameraAssignEnv(RwCamera* camera, iEnv* env_geom)
{
    iAsyncLockRWSentry rw_sentry;

    globalCamera = camera;

    RpWorldAddCamera(env_geom->world, camera);
    xShadowSetWorld(env_geom->world);
}

void iCamGetViewMatrix(RwCamera* camera, xMat4x3* view_matrix)
{
    RwMatrix* rw_view = RwCameraGetViewMatrix(camera);

    memset(view_matrix, 0, sizeof(xMat4x3));

    view_matrix->right.x = rw_view->right.x;
    view_matrix->right.y = rw_view->right.y;
    view_matrix->right.z = rw_view->right.z;
    view_matrix->up.x = rw_view->up.x;
    view_matrix->up.y = rw_view->up.y;
    view_matrix->up.z = rw_view->up.z;
    view_matrix->at.x = rw_view->at.x;
    view_matrix->at.y = rw_view->at.y;
    view_matrix->at.z = rw_view->at.z;
    view_matrix->pos.x = rw_view->pos.x;
    view_matrix->pos.y = rw_view->pos.y;
    view_matrix->pos.z = rw_view->pos.z;
}

void iCameraSetNearFarClip(F32 nearPlane, F32 farPlane)
{
    sCameraNearClip = (nearPlane <= 0.0f) ? 0.2f : nearPlane;
    sCameraFarClip = (farPlane <= 0.0f) ? 400.0f : farPlane;
}

void iCameraSetFogParams(iFogParams* fp, F32 time)
{
    if (!fp || fp->type == rwFOGTYPENAFOGTYPE) {
        xglobals->fog.type = rwFOGTYPENAFOGTYPE;
        xglobals->fogA.type = rwFOGTYPENAFOGTYPE;
    }
    else if (time == 0.0f || fp->type != xglobals->fogA.type) {
        xglobals->fog = *fp;
        xglobals->fogA = *fp;
        xglobals->fog_t0 = 0;
    } else {
        xglobals->fogA = xglobals->fog;
        xglobals->fogB = *fp;
        xglobals->fog_t0 = iTimeGet();
        xglobals->fog_t1 = xglobals->fog_t0 + iTimeFromSec(time);
    }
}

void iCameraUpdateFog(RwCamera*, iTime t)
{
    iFogParams* fp = &xglobals->fog, *fa = &xglobals->fogA, *fb = &xglobals->fogB;
    F32 a;
    RwRGBA ca, cb, color;

    if (xglobals->fog.type == rwFOGTYPENAFOGTYPE) return;
    if (xglobals->fog_t0 == 0) return;

    t = iTimeGet();

    a = iTimeDiffSec(xglobals->fog_t0, t) / iTimeDiffSec(xglobals->fog_t0, xglobals->fog_t1);
    a = CLAMP(a, 0.0f, 1.0f);

    fp->type = xglobals->fogB.type;
    fp->table = xglobals->fogB.table;
    fp->start = fa->start + a * (fb->start - fa->start);
    fp->stop = fa->stop + a * (fb->stop - fa->stop);
    fp->density = fa->density + a * (fb->density - fa->density);

    ca = fa->fogcolor;
    cb = fb->fogcolor;
    color.red = ca.red + a * (cb.red - ca.red);
    color.green = ca.green + a * (cb.green - ca.green);
    color.blue = ca.blue + a * (cb.blue - ca.blue);
    color.alpha = ca.alpha + a * (cb.alpha - ca.alpha);
    fp->fogcolor = color;

    ca = fa->bgcolor;
    cb = fb->bgcolor;
    color.red = ca.red + a * (cb.red - ca.red);
    color.green = ca.green + a * (cb.green - ca.green);
    color.blue = ca.blue + a * (cb.blue - ca.blue);
    color.alpha = ca.alpha + a * (cb.alpha - ca.alpha);
    fp->bgcolor = color;

    if (a == 1.0f) {
        xglobals->fog_t0 = 0;
        xglobals->fogA = xglobals->fogB;
    }
}

void iCameraSetFogRenderStates()
{
    RwCamera* pCamera;
    
    pCamera = RwCameraGetCurrentCamera();
    if (!pCamera) return;

    iFogParams* pFogParams = &xglobals->fog;
    if (pFogParams->type == rwFOGTYPENAFOGTYPE) {
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
        RwCameraSetFarClipPlane(pCamera, sCameraFarClip);
    } else if (pFogParams->start >= pFogParams->stop) {
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
        RwCameraSetFarClipPlane(pCamera, pFogParams->stop);
    } else {
        RwUInt32 bite_me = RWRGBALONG(pFogParams->fogcolor.red,
                                      pFogParams->fogcolor.green,
                                      pFogParams->fogcolor.blue,
                                      pFogParams->fogcolor.alpha);

        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)TRUE);
        RwRenderStateSet(rwRENDERSTATEFOGTYPE, (void*)pFogParams->type);
        RwRenderStateSet(rwRENDERSTATEFOGCOLOR, (void*)bite_me);
        RwRenderStateSet(rwRENDERSTATEFOGDENSITY, (void*)&pFogParams->density);
        RwCameraSetFogDistance(pCamera, pFogParams->start);
        RwCameraSetFarClipPlane(pCamera, pFogParams->stop);
    }
}

void iCameraSetFog(bool enable)
{
    iFogParams* pFogParams = &xglobals->fog;
    if (pFogParams->type == rwFOGTYPENAFOGTYPE || pFogParams->start >= pFogParams->stop) {
        return;
    }

    if (enable) {
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)TRUE);
    } else {
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    }
}

RwCamera* RwCameraBeginUpdateWrapper(RwCamera* camera)
{
    RwCamera* retval = RwCameraBeginUpdate(camera);
    return retval;
}

void UpdateGlobalFrustumPlanes(RwCamera*)
{
}

void iCameraPerformRenderWareHack(RwCamera* camera)
{
    RwUInt32 memSize;
    RpWorldSector** newFrustumSectors;
    RwInt32 cameraExtOffset;
    RpWorldCameraExt* cameraExt;

    memSize = rwCAMERADISPLAYSECTORSINCREASE * sizeof(RpWorldSector*);
    newFrustumSectors = (RpWorldSector**)RwMalloc(memSize, rwMEMHINTDUR_NADURATION);
    cameraExtOffset = RwCameraGetPluginOffset(rwID_WORLDOBJMODULE);
    cameraExt = WORLDCAMERAEXTFROMCAMERA(camera);

    cameraExt->frustumSectors = newFrustumSectors;
    cameraExt->space = rwCAMERADISPLAYSECTORSINCREASE;
}

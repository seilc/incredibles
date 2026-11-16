#include "xCommon.h"
#include "iScrFX.h"

#include "xMath.h"
#include "xScrFx.h"
#include "iCamera.h"

#include "decomp.h"

typedef struct _iMotionBlurData
{
    S32 motionBlurAlpha;
    RwRaster* motionBlurFrontBuffer;
    RwIm2DVertex vertex[4];
    RwImVertexIndex index[6];
    U32 w;
    U32 h;
} iMotionBlurData;

void iScrFxInit()
{
}

DECOMP_FORCEBLOCK((S32 i) {
    *(F32*)0 = 0.0f;
    *(F32*)0 = i;
})

void iScrFxBegin()
{
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
}

void iScrFxEnd()
{
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
}

static U32 sMotionBlurEnabled;
static iMotionBlurData sMBD = { 0 };

void iCameraSetBlurriness(F32 amount)
{
    if (amount <= 0.0f) {
        sMotionBlurEnabled = FALSE;
    } else {
        if (amount > 1.0f) {
            amount = 1.0f;
        }
        sMotionBlurEnabled = TRUE;
        sMBD.motionBlurAlpha = 254.0f * amount + 0.5f;
    }
}

void iScrFxCameraCreated(RwCamera* pCamera)
{
    sMBD.motionBlurAlpha = 144;
    sMBD.motionBlurFrontBuffer = NULL;

    for (U32 i = 0, v = 0; i < 6; i += 6, v += 4) {
        sMBD.index[i + 0] = v;
        sMBD.index[i + 1] = v + 1;
        sMBD.index[i + 2] = v + 2;
        sMBD.index[i + 3] = v;
        sMBD.index[i + 4] = v + 2;
        sMBD.index[i + 5] = v + 3;
    }

    iScrFxMotionBlurOpen(pCamera);
}

void iScrFxCameraEndScene(RwCamera* pCamera)
{
    if (sMotionBlurEnabled && sMBD.motionBlurAlpha) {
        iScrFxMotionBlurRender(pCamera, (U8)sMBD.motionBlurAlpha);
    }
}

static void iScrFxMotionBlurCreateImmediateModeData(RwCamera* camera, RwRect* rect); // UNUSED

DECOMP_FORCEBLOCK((U32 u) {
    *(F32*)0 = u;
})

static void iCameraOverlayRender(RwCamera*, RwRaster* ras, RwRGBA col)
{
    for (U32 i = 0; i < 4; i++) {
        RwIm2DVertexSetIntRGBA(&sMBD.vertex[i], col.red, col.green, col.blue, col.alpha);
    }

    RwRect rect = { 0, 0, 0, 0 };

    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, ras);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

    RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, sMBD.vertex, 4, sMBD.index, 6);

    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
}

S32 iScrFxMotionBlurOpen(RwCamera*)
{
    return 0;
}

S32 iScrFxCameraDestroyed(RwCamera*)
{
    if (sMBD.motionBlurFrontBuffer) {
        RwRasterDestroy(sMBD.motionBlurFrontBuffer);
        sMBD.motionBlurFrontBuffer = NULL;
        return 1;
    }

    return 0;
}

void iScrFxMotionBlurRender(RwCamera* camera, U32 alpha)
{
    if (sMBD.motionBlurFrontBuffer) {
        RwRGBA color = { 255, 255, 255, alpha };
        iCameraOverlayRender(camera, sMBD.motionBlurFrontBuffer, color);
    }
}

static void RenderRect(RwRect* srcRect, F32 cameraZ)
{
    RwIm2DVertex verts[4];
    RwImVertexIndex indices[4];
    RwCamera* camera = RwCameraGetCurrentCamera();
    F32 recipCamZ = 1.0f / cameraZ, screenZ;

    if (RwCameraGetProjection(camera) == rwPERSPECTIVE) {
        screenZ = recipCamZ * camera->zScale + camera->zShift;
    } else {
        screenZ = cameraZ * camera->zScale + camera->zShift;
    }

    RwIm2DVertexSetScreenX(&verts[0], srcRect->x);
    RwIm2DVertexSetScreenY(&verts[0], srcRect->y);
    RwIm2DVertexSetScreenZ(&verts[0], screenZ);
    RwIm2DVertexSetRecipCameraZ(&verts[0], recipCamZ);

    RwIm2DVertexSetScreenX(&verts[1], srcRect->x + srcRect->w);
    RwIm2DVertexSetScreenY(&verts[1], srcRect->y);
    RwIm2DVertexSetScreenZ(&verts[1], screenZ);
    RwIm2DVertexSetRecipCameraZ(&verts[1], recipCamZ);

    RwIm2DVertexSetScreenX(&verts[2], srcRect->x + srcRect->w);
    RwIm2DVertexSetScreenY(&verts[2], srcRect->y + srcRect->h);
    RwIm2DVertexSetScreenZ(&verts[2], screenZ);
    RwIm2DVertexSetRecipCameraZ(&verts[2], recipCamZ);

    RwIm2DVertexSetScreenX(&verts[3], srcRect->x);
    RwIm2DVertexSetScreenY(&verts[3], srcRect->y + srcRect->h);
    RwIm2DVertexSetScreenZ(&verts[3], screenZ);
    RwIm2DVertexSetRecipCameraZ(&verts[3], recipCamZ);

    RwIm2DVertexSetIntRGBA(&verts[0], 255, 255, 255, 255);
    RwIm2DVertexSetIntRGBA(&verts[1], 255, 255, 255, 255);
    RwIm2DVertexSetIntRGBA(&verts[2], 255, 255, 255, 255);
    RwIm2DVertexSetIntRGBA(&verts[3], 255, 255, 255, 255);

    indices[0] = 1;
    indices[1] = 0;
    indices[2] = 2;
    indices[3] = 3;

    RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRISTRIP, verts, 4, indices, 4);
}

U32 FLARE_SIZE = 64;

static S32 DetermineFlareOcclusion(RwCamera* camera, RwTexture*, RwFrame* frame)
{
    RwRect srcRect;
    S32 size = FLARE_SIZE;
    RwVideoMode vidInfo;
    RwV3d FlarePositionPixel;
    RwMatrix invMtx, tmp;
    RwV3d FlarePosition;
    S32 calcEdgeFactor = FALSE;
    S32 edgeFactor = 255;
    S32 area = 0;
    RwMatrix* matrix = RwFrameGetLTM(RwCameraGetFrame(camera));
    const RwV2d* vw = RwCameraGetViewWindow(camera);

    RwMatrixInvert(&invMtx, matrix);
    RwMatrixMultiply(&tmp, RwFrameGetLTM(frame), &invMtx);
    RwV3dAssign(&FlarePosition, RwMatrixGetPos(&tmp));

    if (FlarePosition.z <= 0.0f) {
        return 0;
    }

    RwV3dScale(&FlarePositionPixel, &FlarePosition, 1.0f / FlarePosition.z);

    RwEngineGetVideoModeInfo(&vidInfo, RwEngineGetCurrentVideoMode());

    FlarePositionPixel.x = vidInfo.width * (vw->x - FlarePositionPixel.x) / (2.0f * vw->x);
    FlarePositionPixel.y = vidInfo.height * (vw->y - FlarePositionPixel.y) / (2.0f * vw->y);

    srcRect.w = size;
    srcRect.x = FlarePositionPixel.x - srcRect.w / 2;
    srcRect.h = size * ((F32)vw->x / (F32)vw->y) * ((F32)vidInfo.height / (F32)vidInfo.width);
    srcRect.y = FlarePositionPixel.y - srcRect.h / 2;

    area = srcRect.w * srcRect.h;

    if (srcRect.x + srcRect.w > vidInfo.width) {
        srcRect.w -= srcRect.x + srcRect.w - vidInfo.width;
        srcRect.x = vidInfo.width - srcRect.w;
        calcEdgeFactor = TRUE;
    }
    else if (srcRect.x < 0) {
        area = srcRect.w * srcRect.h;
        srcRect.w -= -srcRect.x;
        srcRect.x = 0;
        calcEdgeFactor = TRUE;
    }

    if (srcRect.y + srcRect.h > vidInfo.height) {
        area = srcRect.w * srcRect.h;
        srcRect.h -= srcRect.y + srcRect.h - vidInfo.height;
        srcRect.y = vidInfo.height - srcRect.h;
        calcEdgeFactor = TRUE;
    }
    else if (srcRect.y < 0) {
        area = srcRect.w * srcRect.h;
        srcRect.h -= -srcRect.y;
        srcRect.y = 0;
        calcEdgeFactor = TRUE;
    }

    if (srcRect.w <= 0 || srcRect.h <= 0) {
        return 0;
    }

    if (calcEdgeFactor) {
        edgeFactor = 255.0f * (srcRect.w * srcRect.h) / area;
    }

    RwCameraBeginUpdateWrapper(camera);

    u32 top_pixels_in, top_pixels_out, bot_pixels_in, bot_pixels_out, clr_pixels_in, copy_clks;

    GXClearPixMetric();
    _rwDlTokenSendNew();

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDZERO);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);

    RenderRect(&srcRect, FlarePosition.z);

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

    U32 token = _rwDlTokenGetCurrent();
    _rwDlTokenSendNew();
    _rwDlTokenWaitDone(token);

    GXReadPixMetric(&top_pixels_in, &top_pixels_out, &bot_pixels_in, &bot_pixels_out, &clr_pixels_in, &copy_clks);

    RwCameraEndUpdate(camera);

    S32 result = MAX(top_pixels_in, bot_pixels_out) * 255 / (F32)area;

    return MIN(255, result);
}

void iScrFxLensFlare_Reset()
{
}

void iScrFxLensFlare_Render(RwCamera* camera, iScrFxLensFlare* flare, void (*renderCB)(RwRaster*, RwFrame*, U8, void*))
{
    if (!camera || !flare) {
        return;
    }

    if (!gxFlareTexture) {
        return;
    }

    RwFrame* frame = gxFlareFrame;
    RwFrameTranslate(frame, (RwV3d*)&flare->pos, rwCOMBINEREPLACE);
    RwFrameUpdateObjects(frame);

    RwCameraEndUpdate(camera);

    S32 result = DetermineFlareOcclusion(camera, gxFlareTexture, frame);
    if (result > 0) {
        if (RwCameraBeginUpdateWrapper(camera)) {
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
            renderCB(RwTextureGetRaster(gxFlareTexture), frame, result, flare->source);
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            RwCameraEndUpdate(camera);
        }
    }

    RwCameraBeginUpdateWrapper(camera);
}

#include "xScrFx.h"

#include "xEnt.h"
#include "xGroup.h"
#include "xIM.h"
#include "xMemMgr.h"
#include "xRand.h"
#include "xScrPulseGlare.h"
#include "xString.h"
#include "xstransvc.h"
#include "xutil.h"
#include "iAsync.h"
#include "iCamera.h"
#include "iDebug.h"
#include "iMath.h"
#include "iScrFX.h"
#include "zBulletTime.h"
#include "zCam.h"
#include "zGame.h"
#include "zGlobals.h"
#include "zMenu.h"
#include "zOtherAsset.h"
#include "zScene.h"

#include <float.h>
#include <string.h>

#define MAX_GLARES 10
#define MAX_CAMERA_FX 10
#define MAX_LENS_FLARES 128
#define NUM_QVERTS 9
#define LENS_FLARE_SOURCE_VERSION 3

#define CAMERAFX_ACTIVE 0x1
#define CAMERAFX_KILL 0x2

#define EFFECT_LENS_FLARE_SOURCE_ID 0xA072A4DA // xStrHash("effect:Lens Flare Source")
#define EFFECT_LENS_FLARE_PARAMS_ID 0xB7BA72D5 // xStrHash("effect:Lens Flare Params")
#define FX_STREAK2_ID 0x9F681A2B // xStrHash("fx_streak2")
#define LENSFLARE01_ID 0x8D5DBC59 // xStrHash("LensFlare01")

bool g_debugRenderSafeArea;

static void xCameraFXInit();
static void xScrFxFadeUpdate(F32 seconds);
static void xScrFxFadeRender();

void xScrFxInit()
{
    xScrFxFadeInit();
    xScrFXGlareInit();
    iScrFxInit();
    xCameraFXInit();
}

void xScrFxReset()
{
    xScrFXGlareReset();
}

void xScrFxUpdate(RwCamera*, F32 dt)
{
    iScrFxBegin();
    xScrFxFadeUpdate(dt);
    xScrFXGlareUpdate(dt);
    xScrFX_PulseGlaresUpdate(dt);
}

void xScrFxRender(RwCamera*)
{
    iScrFxBegin();

    xScrFxFadeRender();

    if (g_debugRenderSafeArea) {
        xScrFxDrawSafeArea();
    }

    iScrFxEnd();
}

void xScrFxDrawScreenSizeRectangle()
{
    RwVideoMode video_mode;
    RwEngineGetVideoModeInfo(&video_mode, RwEngineGetCurrentVideoMode());

    xScrFxDrawBox(0.0f, 0.0f, video_mode.width, video_mode.height, 0, 0, 0, 255, false);
}

void xScrFxDrawBox(F32 x1, F32 y1, F32 x2, F32 y2, U8 red, U8 green, U8 blue, U8 alpha, bool clear_z_to_far)
{
    RwIm2DVertex v[4];
    RwImVertexIndex indices[4] = { 0, 1, 2, 3 };
    F32 fRecipZ = 5.0f;

    RwIm2DVertexSetScreenX(&v[0], x1);
    RwIm2DVertexSetScreenY(&v[0], y1);
    RwIm2DVertexSetScreenX(&v[1], x2);
    RwIm2DVertexSetScreenY(&v[1], y1);
    RwIm2DVertexSetScreenX(&v[2], x1);
    RwIm2DVertexSetScreenY(&v[2], y2);
    RwIm2DVertexSetScreenX(&v[3], x2);
    RwIm2DVertexSetScreenY(&v[3], y2);

    RwIm2DVertexSetIntRGBA(&v[0], red, green, blue, alpha);
    RwIm2DVertexSetIntRGBA(&v[1], red, green, blue, alpha);
    RwIm2DVertexSetIntRGBA(&v[2], red, green, blue, alpha);
    RwIm2DVertexSetIntRGBA(&v[3], red, green, blue, alpha);

    RwIm2DVertexSetU(&v[0], 0.0f, fRecipZ);
    RwIm2DVertexSetV(&v[0], 0.0f, fRecipZ);
    RwIm2DVertexSetU(&v[1], 1.0f, fRecipZ);
    RwIm2DVertexSetV(&v[1], 0.0f, fRecipZ);
    RwIm2DVertexSetU(&v[2], 0.0f, fRecipZ);
    RwIm2DVertexSetV(&v[2], 1.0f, fRecipZ);
    RwIm2DVertexSetU(&v[3], 1.0f, fRecipZ);
    RwIm2DVertexSetV(&v[3], 1.0f, fRecipZ);

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);

    if (clear_z_to_far == true) {
        RwReal farz = RwIm2DGetFarScreenZ();

        RwIm2DVertexSetScreenZ(&v[0], farz);
        RwIm2DVertexSetScreenZ(&v[1], farz);
        RwIm2DVertexSetScreenZ(&v[2], farz);
        RwIm2DVertexSetScreenZ(&v[3], farz);

        S32 old_z_write;
        RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &old_z_write);

        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);

        RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRISTRIP, v, ARRAY_LENGTH(v), indices, ARRAY_LENGTH(indices));

        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)old_z_write);
    } else {
        RwReal nearz = RwIm2DGetNearScreenZ();

        RwIm2DVertexSetScreenZ(&v[0], nearz);
        RwIm2DVertexSetScreenZ(&v[1], nearz);
        RwIm2DVertexSetScreenZ(&v[2], nearz);
        RwIm2DVertexSetScreenZ(&v[3], nearz);

        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);

        RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRISTRIP, v, ARRAY_LENGTH(v), indices, ARRAY_LENGTH(indices));
    }
}

typedef struct _xFadeData
{
    bool active;
    bool hold_at_dest;
    xColor src;
    xColor dest;
    F32 time_passed;
    F32 time_total;
    xColor current_color;
} xFadeData;

static xFadeData mFade;

void xScrFxFadeInit()
{
    memset(&mFade, 0, sizeof(mFade));
}

static U8 InterpCol(F32 t, U8 s, U8 d)
{
    U8 uval;

    F32 val = (F32)(d - s);
    val *= t;
    val += (F32)s;

    uval = (U8)val;
    return uval;
}

void xScrFxFade(const xColor* base, const xColor* dest, F32 seconds, S32 hold)
{
    if (seconds < 0.0f) seconds = 0.0f;

    mFade.active = true;
    mFade.src = *base;
    mFade.dest = *dest;

    mFade.time_passed = 0.0f;
    mFade.time_total = seconds;
    mFade.hold_at_dest = (hold != 0);
}

void xScrFxStopFade()
{
    mFade.active = false;
}

static void xScrFxFadeUpdate(F32 seconds)
{
    if (zGameIsPaused()) {
        return;
    }

    if (!mFade.active) {
        return;
    }

    mFade.time_passed += seconds;

    if (mFade.time_passed >= mFade.time_total) {
        if (!mFade.hold_at_dest) {
            mFade.active = false;
            return;
        }

        mFade.current_color = mFade.dest;
        return;
    }

    F32 percent = mFade.time_passed / mFade.time_total;

    mFade.current_color.r = InterpCol(percent, mFade.src.r, mFade.dest.r);
    mFade.current_color.g = InterpCol(percent, mFade.src.g, mFade.dest.g);
    mFade.current_color.b = InterpCol(percent, mFade.src.b, mFade.dest.b);
    mFade.current_color.a = InterpCol(percent, mFade.src.a, mFade.dest.a);
}

static void xScrFxFadeRender()
{
    if (mFade.active) {
        RwVideoMode video_mode;
        RwEngineGetVideoModeInfo(&video_mode, RwEngineGetCurrentVideoMode());

        xScrFxDrawBox(0.0f, 0.0f, video_mode.width, video_mode.height, mFade.current_color.r, mFade.current_color.g, mFade.current_color.b, mFade.current_color.a, true);
    }
}

static F32 mLetterboxO;
static F32 mLetterboxTO;
static F32 sLetterBoxSize;
static U8 sLetterBoxAlpha = 255;

void xScrFxLetterboxReset()
{
    mLetterboxO = 0.0f;
    mLetterboxTO = 0.0f;
}

void xScrFxLetterBoxSetSize(F32 size)
{
    F32 fNormalValue = size / 480.0f;

    sLetterBoxSize = fNormalValue * FB_YRES;
}

F32 xScrFxLetterBoxGetSizeInScreenPercent()
{
    return sLetterBoxSize / FB_YRES;
}

void xScrFxLetterBoxSetAlpha(U8 alpha)
{
    sLetterBoxAlpha = alpha;
}

void xScrFxLetterbox(S32 enable)
{
    if (zMenuRunning()) {
        enable = FALSE;
    }

    if (enable) {
        mLetterboxTO = sLetterBoxSize;

        if (xglobals->beforeFirstFrame) {
            mLetterboxO = mLetterboxTO;
        }
    } else {
        mLetterboxTO = 0.0f;
    }
}

void xScrFxRenderLetterBox(RwCamera*)
{
    if (!xglobals->sceneCur->enableDrawing) {
        return;
    }

    if (mLetterboxO < mLetterboxTO) {
        mLetterboxO += 100.0f * xglobals->update_dt;
        if (mLetterboxO > mLetterboxTO)
            mLetterboxO = mLetterboxTO;
    }
    else if (mLetterboxO > mLetterboxTO) {
        mLetterboxO -= 100.0f * xglobals->update_dt;
        if (mLetterboxO < mLetterboxTO) {
            mLetterboxO = mLetterboxTO;
        }
    }

    if (mLetterboxO > 0.0f) {
        F32 y_offset = mLetterboxO;

        RwVideoMode video_mode;
        RwEngineGetVideoModeInfo(&video_mode, RwEngineGetCurrentVideoMode());

        xScrFxDrawBox(0.0f, 0.0f, video_mode.width, y_offset, 0, 0, 0, sLetterBoxAlpha, false);

        xScrFxDrawBox(0.0f, video_mode.height - y_offset, video_mode.width, video_mode.height, 0, 0, 0, sLetterBoxAlpha, false);
    }
}

void xScrFxDrawSafeArea()
{
    RwVideoMode videoMode;
    RwEngineGetVideoModeInfo(&videoMode, RwEngineGetCurrentVideoMode());

    bool bHiDef = false;
    if (videoMode.flags & (rwVIDEOMODE_XBOX_WIDESCREEN | rwVIDEOMODE_XBOX_PROGRESSIVE)) {
        bHiDef = true;
    }

    F32 fSafeArea = 0.85f;
    if (bHiDef) {
        fSafeArea = 0.9f;
    }

    U32 screenWidth = videoMode.width;
    U32 screenHeight = videoMode.height;
    U32 safeX = screenWidth * fSafeArea;
    U32 safeY = screenHeight * fSafeArea;
    U32 xMargin = (screenWidth - safeX) / 2;
    U32 yMargin = (screenHeight - safeY) / 2;

    S32 top = yMargin;
    S32 bottom = screenHeight - yMargin;
    S32 left = xMargin;
    S32 right = screenWidth - xMargin;
    S32 width = 2;

    xScrFxDrawBox((F32)left - width, (F32)top - width, (F32)right + width, top, 255, 0, 0, 255, false);
    xScrFxDrawBox((F32)left - width, bottom, (F32)right + width, (F32)bottom + width, 255, 0, 0, 255, false);
    xScrFxDrawBox((F32)left - width, top, left, bottom, 255, 0, 0, 255, false);
    xScrFxDrawBox(right, top, (F32)right + width, bottom, 255, 0, 0, 255, false);
}

struct xGlare
{
    S32 flags;
    xVec3 pos;
    F32 intensity;
    F32 intensityFadeRate;
    F32 lifetime;
    F32 size;
    RwRGBAReal col;
    RwRaster* raster;
};

xGlare sGlare[MAX_GLARES];

void xScrFXGlareInit()
{
    for (S32 i = 0; i < MAX_GLARES; i++)
        sGlare[i].flags = 0;
}

void xScrFXGlareReset()
{
    xScrFXGlareInit();
}

void xScrFXGlareDisable(S32 id)
{
    sGlare[id].flags = 0;
}

S32 xScrFXGlareAdd(const xVec3* pos, F32 life, F32 intensity, F32 size, F32 r, F32 g, F32 b, F32 a, RwRaster* raster)
{
    for (S32 i = 0; i < MAX_GLARES; i++) {
        if (sGlare[i].flags == 0) {
            sGlare[i].lifetime = life;
            sGlare[i].intensity = intensity;
            sGlare[i].pos = *pos;
            sGlare[i].flags = 1;
            sGlare[i].col.alpha = a;
            sGlare[i].col.red = r;
            sGlare[i].col.green = g;
            sGlare[i].col.blue = b;
            sGlare[i].size = size;
            sGlare[i].intensityFadeRate = sGlare[i].intensity / sGlare[i].lifetime;

            if (!raster) {
                RwTexture* texturePtr = (RwTexture*)xSTFindAsset(LENSFLARE01_ID, NULL);
                if (texturePtr) {
                    sGlare[i].raster = RwTextureGetRaster(texturePtr);
                } else {
                    sGlare[i].raster = NULL;
                }
            } else {
                sGlare[i].raster = raster;
            }

            return i;
        }
    }

    return -1;
}

void xScrFXGlareChange(S32 id, const xVec3* pos, F32 life, F32 intensity)
{
    sGlare[id].pos = *pos;
    if (life >= 0.0f)
        sGlare[id].lifetime = life;
    if (intensity >= 0.0f)
        sGlare[id].intensity = intensity;
}

void xScrFXGlareUpdate(F32 dt)
{
    for (S32 i = 0; i < MAX_GLARES; i++) {
        if (sGlare[i].flags) {
            sGlare[i].lifetime -= dt;
            sGlare[i].intensity -= dt * sGlare[i].intensityFadeRate;
            if (sGlare[i].lifetime <= 0.0f) {
                sGlare[i].flags = 0;
            }
        }
    }
}

xVec3 sFullScreenGlareDir = { -0.3f, 1.0f, 0.4f };
F32 sFullScreenGlareIntensity = 0.5f;
RwRGBA sFullScreenGlareColor = { 255, 255, 255, 64 };
S32 sFullScreenGlareEnabled;
U32 sFullScreenGlareTextureID = FX_STREAK2_ID;
RwTexture* sFullScreenGlareTexturePtr;

void xScrFXFullScreenGlareRender()
{
    if (sFullScreenGlareEnabled) {
        xMat4x3 mat;
        iCamGetViewMatrix(xglobals->screen->icam, &mat);

        xVec3 v;
        xVec3Copy(&v, &mat.at);
        xVec3Normalize(&v, &v);

        xVec3 glareDir;
        xVec3Normalize(&glareDir, &sFullScreenGlareDir);
        F32 dp = glareDir.dot(v);

        F32 d = dp;
        d = d + 0.5f;
        d = d * (d * xsqr(d));
        d *= sFullScreenGlareIntensity;

        if (d != 0.0f) {
            RwRGBA color;
            color.red = sFullScreenGlareColor.red;
            color.green = sFullScreenGlareColor.green;
            color.blue = sFullScreenGlareColor.blue;

            F32 alpha = sFullScreenGlareColor.alpha * d;
            if (alpha > 255.0f) {
                alpha = 255.0f;
            }
            color.alpha = alpha;

            if (!sFullScreenGlareTexturePtr)
                sFullScreenGlareTexturePtr = (RwTexture*)xSTFindAsset(sFullScreenGlareTextureID, NULL);
            if (sFullScreenGlareTexturePtr) {
                RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(sFullScreenGlareTexturePtr));
            } else {
                RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
            }

            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

            xScrFxDrawBox(0.0f, 0.0f, FB_XRES, FB_YRES, color.red, color.green, color.blue, color.alpha, dp, 0.0f);
        }
    }
}

void xScrFXGlareRender(const xMat4x3& mat)
{
    for (S32 i = 0; i < MAX_GLARES; i++) {
        xGlare* g = &sGlare[i];

        if (g->flags) {
            xVec3 w, h;
            h = mat.right;
            w = mat.up;

            xVec3Normalize(&h, &h);
            xVec3Normalize(&w, &w);
            h *= 0.5f * g->size;
            w *= 0.5f * g->size;

            xVec3 v;
            xVec3Copy(&v, &mat.at);
            xVec3Normalize(&v, &v);

            xVec3 glareDir;
            glareDir.x = g->pos.x - mat.pos.x;
            glareDir.y = g->pos.y - mat.pos.y;
            glareDir.z = g->pos.z - mat.pos.z;
            xVec3Normalize(&glareDir, &glareDir);
            F32 dot = glareDir.dot(v);

            F32 val = (1.0f + dot) / 2.0f;
            val = val * (val * xsqr(val));

            F32 glareDirLen = xVec3Length(&glareDir);
            if (glareDirLen != 0.0f) {
                val *= g->intensity / glareDirLen;

                if (val != 0.0f) {
                    xVec3 toCam = xglobals->cam->mat.pos - g->pos;
                    toCam.normalize();
                    toCam *= 0.5f;
                    g->pos += toCam;

                    static RwIm3DVertex sStripVert[4];
                    RwIm3DVertex* vert = sStripVert;

                    RwIm3DVertexSetPos(vert, g->pos.x - w.x - h.x, g->pos.y - w.y - h.y, g->pos.z - w.z - h.z);
                    vert->u = 0.0f;
                    vert->v = 0.0f;
                    RwIm3DVertexSetRGBA(vert, g->col.red, g->col.green, g->col.blue, g->col.alpha * val);
                    vert++;

                    RwIm3DVertexSetPos(vert, g->pos.x - w.x + h.x, g->pos.y - w.y + h.y, g->pos.z - w.z + h.z);
                    vert->u = 0.0f;
                    vert->v = 1.0f;
                    RwIm3DVertexSetRGBA(vert, g->col.red, g->col.green, g->col.blue, g->col.alpha * val);
                    vert++;

                    RwIm3DVertexSetPos(vert, g->pos.x + w.x - h.x, g->pos.y + w.y - h.y, g->pos.z + w.z - h.z);
                    vert->u = 1.0f;
                    vert->v = 0.0f;
                    RwIm3DVertexSetRGBA(vert, g->col.red, g->col.green, g->col.blue, g->col.alpha * val);
                    vert++;

                    RwIm3DVertexSetPos(vert, g->pos.x + w.x + h.x, g->pos.y + w.y + h.y, g->pos.z + w.z + h.z);
                    vert->u = 1.0f;
                    vert->v = 1.0f;
                    RwIm3DVertexSetRGBA(vert, g->col.red, g->col.green, g->col.blue, g->col.alpha * val);
                    vert++;

                    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, g->raster);

                    xIMRenderLegacy(sStripVert, ARRAY_LENGTH(sStripVert), NULL, rwPRIMTYPETRISTRIP);
                }
            }
        }
    }
}

void xScrFxDrawBox(F32 x1, F32 y1, F32 x2, F32 y2, U8 red, U8 green, U8 blue, U8 alpha, F32 ushift, F32 vshift)
{
    static RwIm2DVertex v[4];
    static RwImVertexIndex indices[4] = { 0, 2, 1, 3 };

    RwReal oocameraNearClipPlane = RwIm2DGetNearScreenZ();

    if (!alpha) {
        return;
    }

    RwIm2DVertexSetScreenX(&v[0], x1);
    RwIm2DVertexSetScreenY(&v[0], y1);
    RwIm2DVertexSetScreenX(&v[1], x2);
    RwIm2DVertexSetScreenY(&v[1], y1);
    RwIm2DVertexSetScreenX(&v[2], x1);
    RwIm2DVertexSetScreenY(&v[2], y2);
    RwIm2DVertexSetScreenX(&v[3], x2);
    RwIm2DVertexSetScreenY(&v[3], y2);

    RwIm2DVertexSetScreenZ(&v[0], oocameraNearClipPlane);
    RwIm2DVertexSetScreenZ(&v[1], oocameraNearClipPlane);
    RwIm2DVertexSetScreenZ(&v[2], oocameraNearClipPlane);
    RwIm2DVertexSetScreenZ(&v[3], oocameraNearClipPlane);

    RwIm2DVertexSetIntRGBA(&v[0], red, green, blue, alpha);
    RwIm2DVertexSetIntRGBA(&v[1], red, green, blue, alpha);
    RwIm2DVertexSetIntRGBA(&v[2], red, green, blue, alpha);
    RwIm2DVertexSetIntRGBA(&v[3], red, green, blue, alpha);

    RwIm2DVertexSetU(&v[0], ushift, oocameraNearClipPlane);
    RwIm2DVertexSetV(&v[0], vshift, oocameraNearClipPlane);

    RwIm2DVertexSetU(&v[1], 1.0f + ushift, oocameraNearClipPlane);
    RwIm2DVertexSetV(&v[1], vshift, oocameraNearClipPlane);

    RwIm2DVertexSetU(&v[2], ushift, oocameraNearClipPlane);
    RwIm2DVertexSetV(&v[2], 1.0f + vshift, oocameraNearClipPlane);

    RwIm2DVertexSetU(&v[3], 1.0f + ushift, oocameraNearClipPlane);
    RwIm2DVertexSetV(&v[3], 1.0f + vshift, oocameraNearClipPlane);

    RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRISTRIP, v, ARRAY_LENGTH(v), indices, ARRAY_LENGTH(indices));
}

static xMat4x3 sCameraFXMatOld;

void xCameraFXBegin(const xMat4x3& mat)
{
    xMat4x3Identity(&sCameraFXMatOld);
    xMat4x3Copy(&sCameraFXMatOld, &mat);
}

enum
{
    eZoomIn,
    eZoomOut,
    eZoomHold,
    eZoomDone
};

enum
{
    eCameraFXNone,
    eCameraFXZoom,
    eCameraFXShake,
    eCameraFXCount
};

struct cameraFXTableEntry
{
    S32 type;
    void (*func)(cameraFX*, F32, const xMat4x3*, xMat4x3*);
    void (*funcKill)(cameraFX*);
};

namespace {
    bool anycameraFXActive;
}

bool xCameraFXAnyActive()
{
    return anycameraFXActive;
}

cameraFXTableEntry sCameraFXTable[eCameraFXCount] = {
    { eCameraFXNone, NULL, NULL },
    { eCameraFXZoom, xCameraFXZoomUpdate, NULL },
    { eCameraFXShake, xCameraFXShakeUpdate, NULL }
};

cameraFX sCameraFX[MAX_CAMERA_FX];

static void xCameraFXInit()
{
    memset(sCameraFX, 0, sizeof(sCameraFX));

    for (S32 i = 0; i < MAX_CAMERA_FX; i++) {
        sCameraFX[i].flags = 0;
    }
}

void xCameraFXKill()
{
    for (S32 i = 0; i < MAX_CAMERA_FX; i++) {
        if (sCameraFX[i].flags & CAMERAFX_ACTIVE)
            sCameraFX[i].flags |= CAMERAFX_KILL;
    }
}

cameraFX* xCameraFXAlloc()
{
    for (S32 i = 0; i < MAX_CAMERA_FX; i++) {
        cameraFX* f = &sCameraFX[i];

        if (f->flags == 0) {
            f->flags = CAMERAFX_ACTIVE;
            f->elapsedTime = 0.0f;
            f->maxTime = 0.0f;
            return f;
        }
    }
    return NULL;
}

void xCameraFXZoomUpdate(cameraFX* f, F32 dt, const xMat4x3*, xMat4x3* m)
{
    switch (f->zoom.mode) {
    case eZoomIn:
        f->zoom.velCur += f->zoom.accel * dt;
        f->zoom.distanceCur += f->zoom.velCur * dt;
        if (f->zoom.distanceCur >= f->zoom.distance) {
            f->zoom.distanceCur = f->zoom.distance;
            f->zoom.mode = eZoomHold;
            f->zoom.holdTimeCur = 0.0f;
        }
        xMat4x3MoveLocalAt(m, f->zoom.distanceCur);
        break;
    case eZoomHold:
        f->zoom.holdTimeCur += dt;
        if (f->zoom.holdTimeCur > f->zoom.holdTime) {
            f->zoom.mode = eZoomOut;
            f->zoom.distanceCur = f->zoom.distance;

            f->zoom.velCur = f->zoom.vel;
        }
        xMat4x3MoveLocalAt(m, f->zoom.distance);
        break;
    case eZoomOut:
        f->zoom.velCur += f->zoom.accel * dt;
        f->zoom.distanceCur -= f->zoom.velCur * dt;
        if (f->zoom.distanceCur <= 0.0f) {
            f->zoom.distanceCur = 0.0f;
            f->zoom.mode = eZoomDone;

            f->flags |= CAMERAFX_KILL;
        }
        xMat4x3MoveLocalAt(m, f->zoom.distanceCur);
        break;
    case eZoomDone:
        break;
    }
}

static const F32 SHAKE_FOREVER = FLT_MAX;

cameraFX* xCameraFXShakeForever(F32 magnitude, F32 cycleMax, F32 rotate_magnitude, F32 radius, const xVec3* epicenter, const xVec3* player, bool shakeY)
{
    return xCameraFXShake(SHAKE_FOREVER, magnitude, cycleMax, rotate_magnitude, radius, epicenter, player, shakeY);
}
void xCameraFXShakeEnd(cameraFX* fx, F32 fadeOutTime) {
    fx->maxTime = fadeOutTime;
    fx->elapsedTime = 0.0f;
    fx->shake.dampenRate = 1.0f / fadeOutTime;
}

cameraFX* xCameraFXShake(F32 maxTime, F32 magnitude, F32 cycleMax, F32 rotate_magnitude, F32 radius, const xVec3* epicenter, const xVec3* player, bool shakeY)
{
    if (zBulletTime_IsActive()) {
        return NULL;
    }

    cameraFX* f = xCameraFXAlloc();
    if (!f) {
        return NULL;
    }

    f->type = eCameraFXShake;
    f->maxTime = maxTime;
    f->shake.magnitude = magnitude;

    if (shakeY) {
        f->shake.dir.x = 0.0f;
        f->shake.dir.y = 1.0f;
    } else {
        f->shake.dir.x = 1.0f;
        f->shake.dir.y = 0.0f;
    }

    f->shake.cycleMax = cycleMax;
    f->shake.cycleTime = 0.0f;
    f->shake.dampen = 0.0f;
    f->shake.dampenRate = 1.0f / maxTime;
    f->shake.rotate_magnitude = rotate_magnitude;
    f->shake.radius = radius;
    f->shake.epicenterP = epicenter;
    if (f->shake.epicenterP)
        f->shake.epicenter = *f->shake.epicenterP;
    f->shake.player = player;
    return f;
}

void xCameraFXShakeUpdate(cameraFX* f, F32 dt, const xMat4x3*, xMat4x3* m)
{
    f->shake.cycleTime += dt;

    while (f->shake.cycleTime > f->shake.cycleMax) {
        f->shake.dir.x = -f->shake.dir.x;
        f->shake.dir.y = -f->shake.dir.y;
        f->shake.cycleTime -= f->shake.cycleMax;
    }

    F32 x, y;
    F32 scale = 1.0f;
    if (f->maxTime != SHAKE_FOREVER) {
        scale = f->shake.dampenRate * (f->maxTime - f->elapsedTime);
    }

    F32 noise = 0.1f * (xurand() - 0.5f);

    if (f->shake.radius > 0.0f && f->shake.epicenterP) {
        xVec3 d;
        if (!f->shake.player)
            xVec3Sub(&d, &sCameraFXMatOld.pos, &f->shake.epicenter);
        else
            xVec3Sub(&d, f->shake.player, &f->shake.epicenter);

        F32 len = d.length();
        if (len > f->shake.radius) {
            scale = 0.0f;
        } else {
            F32 s = 1.0f - len / f->shake.radius;
            scale *= xsqr(s);
        }
    }

    x = f->shake.cycleTime * ((scale * (f->shake.dir.x * (f->shake.magnitude + noise))) / f->shake.cycleMax) * isin(PI * (f->shake.cycleTime / f->shake.cycleMax));
    noise = 0.1f * (xurand() - 0.5f);
    y = f->shake.cycleTime * ((scale * (f->shake.dir.y * (f->shake.magnitude + noise))) / f->shake.cycleMax) * isin(PI * (f->shake.cycleTime / f->shake.cycleMax));

    xMat4x3MoveLocalRight(m, x);
    xMat4x3MoveLocalUp(m, y);

    xVec3 e;
    xMat3x3GetEuler(m, &e);
    e.z += f->shake.rotate_magnitude * (scale * (0.1f * (0.63661975f * (f->shake.cycleTime / f->shake.cycleMax - 0.5f))));
    xMat3x3Euler(m, &e);
}

void xCameraFXUpdate(xMat4x3& mat, RwCamera* icam, F32 dt)
{
    anycameraFXActive = false;

    for (S32 i = 0; i < MAX_CAMERA_FX; i++) {
        cameraFX* f = &sCameraFX[i];
        if (f->flags & CAMERAFX_ACTIVE) {
            anycameraFXActive = true;

            f->elapsedTime += dt;
            if ((f->maxTime > 0.0f && f->elapsedTime > f->maxTime) || (f->flags & CAMERAFX_KILL)) {
                f->flags = 0;
                if (sCameraFXTable[f->type].funcKill) {
                    sCameraFXTable[f->type].funcKill(f);
                }
            } else {
                if (sCameraFXTable[f->type].func) {
                    sCameraFXTable[f->type].func(f, dt, &sCameraFXMatOld, &mat);
                }
            }
        }
    }

    iCameraUpdatePos(icam, &mat);
}

void xCameraFXEnd(xMat4x3& mat, RwCamera* icam)
{
    xMat4x3Copy(&mat, &sCameraFXMatOld);
    iCameraUpdatePos(icam, &sCameraFXMatOld);
}

struct xScrFxLensFlareSource
{
    U32 flags;
    xVec3* pos;
    xVec3* at;
    xScrFxLensFlareSourceAsset* asset;
};

static xScrFxLensFlareSource* sFlareSource;
static S32 sNumFlareSources;
static xScrFxLensFlareParams sDefaultLensFlareParams = { 40.0f, 80.0f };
static xScrFxLensFlareParams* sLensFlareParams = &sDefaultLensFlareParams;
static iScrFxLensFlare sFlare[MAX_LENS_FLARES];
static S32 sNumFlares;
RwTexture* gxFlareTexture;
RwFrame* gxFlareFrame;

static S32 ObjRecurse(U32 assetID, void (*callback)(void*, U32, void*), void* data)
{
    if (!assetID) {
        return 0;
    }

    S32 count = 0;
    xBase* base = zSceneFindObject(assetID);
    if (base) {
        if (base->baseType == eBaseTypeGroup) {
            xGroup* group = (xGroup*)base;
            U32 numItems = xGroupGetCount(group);
            for (U32 i = 0; i < numItems; i++) {
                U32 memberID = xGroupGetItem(group, i);
                count += ObjRecurse(memberID, callback, data);
            }
        }
        else if (base->baseFlags & k_XBASE_IS_ENTITY) {
            if (callback) {
                callback(base, 0, data);
            }

            count++;
        }
    } else {
        U32 size;
        void* obj = xSTFindAsset(assetID, &size);
        if (obj) {
            if (size == sizeof(xVec3)) {
                if (callback) {
                    callback(obj, 1, data);
                }
                count++;
            }
            xDynAsset* da = (xDynAsset*)obj;
            if (da->type == xStrHash(location_asset::type_name())) {
                if (callback) {
                    callback(obj, 2, data);
                }
                count++;
            }
            else if (da->type == xStrHash(pointer_asset::type_name())) {
                if (callback) {
                    callback(obj, 3, data);
                }
                count++;
            } else if (da->type == xStrHash("effect:Lens Flare Element")) {
                if (callback) {
                    callback(obj, 4, data);
                }
                count++;
            }
        }
    }

    return count;
}

static void LensFlareElementCB(void* obj, U32, void* data)
{
    xScrFxLensFlareElement* element = (xScrFxLensFlareElement*)obj;
    xScrFxLensFlareSource* source = (xScrFxLensFlareSource*)data;
    xScrFxLensFlareSourceAsset* asset = source->asset;

    asset->elementArray[asset->numElements] = element;
    asset->numElements++;
}

static void LensFlareSourceCB(void* obj, U32 type, void* data)
{
    xScrFxLensFlareSource* source = &sFlareSource[sNumFlareSources];
    memset(source, 0, sizeof(xScrFxLensFlareSource));

    source->asset = (xScrFxLensFlareSourceAsset*)data;

    switch (type) {
    case 2:
        source->pos = &((location_asset*)obj)->loc;
        break;
    case 3:
        source->pos = &((pointer_asset*)obj)->loc;
        source->at = &((pointer_asset*)obj)->loc;
        break;
    case 1:
        source->pos = (xVec3*)obj;
        break;
    case 0:
        source->pos = (xVec3*)&((xEnt*)obj)->model->Mat->pos;
        source->at = (xVec3*)&((xEnt*)obj)->model->Mat->at;
        break;
    default:
        return;
    }

    sNumFlareSources++;

    if (!source->asset->elementArray) {
        U32 elementID = source->asset->elementID;
        S32 numElements = ObjRecurse(elementID, NULL, NULL);

        source->asset->elementArray = (xScrFxLensFlareElement**)xMEMALLOC(numElements * sizeof(xScrFxLensFlareElement*), 0);

        ObjRecurse(elementID, LensFlareElementCB, source);

        U32 texID = 0;
        for (S32 i = 0; i < (S32)source->asset->numElements; i++) {
            xScrFxLensFlareElement* element = source->asset->elementArray[i];
            if (element->textureID != 0) {
                if (texID == 0) {
                    texID = element->textureID;
                } else {
                    if (element->textureID != texID)
                        element->textureID = 0;
                }
            }
        }

        for (S32 i = 0; i < (S32)source->asset->numElements; i++) {
            xScrFxLensFlareElement* element = source->asset->elementArray[i];
            if (!element->texture) {
                RwTexture* tex = (RwTexture*)xSTFindAsset(element->textureID, NULL);
                element->texture = tex;
                RwRaster* raster = RwTextureGetRaster(element->texture);

                if (!(RwRasterGetFormat(raster) & rwRASTERFORMATPAL8) || !(RwRasterGetFormat(raster) & rwRASTERFORMAT8888)) {
                    RwInt32 width = RwRasterGetWidth(raster);
                    RwInt32 height = RwRasterGetHeight(raster);
                    RwRaster* newRaster;

                    RwImage* image = RwImageCreate(width, height, 32);
                    RwImageAllocatePixels(image);
                    RwImageSetFromRaster(image, raster);

                    RwInt32 format = RwRasterGetFormat(raster) | RwRasterGetType(raster);
                    format &= ~(RwRasterGetFormat(raster) & rwRASTERFORMATPIXELFORMATMASK);
                    format |= rwRASTERFORMAT8888 | rwRASTERFORMATPAL8;

                    newRaster = RwRasterCreate(width, height, 0, format);
                    if (newRaster) {
                        RwRasterSetFromImage(newRaster, image);
                        RwTextureSetRaster(tex, newRaster);
                        RwRasterDestroy(raster);
                    }

                    RwImageDestroy(image);
                }
            }
        }
    }
}

void xScrFxLensFlare_SceneEnter()
{
    iAsyncLockRWSentry rw_sentry;

    S32 numDyns = xSTAssetCountByType('DYNA');

    sNumFlareSources = 0;

    gxFlareFrame = RwFrameCreate();

    sLensFlareParams = &sDefaultLensFlareParams;
    for (S32 i = 0; i < numDyns; i++) {
        xDynAsset* dyn = (xDynAsset*)xSTFindAssetByType('DYNA', i, NULL);
        if (dyn->type == EFFECT_LENS_FLARE_PARAMS_ID) {
            sLensFlareParams = (xScrFxLensFlareParams*)(dyn + 1);
        }
    }
    sLensFlareParams->near_dist2 = xsqr(sLensFlareParams->near_dist);
    sLensFlareParams->far_dist2 = xsqr(sLensFlareParams->far_dist);

    S32 numSources = 0;
    for (S32 i = 0; i < numDyns; i++) {
        xDynAsset* dyn = (xDynAsset*)xSTFindAssetByType('DYNA', i, NULL);
        if (dyn->type == EFFECT_LENS_FLARE_SOURCE_ID) {
            if (dyn->version != LENS_FLARE_SOURCE_VERSION) {
                iprintf("WARNING:  skipping incompatible lens flare source %08X (%s)\n(version %d, current is %d)\n", dyn->id, xSTAssetName(dyn->id), dyn->version, LENS_FLARE_SOURCE_VERSION);
            } else
                numSources += ObjRecurse(((xScrFxLensFlareSourceAsset*)dyn)->object, NULL, NULL);
        }
    }

    if (numSources) {
        sFlareSource = (xScrFxLensFlareSource*)xMEMALLOC(numSources * sizeof(xScrFxLensFlareSource), 0);

        for (S32 i = 0; i < numDyns; i++) {
            xDynAsset* dyn = (xDynAsset*)xSTFindAssetByType('DYNA', i, NULL);
            if (dyn->type == EFFECT_LENS_FLARE_SOURCE_ID && dyn->version == LENS_FLARE_SOURCE_VERSION) {
                ObjRecurse(((xScrFxLensFlareSourceAsset*)dyn)->object, LensFlareSourceCB, dyn);
            }
        }
    }

    xScrFxLensFlare_Reset();
}

void xScrFxLensFlare_SceneExit()
{
    if (gxFlareFrame) {
        RwFrameDestroy(gxFlareFrame);
        gxFlareFrame = NULL;
    }
}

static void xScrFxLensFlare_Add(const xVec3* pos, const xVec3* at, F32 intensity, xScrFxLensFlareSource* source)
{
    if (sNumFlares >= MAX_LENS_FLARES) {
        return;
    }

    if (!source)
        return;

    iScrFxLensFlare* flare = &sFlare[sNumFlares];
    flare->pos = *pos;
    if (at) {
        flare->flags |= 0x1;
        flare->at = *at;
    }

    flare->intensity = intensity;
    flare->source = source;
    sNumFlares++;
}

static void xScrFxLensFlare_Add(xScrFxLensFlareSource* source)
{
    xScrFxLensFlare_Add(source->pos, source->at, source->asset->intensity, source);
}

void xScrFxLensFlare_Reset()
{
    iScrFxLensFlare_Reset();

    sNumFlares = 0;

    for (S32 i = 0; i < sNumFlareSources; i++)
        xScrFxLensFlare_Add(&sFlareSource[i]);
}

static void xScrFxLensFlare_RenderElements(RwRaster*, RwFrame* frame, U8 vCol, void* flr)
{
    RwIm3DVertex verts[4];
    RwImVertexIndex indices[4] = { 0, 1, 3, 2 };
    RwIm3DVertex qverts[NUM_QVERTS];
    RwImVertexIndex qindices[10] = { 4, 0, 1, 2, 5, 8, 7, 6, 3, 0 };
    RwCamera* camera = RwCameraGetCurrentCamera();
    const RwV2d* vw = RwCameraGetViewWindow(camera);
    RwMatrix invMtx, transform, tmp;
    RwMatrix* matrix = RwFrameGetLTM(RwCameraGetFrame(camera));

    if (!gxFlareTexture) {
        return;
    }

    RwV3d v;
    RwReal length;
    RwV3d position, FlarePosition;
    RwMatrixInvert(&invMtx, matrix);
    RwMatrixMultiply(&tmp, RwFrameGetLTM(frame), &invMtx);
    RwV3dAssign(&FlarePosition, RwMatrixGetPos(&tmp));

    if (FlarePosition.z <= 0.0f) {
        return;
    }

    RwV3dScale(&FlarePosition, &FlarePosition, 1.0f / FlarePosition.z);
    FlarePosition.z = 0.0f;

    RwV3dScale(&v, &FlarePosition, 1.0f);

    length = RwV3dLength(&v);

    if (length > 0.0f) {
        RwV3dScale(&v, &v, 1.0f / length);
    }

    RwIm3DVertexSetPos(&verts[0], -vw->x, vw->y, 1.0f);
    RwIm3DVertexSetPos(&verts[1], -vw->x, -vw->y, 1.0f);
    RwIm3DVertexSetPos(&verts[2], vw->x, -vw->y, 1.0f);
    RwIm3DVertexSetPos(&verts[3], vw->x, vw->y, 1.0f);

    RwIm3DVertexSetPos(&qverts[0], -vw->x, -vw->y, 1.0f);
    RwIm3DVertexSetPos(&qverts[1], 0.0f, -vw->y, 1.0f);
    RwIm3DVertexSetPos(&qverts[2], vw->x, -vw->y, 1.0f);
    RwIm3DVertexSetPos(&qverts[3], -vw->x, 0.0f, 1.0f);
    RwIm3DVertexSetPos(&qverts[4], 0.0f, 0.0f, 1.0f);
    RwIm3DVertexSetPos(&qverts[5], vw->x, 0.0f, 1.0f);
    RwIm3DVertexSetPos(&qverts[6], -vw->x, vw->y, 1.0f);
    RwIm3DVertexSetPos(&qverts[7], 0.0f, vw->y, 1.0f);
    RwIm3DVertexSetPos(&qverts[8], vw->x, vw->y, 1.0f);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(gxFlareTexture));

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);

    S32 done = FALSE;
    xScrFxLensFlareElement* ep;
    iScrFxLensFlare* flare = (iScrFxLensFlare*)flr;
    xScrFxLensFlareSource* source = (xScrFxLensFlareSource*)flare->source;

    vCol = vCol * flare->attenuatedIntensity;

    for (S32 i = 0; i < (S32)source->asset->numElements; i++) {
        ep = source->asset->elementArray[i];
        if (!ep) {
            continue;
        }

        if ((ep->mode & 0xFF) == 0) {
            verts[0].u = ep->uv[1][0];
            verts[0].v = ep->uv[0][1];
            verts[1].u = ep->uv[1][0];
            verts[1].v = ep->uv[1][1];
            verts[2].u = ep->uv[0][0];
            verts[2].v = ep->uv[1][1];
            verts[3].u = ep->uv[0][0];
            verts[3].v = ep->uv[0][1];

            U8 r = ((ep->color & 0xFF) * vCol) >> 8;
            U8 g = (((ep->color >> 8) & 0xFF) * vCol) >> 8;
            U8 b = (((ep->color >> 16) & 0xFF) * vCol) >> 8;

            RwIm3DVertexSetRGBA(&verts[0], r, g, b, 0xFF);
            RwIm3DVertexSetRGBA(&verts[1], r, g, b, 0xFF);
            RwIm3DVertexSetRGBA(&verts[2], r, g, b, 0xFF);
            RwIm3DVertexSetRGBA(&verts[3], r, g, b, 0xFF);

            RwV3dScale(&position, &v, ep->position * length);

            RwV3d flareSize;
            flareSize.x = ep->width;
            flareSize.y = ep->height;
            flareSize.z = 1.0f;
            RwMatrixScale(&tmp, &flareSize, rwCOMBINEREPLACE);
            RwMatrixTranslate(&tmp, &position, rwCOMBINEPOSTCONCAT);
            RwMatrixMultiply(&transform, &tmp, matrix);

            RwIm3DTransform(verts, ARRAY_LENGTH(verts), &transform, rwIM3D_VERTEXUV | rwIM3D_ALLOPAQUE | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA);

            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRISTRIP, indices, ARRAY_LENGTH(indices));
            RwIm3DEnd();
        } else if ((ep->mode & 0xFF) == 1) {
            F32 uadj = 0.5f / RwRasterGetWidth(RwTextureGetRaster(gxFlareTexture));
            F32 vadj = 0.5f / RwRasterGetHeight(RwTextureGetRaster(gxFlareTexture));

            qverts[0].u = ep->uv[0][0] + uadj;
            qverts[0].v = ep->uv[0][1] + vadj;
            qverts[1].u = ep->uv[1][0] - uadj;
            qverts[1].v = ep->uv[0][1] + vadj;
            qverts[2].u = ep->uv[0][0] + uadj;
            qverts[2].v = ep->uv[0][1] + vadj;
            qverts[3].u = ep->uv[0][0] + uadj;
            qverts[3].v = ep->uv[1][1] - vadj;
            qverts[4].u = ep->uv[1][0] - uadj;
            qverts[4].v = ep->uv[1][1] - vadj;
            qverts[5].u = ep->uv[0][0] + uadj;
            qverts[5].v = ep->uv[1][1] - vadj;
            qverts[6].u = ep->uv[0][0] + uadj;
            qverts[6].v = ep->uv[0][1] - vadj;
            qverts[7].u = ep->uv[1][0] - uadj;
            qverts[7].v = ep->uv[0][1] - vadj;
            qverts[8].u = ep->uv[0][0] + uadj;
            qverts[8].v = ep->uv[0][1] + vadj;

            U8 r = ((ep->color & 0xFF) * vCol) >> 8;
            U8 g = (((ep->color >> 8) & 0xFF) * vCol) >> 8;
            U8 b = (((ep->color >> 16) & 0xFF) * vCol) >> 8;

            for (S32 j = 0; j < NUM_QVERTS; j++) {
                RwIm3DVertexSetRGBA(&qverts[j], r, g, b, 0xFF);
            }

            RwV3dScale(&position, &v, ep->position * length);

            RwV3d flareSize;
            flareSize.x = ep->width;
            flareSize.y = ep->height;
            flareSize.z = 1.0f;
            RwMatrixScale(&tmp, &flareSize, rwCOMBINEREPLACE);
            RwMatrixTranslate(&tmp, &position, rwCOMBINEPOSTCONCAT);
            RwMatrixMultiply(&transform, &tmp, matrix);

            RwIm3DTransform(qverts, NUM_QVERTS, &transform, rwIM3D_VERTEXUV | rwIM3D_ALLOPAQUE | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA);

            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRIFAN, qindices, ARRAY_LENGTH(qindices));
            RwIm3DEnd();
        }
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
}

static S32 lfcmp(const void* a, const void* b); // UNUSED

static void dumpFlare(iScrFxLensFlare* flare)
{
}

S32 dumpFlares;

void xScrFxLensFlare_Render(RwCamera* cam)
{
    {
        iScrFxLensFlare* flare;
        RwMatrix* cammat = RwFrameGetMatrix(RwCameraGetFrame(globalCamera));
        xVec3* campos = (xVec3*)RwMatrixGetPos(cammat);
        xVec3* cam_at = (xVec3*)&cammat->at;

        flare = sFlare;
        for (S32 i = 0; i < sNumFlares; i++, flare++) {
            flare->dist2 = xVec3Dist2(&flare->pos, campos);
        }

        if (dumpFlares) {
            for (S32 i = 0; i < sNumFlares; i++) {
                dumpFlare(&sFlare[i]);
            }
        }

        if (dumpFlares) {
            for (S32 i = 0; i < sNumFlares; i++) {
                dumpFlare(&sFlare[i]);
            }
        }

        flare = sFlare;
        for (S32 i = 0; i < sNumFlares; i++, flare++) {
            xScrFxLensFlareSource* source = (xScrFxLensFlareSource*)flare->source;
            F32 intensity = flare->intensity;
            if (intensity == 0.0f) {
                continue;
            }

            if (source->asset->distanceAttenuationMode == 1) {
                if (flare->dist2 >= sLensFlareParams->far_dist2) {
                    continue;
                }

                if (flare->dist2 > sLensFlareParams->near_dist2) {
                    intensity *= (sLensFlareParams->far_dist2 - flare->dist2) / (sLensFlareParams->far_dist2 - sLensFlareParams->near_dist2);
                }
            }

            xVec3 v1, v2;
            if (flare->flags & 0x1) {
                v1 = flare->at;
                xVec3Sub(&v2, campos, &flare->pos);
            } else {
                v1 = *cam_at;
                xVec3Inv(&v1, &v1);
                xVec3Sub(&v2, campos, &flare->pos);
            }

            xVec3Normalize(&v1, &v1);
            xVec3Normalize(&v2, &v2);
            F32 dot = xVec3Dot(&v1, &v2);

            F32 inner = (PI * source->asset->coneAngle) / 180.0f;
            F32 outer = 1.25f * inner;
            inner = icos(inner);
            outer = icos(outer);

            if (dot < outer)
                continue;

            if (dot >= outer && dot < inner) {
                intensity *= (dot - outer) / (inner - outer);
            }

            intensity = CLAMP(intensity, 0.0f, 1.0f);
            flare->attenuatedIntensity = intensity;

            gxFlareTexture = source->asset->elementArray[0]->texture;
            iScrFxLensFlare_Render(cam, &sFlare[i], xScrFxLensFlare_RenderElements);
            gxFlareTexture = NULL;
        }
    }

    xScrFxLensFlare_Reset();
}

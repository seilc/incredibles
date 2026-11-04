#include "xFXHighDynamicRange.h"

#include "xDebugTweak.h"
#include "xColor.h"
#include "xGlobals.h"
#include "xMath.h"
#include "iDraw.h"
#include "zGameState.h"
#include "zMain.h"
#include "zMenu.h"
#include "zRenderState.h"

#define MAX_TEXDATA 8

bool HDR_brightening;

namespace {

    inline void refresh_blur_packets()
    {
    }

    namespace tweak {

        xTWEAKABLE S32 background_glow_default = 10;
        xTWEAKABLE S32 background_darken_default = 80;
        F32 overbrighten = 3.0f;
        F32 overbrighten_decay = 0.3f;
        S32 downsamples = 4;
        xTWEAKABLE bool render = true;
        bool blur_faster = true;
        F32 colorize_red = 1.0f;
        F32 colorize_green = 1.0f;
        F32 colorize_blue = 1.0f;
        F32 colorize_alpha = 1.0f;

        void add_tweaks() {}

    }

    bool enabled = true;
    S32 background_glow_normal;
    S32 background_darken_normal;

    struct interpolator
    {
        S32 value;
        F32 start;
        F32 end;
        F32 current;
        F32 t;
        F32 trate;

        void reset(S32 value)
        {
            start = current = end = this->value = value;
            t = trate = 0.0f;
        }

        void go(S32 value, F32 time)
        {
            if (time <= 0.0f || value == this->value) {
                reset(value);
                return;
            }

            start = current;
            end = value;
            t = 0.0f;
            trate = 1.0f / time;
        }

        void update(F32 dt)
        {
            if (trate == 0.0f) return;

            t += trate * dt;
            if (t >= 1.0f) {
                start = current = end;
                t = 0.0f;
                trate = 0.0f;
            } else {
                F32 s = t;
                current = 0.5f + ((1.0f - s) * start + s * end);
            }

            value = (S32)(0.5f + current);
        }
    };

    interpolator background_glow;
    interpolator background_darken;

}

void xFXHighDynamicRangeSetColorize(F32 r, F32 g, F32 b, F32 alpha)
{
    tweak::colorize_red = r;
    tweak::colorize_green = g;
    tweak::colorize_blue = b;
    tweak::colorize_alpha = alpha;
}

namespace {

    inline void set_vert(RwIm2DVertex& vert, F32 x, F32 y, F32 u, F32 v, xColor color, F32 rcz, F32 nsz)
    {
        RwIm2DVertexSetScreenX(&vert, x);
        RwIm2DVertexSetScreenY(&vert, y);
        RwIm2DVertexSetScreenZ(&vert, nsz);

        RwIm2DVertexSetU(&vert, u, rcz);
        RwIm2DVertexSetV(&vert, v, rcz);
        RwIm2DVertexSetIntRGBA(&vert, color.r, color.g, color.b, color.a);
    }

}

S32 hdr_dump_efb;
GXBlendFactor hdr_src = GX_BL_SRCALPHA;
GXBlendFactor hdr_dst = GX_BL_ONE;
GXBlendFactor hdr_down_src = GX_BL_SRCALPHA;
GXBlendFactor hdr_down_dst = GX_BL_ZERO;
void* texData[MAX_TEXDATA];
U16 sizes[9][2] = {
    { 256, 240 },
    { 128, 128 },
    { 64, 64 },
    { 32, 32 },
    { 16, 16 },
    { 8, 8 },
    { 4, 4 },
    { 2, 2 },
    { 1, 1 }
};

namespace {

    inline void add_global_tweaks() {}

}

void xFXHighDynamicRangeInit()
{
    enabled = true;

    background_glow_normal = tweak::background_glow_default;
    background_darken_normal = tweak::background_darken_default;

    xFXHighDynamicRangeReset();

    add_global_tweaks();
    tweak::add_tweaks();
}

void xFXHighDynamicRangeSceneEnter()
{
}

void xFXHighDynamicRangeSceneExit()
{
}

void xFXHighDynamicRangeReset()
{
    enabled = true;
    background_glow.reset(background_glow_normal);
    background_darken.reset(background_darken_normal);
}

void xFXHighDynamicRangeStartRender()
{
    if (!tweak::render || !enabled)
        return;

    HDR_brightening = false;

    U32 mask = iDrawGetFBMSK();
    iDrawSetFBMSK(0xFFFFFF, 0xFFFFFFFF, true);

    RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, 0);

    _rwDlRenderStateSetZCompLoc(FALSE);

    zRenderState(SDRS_Fill);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwReal rcz = 1.0f / RwCameraGetNearClipPlane(RwCameraGetCurrentCamera());
    RwReal nsz = RwIm2DGetNearScreenZ();

    RwIm2DVertex verts[4];
    xColor clearcolor;

    clearcolor.r = 0;
    clearcolor.g = 0;
    clearcolor.b = 0;
    clearcolor.a = MAX(1, background_glow.value);

    set_vert(verts[0], 0.0f, 0.0f, 0.0f, 0.0f, clearcolor, rcz, nsz);
    set_vert(verts[1], 0.0f, FB_YRES, 0.0f, 0.0f, clearcolor, rcz, nsz);
    set_vert(verts[2], FB_XRES, 0.0f, 0.0f, 0.0f, clearcolor, rcz, nsz);
    set_vert(verts[3], FB_XRES, FB_YRES, 0.0f, 0.0f, clearcolor, rcz, nsz);
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, 4);

    iDrawSetFBMSK(mask, 0xFFFFFFFF, true);

    zRenderState(SDRS_HDRBeginRender);
}

void xFXHighDynamicRangeRender(RwCamera*)
{
    S32 i;
    U16 sX, sY;

    if (!tweak::render || !enabled)
        return;

    if (globals.inLoadingScreen || (!zMenuRunning() && zGameGetOstrich() != eGameOstrich_InScene))
        return;

    zRenderState(SDRS_Fill);

    U16 curSizeX = FB_XRES / 2, curSizeY = FB_YRES / 2;
    sizes[0][1] = FB_YRES / 2;

    iXFBreset();

    for (i = 0; i < MAX_TEXDATA; i++) {
        sX = sizes[i][0];
        sY = sizes[i][1];
        texData[i] = iXFBalloc(sX * sY * 4);
    }

    GXSetTexCopySrc(0, 0, FB_XRES, FB_YRES);
    GXSetTexCopyDst(curSizeX, curSizeY, GX_TF_RGBA8, GX_TRUE);
    GXCopyTex(texData[0], GX_FALSE);
    GXPixModeSync();

    void* frame = iXFBalloc(128 * 128 * 4);
    GXSetTexCopySrc(0, 0, 128, 128);
    GXSetTexCopyDst(128, 128, GX_TF_RGBA8, GX_FALSE);
    GXCopyTex(frame, GX_FALSE);
    GXPixModeSync();

    Mtx44 mProjection;
    MTXOrtho(mProjection, 0.0f, FB_YRES, 0.0f, FB_XRES, 0.0f, 1.0f);
    GXSetProjection(mProjection, GX_ORTHOGRAPHIC);

    GXSetCurrentMtx(GX_PNMTX0);

    Mtx44 mIdentity;
    MTXIdentity(mIdentity);
    GXLoadPosMtxImm(mIdentity, GX_PNMTX0);

    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);

    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    GXSetNumChans(0);
    RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetCullMode(GX_CULL_NONE);

    GXSetNumTevStages(5);

    F32 overbrighten = tweak::overbrighten;
    F32 alpha, resample_alpha;
    S32 resamples;
    GXColor scaleColor;

    resamples = 5;
    resample_alpha = 1.0f / resamples;
    alpha = (1.0f - resample_alpha) * (3.0f * overbrighten) + resample_alpha;
    overbrighten *= tweak::overbrighten_decay;

    scaleColor.r = 255 / resamples;
    scaleColor.g = 255 / resamples;
    scaleColor.b = 255 / resamples;
    scaleColor.a = 255;

    if (overbrighten < 0.0f)
        overbrighten = 0.0f;

    U8 ualpha = 255.0f * alpha;
    scaleColor.r = ualpha;
    scaleColor.g = ualpha;
    scaleColor.b = ualpha;
    scaleColor.a = ualpha;
    GXSetTevColor(GX_TEVREG0, scaleColor);

    for (S32 i = 0; i < 5; i++) {
        GXSetTevColorIn((GXTevStageID)i, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0, GX_CC_CPREV);
        GXSetTevColorOp((GXTevStageID)i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn((GXTevStageID)i, GX_CA_ZERO, GX_CA_TEXA, GX_CA_A0, GX_CA_APREV);
        GXSetTevAlphaOp((GXTevStageID)i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevOrder((GXTevStageID)i, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    }

    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0, GX_CC_ZERO);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_A0, GX_CA_ZERO);
    GXSetBlendMode(GX_BM_BLEND, hdr_down_src, hdr_down_dst, GX_LO_SET);

    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
    GXSetTexCoordGen(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX1);
    GXSetTexCoordGen(GX_TEXCOORD3, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX2);
    GXSetTexCoordGen(GX_TEXCOORD4, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX3);

    GXSetViewport(0.0f, 0.0f, FB_XRES, FB_YRES, 0.0f, 1.0f);
    GXSetScissor(0, 0, FB_XRES, FB_YRES);

    GXTexObj texObject ATTRIBUTE_ALIGN(16);
    GXInitTexObj(&texObject, texData[0], curSizeX, curSizeY, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);

    GXSetAlphaUpdate(GX_TRUE);
    GXPokeAlphaRead(GX_READ_NONE);

    curSizeX = 128;
    curSizeY = 128;

    for (S32 i = 1; i < tweak::downsamples; i++) {
        GXLoadTexObj(&texObject, GX_TEXMAP0);

        F32 isizex = 1.0f / curSizeX;
        F32 isizey = 1.0f / curSizeY;

        static const F32 offsets[4][2] = {
            { -2.5f, -2.5f },
            { 2.5f, -2.5f },
            { -2.5f, 2.5f },
            { 2.5f, 2.5f }
        };

        F32 matrix[2][4] = { { 1.0f, 0.0f, 0.0f, 0.0f },
                             { 0.0f, 1.0f, 0.0f, 0.0f } };

        matrix[0][3] = offsets[0][0] * isizex; matrix[1][3] = offsets[0][1] * isizey;
        GXLoadTexMtxImm(matrix, GX_TEXMTX0, GX_MTX2x4);

        matrix[0][3] = offsets[1][0] * isizex; matrix[1][3] = offsets[1][1] * isizey;
        GXLoadTexMtxImm(matrix, GX_TEXMTX1, GX_MTX2x4);

        matrix[0][3] = offsets[2][0] * isizex; matrix[1][3] = offsets[2][1] * isizey;
        GXLoadTexMtxImm(matrix, GX_TEXMTX2, GX_MTX2x4);

        matrix[0][3] = offsets[3][0] * isizex; matrix[1][3] = offsets[3][1] * isizey;
        GXLoadTexMtxImm(matrix, GX_TEXMTX3, GX_MTX2x4);

        GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
        GXPosition2f32(0.0f, 0.0f);
        GXTexCoord2f32(0.0f, 0.0f);
        GXPosition2f32(0.0f, curSizeY);
        GXTexCoord2f32(0.0f, 1.0f);
        GXPosition2f32(curSizeX, 0.0f);
        GXTexCoord2f32(1.0f, 0.0f);
        GXPosition2f32(curSizeX, curSizeY);
        GXTexCoord2f32(1.0f, 1.0f);
        GXEnd();

        alpha = (1.0f - resample_alpha) * (3.0f * overbrighten) + resample_alpha;
        overbrighten *= tweak::overbrighten_decay;
        if (overbrighten < 0.0f)
            overbrighten = 0.0f;

        ualpha = 255.0f * alpha;
        scaleColor.r = ualpha;
        scaleColor.g = ualpha;
        scaleColor.b = ualpha;
        scaleColor.a = ualpha;
        GXSetTevColor(GX_TEVREG0, scaleColor);

        GXSetTexCopySrc(0, 0, curSizeX, curSizeY);
        GXSetTexCopyDst(curSizeX, curSizeY, GX_TF_RGBA8, GX_FALSE);
        GXCopyTex(texData[i], GX_FALSE);

        GXPixModeSync();
        GXInvalidateTexAll();

        GXInitTexObj(&texObject, texData[i], curSizeX, curSizeY, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);

        curSizeX >>= 1;
        curSizeY >>= 1;

        GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);
    }

    GXInitTexObj(&texObject, texData[1], sizes[1][0], sizes[1][1], GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);

    GXSetViewport(0.0f, 0.0f, FB_XRES, FB_YRES, 0.0f, 1.0f);
    GXSetScissor(0, 0, FB_XRES, FB_YRES);

    GXTexObj screenBack ATTRIBUTE_ALIGN(16);
    GXInitTexObj(&screenBack, frame, 128, 128, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);

    GXSetNumTexGens(1);
    GXSetNumTevStages(1);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXLoadTexObj(&screenBack, GX_TEXMAP0);

    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
    GXPosition2f32(0.0f, 0.0f);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition2f32(0.0f, 128.0f);
    GXTexCoord2f32(0.0f, 1.0f);
    GXPosition2f32(128.0f, 0.0f);
    GXTexCoord2f32(1.0f, 0.0f);
    GXPosition2f32(128.0f, 128.0f);
    GXTexCoord2f32(1.0f, 1.0f);
    GXEnd();

    GXSetAlphaUpdate(GX_FALSE);
    GXLoadTexObj(&texObject, GX_TEXMAP0);

    GXSetBlendMode(GX_BM_BLEND, hdr_src, hdr_dst, GX_LO_SET);

    scaleColor.r = 128;
    scaleColor.g = 128;
    scaleColor.b = 128;
    scaleColor.a = 255;
    GXSetTevColor(GX_TEVREG0, scaleColor);

    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_A0, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
    GXPosition2f32(0.0f, 0.0f);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition2f32(0.0f, FB_YRES);
    GXTexCoord2f32(0.0f, 1.0f);
    GXPosition2f32(FB_XRES, 0.0f);
    GXTexCoord2f32(1.0f, 0.0f);
    GXPosition2f32(FB_XRES, FB_YRES);
    GXTexCoord2f32(1.0f, 1.0f);
    GXEnd();

    GXDrawDone();

    hdr_dump_efb = 0;
}

U32 xFXHighDynamicRangeBrightenPtank(RpAtomic* ptank, bool bright, S32)
{
    return 0;
}

void xFXHighDynamicRangeRestorePtank(RpAtomic* ptank, U32 oldtest)
{
}

void xFXHighDynamicRangeTunePalette(RwRaster* raster, F32 base_alpha, bool additive)
{
}

void xFXHighDynamicRangeUpdate(F32 dt)
{
    background_glow.update(dt);
    background_darken.update(dt);
}

S32 xFXHighDynamicRangeGetBackgroundGlow() { return background_glow.value; }

void xFXHighDynamicRangeSetConfiguration(const xFXHighDynamicRangeConfiguration& configuration)
{
    background_glow.value = configuration.glow;
    background_darken.value = configuration.darken;
    tweak::overbrighten = configuration.overbrighten;
    tweak::overbrighten_decay = configuration.overbrighten_decay;
    tweak::downsamples = configuration.downsamples;
    tweak::blur_faster = configuration.blur_faster;
    refresh_blur_packets();
}

void xFXHighDynamicRangeGetConfiguration(xFXHighDynamicRangeConfiguration& configuration)
{
    configuration.glow = background_glow.value;
    configuration.darken = background_darken.value;
    configuration.overbrighten = tweak::overbrighten;
    configuration.overbrighten_decay = tweak::overbrighten_decay;
    configuration.downsamples = tweak::downsamples;
    configuration.blur_faster = tweak::blur_faster;
}

void xFXHighDynamicRangeSetGlowClamp(S32 glow_min, bool dest_test)
{
    RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_GEQUAL, CLAMP(glow_min, 0, 255));
    _rwDlRenderStateSetZCompLoc(FALSE);
}

void xFXHighDynamicRangeResetGlowClamp()
{
}

void xFXHighDynamicRangeSetBackground(S32 glow, S32 darken, F32 time)
{
    if (glow < 0)
        glow = background_glow_normal;
    else if (glow > 255)
        glow = 255;
    background_glow.go(glow, time);

    if (darken < 0)
        darken = background_darken_normal;
    else if (darken > 255)
        darken = 255;
    background_darken.go(darken, time);
}

void xFXHighDynamicRangeSetBackgroundDefault(S32 glow, S32 darken, bool apply)
{
    background_glow_normal = CLAMP(glow, 0, 255);
    background_darken_normal = CLAMP(darken, 0, 255);

    if (apply) {
        background_glow.reset(background_glow_normal);
        background_darken.reset(background_darken_normal);
    }
}

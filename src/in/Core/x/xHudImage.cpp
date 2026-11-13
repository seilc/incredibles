#include "xCommon.h"
#include "xHudImage.h"

#include "xHudAsset.h"
#include "xFont.h"
#include "xMath.h"
#include "xRand.h"
#include "xstransvc.h"
#include "zMain.h"
#include "iDraw.h"
#include "iMath.h"

#include <string.h>

#define ADD_TRI(a, b, c)                        \
    *pIndex++ = m_uVertexCount + (a);           \
    *pIndex++ = m_uVertexCount + (b);           \
    *pIndex++ = m_uVertexCount + (c)

namespace xhud {

void image_widget::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    init_base(data, asset, sizeof(widget_chunk));
    widget_chunk& wc = (widget_chunk&)data;
    new (&wc.w) image_widget((image_asset&)asset);
}

image_widget::image_widget(const image_asset& a)
    : widget(a)
    , m_pRaster(NULL)
    , m_pLayers(NULL)
    , m_paVertices(NULL)
    , m_paIndices(NULL)
    , m_uVertexCount(0)
    , m_uIndexCount(0)
    , m_fRecipZ(5.0f)
    , m_VertexMax(0)
    , m_IndexMax(0)
{
    RwTexture* pTexture = (RwTexture*)xSTFindAsset(a.image, NULL);

    m_pRaster = RwTextureGetRaster(pTexture);
}

void image_widget::destruct()
{
    m_pRaster = NULL;
    m_pLayers = NULL;
    m_paVertices = NULL;
    m_paIndices = NULL;
    m_VertexMax = 0;
    m_IndexMax = 0;
    widget::destruct();
}

void image_widget::destroy()
{
    destruct();
}

U32 image_widget::type() const
{
    static U32 myid = xStrHash(model_asset::type_name());
    return myid;
}

bool image_widget::is(U32 id) const
{
    return id == image_widget::type() || widget::is(id);
}

void image_widget::update(F32 dt)
{
    updater(dt);

    if (!visible() || rc.a <= 0.5f / 255.0f) {
        return;
    }

    for (U32 uLayer = 0; uLayer < m_uLayerCount; uLayer++) {
        if (m_pLayers[uLayer].eType == STROBE_EFFECT)
            update_strobe_effect(uLayer, dt);
    }
}

void image_widget::update_strobe_effect(U32 uLayer, F32 dt)
{
    F32 fStrobeRangePixels = INSCREENY(rc.size.y);
    F32 fStrobeRangePercent = 1.0f + m_pLayers[uLayer].fStrobeHeight / fStrobeRangePixels;

    F32 fUpdateAmount = dt * fStrobeRangePercent / m_pLayers[uLayer].fStrobeFreq;
    m_pLayers[uLayer].fStrobe += fUpdateAmount;

    if (m_pLayers[uLayer].fStrobe > 1.0f) {
        if (m_pLayers[uLayer].nStrobeCount > 0) {
            m_pLayers[uLayer].nStrobeCount--;
        }

        m_pLayers[uLayer].fStrobe = 0.0f;
    }
}

void image_widget::render()
{
    if (rc.a >= -0.00001f && rc.a <= 0.00001f) {
        return;
    }

    m_paVertices = (RwIm2DVertex*)xMEMPUSHTEMP(m_VertexMax * sizeof(RwIm2DVertex));
    m_paIndices = (U16*)xMEMPUSHTEMP(m_IndexMax * sizeof(U16));

    static RwReal fNearZ = RwIm2DGetNearScreenZ();
    m_fRecipZ = 5.0f;
    if (RwCameraGetCurrentCamera()) {
        m_fRecipZ = 1.0f / RwCameraGetNearClipPlane(RwCameraGetCurrentCamera());
    }

    for (U32 i = 0; i < m_VertexMax; i++) {
        RwIm2DVertexSetScreenZ(&m_paVertices[i], fNearZ);
        RwIm2DVertexSetIntRGBA(&m_paVertices[i], 0xFF, 0xFF, 0xFF, 0xFF);
    }

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, m_pRaster);

    m_uVertexCount = 0;
    m_uIndexCount = 0;
    m_pVertices = m_paVertices;

    LAYER_FLAGS eLayerMode = (LAYER_FLAGS)-1;
    bool bFlushTempBuffer = false;
    for (U32 uLayer = 0; uLayer < m_uRenderLayerCount; uLayer++) {
        if (m_uVertexCount + 12 >= m_VertexMax) {
            bFlushTempBuffer = true;
        }
        if (m_uIndexCount + 24 >= m_IndexMax) {
            bFlushTempBuffer = true;
        }

        U32 uNewMode = m_pLayers[uLayer].uFlags & BLEND_MASK;
        if (eLayerMode != uNewMode || bFlushTempBuffer) {
            if (m_uVertexCount) {
                RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, m_paVertices, m_uVertexCount, m_paIndices, m_uIndexCount);
                bFlushTempBuffer = false;
                m_uVertexCount = 0;
                m_uIndexCount = 0;
            }

            eLayerMode = (LAYER_FLAGS)uNewMode;
            if (uNewMode == BLEND) {
                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
                iDrawSetFBMSK(0xFF000000, 0xFFFFFFFF, true);
            }
            else if (uNewMode == ADDITIVE) {
                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
                iDrawSetFBMSK(0xFF000000, 0xFFFFFFFF, true);
            }
            else if (uNewMode == SET_DEST_ALPHA) {
                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
                iDrawSetFBMSK(0x00FFFFFF, 0xFFFFFFFF, true);
                RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
            }
            else if (uNewMode == USE_DEST_ALPHA) {
                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDDESTALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
                iDrawSetFBMSK(0xFF000000, 0xFFFFFFFF, true);
            }
        }

        bool bRendered = false;
        switch (m_pLayers[uLayer].eType) {
        case TOPLEFT:
        case BOTTOMLEFT:
        case BOTTOMRIGHT:
        case TOPRIGHT:
        case OFFSET_CENTERPT:
            bRendered = render_one_to_one(uLayer, m_pLayers[uLayer].eType);
            break;
        case OFFSET_PIE:
            bRendered = render_offset_pie(uLayer);
            break;
        case ROTATE_CCW:
            bRendered = render_rotate_ccw(uLayer);
            break;
        case OFFSET_ROTATE_CCW:
            bRendered = render_offset_rotate_ccw(uLayer);
            break;
        case STROBE_EFFECT:
            bRendered = render_strobe_effect(uLayer);
            break;
        case SPECIAL_STROBE:
            bRendered = render_special_strobe(uLayer);
            break;
        case VARIABLE_TOP:
            bRendered = render_variable_top(uLayer);
            break;
        case VARIABLE_RIGHT:
            bRendered = render_variable_right(uLayer);
            break;
        case COUNTER_CLOCKWISE:
            bRendered = render_counter_clockwise(uLayer);
            break;
        case HALF_CCW:
            bRendered = render_half_ccw(uLayer);
            break;
        case RANDOM_V:
            bRendered = render_random_v(uLayer);
            break;
        }

        if (bRendered) {
            set_buffer_to_layer(m_pLayers[uLayer].eType);
        }
    }

    if (m_uVertexCount) {
        RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, m_paVertices, m_uVertexCount, m_paIndices, m_uIndexCount);
    }

    xMEMPOPTEMP(m_paIndices);
    m_paIndices = NULL;

    xMEMPOPTEMP(m_paVertices);
    m_paVertices = NULL;
    m_pVertices = NULL;
}

bool image_widget::render_one_to_one(U32 uLayer, RENDERTYPE eType)
{
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 u0 = m_pLayers[uLayer].upperLeft.u;
    F32 v0 = m_pLayers[uLayer].upperLeft.v;
    F32 u1 = m_pLayers[uLayer].lowerRight.u;
    F32 v1 = m_pLayers[uLayer].lowerRight.v;
    F32 fQuadX = rc.loc.x * FB_XRES;
    F32 fQuadY = rc.loc.y * FB_YRES;
    F32 fQuadWidth = (m_pLayers[uLayer].lowerRight.u - m_pLayers[uLayer].upperLeft.u) * get_texture_width();
    F32 fQuadHeight = (m_pLayers[uLayer].lowerRight.v - m_pLayers[uLayer].upperLeft.v) * get_texture_height();

    if (eType != TOPLEFT)
        if (eType == TOPRIGHT) {
            fQuadX = rc.loc.x * FB_XRES + INSCREENX(rc.size.x) - fQuadWidth;
        }
        else if (eType == BOTTOMLEFT) {
            fQuadY = rc.loc.y * FB_YRES + INSCREENY(rc.size.y) - fQuadHeight;
        }
        else if (eType == BOTTOMRIGHT) {
            fQuadX = rc.loc.x * FB_XRES + INSCREENX(rc.size.x) - fQuadWidth;
            fQuadY = rc.loc.y * FB_YRES + INSCREENY(rc.size.y) - fQuadHeight;
        }
        else if (eType == OFFSET_CENTERPT) {
            F32 fCenterX = rc.loc.x * FB_XRES + 0.5f * INSCREENX(rc.size.x);
            F32 fCenterY = rc.loc.y * FB_YRES + 0.5f * INSCREENY(rc.size.y);
            fQuadWidth *= m_pLayers[uLayer].fScaleX;
            fQuadHeight *= m_pLayers[uLayer].fScaleY;
            F32 fHalfImageWidth = 0.5f * fQuadWidth;
            F32 fHalfImageHeight = 0.5f * fQuadHeight;
            fQuadX = fCenterX + m_pLayers[uLayer].fOffsetX - fHalfImageWidth;
            fQuadY = fCenterY + m_pLayers[uLayer].fOffsetY - fHalfImageHeight;
        }

    SetFourVertsToQuad(&m_pVertices[m_uVertexCount], fQuadX, fQuadY, fQuadWidth, fQuadHeight, u0, v0, u1, v1, m_fRecipZ);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a * m_pLayers[uLayer].fAlpha);
    return true;
}

bool image_widget::render_variable_top(U32 uLayer)
{
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fPercent = m_pLayers[uLayer].fCurPercent;
    F32 fHeight = fPercent * INSCREENY(rc.size.y);
    F32 fTopY = rc.loc.y * FB_YRES + INSCREENY(rc.size.y) - fHeight;
    F32 fTopV = fPercent * (m_pLayers[uLayer].upperLeft.v - m_pLayers[uLayer].lowerRight.v) + m_pLayers[uLayer].lowerRight.v;

    F32 fWidth = INSCREENX(rc.size.x);
    F32 fTopX = rc.loc.x * FB_XRES;
    F32 fTopU = m_pLayers[uLayer].upperLeft.u;
    F32 fBotU = m_pLayers[uLayer].lowerRight.u;
    SetFourVertsToQuad(&m_pVertices[m_uVertexCount], fTopX, fTopY, fWidth, fHeight,
                       fTopU, fTopV, fBotU, m_pLayers[uLayer].lowerRight.v,
                       m_fRecipZ);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a * m_pLayers[uLayer].fAlpha);
    return true;
}

bool image_widget::render_variable_right(U32 uLayer)
{
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fPercent = m_pLayers[uLayer].fCurPercent;
    F32 fTopX = rc.loc.x * FB_XRES;
    F32 fTopY = rc.loc.y * FB_YRES;
    F32 fTopU = m_pLayers[uLayer].upperLeft.u;
    F32 fTopV = m_pLayers[uLayer].upperLeft.v;
    F32 fWidth = fPercent * INSCREENX(rc.size.x);
    F32 fHeight = INSCREENY(rc.size.y);

    F32 fBotU = fPercent * (m_pLayers[uLayer].lowerRight.u - m_pLayers[uLayer].upperLeft.u) + m_pLayers[uLayer].upperLeft.u;
    F32 fBotV = m_pLayers[uLayer].lowerRight.v;
    SetFourVertsToQuad(&m_pVertices[m_uVertexCount], fTopX, fTopY, fWidth, fHeight, fTopU, fTopV, fBotU, fBotV, m_fRecipZ);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a * m_pLayers[uLayer].fAlpha);
    return true;
}

bool image_widget::render_counter_clockwise(U32 uLayer)
{
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fPercent = m_pLayers[uLayer].fCurPercent;
    F32 u0 = m_pLayers[uLayer].upperLeft.u;
    F32 v0 = m_pLayers[uLayer].upperLeft.v;
    F32 u1 = m_pLayers[uLayer].lowerRight.u;
    F32 v1 = m_pLayers[uLayer].lowerRight.v;

    F32 fHalfWidth = INSCREENX(0.5f * rc.size.x);
    F32 fHalfHeight = INSCREENY(0.5f * rc.size.y);
    F32 fCenterX = rc.loc.x * FB_XRES + fHalfWidth;
    F32 fCenterY = rc.loc.y * FB_YRES + fHalfHeight;
    F32 fCenterYPlusfHalfHeight = fCenterY + fHalfHeight;
    F32 fCenterYMinusfHalfHeight = fCenterY - fHalfHeight;
    F32 fCenterU = 0.5f * (u0 + u1);
    F32 fCenterV = 0.5f * (v0 + v1);

    F32 fRangeToEight = 8.0f * fPercent;
    S32 nWholeTris = fRangeToEight;
    S32 nWholeTrisPlus1 = nWholeTris + 1;
    S32 nWholeTrisPlus2 = nWholeTris + 2;

    switch (nWholeTrisPlus1) {
    case 9:
    case 8:
        VertSetXYUV(m_pVertices[m_uVertexCount + 9], fCenterX, fCenterYMinusfHalfHeight, fCenterU, v0, m_fRecipZ);
    case 7:
        VertSetXYUV(m_pVertices[m_uVertexCount + 8], fCenterX + fHalfWidth, fCenterYMinusfHalfHeight, u1, v0, m_fRecipZ);
    case 6:
        VertSetXYUV(m_pVertices[m_uVertexCount + 7], fCenterX + fHalfWidth, fCenterY, u1, fCenterV, m_fRecipZ);
    case 5:
        VertSetXYUV(m_pVertices[m_uVertexCount + 6], fCenterX + fHalfWidth, fCenterYPlusfHalfHeight, u1, v1, m_fRecipZ);
    case 4:
        VertSetXYUV(m_pVertices[m_uVertexCount + 5], fCenterX, fCenterYPlusfHalfHeight, fCenterU, v1, m_fRecipZ);
    case 3:
        VertSetXYUV(m_pVertices[m_uVertexCount + 4], fCenterX - fHalfWidth, fCenterYPlusfHalfHeight, u0, v1, m_fRecipZ);
    case 2:
        VertSetXYUV(m_pVertices[m_uVertexCount + 3], fCenterX - fHalfWidth, fCenterY, u0, fCenterV, m_fRecipZ);
    case 1:
        VertSetXYUV(m_pVertices[m_uVertexCount + 2], fCenterX - fHalfWidth, fCenterYMinusfHalfHeight, u0, v0, m_fRecipZ);
        VertSetXYUV(m_pVertices[m_uVertexCount + 1], fCenterX, fCenterYMinusfHalfHeight, fCenterU, v0, m_fRecipZ);
        VertSetXYUV(m_pVertices[m_uVertexCount], fCenterX, fCenterY, fCenterU, fCenterV, m_fRecipZ);
        break;
    }

    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount + 4], rc.a);
    RwIm2DVertexSetIntRGBA(&m_pVertices[m_uVertexCount + 8], 0xFF, 0xFF, 0xFF, 255.0f * rc.a);
    RwIm2DVertexSetIntRGBA(&m_pVertices[m_uVertexCount + 9], 0xFF, 0xFF, 0xFF, 255.0f * rc.a);

    if (nWholeTris < 8) {
        F32 fInterpAmount = fRangeToEight - nWholeTris;
        F32 xCoord, yCoord, uCoord, vCoord;
        S32 vertexIndex0 = m_uVertexCount + nWholeTris;
        S32 vertexIndex1 = m_uVertexCount + nWholeTrisPlus1;
        S32 vertexIndex2 = m_uVertexCount + nWholeTrisPlus2;

        if (nWholeTris & 1) {
            xCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].x - m_pVertices[vertexIndex0].x) + (m_pVertices[vertexIndex2].x + m_pVertices[vertexIndex0].x));
            yCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].y - m_pVertices[vertexIndex0].y) + (m_pVertices[vertexIndex2].y + m_pVertices[vertexIndex0].y));
            uCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].u - m_pVertices[vertexIndex0].u) + (m_pVertices[vertexIndex2].u + m_pVertices[vertexIndex0].u));
            vCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].v - m_pVertices[vertexIndex0].v) + (m_pVertices[vertexIndex2].v + m_pVertices[vertexIndex0].v));
        } else {
            xCoord = fInterpAmount * m_pVertices[vertexIndex2].x + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].x;
            yCoord = fInterpAmount * m_pVertices[vertexIndex2].y + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].y;
            uCoord = fInterpAmount * m_pVertices[vertexIndex2].u + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].u;
            vCoord = fInterpAmount * m_pVertices[vertexIndex2].v + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].v;
        }

        for (S32 VertIndex = 9; VertIndex >= nWholeTrisPlus2; VertIndex--)
            VertSetXYUV(m_pVertices[m_uVertexCount + VertIndex], xCoord, yCoord, uCoord, vCoord, m_fRecipZ);
    }

    return true;
}

bool image_widget::render_half_ccw(U32 uLayer)
{
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fPercent = m_pLayers[uLayer].fCurPercent;

    F32 fClampTopY = rc.loc.y * FB_YRES + INSCREENY(rc.size.y) - m_pLayers[uLayer].fClampTopPercent * INSCREENY(rc.size.y);
    F32 fClampTopV = m_pLayers[uLayer].fClampTopPercent * (m_pLayers[uLayer].upperLeft.v - m_pLayers[uLayer].lowerRight.v) + m_pLayers[uLayer].lowerRight.v;

    F32 fHalfWidth = INSCREENX(0.5f * rc.size.x);
    F32 fHalfHeight = INSCREENY(0.5f * rc.size.y);
    F32 fCenterX = rc.loc.x * FB_XRES + fHalfWidth;
    F32 fCenterY = rc.loc.y * FB_YRES + fHalfHeight;
    F32 fCenterYPlusfHalfHeight = fCenterY + fHalfHeight;
    F32 fCenterYMinusfHalfHeight = fCenterY - fHalfHeight;

    fCenterY = fCenterY < fClampTopY ? fClampTopY : fCenterY;
    fCenterYPlusfHalfHeight = fCenterYPlusfHalfHeight < fClampTopY ? fClampTopY : fCenterYPlusfHalfHeight;
    fCenterYMinusfHalfHeight = fCenterYMinusfHalfHeight < fClampTopY ? fClampTopY : fCenterYMinusfHalfHeight;

    F32 u0 = m_pLayers[uLayer].upperLeft.u;
    F32 u1 = m_pLayers[uLayer].lowerRight.u;
    F32 fCenterU = 0.5f * (u0 + u1);

    F32 v0 = m_pLayers[uLayer].upperLeft.v;
    F32 v1 = m_pLayers[uLayer].lowerRight.v;
    F32 fCenterV = 0.5f * (v0 + v1);

    v0 = v0 < fClampTopV ? fClampTopV : v0;
    v1 = v1 < fClampTopV ? fClampTopV : v1;
    fCenterV = fCenterV < fClampTopV ? fClampTopV : fCenterV;

    F32 fRangeToFour = 4.0f * fPercent;
    S32 nWholeTris = fRangeToFour;
    S32 nWholeTrisPlus1 = nWholeTris + 1;
    S32 nWholeTrisPlus2 = nWholeTris + 2;

    VertSetXYUV(m_pVertices[m_uVertexCount + 11], fCenterX, fCenterYMinusfHalfHeight, fCenterU, v0, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 10], fCenterX + fHalfWidth, fCenterYMinusfHalfHeight, u1, v0, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 9], fCenterX + fHalfWidth, fCenterY, u1, fCenterV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 8], fCenterX + fHalfWidth, fCenterYPlusfHalfHeight, u1, v1, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 7], fCenterX, fCenterYPlusfHalfHeight, fCenterU, v1, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 6], fCenterX, fCenterY, fCenterU, fCenterV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 5], fCenterX, fCenterYPlusfHalfHeight, fCenterU, v1, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 4], fCenterX - fHalfWidth, fCenterYPlusfHalfHeight, u0, v1, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 3], fCenterX - fHalfWidth, fCenterY, u0, fCenterV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 2], fCenterX - fHalfWidth, fCenterYMinusfHalfHeight, u0, v0, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 1], fCenterX, fCenterYMinusfHalfHeight, fCenterU, v0, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount], fCenterX, fCenterY, fCenterU, fCenterV, m_fRecipZ);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount + 4], rc.a);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount + 8], rc.a);

    if (nWholeTris < 4) {
        F32 fInterpAmount = fRangeToFour - nWholeTris;

        U32 vertexIndex0, vertexIndex1, vertexIndex2;
        F32 xCoord[2], yCoord[2], uCoord[2], vCoord[2];
        for (U32 uHalfOffset = 0; uHalfOffset <= 1; uHalfOffset++) {
            U32 uHalfOffsetX6 = uHalfOffset * 6;
            vertexIndex0 = m_uVertexCount + nWholeTris + uHalfOffsetX6;
            vertexIndex1 = m_uVertexCount + nWholeTrisPlus1 + uHalfOffsetX6;
            vertexIndex2 = m_uVertexCount + nWholeTrisPlus2 + uHalfOffsetX6;

            if (nWholeTris & 1) {
                xCoord[uHalfOffset] = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].x - m_pVertices[vertexIndex0].x) + (m_pVertices[vertexIndex2].x + m_pVertices[vertexIndex0].x));
                yCoord[uHalfOffset] = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].y - m_pVertices[vertexIndex0].y) + (m_pVertices[vertexIndex2].y + m_pVertices[vertexIndex0].y));
                uCoord[uHalfOffset] = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].u - m_pVertices[vertexIndex0].u) + (m_pVertices[vertexIndex2].u + m_pVertices[vertexIndex0].u));
                vCoord[uHalfOffset] = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].v - m_pVertices[vertexIndex0].v) + (m_pVertices[vertexIndex2].v + m_pVertices[vertexIndex0].v));
            } else {
                xCoord[uHalfOffset] = fInterpAmount * m_pVertices[vertexIndex2].x + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].x;
                yCoord[uHalfOffset] = fInterpAmount * m_pVertices[vertexIndex2].y + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].y;
                uCoord[uHalfOffset] = fInterpAmount * m_pVertices[vertexIndex2].u + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].u;
                vCoord[uHalfOffset] = fInterpAmount * m_pVertices[vertexIndex2].v + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].v;
            }

            for (U32 VertIndex = uHalfOffsetX6 + 5; VertIndex >= uHalfOffsetX6 + nWholeTrisPlus2; VertIndex--)
                VertSetXYUV(m_pVertices[m_uVertexCount + VertIndex], xCoord[uHalfOffset], yCoord[uHalfOffset], uCoord[uHalfOffset], vCoord[uHalfOffset], m_fRecipZ);
        }
    }

    return true;
}

bool image_widget::render_rotate_ccw(U32 uLayer)
{
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fRadians = 2.0f * PI * -m_pLayers[uLayer].fCurPercent;

    F32 fTopU = m_pLayers[uLayer].upperLeft.u;
    F32 fTopV = m_pLayers[uLayer].upperLeft.v;
    F32 fBotU = m_pLayers[uLayer].lowerRight.u;
    F32 fBotV = m_pLayers[uLayer].lowerRight.v;

    F32 fTopX = rc.loc.x * FB_XRES;
    F32 fTopY = rc.loc.y * FB_YRES;

    F32 fCenterX = fTopX + m_pLayers[uLayer].fOffsetX;
    F32 fCenterY = fTopY + m_pLayers[uLayer].fOffsetY;

    F32 fBotX = fTopX + INSCREENX(rc.size.x);
    F32 fBotY = fTopY + INSCREENY(rc.size.y);

    xVec2 toTopLeft = { (fTopX - fCenterX) * m_pLayers[uLayer].fScaleX, (fTopY - fCenterY) * m_pLayers[uLayer].fScaleY };
    xVec2 toBotLeft = { (fTopX - fCenterX) * m_pLayers[uLayer].fScaleX, (fBotY - fCenterY) * m_pLayers[uLayer].fScaleY };
    xVec2 toTopRight = { (fBotX - fCenterX) * m_pLayers[uLayer].fScaleX, (fTopY - fCenterY) * m_pLayers[uLayer].fScaleY };
    xVec2 toBotRight = { (fBotX - fCenterX) * m_pLayers[uLayer].fScaleX, (fBotY - fCenterY) * m_pLayers[uLayer].fScaleY };
    F32 fSin = isin(fRadians);
    F32 fCos = icos(fRadians);

    F32 fTemp10 = toTopLeft.x * fCos - toTopLeft.y * fSin;
    F32 fTemp20 = toTopLeft.x * fSin + toTopLeft.y * fCos;
    toTopLeft.x = fTemp10;
    toTopLeft.y = fTemp20;

    fTemp10 = toBotLeft.x * fCos - toBotLeft.y * fSin;
    fTemp20 = toBotLeft.x * fSin + toBotLeft.y * fCos;
    toBotLeft.x = fTemp10;
    toBotLeft.y = fTemp20;

    fTemp10 = toTopRight.x * fCos - toTopRight.y * fSin;
    fTemp20 = toTopRight.x * fSin + toTopRight.y * fCos;
    toTopRight.x = fTemp10;
    toTopRight.y = fTemp20;

    fTemp10 = toBotRight.x * fCos - toBotRight.y * fSin;
    fTemp20 = toBotRight.x * fSin + toBotRight.y * fCos;
    toBotRight.x = fTemp10;
    toBotRight.y = fTemp20;

    VertSetXYUV(m_pVertices[m_uVertexCount], fCenterX + toTopLeft.x, fCenterY + toTopLeft.y, fTopU, fTopV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 1], fCenterX + toTopRight.x, fCenterY + toTopRight.y, fBotU, fTopV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 2], fCenterX + toBotLeft.x, fCenterY + toBotLeft.y, fTopU, fBotV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 3], fCenterX + toBotRight.x, fCenterY + toBotRight.y, fBotU, fBotV, m_fRecipZ);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a * m_pLayers[uLayer].fAlpha);
    return true;
}

bool image_widget::render_offset_rotate_ccw(U32 uLayer)
{
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fRadians = 2.0f * PI * -m_pLayers[uLayer].fCurPercent;

    F32 fTopU = m_pLayers[uLayer].upperLeft.u;
    F32 fTopV = m_pLayers[uLayer].upperLeft.v;
    F32 fBotU = m_pLayers[uLayer].lowerRight.u;
    F32 fBotV = m_pLayers[uLayer].lowerRight.v;

    F32 fTopX = rc.loc.x * FB_XRES;
    F32 fTopY = rc.loc.y * FB_YRES;

    F32 fBotX = fTopX + INSCREENX(rc.size.x);
    F32 fBotY = fTopY + INSCREENY(rc.size.y);

    F32 fCenterX = 0.5f * (fTopX + fBotX);
    F32 fCenterY = 0.5f * (fTopY + fBotY);

    xVec2 toTopLeft = { (fTopX - fCenterX) * m_pLayers[uLayer].fScaleX, (fTopY - fCenterY) * m_pLayers[uLayer].fScaleY };
    xVec2 toBotLeft = { (fTopX - fCenterX) * m_pLayers[uLayer].fScaleX, (fBotY - fCenterY) * m_pLayers[uLayer].fScaleY };
    xVec2 toTopRight = { (fBotX - fCenterX) * m_pLayers[uLayer].fScaleX, (fTopY - fCenterY) * m_pLayers[uLayer].fScaleY };
    xVec2 toBotRight = { (fBotX - fCenterX) * m_pLayers[uLayer].fScaleX, (fBotY - fCenterY) * m_pLayers[uLayer].fScaleY };
    F32 fSin = isin(fRadians);
    F32 fCos = icos(fRadians);

    F32 fTemp10 = toTopLeft.x * fCos - toTopLeft.y * fSin;
    F32 fTemp20 = toTopLeft.x * fSin + toTopLeft.y * fCos;
    toTopLeft.x = fTemp10;
    toTopLeft.y = fTemp20;

    fTemp10 = toBotLeft.x * fCos - toBotLeft.y * fSin;
    fTemp20 = toBotLeft.x * fSin + toBotLeft.y * fCos;
    toBotLeft.x = fTemp10;
    toBotLeft.y = fTemp20;

    fTemp10 = toTopRight.x * fCos - toTopRight.y * fSin;
    fTemp20 = toTopRight.x * fSin + toTopRight.y * fCos;
    toTopRight.x = fTemp10;
    toTopRight.y = fTemp20;

    fTemp10 = toBotRight.x * fCos - toBotRight.y * fSin;
    fTemp20 = toBotRight.x * fSin + toBotRight.y * fCos;
    toBotRight.x = fTemp10;
    toBotRight.y = fTemp20;

    VertSetXYUV(m_pVertices[m_uVertexCount], fCenterX + toTopLeft.x + m_pLayers[uLayer].fOffsetX, fCenterY + toTopLeft.y + m_pLayers[uLayer].fOffsetY, fTopU, fTopV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 1], fCenterX + toTopRight.x + m_pLayers[uLayer].fOffsetX, fCenterY + toTopRight.y + m_pLayers[uLayer].fOffsetY, fBotU, fTopV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 2], fCenterX + toBotLeft.x + m_pLayers[uLayer].fOffsetX, fCenterY + toBotLeft.y + m_pLayers[uLayer].fOffsetY, fTopU, fBotV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 3], fCenterX + toBotRight.x + m_pLayers[uLayer].fOffsetX, fCenterY + toBotRight.y + m_pLayers[uLayer].fOffsetY, fBotU, fBotV, m_fRecipZ);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a * m_pLayers[uLayer].fAlpha);
    return true;
}

bool image_widget::render_offset_pie(U32 uLayer)
{
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 u0 = m_pLayers[uLayer].upperLeft.u;
    F32 v0 = m_pLayers[uLayer].upperLeft.v;
    F32 u1 = m_pLayers[uLayer].lowerRight.u;
    F32 v1 = m_pLayers[uLayer].lowerRight.v;

    F32 fQuadWidth = (m_pLayers[uLayer].lowerRight.u - m_pLayers[uLayer].upperLeft.u) * get_texture_width();
    F32 fQuadHeight = (m_pLayers[uLayer].lowerRight.v - m_pLayers[uLayer].upperLeft.v) * get_texture_height();

    F32 fCenterX = m_pLayers[uLayer].fOffsetX + (rc.loc.x * FB_XRES + 0.5f * INSCREENX(rc.size.x));
    F32 fCenterY = m_pLayers[uLayer].fOffsetY + (rc.loc.y * FB_YRES + 0.5f * INSCREENY(rc.size.y));

    fQuadWidth *= m_pLayers[uLayer].fScaleX;
    fQuadHeight *= m_pLayers[uLayer].fScaleY;

    F32 fHalfWidth = 0.5f * fQuadWidth;
    F32 fHalfHeight = 0.5f * fQuadHeight;

    F32 fQuadX = fCenterX + m_pLayers[uLayer].fOffsetX - fHalfWidth;
    F32 fQuadY = fCenterY + m_pLayers[uLayer].fOffsetY - fHalfHeight;

    F32 fPercent = m_pLayers[uLayer].fCurPercent;

    F32 fCenterYPlusfHalfHeight = fCenterY + fHalfHeight;
    F32 fCenterYMinusfHalfHeight = fCenterY - fHalfHeight;
    F32 fCenterU = 0.5f * (u0 + u1);
    F32 fCenterV = 0.5f * (v0 + v1);

    F32 fRangeToEight = 8.0f * fPercent;
    S32 nWholeTris = fRangeToEight;
    S32 nWholeTrisPlus1 = nWholeTris + 1;
    S32 nWholeTrisPlus2 = nWholeTris + 2;

    switch (nWholeTrisPlus1) {
    case 9:
    case 8:
        VertSetXYUV(m_pVertices[m_uVertexCount + 9], fCenterX, fCenterYMinusfHalfHeight, fCenterU, v0, m_fRecipZ);
    case 7:
        VertSetXYUV(m_pVertices[m_uVertexCount + 8], fCenterX + fHalfWidth, fCenterYMinusfHalfHeight, u1, v0, m_fRecipZ);
    case 6:
        VertSetXYUV(m_pVertices[m_uVertexCount + 7], fCenterX + fHalfWidth, fCenterY, u1, fCenterV, m_fRecipZ);
    case 5:
        VertSetXYUV(m_pVertices[m_uVertexCount + 6], fCenterX + fHalfWidth, fCenterYPlusfHalfHeight, u1, v1, m_fRecipZ);
    case 4:
        VertSetXYUV(m_pVertices[m_uVertexCount + 5], fCenterX, fCenterYPlusfHalfHeight, fCenterU, v1, m_fRecipZ);
    case 3:
        VertSetXYUV(m_pVertices[m_uVertexCount + 4], fCenterX - fHalfWidth, fCenterYPlusfHalfHeight, u0, v1, m_fRecipZ);
    case 2:
        VertSetXYUV(m_pVertices[m_uVertexCount + 3], fCenterX - fHalfWidth, fCenterY, u0, fCenterV, m_fRecipZ);
    case 1:
        VertSetXYUV(m_pVertices[m_uVertexCount + 2], fCenterX - fHalfWidth, fCenterYMinusfHalfHeight, u0, v0, m_fRecipZ);
        VertSetXYUV(m_pVertices[m_uVertexCount + 1], fCenterX, fCenterYMinusfHalfHeight, fCenterU, v0, m_fRecipZ);
        VertSetXYUV(m_pVertices[m_uVertexCount], fCenterX, fCenterY, fCenterU, fCenterV, m_fRecipZ);
        break;
    }

    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount + 4], rc.a);
    RwIm2DVertexSetIntRGBA(&m_pVertices[m_uVertexCount + 8], 0xFF, 0xFF, 0xFF, 255.0f * rc.a);
    RwIm2DVertexSetIntRGBA(&m_pVertices[m_uVertexCount + 9], 0xFF, 0xFF, 0xFF, 255.0f * rc.a);

    if (nWholeTris < 8) {
        F32 fInterpAmount = fRangeToEight - nWholeTris;
        F32 xCoord, yCoord, uCoord, vCoord;
        S32 vertexIndex0 = m_uVertexCount + nWholeTris;
        S32 vertexIndex1 = m_uVertexCount + nWholeTrisPlus1;
        S32 vertexIndex2 = m_uVertexCount + nWholeTrisPlus2;

        if (nWholeTris & 1) {
            xCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].x - m_pVertices[vertexIndex0].x) + (m_pVertices[vertexIndex2].x + m_pVertices[vertexIndex0].x));
            yCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].y - m_pVertices[vertexIndex0].y) + (m_pVertices[vertexIndex2].y + m_pVertices[vertexIndex0].y));
            uCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].u - m_pVertices[vertexIndex0].u) + (m_pVertices[vertexIndex2].u + m_pVertices[vertexIndex0].u));
            vCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].v - m_pVertices[vertexIndex0].v) + (m_pVertices[vertexIndex2].v + m_pVertices[vertexIndex0].v));
        } else {
            xCoord = fInterpAmount * m_pVertices[vertexIndex2].x + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].x;
            yCoord = fInterpAmount * m_pVertices[vertexIndex2].y + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].y;
            uCoord = fInterpAmount * m_pVertices[vertexIndex2].u + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].u;
            vCoord = fInterpAmount * m_pVertices[vertexIndex2].v + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].v;
        }

        for (S32 VertIndex = 9; VertIndex >= nWholeTrisPlus2; VertIndex--)
            VertSetXYUV(m_pVertices[m_uVertexCount + VertIndex], xCoord, yCoord, uCoord, vCoord, m_fRecipZ);
    }

    return true;
}

bool image_widget::render_strobe_effect(U32 uLayer)
{
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA)
        return false;
    if (m_pLayers[uLayer].nStrobeCount == 0) {
        return false;
    }

    F32 fRealBottomY = INSCREENY(rc.size.y);
    F32 fVirtualBottomY = fRealBottomY + m_pLayers[uLayer].fStrobeHeight;
    F32 fStrobeRangePixels = (m_pLayers[uLayer].fMaxPercent - m_pLayers[uLayer].fMinPercent) * INSCREENY(rc.size.y) + m_pLayers[uLayer].fStrobeHeight;
    F32 fBottomY = fVirtualBottomY - fStrobeRangePixels * m_pLayers[uLayer].fStrobe;
    F32 fTopY = fBottomY - m_pLayers[uLayer].fStrobeHeight;
    if (fBottomY > fRealBottomY) {
        fBottomY = fRealBottomY;
    }

    F32 fRealTopY = (1.0f - m_pLayers[uLayer].fCurPercent) * INSCREENY(rc.size.y);
    if (fRealTopY >= fBottomY) {
        fTopY = fBottomY;
    }
    else if (fTopY < fRealTopY) {
        fTopY = fRealTopY;
    }

    F32 fTopYPercent = fTopY / INSCREENY(rc.size.y);
    F32 fBottomYPercent = fBottomY / INSCREENY(rc.size.y);

    F32 vRange = m_pLayers[uLayer].lowerRight.v - m_pLayers[uLayer].upperLeft.v;
    F32 v0 = fTopYPercent * vRange + m_pLayers[uLayer].upperLeft.v;
    F32 v1 = fBottomYPercent * vRange + m_pLayers[uLayer].upperLeft.v;
    F32 u0 = m_pLayers[uLayer].upperLeft.u;
    F32 u1 = m_pLayers[uLayer].lowerRight.u;

    fTopY = rc.loc.y * FB_YRES + fTopY;
    fBottomY = rc.loc.y * FB_YRES + fBottomY;

    F32 fTopX = rc.loc.x * FB_XRES;
    F32 fWidth = INSCREENX(rc.size.x);

    SetFourVertsToQuad(&m_pVertices[m_uVertexCount], fTopX, fTopY, fWidth, fBottomY - fTopY, u0, v0, u1, v1, m_fRecipZ);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a * m_pLayers[uLayer].fAlpha);
    return true;
}

bool image_widget::render_special_strobe(U32 uLayer)
{
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fRealBottomY = INSCREENY(rc.size.y);
    F32 fVirtualBottomY = fRealBottomY + m_pLayers[uLayer].fStrobeHeight;
    F32 fStrobeRangePixels = (m_pLayers[uLayer].fMaxPercent - m_pLayers[uLayer].fMinPercent) * INSCREENY(rc.size.y) + m_pLayers[uLayer].fStrobeHeight;
    F32 fBottomY = fVirtualBottomY - fStrobeRangePixels * m_pLayers[uLayer].fStrobe;
    F32 fTopY = fBottomY - m_pLayers[uLayer].fStrobeHeight;
    if (fBottomY > fRealBottomY) {
        fBottomY = fRealBottomY;
    }

    F32 fRealTopY = 0.0f;
    if (fTopY < fRealTopY) {
        fTopY = fRealTopY;
    }

    F32 fTopYPercent = fTopY / INSCREENY(rc.size.y);
    F32 fBottomYPercent = fBottomY / INSCREENY(rc.size.y);

    F32 v0 = m_pLayers[uLayer].upperLeft.v;
    F32 v1 = m_pLayers[uLayer].lowerRight.v;
    F32 u0 = m_pLayers[uLayer].upperLeft.u;
    F32 u1 = m_pLayers[uLayer].lowerRight.u;

    fTopY = rc.loc.y * FB_YRES + fTopY;
    fBottomY = rc.loc.y * FB_YRES + fBottomY;

    F32 fTopX = rc.loc.x * FB_XRES;
    F32 fWidth = INSCREENX(rc.size.x);

    SetFourVertsToQuad(&m_pVertices[m_uVertexCount], fTopX, fTopY, fWidth, fBottomY - fTopY, u0, v0, u1, v1, m_fRecipZ);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a * m_pLayers[uLayer].fAlpha);
    return true;
}

bool image_widget::render_random_v(U32 uLayer)
{
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fTopX = rc.loc.x * FB_XRES;
    F32 fTopY = rc.loc.y * FB_YRES;
    F32 fHeight = INSCREENY(rc.size.y);
    F32 fWidth = INSCREENX(rc.size.x) - 1.0f;
    F32 fDeltaV = fWidth / get_texture_height();
    F32 fTopU = m_pLayers[uLayer].upperLeft.u;
    F32 fTopV = xrand_RandomFloatRange(m_pLayers[uLayer].upperLeft.v, m_pLayers[uLayer].lowerRight.v);

    F32 fBotU = m_pLayers[uLayer].lowerRight.u;
    F32 fBotV = fTopV + fDeltaV;
    SetFourVertsToQuad(&m_pVertices[m_uVertexCount], fTopX, fTopY, fWidth, fHeight, fTopU, fTopV, fBotU, fBotV, m_fRecipZ);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a * m_pLayers[uLayer].fAlpha);
    return true;
}

bool image_widget::init_layers(U16 uLayers, U16 nVertsMax, U16 nIndicesMax)
{
    m_pLayers = new (eMemStaticType_0, 0) LayerInfo[uLayers];
    m_uLayerCount = uLayers;
    m_uRenderLayerCount = uLayers;
    m_VertexMax = nVertsMax;
    m_IndexMax = nIndicesMax;

    for (U32 i = 0; i < m_uLayerCount; i++) {
        LayerInfo* pLayer = &m_pLayers[i];
        memset(pLayer, 0, sizeof(LayerInfo));
        pLayer->fCurPercent = 0.0f;
        pLayer->fMaxPercent = 1.0f;
        pLayer->fClampTopPercent = 1.0f;
        pLayer->fScaleX = 1.0f;
        pLayer->fScaleY = 1.0f;
        pLayer->fAlpha = 1.0f;
    }

    return m_pLayers != NULL;
}

bool image_widget::set_layers_to_render(U16 uLayers)
{
    if (!m_pLayers || uLayers > m_uLayerCount) {
        return false;
    }

    m_uRenderLayerCount = uLayers;
    return true;
}

bool image_widget::set_layer_minmax(U32 whichLayer, F32 fMinPercent, F32 fMaxPercent)
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return false;
    }

    m_pLayers[whichLayer].fMinPercent = CLAMP(fMinPercent, 0.0f, 1.0f);
    m_pLayers[whichLayer].fMaxPercent = CLAMP(fMaxPercent, 0.0f, 1.0f);
    return true;
}

bool image_widget::set_layer_value(U32 whichLayer, F32 fVal)
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return false;
    }

    if (fVal <= 0.0f) {
        m_pLayers[whichLayer].fCurPercent = m_pLayers[whichLayer].fMinPercent;
    } else if (fVal >= 1.0f) {
        m_pLayers[whichLayer].fCurPercent = m_pLayers[whichLayer].fMaxPercent;
    } else {
        m_pLayers[whichLayer].fCurPercent = (1.0f - fVal) * m_pLayers[whichLayer].fMinPercent + fVal * m_pLayers[whichLayer].fMaxPercent;
    }

    return true;
}

F32 image_widget::get_layer_value(U32 whichLayer) const
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return 0.0f;
    }

    return m_pLayers[whichLayer].fCurPercent;
}

bool image_widget::set_layer_clamp_top(U32 whichLayer, F32 fPercent)
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return false;
    }

    m_pLayers[whichLayer].fClampTopPercent = CLAMP(fPercent, 0.0f, 1.0f);
    return true;
}

F32 image_widget::get_layer_clamp_top(U32 whichLayer) const
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return 0.0f;
    }

    return m_pLayers[whichLayer].fClampTopPercent;
}

bool image_widget::set_layer_scale(U32 whichLayer, F32 fScaleX, F32 fScaleY)
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return false;
    }

    m_pLayers[whichLayer].fScaleX = MAX(fScaleX, 0.0f);
    m_pLayers[whichLayer].fScaleY = MAX(fScaleY, 0.0f);
    return true;
}

bool image_widget::set_layer_offset(U32 whichLayer, F32 fOffsetX, F32 fOffsetY)
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return false;
    }

    m_pLayers[whichLayer].fOffsetX = fOffsetX;
    m_pLayers[whichLayer].fOffsetY = fOffsetY;
    return true;
}

bool image_widget::set_layer_alpha(U32 whichLayer, F32 fAlpha)
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return false;
    }

    m_pLayers[whichLayer].fAlpha = fAlpha;
    return true;
}

F32 image_widget::get_layer_alpha(U32 whichLayer)
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return 0.0f;
    }

    return m_pLayers[whichLayer].fAlpha;
}

F32 image_widget::get_layer_max(U32 whichLayer) const
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return 0.0f;
    }

    return m_pLayers[whichLayer].fMaxPercent;
}

bool image_widget::set_layer_uvs(U32 whichLayer, RwTexCoords& upperLeft, RwTexCoords& lowerRight)
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return false;
    }

    m_pLayers[whichLayer].upperLeft = upperLeft;
    m_pLayers[whichLayer].lowerRight = lowerRight;
    return true;
}

bool image_widget::set_layer_type(U32 whichLayer, RENDERTYPE eType, LAYER_FLAGS uBlendFlags)
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return false;
    }

    m_pLayers[whichLayer].eType = eType;
    m_pLayers[whichLayer].uFlags |= uBlendFlags & BLEND_MASK;
    return true;
}

void image_widget::set_buffer_to_layer(RENDERTYPE eType)
{
    U16* pIndex = &m_paIndices[m_uIndexCount];

    switch (eType) {
    case TOPLEFT:
    case BOTTOMLEFT:
    case BOTTOMRIGHT:
    case TOPRIGHT:
    case OFFSET_CENTERPT:
    case STROBE_EFFECT:
    case ROTATE_CCW:
    case OFFSET_ROTATE_CCW:
    case VARIABLE_TOP:
    case VARIABLE_RIGHT:
    case RANDOM_V:
    case SPECIAL_STROBE:
        ADD_TRI(0, 2, 1);
        ADD_TRI(1, 2, 3);

        m_uVertexCount += 4;
        m_uIndexCount += 6;
        break;
    case OFFSET_PIE:
    case COUNTER_CLOCKWISE:
        ADD_TRI(0, 1, 2);
        ADD_TRI(0, 2, 3);
        ADD_TRI(0, 3, 4);
        ADD_TRI(0, 4, 5);
        ADD_TRI(0, 5, 6);
        ADD_TRI(0, 6, 7);
        ADD_TRI(0, 7, 8);
        ADD_TRI(0, 8, 9);

        m_uVertexCount += 10;
        m_uIndexCount += 24;
        break;
    case HALF_CCW:
        ADD_TRI(0, 1, 2);
        ADD_TRI(0, 2, 3);
        ADD_TRI(0, 3, 4);
        ADD_TRI(0, 4, 5);
        ADD_TRI(6, 7, 8);
        ADD_TRI(6, 8, 9);
        ADD_TRI(6, 9, 10);
        ADD_TRI(6, 10, 11);

        m_uVertexCount += 12;
        m_uIndexCount += 24;
        break;
    }
}

bool image_widget::set_layer_strobe_prop(U32 whichLayer, F32 fStrobeFreq, F32 fStrobeHeight, S32 nStrobeCount)
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return false;
    }
    if (m_pLayers[whichLayer].eType != STROBE_EFFECT && m_pLayers[whichLayer].eType != SPECIAL_STROBE) {
        return false;
    }

    m_pLayers[whichLayer].fStrobeFreq = fStrobeFreq;
    m_pLayers[whichLayer].fStrobeHeight = fStrobeHeight;
    m_pLayers[whichLayer].nStrobeCount = nStrobeCount;
    return true;
}

void image_widget::set_layer_strobe(U32 whichLayer, F32 fStrobe)
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return;
    }
    if (m_pLayers[whichLayer].eType != STROBE_EFFECT && m_pLayers[whichLayer].eType != SPECIAL_STROBE) {
        return;
    }

    m_pLayers[whichLayer].fStrobe = fStrobe;
}

F32 image_widget::get_layer_strobe(U32 whichLayer)
{
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return 0.0f;
    }
    if (m_pLayers[whichLayer].eType != STROBE_EFFECT && m_pLayers[whichLayer].eType != SPECIAL_STROBE) {
        return 0.0f;
    }

    return m_pLayers[whichLayer].fStrobe;
}

}

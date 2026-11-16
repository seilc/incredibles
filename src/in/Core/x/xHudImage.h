#ifndef XHUDIMAGE_H
#define XHUDIMAGE_H

#include "xHud.h"

#include <rwcore.h>

inline void VertSetXYUV(RwIm2DVertex& rVert, F32 x, F32 y, F32 u, F32 v, F32 recipZ)
{
    RwIm2DVertexSetScreenX(&rVert, x);
    RwIm2DVertexSetScreenY(&rVert, y);
    RwIm2DVertexSetU(&rVert, u, recipZ);
    RwIm2DVertexSetV(&rVert, v, recipZ);
}

inline void SetFourVertsAlpha(RwIm2DVertex* pVerts, F32 fAlphaPercent)
{
    U32 uAlpha = 255.0f * fAlphaPercent;
    RwIm2DVertexSetIntRGBA(&pVerts[0], 0xFF, 0xFF, 0xFF, uAlpha);
    RwIm2DVertexSetIntRGBA(&pVerts[1], 0xFF, 0xFF, 0xFF, uAlpha);
    RwIm2DVertexSetIntRGBA(&pVerts[2], 0xFF, 0xFF, 0xFF, uAlpha);
    RwIm2DVertexSetIntRGBA(&pVerts[3], 0xFF, 0xFF, 0xFF, uAlpha);
}

inline void SetFourVertsToQuad(RwIm2DVertex* pVerts, F32 fX, F32 fY, F32 fWidth, F32 fHeight, F32 u0, F32 v0, F32 u1, F32 v1,
                               F32 recipZ)
{
    F32 fRight = fX + fWidth;
    F32 fBottom = fY + fHeight;
    VertSetXYUV(pVerts[0], fX, fY, u0, v0, recipZ);
    VertSetXYUV(pVerts[1], fRight, fY, u1, v0, recipZ);
    VertSetXYUV(pVerts[2], fX, fBottom, u0, v1, recipZ);
    VertSetXYUV(pVerts[3], fRight, fBottom, u1, v1, recipZ);
}

namespace xhud {

struct image_asset;

class image_widget : public widget
{
public:
    enum RENDERTYPE
    {
        TOPLEFT,
        BOTTOMLEFT,
        BOTTOMRIGHT,
        TOPRIGHT,
        OFFSET_CENTERPT,
        OFFSET_PIE,
        STROBE_EFFECT,
        ROTATE_CCW,
        OFFSET_ROTATE_CCW,
        VARIABLE_TOP,
        VARIABLE_RIGHT,
        COUNTER_CLOCKWISE,
        HALF_CCW,
        RANDOM_V,
        SPECIAL_STROBE
    };

    // LayerInfo::uFlags
    enum LAYER_FLAGS
    {
        BLEND = 0x1,
        ADDITIVE = 0x2,
        SET_DEST_ALPHA = 0x4,
        USE_DEST_ALPHA = 0x8,
        BLEND_MASK = 0xF
    };

    struct widget_chunk;

    static void load(xBase& data, xDynAsset& asset, size_t asset_size);

    image_widget(const image_asset& a);

    void destruct();
    virtual void destroy();
    virtual U32 type() const;
    virtual bool is(U32 id) const;
    virtual void update(F32 dt);
    void update_strobe_effect(U32 uLayer, F32 dt);
    virtual void render();
    bool render_one_to_one(U32 uLayer, RENDERTYPE eType);
    bool render_variable_top(U32 uLayer);
    bool render_variable_right(U32 uLayer);
    bool render_counter_clockwise(U32 uLayer);
    bool render_half_ccw(U32 uLayer);
    bool render_rotate_ccw(U32 uLayer);
    bool render_offset_rotate_ccw(U32 uLayer);
    bool render_offset_pie(U32 uLayer);
    bool render_strobe_effect(U32 uLayer);
    bool render_special_strobe(U32 uLayer);
    bool render_random_v(U32 uLayer);
    bool init_layers(U16 uLayers, U16 nVertsMax, U16 nIndicesMax);
    bool set_layers_to_render(U16 uLayers);
    bool set_layer_minmax(U32 whichLayer, F32 fMinPercent, F32 fMaxPercent);
    bool set_layer_value(U32 whichLayer, F32 fVal);
    F32 get_layer_value(U32 whichLayer) const;
    bool set_layer_clamp_top(U32 whichLayer, F32 fPercent);
    F32 get_layer_clamp_top(U32 whichLayer) const;
    bool set_layer_scale(U32 whichLayer, F32 fScaleX, F32 fScaleY);
    bool set_layer_offset(U32 whichLayer, F32 fOffsetX, F32 fOffsetY);
    bool get_layer_offset(U32 whichLayer, F32& fOffsetX, F32& fOffsetY);
    bool set_layer_alpha(U32 whichLayer, F32 fAlpha);
    F32 get_layer_alpha(U32 whichLayer);
    F32 get_layer_max(U32 whichLayer) const;
    bool set_layer_uvs(U32 whichLayer, RwTexCoords& upperLeft, RwTexCoords& lowerRight);
    bool set_layer_type(U32 whichLayer, RENDERTYPE eType, LAYER_FLAGS uBlendFlags);
    void set_buffer_to_layer(RENDERTYPE eType);
    bool set_layer_strobe_prop(U32 whichLayer, F32 fStrobeFreq, F32 fStrobeHeight, S32 nStrobeCount);
    void set_layer_strobe(U32 whichLayer, F32 fStrobe);
    F32 get_layer_strobe(U32 whichLayer);

    F32 get_texture_width()
    {
        return RwRasterGetWidth(m_pRaster);
    }

    F32 get_texture_height()
    {
        return RwRasterGetHeight(m_pRaster);
    }

protected:
    struct LayerInfo
    {
        RENDERTYPE eType;
        U32 uFlags;
        RwTexCoords upperLeft;
        RwTexCoords lowerRight;
        F32 fCurPercent;
        F32 fMinPercent;
        F32 fMaxPercent;
        F32 fClampTopPercent;
        F32 fOffsetX;
        F32 fOffsetY;
        F32 fScaleX;
        F32 fScaleY;
        F32 fAlpha;
        F32 fStrobe;
        F32 fStrobeFreq;
        F32 fStrobeHeight;
        S32 nStrobeCount;
    };

    RwRaster* m_pRaster;
    LayerInfo* m_pLayers;
    U32 m_uLayerCount;
    U32 m_uRenderLayerCount;
    RwIm2DVertex* m_pVertices;
    RwIm2DVertex* m_paVertices;
    U16* m_paIndices;
    U16 m_uVertexCount;
    U16 m_uIndexCount;
    F32 m_fRecipZ;
    U16 m_VertexMax;
    U16 m_IndexMax;
};

struct image_widget::widget_chunk : xBase
{
    image_widget w;
};

}

#endif

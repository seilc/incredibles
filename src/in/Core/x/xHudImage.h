#ifndef XHUDIMAGE_H
#define XHUDIMAGE_H

#include "xHud.h"

#include <rwcore.h>

namespace xhud
{
    class image_widget : public widget
    {
    public:
        enum RENDERTYPE
        {
            TOPLEFT,
            BOTTOMLEFT,
            BOTTOMRIGHT,
            TOPRIGHT,
            STROBE_EFFECT,
            OFFSET_CENTERPT,
            ROTATE_CCW,
            OFFSET_ROTATE_CCW,
            VARIABLE_TOP,
            VARIABLE_RIGHT,
            COUNTER_CLOCKWISE,
            HALF_CCW,
            RANDOM_V,
            SPECIAL_STROBE
        };

        enum LAYER_FLAGS
        {
            BLEND = 0x1,
            ADDITIVE = 0x2,
            SET_DEST_ALPHA = 0x4,
            USE_DEST_ALPHA = 0x8,
            BLEND_MASK = 0xf
        };

        bool init_layers(U16 uLayers, U16 nVertsMax, U16 nIndicesMax);
        bool set_layer_value(U32 whichLayer, F32 fVal);
        bool set_layer_offset(U32 whichLayer, F32 fOffsetX, F32 fOffsetY);
        bool set_layer_alpha(U32 whichLayer, F32 fAlpha);
        bool set_layer_uvs(U32 whichLayer, RwTexCoords& upperLeft, RwTexCoords& lowerRight);
        bool set_layer_type(U32 whichLayer, RENDERTYPE eType, LAYER_FLAGS uBlendFlags);

    private:
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
}; // namespace xhud

#endif
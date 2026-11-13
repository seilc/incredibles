#include "water/fxmain.h"

#include "iWater.h"
#include "water/fxembm.h"
#include "water/textures.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rpmatfx.h>
#include <rpskin.h>

#define NUM_EFFECTS (sizeof(EffectOpenFunctions) / sizeof(EffectOpenFunctions[0]))

RwFrame* FxFrameArray[NUM_FX_FRAMES] = { NULL, NULL };

static FxParam* (*EffectOpenFunctions[])() = { FxEMBMOpen };
static RwChar* EffectNames[NUM_EFFECTS];
static FxParam* Effects[NUM_EFFECTS];
static S32 EffectIndex;

static RpMaterial* MaterialSetupEffect(RpMaterial* material, void* data)
{
    RpMaterial* srcMaterial = (RpMaterial*)data;
    RpMultiTexture* srcMT, *dstMT;
    RwUInt32 numTextures;

    srcMT = RpMaterialGetMultiTexture(srcMaterial, rwID_GAMECUBE);
    numTextures = RpMultiTextureGetNumTextures(srcMT);

    RpMaterialCreateMultiTexture(material, rwID_GAMECUBE, numTextures);
    dstMT = RpMaterialGetMultiTexture(material, rwID_GAMECUBE);

    RpMultiTextureSetTexture(dstMT, 0, RpMaterialGetTexture(material));
    RpMultiTextureSetCoords(dstMT, 0, 0);
    for (RwUInt32 i = 1; i < numTextures; i++) {
        RpMultiTextureSetTexture(dstMT, i, RpMultiTextureGetTexture(srcMT, i));
        RpMultiTextureSetCoords(dstMT, i, 0);
    }

    RpMultiTextureSetEffect(dstMT, RpMultiTextureGetEffect(srcMT));

    RpGameCubeMTRenderCallBack preRender, postRender;
    void* cbData;
    RpMultiTextureGetGameCubeMTCallBacks(srcMT, &preRender, &postRender, &cbData);
    RpMultiTextureSetGameCubeMTCallBacks(dstMT, preRender, postRender, cbData);

    return NULL;
}

RpAtomic* AtomicSetupEffect(RpAtomic* atomic, void* data)
{
    RpMaterial* material = (RpMaterial*)data;
    RpGeometry* geom = RpAtomicGetGeometry(atomic);
    if (RpGeometryGetNumTexCoordSets(geom) > 0) {
        RpGeometryForAllMaterials(geom, MaterialSetupEffect, material);
        RpMatFXAtomicEnableEffects(atomic);
        if (RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic))) {
            RpSkinAtomicSetType(atomic, rpSKINTYPEMATFX);
        }
    }

    return atomic;
}

S32 FxOpen()
{
    FxFrameArray[0] = RpLightGetFrame(iWaterDirectionalLight);
    FxFrameArray[1] = NULL;
    RpGameCubeMTSetMiscFrameArray(FxFrameArray, NUM_FX_FRAMES);

    if (!TexturesOpen()) {
        return 0;
    }

    for (U32 i = 0; i < NUM_EFFECTS; i++) {
        Effects[i] =EffectOpenFunctions[i]();
        if (i == 0) {
            iWaterEMBM = Effects[i];
        }
        if (!Effects[i]) {
            return 0;
        }
        EffectNames[i] = RpMTEffectGetName(Effects[i]->effect);
    }

    Effects[EffectIndex]->set();

    return 1;
}

void FxClose()
{
    for (S32 i = 0; i < NUM_EFFECTS; i++) {
        if (Effects[i]->close) {
            Effects[i]->close();
        }
    }

    TexturesClose();
}

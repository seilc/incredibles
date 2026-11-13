#ifndef RPMATFX_H
#define RPMATFX_H

#include "rwcore.h"
#include "rpworld.h"

typedef struct RpMTEffect RpMTEffect;

typedef struct RpMultiTexture RpMultiTexture;

typedef RpMaterial* (*RpGameCubeMTRenderCallBack)(RpMaterial* material, void* object, RxGameCubePipeData* pipeData, void* data);

enum RpMatFXMaterialFlags
{
    rpMATFXEFFECTNULL,
    rpMATFXEFFECTBUMPMAP,
    rpMATFXEFFECTENVMAP,
    rpMATFXEFFECTBUMPENVMAP,
    rpMATFXEFFECTDUAL,
    rpMATFXEFFECTUVTRANSFORM,
    rpMATFXEFFECTDUALUVTRANSFORM,
    rpMATFXEFFECTMAX,
    rpMATFXNUMEFFECTS = 6,
    rpMATFXFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpMatFXMaterialFlags RpMatFXMaterialFlags;

struct RpMTEffect
{
    RwPlatformID platformID;
    RwUInt32 refCount;
    RwChar name[32];
    RwLLLink dictLink;
};

#ifdef __cplusplus
extern "C" {
#endif

void RpGameCubeMTSetMiscFrameArray(RwFrame** frames, RwUInt32 numFrames);
RwChar* RpMTEffectGetName(RpMTEffect* effect);
RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* atomic);
RpMatFXMaterialFlags RpMatFXMaterialGetEffects(const RpMaterial* material);
RpMaterial* RpMatFXMaterialSetBumpMapCoefficient(RpMaterial* material, RwReal coef);
RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material, RpMatFXMaterialFlags flags);
RpMaterial* RpMatFXMaterialSetEnvMapCoefficient(RpMaterial* material, RwReal coef);
RpMaterial* RpMatFXMaterialSetUVTransformMatrices(RpMaterial* material, RwMatrix* baseTransform, RwMatrix* dualTransform);
RpMaterial* RpMatFXMaterialSetupBumpMap(RpMaterial* material, RwTexture* texture, RwFrame* frame, RwReal coef);
RpMaterial* RpMatFXMaterialSetupDualTexture(RpMaterial* material, RwTexture* texture, RwBlendFunction srcBlendMode, RwBlendFunction dstBlendMode);
RpMaterial* RpMatFXMaterialSetupEnvMap(RpMaterial* material, RwTexture* texture, RwFrame* frame, RwBool useFrameBufferAlpha, RwReal coef);
RpMaterial* RpMaterialCreateMultiTexture(RpMaterial* material, RwPlatformID platformID, RwUInt32 numTextures);
RpMultiTexture* RpMaterialGetMultiTexture(const RpMaterial* material, RwPlatformID platformID);
RpMTEffect* RpMultiTextureGetEffect(const RpMultiTexture* multiTexture);
RpMultiTexture* RpMultiTextureGetGameCubeMTCallBacks(RpMultiTexture* multiTexture, RpGameCubeMTRenderCallBack* preRenderCallBack, RpGameCubeMTRenderCallBack* postRenderCallBack, void** callBackData);
RwUInt32 RpMultiTextureGetNumTextures(const RpMultiTexture* multiTexture);
RwTexture* RpMultiTextureGetTexture(const RpMultiTexture* multiTexture, RwUInt32 index);
RpMultiTexture* RpMultiTextureSetCoords(RpMultiTexture* multiTexture, RwUInt32 index, RwUInt32 texCoordIndex);
RpMultiTexture* RpMultiTextureSetEffect(RpMultiTexture* multiTexture, RpMTEffect* effect);
RpMultiTexture* RpMultiTextureSetGameCubeMTCallBacks(RpMultiTexture* multiTexture, RpGameCubeMTRenderCallBack preRenderCallBack, RpGameCubeMTRenderCallBack postRenderCallBack, void* callBackData);
RpMultiTexture* RpMultiTextureSetTexture(RpMultiTexture* multiTexture, RwUInt32 index, RwTexture* texture);

#ifdef __cplusplus
}
#endif

#endif

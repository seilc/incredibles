#ifndef RPMATFX_H
#define RPMATFX_H

#include "rwcore.h"
#include "rpworld.h"

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

#ifdef __cplusplus
extern "C" {
#endif

RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* atomic);
RpMatFXMaterialFlags RpMatFXMaterialGetEffects(const RpMaterial* material);
RpMaterial* RpMatFXMaterialSetBumpMapCoefficient(RpMaterial* material, RwReal coef);
RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material, RpMatFXMaterialFlags flags);
RpMaterial* RpMatFXMaterialSetEnvMapCoefficient(RpMaterial* material, RwReal coef);
RpMaterial* RpMatFXMaterialSetUVTransformMatrices(RpMaterial* material, RwMatrix* baseTransform, RwMatrix* dualTransform);
RpMaterial* RpMatFXMaterialSetupBumpMap(RpMaterial* material, RwTexture* texture, RwFrame* frame, RwReal coef);
RpMaterial* RpMatFXMaterialSetupDualTexture(RpMaterial* material, RwTexture* texture, RwBlendFunction srcBlendMode, RwBlendFunction dstBlendMode);
RpMaterial* RpMatFXMaterialSetupEnvMap(RpMaterial* material, RwTexture* texture, RwFrame* frame, RwBool useFrameBufferAlpha, RwReal coef);

#ifdef __cplusplus
}
#endif

#endif

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
RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material, RpMatFXMaterialFlags flags);

#ifdef __cplusplus
}
#endif

#endif

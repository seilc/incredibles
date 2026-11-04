#ifndef RPSKIN_H
#define RPSKIN_H

#include "rwcore.h"
#include "rpworld.h"
#include "rphanim.h"

typedef struct RpSkin RpSkin;

enum RpSkinType
{
    rpNASKINTYPE,
    rpSKINTYPEGENERIC,
    rpSKINTYPEMATFX,
    rpSKINTYPETOON,
    rpSKINTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpSkinType RpSkinType;

#ifdef __cplusplus
extern "C" {
#endif

RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic, RpHAnimHierarchy* hierarchy);
RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, RpSkinType type);
RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry);

#ifdef __cplusplus
}
#endif

#endif

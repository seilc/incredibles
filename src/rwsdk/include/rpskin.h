#ifndef RPSKIN_H
#define RPSKIN_H

#include "rwcore.h"
#include "rpworld.h"
#include "rphanim.h"

typedef struct RwMatrixWeights RwMatrixWeights;

typedef struct RpSkin RpSkin;

struct RwMatrixWeights
{
    RwReal w0;
    RwReal w1;
    RwReal w2;
    RwReal w3;
};

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

RpHAnimHierarchy* RpSkinAtomicGetHAnimHierarchy(const RpAtomic* atomic);
RpSkinType RpSkinAtomicGetType(RpAtomic* atomic);
RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic, RpHAnimHierarchy* hierarchy);
RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, RpSkinType type);
RpSkin* RpSkinCreate(RwUInt32 numVertices, RwUInt32 numBones, RwMatrixWeights* vertexWeights, RwUInt32* vertexIndices, RwMatrix* inverseMatrices);
RpSkin* RpSkinDestroy(RpSkin* skin);
RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry);
RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin);
RwUInt32 RpSkinGetNumBones(RpSkin* skin);
const RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* skin);
const RwUInt32* RpSkinGetVertexBoneIndices(RpSkin* skin);
const RwMatrixWeights* RpSkinGetVertexBoneWeights(RpSkin* skin);

#ifdef __cplusplus
}
#endif

#endif

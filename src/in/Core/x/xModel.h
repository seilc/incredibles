#ifndef XMODEL_H
#define XMODEL_H

#include "iModel.h"
#include "xAnim.h"
#include "xSurface.h"
#include "xLightKit.h"

#include <rwcore.h>
#include <rpworld.h>

struct xModelPipe
{
    U32 Flags;
    U8 Layer;
    U8 AlphaDiscard;
    U16 PipePad;
};

struct xModelPool;
struct xModelBucket;
template <class T> struct basic_rect;

struct xModelInstance
{
    xModelInstance* Next;
    xModelInstance* Parent;
    xModelPool* Pool;
    xAnimPlay* Anim;
    RpAtomic* Data;
    xModelPipe Pipe;
    U8 InFrustum;
    U8 TrueClip;
    S8 sortBias;
    U8 modelpad;
    F32 RedMultiplier;
    F32 GreenMultiplier;
    F32 BlueMultiplier;
    F32 Alpha;
    F32 FadeStart;
    F32 FadeEnd;
    xSurface* Surf;
    xModelBucket** Bucket;
    xModelInstance* BucketNext;
    xLightKit* LightKit;
    void* Object;
    U16 Flags;
    U8 BoneCount;
    U8 BoneIndex;
    U8* BoneRemap;
    RwMatrix* Mat;
    xVec3 Scale;
    xBox animBound;
    xBox combinedAnimBound;
    U32 modelID;
    U32 shadowID;
    RpAtomic* shadowmapAtomic;
    struct
    {
        xVec3* verts;
    } anim_coll;
};

struct xModelPool
{
    xModelPool* Next;
    U32 NumMatrices;
    xModelInstance* List;
};

struct xModelTag
{
    xVec3 v;
    U32 matidx;
    F32 wt[4];
};

struct xModelAssetInfo
{
    U32 Magic;
    U32 NumModelInst;
    U32 AnimTableID;
    U32 CombatID;
    U32 BrainID;
};

struct xModelAssetParam
{
    U32 HashID;
    U8 WordLength;
    U8 String[3];
};

void xModelUpdate(xModelInstance* modelInst, F32 timeDelta);
void xModelEval(xModelInstance* modelInst);
S32 xModelCullSingle(xModelInstance* minst);
void xModelSetLightKit(xModelInstance* minst, xLightKit* lightKit);
void xModelRender2D(const xModelInstance& model, const basic_rect<F32>& r, const xVec3& from, const xVec3& to);

xMat4x3* xModelGetFrame(xModelInstance* minst);

inline void xModelSetFrame(xModelInstance* model, const xMat4x3* frame)
{
    xMat4x3Copy((xMat4x3*)model->Mat, frame);
}

xModelInstance* xModelInstanceAllocReuse(RpAtomic* data, void* object, U16 flags, U8 boneIndex, U8* boneRemap, xModelInstance* reuse_instance);

inline xModelInstance* xModelInstanceAlloc(RpAtomic* data, void* object, U16 flags, U8 boneIndex, U8* boneRemap)
{
    return xModelInstanceAllocReuse(data, object, flags, boneIndex, boneRemap, NULL);
}

inline void xModelSetMaterialAlpha(xModelInstance* modelInst, U8 alpha)
{
    iModelSetMaterialAlpha(modelInst->Data, alpha);
}

inline void xModelMaterialMul(xModelInstance* modelInst, F32 rm, F32 gm, F32 bm)
{
    iModelMaterialMul(modelInst->Data, rm, gm, bm);
}

inline void xModelResetMaterial(xModelInstance* modelInst)
{
    iModelResetMaterial(modelInst->Data);
}

#endif
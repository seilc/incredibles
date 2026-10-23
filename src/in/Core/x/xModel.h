#ifndef XMODEL_H
#define XMODEL_H

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

xMat4x3* xModelGetFrame(xModelInstance* minst);

#endif
#include "xModelBucket.h"

#include "xModel.h"
#include "xGlobals.h"
#include "xCam.h"
#include "xMemMgr.h"
#include "xMath.h"
#include "xEnt.h"
#include "xRenderState.h"
#include "xFXHighDynamicRange.h"
#include "xutil.h"
#include "iModel.h"
#include "iCamera.h"
#include "iDraw.h"
#include "iAsync.h"

#include <rpmatfx.h>
#include <rpskin.h>
#include <stdlib.h>

struct xModelAlphaBucket
{
    RpAtomic* Data;
    xModelInstance* MInst;
    F32 AlphaFade;
    F32 SortValue;
    U32 Layer;
};

static RpWorld* sBucketDummyWorld;
static RwCamera* sBucketDummyCamera;
static S32 sBucketCurr;
static S32 sBucketCount;
static xModelBucket* sBucketList;
static S32 sBucketClipCullCurr;
static S32 sBucketClipCullCount;
static xModelBucket** sBucketClipCullList;
static S32 sAlphaCurr;
static S32 sAlphaStart;
static S32 sAlphaCount;
static xModelAlphaBucket* sAlphaList;

static S32 sBucketOpaqueRenderStart;

namespace {
    class CmpAlphaBucket
    {
    public:
        S32 operator()(const void* _a, const void* _b) const
        {
            const xModelAlphaBucket* a = (const xModelAlphaBucket*)_a;
            const xModelAlphaBucket* b = (const xModelAlphaBucket*)_b;

            if (a->Layer != b->Layer) {
                return a->Layer - b->Layer;
            }

            if (a->SortValue < b->SortValue) {
                return 1;
            }
            if (a->SortValue > b->SortValue) {
                return -1;
            }
            return 0;
        }
    };

    S32 compare_opaque_buckets(const void* e1, const void* e2)
    {
        const xModelBucket& b1 = *(const xModelBucket*)e1, &b2 = *(const xModelBucket*)e2;
        S32 use_layer1 = b1.Pipe.Flags & eALPHASORTDISABLE;
        S32 use_layer2 = b2.Pipe.Flags & eALPHASORTDISABLE;
        if (use_layer1 != use_layer2)
            return use_layer1 - use_layer2;
        if (b1.Pipe.Layer != b2.Pipe.Layer)
            return b1.Pipe.Layer - b2.Pipe.Layer;
        if (b1.Pipe.Flags != b2.Pipe.Flags)
            return b1.Pipe.Flags - b2.Pipe.Flags;
        if (b1.Pipe.AlphaDiscard != b2.Pipe.AlphaDiscard)
            return b1.Pipe.AlphaDiscard - b2.Pipe.AlphaDiscard;
        if (b1.OriginalData != b2.OriginalData) {
            return (U32)b1.OriginalData - (U32)b2.OriginalData;
        }

        return b2.ClipFlags - b1.ClipFlags;
    }
}

void xModelBucket_PreCountReset()
{
    sBucketCurr = 0;
    sBucketCount = 0;
    sBucketList = NULL;
    sBucketClipCullCurr = 0;
    sBucketClipCullCount = 0;
    sBucketClipCullList = NULL;
    sAlphaCurr = 0;
    sAlphaCount = 0;
    sAlphaList = NULL;
    sAlphaStart = 0;
}

void xModelBucket_PreCountBucket(RpAtomic* data, xModelPipe& pipe, U32 subObjects)
{
    U32 pipeFlags = (pipe.Flags & ~ePIPELINE_MASK) | ePIPELINE_CLIP;
    S32 numbuckets;

    switch (pipeFlags & ePIPELINE_MASK) {
    case ePIPELINE_CULL:
        numbuckets = 1;
        break;
    case ePIPELINE_CLIP:
        numbuckets = 1;
        break;
    case ePIPELINE_BOTH:
    case ePIPELINE_CLONE:
        numbuckets = 2;
        break;
    case ePIPELINE_DEFAULT:
    default:
        if (iModelNumBones(data)) {
            numbuckets = 1;
        } else {
            numbuckets = 2;
        }
    }

    S32 modelcount = 0;
    do {
        if (subObjects & 1) {
            modelcount++;
        }

        subObjects >>= 1;
        data = iModelFile_RWMultiAtomic(data);
    } while (data);

    sBucketCount += modelcount * numbuckets;
    sBucketClipCullCount += modelcount * 2;
}

void xModelBucket_PreCountAlloc(S32 maxAlphaModels)
{
    sAlphaCount = maxAlphaModels;
    sBucketList = (xModelBucket*)xMEMALLOC(sBucketCount * sizeof(xModelBucket), 0);
    sBucketClipCullList = (xModelBucket**)xMEMALLOC(sBucketClipCullCount * sizeof(xModelBucket*), 0);
    sAlphaList = (xModelAlphaBucket*)xMEMALLOC(sAlphaCount * sizeof(xModelAlphaBucket), 0);

    RwBBox bbox;
    bbox.sup.x = 100000.0f;
    bbox.sup.y = 100000.0f;
    bbox.sup.z = 100000.0f;
    bbox.inf.x = -100000.0f;
    bbox.inf.y = -100000.0f;
    bbox.inf.z = -100000.0f;
    iAsyncLockRW();
    sBucketDummyWorld = RpWorldCreate(&bbox);
    iAsyncUnlockRW();

    sBucketDummyCamera = iCameraCreate(0, 0, 0);

    iAsyncLockRW();
    RpWorldAddCamera(sBucketDummyWorld, sBucketDummyCamera);
    iAsyncUnlockRW();
}

static RpAtomic* GimmeAtomicCallback(RpAtomic* atomic, void* tmp)
{
    *(RpAtomic**)tmp = atomic;
    return atomic;
}

void FullAtomicDupe(RpAtomic* atomic, S32 count, RpAtomic** output)
{
    S32 i;
    RwStream* stream;
    RwMemory rwmemA = { NULL, 0 };
    RpAtomic* tempAtom;

    iAsyncLockRW();
    stream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, &rwmemA);
    if (stream) {
        RpClump* clump = RpAtomicGetClump(atomic);
        RwLinkList backupClump_atomicList = clump->atomicList;
        RwLLLink backupAtomic_inClumpLink = atomic->inClumpLink;
        clump->atomicList.link.next = &atomic->inClumpLink;
        clump->atomicList.link.prev = &atomic->inClumpLink;
        atomic->inClumpLink.next = &clump->atomicList.link;
        atomic->inClumpLink.prev = &clump->atomicList.link;
        RpClumpStreamWrite(clump, stream);
        clump->atomicList = backupClump_atomicList;
        atomic->inClumpLink = backupAtomic_inClumpLink;
        RwStreamClose(stream, &rwmemA);
    }
    iAsyncUnlockRW();

    for (i = 0; i < count; i++) {
        iAsyncLockRW();
        stream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &rwmemA);
        RwStreamFindChunk(stream, rwID_CLUMP, NULL, NULL);
        RpClump* tempClump = RpClumpStreamRead(stream);
        RwStreamClose(stream, &rwmemA);
        RpClumpForAllAtomics(tempClump, GimmeAtomicCallback, &tempAtom);
        iAsyncUnlockRW();

        iAsyncLockRW();
        RwMemory rwmemB = { NULL, 0 };
        stream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, &rwmemB);
        if (stream) {
            RpClumpStreamWrite(tempClump, stream);
            RwStreamClose(stream, &rwmemB);
        }
        stream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &rwmemB);
        if (stream) {
            RwStreamFindChunk(stream, rwID_CLUMP, NULL, NULL);
            RpClump* outClump = RpClumpStreamRead(stream);
            RpClumpForAllAtomics(outClump, GimmeAtomicCallback, &output[i]);
            RwStreamClose(stream, &rwmemB);
        } else {
            output[i] = NULL;
        }
        RwFree(rwmemB.start);
        _rwFrameSyncDirty();
        iModelUnload(tempAtom);
        if (output[i]) {
            RpHAnimHierarchy* pHier = iModelGetHierarchy(output[i]);
            RpSkin* pSkin = RpSkinGeometryGetSkin(output[i]->geometry);
            if (pHier && pSkin) {
                RpSkinAtomicSetHAnimHierarchy(output[i], pHier);
            }
        }
        iAsyncUnlockRW();
    }

    iAsyncLockRW();
    RwFree(rwmemA.start);

    iAsyncUnlockRW();
}

void xModelBucket_SetMaterialFX(RpAtomic* model, xModelPipe& pipe)
{
    if (pipe.Flags & eMATFX_MASK) {
        while (model) {
            iAsyncLockRW();

            RpMatFXAtomicEnableEffects(model);
            RpGeometry* geom = RpAtomicGetGeometry(model);

            bool skinned = RpSkinGeometryGetSkin(geom) != NULL;
            if (skinned) {
                RpSkinAtomicSetType(model, rpSKINTYPEMATFX);
            }

            S32 numMaterials = RpGeometryGetNumMaterials(geom);
            for (S32 i = 0; i < numMaterials; i++) {
                RpMaterial* material = RpGeometryGetMaterial(geom, i);

                if (pipe.Flags & ePIPEMATFX_UV2PANIM) {
                    RpMatFXMaterialSetEffects(material, rpMATFXEFFECTDUALUVTRANSFORM);
                } else {
                    RpMatFXMaterialSetEffects(material, rpMATFXEFFECTUVTRANSFORM);
                }
            }
            iAsyncUnlockRW();

            model = iModelFile_RWMultiAtomic(model);
        }
    }
}

void xModelBucket_InsertBucket(RpAtomic* data, xModelPipe& pipe, U32 subObjects)
{
    xModelBucket_SetMaterialFX(data, pipe);

    U32 pipeFlags = (pipe.Flags & ~ePIPELINE_MASK) | ePIPELINE_CLIP;
    S32 numbuckets;
    S32 pipeSetting[2];

    switch (pipeFlags & ePIPELINE_MASK) {
    case ePIPELINE_CULL:
        numbuckets = 1;
        pipeSetting[0] = 1;
        break;
    case ePIPELINE_CLIP:
        numbuckets = 1;
        pipeSetting[0] = 2;
        break;
    case ePIPELINE_BOTH:
    case ePIPELINE_CLONE:
        numbuckets = 2;
        pipeSetting[0] = 2;
        pipeSetting[1] = 1;
        break;
    case ePIPELINE_DEFAULT:
    default:
        if (iModelNumBones(data)) {
            numbuckets = 1;
            pipeSetting[0] = 1;
        } else {
            numbuckets = 2;
            pipeSetting[0] = 2;
            pipeSetting[1] = 1;
        }
    }

    RpAtomic* extraData = NULL;

    do {
        if (subObjects & 1) {
            sBucketClipCullList[sBucketClipCullCurr] = &sBucketList[sBucketCurr];
            sBucketClipCullList[sBucketClipCullCurr + 1] = &sBucketList[sBucketCurr];
            sBucketList[sBucketCurr].Data = data;
            sBucketList[sBucketCurr].OriginalData = data;
            sBucketList[sBucketCurr].BackRef = &sBucketClipCullList[sBucketClipCullCurr + 1];
            sBucketList[sBucketCurr].ClipFlags = pipeSetting[0];
            sBucketList[sBucketCurr].Pipe = pipe;
            if (numbuckets == 2) {
                FullAtomicDupe(data, 1, &extraData);
                sBucketList[sBucketCurr + 1].Data = extraData;
                sBucketList[sBucketCurr + 1].OriginalData = data;
                sBucketList[sBucketCurr + 1].BackRef = &sBucketClipCullList[sBucketClipCullCurr];
                sBucketList[sBucketCurr + 1].ClipFlags = pipeSetting[1];
                sBucketList[sBucketCurr + 1].Pipe = pipe;
                sBucketClipCullList[sBucketClipCullCurr] = &sBucketList[sBucketCurr + 1];
                extraData = NULL;
            }

            sBucketCurr += numbuckets;
            sBucketClipCullCurr += 2;
        }

        subObjects >>= 1;
        data = iModelFile_RWMultiAtomic(data);
    } while (data);
}

void xModelBucket_Init()
{
    if (sBucketDummyCamera) {
        iAsyncLockRW();
        RpWorldRemoveCamera(sBucketDummyWorld, sBucketDummyCamera);
        iAsyncUnlockRW();
        iCameraDestroy(sBucketDummyCamera);
        iAsyncLockRW();
        RpWorldDestroy(sBucketDummyWorld);
        iAsyncUnlockRW();
        sBucketDummyCamera = NULL;
        sBucketDummyWorld = NULL;
    }

    qsort(sBucketList, sBucketCount, sizeof(xModelBucket), compare_opaque_buckets);

    for (S32 i = 0; i < sBucketCount; i++) {
        xModelBucket* bucket = &sBucketList[i];
        xModelBucket** backref = bucket->BackRef;

        if (backref > sBucketClipCullList && backref[0] == backref[-1]) {
            backref[-1] = bucket;
        }

        backref[0] = bucket;

        bucket->List = NULL;
    }
}

xModelBucket** xModelBucket_GetBuckets(RpAtomic* data)
{
    for (S32 i = 0; i < sBucketClipCullCount; i += 2) {
        if (sBucketClipCullList[i]->OriginalData == data) {
            return &sBucketClipCullList[i];
        }
    }
    return NULL;
}

void xModelBucket_Begin()
{
    for (S32 i = 0; i < sBucketCount; i++) {
        sBucketList[i].List = NULL;
    }

    xModelBucketEnabled = TRUE;
}

void xModelBucket_Add(xModelInstance* minst)
{
    if ((minst->Flags & 0x401) != 0x1) {
        return;
    }

    RpAtomic* model = minst->Data;

    if (xModelCullSingle(minst)) {
        return;
    }

    xMat4x3* cammat = &xglobals->cam->mat;
    F32 camdist2 = xsqr(model->worldBoundingSphere.center.x - cammat->pos.x) +
                   xsqr(model->worldBoundingSphere.center.y - cammat->pos.y) +
                   xsqr(model->worldBoundingSphere.center.z - cammat->pos.z);

    if (camdist2 >= xsqr(minst->FadeEnd)) {
        return;
    }

    xModelBucket* bucket;
    F32 camdot = cammat->at.x * (model->worldBoundingSphere.center.x - cammat->pos.x) +
                 cammat->at.y * (model->worldBoundingSphere.center.y - cammat->pos.y) +
                 cammat->at.z * (model->worldBoundingSphere.center.z - cammat->pos.z);

    if (camdot < 1.5f * model->worldBoundingSphere.radius) {
        bucket = minst->Bucket[1];
    } else {
        bucket = minst->Bucket[0];
    }

    F32 alphaFade = 1.0f;
    if (camdist2 > xsqr(minst->FadeStart)) {
        alphaFade = (minst->FadeEnd - xsqrt(camdist2)) / (minst->FadeEnd - minst->FadeStart);
        if (alphaFade <= 0.0f) {
            return;
        }

        alphaFade = MIN(alphaFade, 1.0f);
    }

    bool alphaFlags = (minst->Pipe.Flags & (eSRCBLEND_MASK | eDESTBLEND_MASK)) != 0;
    bool alphaLOD = 1.0f != alphaFade;
    bool alphaModel = 1.0f != minst->Alpha;

    if (minst->Pipe.Flags & eALPHASORTDISABLE) {
        alphaFlags = false;
        alphaModel = false;
    }
    else if ((minst->Pipe.Flags & eFORCEOPAQUE_MASK) == ePIPEFORCEOPAQUE_BLEND)
        alphaFlags = false;
    else if ((minst->Pipe.Flags & eFORCEOPAQUE_MASK) == ePIPEFORCEOPAQUE_ALWAYS)
        alphaFlags = alphaLOD = alphaModel = false;

    bool alphaProg = alphaLOD || alphaModel;

    if (alphaFlags || alphaProg) {
        if (sAlphaCurr < sAlphaCount) {
            sAlphaList[sAlphaCurr].Data = bucket->Data;
            sAlphaList[sAlphaCurr].MInst = minst;
            sAlphaList[sAlphaCurr].AlphaFade = alphaFade;
            sAlphaList[sAlphaCurr].SortValue = camdot;
            sAlphaList[sAlphaCurr].SortValue += minst->sortBias / 32.0f;
            sAlphaList[sAlphaCurr].Layer = minst->Pipe.Layer;
            sAlphaCurr++;
        }
    } else {
        minst->BucketNext = bucket->List;
        bucket->List = minst;
    }
}

void xModelBucket_AddAlphaFade(xModelInstance* minst, F32 alphaFade)
{
    if (alphaFade <= 0.0f) {
        return;
    }

    if ((minst->Flags & 0x401) != 0x1) {
        return;
    }

    RpAtomic* model = minst->Data;

    if (xModelCullSingle(minst)) {
        return;
    }

    xMat4x3* cammat = &xglobals->cam->mat;
    F32 camdist2 = xsqr(model->worldBoundingSphere.center.x - cammat->pos.x) +
                   xsqr(model->worldBoundingSphere.center.y - cammat->pos.y) +
                   xsqr(model->worldBoundingSphere.center.z - cammat->pos.z);

    if (camdist2 >= xsqr(minst->FadeEnd)) {
        return;
    }

    xModelBucket* bucket;
    F32 camdot = cammat->at.x * (model->worldBoundingSphere.center.x - cammat->pos.x) +
                 cammat->at.y * (model->worldBoundingSphere.center.y - cammat->pos.y) +
                 cammat->at.z * (model->worldBoundingSphere.center.z - cammat->pos.z);

    if (camdot < 1.5f * model->worldBoundingSphere.radius) {
        bucket = minst->Bucket[1];
    } else {
        bucket = minst->Bucket[0];
    }

    bool alphaFlags = (minst->Pipe.Flags & (eSRCBLEND_MASK | eDESTBLEND_MASK)) != 0;
    bool alphaLOD = 1.0f != alphaFade;
    bool alphaModel = 1.0f != minst->Alpha;

    if (minst->Pipe.Flags & eALPHASORTDISABLE) {
        alphaFlags = false;
        alphaModel = false;
    }
    else if ((minst->Pipe.Flags & eFORCEOPAQUE_MASK) == ePIPEFORCEOPAQUE_BLEND)
        alphaFlags = false;
    else if ((minst->Pipe.Flags & eFORCEOPAQUE_MASK) == ePIPEFORCEOPAQUE_ALWAYS)
        alphaFlags = alphaLOD = alphaModel = false;

    bool alphaProg = alphaLOD || alphaModel;

    if (alphaFlags || alphaProg) {
        if (sAlphaCurr < sAlphaCount) {
            sAlphaList[sAlphaCurr].Data = bucket->Data;
            sAlphaList[sAlphaCurr].MInst = minst;
            sAlphaList[sAlphaCurr].AlphaFade = alphaFade;
            sAlphaList[sAlphaCurr].SortValue = camdot;
            sAlphaList[sAlphaCurr].SortValue += minst->sortBias / 32.0f;
            sAlphaList[sAlphaCurr].Layer = minst->Pipe.Layer;
            sAlphaCurr++;
        }
    } else {
        minst->BucketNext = bucket->List;
        bucket->List = minst;
    }
}

void xModelBucket_RenderOpaqueBegin()
{
    sBucketOpaqueRenderStart = 0;
}

void xModelBucket_RenderOpaque(S32 maxLayer)
{
    S32 i;

    xModelBucketEnabled = FALSE;

    for (i = sBucketOpaqueRenderStart; i < sBucketCount; i++) {
        xModelBucket* bucket = &sBucketList[i];
        RpAtomic* data = bucket->Data;
        xModelInstance* minst = bucket->List;

        if ((bucket->Pipe.Flags & eALPHASORTDISABLE) && bucket->Pipe.Layer > maxLayer) {
            break;
        }

        while (minst) {
            if (minst->Surf) {
                xEntSetupPipeline(minst->Surf, data);
            }

            RpAtomic* oldmodel = minst->Data;
            minst->Data = data;

            S32 flags = minst->Pipe.Flags;

            if ((flags & eCULL_MASK) == ePIPECULL_FRONTONLY) {
                RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
            }

            if (flags & eFOGDISABLE) {
                iCameraSetFog(false);
            }

            if ((flags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE) {
                RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
            }

            if (flags & eSRCBLEND_MASK) {
                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)((flags >> eSRCBLEND_SHIFT) & 0xf));
            }

            if (flags & eDESTBLEND_MASK) {
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)((flags >> eDESTBLEND_SHIFT) & 0xf));
            }

            if (flags & eBRIGHT) {
                xFXHighDynamicRangeBrighten(true, false);
            }

            xModelRenderSingle(minst);

            if (flags & eFOGDISABLE) {
                iCameraSetFog(true);
            }

            if ((flags & eCULL_MASK) == ePIPECULL_FRONTONLY) {
                RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
            }

            if ((flags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE) {
                RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
            }

            if (flags & eSRCBLEND_MASK) {
                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
            }

            if (flags & eDESTBLEND_MASK) {
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            }

            if (flags & eBRIGHT) {
                xFXHighDynamicRangeBrighten(false, false);
            }

            minst->Data = oldmodel;

            minst = minst->BucketNext;
        }

        bucket->List = NULL;
    }

    sBucketOpaqueRenderStart = i;
}

void xModelBucket_RenderAlphaBegin()
{
    xModelBucketEnabled = FALSE;
    sAlphaStart = 0;

    if (sAlphaCurr != 0) {
        quicksort(sAlphaList, sAlphaCurr, sizeof(xModelAlphaBucket), CmpAlphaBucket());
    }
}

inline void ValidateRenderStateExpectations(const xModelInstance*, U32)
{
}

static void xModelBucket_RenderAlphaLayerInternal(S32 maxLayer)
{
    U32 pipeMask, curPipeFlags, lastPipeFlags;
    U8 curAlphaDiscard, lastAlphaDiscard;
    RwFogType oldfogtype;
    S32 layer;
    xModelInstance* minst;

    pipeMask = 0xFFF3FF38;

    lastPipeFlags = (ePIPEBLEND_SRCALPHA << eSRCBLEND_SHIFT) | (ePIPEBLEND_INVSRCALPHA << eDESTBLEND_SHIFT);
    lastAlphaDiscard = 0;

    oldfogtype = xglobals->fog.type;
    if (!oldfogtype) {
        pipeMask &= ~eFOGDISABLE;
    }

    ValidateRenderStateExpectations(NULL, 0);

    while (sAlphaStart < sAlphaCurr) {
        layer = sAlphaList[sAlphaStart].Layer;
        if (layer > maxLayer) {
            break;
        }

        minst = sAlphaList[sAlphaStart].MInst;

        RpAtomic* oldmodel = minst->Data;
        minst->Data = sAlphaList[sAlphaStart].Data;

        if (minst->Surf) {
            xEntSetupPipeline(minst->Surf, minst->Data);
        }

        curPipeFlags = minst->Pipe.Flags;
        if (!(curPipeFlags & eSRCBLEND_MASK)) {
            curPipeFlags |= ePIPEBLEND_SRCALPHA << eSRCBLEND_SHIFT;
        }
        if (!(curPipeFlags & eDESTBLEND_MASK)) {
            curPipeFlags |= ePIPEBLEND_INVSRCALPHA << eDESTBLEND_SHIFT;
        }

        curPipeFlags &= pipeMask;

        curAlphaDiscard = minst->Pipe.AlphaDiscard;
        F32 oldAlpha = minst->Alpha;
        F32 fade = sAlphaList[sAlphaStart].AlphaFade;
        minst->Alpha *= fade;

        ValidateRenderStateExpectations(minst, lastPipeFlags);

        U32 xorPipeFlags = curPipeFlags ^ lastPipeFlags;
        bool fadeChanged = false;

        if (xorPipeFlags || fadeChanged) {
            if (xorPipeFlags & eSRCBLEND_MASK) {
                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)((curPipeFlags >> eSRCBLEND_SHIFT) & 0xf));
            }

            if (xorPipeFlags & eDESTBLEND_MASK) {
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)((curPipeFlags >> eDESTBLEND_SHIFT) & 0xf));
            }

            if (xorPipeFlags & eZBUFFER_MASK) {
                if ((curPipeFlags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE) {
                    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
                } else {
                    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
                }
            }

            if (xorPipeFlags & eCULL_MASK) {
                if ((curPipeFlags & eCULL_MASK) == ePIPECULL_FRONTONLY) {
                    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
                }
                else if ((curPipeFlags & eCULL_MASK) <= ePIPECULL_DOUBLESIDED) {
                    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
                }
            }

            if (xorPipeFlags & eFOGDISABLE) {
                if (curPipeFlags & eFOGDISABLE) {
                    iCameraSetFog(false);
                } else {
                    iCameraSetFog(true);
                }
            }

            if (xorPipeFlags & eBRIGHT) {
                xFXHighDynamicRangeBrighten((curPipeFlags & eBRIGHT) != 0, false);
            }

            lastPipeFlags = curPipeFlags;
        }

        if (curAlphaDiscard) {
            RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, curAlphaDiscard);
            GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, curAlphaDiscard);
            _rwDlRenderStateSetZCompLoc(FALSE);
            GXSetZCompLoc(GX_FALSE);
        } else {
            RwGameCubeSetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_ALWAYS, 0);
            GXSetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_ALWAYS, 0);
            _rwDlRenderStateSetZCompLoc(TRUE);
            GXSetZCompLoc(GX_TRUE);
        }
        lastAlphaDiscard = curAlphaDiscard;

        if (curPipeFlags & eBRIGHT) {
            xRenderFixUntexturedBegin(minst->Data);
        }

        if ((curPipeFlags & eCULL_MASK) == ePIPECULL_BACKTHENFRONT) {
            RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLFRONT);
            xModelRenderSingle(minst);
            RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
            xModelRenderSingle(minst);
        }
        else if ((curPipeFlags & eZBUFFER_MASK) == ePIPEZBUFFER_ZFIRST) {
            iDrawSetFBMSK(0xFFFFFFFF, 0x00FFFFFF, true);
            xModelRenderSingle(minst);
            iDrawSetFBMSK(0, 0x00FFFFFF, true);
            xModelRenderSingle(minst);
        } else {
            xModelRenderSingle(minst);
        }

        if (curPipeFlags & eBRIGHT) {
            xRenderFixUntexturedEnd(minst->Data);
        }

        minst->Alpha = oldAlpha;

        if (minst->Surf) {
            xEntRestorePipeline(minst->Surf, minst->Data);
        }

        minst->Data = oldmodel;

        sAlphaStart++;
    }

    if (lastPipeFlags != ((ePIPEBLEND_SRCALPHA << eSRCBLEND_SHIFT) | (ePIPEBLEND_INVSRCALPHA << eDESTBLEND_SHIFT))) {
        if (((lastPipeFlags >> eSRCBLEND_SHIFT) & 0xf) != ePIPEBLEND_SRCALPHA) {
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        }

        if (((lastPipeFlags >> eDESTBLEND_SHIFT) & 0xf) != ePIPEBLEND_INVSRCALPHA) {
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
        }

        if ((lastPipeFlags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE) {
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
        }

        if ((lastPipeFlags & eCULL_MASK) == ePIPECULL_FRONTONLY || (lastPipeFlags & eCULL_MASK) == ePIPECULL_BACKTHENFRONT) {
            RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
        }

        if (lastPipeFlags & eFOGDISABLE) {
            iCameraSetFog(true);
        }

        if (lastPipeFlags & eBRIGHT) {
            xFXHighDynamicRangeBrighten(false, false);
        }
    }

    RwGameCubeSetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_ALWAYS, 0);
    _rwDlRenderStateSetZCompLoc(TRUE);
    GXSetZCompLoc(GX_TRUE);
}

void xModelBucket_RenderAlphaLayer(S32 maxLayer)
{
    S32 lastLayer = 0;
    while (sAlphaStart < sAlphaCurr) {
        S32 layer = sAlphaList[sAlphaStart].Layer;
        if (layer > maxLayer)
            layer = maxLayer;
        if (layer == lastLayer && maxLayer != 31) {
            break;
        }
        xModelBucket_RenderOpaque(layer);
        xModelBucket_RenderAlphaLayerInternal(layer);
        lastLayer = layer;
    }
    if (lastLayer < maxLayer)
        xModelBucket_RenderOpaque(maxLayer);
}

bool xModelBucket_RenderAny(S32 maxLayer)
{
    if (sAlphaStart < sAlphaCurr && (S32)sAlphaList[sAlphaStart].Layer <= maxLayer)
        return true;
    return sBucketOpaqueRenderStart < sBucketCount && sBucketList[sBucketOpaqueRenderStart].Pipe.Layer <= maxLayer;
}

void xModelBucket_RenderAlphaEnd()
{
    sAlphaCurr = 0;
}

void xModelBucket_Deinit()
{
    for (S32 i = 0; i < sBucketCount; i++) {
        if (sBucketList[i].Data != sBucketList[i].OriginalData) {
            _rwFrameSyncDirty();
            iModelUnload(sBucketList[i].Data);
        }
    }

    xModelBucket_PreCountReset();
}

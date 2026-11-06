#include "xModel.h"

#include "xGlobals.h"
#include "xMath.h"
#include "xMath2.h"
#include "xMath3.h"
#include "xModelAsset.h"
#include "xMemMgr.h"
#include "xMemory.h"
#include "xMorph.h"
#include "xString.h"
#include "xstransvc.h"
#include "iCamera.h"
#include "iAsync.h"
#include "iModel.h"

RpMorphTarget anim_coll_old_mt;
static xModelPool* sxModelPoolList;
static RwCamera* subcamera;
S32 xModelPipeNumTables;
S32 xModelPipeCount[16];
xModelPipeTable* xModelPipeData[16];
S32 xModelLookupCount;
xModelPipeLookup* xModelLookupList;
S32 xModelInstStaticAlloc;
S32 xModelBucketEnabled;

xModelPipe modelDefaultPipe = { 0, 0, 0x80 };

xModelPipe xModelGetPipe(RpAtomic* model)
{
    for (S32 i = 0; i < xModelLookupCount; i++) {
        if (xModelLookupList[i].model == model) {
            return xModelLookupList[i].Pipe;
        }
    }
    return modelDefaultPipe;
}

static RwCamera* CameraCreate(S32 width, S32 height, S32 zBuffer);

void xModelInit()
{
    iModelInit();

    sxModelPoolList = NULL;

    xModelPipeNumTables = 0;
}

void xModelInitOnce()
{
    if (!subcamera) {
        subcamera = CameraCreate(0, 0, TRUE);
    }
}

void xModelPoolInit(U32 count, U32 numMatrices)
{
    S32 i;
    U8* buffer;
    RwMatrix* mat;
    xModelPool* pool;
    xModelPool* curr;
    xModelPool** prev;
    xModelInstance* inst;

    if (numMatrices < 1) {
        numMatrices = 1;
    }

    buffer = (U8*)xMEMALLOC(sizeof(xModelPool) + count * numMatrices * sizeof(RwMatrix) + count * sizeof(xModelInstance), 16);

    mat = (RwMatrix*)buffer; buffer += count * numMatrices * sizeof(RwMatrix);
    pool = (xModelPool*)buffer;
    buffer += sizeof(xModelPool);
    inst = (xModelInstance*)buffer;

    for (i = 0; i < (S32)count; i++) {
        inst[i].Next = &inst[i + 1];
        inst[i].Pool = pool;
        inst[i].Mat = mat;
        mat += numMatrices;
    }
    inst[count - 1].Next = NULL;

    pool->NumMatrices = numMatrices;
    pool->List = inst;

    prev = &sxModelPoolList;
    curr = sxModelPoolList;
    while (curr && numMatrices < curr->NumMatrices) {
        prev = &curr->Next;
        curr = curr->Next;
    }
    pool->Next = curr;
    *prev = pool;
}

bool xModelInstanceAvailable(S32 bones, S32 need)
{
    S32 mats = bones + 1;
    for (xModelPool* pool = sxModelPoolList; pool; pool = pool->Next) {
        if (mats <= (S32)pool->NumMatrices) {
            for (xModelInstance* m = pool->List; m; m = m->Next) {
                if (--need <= 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

xModelInstance* xModelInstanceAllocReuse(RpAtomic* data, void* object, U16 flags, U8 boneIndex, U8* boneRemap, xModelInstance* reuse_instance)
{
    S32 i;
    U32 boneCount, matCount;
    xModelPool* curr, *found = NULL;
    xModelInstance* dude;

    boneCount = iModelNumBones(data);
    matCount = boneCount + 1 + ((flags & ModelInstance_ChildTransform) != 0);

    if (reuse_instance) {
        dude = reuse_instance;
    } else {
        if (xModelInstStaticAlloc) {
            if (flags & 0x2000) {
                dude = (xModelInstance*)xMEMALLOC(sizeof(xModelInstance), 0);
                dude->Pool = NULL;
                dude->Mat = NULL;
            } else {
                RwMatrix* allocmats = (RwMatrix*)xMEMALLOC(matCount * sizeof(RwMatrix) + sizeof(xModelInstance), 16);
                dude = (xModelInstance*)(allocmats + matCount);
                dude->Pool = NULL;
                dude->Mat = allocmats;
            }
        } else {
            if (flags & 0x2000) {
                flags &= (U16)~0x2000;
                flags |= 0x8;
            }

            curr = sxModelPoolList;
            while (curr) {
                if (curr->List && matCount <= curr->NumMatrices) {
                    found = curr;
                }
                curr = curr->Next;
            }

            if (!found) {
                return NULL;
            }

            dude = found->List;
            found->List = dude->Next;
        }
    }

    dude->Next = NULL;
    dude->Parent = NULL;
    dude->Anim = NULL;
    dude->Data = data;
    dude->Object = object;
    dude->Flags = flags | 0x1 | 0x2;
    dude->BoneCount = boneCount;
    dude->BoneIndex = boneIndex;
    dude->BoneRemap = boneRemap;
    dude->modelID = 0;
    dude->shadowID = 0xDEADBEEF;

    dude->Scale.x = 0.0f;
    dude->Scale.y = 0.0f;
    dude->Scale.z = 0.0f;

    dude->RedMultiplier = 1.0f;
    dude->GreenMultiplier = 1.0f;
    dude->BlueMultiplier = 1.0f;

    dude->Alpha = data->geometry->matList.materials[0]->color.alpha / 255.0f;
    dude->Surf = NULL;
    dude->FadeStart = 9e37f;
    dude->FadeEnd = 1e38f;

    if (dude->Mat) {
        for (i = 0; i < (S32)matCount; i++)
            xMat4x3Identity((xMat4x3*)&dude->Mat[i]);
    }

    dude->Bucket = xModelBucket_GetBuckets(dude->Data);
    dude->LightKit = NULL;
    if (dude->Bucket) {
        dude->Pipe = dude->Bucket[0]->Pipe;
    } else {
        dude->Pipe = xModelGetPipe(dude->Data);
    }

    if (!dude->Pipe.Layer) {
        bool forceopaque = (dude->Pipe.Flags & eFORCEOPAQUE_MASK) != 0;
        bool nosrcblend = ((dude->Pipe.Flags >> eSRCBLEND_SHIFT) & 0xf) == 0;
        bool nodestblend = ((dude->Pipe.Flags >> eDESTBLEND_SHIFT) & 0xf) == 0;
        bool noreallywewantitthisway = forceopaque || (nosrcblend && nodestblend);
        if (!noreallywewantitthisway) {
            dude->Pipe.Layer = 19;
        }
    }

    dude->anim_coll.verts = NULL;
    dude->sortBias = 0;

    return dude;
}

static xModelInstance* FindChild(xModelInstance* inst)
{
    xModelInstance* curr = inst->Next;
    while (curr) {
        if (curr->Parent == inst)
            return curr;
        curr = curr->Next;
    }
    return NULL;
}

void xModelInstanceFree(xModelInstance* modelInst)
{
    xModelInstance* child;
    xModelInstance* curr;
    xModelInstance** prev;

    while (child = FindChild(modelInst)) {
        xModelInstanceFree(child);
    }

    if (modelInst->Parent) {
        curr = modelInst->Parent->Next; prev = &modelInst->Parent->Next;
        while (curr && curr != modelInst) {
            prev = &curr->Next;
            curr = curr->Next;
        }
        *prev = curr->Next;
    }

    if (modelInst->Anim) {
        xAnimPoolFree(modelInst->Anim);
        modelInst->Anim = NULL;
    }

    if (modelInst->Pool) {
        modelInst->Next = modelInst->Pool->List;
        modelInst->Pool->List = modelInst;
    }
}

void xModelInstanceAttach(xModelInstance* inst, xModelInstance* parent)
{
    xModelInstance* curr;

    curr = parent;
    while (curr->Next) {
        curr = curr->Next;
    }
    curr->Next = inst;
    inst->Parent = parent;

    if (inst->Flags & 0x2000) {
        inst->Mat = parent->Mat;
    }
}

void xModelUpdate(xModelInstance* modelInst, F32 timeDelta)
{
    while (modelInst) {
        if (modelInst->Anim && (modelInst->Flags & 0x4)) {
            if (modelInst->Flags & 0x100) {
                xAnimPlayChooseTransition(modelInst->Anim);
            }
            xAnimPlayUpdate(modelInst->Anim, timeDelta);
        }

        modelInst = modelInst->Next;
    }
}

void xModelEvalSingle(xModelInstance* modelInst, xBox* combinedAnimBound)
{
    S32 i;
    U16 flags = modelInst->Flags;

    if (flags & 0x2) {
        modelInst->Flags &= (U16)~0x1000;

        if (modelInst->Anim) {
            xAnimPlayEval(modelInst->Anim);
        }

        if (modelInst->Parent) {
            xModelInstance* dad = modelInst->Parent;

            if (flags & 0x8) {
                xMemoryCopyUpA128(modelInst->Mat, dad->Mat, (modelInst->BoneCount + 1) * sizeof(RwMatrix));
            } else if (flags & 0x10) {
                U8* remap = modelInst->BoneRemap;
                xMemoryCopyUpA128(modelInst->Mat, dad->Mat, sizeof(RwMatrix));

                for (i = 0; i < modelInst->BoneCount; i++) {
                    xMemoryCopyUpA128(&modelInst->Mat[i + 1], &dad->Mat[remap[i] + 1], sizeof(RwMatrix));
                }
            } else if (flags & 0x20) {
                if (dad->BoneCount && modelInst->BoneIndex) {
                    xMat4x3Mul((xMat4x3*)modelInst->Mat, (xMat4x3*)&dad->Mat[modelInst->BoneIndex + 1], (xMat4x3*)dad->Mat);
                } else {
                    xMemoryCopyUpA128(modelInst->Mat, dad->Mat, sizeof(RwMatrix));
                }

                if (flags & ModelInstance_ChildTransform) {
                    xMat4x3Mul((xMat4x3*)modelInst->Mat, (xMat4x3*)&modelInst->Mat[modelInst->BoneCount + 1], (xMat4x3*)modelInst->Mat);
                }
            }
        }
    }

    if ((flags & 0x1) && modelInst->BoneCount) {
        if (modelInst->Scale.x != 0.0f) {
            xMat4x3& mat = *(xMat4x3*)modelInst->Mat;

            xMat3x3 temp_mat = mat;
            xMat3x3MulScale(&mat, &mat, &modelInst->Scale);
            iModelAnimBoundWorld(&modelInst->animBound, combinedAnimBound, modelInst->Data, modelInst->Mat, modelInst->Mat + 1);

            (xMat3x3&)mat = temp_mat;
        } else {
            iModelAnimBoundWorld(&modelInst->animBound, combinedAnimBound, modelInst->Data, modelInst->Mat, modelInst->Mat + 1);
        }
    }
}

void xModelEval(xModelInstance* modelInst)
{
    xBox* combinedAnimBound = &modelInst->combinedAnimBound;
    combinedAnimBound->upper.x = -1e38f;

    while (modelInst) {
        xModelEvalSingle(modelInst, combinedAnimBound);
        modelInst = modelInst->Next;
    }
}

void xModelRenderSingle(xModelInstance* modelInst)
{
    if ((modelInst->Flags & 0x401) == 0x1) {
        bool reset = false;
        xMat3x3 tmpmat;

        if (modelInst->Scale.x) {
            xMat3x3Copy(&tmpmat, (xMat3x3*)modelInst->Mat);

            modelInst->Mat->right.x *= modelInst->Scale.x;
            modelInst->Mat->right.y *= modelInst->Scale.x;
            modelInst->Mat->right.z *= modelInst->Scale.x;

            modelInst->Mat->up.x *= modelInst->Scale.y;
            modelInst->Mat->up.y *= modelInst->Scale.y;
            modelInst->Mat->up.z *= modelInst->Scale.y;

            modelInst->Mat->at.x *= modelInst->Scale.z;
            modelInst->Mat->at.y *= modelInst->Scale.z;
            modelInst->Mat->at.z *= modelInst->Scale.z;
        }

        if (!xModelCullSingle(modelInst)) {
            xLightKit_Enable(modelInst->LightKit, xglobals->currWorld);

            if (modelInst->RedMultiplier != 1.0f || modelInst->GreenMultiplier != 1.0f || modelInst->BlueMultiplier != 1.0f) {
                reset = true;
                xModelMaterialMul(modelInst, modelInst->RedMultiplier, modelInst->GreenMultiplier, modelInst->BlueMultiplier);
            }

            U8 alpha = 255.0f * modelInst->Alpha;

            if (alpha != 255) {
                reset = true;
                xModelSetMaterialAlpha(modelInst, alpha);
            }

            if (modelInst->Flags & 0x80) {
                xAnimPlay* a = modelInst->Anim;

                for (U16 i = 0; i < a->NumSingle; i++)
                    if (a->Single[i].SingleFlags & 0x8000) {
                        if (a->Single[i].State) {
                            xMorphRender(*(xMorphSeqFile**)a->Single[i].State->Data->RawData, modelInst->Mat, xAnimFileRawTime(a->Single[i].State->Data, a->Single[i].Time));
                        }
                        break;
                    }
            } else {
                iModelRender(modelInst->Data, modelInst->Mat);
            }
        }

        if (reset) {
            xModelResetMaterial(modelInst);
        }

        if (modelInst->Scale.x) {
            xMat3x3Copy((xMat3x3*)modelInst->Mat, &tmpmat);
        }
    }
}

void xModelRender(xModelInstance* modelInst)
{
    while (modelInst) {
        if (xModelBucketEnabled)
            xModelBucket_Add(modelInst);
        else
            xModelRenderSingle(modelInst);
        modelInst = modelInst->Next;
    }
}

void xModelRender2D(const xModelInstance& model, const basic_rect<F32>& r, const xVec3& from, const xVec3& to)
{
    RwCamera* camera;
    RwFrame* frame;
    RwMatrix* cammat;
    xMat4x3 objmat, shearmat, temp1, temp2;
    const RwV2d* camvw;
    F32 viewscale, shearX, shearY;
    xVec3 vscale;
    xMat4x3 objmat1;
    if (r.w <= 0.0f || r.h <= 0.0f || r.x + r.w < 0.0f || r.x > 1.0f || r.y + r.h < 0.0f || r.y > 1.0f) {
        return;
    }

    camera = RwCameraGetCurrentCamera();
    frame = RwCameraGetFrame(camera);
    cammat = RwFrameGetLTM(frame);

    xMat3x3LookAt(&objmat, &to, &from);
    xMat3x3Transpose(&objmat, &objmat);
    objmat.pos.x = 0.0f;
    objmat.pos.y = 0.0f;
    objmat.pos.z = xVec3Dist(&to, &from);
    objmat.flags = 0;

    camvw = RwCameraGetViewWindow(camera);

    viewscale = 2.0f * (camvw->x * r.w);
    shearX = camvw->x * -(2.0f * r.x + r.w - 1.0f);
    shearY = camvw->y * -(2.0f * r.y + r.h - 1.0f);

    shearmat.right.x = viewscale;
    shearmat.right.y = 0.0f;
    shearmat.right.z = 0.0f;
    shearmat.up.x = 0.0f;
    shearmat.up.y = viewscale;
    shearmat.up.z = 0.0f;
    shearmat.at.x = shearX;
    shearmat.at.y = shearY;
    shearmat.at.z = 1.0f;
    shearmat.pos.x = 0.0f;
    shearmat.pos.y = 0.0f;
    shearmat.pos.z = 0.0f;
    shearmat.flags = 0;

    xMat4x3Mul(&temp1, &objmat, &shearmat);
    xMat4x3Mul(&temp2, &temp1, (xMat4x3*)cammat);
    xMat4x3Mul(&objmat, (xMat4x3*)model.Mat, &temp2);

    if (model.Scale.x != 0.0f) {
        vscale.x = model.Scale.x;
        vscale.y = model.Scale.y;
        vscale.z = model.Scale.z;
    } else {
        vscale.assign(1.0f, 1.0f, 1.0f);
    }

    xMat4x3Scale(&objmat1, &vscale);
    xMat4x3Mul(&objmat1, &objmat1, &objmat);

    temp1 = *(xMat4x3*)model.Mat;
    *model.Mat = *(RwMatrix*)&objmat1;
    iModelRender(model.Data, model.Mat);
    *model.Mat = *(RwMatrix*)&temp1;
}

static void CameraDestroy(RwCamera* camera);

static RwCamera* CameraCreate(S32 width, S32 height, S32 zBuffer)
{
    RwCamera* camera;

    camera = RwCameraCreateWrapper();
    if (camera) {
        RwCameraSetFrame(camera, RwFrameCreate());
        RwCameraSetRaster(camera, RwRasterCreate(width, height, 0, rwRASTERTYPECAMERA));

        if (zBuffer) {
            RwCameraSetZRaster(camera, RwRasterCreate(width, height, 0, rwRASTERTYPEZBUFFER));
        }

        if (RwCameraGetFrame(camera) && RwCameraGetRaster(camera) && (!zBuffer || RwCameraGetZRaster(camera))) {
            return camera;
        }
    }

    CameraDestroy(camera);

    return NULL;
}

static void CameraDestroy(RwCamera* camera)
{
    if (camera) {
        RwRaster* raster;

        _rwFrameSyncDirty();

        RwFrame* frame = RwCameraGetFrame(camera);
        if (frame) {
            RwCameraSetFrame(camera, NULL);

            RwFrameDestroy(frame);
        }

        raster = RwCameraGetRaster(camera);
        if (raster) {
            RwRasterDestroy(raster);

            RwCameraSetRaster(camera, NULL);
        }

        raster = RwCameraGetZRaster(camera);
        if (raster) {
            RwRasterDestroy(raster);

            RwCameraSetZRaster(camera, NULL);
        }

        RwCameraDestroy(camera);
    }
}

void xModel_SceneEnter(RpWorld* world)
{
    RpWorldAddCamera(world, subcamera);
}

void xModel_SceneExit(RpWorld* world)
{
    RpWorldRemoveCamera(world, subcamera);

    iModelDeInit();
}

void xModelAnimCollStart(xModelInstance& m)
{
    m.Flags = (m.Flags & ~0x1000) | 0x800;
    if (m.anim_coll.verts)
        return;

    U32 size = iModelVertCount(m.Data);
    if (size) {
        m.anim_coll.verts = (xVec3*)xMEMALLOC(size * sizeof(xVec3), 0);
    }
}

void xModelAnimCollRefresh(const xModelInstance& cm)
{
    xModelInstance& m = (xModelInstance&)cm;
    U32 size = iModelVertCount(m.Data);
    xMat4x3& mat = *(xMat4x3*)m.Mat;
    xMat4x3 old_mat = mat;
    mat = g_I3;
    iModelVertEval(m.Data, 0, size, m.Mat, NULL, m.anim_coll.verts);
    mat = old_mat;
    m.Flags |= 0x1000;
}

void xModelGetBoneLocation(xVec3& loc, const xModelInstance& model, size_t index)
{
    xMat4x3& root_mat = *(xMat4x3*)model.Mat;
    xMat4x3& anim_mat = *(xMat4x3*)(model.Mat + index);
    if (index == 0)
        loc = root_mat.pos;
    else
        xMat4x3Toworld(&loc, &root_mat, &anim_mat.pos);
}

void xModelGetBoneMat(xMat3x3& mat, const xModelInstance& model, size_t index)
{
    const xMat3x3& root_mat = *(const xMat3x3*)model.Mat;
    if (index == 0) mat = root_mat;
    else {
        const xMat3x3& anim_mat = *(const xMat3x3*)(model.Mat + index);
        xMat3x3Mul(&mat, &anim_mat, &root_mat);
    }
}

void xModelGetBoneMat(xMat4x3& mat, const xModelInstance& model, size_t index)
{
    const xMat4x3& root_mat = *(const xMat4x3*)model.Mat;
    if (index == 0) mat = root_mat;
    else {
        const xMat4x3& anim_mat = *(const xMat4x3*)(model.Mat + index);
        xMat4x3Mul(&mat, &anim_mat, &root_mat);
    }
}

void xModelGetBoneLocationScaled(xVec3& loc, const xModelInstance& model, size_t index)
{
    xMat4x3 temp_mat;
    const xMat4x3* root_mat = (const xMat4x3*)model.Mat;
    if (model.Scale.x != 0.0f) {
        xMat3x3MulScaleC(&temp_mat, root_mat, model.Scale.x, model.Scale.y, model.Scale.z);
        temp_mat.pos = root_mat->pos;
        root_mat = &temp_mat;
    }

    if (index == 0) loc = root_mat->pos;
    else {
        const xMat4x3& anim_mat = *(const xMat4x3*)(model.Mat + index);
        xMat4x3Toworld(&loc, root_mat, &anim_mat.pos);
    }
}

void xModelGetBoneMatScaled(xMat4x3& mat, const xModelInstance& model, size_t index)
{
    xMat4x3 temp_mat;
    const xMat4x3* root_mat = (const xMat4x3*)model.Mat;
    if (model.Scale.x != 0.0f) {
        xMat3x3MulScaleC(&temp_mat, root_mat, model.Scale.x, model.Scale.y, model.Scale.z);
        temp_mat.pos = root_mat->pos;
        root_mat = &temp_mat;
    }

    if (index == 0) mat = *root_mat;
    else {
        const xMat4x3& anim_mat = *(const xMat4x3*)(model.Mat + index);
        xMat4x3Mul(&mat, &anim_mat, root_mat);
    }
}

S32 xModelSphereCull(xSphere* sphere)
{
    if (sphere == NULL) {
        return TRUE;
    }

    return RwCameraFrustumTestSphere(RwCameraGetCurrentCamera(), (RwSphere*)sphere) == rwSPHEREOUTSIDE;
}

S32 xModelSphereCullPlusShadow(xSphere* sphere, xVec3* shadowVec, S32* shadowOutside)
{
    RwCamera* cam = RwCameraGetCurrentCamera();
    RwSphere* worldsph = (RwSphere*)sphere;
    const RwFrustumPlane* frustumPlane;
    S32 numPlanes;

    frustumPlane = cam->frustumPlanes;
    numPlanes = 6;
    while (numPlanes--) {
        F32 nDot = RwV3dDotProductMacro(&worldsph->center, &frustumPlane->plane.normal);
        nDot -= frustumPlane->plane.distance;

        if (nDot > worldsph->radius) {
            goto outside;
        }

        frustumPlane++;
    }

    *shadowOutside = FALSE;
    return FALSE;

outside:
    F32 nDot, sDot;

    sDot = RwV3dDotProductMacro((RwV3d*)shadowVec, &frustumPlane->plane.normal);
    sDot -= frustumPlane->plane.distance;
    if (sDot > worldsph->radius) {
        *shadowOutside = TRUE;
        return TRUE;
    }

    frustumPlane++;
    while (numPlanes--) {
        nDot = RwV3dDotProductMacro(&worldsph->center, &frustumPlane->plane.normal);
        nDot -= frustumPlane->plane.distance;
        sDot = RwV3dDotProductMacro((RwV3d*)shadowVec, &frustumPlane->plane.normal);
        sDot -= frustumPlane->plane.distance;
        if (nDot > worldsph->radius && sDot > worldsph->radius) {
            *shadowOutside = TRUE;
            return TRUE;
        }
        frustumPlane++;
    }

    *shadowOutside = FALSE;
    return TRUE;
}

S32 xModelBoxCull(xBox* box)
{
    const RwFrustumPlane* frustumPlane;
    S32 numPlanes;
    RwFrustumTestResult result;
    RwCamera* camera = RwCameraGetCurrentCamera();

    F32 centerX = 0.5f * (box->upper.x + box->lower.x);
    F32 centerY = 0.5f * (box->upper.y + box->lower.y);
    F32 centerZ = 0.5f * (box->upper.z + box->lower.z);
    F32 sizeX = 0.5f * (box->upper.x - box->lower.x);
    F32 sizeY = 0.5f * (box->upper.y - box->lower.y);
    F32 sizeZ = 0.5f * (box->upper.z - box->lower.z);

    result = rwSPHEREINSIDE;

    frustumPlane = camera->frustumPlanes;
    numPlanes = 6;
    while (numPlanes--) {
        F32 centerDot = centerX * frustumPlane->plane.normal.x + centerY * frustumPlane->plane.normal.y +
                        centerZ * frustumPlane->plane.normal.z;
        centerDot -= frustumPlane->plane.distance;
        F32 nearDot = xabs(sizeX * frustumPlane->plane.normal.x) + xabs(sizeY * frustumPlane->plane.normal.y) +
                      xabs(sizeZ * frustumPlane->plane.normal.z);

        if (centerDot > nearDot) {
            result = rwSPHEREOUTSIDE;
            break;
        }

        if (centerDot > -nearDot) {
            result = rwSPHEREBOUNDARY;
        }

        frustumPlane++;
    }

    return result == rwSPHEREOUTSIDE;
}

S32 xModelBoxCullPlusShadow(xBox* box, xVec3* shadowVec, S32* shadowOutside)
{
    const RwFrustumPlane* frustumPlane;
    S32 numPlanes;
    RwCamera* camera = RwCameraGetCurrentCamera();
    F32 nearDot;

    if (box->upper.x == -1e38f) {
        return TRUE;
    }

    F32 centerX = 0.5f * (box->upper.x + box->lower.x);
    F32 centerY = 0.5f * (box->upper.y + box->lower.y);
    F32 centerZ = 0.5f * (box->upper.z + box->lower.z);
    F32 sizeX = 0.5f * (box->upper.x - box->lower.x);
    F32 sizeY = 0.5f * (box->upper.y - box->lower.y);
    F32 sizeZ = 0.5f * (box->upper.z - box->lower.z);

    frustumPlane = camera->frustumPlanes;
    numPlanes = 6;
    while (numPlanes--) {
        F32 centerDot = centerX * frustumPlane->plane.normal.x + centerY * frustumPlane->plane.normal.y +
                        centerZ * frustumPlane->plane.normal.z;
        centerDot -= frustumPlane->plane.distance;
        nearDot = xabs(sizeX * frustumPlane->plane.normal.x) + xabs(sizeY * frustumPlane->plane.normal.y) +
                  xabs(sizeZ * frustumPlane->plane.normal.z);

        if (centerDot > nearDot) {
            goto outside;
        }

        frustumPlane++;
    }

    *shadowOutside = FALSE;
    return FALSE;

outside:
    F32 shadowDot = RwV3dDotProductMacro(shadowVec, &frustumPlane->plane.normal);
    shadowDot -= frustumPlane->plane.distance;
    if (shadowDot > nearDot) {
        *shadowOutside = TRUE;
        return TRUE;
    }

    frustumPlane++;
    while (numPlanes--) {
        F32 centerDot = centerX * frustumPlane->plane.normal.x + centerY * frustumPlane->plane.normal.y +
                        centerZ * frustumPlane->plane.normal.z;
        centerDot -= frustumPlane->plane.distance;
        shadowDot = RwV3dDotProductMacro(shadowVec, &frustumPlane->plane.normal);
        shadowDot -= frustumPlane->plane.distance;
        nearDot = xabs(sizeX * frustumPlane->plane.normal.x) + xabs(sizeY * frustumPlane->plane.normal.y) +
                  xabs(sizeZ * frustumPlane->plane.normal.z);

        if (centerDot > nearDot && shadowDot > nearDot) {
            *shadowOutside = TRUE;
            return TRUE;
        }

        frustumPlane++;
    }

    *shadowOutside = FALSE;
    return TRUE;
}

static const S32 BoxEnable = TRUE;

S32 xModelCullGroup(xModelInstance* minst)
{
    if (BoxEnable && minst->BoneCount) {
        return xModelBoxCull(&minst->combinedAnimBound);
    }

    return xModelSphereCull(iModelWorldBoundSphere(minst->Data, minst->Mat, NULL));
}

S32 xModelCullGroupPlusShadow(xModelInstance* minst, F32 shadowDepth, S32* shadowOutside)
{
    xVec3 shadVec;

    if (BoxEnable && minst->BoneCount) {
        xBox* bbox = &minst->combinedAnimBound;
        shadVec.x = 0.5f * (bbox->upper.x + bbox->lower.x);
        shadVec.y = minst->Mat->pos.y + 0.5f * (bbox->upper.x - bbox->lower.x) - shadowDepth;
        shadVec.z = 0.5f * (bbox->upper.z + bbox->lower.z);
        return xModelBoxCullPlusShadow(&minst->combinedAnimBound, &shadVec, shadowOutside);
    }

    shadVec.x = minst->Mat->pos.x;
    shadVec.y = minst->Mat->pos.y - 10.0f;
    shadVec.z = minst->Mat->pos.z;
    return xModelSphereCullPlusShadow(iModelWorldBoundSphere(minst->Data, minst->Mat, NULL), &shadVec, shadowOutside);
}

S32 xModelCullSingle(xModelInstance* minst)
{
    if (BoxEnable && minst->BoneCount) {
        S32 result = xModelBoxCull(&minst->animBound);
        if (!result) {
            iModelSetWorldBoundFromBox(minst->Data, &minst->animBound);
        }

        return result;
    }

    return xModelSphereCull(iModelWorldBoundSphere(minst->Data, minst->Mat, NULL));
}

void xModelSetLightKit(xModelInstance* minst, xLightKit* lightKit)
{
    while (minst) {
        if ((minst->Pipe.Flags & eLIGHT_MASK) != ePIPELIGHT_PRELIGHTONLY) {
            minst->LightKit = lightKit;
        } else {
            minst->LightKit = NULL;
        }

        minst = minst->Next;
    }
}

void xModelCloneAtomic(RpAtomic** out, S32 out_size, RpAtomic* in)
{
    iAsyncLockRWSentry rw_sentry;

    RwMemory rwmem = { NULL, 0 };
    RwStream* ostream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, &rwmem);
    RpAtomicStreamWrite(in, ostream);
    RwStreamClose(ostream, &rwmem);

    RpAtomic** end_out = out + out_size;
    for (; out != end_out; out++) {
        RwStream* istream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &rwmem);
        RwStreamFindChunk(istream, rwID_ATOMIC, NULL, NULL);
        *out = RpAtomicStreamRead(istream);
        RwStreamClose(istream, &rwmem);
        RpAtomicSetFrame(*out, RwFrameCreate());
    }

    RwFree(rwmem.start);
}

namespace {
RpAtomic* find_first_atomic(xModelAssetInfo* minf)
{
    xModelAssetInst* inst = (xModelAssetInst*)(minf + 1);
    xModelAssetInst* end_inst = inst + minf->NumModelInst;
    for (; inst != end_inst; inst++) {
        if (!inst->ModelID)
            continue;

        void* data = xSTFindAsset(inst->ModelID, NULL);
        if (!data)
            continue;

        if (*(U32*)data == 'FNIM') {
            RpAtomic* child_atomic = find_first_atomic((xModelAssetInfo*)data);
            if (child_atomic)
                return child_atomic;
        } else
            return (RpAtomic*)data;
    }

    return NULL;
}
}

RpAtomic* xModelFindAtomic(U32 id, U32* actual_id)
{
    RpAtomic* atomic = NULL;
    U32 aid = id;

    if (id) {
        atomic = (RpAtomic*)xSTFindAsset(aid, NULL);
        if (!atomic) {
            aid = xStrHashCat(id, ".dff");
            atomic = (RpAtomic*)xSTFindAsset(aid, NULL);
            if (!atomic) {
                aid = xStrHashCat(id, ".minf");
                xModelAssetInfo* minf = (xModelAssetInfo*)xSTFindAsset(aid, NULL);
                if (minf)
                    atomic = find_first_atomic(minf);
            }
        }
    }

    if (actual_id)
        *actual_id = !atomic ? id : aid;
    return atomic;
}

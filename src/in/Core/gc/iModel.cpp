#include "xCommon.h"
#include "iModel.h"

#include "xMath3.h"
#include "xMemMgr.h"
#include "xModel.h"
#include "xVec3.h"
#include "xGlobals.h"
#include "iAnim.h"
#include "iCamera.h"
#include "iSystem.h"

#include <rpskin.h>
#include <rpmatfx.h>
#include <rpusrdat.h>
#include <rtskinsp.h>
#include <string.h>

#define HANIMMATRIXSTACKHEIGHT 32
#define IMODEL_MAX_MATERIALS 32
#define IMODEL_MAX_ATOMICS 256
#define IMODEL_NUM_EMPTY_DIRLIGHTS 4

RpWorld* instance_world;
RwCamera* instance_camera;

static U32 gLastAtomicCount;
static RpAtomic* gLastAtomicList[IMODEL_MAX_ATOMICS];
static RpLight* sEmptyDirectionalLight[IMODEL_NUM_EMPTY_DIRLIGHTS];
static RpLight* sEmptyAmbientLight;

static RwFrame* GetChildFrameHierarchy(RwFrame* frame, void* data)
{
    RpHAnimHierarchy* hierarchy = *(RpHAnimHierarchy**)data;
    hierarchy = RpHAnimFrameGetHierarchy(frame);
    if (!hierarchy) {
        RwFrameForAllChildren(frame, GetChildFrameHierarchy, data);
        return frame;
    }
    *(RpHAnimHierarchy**)data = hierarchy;
    return NULL;
}

RpHAnimHierarchy* iModelGetHierarchy(RpAtomic* imodel)
{
    RpHAnimHierarchy* hierarchy = NULL;
    RwFrame* frame = RpAtomicGetFrame(imodel);
    GetChildFrameHierarchy(frame, &hierarchy);
    return hierarchy;
}

void iModelInit()
{
    RwFrame* frame;
    RwRGBAReal black = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (!sEmptyDirectionalLight[0]) {
        for (S32 i = 0; i < IMODEL_NUM_EMPTY_DIRLIGHTS; i++) {
            sEmptyDirectionalLight[i] = RpLightCreate(rpLIGHTDIRECTIONAL);
            RpLightSetColor(sEmptyDirectionalLight[i], &black);
            frame = RwFrameCreate();
            RpLightSetFrame(sEmptyDirectionalLight[i], frame);
        }
        sEmptyAmbientLight = RpLightCreate(rpLIGHTAMBIENT);
        RpLightSetColor(sEmptyAmbientLight, &black);
    }
}

void iModelDeInit()
{
    if (sEmptyDirectionalLight[0]) {
        for (S32 i = 0; i < IMODEL_NUM_EMPTY_DIRLIGHTS; i++) {
            RwFrame* frame = RpLightGetFrame(sEmptyDirectionalLight[i]);
            RwFrameDestroy(frame);
            RpLightDestroy(sEmptyDirectionalLight[i]);
            sEmptyDirectionalLight[i] = NULL;
        }
        RpLightDestroy(sEmptyAmbientLight);
        sEmptyAmbientLight = NULL;
    }
}

static RpAtomic* FindAndInstanceAtomicCallback(RpAtomic* atomic, void*)
{
    RpHAnimHierarchy* pHier = iModelGetHierarchy(atomic);

    RpGeometry* pGeom = RpAtomicGetGeometry(atomic);
    RpSkin* pSkin = RpSkinGeometryGetSkin(pGeom);

    if (pSkin && !pHier) {
        pHier = RpHAnimHierarchyCreate(RpSkinGetNumBones(pSkin), NULL, NULL, rpHANIMHIERARCHYLOCALSPACEMATRICES,
                                       rpHANIMSTDKEYFRAMESIZE);
        RpHAnimFrameSetHierarchy(RpAtomicGetFrame(atomic), pHier);
    }

    if (pHier && pSkin) {
        RpSkinAtomicSetHAnimHierarchy(atomic, pHier);
    }

    if (pHier) {
        pHier->flags = rpHANIMHIERARCHYLOCALSPACEMATRICES;
    }

    if (pHier && pSkin) {
        iModelAnimBoundPrepare(atomic);
    }

    RwFrame* frame = RpAtomicGetFrame(atomic);
    RwFrame* root = RwFrameGetRoot(frame);

    if (gLastAtomicCount < IMODEL_MAX_ATOMICS)
        gLastAtomicList[gLastAtomicCount++] = atomic;
    return atomic;
}

static RpAtomic* iModelStreamRead(RwStream* stream)
{
    static S32 num_models = 0;

    if (!stream) {
        return NULL;
    }

    if (!RwStreamFindChunk(stream, rwID_CLUMP, NULL, NULL)) {
        RwStreamClose(stream, NULL);
        return NULL;
    }

    RpClump* clump = RpClumpStreamRead(stream);
    RwStreamClose(stream, NULL);
    if (!clump) {
        return NULL;
    }

    RwBBox tmpbbox = { { 1000.0f, 1000.0f, 1000.0f }, { -1000.0f, -1000.0f, -1000.0f } };
    instance_world = RpWorldCreate(&tmpbbox);

    instance_camera = iCameraCreate(FB_XRES, FB_YRES, 0);
    RpWorldAddCamera(instance_world, instance_camera);

    gLastAtomicCount = 0;
    RpClumpForAllAtomics(clump, FindAndInstanceAtomicCallback, NULL);

    RpWorldRemoveCamera(instance_world, instance_camera);
    iCameraDestroy(instance_camera);
    RpWorldDestroy(instance_world);

    if (gLastAtomicCount > 1) {
        U32 i, maxIndex = 0;
        F32 maxRadius = -1.0f, testRadius;

        for (i = 0; i < gLastAtomicCount; i++) {
            if (gLastAtomicList[i]->boundingSphere.radius > maxRadius) {
                maxRadius = gLastAtomicList[i]->boundingSphere.radius;
                maxIndex = i;
            }
        }

        for (i = 0; i < gLastAtomicCount; i++) {
            if (i != maxIndex) {
                testRadius = gLastAtomicList[i]->boundingSphere.radius +
                             xVec3Dist((xVec3*)&gLastAtomicList[i]->boundingSphere.center,
                                       (xVec3*)&gLastAtomicList[maxIndex]->boundingSphere.center);
                if (testRadius > maxRadius) {
                    maxRadius = testRadius;
                }
            }
        }

        maxRadius *= 1.05f;

        for (i = 0; i < gLastAtomicCount; i++) {
            if (i != maxIndex) {
                gLastAtomicList[i]->boundingSphere.center = gLastAtomicList[maxIndex]->boundingSphere.center;
            }
            gLastAtomicList[i]->boundingSphere.radius = maxRadius;
            gLastAtomicList[i]->interpolator.flags &= ~rpINTERPOLATORDIRTYSPHERE;
        }
    }

    for (S32 i = 0; i < gLastAtomicCount; i++) {
        RpAtomic* atomic = gLastAtomicList[i];
        RpGeometry* geom = RpAtomicGetGeometry(atomic);
        if (geom) {
            RpSkin* skin = RpSkinGeometryGetSkin(geom);
            if (skin)
                RtSkinSplitAtomicSplitGeometry(atomic, 10);
        }
    }

    for (S32 i = 0; i < gLastAtomicCount; i++) {
        RpAtomic* atomic = gLastAtomicList[i];
        RpGeometry* geom = RpAtomicGetGeometry(atomic);
        for (S32 j = 0; j < geom->matList.numMaterials; j++) {
            RpMaterial* material = geom->matList.materials[j];
            if (RpMatFXMaterialGetEffects(material) == rpMATFXEFFECTENVMAP ||
                RpMatFXMaterialGetEffects(material) == rpMATFXEFFECTBUMPENVMAP) {
                RwReal coeff = RpMatFXMaterialGetEnvMapCoefficient(material);
                RpMatFXMaterialSetEnvMapCoefficient(material, 0.5f * coeff);
            }
        }
    }

    return gLastAtomicList[0];
}

RpAtomic* iModelFileNew(void* buffer, U32 size)
{
    RwMemory rwmem;

    rwmem.start = (RwUInt8*)buffer;
    rwmem.length = size;
    return iModelStreamRead(RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &rwmem));
}

void iModelUnload(RpAtomic* userdata)
{
    RpClump* clump;

    RpAtomic* atomic = userdata;
    RpAtomic* atomicCopy = atomic;
    clump = RpAtomicGetClump(atomicCopy);

    RwFrame* frame = RpClumpGetFrame(clump);
    if (frame) {
        RwFrame* root = RwFrameGetRoot(frame);
        if (root) {
            frame = root;
        }

        RwFrameDestroyHierarchy(frame);
        RpClumpSetFrame(clump, NULL);
    }

    if (!clump || !RpClumpDestroy(clump)) {
    }
}

static RpAtomic* NextAtomicCallback(RpAtomic* atomic, void* data)
{
    RpAtomic** nextModel = (RpAtomic**)data;

    if (*nextModel == atomic)
        *nextModel = NULL;
    else if (!*nextModel)
        *nextModel = atomic;

    return atomic;
}

RpAtomic* iModelFile_RWMultiAtomic(RpAtomic* model)
{
    if (!model) return NULL;

    RpClump* clump = RpAtomicGetClump(model);
    RpAtomic* nextModel = model;
    RpClumpForAllAtomics(clump, NextAtomicCallback, &nextModel);
    return nextModel;
}

U32 iModelNumBones(RpAtomic* model)
{
    RpHAnimHierarchy* pHier = iModelGetHierarchy(model);

    if (!pHier) {
        return 0;
    }

    return pHier->numNodes;
}

void iModelQuatToMat(xQuat* quat, xVec3* tran, RwMatrix* mat)
{
    quat->s = -quat->s;
    xQuatToMat(quat, (xMat3x3*)mat);
    quat->s = -quat->s;

    mat->pos.x = tran->x;
    mat->pos.y = tran->y;
    mat->pos.z = tran->z;
}

void iModelAnimMatrices(RpAtomic* model, xQuat* quat, xVec3* tran, RwMatrix* mat)
{
    RpHAnimHierarchy* pHierarchy = iModelGetHierarchy(model);

    if (!pHierarchy) {
        return;
    }

    RwMatrix matrixStack[HANIMMATRIXSTACKHEIGHT];
    RwMatrix* pMatrixStackTop = &matrixStack[0];
    RpHAnimNodeInfo* pCurrentFrame;
    S32 currentFrameFlags, i, numFrames;
    RwMatrix* pMatrixArray;
    RwMatrix parentMatrix, frameMatrix, tempMatrix;
    RwMatrix* ptrMatrix;

    RwMatrixSetIdentity(&parentMatrix);
    RwMatrixCopy(pMatrixStackTop, &parentMatrix);
    pMatrixStackTop++;

    numFrames = pHierarchy->numNodes;
    pCurrentFrame = pHierarchy->pNodeInfo;

    pMatrixArray = mat;

    for (i = 0; i < numFrames; i++) {
        currentFrameFlags = pCurrentFrame->flags;
        if (currentFrameFlags & rpHANIMPUSHPARENTMATRIX) {
            ptrMatrix = pMatrixStackTop++;
            RwMatrixCopy(ptrMatrix, &parentMatrix);
        }

        iModelQuatToMat(quat, tran, &frameMatrix);
        xMat4x3Mul((xMat4x3*)&tempMatrix, (xMat4x3*)&frameMatrix, (xMat4x3*)&parentMatrix);
        *pMatrixArray = tempMatrix;

        ptrMatrix = (currentFrameFlags & rpHANIMPOPPARENTMATRIX) ? --pMatrixStackTop : &tempMatrix;

        RwMatrixCopy(&parentMatrix, ptrMatrix);

        pMatrixArray++;
        quat++;
        tran++;

        pCurrentFrame++;
    }
}

static RpAtomic* iModelCacheAtomic(RpAtomic* model)
{
    return model;
}

void iModelRender(RpAtomic* model, RwMatrix* mat)
{
    RpHAnimHierarchy* pHierarchy;
    RwMatrix* pAnimOldMatrix;
    RwFrame* frame;

    pHierarchy = iModelGetHierarchy(model);

    static S32 draw_all = 1;

    if (pHierarchy) {
        pAnimOldMatrix = pHierarchy->pMatrixArray;
        pHierarchy->pMatrixArray = mat + 1;
    }

    frame = RpAtomicGetFrame(model);

    frame->ltm = *mat;
    RwMatrixUpdate(&frame->ltm);

    if (pHierarchy && RpGeometryGetNumTriangles(RpAtomicGetGeometry(model)) > 3000) {
        iRwResourcesEmptyArena();
    }

    RpAtomicRender(iModelCacheAtomic(model));

    if (pHierarchy) {
        pHierarchy->pMatrixArray = pAnimOldMatrix;
    }
}

xSphere* iModelWorldBoundSphere(RpAtomic* model, RwMatrix* mat, xSphere* sphere)
{
    RwSphere* modelsphere = &model->boundingSphere;
    RwSphere* worldsphere = sphere ? (RwSphere*)sphere : &model->worldBoundingSphere;
    RwReal xScale2, yScale2, zScale2;
    F32 atomicScale;

    RwV3dTransformPoints(&worldsphere->center, &modelsphere->center, 1, mat);

    RwV3d* right = &mat->right;
    RwV3d* up = &mat->up;
    RwV3d* at = &mat->at;
    xScale2 = RwV3dDotProduct(right, right);
    yScale2 = RwV3dDotProduct(up, up);
    zScale2 = RwV3dDotProduct(at, at);

    atomicScale = xsqrt(RwRealMax2(xScale2, RwRealMax2(yScale2, zScale2)));

    worldsphere->radius = modelsphere->radius * atomicScale;

    return (xSphere*)worldsphere;
}

void iModelSetWorldBoundFromBox(RpAtomic* model, xBox* box)
{
    model->worldBoundingSphere.center.x = 0.5f * (box->upper.x + box->lower.x);
    model->worldBoundingSphere.center.y = 0.5f * (box->upper.y + box->lower.y);
    model->worldBoundingSphere.center.z = 0.5f * (box->upper.z + box->lower.z);

    F32 sizeX = 0.5f * (box->upper.x - box->lower.x);
    F32 sizeY = 0.5f * (box->upper.y - box->lower.y);
    F32 sizeZ = 0.5f * (box->upper.z - box->lower.z);
    model->worldBoundingSphere.radius = xsqrt(xsqr(sizeX) + xsqr(sizeY) + xsqr(sizeZ));
}

U32 iModelVertCount(RpAtomic* model)
{
    return RpGeometryGetNumVertices(RpAtomicGetGeometry(model));
}

static inline void SkinXform(xVec3* dest, const xVec3* vert, RwMatrix* mat, const RwMatrix* skinmat, const F32* wt,
                             const U32* idx, U32 count)
{
    U32 catMatFlags[2] = { 0, 0 };
    RwMatrix* catmat = (RwMatrix*)xMemScratchPush(IANIM_MAX_BONES * sizeof(RwMatrix), 0);

    RwMatrix* rootmat = mat;
    mat++;

    while (count) {
        U32 i, midx;

        for (i = 0; i < 4; i++) {
            midx = (*idx >> (i * 8)) & 0xFF;
            if (!(catMatFlags[midx >> 5] & (1 << (midx & 31)))) {
                xMat4x3Mul((xMat4x3*)&catmat[midx], (const xMat4x3*)&skinmat[midx], (const xMat4x3*)&mat[midx]);
                catMatFlags[midx >> 5] |= 1 << (midx & 31);
            }
        }

        xVec3 accumV;
        RwMatrix* pMatrix;
        const F32* fwt = wt;
        U32 wtidx = *idx;
        U32 maxwt = 4;
        accumV.x = accumV.y = accumV.z = 0.0f;

        while (*fwt && maxwt) {
            pMatrix = &catmat[wtidx & 0xFF];
            wtidx >>= 8;

            accumV.x += *fwt * (pMatrix->right.x * vert->x + pMatrix->up.x * vert->y + pMatrix->at.x * vert->z +
                                pMatrix->pos.x);
            accumV.y += *fwt * (pMatrix->right.y * vert->x + pMatrix->up.y * vert->y + pMatrix->at.y * vert->z +
                                pMatrix->pos.y);
            accumV.z += *fwt * (pMatrix->right.z * vert->x + pMatrix->up.z * vert->y + pMatrix->at.z * vert->z +
                                pMatrix->pos.z);

            fwt++;
            maxwt--;
        }

        dest->x = rootmat->right.x * accumV.x + rootmat->up.x * accumV.y + rootmat->at.x * accumV.z + rootmat->pos.x;
        dest->y = rootmat->right.y * accumV.x + rootmat->up.y * accumV.y + rootmat->at.y * accumV.z + rootmat->pos.y;
        dest->z = rootmat->right.z * accumV.x + rootmat->up.z * accumV.y + rootmat->at.z * accumV.z + rootmat->pos.z;

        dest++;

        vert++;
        idx++;
        wt += 4;
        count--;
    }

    xMemScratchPop(catmat);
}

U32 iModelVertEval(RpAtomic* model, U32 index, U32 count, RwMatrix* mat, xVec3* vert, xVec3* dest)
{
    RpGeometry* geom = RpAtomicGetGeometry(model);
    U32 numV;
    RpSkin* skin;

    if (!vert) {
        numV = RpGeometryGetNumVertices(geom);

        if (index >= numV || !count) return 0;
        count = MIN(count, numV - index);

        vert = (xVec3*)RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geom, 0));
        vert += index;
    }

    skin = RpSkinGeometryGetSkin(RpAtomicGetGeometry(model));
    if (skin) {
        SkinXform(dest, vert, mat, RpSkinGetSkinToBoneMatrices(skin),
                  (const F32*)&RpSkinGetVertexBoneWeights(skin)[index], &RpSkinGetVertexBoneIndices(skin)[index],
                  count);
    } else {
        RwV3dTransformPoints((RwV3d*)dest, (RwV3d*)vert, count, mat);
    }

    return count;
}

static U32 iModelTagUserData(xModelTag* tag, const RpAtomic* model, F32 x, F32 y, F32 z, S32 closeV)
{
    S32 i, count = RpGeometryGetUserDataArrayCount(model->geometry);
    RpUserDataArray* array = NULL;
    for (i = 0; i < count; i++) {
        RpUserDataArray* testarray = RpGeometryGetUserDataArray(model->geometry, i);
        if (!strcmp(testarray->name, "HI_Tags")) {
            array = testarray;
            break;
        }
    }
    if (!array) {
        memset(tag, 0, sizeof(xModelTag));
        return 0;
    }

    F32 distSqr, closeDistSqr = 1000000000.0f;
    S32 numTags, t;
    xModelTag* tagList;

    numTags = *(S32*)array->data;
    tagList = (xModelTag*)((S32*)array->data + 1);

    if (closeV < 0 || closeV > numTags) {
        closeV = 0;
        for (t = 0; t < numTags; t++) {
            distSqr = xsqr(tagList[t].v.x - x) + xsqr(tagList[t].v.y - y) + xsqr(tagList[t].v.z - z);
            if (distSqr < closeDistSqr) {
                closeV = t;
                closeDistSqr = distSqr;
            }
        }
        if (tag) {
            *tag = tagList[closeV];
        }
    } else {
        if (tag) {
            *tag = tagList[closeV];
        }
    }

    return closeV;
}

static U32 iModelTagInternal(xModelTag* tag, const RpAtomic* model, F32 x, F32 y, F32 z, S32 closeV)
{
    RpGeometry* geom = RpAtomicGetGeometry(model);
    RwV3d* vert = RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geom, 0));
    S32 v, numV;
    F32 distSqr, closeDistSqr;

    if (!vert) {
        return iModelTagUserData(tag, model, x, y, z, closeV);
    }

    numV = RpGeometryGetNumVertices(geom);
    closeDistSqr = 1000000000.0f;

    if (closeV < 0 || closeV > numV) {
        closeV = 0;
        for (v = 0; v < numV; v++) {
            distSqr = xsqr(vert[v].x - x) + xsqr(vert[v].y - y) + xsqr(vert[v].z - z);
            if (distSqr < closeDistSqr) {
                closeV = v;
                closeDistSqr = distSqr;
            }
        }
        if (tag) {
            tag->v.x = x;
            tag->v.y = y;
            tag->v.z = z;
        }
    } else {
        if (tag) {
            tag->v.x = vert[closeV].x;
            tag->v.y = vert[closeV].y;
            tag->v.z = vert[closeV].z;
        }
    }

    if (tag) {
        RpSkin* skin = RpSkinGeometryGetSkin(RpAtomicGetGeometry(model));
        if (skin) {
            const RwMatrixWeights* wt = &RpSkinGetVertexBoneWeights(skin)[closeV];
            tag->matidx = RpSkinGetVertexBoneIndices(skin)[closeV];
            tag->wt[0] = wt->w0;
            tag->wt[1] = wt->w1;
            tag->wt[2] = wt->w2;
            tag->wt[3] = wt->w3;
        } else {
            tag->matidx = 0;
            tag->wt[0] = 0.0f;
            tag->wt[1] = 0.0f;
            tag->wt[2] = 0.0f;
            tag->wt[3] = 0.0f;
        }
    }

    return closeV;
}

U32 iModelTagSetup(xModelTag* tag, const RpAtomic* model, F32 x, F32 y, F32 z)
{
    return iModelTagInternal(tag, model, x, y, z, -1);
}

void iModelTagEval(RpAtomic* model, const xModelTag* tag, RwMatrix* mat, xVec3* dest)
{
    if (tag->wt[0]) {
        RpGeometry* geom = RpAtomicGetGeometry(model);
        RpSkin* skin = RpSkinGeometryGetSkin(geom);
        const RwMatrix* skinmat = RpSkinGetSkinToBoneMatrices(skin);
        SkinXform(dest, &tag->v, mat, skinmat, tag->wt, &tag->matidx, 1);
    } else {
        RwV3dTransformPoints((RwV3d*)dest, (const RwV3d*)&tag->v, 1, mat);
    }
}

static RwRGBA sMaterialColor[IMODEL_MAX_MATERIALS];
static RwTexture* sMaterialTexture[IMODEL_MAX_MATERIALS];
static U8 sMaterialAlpha[IMODEL_MAX_MATERIALS];
static U32 sMaterialIdx;
static U32 sMaterialFlags;
static RpAtomic* sLastMaterial;

void iModelSetMaterialAlpha(RpAtomic* model, U8 alpha)
{
    RpGeometry* geom = RpAtomicGetGeometry(model);

    if (model != sLastMaterial) {
        sMaterialFlags = 0;
    }

    RpGeometrySetFlags(geom, RpGeometryGetFlags(geom) | rpGEOMETRYMODULATEMATERIALCOLOR);

    sMaterialIdx = geom->matList.numMaterials;
    if (sMaterialIdx > IMODEL_MAX_MATERIALS)
        sMaterialIdx = IMODEL_MAX_MATERIALS;
    for (S32 i = 0; i < geom->matList.numMaterials; i++) {
        RpMaterial* material = geom->matList.materials[i];
        const RwRGBA* col = RpMaterialGetColor(material);
        sMaterialAlpha[i] = col->alpha;
        RwRGBA new_col = *col;
        new_col.alpha = alpha;
        RpMaterialSetColor(material, &new_col);
    }

    sMaterialFlags |= 0x1;
    sLastMaterial = model;
}

void iModelResetMaterial(RpAtomic* model)
{
    if (model != sLastMaterial) {
        sMaterialFlags = 0;
    }

    RpGeometry* geom = RpAtomicGetGeometry(model);

    for (S32 i = 0; i < geom->matList.numMaterials; i++) {
        RpMaterial* material = geom->matList.materials[i];

        if ((sMaterialFlags & 0x3) == 0x3) {
            RwRGBA newColor = sMaterialColor[i];
            newColor.alpha = sMaterialAlpha[i];
            RpMaterialSetColor(material, &newColor);
        } else {
            if (sMaterialFlags & 0x2) {
                RwRGBA newColor = sMaterialColor[i];
                newColor.alpha = RpMaterialGetColor(material)->alpha;
                RpMaterialSetColor(material, &newColor);
            }

            if (sMaterialFlags & 0x1) {
                RwRGBA newColor = *RpMaterialGetColor(material);
                newColor.alpha = sMaterialAlpha[i];
                RpMaterialSetColor(material, &newColor);
            }
        }

        if (sMaterialFlags & 0x4) {
            RpMaterialSetTexture(material, sMaterialTexture[i]);
        }
    }

    sMaterialFlags = 0;
}

static RpMaterial* iModelSetMaterialTextureCB(RpMaterial* material, void* data)
{
    sMaterialTexture[sMaterialIdx++] = RpMaterialGetTexture(material);

    RpMaterialSetTexture(material, (RwTexture*)data);

    return material;
}

void iModelSetMaterialTexture(RpAtomic* model, void* texture)
{
    if (model != sLastMaterial) {
        sMaterialFlags = 0;
    }

    RpGeometry* geom = RpAtomicGetGeometry(model);

    sMaterialIdx = 0;
    RpGeometryForAllMaterials(geom, iModelSetMaterialTextureCB, texture);

    sMaterialFlags |= 0x4;
    sLastMaterial = model;
}

namespace {
inline void U8_COLOR_CLAMP(U8& destu8, F32 srcf32)
{
    if (srcf32 < 0.0f) srcf32 = 0.0f;
    else if (srcf32 > 255.0f) srcf32 = 255.0f;
    destu8 = srcf32;
}
} // namespace

static RpMaterial* iModelMaterialMulCB(RpMaterial* material, void* data)
{
    const RwRGBA* rw_col = RpMaterialGetColor(material);
    RwRGBA col = sMaterialColor[sMaterialIdx++] = *rw_col;
    F32 tmp;
    F32* mods = (F32*)data;

    tmp = col.red * mods[0];
    U8_COLOR_CLAMP(col.red, tmp);
    tmp = col.green * mods[1];
    U8_COLOR_CLAMP(col.green, tmp);
    tmp = col.blue * mods[2];
    U8_COLOR_CLAMP(col.blue, tmp);

    RpMaterialSetColor(material, &col);

    return material;
}

void iModelMaterialMul(RpAtomic* model, F32 rm, F32 gm, F32 bm)
{
    RpGeometry* geom = RpAtomicGetGeometry(model);
    F32 cols[3];

    if (model != sLastMaterial) {
        sMaterialFlags = 0;
    }

    RpGeometrySetFlags(geom, RpGeometryGetFlags(geom) | rpGEOMETRYMODULATEMATERIALCOLOR);

    cols[0] = rm;
    cols[1] = gm;
    cols[2] = bm;
    sMaterialIdx = 0;
    RpGeometryForAllMaterials(geom, iModelMaterialMulCB, cols);

    sMaterialFlags |= 0x2;
    sLastMaterial = model;
}

void iModelAnimBoundPrepare(RpAtomic* model)
{
    RpHAnimHierarchy* pHier = iModelGetHierarchy(model);
    if (!pHier) {
        return;
    }

    S32 numbones = pHier->numNodes;
    S32 i, j;
    S32 count = RpGeometryGetUserDataArrayCount(model->geometry);
    RpUserDataArray* usrdat;

    for (i = 0; i < count; i++) {
        usrdat = RpGeometryGetUserDataArray(model->geometry, i);
        if (*(U32*)usrdat->name == 'HIAB') {
            goto found;
        }
    }

    {
        RpSkin* skin = RpSkinGeometryGetSkin(model->geometry);

        const U32* boneidx;
        const RwMatrixWeights* bonewt;
        RwV3d* vert;
        S32 numVert = model->geometry->numVertices;

        S32* cntarr = (S32*)xMEMPUSHTEMP(numbones * sizeof(S32));
        xBox* boxarr = (xBox*)xMEMPUSHTEMP(numbones * sizeof(xBox));
        memset(cntarr, 0, numbones * sizeof(S32));

        boneidx = RpSkinGetVertexBoneIndices(skin);
        bonewt = RpSkinGetVertexBoneWeights(skin);
        vert = model->geometry->morphTarget->verts;
        for (i = 0; i < numVert; i++) {
            const F32* currwt = (const F32*)bonewt;
            U32 curridx = *boneidx;
            for (j = 0; j < 4 && *currwt != 0.0f; j++) {
                U32 matidx = curridx & 0xFF;
                if (!cntarr[matidx]) {
                    boxarr[matidx].upper = *(xVec3*)vert;
                    boxarr[matidx].lower = *(xVec3*)vert;
                } else {
                    boxarr[matidx].upper.x = MAX(boxarr[matidx].upper.x, vert->x);
                    boxarr[matidx].upper.y = MAX(boxarr[matidx].upper.y, vert->y);
                    boxarr[matidx].upper.z = MAX(boxarr[matidx].upper.z, vert->z);
                    boxarr[matidx].lower.x = MIN(boxarr[matidx].lower.x, vert->x);
                    boxarr[matidx].lower.y = MIN(boxarr[matidx].lower.y, vert->y);
                    boxarr[matidx].lower.z = MIN(boxarr[matidx].lower.z, vert->z);
                }
                cntarr[matidx]++;
                currwt++;
                curridx >>= 8;
            }
            vert++;
            boneidx++;
            bonewt++;
        }

        xSphere* spharr = (xSphere*)boxarr;
        for (i = 0; i < numbones; i++) {
            spharr[i].center.x = 0.5f * (boxarr[i].upper.x + boxarr[i].lower.x);
            spharr[i].center.y = 0.5f * (boxarr[i].upper.y + boxarr[i].lower.y);
            spharr[i].center.z = 0.5f * (boxarr[i].upper.z + boxarr[i].lower.z);
            spharr[i].r = 0.0f;
        }

        boneidx = RpSkinGetVertexBoneIndices(skin);
        bonewt = RpSkinGetVertexBoneWeights(skin);
        vert = model->geometry->morphTarget->verts;
        for (i = 0; i < numVert; i++) {
            const F32* currwt = (const F32*)bonewt;
            F32 dist2;
            U32 curridx = *boneidx;
            for (j = 0; j < 4 && *currwt != 0.0f; j++) {
                U32 matidx = curridx & 0xFF;
                dist2 = xsqr(vert->x - spharr[matidx].center.x) + xsqr(vert->y - spharr[matidx].center.y) +
                        xsqr(vert->z - spharr[matidx].center.z);
                spharr[matidx].r = MAX(spharr[matidx].r, dist2);
                currwt++;
                curridx >>= 8;
            }
            vert++;
            boneidx++;
            bonewt++;
        }

        S32 numused = 0;
        const RwMatrix* skinmat = RpSkinGetSkinToBoneMatrices(skin);
        for (i = 0; i < numbones; i++) {
            if (cntarr[i]) {
                numused++;
                spharr[i].r = xsqrt(spharr[i].r);
                xMat4x3Toworld(&spharr[i].center, (const xMat4x3*)&skinmat[i], &spharr[i].center);
            }
        }

        U32 AnimBoundUsrDatArray[2] = { 'HIAB', 0 };
        S32 index = RpGeometryAddUserDataArray(model->geometry, (RwChar*)AnimBoundUsrDatArray, rpINTUSERDATA,
                                               numused * 5 + 5);
        usrdat = RpGeometryGetUserDataArray(model->geometry, index);

        S32* idata = (S32*)usrdat->data;
        S32* idata_boneidx = idata + 2;
        xSphere* idata_sphere = (xSphere*)(idata_boneidx + numused);
        idata[0] = numused;
        idata[1] = numused + 2;
        for (i = 0; i < numbones; i++) {
            if (cntarr[i]) {
                *idata_boneidx = i;
                *idata_sphere = spharr[i];
                idata_boneidx++;
                idata_sphere++;
            }
        }

        xMEMPOPTEMP(boxarr);
        xMEMPOPTEMP(cntarr);
    }

found:
    S32* idata = (S32*)usrdat->data;
    S32* idata_boneidx = idata + 2;
    S32* idata_sphstart = idata_boneidx + idata[0];
    S32* idata_sphactual = idata + idata[1];

    while ((U32)idata_sphstart & 0xF) {
        idata_sphstart++;
    }

    if (idata_sphstart != idata_sphactual) {
        S32 numel = idata[0] * 4;
        idata[1] = idata_sphstart - idata;
        if (idata_sphstart > idata_sphactual) {
            for (i = numel - 1; i >= 0; i--) {
                idata_sphstart[i] = idata_sphactual[i];
            }
        } else {
            for (i = 0; i < numel; i++) {
                idata_sphstart[i] = idata_sphactual[i];
            }
        }
    }
}

void iModelAnimBoundWorld(xBox* box, xBox* combinedBox, RpAtomic* model, RwMatrix* mat, RwMatrix* bonemat)
{
    S32 i, *data, count = RpGeometryGetUserDataArrayCount(model->geometry);
    RpUserDataArray* usrdat = NULL;
    for (i = 0; i < count; i++) {
        usrdat = RpGeometryGetUserDataArray(model->geometry, i);
        if (*(U32*)usrdat->name == 'HIAB') {
            break;
        }
    }
    data = (S32*)usrdat->data;
    S32 numbones = data[0];
    S32* boneidx = data + 2;
    xSphere* bonesph = (xSphere*)(data + data[1]);

    F32 maxscale;
    F32 scaleX = xsqr(mat->right.x) + xsqr(mat->right.y) + xsqr(mat->right.z);
    F32 scaleY = xsqr(mat->up.x) + xsqr(mat->up.y) + xsqr(mat->up.z);
    F32 scaleZ = xsqr(mat->at.x) + xsqr(mat->at.y) + xsqr(mat->at.z);
    maxscale = xsqrt(MAX3(scaleX, scaleY, scaleZ));

    box->upper.x = -1e38f;
    box->upper.y = -1e38f;
    box->upper.z = -1e38f;
    box->lower.x = 1e38f;
    box->lower.y = 1e38f;
    box->lower.z = 1e38f;

    for (i = 0; i < numbones; i++) {
        xVec3 sphcenter;
        xMat4x3Toworld(&sphcenter, (const xMat4x3*)&bonemat[*boneidx], &bonesph->center);
        xMat4x3Toworld(&sphcenter, (const xMat4x3*)mat, &sphcenter);

        F32 scalerad = bonesph->r * maxscale;

        box->upper.x = MAX(box->upper.x, sphcenter.x + scalerad);
        box->upper.y = MAX(box->upper.y, sphcenter.y + scalerad);
        box->upper.z = MAX(box->upper.z, sphcenter.z + scalerad);
        box->lower.x = MIN(box->lower.x, sphcenter.x - scalerad);
        box->lower.y = MIN(box->lower.y, sphcenter.y - scalerad);
        box->lower.z = MIN(box->lower.z, sphcenter.z - scalerad);

        bonesph++;
        boneidx++;
    }

    if (combinedBox) {
        if (combinedBox->upper.x == -1e38f) {
            *combinedBox = *box;
        } else {
            combinedBox->upper.x = MAX(combinedBox->upper.x, box->upper.x);
            combinedBox->upper.y = MAX(combinedBox->upper.y, box->upper.y);
            combinedBox->upper.z = MAX(combinedBox->upper.z, box->upper.z);
            combinedBox->lower.x = MIN(combinedBox->lower.x, box->lower.x);
            combinedBox->lower.y = MIN(combinedBox->lower.y, box->lower.y);
            combinedBox->lower.z = MIN(combinedBox->lower.z, box->lower.z);
        }
    }
}

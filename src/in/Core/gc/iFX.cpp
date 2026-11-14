#include "xCommon.h"
#include "iFX.h"

#include "xFX.h"

#include <rpworld.h>

static void* _iGCUVRenderCallback(void* object, RxGameCubePipeData* pipeData)
{
    U32 numMeshes;
    RxGameCubeVertexBuffer* vbHeader;
    RxGameCubeDisplayList* dList;
    RpMesh* mesh;
    RwMatrix* ltm;
    RxGameCubeMatFunc matFunc;

    vbHeader = (RxGameCubeVertexBuffer*)(pipeData->resEntry + 1);

    dList = (RxGameCubeDisplayList*)((RxGameCubeVertexAttr*)(vbHeader + 1) + (vbHeader->numAttrArrays - 1));

    vbHeader->token = _rwDlTokenGetCurrent();

    if (rpATOMIC == RwObjectGetType(object)) {
        RpAtomic* atomic = (RpAtomic*)object;

        ltm = RwFrameGetLTM(RpAtomicGetFrame(atomic));
        _rwDlVtxFmtSetup(RpGameCubeGeometryGetVtxFmt(RpAtomicGetGeometry(atomic)), pipeData);
    } else {
        ltm = NULL;
        _rwDlVtxFmtSetup(RpGameCubeWorldGetVtxFmt((RpWorld*)RWSRCGLOBAL(curWorld)), pipeData);
    }

    if (rxGEOMETRY_NORMALS & pipeData->flags) {
        _rwDlTransformSetup(ltm, TRUE);
    } else {
        _rwDlTransformSetup(ltm, FALSE);
    }

    matFunc = _rwDlObjectRenderSetup(pipeData->flags, pipeData->lightMask, pipeData->ambientLight, vbHeader->flags & 1);

    F32 tmtx[3][4] = {
        { xFXanimUVRotMat0[0], xFXanimUVRotMat0[1], xFXanimUVTrans[0], xFXanimUVScale[0] },
        { xFXanimUVRotMat1[0], xFXanimUVRotMat1[1], xFXanimUVTrans[1], xFXanimUVScale[1] }
    };
    GXLoadTexMtxImm(tmtx, GX_TEXMTX0, GX_MTX2x4);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);

    numMeshes = pipeData->meshHeader->numMeshes;
    mesh = (RpMesh*)(pipeData->meshHeader + 1);

    if ((rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2) & pipeData->flags) {
        while (numMeshes--) {
            RwTexture* texture = mesh->material->texture;
            _rwDlTextureSet(texture, 0);

            if (texture && texture->raster) {
                RwGameCubeRasterExtension* rasExt = RwGameCubeRasterGetExtension(RwRasterGetParent(texture->raster));

                _rwDlRenderStateSetZCompLoc((rasExt->flags & 1) ? FALSE : TRUE);
            }

            if (matFunc) {
                matFunc(&pipeData->ambientLightColor, &mesh->material->color, mesh->material->surfaceProps.ambient);
            }

            GXCallDisplayList(dList->displayList, dList->size);

            dList++;
            mesh++;
        }
    } else {
        while (numMeshes--) {
            if (matFunc) {
                matFunc(&pipeData->ambientLightColor, &mesh->material->color, mesh->material->surfaceProps.ambient);
            }

            GXCallDisplayList(dList->displayList, dList->size);

            dList++;
            mesh++;
        }
    }

    return object;
}

RxPipeline* iFXanimUVCreatePipe()
{
    RxPipeline* newPipe = NULL;

    newPipe = RxPipelineCreate();
    if (newPipe) {
        RxLockedPipe* lpipe = RxPipelineLock(newPipe);
        if (lpipe) {
            RxNodeDefinition* instanceNode = RxNodeDefinitionGetGameCubeAtomicAllInOne();

            lpipe = RxLockedPipeAddFragment(lpipe, NULL, instanceNode, NULL);

            lpipe = RxLockedPipeUnlock(lpipe);

            RxPipelineNode* node = RxPipelineFindNodeByName(newPipe, instanceNode->name, NULL, NULL);

            RxGameCubeAllInOneSetRenderCallBack(node, _iGCUVRenderCallback);

            return newPipe;
        }

        RxPipelineDestroy(newPipe);
        newPipe = NULL;
    }

    return newPipe;
}

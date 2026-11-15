#include "xCommon.h"
#include "iWater.h"

#include "xCam.h"
#include "xDebugTweak.h"
#include "xEnt.h"
#include "xFX.h"
#include "xLightKit.h"
#include "xMath.h"
#include "xMath3.h"
#include "xModel.h"
#include "xShadow.h"
#include "xstransvc.h"
#include "xutil.h"
#include "xVec3.h"
#include "iCamera.h"
#include "iDebug.h"
#include "iModel.h"
#include "zBase.h"
#include "zEntSimpleObj.h"
#include "zGlobals.h"
#include "zMain.h"
#include "zScene.h"
#include "zWater.h"
#include "water/fxembm.h"
#include "water/texconv.h"
#include "water/textures.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rpmatfx.h>

#define WATER_BLUE_GREEN_NJ_ID 0xA362C765 // xStrHash("water_blue_green_NJ")
#define GC_WATER_LIGHTKIT_ID 0x01092433 // xStrHash("gc_water_lightkit")

#define WATER_PRESET(width, shininess, height)  \
{                                               \
    BumpWidth = (width);                        \
    Shininess = (shininess);                    \
    BumpHeight = (height);                      \
}

struct bump_layer
{
    F32 u;
    F32 v;
    F32 us;
    F32 vs;
    F32 scale;
    U8 alpha;
    U8 pad[3];
};

S32 gGCwaterDisable;
RpLight* iWaterDirectionalLight;
FxParam* iWaterEMBM;
xTWEAKABLE F32 frameCountScale = 0.0015f;
xTWEAKABLE S32 bump_mode = 1;
xTWEAKABLE U8 dff_alpha = 128;
xTWEAKABLE S32 override_width = TRUE;
xTWEAKABLE F32 yscale = 1.0f;
xTWEAKABLE bump_layer bump_layers[4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0x60 },
    { 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 0x40 },
    { 0.25f, 0.1f, 1.0f, -1.0f, 3.0f, 0x20 },
    { 0.69f, 0.47f, 1.0f, 1.0f, 4.0f, 0x40 },
};
xTWEAKABLE S32 num_bump_layers = 4;
static RwTexture* bumpTex;
static RwRaster* bumpRaster;

static RpLight* CreateDirectionalLight()
{
    RpLight* light = RpLightCreate(rpLIGHTDIRECTIONAL);
    if (light) {
        RwFrame* frame = RwFrameCreate();
        if (frame) {
            RwRGBAReal color;

            color.red = color.green = color.blue = 1.0f;
            color.alpha = 1.0f;

            RpLightSetColor(light, &color);
            RpLightSetFrame(light, frame);

            return light;
        }

        RpLightDestroy(light);
    }

    return NULL;
}

static RwCamera* saveCamera;
static RwCamera* bumpCamera;
static RwCamera* clonedShadowCamera;

static void iWaterBumpGenRenderLayer(RwCamera* camera, RwTexture* texture, F32 u, F32 v, U8 alpha, F32 scale)
{
    RwIm2DVertex vx[4];
    F32 crw, crh;
    RwRaster* camRas;
    F32 recipZ;

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)RwTextureGetRaster(texture));

    camRas = RwCameraGetRaster(camera);
    crw = RwRasterGetWidth(camRas);
    crh = RwRasterGetHeight(camRas);

    recipZ = 1.0f / RwCameraGetNearClipPlane(camera);

    RwIm2DVertexSetScreenX(&vx[0], 0.0f);
    RwIm2DVertexSetScreenY(&vx[0], 0.0f);
    RwIm2DVertexSetScreenZ(&vx[0], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetRecipCameraZ(&vx[0], recipZ);
    RwIm2DVertexSetIntRGBA(&vx[0], alpha, alpha, alpha, 255);
    RwIm2DVertexSetU(&vx[0], u, recipZ);
    RwIm2DVertexSetV(&vx[0], v, recipZ);

    RwIm2DVertexSetScreenX(&vx[1], 0.0f);
    RwIm2DVertexSetScreenY(&vx[1], crh);
    RwIm2DVertexSetScreenZ(&vx[1], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetRecipCameraZ(&vx[1], recipZ);
    RwIm2DVertexSetIntRGBA(&vx[1], alpha, alpha, alpha, 255);
    RwIm2DVertexSetU(&vx[1], u, recipZ);
    RwIm2DVertexSetV(&vx[1], v + scale, recipZ);

    RwIm2DVertexSetScreenX(&vx[2], crw);
    RwIm2DVertexSetScreenY(&vx[2], 0.0f);
    RwIm2DVertexSetScreenZ(&vx[2], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetRecipCameraZ(&vx[2], recipZ);
    RwIm2DVertexSetIntRGBA(&vx[2], alpha, alpha, alpha, 255);
    RwIm2DVertexSetU(&vx[2], u + scale, recipZ);
    RwIm2DVertexSetV(&vx[2], v, recipZ);

    RwIm2DVertexSetScreenX(&vx[3], crw);
    RwIm2DVertexSetScreenY(&vx[3], crh);
    RwIm2DVertexSetScreenZ(&vx[3], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetRecipCameraZ(&vx[3], recipZ);
    RwIm2DVertexSetIntRGBA(&vx[3], alpha, alpha, alpha, 255);
    RwIm2DVertexSetU(&vx[3], u + scale, recipZ);
    RwIm2DVertexSetV(&vx[3], v + scale, recipZ);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vx, 4);
}

static S32 iWaterBumpGenBegin(RwCamera* bumpCamera, RwRGBA* bgColor)
{
    saveCamera = RwCameraGetCurrentCamera();
    if (saveCamera) {
        RwCameraEndUpdate(saveCamera);
    }

    if (bgColor) {
        RwCameraClear(bumpCamera, bgColor, rwCAMERACLEARIMAGE);
    }

    if (!RwCameraBeginUpdateWrapper(bumpCamera)) {
        return FALSE;
    }

    GXSetDither(GX_FALSE);

    return TRUE;
}

static S32 iWaterUpdateEMBM(F32, S32 check);

static void iWaterBumpGenEnd()
{
    GXSetDither(GX_TRUE);

    RwCameraEndUpdate(bumpCamera);

    RwGameCubeCameraTextureFlush(RwCameraGetRaster(bumpCamera), FALSE);
    GXPixModeSync();

    if (saveCamera) {
        RwCameraBeginUpdateWrapper(saveCamera);
    }

    iWaterUpdateEMBM(xglobals->update_dt, FALSE);
}

static RwTexture* iWaterBumpGen(RwTexture* bump, RwTexture* src)
{
    if (!bump) {
        return NULL;
    }

    F32 foo = xfmod(gFrameCount * frameCountScale, 1.0f);

    RwRGBA black = { 0, 0, 0, 255 };

    if (!clonedShadowCamera) {
        clonedShadowCamera = RwCameraClone(ShadowCamera);
        RwCameraSetRaster(clonedShadowCamera, bumpRaster);
    }

    bumpCamera = clonedShadowCamera;

    if (iWaterBumpGenBegin(bumpCamera, &black)) {
        xTWEAKABLE bump_layer* bp = bump_layers;
        for (S32 i = 0; i < num_bump_layers; i++, bp++) {
            if (bp->alpha) {
                iWaterBumpGenRenderLayer(bumpCamera, src, bp->u + bp->us * foo, bp->v + bp->vs * foo, bp->alpha, bp->scale);
            }
        }

        iWaterBumpGenEnd();
    }

    return bump;
}

void iWaterBumpGen()
{
    if (bump_mode == 1)
        iWaterBumpGen(bumpTex, iWaterBumpTex);
}

void iWaterCreateMTEffect()
{
    if (!gGCwaterDisable)
        FxOpen();
}

static xLightKit* waterLightKit;
static xTWEAKABLE RwRGBAReal waterHackColor = { 1.0f, 1.0f, 1.0f, 1.0f };

static void iWaterObjectRender(xEnt* ent)
{
    if (!ent->model || !xEntIsVisible(ent)) {
        return;
    }

    if (waterLightKit)
        xModelSetLightKit(ent->model, waterLightKit);
    ent->model->RedMultiplier = waterHackColor.red;
    ent->model->GreenMultiplier = waterHackColor.green;
    ent->model->BlueMultiplier = waterHackColor.blue;
    ent->model->Alpha = waterHackColor.alpha;

    xModelRender(ent->model);
}

xEnt* waterObjects[8];
U32 numWaterObjects;
static RpAtomicCallBackRender defaultRenderCB;

static RpAtomic* iWaterAtomicRender(RpAtomic* atomic)
{
    return defaultRenderCB(atomic);
}

U32 waterTexID = WATER_BLUE_GREEN_NJ_ID;

#pragma push
#pragma optimization_level 0
static void iWaterObjectSetup(xEnt* ent)
{
    RpAtomic* atomic = ent->model->Data;
    RpGeometry* geom = RpAtomicGetGeometry(atomic);

    RwTexture* newtex = (RwTexture*)xSTFindAsset(waterTexID, NULL);
    if (newtex) {
        for (S32 i = 0; i < geom->matList.numMaterials; i++) {
            RpMaterial* material = geom->matList.materials[i];
            RwTexture* tex = RpMaterialGetTexture(material);
            if (tex != newtex) {
                PKRAssetTOCInfo ainfo;
                xSTRandMemToAsset(&ainfo, tex);

                RpMaterialSetTexture(material, newtex);
            }
        }
    }
}
#pragma pop

void iWaterSceneEnter(RpWorld*)
{
    zScene& scene = *globals.sceneCur;
    zWaterBody* it = (zWaterBody*)scene.baseList[eBaseTypeWaterBody], *end = it + scene.baseCount[eBaseTypeWaterBody];

    numWaterObjects = 0;
    for (; it < end; ++it) {
        U32 body = it->get_bodyID();
        zEntSimpleObj* ent = (zEntSimpleObj*)zSceneFindObject(it->get_bodyID());
        OSReport("iWaterSceneEnter:  water body %08X (%s)\n", body, xSTAssetName(body));
        if (ent) {
            waterObjects[numWaterObjects] = ent;
            numWaterObjects++;
        }
    }

    if (numWaterObjects == 0) {
        gGCwaterDisable = TRUE;
        return;
    }

    S32 flags = rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888 | rwRASTERFORMATAUTOMIPMAP | rwRASTERFORMATMIPMAP;

    bumpRaster = RwRasterCreate(64, 64, 32, flags);
    if (bumpRaster)
        bumpTex = RwTextureCreate(bumpRaster);

    iWaterDirectionalLight = CreateDirectionalLight();
    if (!iWaterDirectionalLight) {
        return;
    }

    iWaterCreateMTEffect();

    for (S32 i = 0; i < numWaterObjects; i++) {
        xEnt* ent = waterObjects[i];

        if (!ent || !ent->model || !ent->model->Mat) {
            continue;
        }

        RwMatrix newMat;
        RwMatrixSetIdentity(&newMat);
        // BUG: arguments swapped (Rat: RwMatrixOrthoNormalize(&newMat, Mat))
        RwMatrixOrthoNormalize(ent->model->Mat, &newMat);
        *ent->model->Mat = newMat;

        iWaterObjectSetup(ent);
    }

    iWaterReset();

    switch (globals.sceneCur->sceneID) {
    case IDTAG('N','J','0','1'):
        WATER_PRESET(0.105f, 0.2f, 1.0f);
        break;
    case IDTAG('N','J','0','2'):
        WATER_PRESET(0.3f, 0.12f, 1.0f);
        break;
    case IDTAG('H','S','0','1'):
    case IDTAG('F','T','0','4'):
    default:
        WATER_PRESET(0.045f, 0.4f, 1.0f);
        break;
    case IDTAG('F','T','0','1'):
    case IDTAG('N','I','0','3'):
        WATER_PRESET(0.045f, 0.15f, 1.0f);
        break;
    }
}

void iWaterSceneExit()
{
    if (!gGCwaterDisable) {
        FxClose();
    }

    gGCwaterDisable = FALSE;
    waterLightKit = NULL;
    numWaterObjects = 0;

    if (clonedShadowCamera) {
        RwCameraDestroy(clonedShadowCamera);
        clonedShadowCamera = NULL;
        bumpCamera = NULL;
    }

    if (bumpTex) {
        RwTextureDestroy(bumpTex);
        bumpTex = NULL;
    }

    if (iWaterDirectionalLight) {
        RwFrame* frame = RpLightGetFrame(iWaterDirectionalLight);

        if (frame) {
            RwFrameDestroy(frame);
        }

        RpLightDestroy(iWaterDirectionalLight);
        iWaterDirectionalLight = NULL;
    }
}

xTWEAKABLE xVec3 camscale = { 0.0f, 0.0f, 0.0f };

static S32 iWaterUpdateEMBM(F32, S32 check)
{
    if (!check) {
        RpMultiTexture* mt;
        RpGameCubeMTConfig* config;
        U32 rasSize;
        F32 scale;

        RwTexture* bump = NULL;
        RwTexture* env = iWaterEnvTex;
        if (bump_mode == 1) {
            bump = bumpTex;
        }

        static xTWEAKABLE F32 uvscale = 1280.0f;
        static xTWEAKABLE F32 range_scale = 1.0f;

        if (bump == bumpTex) {
            TextureDiffBumpRGBA8(bumpTex, range_scale);
        } else {
            TextureDiffBumpRGBA8(bump, bumpTex);
            bump = bumpTex;
        }

        if (!env || !bump)
            return FALSE;
        RwTextureSetFilterMode(bump, rwFILTERLINEARMIPLINEAR);
        RwTextureSetAddressing(bump, rwTEXTUREADDRESSWRAP);
        RwTextureSetFilterMode(env, rwFILTERLINEARMIPLINEAR);
        RwTextureSetAddressing(env, rwTEXTUREADDRESSWRAP);

        mt = RpMaterialGetMultiTexture(EMBM.material, rwID_GAMECUBE);
        config = RpGameCubeMTEffectGetConfig(EMBM.effect);
        RpMultiTextureSetTexture(mt, 1, bump);
        RpMultiTextureSetTexture(mt, 2, env);
        rasSize = RwRasterGetWidth(RwTextureGetRaster(bump));

        scale = BumpWidth * (uvscale / rasSize);

        config->texMtx[0].data[0][0] = scale;
        config->texMtx[0].data[1][1] = scale;
        config->texMtx[1].data[0][0] = 0.5f * -BumpHeight;
        config->texMtx[1].data[1][1] = 0.5f * -BumpHeight;
        config->kreg[0].alpha = 255.0f * Shininess;

        RwFrame* frame = RpLightGetFrame(iWaterDirectionalLight);

        xVec3 at;
        at = g_O3;
        xVec3AddScaled(&at, &g_Y3, yscale);
        xVec3AddScaled(&at, (xVec3*)&xglobals->screen->icam->viewMatrix.right, camscale.x);
        xVec3AddScaled(&at, (xVec3*)&xglobals->screen->icam->viewMatrix.up, camscale.y);
        xVec3AddScaled(&at, (xVec3*)&xglobals->screen->icam->viewMatrix.at, camscale.z);
        xVec3Normalize(&at, &at);
        xMat3x3LookVec((xMat3x3*)RwFrameGetLTM(frame), &at);
        FxFrameArray[0] = frame;
        config->texMtx[1].refFrame = rpGAMECUBETEXFRAME_MISC0;
        config->texMtx[2].refFrame = rpGAMECUBETEXFRAME_MISC0;

        FxFrameArray[1] = NULL;
        RpGameCubeMTSetMiscFrameArray(FxFrameArray, NUM_FX_FRAMES);

        if (EnvMode == 0) {
            config->tevStages[3].op.colorA = rpGX_CC_CPREV;
            config->tevStages[3].op.colorB = rpGX_CC_TEXC;
            config->tevStages[3].op.colorC = rpGX_CC_KONST;
            config->tevStages[3].op.colorD = rpGX_CC_ZERO;

            config->tevStages[2].op.colorSel = rpGX_TEV_KCSEL_1;
            config->tevStages[2].op.colorScale = rpGX_CS_SCALE_1;
        } else {
            config->tevStages[3].op.colorA = rpGX_CC_ZERO;
            config->tevStages[3].op.colorB = rpGX_CC_TEXC;
            config->tevStages[3].op.colorC = rpGX_CC_CPREV;
            config->tevStages[3].op.colorD = rpGX_CC_ZERO;

            config->tevStages[2].op.colorSel = rpGX_TEV_KCSEL_K0_A;
            config->tevStages[2].op.colorScale = rpGX_CS_SCALE_4;
        }
    }

    return TRUE;
}

U32 enableWater = TRUE;

void iWaterEnable(RpAtomic* atomic)
{
    if (gGCwaterDisable) {
        return;
    }

    if (enableWater) {
        AtomicSetupEffect(atomic, iWaterEMBM->material);
    } else {
        AtomicDisableMatFX(atomic);
    }
}

xTWEAKABLE xVec3 NJ02start = { -269.0f, 6.8f, -162.0f };

void iWaterUpdate(F32)
{
    if (gGCwaterDisable) {
        return;
    }

    if (override_width) {
        switch (globals.sceneCur->sceneID) {
        case IDTAG('N','J','0','2'):
            if (xVec3Dist2(xEntGetPos(ZPLAYER), &NJ02start) < 625.0f) {
                WATER_PRESET(0.3f, 0.12f, 1.0f);
            } else {
                WATER_PRESET(0.045f, 0.15f, 1.0f);
            }
            break;
        }
    }

    for (S32 i = 0; i < numWaterObjects; i++) {
        iModelSetMaterialAlpha(waterObjects[i]->model->Data, dff_alpha);
        iWaterEnable(waterObjects[i]->model->Data);
        iWaterObjectSetup(waterObjects[i]);
    }
}

void iWaterReset()
{
    if (gGCwaterDisable) {
        return;
    }

    waterLightKit = (xLightKit*)xSTFindAsset(GC_WATER_LIGHTKIT_ID, NULL);
    if (!waterLightKit)
        return;

    for (S32 i = 0; i < numWaterObjects; i++) {
        zEntSimpleObj* ent = (zEntSimpleObj*)waterObjects[i];

        zScene* s = globals.sceneCur;

        iprintf("Setting water light kit on object %08X (%s) model %08X (%s)\n", ent->id, xSTAssetName(ent->id), ent->model->modelID, xSTAssetName(ent->model->modelID));

        xModelSetLightKit(ent->model, waterLightKit);
        ent->sflags |= 0x8;
        ent->render = iWaterObjectRender;

        if (!defaultRenderCB) {
            defaultRenderCB = RpAtomicGetRenderCallBack(ent->model->Data);
        }

        RpAtomicSetRenderCallBack(ent->model->Data, iWaterAtomicRender);
    }
}

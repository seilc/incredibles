#include "water/fxembm.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rpmatfx.h>
#include <string.h>

static RpMTEffect* CreateEMBMEffect()
{
    RpMTEffect* effect = RpGameCubeMTEffectCreate(4, 5, 3, 1, 1);
    RpMTEffectSetName(effect, "EMBM.mte");

    RpGameCubeMTConfig* config = RpGameCubeMTEffectGetConfig(effect);

    config->flags = rpGAMECUBEMTCONFIG_REG | rpGAMECUBEMTCONFIG_KREG;
    config->kreg[0].alpha = 128;
    config->reg[0].r = 250;
    config->reg[0].g = 200;
    config->reg[0].b = 100;

    RwReal mtx[3][2][4] = {
        { { 2.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 2.0f, 0.0f, 0.0f } },
        { { -0.5f, 0.0f, 0.0f, 0.0f }, { 0.0f, -0.5f, 0.0f, 0.0f } },
        { { -0.5f, 0.0f, 0.0f, 0.5f }, { 0.0f, -0.5f, 0.0f, 0.5f } },
    };

    config->numTexMtx = 3;

    config->texMtx[0].offset = rpGX_TEXMTX0;
    config->texMtx[0].flags = rpGAMECUBETEXMTX_2X4;
    config->texMtx[0].refFrame = rpGAMECUBETEXFRAME_NONE;
    memcpy(config->texMtx[0].data, mtx[0], sizeof(mtx[0]));

    config->texMtx[1].offset = rpGX_TEXMTX1;
    config->texMtx[1].flags = rpGAMECUBETEXMTX_2X4;
    config->texMtx[1].refFrame = rpGAMECUBETEXFRAME_CAMERA;
    memcpy(config->texMtx[1].data, mtx[1], sizeof(mtx[1]));

    config->texMtx[2].offset = rpGX_TEXMTX2;
    config->texMtx[2].flags = rpGAMECUBETEXMTX_2X4;
    config->texMtx[2].refFrame = rpGAMECUBETEXFRAME_CAMERA;
    memcpy(config->texMtx[2].data, mtx[2], sizeof(mtx[2]));

    RpGameCubeTexGen texGens[5] = {
        { 0, rpGX_TG_MTX2x4, rpGX_TG_TEX0, rpGX_IDENTITY, rpGX_PTIDENTITY, 0, 0 },
        { 0, rpGX_TG_MTX2x4, rpGX_TG_TEX0, rpGX_TEXMTX0, rpGX_PTIDENTITY, 0, 0 },
        { 0, rpGX_TG_MTX2x4, rpGX_TG_BINRM, rpGX_TEXMTX1, rpGX_PTIDENTITY, 0, 0 },
        { 0, rpGX_TG_MTX2x4, rpGX_TG_TANGENT, rpGX_TEXMTX1, rpGX_PTIDENTITY, 0, 0 },
        { 0, rpGX_TG_MTX2x4, rpGX_TG_NRM, rpGX_TEXMTX2, rpGX_PTIDENTITY, 0, 0 },
    };

    config->numTexGens = 5;
    memcpy(config->texGens, texGens, sizeof(texGens));

    config->numIndMtx = 1;
    memset(config->indMtx, 0, sizeof(RpGameCubeIndStage));
    config->indMtx->id = rpGX_ITM_0;
    config->indMtx->flags = 0;
    config->indMtx->refFrame = rpGAMECUBETEXFRAME_NONE;
    config->indMtx->scale = 1;

    config->numIndStages = 1;
    config->indStages->texMapID = rpGX_TEXMAP1;
    config->indStages->texCoordID = rpGX_TEXCOORD1;
    config->indStages->scaleS = rpGX_ITS_1;
    config->indStages->scaleT = rpGX_ITS_1;
    config->indStages->flags = 0;

    RwUInt32 i;

    RpGameCubeTevOp tevBlendBase = {
        rpGX_CC_ZERO, rpGX_CC_RASC, rpGX_CC_TEXC, rpGX_CC_ZERO,
        rpGX_CA_ZERO, rpGX_CA_ZERO, rpGX_CA_ZERO, rpGX_CA_ZERO,
        rpGX_TEV_ADD, rpGX_TB_ZERO, rpGX_CS_SCALE_1, TRUE, rpGX_TEVPREV,
        rpGX_TEV_ADD, rpGX_TB_ZERO, rpGX_CS_SCALE_1, TRUE, rpGX_TEVPREV,
        rpGX_TEV_KCSEL_K0_A, rpGX_TEV_KASEL_1
    };

    RpGameCubeTevOp tevBlendMod1 = {
        rpGX_CC_ZERO, rpGX_CC_RASA, rpGX_CC_CPREV, rpGX_CC_CPREV,
        rpGX_CA_ZERO, rpGX_CA_ZERO, rpGX_CA_ZERO, rpGX_CA_ZERO,
        rpGX_TEV_ADD, rpGX_TB_ZERO, rpGX_CS_DIVIDE_2, TRUE, rpGX_TEVPREV,
        rpGX_TEV_ADD, rpGX_TB_ZERO, rpGX_CS_SCALE_1, TRUE, rpGX_TEVPREV,
        rpGX_TEV_KCSEL_K0_A, rpGX_TEV_KASEL_1
    };

    RpGameCubeTevOp tevBlendMod2 = {
        rpGX_CC_ZERO, rpGX_CC_CPREV, rpGX_CC_KONST, rpGX_CC_ZERO,
        rpGX_CA_ZERO, rpGX_CA_ZERO, rpGX_CA_ZERO, rpGX_CA_ZERO,
        rpGX_TEV_ADD, rpGX_TB_ZERO, rpGX_CS_SCALE_1, TRUE, rpGX_TEVPREV,
        rpGX_TEV_ADD, rpGX_TB_ZERO, rpGX_CS_SCALE_1, TRUE, rpGX_TEVPREV,
        rpGX_TEV_KCSEL_1, rpGX_TEV_KASEL_1
    };

    RpGameCubeTevOp tevBlendEnv = {
        rpGX_CC_CPREV, rpGX_CC_TEXC, rpGX_CC_KONST, rpGX_CC_ZERO,
        rpGX_CA_KONST, rpGX_CA_ZERO, rpGX_CA_ZERO, rpGX_CA_ZERO,
        rpGX_TEV_ADD, rpGX_TB_ZERO, rpGX_CS_SCALE_1, TRUE, rpGX_TEVPREV,
        rpGX_TEV_ADD, rpGX_TB_ADDHALF, rpGX_CS_DIVIDE_2, TRUE, rpGX_TEVPREV,
        rpGX_TEV_KCSEL_K0_A, rpGX_TEV_KASEL_1
    };

    RpGameCubeTevInd tevInd = {
        rpGX_INDTEXSTAGE0, rpGX_ITF_8, rpGX_ITB_ST, rpGX_ITM_S0, rpGX_ITW_0, rpGX_ITW_0, FALSE, FALSE, rpGX_ITBA_U
    };

    RpGameCubeTevInd tevIndAddPrev = {
        rpGX_INDTEXSTAGE0, rpGX_ITF_8, rpGX_ITB_NONE, rpGX_ITM_OFF, rpGX_ITW_OFF, rpGX_ITW_OFF, TRUE, FALSE, rpGX_ITBA_U
    };

    i = 0;

    config->tevStages[i].op = tevBlendBase;
    config->tevStages[i].texCoordID = rpGX_TEXCOORD0;
    config->tevStages[i].texMapID = rpGX_TEXMAP0;
    config->tevStages[i].channelID = rpGX_COLOR0A0;
    config->tevStages[i].flags = 0;
    config->tevStages[i].indirect = 0;
    i++;

    config->tevStages[i].op = tevBlendMod1;
    config->tevStages[i].texCoordID = rpGX_TEXCOORD2;
    config->tevStages[i].texMapID = (RpGXTexMapID)(rpGX_TEXMAP2 | rpGX_TEX_DISABLE);
    config->tevStages[i].channelID = rpGX_ALPHA_BUMP;
    config->tevStages[i].flags = 0;
    RpGameCubeTevIndPack(&config->tevStages[i].indirect, &tevInd);
    i++;

    config->tevStages[i].op = tevBlendMod2;
    config->tevStages[i].texCoordID = rpGX_TEXCOORD3;
    config->tevStages[i].texMapID = (RpGXTexMapID)(rpGX_TEXMAP2 | rpGX_TEX_DISABLE);
    config->tevStages[i].channelID = rpGX_COLOR0A0;
    config->tevStages[i].flags = 0;
    tevInd.matrixSel = rpGX_ITM_T0;
    tevInd.addPrev = TRUE;
    RpGameCubeTevIndPack(&config->tevStages[i].indirect, &tevInd);
    i++;

    config->tevStages[i].op = tevBlendEnv;
    config->tevStages[i].texCoordID = rpGX_TEXCOORD4;
    config->tevStages[i].texMapID = rpGX_TEXMAP2;
    config->tevStages[i].channelID = rpGX_COLOR_NULL;
    config->tevStages[i].flags = 0;
    RpGameCubeTevIndPack(&config->tevStages[i].indirect, &tevIndAddPrev);
    i++;

    config->numTevStages = i;

    return effect;
}

FxParam EMBM;
F32 BumpHeight = 0.05f;
F32 BumpWidth = 2.0f;
F32 Shininess = 0.08f;
S32 EnvMode;

static void EffectSet()
{
}

static void EffectUnset()
{
}

static void EffectClose()
{
    if (EMBM.material) {
        RpMaterialDestroy(EMBM.material);
        EMBM.material = NULL;

        if (EMBM.effect) {
            RpMTEffectDestroy(EMBM.effect);
            EMBM.effect = NULL;
        }
    }
}

FxParam* FxEMBMOpen()
{
    EMBM.set = EffectSet;
    EMBM.unset = EffectUnset;
    EMBM.close = EffectClose;

    EMBM.material = RpMaterialCreate();
    if (EMBM.material && RpMaterialCreateMultiTexture(EMBM.material, rwID_GAMECUBE, 3)) {
        RpMultiTexture* mt = RpMaterialGetMultiTexture(EMBM.material, rwID_GAMECUBE);

        RpMultiTextureSetCoords(mt, 0, 0);
        RpMultiTextureSetCoords(mt, 1, 0);

        EMBM.effect = CreateEMBMEffect();
        if (EMBM.effect) {
            RpMultiTextureSetEffect(mt, EMBM.effect);
            RpMTEffectDestroy(EMBM.effect);
            return &EMBM;
        }
    }

    return NULL;
}

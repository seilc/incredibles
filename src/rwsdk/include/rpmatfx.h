#ifndef RPMATFX_H
#define RPMATFX_H

#include "rwcore.h"
#include "rpworld.h"

typedef struct RpGXColorS10 RpGXColorS10;
typedef struct RpGameCubeIndMtx RpGameCubeIndMtx;
typedef struct RpGameCubeIndStage RpGameCubeIndStage;
typedef struct RpGameCubeMTConfig RpGameCubeMTConfig;
typedef struct RpGameCubeTevInd RpGameCubeTevInd;
typedef struct RpGameCubeTevOp RpGameCubeTevOp;
typedef struct RpGameCubeTevStage RpGameCubeTevStage;
typedef struct RpGameCubeTexGen RpGameCubeTexGen;
typedef struct RpGameCubeTexMtx RpGameCubeTexMtx;
typedef struct RpMTEffect RpMTEffect;

typedef struct RpMultiTexture RpMultiTexture;

typedef RpMaterial* (*RpGameCubeMTCallBack)(RpMaterial* material, void* object, RxGameCubePipeData* pipeData, void* data);

enum RpGXTexMtx
{
    rpGX_TEXMTX0 = 30,
    rpGX_TEXMTX1 = 33,
    rpGX_TEXMTX2 = 36,
    rpGX_TEXMTX3 = 39,
    rpGX_TEXMTX4 = 42,
    rpGX_TEXMTX5 = 45,
    rpGX_TEXMTX6 = 48,
    rpGX_TEXMTX7 = 51,
    rpGX_TEXMTX8 = 54,
    rpGX_TEXMTX9 = 57,
    rpGX_IDENTITY = 60,
    rpGX_PTTEXMTX0 = 64,
    rpGX_PTTEXMTX1 = 67,
    rpGX_PTTEXMTX2 = 70,
    rpGX_PTTEXMTX3 = 73,
    rpGX_PTTEXMTX4 = 76,
    rpGX_PTTEXMTX5 = 79,
    rpGX_PTTEXMTX6 = 82,
    rpGX_PTTEXMTX7 = 85,
    rpGX_PTTEXMTX8 = 88,
    rpGX_PTTEXMTX9 = 91,
    rpGX_PTTEXMTX10 = 94,
    rpGX_PTTEXMTX11 = 97,
    rpGX_PTTEXMTX12 = 100,
    rpGX_PTTEXMTX13 = 103,
    rpGX_PTTEXMTX14 = 106,
    rpGX_PTTEXMTX15 = 109,
    rpGX_PTTEXMTX16 = 112,
    rpGX_PTTEXMTX17 = 115,
    rpGX_PTTEXMTX18 = 118,
    rpGX_PTTEXMTX19 = 121,
    rpGX_PTIDENTITY = 125,
};
typedef enum RpGXTexMtx RpGXTexMtx;

enum RpGameCubeMTConfigFlag
{
    rpGAMECUBEMTCONFIG_REG = 0x1,
    rpGAMECUBEMTCONFIG_KREG = 0x2,
    rpGAMECUBEMTCONFIG_BLEND = 0x4,
    rpGAMECUBEMTCONFIG_ALPHACOMP = 0x8,
    rpGAMECUBEMTCONFIG_AUTOLIGHT = 0x10,
    rpGAMECUBEMTCONFIG_ENVMATRIX = 0x20,
    rpGAMECUBEMTCONFIG_FORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpGameCubeMTConfigFlag RpGameCubeMTConfigFlag;

enum RpGameCubeTexMtxFlag
{
    rpGAMECUBETEXMTX_2X4 = 0x1,
    rpGAMECUBETEXMTX_USEFRAMEPOS = 0x2,
    rpGAMECUBETEXMTX_FORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpGameCubeTexMtxFlag RpGameCubeTexMtxFlag;

enum RpMatFXMaterialFlags
{
    rpMATFXEFFECTNULL,
    rpMATFXEFFECTBUMPMAP,
    rpMATFXEFFECTENVMAP,
    rpMATFXEFFECTBUMPENVMAP,
    rpMATFXEFFECTDUAL,
    rpMATFXEFFECTUVTRANSFORM,
    rpMATFXEFFECTDUALUVTRANSFORM,
    rpMATFXEFFECTMAX,
    rpMATFXNUMEFFECTS = 6,
    rpMATFXFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpMatFXMaterialFlags RpMatFXMaterialFlags;

enum RpGXChannelID
{
    rpGX_COLOR0 = 0x0,
    rpGX_COLOR1 = 0x1,
    rpGX_ALPHA0 = 0x2,
    rpGX_ALPHA1 = 0x3,
    rpGX_COLOR0A0 = 0x4,
    rpGX_COLOR1A1 = 0x5,
    rpGX_COLOR_ZERO = 0x6,
    rpGX_ALPHA_BUMP = 0x7,
    rpGX_ALPHA_BUMPN = 0x8,
    rpGX_COLOR_NULL = 0xFF,
};
typedef enum RpGXChannelID RpGXChannelID;

struct RpGXColorS10
{
    RwInt16 r;
    RwInt16 g;
    RwInt16 b;
    RwInt16 a;
};

enum RpGXIndTexAlphaSel
{
    rpGX_ITBA_OFF,
    rpGX_ITBA_S,
    rpGX_ITBA_T,
    rpGX_ITBA_U,
};
typedef enum RpGXIndTexAlphaSel RpGXIndTexAlphaSel;

enum RpGXIndTexBiasSel
{
    rpGX_ITB_NONE,
    rpGX_ITB_S,
    rpGX_ITB_T,
    rpGX_ITB_ST,
    rpGX_ITB_U,
    rpGX_ITB_SU,
    rpGX_ITB_TU,
    rpGX_ITB_STU,
};
typedef enum RpGXIndTexBiasSel RpGXIndTexBiasSel;

enum RpGXIndTexFormat
{
    rpGX_ITF_8,
    rpGX_ITF_5,
    rpGX_ITF_4,
    rpGX_ITF_3,
};
typedef enum RpGXIndTexFormat RpGXIndTexFormat;

enum RpGXIndTexMtxID
{
    rpGX_ITM_OFF,
    rpGX_ITM_0,
    rpGX_ITM_1,
    rpGX_ITM_2,
    rpGX_ITM_S0 = 5,
    rpGX_ITM_S1,
    rpGX_ITM_S2,
    rpGX_ITM_T0 = 9,
    rpGX_ITM_T1,
    rpGX_ITM_T2,
};
typedef enum RpGXIndTexMtxID RpGXIndTexMtxID;

enum RpGXIndTexScale
{
    rpGX_ITS_1,
    rpGX_ITS_2,
    rpGX_ITS_4,
    rpGX_ITS_8,
    rpGX_ITS_16,
    rpGX_ITS_32,
    rpGX_ITS_64,
    rpGX_ITS_128,
    rpGX_ITS_256,
};
typedef enum RpGXIndTexScale RpGXIndTexScale;

enum RpGXIndTexStageID
{
    rpGX_INDTEXSTAGE0,
    rpGX_INDTEXSTAGE1,
    rpGX_INDTEXSTAGE2,
    rpGX_INDTEXSTAGE3,
};
typedef enum RpGXIndTexStageID RpGXIndTexStageID;

enum RpGXIndTexWrap
{
    rpGX_ITW_OFF,
    rpGX_ITW_256,
    rpGX_ITW_128,
    rpGX_ITW_64,
    rpGX_ITW_32,
    rpGX_ITW_16,
    rpGX_ITW_0,
};
typedef enum RpGXIndTexWrap RpGXIndTexWrap;

enum RpGXTevAlphaArg
{
    rpGX_CA_APREV,
    rpGX_CA_A0,
    rpGX_CA_A1,
    rpGX_CA_A2,
    rpGX_CA_TEXA,
    rpGX_CA_RASA,
    rpGX_CA_KONST,
    rpGX_CA_ZERO,
};
typedef enum RpGXTevAlphaArg RpGXTevAlphaArg;

enum RpGXTevBias
{
    rpGX_TB_ZERO,
    rpGX_TB_ADDHALF,
    rpGX_TB_SUBHALF,
    rpGX_MAX_TEVBIAS,
};
typedef enum RpGXTevBias RpGXTevBias;

enum RpGXTevColorArg
{
    rpGX_CC_CPREV,
    rpGX_CC_APREV,
    rpGX_CC_C0,
    rpGX_CC_A0,
    rpGX_CC_C1,
    rpGX_CC_A1,
    rpGX_CC_C2,
    rpGX_CC_A2,
    rpGX_CC_TEXC,
    rpGX_CC_TEXA,
    rpGX_CC_RASC,
    rpGX_CC_RASA,
    rpGX_CC_ONE,
    rpGX_CC_HALF,
    rpGX_CC_KONST,
    rpGX_CC_ZERO,
};
typedef enum RpGXTevColorArg RpGXTevColorArg;

enum RpGXTevKAlphaSel
{
    rpGX_TEV_KASEL_1 = 0x0,
    rpGX_TEV_KASEL_7_8 = 0x1,
    rpGX_TEV_KASEL_3_4 = 0x2,
    rpGX_TEV_KASEL_5_8 = 0x3,
    rpGX_TEV_KASEL_1_2 = 0x4,
    rpGX_TEV_KASEL_3_8 = 0x5,
    rpGX_TEV_KASEL_1_4 = 0x6,
    rpGX_TEV_KASEL_1_8 = 0x7,
    rpGX_TEV_KASEL_K0_R = 0x10,
    rpGX_TEV_KASEL_K1_R = 0x11,
    rpGX_TEV_KASEL_K2_R = 0x12,
    rpGX_TEV_KASEL_K3_R = 0x13,
    rpGX_TEV_KASEL_K0_G = 0x14,
    rpGX_TEV_KASEL_K1_G = 0x15,
    rpGX_TEV_KASEL_K2_G = 0x16,
    rpGX_TEV_KASEL_K3_G = 0x17,
    rpGX_TEV_KASEL_K0_B = 0x18,
    rpGX_TEV_KASEL_K1_B = 0x19,
    rpGX_TEV_KASEL_K2_B = 0x1A,
    rpGX_TEV_KASEL_K3_B = 0x1B,
    rpGX_TEV_KASEL_K0_A = 0x1C,
    rpGX_TEV_KASEL_K1_A = 0x1D,
    rpGX_TEV_KASEL_K2_A = 0x1E,
    rpGX_TEV_KASEL_K3_A = 0x1F,
};
typedef enum RpGXTevKAlphaSel RpGXTevKAlphaSel;

enum RpGXTevKColorSel
{
    rpGX_TEV_KCSEL_1 = 0x0,
    rpGX_TEV_KCSEL_7_8 = 0x1,
    rpGX_TEV_KCSEL_3_4 = 0x2,
    rpGX_TEV_KCSEL_5_8 = 0x3,
    rpGX_TEV_KCSEL_1_2 = 0x4,
    rpGX_TEV_KCSEL_3_8 = 0x5,
    rpGX_TEV_KCSEL_1_4 = 0x6,
    rpGX_TEV_KCSEL_1_8 = 0x7,
    rpGX_TEV_KCSEL_K0 = 0xC,
    rpGX_TEV_KCSEL_K1 = 0xD,
    rpGX_TEV_KCSEL_K2 = 0xE,
    rpGX_TEV_KCSEL_K3 = 0xF,
    rpGX_TEV_KCSEL_K0_R = 0x10,
    rpGX_TEV_KCSEL_K1_R = 0x11,
    rpGX_TEV_KCSEL_K2_R = 0x12,
    rpGX_TEV_KCSEL_K3_R = 0x13,
    rpGX_TEV_KCSEL_K0_G = 0x14,
    rpGX_TEV_KCSEL_K1_G = 0x15,
    rpGX_TEV_KCSEL_K2_G = 0x16,
    rpGX_TEV_KCSEL_K3_G = 0x17,
    rpGX_TEV_KCSEL_K0_B = 0x18,
    rpGX_TEV_KCSEL_K1_B = 0x19,
    rpGX_TEV_KCSEL_K2_B = 0x1A,
    rpGX_TEV_KCSEL_K3_B = 0x1B,
    rpGX_TEV_KCSEL_K0_A = 0x1C,
    rpGX_TEV_KCSEL_K1_A = 0x1D,
    rpGX_TEV_KCSEL_K2_A = 0x1E,
    rpGX_TEV_KCSEL_K3_A = 0x1F,
};
typedef enum RpGXTevKColorSel RpGXTevKColorSel;

enum RpGXTevOp
{
    rpGX_TEV_ADD,
    rpGX_TEV_SUB,
    rpGX_TEV_COMP_R8_GT = 8,
    rpGX_TEV_COMP_R8_EQ,
    rpGX_TEV_COMP_GR16_GT,
    rpGX_TEV_COMP_GR16_EQ,
    rpGX_TEV_COMP_BGR24_GT,
    rpGX_TEV_COMP_BGR24_EQ,
    rpGX_TEV_COMP_RGB8_GT,
    rpGX_TEV_COMP_RGB8_EQ,
    rpGX_TEV_COMP_A8_GT = 14,
    rpGX_TEV_COMP_A8_EQ,
};
typedef enum RpGXTevOp RpGXTevOp;

enum RpGXTevRegID
{
    rpGX_TEVPREV,
    rpGX_TEVREG0,
    rpGX_TEVREG1,
    rpGX_TEVREG2,
    rpGX_MAX_TEVREG,
};
typedef enum RpGXTevRegID RpGXTevRegID;

enum RpGXTevScale
{
    rpGX_CS_SCALE_1,
    rpGX_CS_SCALE_2,
    rpGX_CS_SCALE_4,
    rpGX_CS_DIVIDE_2,
    rpGX_MAX_TEVSCALE,
};
typedef enum RpGXTevScale RpGXTevScale;

enum RpGXTexCoordID
{
    rpGX_TEXCOORD0 = 0x0,
    rpGX_TEXCOORD1 = 0x1,
    rpGX_TEXCOORD2 = 0x2,
    rpGX_TEXCOORD3 = 0x3,
    rpGX_TEXCOORD4 = 0x4,
    rpGX_TEXCOORD5 = 0x5,
    rpGX_TEXCOORD6 = 0x6,
    rpGX_TEXCOORD7 = 0x7,
    rpGX_MAX_TEXCOORD = 0x8,
    rpGX_TEXCOORD_NULL = 0xFF,
};
typedef enum RpGXTexCoordID RpGXTexCoordID;

enum RpGXTexGenSrc
{
    rpGX_TG_POS,
    rpGX_TG_NRM,
    rpGX_TG_BINRM,
    rpGX_TG_TANGENT,
    rpGX_TG_TEX0,
    rpGX_TG_TEX1,
    rpGX_TG_TEX2,
    rpGX_TG_TEX3,
    rpGX_TG_TEX4,
    rpGX_TG_TEX5,
    rpGX_TG_TEX6,
    rpGX_TG_TEX7,
    rpGX_TG_TEXCOORD0,
    rpGX_TG_TEXCOORD1,
    rpGX_TG_TEXCOORD2,
    rpGX_TG_TEXCOORD3,
    rpGX_TG_TEXCOORD4,
    rpGX_TG_TEXCOORD5,
    rpGX_TG_TEXCOORD6,
    rpGX_TG_COLOR0,
    rpGX_TG_COLOR1,
};
typedef enum RpGXTexGenSrc RpGXTexGenSrc;

enum RpGXTexGenType
{
    rpGX_TG_MTX3x4,
    rpGX_TG_MTX2x4,
    rpGX_TG_BUMP0,
    rpGX_TG_BUMP1,
    rpGX_TG_BUMP2,
    rpGX_TG_BUMP3,
    rpGX_TG_BUMP4,
    rpGX_TG_BUMP5,
    rpGX_TG_BUMP6,
    rpGX_TG_BUMP7,
    rpGX_TG_SRTG,
};
typedef enum RpGXTexGenType RpGXTexGenType;

enum RpGXTexMapID
{
    rpGX_TEXMAP0 = 0x0,
    rpGX_TEXMAP1 = 0x1,
    rpGX_TEXMAP2 = 0x2,
    rpGX_TEXMAP3 = 0x3,
    rpGX_TEXMAP4 = 0x4,
    rpGX_TEXMAP5 = 0x5,
    rpGX_TEXMAP6 = 0x6,
    rpGX_TEXMAP7 = 0x7,
    rpGX_MAX_TEXMAP = 0x8,
    rpGX_TEXMAP_NULL = 0xFF,
    rpGX_TEX_DISABLE = 0x100,
};
typedef enum RpGXTexMapID RpGXTexMapID;

enum RpGameCubeTexFrameID
{
    rpGAMECUBETEXFRAME_NONE = 0x0,
    rpGAMECUBETEXFRAME_OBJECT = 0x1,
    rpGAMECUBETEXFRAME_WORLD = 0x2,
    rpGAMECUBETEXFRAME_CAMERA = 0x3,
    rpGAMECUBETEXFRAME_MISC0 = 0x10,
    rpGAMECUBETEXFRAME_MISC1 = 0x11,
    rpGAMECUBETEXFRAME_MISC2 = 0x12,
    rpGAMECUBETEXFRAME_MISC3 = 0x13,
    rpGAMECUBETEXFRAME_MISCMAX = 0x2F,
    rpGAMECUBETEXFRAME_FORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpGameCubeTexFrameID RpGameCubeTexFrameID;

struct RpGameCubeIndMtx
{
    RpGXIndTexMtxID id;
    RpGameCubeTexFrameID refFrame;
    RwUInt32 flags;
    RwReal data[2][3];
    RwInt32 scale;
};

struct RpGameCubeIndStage
{
    RwUInt32 flags;
    RpGXTexCoordID texCoordID;
    RpGXTexMapID texMapID;
    RpGXIndTexScale scaleS;
    RpGXIndTexScale scaleT;
};

struct RpGameCubeMTConfig
{
    RwUInt32 flags;
    RwUInt8 maxNumTexGens;
    RwUInt8 maxNumTevStages;
    RwUInt8 maxNumTexMtx;
    RwUInt8 maxNumIndStages;
    RwUInt8 maxNumIndMtx;
    RwUInt8 numTexGens;
    RwUInt8 numTevStages;
    RwUInt8 numTexMtx;
    RwUInt8 numIndStages;
    RwUInt8 numIndMtx;
    RpGXColorS10 reg[4];
    RwRGBA kreg[4];
    RpGameCubeTexMtx* texMtx;
    RpGameCubeTexGen* texGens;
    RpGameCubeIndMtx* indMtx;
    RpGameCubeIndStage* indStages;
    RpGameCubeTevStage* tevStages;
    RwBlendFunction srcBlend;
    RwBlendFunction destBlend;
};

struct RpGameCubeTevInd
{
    RpGXIndTexStageID indStage;
    RpGXIndTexFormat format;
    RpGXIndTexBiasSel biasSel;
    RpGXIndTexMtxID matrixSel;
    RpGXIndTexWrap wrapS;
    RpGXIndTexWrap wrapT;
    RwUInt8 addPrev;
    RwUInt8 utcLod;
    RpGXIndTexAlphaSel alphaSel;
};

struct RpGameCubeTevOp
{
    RpGXTevColorArg colorA;
    RpGXTevColorArg colorB;
    RpGXTevColorArg colorC;
    RpGXTevColorArg colorD;
    RpGXTevAlphaArg alphaA;
    RpGXTevAlphaArg alphaB;
    RpGXTevAlphaArg alphaC;
    RpGXTevAlphaArg alphaD;
    RpGXTevOp colorOp;
    RpGXTevBias colorBias;
    RpGXTevScale colorScale;
    RwUInt8 colorClamp;
    RpGXTevRegID colorOutReg;
    RpGXTevOp alphaOp;
    RpGXTevBias alphaBias;
    RpGXTevScale alphaScale;
    RwUInt8 alphaClamp;
    RpGXTevRegID alphaOutReg;
    RpGXTevKColorSel colorSel;
    RpGXTevKAlphaSel alphaSel;
};

struct RpGameCubeTevStage
{
    RpGameCubeTevOp op;
    RpGXTexCoordID texCoordID;
    RpGXTexMapID texMapID;
    RpGXChannelID channelID;
    RwUInt32 flags;
    RwUInt32 indirect;
};

struct RpGameCubeTexGen
{
    RwUInt32 flags;
    RpGXTexGenType func;
    RpGXTexGenSrc srcParam;
    RwUInt32 mtx;
    RwUInt32 postMtx;
    RwUInt16 scaleS;
    RwUInt16 scaleT;
};

struct RpGameCubeTexMtx
{
    RwUInt32 offset;
    RpGameCubeTexFrameID refFrame;
    RwUInt32 flags;
    RwReal data[3][4];
};

struct RpMTEffect
{
    RwPlatformID platformID;
    RwUInt32 refCount;
    RwChar name[32];
    RwLLLink dictLink;
};

#ifdef __cplusplus
extern "C" {
#endif

RpMTEffect* RpGameCubeMTEffectCreate(RwUInt32 numTevStages, RwUInt32 numTexGens, RwUInt32 numTexMtx, RwUInt32 numIndStages, RwUInt32 numIndMtx);
RpGameCubeMTConfig* RpGameCubeMTEffectGetConfig(RpMTEffect* effect);
void RpGameCubeMTSetMiscFrameArray(RwFrame** frames, RwUInt32 numFrames);
void RpGameCubeTevIndPack(RwUInt32* pkData, RpGameCubeTevInd* data);
void RpMTEffectDestroy(RpMTEffect* effect);
RwChar* RpMTEffectGetName(RpMTEffect* effect);
RpMTEffect* RpMTEffectSetName(RpMTEffect* effect, RwChar* name);
RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* atomic);
RpMatFXMaterialFlags RpMatFXMaterialGetEffects(const RpMaterial* material);
RwReal RpMatFXMaterialGetEnvMapCoefficient(const RpMaterial* material);
RpMaterial* RpMatFXMaterialSetBumpMapCoefficient(RpMaterial* material, RwReal coef);
RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material, RpMatFXMaterialFlags flags);
RpMaterial* RpMatFXMaterialSetEnvMapCoefficient(RpMaterial* material, RwReal coef);
RpMaterial* RpMatFXMaterialSetUVTransformMatrices(RpMaterial* material, RwMatrix* baseTransform, RwMatrix* dualTransform);
RpMaterial* RpMatFXMaterialSetupBumpMap(RpMaterial* material, RwTexture* texture, RwFrame* frame, RwReal coef);
RpMaterial* RpMatFXMaterialSetupDualTexture(RpMaterial* material, RwTexture* texture, RwBlendFunction srcBlendMode, RwBlendFunction dstBlendMode);
RpMaterial* RpMatFXMaterialSetupEnvMap(RpMaterial* material, RwTexture* texture, RwFrame* frame, RwBool useFrameBufferAlpha, RwReal coef);
RwBool RpMatFXPluginAttach(void);
RpMaterial* RpMaterialCreateMultiTexture(RpMaterial* material, RwPlatformID platformID, RwUInt32 numTextures);
RpMultiTexture* RpMaterialGetMultiTexture(const RpMaterial* material, RwPlatformID platformID);
RpMTEffect* RpMultiTextureGetEffect(const RpMultiTexture* multiTexture);
RpMultiTexture* RpMultiTextureGetGameCubeMTCallBacks(RpMultiTexture* multiTexture, RpGameCubeMTCallBack* preRenderCallBack, RpGameCubeMTCallBack* postRenderCallBack, void** callBackData);
RwUInt32 RpMultiTextureGetNumTextures(const RpMultiTexture* multiTexture);
RwTexture* RpMultiTextureGetTexture(const RpMultiTexture* multiTexture, RwUInt32 index);
RpMultiTexture* RpMultiTextureSetCoords(RpMultiTexture* multiTexture, RwUInt32 index, RwUInt32 texCoordIndex);
RpMultiTexture* RpMultiTextureSetEffect(RpMultiTexture* multiTexture, RpMTEffect* effect);
RpMultiTexture* RpMultiTextureSetGameCubeMTCallBacks(RpMultiTexture* multiTexture, RpGameCubeMTCallBack preRenderCallBack, RpGameCubeMTCallBack postRenderCallBack, void* callBackData);
RpMultiTexture* RpMultiTextureSetTexture(RpMultiTexture* multiTexture, RwUInt32 index, RwTexture* texture);

#ifdef __cplusplus
}
#endif

#endif

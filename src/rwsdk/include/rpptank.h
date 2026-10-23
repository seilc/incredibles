#ifndef RPPTANK_H
#define RPPTANK_H

#include <string.h>
#include "rwcore.h"
#include "rpworld.h"

typedef struct RpPTANKInstanceSetupData RpPTANKInstanceSetupData;
typedef struct RpPTankAtomicExtPrv RpPTankAtomicExtPrv;
typedef struct RpPTankCallBacks RpPTankCallBacks;
typedef struct RpPTankData RpPTankData;
typedef struct RpPTankFormatDescriptor RpPTankFormatDescriptor;
typedef struct RpPTankLockStruct RpPTankLockStruct;

enum RpPTankDataFlags
{
    rpPTANKDFLAGNONE = 0x0,
    rpPTANKDFLAGPOSITION = 0x1,
    rpPTANKDFLAGCOLOR = 0x2,
    rpPTANKDFLAGSIZE = 0x4,
    rpPTANKDFLAGMATRIX = 0x8,
    rpPTANKDFLAGNORMAL = 0x10,
    rpPTANKDFLAG2DROTATE = 0x20,
    rpPTANKDFLAGVTXCOLOR = 0x40,
    rpPTANKDFLAGVTX2TEXCOORDS = 0x80,
    rpPTANKDFLAGVTX4TEXCOORDS = 0x100,
    rpPTANKDFLAGCNSMATRIX = 0x8000,
    rpPTANKDFLAGCNS2DROTATE = 0x20000,
    rpPTANKDFLAGCNSVTXCOLOR = 0x40000,
    rpPTANKDFLAGCNSVTX2TEXCOORDS = 0x80000,
    rpPTANKDFLAGCNSVTX4TEXCOORDS = 0x100000,
    rpPTANKDFLAGUSECENTER = 0x1000000,
    rpPTANKDFLAGARRAY = 0x10000000,
    rpPTANKDFLAGSTRUCTURE = 0x20000000,
    RPPTANKDATAFLAGSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpPTankDataFlags RpPTankDataFlags;

enum RpPTankDataLockFlags
{
    rpPTANKLFLAGNONE = 0x0,
    rpPTANKLFLAGPOSITION = 0x1,
    rpPTANKLFLAGCOLOR = 0x2,
    rpPTANKLFLAGSIZE = 0x4,
    rpPTANKLFLAGMATRIX = 0x8,
    rpPTANKLFLAGNORMAL = 0x10,
    rpPTANKLFLAG2DROTATE = 0x20,
    rpPTANKLFLAGVTXCOLOR = 0x40,
    rpPTANKLFLAGVTX2TEXCOORDS = 0x80,
    rpPTANKLFLAGVTX4TEXCOORDS = 0x100,
    RPPTANKLOCKFLAGSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpPTankDataLockFlags RpPTankDataLockFlags;

enum RpPTankInstanceFlags
{
    rpPTANKIFLAGNONE = 0x0,
    rpPTANKIFLAGPOSITION = 0x1,
    rpPTANKIFLAGCOLOR = 0x2,
    rpPTANKIFLAGSIZE = 0x4,
    rpPTANKIFLAGMATRIX = 0x8,
    rpPTANKIFLAGNORMAL = 0x10,
    rpPTANKIFLAG2DROTATE = 0x20,
    rpPTANKIFLAGVTXCOLOR = 0x40,
    rpPTANKIFLAGVTX2TEXCOORDS = 0x80,
    rpPTANKIFLAGVTX4TEXCOORDS = 0x100,
    rpPTANKIFLAGCNSCOLOR = 0x2000,
    rpPTANKIFLAGCNSSIZE = 0x4000,
    rpPTANKIFLAGCNSMATRIX = 0x8000,
    rpPTANKIFLAGCNSNORMAL = 0x10000,
    rpPTANKIFLAGCNS2DROTATE = 0x20000,
    rpPTANKIFLAGCNSVTXCOLOR = 0x40000,
    rpPTANKIFLAGCNSVTX2TEXCOORDS = 0x80000,
    rpPTANKIFLAGCNSVTX4TEXCOORDS = 0x100000,
    rpPTANKIFLAGACTNUMCHG = 0x800000,
    rpPTANKIFLAGCENTER = 0x1000000,
    rpPTANKIFLAGALPHABLENDING = 0x10000000,
    rpPTANKIFLAGALL = 0xFFFFFFFF,
    RPPTANKINSTANCEFLAGSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpPTankInstanceFlags RpPTankInstanceFlags;

enum RpPTankLockFlags
{
    rpPTANKLOCKWRITE = 0x40000000,
    rpPTANKLOCKREAD = (int)0x80000000,
};
typedef enum RpPTankLockFlags RpPTankLockFlags;

struct RpPTankLockStruct
{
    RwUInt8* data;
    RwInt32 stride;
};

struct RpPTankFormatDescriptor
{
    RwInt32 numClusters;
    RwInt32 stride;
    RwInt32 dataFlags;
};

struct RpPTankData
{
    void* data;
    RpPTankLockStruct clusters[9];
    void* userData;
    RpPTankFormatDescriptor format;
    RwUInt32 srcBlend;
    RwUInt32 dstBlend;
    RwBool vertexAlphaBlend;
    RwV2d cCenter;
    RwV2d cSize;
    RwReal cRotate;
    RwRGBA cColor;
    RwRGBA cVtxColor[4];
    RwTexCoords cUV[4];
    RwMatrix cMatrix;
};

typedef void* (*RpPTankAllocCallBack)(RpPTankData* ptankGlobal, RwInt32 maxPCount, RwUInt32 dataFlags, RwUInt32 platFlags);
typedef RwBool (*RpPTankCreateCallBack)(RpAtomic* atomic, RpPTankData* ptankGlobal, RwInt32 maxPCount, RwUInt32 dataFlags, RwUInt32 platFlags);
typedef RwBool (*RpPTankInstanceCallBack)(RpAtomic* atomic, RpPTankData* ptankGlobal, RwInt32 actPCount, RwUInt32 instFlags);
typedef RwBool (*RpPTankRenderCallBack)(RpAtomic* atomic, RpPTankData* ptankGlobal, RwInt32 actPCount);

struct RpPTankCallBacks
{
    RpPTankAllocCallBack alloc;
    RpPTankCreateCallBack create;
    RpPTankInstanceCallBack instance;
    RpPTankRenderCallBack render;
};

typedef void (*RpPTankGENInstanceSetupCallback)(RpPTANKInstanceSetupData* data, RpAtomic* atomic, RpPTankData* ptankGlobal, RwInt32 actPCount, RwUInt32 instFlags);
typedef void (*RpPTankGENInstancePosCallback)(RpPTankLockStruct* dstCluster, RwV3d* right, RwV3d* up, RwInt32 pCount, RpPTankData* ptankGlobal);
typedef void (*RpPTankGENInstanceCallback)(RpPTankLockStruct* dstCluster, RwInt32 pCount, RpPTankData* ptankGlobal);
typedef void (*RpPTankGENInstanceEndingCallback)(RpPTANKInstanceSetupData* data, RpAtomic* atomic, RpPTankData* ptankGlobal, RwInt32 actPCount, RwUInt32 instFlags);

struct RpPTankAtomicExtPrv
{
    RwInt32 maxPCount;
    RwInt32 actPCount;
    RwBool isAStructure;
    void* rawdata;
    RpAtomicCallBackRender defaultRenderCB;
    RpPTankCallBacks ptankCallBacks;
    RpPTankGENInstanceSetupCallback insSetupCB;
    RpPTankGENInstancePosCallback insPosCB;
    RpPTankGENInstanceCallback insUVCB;
    RpPTankGENInstanceCallback insColorsCB;
    RpPTankGENInstanceCallback insNormalsCB;
    RpPTankGENInstanceEndingCallback insEndingCB;
    RwUInt32 lockFlags;
    RwUInt32 instFlags;
    RwUInt32 platFlags;
    RpPTankData publicData;
};

#define RPATOMICPTANKPLUGINDATA(atomic) \
    (*RWPLUGINOFFSET(RpPTankAtomicExtPrv *, (atomic), _rpPTankAtomicDataOffset))

#define RpPTankAtomicGetActiveParticlesCountMacro(_atm) (RPATOMICPTANKPLUGINDATA(_atm)->actPCount)

#define RpPTankAtomicGetTextureMacro(atm_) \
    (RpMaterialGetTexture(RpGeometryGetMaterial(RpAtomicGetGeometry(atm_),0)))

#define RpPTankAtomicSetBlendModesMacro(atm_,src_,dst_)                         \
    MACRO_START                                                                 \
    {                                                                           \
        RPATOMICPTANKPLUGINDATA(atm_)->publicData.srcBlend = src_;              \
        RPATOMICPTANKPLUGINDATA(atm_)->publicData.dstBlend = dst_;              \
        RPATOMICPTANKPLUGINDATA(atm_)->instFlags |= rpPTANKIFLAGALPHABLENDING;  \
    }                                                                           \
    MACRO_STOP

#define RpPTankAtomicSetConstantVtx2TexCoordsMacro(atm_, uv_)                               \
    MACRO_START                                                                             \
    {                                                                                       \
        memcpy(RPATOMICPTANKPLUGINDATA(atm_)->publicData.cUV, uv_, sizeof(RwTexCoords)*2);  \
        RPATOMICPTANKPLUGINDATA(atm_)->instFlags |= rpPTANKIFLAGCNSVTX2TEXCOORDS;           \
    }                                                                                       \
    MACRO_STOP

#define RpPTankAtomicSetTextureMacro(atm_, tex_)                                         \
    MACRO_START                                                                          \
    {                                                                                    \
        RpMaterialSetTexture(RpGeometryGetMaterial(RpAtomicGetGeometry(atm_),0), tex_);  \
    }                                                                                    \
    MACRO_STOP

#define RpPTankAtomicSetVertexAlphaMacro(atm_, vas_)                        \
    MACRO_START                                                             \
    {                                                                       \
        RPATOMICPTANKPLUGINDATA(atm_)->publicData.vertexAlphaBlend = vas_;  \
    }                                                                       \
    MACRO_STOP

#ifdef __cplusplus
extern "C" {
#endif

extern RwInt32 _rpPTankAtomicDataOffset;

RpAtomic* RpPTankAtomicCreate(RwInt32 maxParticleNum, RwUInt32 dataFlags, RwUInt32 platFlags);
void RpPTankAtomicDestroy(RpAtomic* ptank);
RwBool RpPTankAtomicLock(RpAtomic* atomic, RpPTankLockStruct* dst, RwUInt32 dataFlags, RpPTankLockFlags lockFlag);

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
RwInt32 RpPTankAtomicGetActiveParticlesCount(RpAtomic* atomic);
RwTexture* RpPTankAtomicGetTexture(RpAtomic* atomic);
void RpPTankAtomicSetBlendModes(RpAtomic* atomic, RwBlendFunction srcBlendMode, RwBlendFunction dstBlendMode);
void RpPTankAtomicSetConstantVtx2TexCoords(RpAtomic* atomic, RwTexCoords* UVs);
void RpPTankAtomicSetTexture(RpAtomic* atomic, RwTexture* texture);
void RpPTankAtomicSetVertexAlpha(RpAtomic* atomic, RwBool vtxAlphaState);
#endif

#ifdef __cplusplus
}
#endif

#if !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
#define RpPTankAtomicGetActiveParticlesCount(atm_) RpPTankAtomicGetActiveParticlesCountMacro(atm_)
#define RpPTankAtomicGetTexture(atm_) RpPTankAtomicGetTextureMacro(atm_)
#define RpPTankAtomicSetBlendModes(atm_,src_,dst_) RpPTankAtomicSetBlendModesMacro(atm_,src_,dst_)
#define RpPTankAtomicSetConstantVtx2TexCoords(atm_, uv_) RpPTankAtomicSetConstantVtx2TexCoordsMacro(atm_, uv_)
#define RpPTankAtomicSetTexture(atm_,tex_) RpPTankAtomicSetTextureMacro(atm_,tex_)
#define RpPTankAtomicSetVertexAlpha(atm_, vas_) RpPTankAtomicSetVertexAlphaMacro(atm_, vas_)
#endif

#endif

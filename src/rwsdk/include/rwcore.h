#ifndef RWCORE_H
#define RWCORE_H

#include "rwplcore.h"

typedef struct RwBBox RwBBox;
typedef struct RwCamera RwCamera;
typedef struct RwFrame RwFrame;
typedef struct RwFrustumPlane RwFrustumPlane;
typedef struct RwGameCubeDeviceConfig RwGameCubeDeviceConfig;
typedef struct RwGameCubeRasterExtension RwGameCubeRasterExtension;
typedef struct RwImage RwImage;
typedef struct RwObjectHasFrame RwObjectHasFrame;
typedef struct RwRaster RwRaster;
typedef struct RwTexDictionary RwTexDictionary;
typedef struct RwTexture RwTexture;
typedef struct RxCluster RxCluster;
typedef struct RxClusterDefinition RxClusterDefinition;
typedef struct RxClusterRef RxClusterRef;
typedef struct RxHeap RxHeap;
typedef struct RxIoSpec RxIoSpec;
typedef struct RxNodeDefinition RxNodeDefinition;
typedef struct RxNodeMethods RxNodeMethods;
typedef struct RxObjSpace3DVertex RxObjSpace3DVertex;
typedef struct RxOutputSpec RxOutputSpec;
typedef struct RxPacket RxPacket;
typedef struct RxPipeline RxPipeline;
typedef struct RxPipelineCluster RxPipelineCluster;
typedef struct RxPipelineNode RxPipelineNode;
typedef struct RxPipelineNodeParam RxPipelineNodeParam;
typedef struct RxPipelineNodeTopSortData RxPipelineNodeTopSortData;
typedef struct RxPipelineRequiresCluster RxPipelineRequiresCluster;
typedef struct RxRenderStateVector RxRenderStateVector;
typedef struct rxHeapBlockHeader rxHeapBlockHeader;
typedef struct rxHeapFreeBlock rxHeapFreeBlock;
typedef struct rxHeapSuperBlockDescriptor rxHeapSuperBlockDescriptor;

typedef RxObjSpace3DVertex RxObjSpace3DLitVertex;
typedef RxObjSpace3DLitVertex RwIm3DVertex;
typedef RxPipeline RxLockedPipe;

enum RwRasterLockMode
{
    rwRASTERLOCKWRITE = 0x1,
    rwRASTERLOCKREAD = 0x2,
    rwRASTERLOCKNOFETCH = 0x4,
    rwRASTERLOCKRAW = 0x8,
    rwRASTERLOCKMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwRasterLockMode RwRasterLockMode;

enum RwRasterFlipMode
{
    rwRASTERFLIPDONTWAIT,
    rwRASTERFLIPWAITVSYNC,
    rwRASTERFLIPMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwRasterFlipMode RwRasterFlipMode;

enum RwRasterType
{
    rwRASTERTYPENORMAL = 0x0,
    rwRASTERTYPEZBUFFER = 0x1,
    rwRASTERTYPECAMERA = 0x2,
    rwRASTERTYPETEXTURE = 0x4,
    rwRASTERTYPECAMERATEXTURE = 0x5,
    rwRASTERTYPEMASK = 0x7,
    rwRASTERPALETTEVOLATILE = 0x40,
    rwRASTERDONTALLOCATE = 0x80,
    rwRASTERTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwRasterType RwRasterType;

enum RxGeometryFlag
{
    rxGEOMETRY_TRISTRIP = 0x1,
    rxGEOMETRY_POSITIONS = 0x2,
    rxGEOMETRY_TEXTURED = 0x4,
    rxGEOMETRY_PRELIT = 0x8,
    rxGEOMETRY_NORMALS = 0x10,
    rxGEOMETRY_LIGHT = 0x20,
    rxGEOMETRY_MODULATE = 0x40,
    rxGEOMETRY_TEXTURED2 = 0x80,
    rxGEOMETRY_COLORED = 0x100,
    rxGEOMETRYFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RxGeometryFlag RxGeometryFlag;

enum RxRenderStateFlag
{
    rxRENDERSTATEFLAG_TEXTUREPERSPECTIVE = 0x1,
    rxRENDERSTATEFLAG_ZTESTENABLE = 0x2,
    rxRENDERSTATEFLAG_ZWRITEENABLE = 0x4,
    rxRENDERSTATEFLAG_VERTEXALPHAENABLE = 0x8,
    rxRENDERSTATEFLAG_FOGENABLE = 0x10,
    rxRENDERSTATEFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RxRenderStateFlag RxRenderStateFlag;

enum RwIm3DTransformFlags
{
    rwIM3D_VERTEXUV = 0x1,
    rwIM3D_ALLOPAQUE = 0x2,
    rwIM3D_NOCLIP = 0x4,
    rwIM3D_VERTEXXYZ = 0x8,
    rwIM3D_VERTEXRGBA = 0x10,
    rwIM3DTRANSFORMFLAGSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwIm3DTransformFlags RwIm3DTransformFlags;

enum RwCameraClearMode
{
    rwCAMERACLEARIMAGE = 0x1,
    rwCAMERACLEARZ = 0x2,
    rwCAMERACLEARSTENCIL = 0x4,
    rwCAMERACLEARMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwCameraClearMode RwCameraClearMode;

struct rxHeapFreeBlock
{
    RwUInt32 size;
    rxHeapBlockHeader* ptr;
};

struct rxHeapSuperBlockDescriptor
{
    void* start;
    RwUInt32 size;
    rxHeapSuperBlockDescriptor* next;
};

struct RxHeap
{
    RwUInt32 superBlockSize;
    rxHeapSuperBlockDescriptor* head;
    rxHeapBlockHeader* headBlock;
    rxHeapFreeBlock* freeBlocks;
    RwUInt32 entriesAlloced;
    RwUInt32 entriesUsed;
    RwBool dirty;
};

struct rxHeapBlockHeader
{
    rxHeapBlockHeader* prev;
    rxHeapBlockHeader* next;
    RwUInt32 size;
    rxHeapFreeBlock* freeEntry;
    RwUInt32 pad[4];
};

enum RxClusterValidityReq
{
    rxCLREQ_DONTWANT,
    rxCLREQ_REQUIRED,
    rxCLREQ_OPTIONAL,
    rxCLUSTERVALIDITYREQFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RxClusterValidityReq RxClusterValidityReq;

enum RxClusterValid
{
    rxCLVALID_NOCHANGE,
    rxCLVALID_VALID,
    rxCLVALID_INVALID,
    rxCLUSTERVALIDFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RxClusterValid RxClusterValid;

struct RxClusterDefinition
{
    RwChar* name;
    RwUInt32 defaultStride;
    RwUInt32 defaultAttributes;
    const RwChar* attributeSet;
};

struct RxOutputSpec
{
    RwChar* name;
    RxClusterValid* outputClusters;
    RxClusterValid allOtherClusters;
};

enum RxClusterForcePresent
{
    rxCLALLOWABSENT,
    rxCLFORCEPRESENT,
    rxCLUSTERFORCEPRESENTFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RxClusterForcePresent RxClusterForcePresent;

struct RxClusterRef
{
    RxClusterDefinition* clusterDef;
    RxClusterForcePresent forcePresent;
    RwUInt32 reserved;
};

struct RxIoSpec
{
    RwUInt32 numClustersOfInterest;
    RxClusterRef* clustersOfInterest;
    RxClusterValidityReq* inputRequirements;
    RwUInt32 numOutputs;
    RxOutputSpec* outputs;
};

typedef RwBool (*RxNodeBodyFn)(RxPipelineNode* self, const RxPipelineNodeParam* params);
typedef RwBool (*RxNodeInitFn)(RxNodeDefinition* self);
typedef void (*RxNodeTermFn)(RxNodeDefinition* self);
typedef RwBool (*RxPipelineNodeInitFn)(RxPipelineNode* self);
typedef void (*RxPipelineNodeTermFn)(RxPipelineNode* self);
typedef RwBool (*RxPipelineNodeConfigFn)(RxPipelineNode* self, RxPipeline* pipeline);
typedef RwUInt32 (*RxConfigMsgHandlerFn)(RxPipelineNode* self, RwUInt32 msg, RwUInt32 intparam, void* ptrparam);

struct RxNodeMethods
{
    RxNodeBodyFn nodeBody;
    RxNodeInitFn nodeInit;
    RxNodeTermFn nodeTerm;
    RxPipelineNodeInitFn pipelineNodeInit;
    RxPipelineNodeTermFn pipelineNodeTerm;
    RxPipelineNodeConfigFn pipelineNodeConfig;
    RxConfigMsgHandlerFn configMsgHandler;
};

enum RxNodeDefEditable
{
    rxNODEDEFCONST,
    rxNODEDEFEDITABLE,
    rxNODEDEFEDITABLEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RxNodeDefEditable RxNodeDefEditable;

struct RxNodeDefinition
{
    RwChar* name;
    RxNodeMethods nodeMethods;
    RxIoSpec io;
    RwUInt32 pipelineNodePrivateDataSize;
    RxNodeDefEditable editable;
    RwInt32 InputPipesCnt;
};

struct RxPipelineCluster
{
    RxClusterDefinition* clusterRef;
    RwUInt32 creationAttributes;
};

struct RxCluster
{
    RwUInt16 flags;
    RwUInt16 stride;
    void* data;
    void* currentData;
    RwUInt32 numAlloced;
    RwUInt32 numUsed;
    RxPipelineCluster* clusterRef;
    RwUInt32 attributes;
};

struct RxPacket
{
    RwUInt16 flags;
    RwUInt16 numClusters;
    RxPipeline* pipeline;
    RwUInt32* inputToClusterSlot;
    RwUInt32* slotsContinue;
    RxPipelineCluster** slotClusterRefs;
    RxCluster clusters[1];
};

struct RxPipelineNode
{
    RxNodeDefinition* nodeDef;
    RwUInt32 numOutputs;
    RwUInt32* outputs;
    RxPipelineCluster** slotClusterRefs;
    RwUInt32* slotsContinue;
    void* privateData;
    RwUInt32* inputToClusterSlot;
    RxPipelineNodeTopSortData* topSortData;
    void* initializationData;
    RwUInt32 initializationDataSize;
};

struct RxPipelineNodeTopSortData
{
    RwUInt32 numIns;
    RwUInt32 numInsVisited;
    rxReq* req;
};

struct RxPipelineNodeParam
{
    void* dataParam;
    RxHeap* heap;
};

enum rxEmbeddedPacketState
{
    rxPKST_PACKETLESS,
    rxPKST_UNUSED,
    rxPKST_INUSE,
    rxPKST_PENDING,
    rxEMBEDDEDPACKETSTATEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum rxEmbeddedPacketState rxEmbeddedPacketState;

struct RxPipelineRequiresCluster
{
    RxClusterDefinition* clusterDef;
    RxClusterValidityReq rqdOrOpt;
    RwUInt32 slotIndex;
};

struct RxPipeline
{
    RwBool locked;
    RwUInt32 numNodes;
    RxPipelineNode* nodes;
    RwUInt32 packetNumClusterSlots;
    rxEmbeddedPacketState embeddedPacketState;
    RxPacket* embeddedPacket;
    RwUInt32 numInputRequirements;
    RxPipelineRequiresCluster* inputRequirements;
    void* superBlock;
    RwUInt32 superBlockSize;
    RwUInt32 entryPoint;
    RwUInt32 pluginId;
    RwUInt32 pluginData;
};

struct RxObjSpace3DVertex
{
    RwReal x;
    RwReal y;
    RwReal z;
    RwReal nx;
    RwReal ny;
    RwReal nz;
    RwUInt8 r;
    RwUInt8 g;
    RwUInt8 b;
    RwUInt8 a;
    RwReal u;
    RwReal v;
};

enum RwRasterFormat
{
    rwRASTERFORMATDEFAULT = 0x0,
    rwRASTERFORMAT1555 = 0x100,
    rwRASTERFORMAT565 = 0x200,
    rwRASTERFORMAT4444 = 0x300,
    rwRASTERFORMATLUM8 = 0x400,
    rwRASTERFORMAT8888 = 0x500,
    rwRASTERFORMAT888 = 0x600,
    rwRASTERFORMAT16 = 0x700,
    rwRASTERFORMAT24 = 0x800,
    rwRASTERFORMAT32 = 0x900,
    rwRASTERFORMAT555 = 0xA00,
    rwRASTERFORMATAUTOMIPMAP = 0x1000,
    rwRASTERFORMATPAL8 = 0x2000,
    rwRASTERFORMATPAL4 = 0x4000,
    rwRASTERFORMATMIPMAP = 0x8000,
    rwRASTERFORMATPIXELFORMATMASK = 0xF00,
    rwRASTERFORMATMASK = 0xFF00,
    rwRASTERFORMATFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwRasterFormat RwRasterFormat;

struct RwRaster
{
    RwRaster* parent;
    RwUInt8* cpPixels;
    RwUInt8* palette;
    RwInt32 width;
    RwInt32 height;
    RwInt32 depth;
    RwInt32 stride;
    RwInt16 nOffsetX;
    RwInt16 nOffsetY;
    RwUInt8 cType;
    RwUInt8 cFlags;
    RwUInt8 privateFlags;
    RwUInt8 cFormat;
    RwUInt8* originalPixels;
    RwInt32 originalWidth;
    RwInt32 originalHeight;
    RwInt32 originalStride;
};

struct RwGameCubeRasterExtension
{
    RwUInt32 tlutObj[3];
    RwUInt32 format;
    RwUInt32 tlutFmt;
    RwUInt32 flags;
    RwUInt8* memory;
    RwUInt8* pixels;
    RwUInt8* palette;
    RwUInt8* lockedPixels;
    RwUInt8* lockedBuffer;
    void* region;
    RwUInt16 token;
    RwUInt8 maxLOD;
    RwUInt8 lockedMipLevel;
};

struct RwImage
{
    RwInt32 flags;
    RwInt32 width;
    RwInt32 height;
    RwInt32 depth;
    RwInt32 stride;
    RwUInt8* cpPixels;
    RwRGBA* palette;
};

struct RwTexDictionary
{
    RwObject object;
    RwLinkList texturesInDict;
    RwLLLink lInInstance;
};

struct RwTexture
{
    RwRaster* raster;
    RwTexDictionary* dict;
    RwLLLink lInDictionary;
    RwChar name[32];
    RwChar mask[32];
    RwUInt32 filterAddressing;
    RwInt32 refCount;
};

struct RxRenderStateVector
{
    RwUInt32 Flags;
    RwShadeMode ShadeMode;
    RwBlendFunction SrcBlend;
    RwBlendFunction DestBlend;
    RwRaster* TextureRaster;
    RwTextureAddressMode AddressModeU;
    RwTextureAddressMode AddressModeV;
    RwTextureFilterMode FilterMode;
    RwRGBA BorderColor;
    RwFogType FogType;
    RwRGBA FogColor;
};

struct RwBBox
{
    RwV3d sup;
    RwV3d inf;
};

struct RwFrame
{
    RwObject object;
    RwLLLink inDirtyListLink;
    RwMatrix modelling;
    RwMatrix ltm;
    RwLinkList objectList;
    RwFrame* child;
    RwFrame* next;
    RwFrame* root;
};

typedef RwObjectHasFrame* (*RwObjectHasFrameSyncFunction)(RwObjectHasFrame* object);

struct RwObjectHasFrame
{
    RwObject object;
    RwLLLink lFrame;
    RwObjectHasFrameSyncFunction sync;
};

enum RwCameraProjection
{
    rwNACAMERAPROJECTION,
    rwPERSPECTIVE,
    rwPARALLEL,
    rwCAMERAPROJECTIONFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwCameraProjection RwCameraProjection;

enum RwFrustumTestResult
{
    rwSPHEREOUTSIDE,
    rwSPHEREBOUNDARY,
    rwSPHEREINSIDE,
    rwFRUSTUMTESTRESULTFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwFrustumTestResult RwFrustumTestResult;

struct RwFrustumPlane
{
    RwPlane plane;
    RwUInt8 closestX;
    RwUInt8 closestY;
    RwUInt8 closestZ;
    RwUInt8 pad;
};

typedef RwCamera* (*RwCameraBeginUpdateFunc)(RwCamera* camera);
typedef RwCamera* (*RwCameraEndUpdateFunc)(RwCamera* camera);

struct RwCamera
{
    RwObjectHasFrame object;
    RwCameraProjection projectionType;
    RwCameraBeginUpdateFunc beginUpdate;
    RwCameraEndUpdateFunc endUpdate;
    RwMatrix viewMatrix;
    RwRaster* frameBuffer;
    RwRaster* zBuffer;
    RwV2d viewWindow;
    RwV2d recipViewWindow;
    RwV2d viewOffset;
    RwReal nearPlane;
    RwReal farPlane;
    RwReal fogPlane;
    RwReal zScale;
    RwReal zShift;
    RwFrustumPlane frustumPlanes[6];
    RwBBox frustumBoundBox;
    RwV3d frustumCorners[8];
};

struct RwGameCubeDeviceConfig
{
    void* renderMode;
    RwInt32 pixFmt;
    RwUInt32 fifoSize;
};

typedef RwFrame* (*RwFrameCallBack)(RwFrame* frame, void* data);
typedef RwTexture* (*RwTextureCallBackRead)(const RwChar* name, const RwChar* maskName);

#define RwIm3DVertexSetNormal(_vert, _imx, _imy, _imz)  \
    MACRO_START                                         \
    {                                                   \
        RwV3d packed;                                   \
        packed.x = _imx;                                \
        packed.y = _imy;                                \
        packed.z = _imz;                                \
        (_vert)->nx = packed.x;                         \
        (_vert)->ny = packed.y;                         \
        (_vert)->nz = packed.z;                         \
    }                                                   \
    MACRO_STOP

#define RwIm3DVertexSetPos(_vert, _imx, _imy, _imz)  \
    MACRO_START                                      \
    {                                                \
        RwV3d tmp;                                   \
        tmp.x = _imx;                                \
        tmp.y = _imy;                                \
        tmp.z = _imz;                                \
        (_vert)->x = tmp.x;                          \
        (_vert)->y = tmp.y;                          \
        (_vert)->z = tmp.z;                          \
    }                                                \
    MACRO_STOP

#define RwIm3DVertexSetRGBA(_vert, _r, _g, _b, _a)  \
    MACRO_START                                     \
    {                                               \
        RwRGBA col;                                 \
        col.red = _r;                               \
        col.green = _g;                             \
        col.blue = _b;                              \
        col.alpha = _a;                             \
        (_vert)->r = col.red;                       \
        (_vert)->g = col.green;                     \
        (_vert)->b = col.blue;                      \
        (_vert)->a = col.alpha;                     \
    }                                               \
    MACRO_STOP

#define RwIm3DVertexSetUV(_vert, _u, _v)  \
    MACRO_START                           \
    {                                     \
        (_vert)->u = _u;                  \
        (_vert)->v = _v;                  \
    }                                     \
    MACRO_STOP

#define RxPipelineDestroy(_ppln) (_rxPipelineDestroy(_ppln), TRUE)
#define _rwDlTokenGetCurrentMacro() (_RwDlTokenCurrent)

#define _rwDlTokenSendNewMacro()                                           \
    MACRO_START                                                            \
    {                                                                      \
        GXSetDrawSync(_RwDlTokenCurrent);                                  \
        _RwDlTokenCurrent = (RwUInt16)((_RwDlTokenCurrent + 1) % 0xE000);  \
    }                                                                      \
    MACRO_STOP

#define _rwDlTokenWaitDoneMacro(_token)       \
    MACRO_START                               \
    {                                         \
        if ((_token) == _RwDlTokenCurrent)    \
        {                                     \
            _rwDlTokenSendNewMacro();         \
        }                                     \
        while (!_rwDlTokenQueryDone(_token))  \
        {                                     \
        }                                     \
    }                                         \
    MACRO_STOP

#define RwCameraGetCurrentCameraMacro() ((RwCamera *)RWSRCGLOBAL(curCamera))
#define RwCameraGetFarClipPlaneMacro(_camera) ((_camera)->farPlane)
#define RwCameraGetFogDistanceMacro(_camera) ((_camera)->fogPlane)
#define RwCameraGetFrameMacro(_camera) ((RwFrame *)rwObjectGetParent((_camera)))
#define RwCameraGetNearClipPlaneMacro(_camera) ((_camera)->nearPlane)
#define RwCameraGetProjectionMacro(_camera) ((_camera)->projectionType)
#define RwCameraGetRasterMacro(_camera) ((_camera)->frameBuffer)
#define RwCameraGetViewMatrixMacro(_camera) (&((_camera)->viewMatrix))
#define RwCameraGetViewOffsetMacro(_camera) (&((_camera)->viewOffset))
#define RwCameraGetViewWindowMacro(_camera) (&((_camera)->viewWindow))
#define RwCameraGetZRasterMacro(_camera) ((_camera)->zBuffer)
#define RwCameraSetFogDistanceMacro(_camera, _distance) (((_camera)->fogPlane = (_distance)), (_camera))
#define RwCameraSetFrameMacro(_camera, _frame) (_rwObjectHasFrameSetFrame((_camera), (_frame)), (_camera))
#define RwCameraSetRasterMacro(_camera, _raster) (((_camera)->frameBuffer = (_raster)), (_camera))
#define RwCameraSetZRasterMacro(_camera, _raster) (((_camera)->zBuffer = (_raster)), (_camera))
#define RwFrameGetMatrixMacro(_f) (&(_f)->modelling)
#define RwGameCubeRasterGetExtensionMacro(_raster) ((RwGameCubeRasterExtension*)(((RwUInt8*)(_raster)) + _RwGameCubeRasterExtOffset))
#define RwImageGetDepthMacro(_image) ((_image)->depth)
#define RwImageGetHeightMacro(_image) ((_image)->height)
#define RwImageGetPixelsMacro(_image) ((_image)->cpPixels)
#define RwImageGetWidthMacro(_image) ((_image)->width)
#define RwRasterGetFormatMacro(_raster) ((((_raster)->cFormat) & (rwRASTERFORMATMASK >> 8)) << 8)
#define RwRasterGetHeightMacro(_raster) ((_raster)->height)
#define RwRasterGetParentMacro(_raster) ((_raster)->parent)
#define RwRasterGetStrideMacro(_raster) ((_raster)->stride)
#define RwRasterGetTypeMacro(_raster) (((_raster)->cType) & rwRASTERTYPEMASK)
#define RwRasterGetWidthMacro(_raster) ((_raster)->width)
#define RwTextureGetNameMacro(_tex) ((_tex)->name)
#define RwTextureGetRasterMacro(_tex) ((_tex)->raster)
#define rwTEXTUREADDRESSINGUMASK 0x00000F00
#define rwTEXTUREADDRESSINGVMASK 0x0000F000
#define rwTEXTUREADDRESSINGMASK (rwTEXTUREADDRESSINGUMASK | rwTEXTUREADDRESSINGVMASK)

#define RwTextureSetAddressingMacro(_tex, _addressing) \
    (((_tex)->filterAddressing = ((_tex)->filterAddressing & ~rwTEXTUREADDRESSINGMASK) | (((((RwUInt32)(_addressing)) << 8) & rwTEXTUREADDRESSINGUMASK) | ((((RwUInt32)(_addressing)) << 12) & rwTEXTUREADDRESSINGVMASK))), (_tex))

#define rwTEXTUREFILTERMODEMASK 0x000000FF

#define RwTextureSetFilterModeMacro(_tex, _filtering) \
    (((_tex)->filterAddressing = ((_tex)->filterAddressing & ~rwTEXTUREFILTERMODEMASK) | (((RwUInt32)(_filtering)) & rwTEXTUREFILTERMODEMASK)), (_tex))

#define rwObjectHasFrameSetFrame(object, frame) _rwObjectHasFrameSetFrame(object, frame)

#ifdef __cplusplus
extern "C" {
#endif

extern GXRenderModeObj* _RwDlRenderMode;

#ifndef RWDEBUG
extern volatile RwUInt16 _RwDlTokenCurrent;
extern RwInt32 _RwGameCubeRasterExtOffset;
#endif

RwCamera* RwCameraBeginUpdate(RwCamera* camera);
RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* color, RwInt32 clearMode);
RwCamera* RwCameraClone(RwCamera* camera);
RwCamera* RwCameraCreate(void);
RwBool RwCameraDestroy(RwCamera* camera);
RwCamera* RwCameraEndUpdate(RwCamera* camera);
RwFrustumTestResult RwCameraFrustumTestSphere(const RwCamera* camera, const RwSphere* sphere);
RwInt32 RwCameraGetPluginOffset(RwUInt32 pluginID);
RwCamera* RwCameraSetFarClipPlane(RwCamera* camera, RwReal farClip);
RwCamera* RwCameraSetNearClipPlane(RwCamera* camera, RwReal nearClip);
RwCamera* RwCameraSetProjection(RwCamera* camera, RwCameraProjection projection);
RwCamera* RwCameraSetViewOffset(RwCamera* camera, const RwV2d* offset);
RwCamera* RwCameraSetViewWindow(RwCamera* camera, const RwV2d* viewWindow);
RwCamera* RwCameraShowRaster(RwCamera* camera, void* dev, RwUInt32 flags);
RwFrame* RwFrameCreate(void);
RwBool RwFrameDestroy(RwFrame* frame);
RwBool RwFrameDestroyHierarchy(RwFrame* frame);
RwFrame* RwFrameForAllChildren(RwFrame* frame, RwFrameCallBack callBack, void* data);
RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwFrame* RwFrameGetRoot(const RwFrame* frame);
RwFrame* RwFrameOrthoNormalize(RwFrame* frame);
RwFrame* RwFrameRotate(RwFrame* frame, const RwV3d* axis, RwReal angle, RwOpCombineType combine);
RwFrame* RwFrameTransform(RwFrame* frame, const RwMatrix* transform, RwOpCombineType combine);
RwFrame* RwFrameTranslate(RwFrame* frame, const RwV3d* translation, RwOpCombineType combine);
RwFrame* RwFrameUpdateObjects(RwFrame* frame);
void RwGameCubeCameraTextureFlush(RwRaster* raster, RwBool boxFilter);
void RwGameCubeGetXFBs(void** xfbDisp, void** xfbCopy);
void RwGameCubeSetAlphaCompare(RwInt32 comp0, RwUInt8 ref0, RwInt32 op, RwInt32 comp1, RwUInt8 ref1);
RwBool RwIm3DEnd(void);
RwBool RwIm3DRenderIndexedPrimitive(RwPrimitiveType primType, RwImVertexIndex* indices, RwInt32 numIndices);
RwBool RwIm3DRenderPrimitive(RwPrimitiveType primType);
void* RwIm3DTransform(RwIm3DVertex* pVerts, RwUInt32 numVerts, RwMatrix* ltm, RwUInt32 flags);
RwImage* RwImageAllocatePixels(RwImage* image);
RwImage* RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth);
RwBool RwImageDestroy(RwImage* image);
RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster);
RwBool RwRasterClear(RwInt32 pixelValue);
RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags);
RwBool RwRasterDestroy(RwRaster* raster);
RwUInt8* RwRasterLock(RwRaster* raster, RwUInt8 level, RwInt32 lockMode);
RwRaster* RwRasterPopContext(void);
RwRaster* RwRasterPushContext(RwRaster* raster);
RwRaster* RwRasterRenderFast(RwRaster* raster, RwInt32 x, RwInt32 y);
RwRaster* RwRasterSetFromImage(RwRaster* raster, RwImage* image);
RwRaster* RwRasterUnlock(RwRaster* raster);
RwTexture* RwTextureCreate(RwRaster* raster);
RwBool RwTextureDestroy(RwTexture* texture);
RwBool RwTextureSetAutoMipmapping(RwBool enable);
RwBool RwTextureSetMipmapping(RwBool enable);
RwTexture* RwTextureSetRaster(RwTexture* texture, RwRaster* raster);
RwBool RwTextureSetReadCallBack(RwTextureCallBackRead callBack);
RxLockedPipe* RxLockedPipeAddFragment(RxLockedPipe* pipeline, RwUInt32* firstIndex, RxNodeDefinition* nodeDef0, ...);
RxPipeline* RxLockedPipeUnlock(RxLockedPipe* pipeline);
RxPipeline* RxPipelineCreate(void);
RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline, const RwChar* name, RxPipelineNode* start, RwInt32* nodeIndex);
RxLockedPipe* RxPipelineLock(RxPipeline* pipeline);
RxRenderStateVector* RxRenderStateVectorLoadDriverState(RxRenderStateVector* rsvp);
void _rwDlRenderStateSetZCompLoc(RwBool zBeforeTex);
void _rwDlTextureSet(RwTexture* texture, RwInt32 index);
RwBool _rwDlTokenQueryDone(RwUInt16 token);
void _rwDlTransformSetup(RwMatrix* ltm, RwBool normals);
RwBool _rwFrameSyncDirty(void);
void _rwObjectHasFrameSetFrame(void* object, RwFrame* frame);
void _rxPipelineDestroy(RxPipeline* Pipeline);

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
RwCamera* RwCameraGetCurrentCamera(void);
RwReal RwCameraGetFarClipPlane(const RwCamera* camera);
RwReal RwCameraGetFogDistance(const RwCamera* camera);
RwFrame* RwCameraGetFrame(const RwCamera* camera);
RwReal RwCameraGetNearClipPlane(const RwCamera* camera);
RwCameraProjection RwCameraGetProjection(const RwCamera* camera);
RwRaster* RwCameraGetRaster(const RwCamera* camera);
RwMatrix* RwCameraGetViewMatrix(RwCamera* camera);
const RwV2d* RwCameraGetViewOffset(const RwCamera* camera);
const RwV2d* RwCameraGetViewWindow(const RwCamera* camera);
RwRaster* RwCameraGetZRaster(const RwCamera* camera);
RwCamera* RwCameraSetFogDistance(RwCamera* camera, RwReal fogDistance);
RwCamera* RwCameraSetFrame(RwCamera* camera, RwFrame* frame);
RwCamera* RwCameraSetRaster(RwCamera* camera, RwRaster* raster);
RwCamera* RwCameraSetZRaster(RwCamera* camera, RwRaster* zRaster);
RwMatrix* RwFrameGetMatrix(RwFrame* frame);
RwGameCubeRasterExtension* RwGameCubeRasterGetExtension(RwRaster* raster);
RwInt32 RwImageGetDepth(const RwImage* image);
RwInt32 RwImageGetHeight(const RwImage* image);
RwUInt8* RwImageGetPixels(const RwImage* image);
RwInt32 RwImageGetWidth(const RwImage* image);
RwInt32 RwRasterGetFormat(const RwRaster* raster);
RwInt32 RwRasterGetHeight(const RwRaster* raster);
RwRaster* RwRasterGetParent(const RwRaster* raster);
RwInt32 RwRasterGetStride(const RwRaster* raster);
RwInt32 RwRasterGetType(const RwRaster* raster);
RwInt32 RwRasterGetWidth(const RwRaster* raster);
RwChar* RwTextureGetName(RwTexture* texture);
RwRaster* RwTextureGetRaster(const RwTexture* texture);
RwTexture* RwTextureSetAddressing(RwTexture* texture, RwTextureAddressMode addressing);
RwTexture* RwTextureSetFilterMode(RwTexture* texture, RwTextureFilterMode filtering);
RwUInt16 _rwDlTokenGetCurrent(void);
void _rwDlTokenSendNew(void);
void _rwDlTokenWaitDone(RwUInt16 token);
#endif

#ifdef __cplusplus
}
#endif

#if !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
#define RwCameraGetCurrentCamera() RwCameraGetCurrentCameraMacro()
#define RwCameraGetFarClipPlane(_camera) RwCameraGetFarClipPlaneMacro(_camera)
#define RwCameraGetFogDistance(_camera) RwCameraGetFogDistanceMacro(_camera)
#define RwCameraGetFrame(_camera) RwCameraGetFrameMacro(_camera)
#define RwCameraGetNearClipPlane(_camera) RwCameraGetNearClipPlaneMacro(_camera)
#define RwCameraGetProjection(_camera) RwCameraGetProjectionMacro(_camera)
#define RwCameraGetRaster(_camera) RwCameraGetRasterMacro(_camera)
#define RwCameraGetViewMatrix(_camera) RwCameraGetViewMatrixMacro(_camera)
#define RwCameraGetViewOffset(_camera) RwCameraGetViewOffsetMacro(_camera)
#define RwCameraGetViewWindow(_camera) RwCameraGetViewWindowMacro(_camera)
#define RwCameraGetZRaster(_camera) RwCameraGetZRasterMacro(_camera)
#define RwCameraSetFogDistance(_camera, _distance) RwCameraSetFogDistanceMacro(_camera, _distance)
#define RwCameraSetFrame(_camera, _frame) RwCameraSetFrameMacro(_camera, _frame)
#define RwCameraSetRaster(_camera, _raster) RwCameraSetRasterMacro(_camera, _raster)
#define RwCameraSetZRaster(_camera, _raster) RwCameraSetZRasterMacro(_camera, _raster)
#define RwFrameGetMatrix(_f) RwFrameGetMatrixMacro(_f)
#define RwGameCubeRasterGetExtension(_raster) RwGameCubeRasterGetExtensionMacro(_raster)
#define RwImageGetDepth(_image) RwImageGetDepthMacro(_image)
#define RwImageGetHeight(_image) RwImageGetHeightMacro(_image)
#define RwImageGetPixels(_image) RwImageGetPixelsMacro(_image)
#define RwImageGetWidth(_image) RwImageGetWidthMacro(_image)
#define RwRasterGetFormat(_raster) RwRasterGetFormatMacro(_raster)
#define RwRasterGetHeight(_raster) RwRasterGetHeightMacro(_raster)
#define RwRasterGetParent(_raster) RwRasterGetParentMacro(_raster)
#define RwRasterGetStride(_raster) RwRasterGetStrideMacro(_raster)
#define RwRasterGetType(_raster) RwRasterGetTypeMacro(_raster)
#define RwRasterGetWidth(_raster) RwRasterGetWidthMacro(_raster)
#define RwTextureGetName(_tex) RwTextureGetNameMacro(_tex)
#define RwTextureGetRaster(_tex) RwTextureGetRasterMacro(_tex)
#define RwTextureSetAddressing(_tex, _addressing) RwTextureSetAddressingMacro(_tex, _addressing)
#define RwTextureSetFilterMode(_tex, _filtering) RwTextureSetFilterModeMacro(_tex, _filtering)
#define _rwDlTokenGetCurrent() _rwDlTokenGetCurrentMacro()
#define _rwDlTokenSendNew() _rwDlTokenSendNewMacro()
#define _rwDlTokenWaitDone(_token) _rwDlTokenWaitDoneMacro(_token)
#endif

#endif

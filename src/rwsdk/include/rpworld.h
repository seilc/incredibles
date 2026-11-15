#ifndef RPWORLD_H
#define RPWORLD_H

#include "rwcore.h"

typedef struct RpAtomic RpAtomic;
typedef struct RpClump RpClump;
typedef struct RpGeometry RpGeometry;
typedef struct RpInterpolator RpInterpolator;
typedef struct RpLight RpLight;
typedef struct RpLightTie RpLightTie;
typedef struct RpMaterial RpMaterial;
typedef struct RpMaterialList RpMaterialList;
typedef struct RpMesh RpMesh;
typedef struct RpMeshHeader RpMeshHeader;
typedef struct RpMorphTarget RpMorphTarget;
typedef struct RpSector RpSector;
typedef struct RpTie RpTie;
typedef struct RpTriangle RpTriangle;
typedef struct RpVertexNormal RpVertexNormal;
typedef struct RpWorld RpWorld;
typedef struct RpWorldSector RpWorldSector;
typedef struct RxGameCubeDisplayList RxGameCubeDisplayList;
typedef struct RxGameCubePipeData RxGameCubePipeData;
typedef struct RxGameCubeVertexAttr RxGameCubeVertexAttr;
typedef struct RxGameCubeVertexBuffer RxGameCubeVertexBuffer;

typedef struct RpGameCubeVtxFmt RpGameCubeVtxFmt;

typedef void* (*RxGameCubeAllInOneCallBack)(void* object, RxGameCubePipeData* pipeData);
typedef void (*RxGameCubeMatFunc)(RwRGBAReal* ambientColor, RwRGBA* materialColor, RwReal ambientCoef);

enum RpGeometryFlag
{
    rpGEOMETRYTRISTRIP = 0x1,
    rpGEOMETRYPOSITIONS = 0x2,
    rpGEOMETRYTEXTURED = 0x4,
    rpGEOMETRYPRELIT = 0x8,
    rpGEOMETRYNORMALS = 0x10,
    rpGEOMETRYLIGHT = 0x20,
    rpGEOMETRYMODULATEMATERIALCOLOR = 0x40,
    rpGEOMETRYTEXTURED2 = 0x80,
    rpGEOMETRYNATIVE = 0x1000000,
    rpGEOMETRYNATIVEINSTANCE = 0x2000000,
    rpGEOMETRYFLAGSMASK = 0xFF,
    rpGEOMETRYNATIVEFLAGSMASK = 0xF000000,
    rpGEOMETRYFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpGeometryFlag RpGeometryFlag;

enum RpGeometryLockMode
{
    rpGEOMETRYLOCKPOLYGONS = 0x1,
    rpGEOMETRYLOCKVERTICES = 0x2,
    rpGEOMETRYLOCKNORMALS = 0x4,
    rpGEOMETRYLOCKPRELIGHT = 0x8,
    rpGEOMETRYLOCKTEXCOORDS = 0x10,
    rpGEOMETRYLOCKTEXCOORDS1 = 0x10,
    rpGEOMETRYLOCKTEXCOORDS2 = 0x20,
    rpGEOMETRYLOCKTEXCOORDS3 = 0x40,
    rpGEOMETRYLOCKTEXCOORDS4 = 0x80,
    rpGEOMETRYLOCKTEXCOORDS5 = 0x100,
    rpGEOMETRYLOCKTEXCOORDS6 = 0x200,
    rpGEOMETRYLOCKTEXCOORDS7 = 0x400,
    rpGEOMETRYLOCKTEXCOORDS8 = 0x800,
    rpGEOMETRYLOCKTEXCOORDSALL = 0xFF0,
    rpGEOMETRYLOCKALL = 0xFFF,
    rpGEOMETRYLOCKMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpGeometryLockMode RpGeometryLockMode;

enum RpInterpolatorFlag
{
    rpINTERPOLATORDIRTYINSTANCE = 0x1,
    rpINTERPOLATORDIRTYSPHERE = 0x2,
    rpINTERPOLATORNOFRAMEDIRTY = 0x4,
    rpINTERPOLATORFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpInterpolatorFlag RpInterpolatorFlag;

enum RpAtomicFlag
{
    rpATOMICCOLLISIONTEST = 0x1,
    rpATOMICRENDER = 0x4,
    rpATOMICFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpAtomicFlag RpAtomicFlag;

enum RwCriterionWorldID
{
    rwID_NAWORLDID = 0x500,
    rwID_MATERIALMODULE = 0x501,
    rwID_MESHMODULE = 0x502,
    rwID_GEOMETRYMODULE = 0x503,
    rwID_CLUMPMODULE = 0x504,
    rwID_LIGHTMODULE = 0x505,
    rwID_COLLISIONMODULE = 0x506,
    rwID_WORLDMODULE = 0x507,
    rwID_RANDOMMODULE = 0x508,
    rwID_WORLDOBJMODULE = 0x509,
    rwID_SECTORMODULE = 0x50A,
    rwID_BINWORLDMODULE = 0x50B,
    rwID_WORLDPIPEMODULE = 0x50D,
    rwID_BINMESHPLUGIN = 0x50E,
    rwID_RXWORLDDEVICEMODULE = 0x50F,
    rwID_NATIVEDATAPLUGIN = 0x510,
    rwID_VERTEXFMTPLUGIN = 0x511,
    rwCRITERIONWORLDIDFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwCriterionWorldID RwCriterionWorldID;

struct RpMaterial
{
    RwTexture* texture;
    RwRGBA color;
    RxPipeline* pipeline;
    RwSurfaceProperties surfaceProps;
    RwInt16 refCount;
    RwInt16 pad;
};

struct RpMaterialList
{
    RpMaterial** materials;
    RwInt32 numMaterials;
    RwInt32 space;
};

struct RpMesh
{
    RxVertexIndex* indices;
    RwUInt32 numIndices;
    RpMaterial* material;
};

struct RpMeshHeader
{
    RwUInt32 flags;
    RwUInt16 numMeshes;
    RwUInt16 serialNum;
    RwUInt32 totalIndicesInMesh;
    RwUInt32 firstMeshOffset;
};

struct RpMorphTarget
{
    RpGeometry* parentGeom;
    RwSphere boundingSphere;
    RwV3d* verts;
    RwV3d* normals;
};

struct RpTriangle
{
    RwUInt16 vertIndex[3];
    RwUInt16 matIndex;
};

struct RpGeometry
{
    RwObject object;
    RwUInt32 flags;
    RwUInt16 lockedSinceLastInst;
    RwInt16 refCount;
    RwInt32 numTriangles;
    RwInt32 numVertices;
    RwInt32 numMorphTargets;
    RwInt32 numTexCoordSets;
    RpMaterialList matList;
    RpTriangle* triangles;
    RwRGBA* preLitLum;
    RwTexCoords* texCoords[8];
    RpMeshHeader* mesh;
    RwResEntry* repEntry;
    RpMorphTarget* morphTarget;
};

struct RpVertexNormal
{
    RwInt8 x;
    RwInt8 y;
    RwInt8 z;
    RwUInt8 pad;
};

struct RpWorldSector
{
    RwInt32 type;
    RpTriangle* triangles;
    RwV3d* vertices;
    RpVertexNormal* normals;
    RwTexCoords* texCoords[8];
    RwRGBA* preLitLum;
    RwResEntry* repEntry;
    RwLinkList collAtomicsInWorldSector;
    RwLinkList lightsInWorldSector;
    RwBBox boundingBox;
    RwBBox tightBoundingBox;
    RpMeshHeader* mesh;
    RxPipeline* pipeline;
    RwUInt16 matListWindowBase;
    RwUInt16 numVertices;
    RwUInt16 numTriangles;
    RwUInt16 pad;
};

struct RpSector
{
    RwInt32 type;
};

enum RpLightType
{
    rpNALIGHTTYPE,
    rpLIGHTDIRECTIONAL,
    rpLIGHTAMBIENT,
    rpLIGHTPOINT = 128,
    rpLIGHTSPOT,
    rpLIGHTSPOTSOFT,
    rpLIGHTTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpLightType RpLightType;

enum RpLightFlag
{
    rpLIGHTLIGHTATOMICS = 0x1,
    rpLIGHTLIGHTWORLD = 0x2,
    rpLIGHTFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpLightFlag RpLightFlag;

struct RpLight
{
    RwObjectHasFrame object;
    RwReal radius;
    RwRGBAReal color;
    RwReal minusCosAngle;
    RwLinkList WorldSectorsInLight;
    RwLLLink inWorld;
    RwUInt16 lightFrame;
    RwUInt16 pad;
};

struct RpLightTie
{
    RwLLLink lightInWorldSector;
    RpLight* light;
    RwLLLink WorldSectorInLight;
    RpWorldSector* sect;
};

struct RpInterpolator
{
    RwInt32 flags;
    RwInt16 startMorphTarget;
    RwInt16 endMorphTarget;
    RwReal time;
    RwReal recipTime;
    RwReal position;
};

typedef RpClump* (*RpClumpCallBack)(RpClump* clump, void* data);

struct RpClump
{
    RwObject object;
    RwLinkList atomicList;
    RwLinkList lightList;
    RwLinkList cameraList;
    RwLLLink inWorldLink;
    RpClumpCallBack callback;
};

typedef RpAtomic* (*RpAtomicCallBackRender)(RpAtomic* atomic);

struct RpAtomic
{
    RwObjectHasFrame object;
    RwResEntry* repEntry;
    RpGeometry* geometry;
    RwSphere boundingSphere;
    RwSphere worldBoundingSphere;
    RpClump* clump;
    RwLLLink inClumpLink;
    RpAtomicCallBackRender renderCallBack;
    RpInterpolator interpolator;
    RwUInt16 renderFrame;
    RwUInt16 pad;
    RwLinkList llWorldSectorsInAtomic;
    RxPipeline* pipeline;
};

struct RpTie
{
    RwLLLink lAtomicInWorldSector;
    RpAtomic* apAtom;
    RwLLLink lWorldSectorInAtomic;
    RpWorldSector* worldSector;
};

enum RpWorldRenderOrder
{
    rpWORLDRENDERNARENDERORDER,
    rpWORLDRENDERFRONT2BACK,
    rpWORLDRENDERBACK2FRONT,
    rpWORLDRENDERORDERFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpWorldRenderOrder RpWorldRenderOrder;

typedef RpWorldSector* (*RpWorldSectorCallBackRender)(RpWorldSector* worldSector);

struct RpWorld
{
    RwObject object;
    RwUInt32 flags;
    RpWorldRenderOrder renderOrder;
    RpMaterialList matList;
    RpSector* rootSector;
    RwInt32 numTexCoordSets;
    RwInt32 numClumpsInWorld;
    RwLLLink* currentClumpLink;
    RwLinkList clumpList;
    RwLinkList lightList;
    RwLinkList directionalLightList;
    RwV3d worldOrigin;
    RwBBox boundingBox;
    RpWorldSectorCallBackRender renderCallBack;
    RxPipeline* pipeline;
};

struct RxGameCubeDisplayList
{
    void* displayList;
    RwUInt32 size;
};

struct RxGameCubePipeData
{
    RwResEntry* resEntry;
    RpMeshHeader* meshHeader;
    RwInt32 flags;
    RwRGBAReal ambientLightColor;
    RwInt32 ambientLight;
    RwUInt32 lightMask;
    RwInt32 numLights;
    void* nodeData;
};

struct RxGameCubeVertexAttr
{
    void* array;
    RwUInt8 attr;
    RwUInt8 stride;
    RwUInt8 indexType;
    RwUInt8 pad[1];
};

struct RxGameCubeVertexBuffer
{
    RwUInt16 token;
    RwUInt16 serialNumber;
    RwUInt32 flags;
    RwUInt32 numAttrArrays;
    RxGameCubeVertexAttr attr[1];
};

typedef RpAtomic* (*RpAtomicCallBack)(RpAtomic* atomic, void* data);
typedef RpMaterial* (*RpMaterialCallBack)(RpMaterial* material, void* data);
typedef RpMesh* (*RpMeshCallBack)(RpMesh* mesh, RpMeshHeader* meshHeader, void* pData);

#define RpGeometryGetTrianglesMacro(_geometry) ((_geometry)->triangles)
#define rpATOMIC 1

#define RpGameCubeGeometryGetVtxFmtMacro(_geometry) \
    (*RWPLUGINOFFSET(RpGameCubeVtxFmt *, (_geometry), _rpDlGeomVtxFmtOffset))

#define RpGameCubeWorldGetVtxFmtMacro(_world) \
    (*RWPLUGINOFFSET(RpGameCubeVtxFmt *, (_world), _rpDlWorldVtxFmtOffset))

#define RpAtomicGetBoundingSphereMacro(_atomic) \
    ((((_atomic)->interpolator.flags & rpINTERPOLATORDIRTYSPHERE)? _rpAtomicResyncInterpolatedSphere(_atomic), 0: 0), &((_atomic)->boundingSphere))

#define RpAtomicGetClumpMacro(_atomic) ((_atomic)->clump)
#define RpAtomicGetFrameMacro(_atomic) ((RwFrame *) rwObjectGetParent(_atomic))
#define RpAtomicGetGeometryMacro(_atomic) ((_atomic)->geometry)
#define RpAtomicGetRenderCallBackMacro(_atomic) ((_atomic)->renderCallBack)
#define RpAtomicRenderMacro(_atomic) ((_atomic)->renderCallBack(_atomic))
#define RpAtomicSetPipelineMacro(_atomic, _pipeline) (((_atomic)->pipeline = _pipeline), _atomic)

#define RpAtomicSetRenderCallBackMacro(_atomic, _callback)            \
    MACRO_START                                                       \
    {                                                                 \
        (_atomic)->renderCallBack = (_callback);                      \
        if (!(_atomic)->renderCallBack)                               \
        {                                                             \
            (_atomic)->renderCallBack = AtomicDefaultRenderCallBack;  \
        }                                                             \
    }                                                                 \
    MACRO_STOP

#define RpClumpGetFrameMacro(_clump) ((RwFrame *) rwObjectGetParent(_clump))
#define RpClumpSetFrameMacro(_clump, _frame) (rwObjectSetParent(_clump, _frame), (_clump))
#define RpGeometryGetFlagsMacro(_geometry) ((_geometry)->flags)
#define RpGeometryGetMaterialMacro(_geometry, _num) (((_geometry)->matList.materials)[(_num)])
#define RpGeometryGetMorphTargetMacro(_geometry, _index) (&((_geometry)->morphTarget[(_index)]))
#define RpGeometryGetNumMaterialsMacro(_geometry) ((_geometry)->matList.numMaterials)
#define RpGeometryGetNumTexCoordSetsMacro(_geometry) ((_geometry)->numTexCoordSets)
#define RpGeometryGetNumTrianglesMacro(_geometry) ((_geometry)->numTriangles)
#define RpGeometryGetNumVerticesMacro(_geometry) ((_geometry)->numVertices)
#define RpGeometryGetVertexTexCoordsMacro(_geometry, _uvIndex) ((_geometry)->texCoords[(_uvIndex) - 1])
#define RpGeometrySetFlagsMacro(_geometry, _flags) (((_geometry)->flags = (_flags)), (_geometry))
#define RpLightGetFrameMacro(_light) ((RwFrame *)rwObjectGetParent((_light)))
#define RpLightGetRadiusMacro(_light) ((_light)->radius)
#define RpLightSetFlagsMacro(_light, _flags) ((rwObjectSetFlags((_light), (_flags))), (_light))
#define RpLightSetFrameMacro(_light, _frame) (rwObjectHasFrameSetFrame((_light), (_frame)), (_light))
#define RpMaterialGetColorMacro(_material) (&((_material)->color))
#define RpMaterialSetColorMacro(_material, _color) \
    (RwRGBAAssign(&((_material)->color), (_color)), (_material))

#define RpMorphTargetGetVertexNormalsMacro(_mt) ((_mt)->normals)
#define RpMorphTargetGetVerticesMacro(_mt) ((_mt)->verts)

#define RpMorphTargetSetBoundingSphereMacro(_mt, _sphere) \
    (RwSphereAssign(&((_mt)->boundingSphere), (_sphere)), (_mt))

#define RpWorldGetBBoxMacro(_world) (&((_world)->boundingBox))
#define RpWorldGetMaterialMacro(_world, _num) (((_world)->matList.materials)[(_num)])
#define RpWorldGetNumMaterialsMacro(_world) ((_world)->matList.numMaterials)
#define RpMaterialGetTextureMacro(_material) ((_material)->texture)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RWDEBUG
extern RwInt32 _rpDlGeomVtxFmtOffset;
extern RwInt32 _rpDlWorldVtxFmtOffset;
#endif

RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic);
RpAtomic* RpAtomicCreate(void);
RwBool RpAtomicDestroy(RpAtomic* atomic);
RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame);
RpAtomic* RpAtomicSetGeometry(RpAtomic* atomic, RpGeometry* geometry, RwUInt32 flags);
RpAtomic* RpAtomicStreamRead(RwStream* stream);
RpAtomic* RpAtomicStreamWrite(RpAtomic* atomic, RwStream* stream);
RpClump* RpClumpAddAtomic(RpClump* clump, RpAtomic* atomic);
RwBool RpClumpDestroy(RpClump* clump);
RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* pData);
RwInt32 RpClumpGetNumAtomics(RpClump* clump);
RpClump* RpClumpRemoveAtomic(RpClump* clump, RpAtomic* atomic);
RpClump* RpClumpStreamRead(RwStream* stream);
RpClump* RpClumpStreamWrite(RpClump* clump, RwStream* stream);
RpGeometry* RpGeometryCreate(RwInt32 numVert, RwInt32 numTriangles, RwUInt32 format);
RwBool RpGeometryDestroy(RpGeometry* geometry);
RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* pData);
RpGeometry* RpGeometryLock(RpGeometry* geometry, RwInt32 lockMode);
RpGeometry* RpGeometryTriangleSetMaterial(RpGeometry* geometry, RpTriangle* triangle, RpMaterial* material);
RpGeometry* RpGeometryUnlock(RpGeometry* geometry);
RpLight* RpLightCreate(RwInt32 type);
RwBool RpLightDestroy(RpLight* light);
RpLight* RpLightSetColor(RpLight* light, const RwRGBAReal* color);
RpLight* RpLightSetConeAngle(RpLight* light, RwReal angle);
RpLight* RpLightSetRadius(RpLight* light, RwReal radius);
RpMaterial* RpMaterialCreate(void);
RwBool RpMaterialDestroy(RpMaterial* material);
RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture);
const RpMorphTarget* RpMorphTargetCalcBoundingSphere(const RpMorphTarget* morphTarget, RwSphere* boundingSphere);
RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera);
RpWorld* RpWorldAddLight(RpWorld* world, RpLight* light);
RpWorld* RpWorldCreate(RwBBox* boundingBox);
RwBool RpWorldDestroy(RpWorld* world);
RwBool RpWorldPluginAttach(void);
RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera);
RpWorld* RpWorldRemoveLight(RpWorld* world, RpLight* light);
RpWorld* RpWorldRender(RpWorld* world);
RpWorld* RwCameraGetWorld(const RwCamera* camera);
void RxGameCubeAllInOneSetRenderCallBack(RxPipelineNode* node, RxGameCubeAllInOneCallBack callback);
RxNodeDefinition* RxNodeDefinitionGetGameCubeAtomicAllInOne(void);
void _rpAtomicResyncInterpolatedSphere(RpAtomic* atomic);
RpMeshHeader* _rpMeshHeaderForAllMeshes(RpMeshHeader* meshHeader, RpMeshCallBack fpCallBack, void* pData);
RxGameCubeMatFunc _rwDlObjectRenderSetup(RwUInt32 flags, RwUInt32 lightMask, RwInt32 ambient, RwBool prelightAlpha);
void _rwDlVtxFmtSetup(RpGameCubeVtxFmt* fmt, RxGameCubePipeData* pipeData);

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
RwSphere* RpAtomicGetBoundingSphere(RpAtomic* atomic);
RpClump* RpAtomicGetClump(const RpAtomic* atomic);
RwFrame* RpAtomicGetFrame(const RpAtomic* atomic);
RpGeometry* RpAtomicGetGeometry(const RpAtomic* atomic);
RpAtomicCallBackRender RpAtomicGetRenderCallBack(const RpAtomic* atomic);
RpAtomic* RpAtomicRender(RpAtomic* atomic);
RpAtomic* RpAtomicSetPipeline(RpAtomic* atomic, RxPipeline* pipeline);
void RpAtomicSetRenderCallBack(RpAtomic* atomic, RpAtomicCallBackRender callback);
RwFrame* RpClumpGetFrame(const RpClump* clump);
RpClump* RpClumpSetFrame(RpClump* clump, RwFrame* frame);
RpGameCubeVtxFmt* RpGameCubeGeometryGetVtxFmt(RpGeometry* geometry);
RpGameCubeVtxFmt* RpGameCubeWorldGetVtxFmt(RpWorld* world);
RwUInt32 RpGeometryGetFlags(const RpGeometry* geometry);
RpMaterial* RpGeometryGetMaterial(const RpGeometry* geometry, RwInt32 matNum);
RpMorphTarget* RpGeometryGetMorphTarget(const RpGeometry* geometry, RwInt32 morphTarget);
RwInt32 RpGeometryGetNumMaterials(const RpGeometry* geometry);
RwInt32 RpGeometryGetNumTexCoordSets(const RpGeometry* geometry);
RwInt32 RpGeometryGetNumTriangles(const RpGeometry* geometry);
RwInt32 RpGeometryGetNumVertices(const RpGeometry* geometry);
RpTriangle* RpGeometryGetTriangles(const RpGeometry* geometry);
RwTexCoords* RpGeometryGetVertexTexCoords(const RpGeometry* geometry, RwTextureCoordinateIndex uvIndex);
RpGeometry* RpGeometrySetFlags(RpGeometry* geometry, RwUInt32 flags);
RwFrame* RpLightGetFrame(const RpLight* light);
RwReal RpLightGetRadius(const RpLight* light);
RpLight* RpLightSetFlags(RpLight* light, RwUInt32 flags);
RpLight* RpLightSetFrame(RpLight* light, RwFrame* frame);
const RwRGBA* RpMaterialGetColor(const RpMaterial* material);
RwTexture* RpMaterialGetTexture(const RpMaterial* material);
RpMaterial* RpMaterialSetColor(RpMaterial* material, const RwRGBA* color);
RwV3d* RpMorphTargetGetVertexNormals(const RpMorphTarget* morphTarget);
RwV3d* RpMorphTargetGetVertices(const RpMorphTarget* morphTarget);
RpMorphTarget* RpMorphTargetSetBoundingSphere(RpMorphTarget* morphTarget, const RwSphere* boundingSphere);
const RwBBox* RpWorldGetBBox(const RpWorld* world);
RpMaterial* RpWorldGetMaterial(const RpWorld* world, RwInt32 matNum);
RwInt32 RpWorldGetNumMaterials(const RpWorld* world);
#endif

#ifdef __cplusplus
}
#endif

#if !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
#define RpAtomicGetBoundingSphere(_atomic) RpAtomicGetBoundingSphereMacro(_atomic)
#define RpAtomicGetClump(_atomic) RpAtomicGetClumpMacro(_atomic)
#define RpAtomicGetFrame(_atomic) RpAtomicGetFrameMacro(_atomic)
#define RpAtomicGetGeometry(_atomic) RpAtomicGetGeometryMacro(_atomic)
#define RpAtomicGetRenderCallBack(_atomic) RpAtomicGetRenderCallBackMacro(_atomic)
#define RpAtomicRender(_atomic) RpAtomicRenderMacro(_atomic)
#define RpAtomicSetPipeline(_atomic, _pipeline) RpAtomicSetPipelineMacro(_atomic, _pipeline)
#define RpAtomicSetRenderCallBack(_atomic, _callback) RpAtomicSetRenderCallBackMacro(_atomic, _callback)
#define RpClumpGetFrame(_clump) RpClumpGetFrameMacro(_clump)
#define RpClumpSetFrame(_clump, _frame) RpClumpSetFrameMacro(_clump, _frame)
#define RpGameCubeGeometryGetVtxFmt(_geometry) RpGameCubeGeometryGetVtxFmtMacro(_geometry)
#define RpGameCubeWorldGetVtxFmt(_world) RpGameCubeWorldGetVtxFmtMacro(_world)
#define RpGeometryGetFlags(_geometry) RpGeometryGetFlagsMacro(_geometry)
#define RpGeometryGetMaterial(_geometry, _num) RpGeometryGetMaterialMacro(_geometry, _num)
#define RpGeometryGetMorphTarget(_geometry, _index) RpGeometryGetMorphTargetMacro(_geometry, _index)
#define RpGeometryGetNumMaterials(_geometry) RpGeometryGetNumMaterialsMacro(_geometry)
#define RpGeometryGetNumTexCoordSets(_geometry) RpGeometryGetNumTexCoordSetsMacro(_geometry)
#define RpGeometryGetNumTriangles(_geometry) RpGeometryGetNumTrianglesMacro(_geometry)
#define RpGeometryGetNumVertices(_geometry) RpGeometryGetNumVerticesMacro(_geometry)
#define RpGeometryGetTriangles(_geometry) RpGeometryGetTrianglesMacro(_geometry)

#define RpGeometryGetVertexTexCoords(_geometry, _uvIndex) \
    RpGeometryGetVertexTexCoordsMacro(_geometry, _uvIndex)

#define RpGeometrySetFlags(_geometry, _flags) RpGeometrySetFlagsMacro(_geometry, _flags)
#define RpLightGetFrame(_light) RpLightGetFrameMacro(_light)
#define RpLightGetRadius(_light) RpLightGetRadiusMacro(_light)
#define RpLightSetFlags(_light, _flags) RpLightSetFlagsMacro(_light, _flags)
#define RpLightSetFrame(_light, _frame) RpLightSetFrameMacro(_light, _frame)
#define RpMaterialGetColor(_material) RpMaterialGetColorMacro(_material)
#define RpMaterialGetTexture(_material) RpMaterialGetTextureMacro(_material)
#define RpMaterialSetColor(_material, _color) RpMaterialSetColorMacro(_material, _color)
#define RpMorphTargetGetVertexNormals(_mt) RpMorphTargetGetVertexNormalsMacro(_mt)
#define RpMorphTargetGetVertices(_mt) RpMorphTargetGetVerticesMacro(_mt)

#define RpMorphTargetSetBoundingSphere(_geometry, _sphere) \
    RpMorphTargetSetBoundingSphereMacro(_geometry, _sphere)

#define RpWorldGetBBox(_world) RpWorldGetBBoxMacro(_world)
#define RpWorldGetMaterial(_world, _num) RpWorldGetMaterialMacro(_world, _num)
#define RpWorldGetNumMaterials(_world) RpWorldGetNumMaterialsMacro(_world)
#endif

#endif

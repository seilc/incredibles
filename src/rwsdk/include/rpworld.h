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
typedef struct RpMeshHeader RpMeshHeader;
typedef struct RpMorphTarget RpMorphTarget;
typedef struct RpSector RpSector;
typedef struct RpTie RpTie;
typedef struct RpTriangle RpTriangle;
typedef struct RpVertexNormal RpVertexNormal;
typedef struct RpWorld RpWorld;
typedef struct RpWorldSector RpWorldSector;

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

#define RpAtomicGetFrameMacro(_atomic) ((RwFrame *) rwObjectGetParent(_atomic))
#define RpAtomicGetGeometryMacro(_atomic) ((_atomic)->geometry)
#define RpAtomicGetRenderCallBackMacro(_atomic) ((_atomic)->renderCallBack)
#define RpAtomicRenderMacro(_atomic) ((_atomic)->renderCallBack(_atomic))

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

#define RpGeometryGetMaterialMacro(_geometry, _num) (((_geometry)->matList.materials)[(_num)])
#define RpGeometryGetMorphTargetMacro(_geometry, _index) (&((_geometry)->morphTarget[(_index)]))
#define RpGeometryGetNumMaterialsMacro(_geometry) ((_geometry)->matList.numMaterials)
#define RpGeometryGetNumTexCoordSetsMacro(_geometry) ((_geometry)->numTexCoordSets)
#define RpGeometryGetNumVerticesMacro(_geometry) ((_geometry)->numVertices)
#define RpGeometryGetVertexTexCoordsMacro(_geometry, _uvIndex) ((_geometry)->texCoords[(_uvIndex) - 1])
#define RpLightGetFrameMacro(_light) ((RwFrame *)rwObjectGetParent((_light)))
#define RpLightSetFrameMacro(_light, _frame) (rwObjectHasFrameSetFrame((_light), (_frame)), (_light))
#define RpMorphTargetGetVertexNormalsMacro(_mt) ((_mt)->normals)
#define RpMorphTargetGetVerticesMacro(_mt) ((_mt)->verts)
#define RpWorldGetMaterialMacro(_world, _num) (((_world)->matList.materials)[(_num)])
#define RpWorldGetNumMaterialsMacro(_world) ((_world)->matList.numMaterials)
#define RpMaterialGetTextureMacro(_material) ((_material)->texture)

#ifdef __cplusplus
extern "C" {
#endif

RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic);
RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame);
RpGeometry* RpGeometryLock(RpGeometry* geometry, RwInt32 lockMode);
RpGeometry* RpGeometryUnlock(RpGeometry* geometry);
RpLight* RpLightCreate(RwInt32 type);
RwBool RpLightDestroy(RpLight* light);
RpLight* RpLightSetColor(RpLight* light, const RwRGBAReal* color);
RpLight* RpLightSetConeAngle(RpLight* light, RwReal angle);
RpLight* RpLightSetRadius(RpLight* light, RwReal radius);
RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture);
RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera);
RpWorld* RpWorldAddLight(RpWorld* world, RpLight* light);
RpWorld* RpWorldCreate(RwBBox* boundingBox);
RwBool RpWorldDestroy(RpWorld* world);
RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera);
RpWorld* RpWorldRemoveLight(RpWorld* world, RpLight* light);
RpWorld* RwCameraGetWorld(const RwCamera* camera);

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
RwFrame* RpAtomicGetFrame(const RpAtomic* atomic);
RpGeometry* RpAtomicGetGeometry(const RpAtomic* atomic);
RpAtomicCallBackRender RpAtomicGetRenderCallBack(const RpAtomic* atomic);
RpAtomic* RpAtomicRender(RpAtomic* atomic);
void RpAtomicSetRenderCallBack(RpAtomic* atomic, RpAtomicCallBackRender callback);
RpMaterial* RpGeometryGetMaterial(const RpGeometry* geometry, RwInt32 matNum);
RpMorphTarget* RpGeometryGetMorphTarget(const RpGeometry* geometry, RwInt32 morphTarget);
RwInt32 RpGeometryGetNumMaterials(const RpGeometry* geometry);
RwInt32 RpGeometryGetNumTexCoordSets(const RpGeometry* geometry);
RwInt32 RpGeometryGetNumVertices(const RpGeometry* geometry);
RwTexCoords* RpGeometryGetVertexTexCoords(const RpGeometry* geometry, RwTextureCoordinateIndex uvIndex);
RwFrame* RpLightGetFrame(const RpLight* light);
RpLight* RpLightSetFrame(RpLight* light, RwFrame* frame);
RwTexture* RpMaterialGetTexture(const RpMaterial* material);
RwV3d* RpMorphTargetGetVertexNormals(const RpMorphTarget* morphTarget);
RwV3d* RpMorphTargetGetVertices(const RpMorphTarget* morphTarget);
RpMaterial* RpWorldGetMaterial(const RpWorld* world, RwInt32 matNum);
RwInt32 RpWorldGetNumMaterials(const RpWorld* world);
#endif

#ifdef __cplusplus
}
#endif

#if !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
#define RpAtomicGetFrame(_atomic) RpAtomicGetFrameMacro(_atomic)
#define RpAtomicGetGeometry(_atomic) RpAtomicGetGeometryMacro(_atomic)
#define RpAtomicGetRenderCallBack(_atomic) RpAtomicGetRenderCallBackMacro(_atomic)
#define RpAtomicRender(_atomic) RpAtomicRenderMacro(_atomic)
#define RpAtomicSetRenderCallBack(_atomic, _callback) RpAtomicSetRenderCallBackMacro(_atomic, _callback)
#define RpGeometryGetMaterial(_geometry, _num) RpGeometryGetMaterialMacro(_geometry, _num)
#define RpGeometryGetMorphTarget(_geometry, _index) RpGeometryGetMorphTargetMacro(_geometry, _index)
#define RpGeometryGetNumMaterials(_geometry) RpGeometryGetNumMaterialsMacro(_geometry)
#define RpGeometryGetNumTexCoordSets(_geometry) RpGeometryGetNumTexCoordSetsMacro(_geometry)
#define RpGeometryGetNumVertices(_geometry) RpGeometryGetNumVerticesMacro(_geometry)

#define RpGeometryGetVertexTexCoords(_geometry, _uvIndex) \
    RpGeometryGetVertexTexCoordsMacro(_geometry, _uvIndex)

#define RpLightGetFrame(_light) RpLightGetFrameMacro(_light)
#define RpLightSetFrame(_light, _frame) RpLightSetFrameMacro(_light, _frame)
#define RpMaterialGetTexture(_material) RpMaterialGetTextureMacro(_material)
#define RpMorphTargetGetVertexNormals(_mt) RpMorphTargetGetVertexNormalsMacro(_mt)
#define RpMorphTargetGetVertices(_mt) RpMorphTargetGetVerticesMacro(_mt)
#define RpWorldGetMaterial(_world, _num) RpWorldGetMaterialMacro(_world, _num)
#define RpWorldGetNumMaterials(_world) RpWorldGetNumMaterialsMacro(_world)
#endif

#endif

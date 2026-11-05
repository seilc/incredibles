#ifndef RPCOLLIS_H
#define RPCOLLIS_H

#include "rpworld.h"

typedef struct RpCollSector RpCollSector;
typedef struct RpCollSectorLineItNodeInfo RpCollSectorLineItNodeInfo;
typedef struct RpCollSplit RpCollSplit;
typedef struct RpCollTree RpCollTree;
typedef struct RpCollisionData RpCollisionData;
typedef struct RpCollisionTriangle RpCollisionTriangle;
typedef union RpIntersectData RpIntersectData;
typedef struct RpIntersection RpIntersection;

struct RpCollSector
{
    RwUInt8 type;
    RwUInt8 contents;
    RwUInt16 index;
    RwReal value;
};

struct RpCollSplit
{
    RpCollSector leftSector;
    RpCollSector rightSector;
};

struct RpCollTree
{
    RwUInt32 flags;
    RwBBox bbox;
    RwUInt16 numEntries;
    RwUInt16 numSplits;
    RpCollSplit* splits;
    RwUInt16* map;
};

struct RpCollSectorLineItNodeInfo
{
    RpCollSector* sector;
    RwReal pa;
    RwReal pb;
};

struct RpCollisionData
{
    RpCollTree* tree;
};

enum RpIntersectType
{
    rpINTERSECTNONE,
    rpINTERSECTLINE,
    rpINTERSECTPOINT,
    rpINTERSECTSPHERE,
    rpINTERSECTBOX,
    rpINTERSECTATOMIC,
    rpINTERSECTTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpIntersectType RpIntersectType;

union RpIntersectData
{
    RwLine line;
    RwV3d point;
    RwSphere sphere;
    RwBBox box;
    void* object;
};

struct RpIntersection
{
    RpIntersectData t;
    RpIntersectType type;
};

struct RpCollisionTriangle
{
    RwV3d normal;
    RwV3d point;
    RwInt32 index;
    RwV3d* vertices[3];
};

typedef RpAtomic* (*RpIntersectionCallBackAtomic)(RpIntersection* intersection, RpWorldSector* sector, RpAtomic* atomic, RwReal distance, void* data);
typedef RpCollisionTriangle* (*RpIntersectionCallBackGeometryTriangle)(RpIntersection* intersection, RpCollisionTriangle* collTriangle, RwReal distance, void* data);
typedef RpWorldSector* (*RpIntersectionCallBackWorldSector)(RpIntersection* intersection, RpWorldSector* worldSector, void* data);
typedef RpCollisionTriangle* (*RpIntersectionCallBackWorldTriangle)(RpIntersection* intersection, RpWorldSector* sector, RpCollisionTriangle* collTriangle, RwReal distance, void* data);

#define _rpCollisionGeometryGetDataMacro(_geometry) \
    RWPLUGINOFFSET(RpCollisionData, _geometry, _rpCollisionGeometryDataOffset)

#define rpCOLLSECTOR_CONTENTS_SPLIT 0xff
#define rpCOLLSECTOR_TYPE_AXISMASK (0x08 | 0x04)
#define rpCOLLTREE_MAXDEPTH 32

#ifdef __cplusplus
extern "C" {
#endif

extern RwInt32 _rpCollisionGeometryDataOffset;

RpAtomic* RpAtomicForAllIntersections(RpAtomic* atomic, RpIntersection* intersection, RpIntersectionCallBackGeometryTriangle callBack, void* data);

#ifdef __cplusplus
}
#endif

#endif

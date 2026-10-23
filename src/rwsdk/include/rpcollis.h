#ifndef RPCOLLIS_H
#define RPCOLLIS_H

#include "rpworld.h"

typedef struct RpCollisionTriangle RpCollisionTriangle;
typedef union RpIntersectData RpIntersectData;
typedef struct RpIntersection RpIntersection;

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

typedef RpCollisionTriangle* (*RpIntersectionCallBackGeometryTriangle)(RpIntersection* intersection, RpCollisionTriangle* collTriangle, RwReal distance, void* data);
typedef RpCollisionTriangle* (*RpIntersectionCallBackWorldTriangle)(RpIntersection* intersection, RpWorldSector* sector, RpCollisionTriangle* collTriangle, RwReal distance, void* data);

#ifdef __cplusplus
extern "C" {
#endif

RpAtomic* RpAtomicForAllIntersections(RpAtomic* atomic, RpIntersection* intersection, RpIntersectionCallBackGeometryTriangle callBack, void* data);

#ifdef __cplusplus
}
#endif

#endif

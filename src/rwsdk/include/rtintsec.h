#ifndef RTINTSEC_H
#define RTINTSEC_H

#include "rwcore.h"

#define RTINTSECEDGEEPS (RwReal)(1e-5)
#define RTINTSECEPSILON (RwReal)(1e-8)

#ifdef __cplusplus
extern "C" {
#endif

RwBool RtIntersectionBBoxTriangle(const RwBBox* bbox, const RwV3d* v0, const RwV3d* v1, const RwV3d* v2);
RwBool RtIntersectionSphereTriangle(const RwSphere* sphere, const RwV3d* v0, const RwV3d* v1, const RwV3d* v2, RwV3d* normal, RwReal* distance);

#ifdef __cplusplus
}
#endif

#endif

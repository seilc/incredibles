#ifndef XCOLLIDEFAST_H
#define XCOLLIDEFAST_H

#include "xCollide.h"
#include "xBound.h"

U32 xRayHitsSphereFast(const xRay3* r, const xSphere* s);
U32 xRayHitsBoxFast(const xRay3* r, const xBox* b);
U32 xRayHitsCylinderFast(const xRay3* r, const xCylinder* c);
U32 xRayHitsBoundFast(const xRay3* r, const xBound* b);

#endif
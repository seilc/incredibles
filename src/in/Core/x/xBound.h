#ifndef XBOUND_H
#define XBOUND_H

#include "xMath3.h"
#include "xQuickCull.h"
#include "xCollide.h"

struct xBound
{
    xQCData qcd;
    U8 type;
    U8 pad[3];
    union
    {
        xSphere sph;
        xBBox box;
        xCylinder cyl;
    };
    xMat4x3* mat;
};

#define k_XBOUNDTYPE_NONE 0 // None
#define k_XBOUNDTYPE_SPHERE 1 // Sphere (xBound::sph)
#define k_XBOUNDTYPE_BOX 2 // Axis-aligned bounding box (xBound::box)
#define k_XBOUNDTYPE_CYLINDER 3 // Cylinder (xBound::cyl)
#define k_XBOUNDTYPE_OBB 4 // Oriented bounding box (xBound::box + xBound::mat)

void xBoundUpdate(xBound* b);
void xBoundGetBox(xBox& box, const xBound& bound);
void xBoundGetCylinder(xCylinder& o, const xBound& bound);
void xBoundGetSphere(xSphere& o, const xBound& bound);
void xBoundSphereHitsOBB(const xSphere* s, const xBox* b, const xMat4x3* m, xCollis* coll);
void xBoundHitsBound(const xBound* a, const xBound* b, xCollis* c);
void xRayHitsBound(const xRay3* r, const xBound* b, xCollis* c);
void xSphereHitsBound(const xSphere* o, const xBound* b, xCollis* c);
bool xBoundHitsBound(const xBound& a, const xBound& b);
bool xRayHitsBound(const xRay3& r, const xBound& b);
bool xSphereHitsBound(const xVec3& c, F32 r, const xBound& b);
bool xOBBHitsBound(const xBox& box, const xMat4x3& mat, const xBound& b);
void xBoundGetCenter(const xBound* b, xVec3* center);
xVec3* xBoundCenter(xBound* b);

#endif
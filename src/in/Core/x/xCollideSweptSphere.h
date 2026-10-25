#ifndef XCOLLIDESWEPTSPHERE_H
#define XCOLLIDESWEPTSPHERE_H

#include "xCollide.h"
#include "xQuickCull.h"
#include "xJSP.h"

struct xSweptSphere
{
    U8 detect_initial_penetration;
    U8 init_collide;
    xVec3 start;
    xVec3 end;
    F32 radius;
    F32 dist;
    xVec3 dir;
    xBox box;
    xQCData qcd;
    F32 curdist;
    U32 oid;
    void* optr;
    xModelInstance* mptr;
    xVec3 worldContact;
    S32 hitIt;
    xVec3 worldPos;
    xVec3 worldNormal;
    xVec3 worldTangent;
    xJSPHeader* jsp;
    xClumpCollBSPTriangle* bspTriangle;
    xCollideSphereHitType type;
    xPlane tri_plane;

    xSweptSphere();
};

struct xCollideLineSegment
{
    xVec3 start;
    xVec3 dir;
    F32 length;
};

struct xCollideProcessedTri
{
    struct xEdge
    {
        xCollideLineSegment segment;
        xPlane edge;
    };

    xPlane surface;
    xEdge edges[3];
};

void xSweptSphereResetCollision(xSweptSphere* sws);
S32 xSweptSphereToSphere(xSweptSphere* sws, const xSphere* sph);

#endif
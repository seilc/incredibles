#ifndef XHIERARCHYBOUND_H
#define XHIERARCHYBOUND_H

#include "xMath3.h"
#include "xCollideSweptSphere.h"

struct xHierarchyNode
{
    xSphere sphere;
    U16 userData;
    S8 bone;
};

struct xHierarchyBound
{
    xSphere master;
    xHierarchyNode* nodes;
    U8 count;
    U8 maxCount;
};

struct xHierarchyBoundInitData
{
    S8 bone;
    F32 radius;
    U16 userData;
};

void xHierarchyBoundInit(xHierarchyBound* bound, const xHierarchyBoundInitData* init,
                         F32 masterRadius, U32 extraBounds, F32 scale);
bool xHierarchyBoundTest(const xHierarchyBound* bound, const xRay3* testRay, U16* boundUserData,
                         xIsect* isect, S8* subsphere);
bool xHierarchyBoundTest(const xHierarchyBound* bound, xSweptSphere* sphere, U16* boundUserData,
                         S8* subsphere);

#endif
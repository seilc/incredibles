#ifndef XCLUMPCOLL_H
#define XCLUMPCOLL_H

#include "types.h"

#include <rwcore.h>
#include <rpcollis.h>

struct xClumpCollBSPBranchNode
{
    U32 leftInfo;
    U32 rightInfo;
    F32 leftValue;
    F32 rightValue;
};

struct xClumpCollBSPVertInfo
{
    U16 atomIndex;
    U16 meshVertIndex;
};

struct xClumpCollBSPTriangle
{
    union
    {
        xClumpCollBSPVertInfo i;
        U32 rawIdx;
        RwV3d* p;
    } v;
    U8 flags;
    U8 detailed_info_cache_index;
    U16 matIndex;
};

struct xClumpCollBSPTree
{
    S32 numBranchNodes;
    xClumpCollBSPBranchNode* branchNodes;
    S32 numTriangles;
    xClumpCollBSPTriangle* triangles;
};

struct nodeInfo
{
    U32 type;
    U32 index;
};

struct xClumpCollV3dGradient
{
    F32 dydx;
    F32 dzdx;
    F32 dxdy;
    F32 dzdy;
    F32 dxdz;
    F32 dydz;
};

union IntersectionCallBack
{
    RpIntersectionCallBackWorldSector sectorCB;
    RpIntersectionCallBackWorldTriangle worldCB;
    RpIntersectionCallBackAtomic atomicCB;
};

struct CallBackParam
{
    RpIntersection* intersection;
    IntersectionCallBack u;
    void* data;
};

typedef S32 (*xClumpCollBSPLeafCB)(xClumpCollBSPTriangle* triangles, void* data);

#define xClumpCollV3dGradientMacro(grad, vec)                                                     \
MACRO_START                                                                                       \
{                                                                                                 \
    RwReal recip;                                                                                 \
                                                                                                  \
    recip = ((vec)->x != 0.0f) ? (1.0f / (vec)->x) : 0.0f;                                        \
    (grad)->dydx = (vec)->y * recip;                                                              \
    (grad)->dzdx = (vec)->z * recip;                                                              \
                                                                                                  \
    recip = ((vec)->y != 0.0f) ? (1.0f / (vec)->y) : 0.0f;                                        \
    (grad)->dxdy = (vec)->x * recip;                                                              \
    (grad)->dzdy = (vec)->z * recip;                                                              \
                                                                                                  \
    recip = ((vec)->z != 0.0f) ? (1.0f / (vec)->z) : 0.0f;                                        \
    (grad)->dxdz = (vec)->x * recip;                                                              \
    (grad)->dydz = (vec)->y * recip;                                                              \
}                                                                                                 \
MACRO_STOP

#define xClumpCollTriangleNormalMacro(_normal, _v0, _v1, _v2)                                     \
MACRO_START                                                                                       \
{                                                                                                 \
    RwV3d vTmp, vTmp2;                                                                            \
    RwReal recipLength, lengthSq;                                                                 \
                                                                                                  \
    RwV3dSubMacro(&vTmp, (_v1), (_v0));                                                           \
    RwV3dSubMacro(&vTmp2, (_v2), (_v0));                                                          \
    RwV3dCrossProductMacro((_normal), &vTmp, &vTmp2);                                             \
    lengthSq = RwV3dDotProduct((_normal), (_normal));                                             \
    rwInvSqrtMacro(&recipLength, lengthSq);                                                       \
    RwV3dScaleMacro((_normal), (_normal), recipLength);                                           \
}                                                                                                 \
MACRO_STOP

extern U8 xClumpColl_FilterFlags;
extern U8 xClumpColl_FilterFlagsAllMask;
extern U8 xClumpColl_FilterFlagsAllEqual;

U32 xClumpColl_SetFilterFlags(U8 filterFlags, U8 allMask, U8 allEqual);
void xClumpColl_RestoreFilterFlags(U32 previousFlags);
xClumpCollBSPTree* xClumpColl_StaticBufferInit(void* data, U32);
void xClumpColl_InstancePointers(xClumpCollBSPTree*, RpClump*);
xClumpCollBSPTree* xClumpColl_ForAllLeafNodes(xClumpCollBSPTree* tree, xClumpCollBSPLeafCB callBack, void* data);
xClumpCollBSPTree* xClumpColl_ForAllBoxLeafNodeIntersections(xClumpCollBSPTree* tree, RwBBox* box, xClumpCollBSPLeafCB callBack, void* data);
xClumpCollBSPTree* xClumpColl_ForAllLineLeafNodeIntersections(xClumpCollBSPTree* tree, RwLine* line, xClumpCollV3dGradient* grad, xClumpCollBSPLeafCB callBack, void* data);
xClumpCollBSPTree* xClumpColl_ForAllCapsuleLeafNodeIntersections(xClumpCollBSPTree* tree, RwLine* line, F32 radius, xClumpCollV3dGradient* grad, xClumpCollBSPLeafCB callBack, void* data);
xClumpCollBSPTree* xClumpColl_ForAllIntersections(xClumpCollBSPTree* tree, RpIntersection* intersection, RpIntersectionCallBackWorldTriangle callBack, void* data);

inline bool xClumpColl_IsCollidableTriangle(U8 flags)
{
    return (flags & xClumpColl_FilterFlags) || (flags & xClumpColl_FilterFlagsAllMask) == xClumpColl_FilterFlagsAllEqual;
}

#endif

#ifndef XCLUMPCOLL_H
#define XCLUMPCOLL_H

#include <rwcore.h>
#include <types.h>

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

#endif
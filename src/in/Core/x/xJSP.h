#ifndef XJSP_H
#define XJSP_H

#include "xClumpColl.h"
#include "xColor.h"
#include "xVec3.h"

#include <rwcore.h>
#include <rpworld.h>

struct xJSPNodeInfo
{
    S32 originalMatIndex;
    U16 nodeFlags;
    S16 sortOrder;
};

struct xJSPNodeTreeBranch
{
    U16 leftNode;
    U16 rightNode;
    U8 leftType;
    U8 rightType;
    U16 coord;
    F32 leftValue;
    F32 rightValue;
};

struct xJSPNodeTreeLeaf
{
    S32 nodeIndex;
    S32 leafCount;
    RwBBox box;
};

struct xJSPNodeTree
{
    S32 numBranchNodes;
    xJSPNodeTreeBranch* branchNodes;
    S32 numLeafNodes;
    xJSPNodeTreeLeaf* leafNodes;
};

struct xJSPMiniLightTie
{
    RwLLLink lightInWorldSector;
    RpLight* light;
};

struct xJSPNodeLight
{
    RpAtomic* atomic;
    S32 lightCount;
    RpTie dummyTie;
    RpWorldSector dummySector;
    xJSPMiniLightTie dummyLightTie[16];
};

struct xJSPHeader
{
    char idtag[4];
    U32 version;
    U32 jspNodeCount;
    RpClump* clump;
    xClumpCollBSPTree* colltree;
    xJSPNodeInfo* jspNodeList;
    U32 stripVecCount;
    RwV3d* stripVecList;
    U16 vertDataFlags;
    U16 vertDataStride;
    xJSPNodeTree* nodetree;
    xJSPNodeLight* nodelight;
};

U32 get_matid(xJSPHeader* jsp, U32 old_mat_id, U32 jspAssetID);
void xJSP_MultiStreamRead(void* data, U32 size, xJSPHeader** jsp, U32 assetID);
void xJSP_Destroy(xJSPHeader* jsp);
xJSPNodeTree* xJSPNodeTreeBuild(xJSPHeader* jsp);
xVec3* xJSP_GetTriangleVertData(xJSPHeader* jsp, xClumpCollBSPTriangle* tri, xColor* color, xVec3* normal, F32* uv);

#endif

#include "xClumpColl.h"

#include "xMath.h"

#include <rtintsec.h>

U8 xClumpColl_FilterFlags = 0x4;
U8 xClumpColl_FilterFlagsAllMask = 0;
U8 xClumpColl_FilterFlagsAllEqual = 0xFF;

U32 xClumpColl_SetFilterFlags(U8 filterFlags, U8 allMask, U8 allEqual)
{
    U32 previousFlags = xClumpColl_FilterFlags | (xClumpColl_FilterFlagsAllMask << 8) | (xClumpColl_FilterFlagsAllEqual << 16);
    xClumpColl_FilterFlags = filterFlags;
    xClumpColl_FilterFlagsAllMask = allMask;
    xClumpColl_FilterFlagsAllEqual = allEqual;
    return previousFlags;
}

void xClumpColl_RestoreFilterFlags(U32 previousFlags)
{
    xClumpColl_FilterFlags = previousFlags;
    xClumpColl_FilterFlagsAllMask = previousFlags >> 8;
    xClumpColl_FilterFlagsAllEqual = previousFlags >> 16;
}

#define xClumpCollLinePlaneIntersectMacro(vOut, _line, grad, plane, val)                          \
MACRO_START                                                                                       \
{                                                                                                 \
    RwReal delta;                                                                                 \
    switch (plane) {                                                                              \
    case 0:                                                                                       \
        delta = (val) - (_line)->start.x;                                                         \
        (vOut)->x = (val);                                                                        \
        (vOut)->y = (_line)->start.y + (grad)->dydx * delta;                                      \
        (vOut)->z = (_line)->start.z + (grad)->dzdx * delta;                                      \
        break;                                                                                    \
    case 4:                                                                                       \
        delta = (val) - (_line)->start.y;                                                         \
        (vOut)->x = (_line)->start.x + (grad)->dxdy * delta;                                      \
        (vOut)->y = (val);                                                                        \
        (vOut)->z = (_line)->start.z + (grad)->dzdy * delta;                                      \
        break;                                                                                    \
    case 8:                                                                                       \
        delta = (val) - (_line)->start.z;                                                         \
        (vOut)->x = (_line)->start.x + (grad)->dxdz * delta;                                      \
        (vOut)->y = (_line)->start.y + (grad)->dydz * delta;                                      \
        (vOut)->z = (val);                                                                        \
        break;                                                                                    \
    }                                                                                             \
}                                                                                                 \
MACRO_STOP

struct PolyLineTestParam
{
    RwV3d start;
    RpWorldSector* worldSector;
    RwV3d delta;
    xClumpCollV3dGradient grad;
    RwLine line;
    CallBackParam* cbParam;
};

struct PolyTestParam
{
    RwBBox bbox;
    RpWorldSector* worldSector;
    void* leafTestData;
    CallBackParam* cbParam;
};

struct TestSphere
{
    RwSphere* sphere;
    F32 recipRadius;
};

#define xClumpCollLineTriangleIntersectMacro(_result,                                             \
                                             _lineStart, _lineDelta,                              \
                                             _v0, _v1, _v2,                                       \
                                             _distance)                                           \
MACRO_START                                                                                       \
{                                                                                                 \
    RwV3d edge1, edge2, tVec, pVec, qVec;                                                         \
    RwReal det;                                                                                   \
                                                                                                  \
    RwV3dSubMacro(&edge1, (_v1), (_v0));                                                          \
    RwV3dSubMacro(&edge2, (_v2), (_v0));                                                          \
                                                                                                  \
    RwV3dCrossProductMacro(&pVec, (_lineDelta), &edge2);                                          \
                                                                                                  \
    det = RwV3dDotProductMacro(&edge1, &pVec);                                                    \
    if (det < -RTINTSECEPSILON) {                                                                 \
        RwV3d edgetmp = edge1;                                                                    \
                                                                                                  \
        edge1 = edge2;                                                                            \
        edge2 = edgetmp;                                                                          \
        RwV3dCrossProductMacro(&pVec, (_lineDelta), &edge2);                                      \
        det = RwV3dDotProductMacro(&edge1, &pVec);                                                \
    }                                                                                             \
    (_result) = (det > RTINTSECEPSILON);                                                          \
                                                                                                  \
    if ((_result)) {                                                                              \
        RwReal lo, hi, u;                                                                         \
                                                                                                  \
        lo = -det * RTINTSECEDGEEPS;                                                              \
        hi = det - lo;                                                                            \
                                                                                                  \
        RwV3dSubMacro(&tVec, (_lineStart), (_v0));                                                \
        u = RwV3dDotProductMacro(&tVec, &pVec);                                                   \
        (_result) = (u >= lo && u <= hi);                                                         \
                                                                                                  \
        if ((_result)) {                                                                          \
            RwReal v;                                                                             \
                                                                                                  \
            RwV3dCrossProductMacro(&qVec, &tVec, &edge1);                                         \
            v = RwV3dDotProductMacro((_lineDelta), &qVec);                                        \
            (_result) = (v >= lo && (u + v) <= hi);                                               \
                                                                                                  \
            if ((_result)) {                                                                      \
                *(_distance) = RwV3dDotProductMacro(&edge2, &qVec);                               \
                                                                                                  \
                (_result) = (*(_distance) >= lo && *(_distance) <= hi);                           \
                                                                                                  \
                if ((_result)) {                                                                  \
                    *(_distance) = ((*(_distance)) / (det));                                      \
                }                                                                                 \
            }                                                                                     \
        }                                                                                         \
    }                                                                                             \
}                                                                                                 \
MACRO_STOP

#define xClumpCollSphereBBoxMacro(_bb, _center, _rad)                                             \
MACRO_START                                                                                       \
{                                                                                                 \
    (_bb)->inf = (_bb)->sup = *(_center);                                                         \
    (_bb)->inf.x -= (_rad);                                                                       \
    (_bb)->inf.y -= (_rad);                                                                       \
    (_bb)->inf.z -= (_rad);                                                                       \
    (_bb)->sup.x += (_rad);                                                                       \
    (_bb)->sup.y += (_rad);                                                                       \
    (_bb)->sup.z += (_rad);                                                                       \
}                                                                                                 \
MACRO_STOP

xClumpCollBSPTree* xClumpColl_StaticBufferInit(void* data, U32)
{
    U32* header = (U32*)data;
    U32 numBranchNodes = header[1];
    U32 numTriangles = header[2];
    xClumpCollBSPTree* tree;

    tree = (xClumpCollBSPTree*)RwMalloc(sizeof(xClumpCollBSPTree), rwMEMHINTDUR_NADURATION);

    if (numBranchNodes) {
        tree->branchNodes = (xClumpCollBSPBranchNode*)(header + 3);
        tree->triangles = (xClumpCollBSPTriangle*)(tree->branchNodes + numBranchNodes);
    } else {
        tree->branchNodes = NULL;
        tree->triangles = (xClumpCollBSPTriangle*)(header + 3);
    }

    tree->numBranchNodes = numBranchNodes;
    tree->numTriangles = numTriangles;

    return tree;
}

void xClumpColl_InstancePointers(xClumpCollBSPTree*, RpClump*)
{
}

// RESTORED: speculative
xClumpCollBSPTree* xClumpColl_ForAllLeafNodes(xClumpCollBSPTree* tree, xClumpCollBSPLeafCB callBack, void* data)
{
    S32 nStack;
    nodeInfo nodeStack[33], node;

    node.type = tree->branchNodes ? 2 : 1;
    node.index = 0;
    nStack = 0;

    while (nStack >= 0) {
        if (node.type == 1) {
            xClumpCollBSPTriangle* tris = tree->triangles + node.index;
            if (!callBack(tris, data)) {
                return NULL;
            }

            node = nodeStack[nStack];
            nStack--;
        } else {
            xClumpCollBSPBranchNode* branch = tree->branchNodes + node.index;

            node.type = branch->leftInfo & 3;
            node.index = branch->leftInfo >> 12;

            nStack++;
            nodeStack[nStack].type = branch->rightInfo & 3;
            nodeStack[nStack].index = branch->rightInfo >> 12;
        }
    }

    return tree;
}
// END RESTORED

xClumpCollBSPTree* xClumpColl_ForAllBoxLeafNodeIntersections(xClumpCollBSPTree* tree, RwBBox* box, xClumpCollBSPLeafCB callBack, void* data)
{
    S32 nStack;
    nodeInfo nodeStack[33], node;

    node.type = tree->branchNodes ? 2 : 1;
    node.index = 0;
    nStack = 0;

    while (nStack >= 0) {
        if (node.type == 1) {
            xClumpCollBSPTriangle* tris = tree->triangles + node.index;
            if (!callBack(tris, data)) {
                return NULL;
            }

            node = nodeStack[nStack];
            nStack--;
        } else {
            xClumpCollBSPBranchNode* branch = tree->branchNodes + node.index;

            if (GETCOORD(box->inf, branch->leftInfo & 0xC) < branch->leftValue) {
                node.type = branch->leftInfo & 3;
                node.index = branch->leftInfo >> 12;

                if (GETCOORD(box->sup, branch->leftInfo & 0xC) >= branch->rightValue) {
                    nStack++;
                    nodeStack[nStack].type = branch->rightInfo & 3;
                    nodeStack[nStack].index = branch->rightInfo >> 12;
                }
            } else {
                node.type = branch->rightInfo & 3;
                node.index = branch->rightInfo >> 12;
            }
        }
    }

    return tree;
}

xClumpCollBSPTree* xClumpColl_ForAllLineLeafNodeIntersections(xClumpCollBSPTree* tree, RwLine* line, xClumpCollV3dGradient* grad, xClumpCollBSPLeafCB callBack, void* data)
{
    S32 nStack;
    nodeInfo nodeStack[33], node;
    RwLine lineStack[33], currLine;

    node.type = tree->branchNodes ? 2 : 1;
    node.index = 0;
    currLine = *line;
    nStack = 0;

    while (nStack >= 0) {
        if (node.type == 1) {
            xClumpCollBSPTriangle* tris = tree->triangles + node.index;
            if (!callBack(tris, data)) {
                return NULL;
            }

            node = nodeStack[nStack];
            currLine = lineStack[nStack];
            nStack--;
        } else {
            RwSplitBits lStart, lEnd;
            RwSplitBits rStart, rEnd;
            xClumpCollBSPBranchNode* branch;

#define PUSH_NODE_INFO(_type, _index, lineStart, lineEnd)                                         \
MACRO_START                                                                                       \
{                                                                                                 \
    nStack++;                                                                                     \
    nodeStack[nStack].type = (_type);                                                             \
    nodeStack[nStack].index = (_index);                                                           \
    lineStack[nStack].start = (lineStart);                                                        \
    lineStack[nStack].end = (lineEnd);                                                            \
}                                                                                                 \
MACRO_STOP

            branch = tree->branchNodes + node.index;

            lStart.nReal = GETCOORD(currLine.start, branch->leftInfo & 0xC) - branch->leftValue;
            lEnd.nReal = GETCOORD(currLine.end, branch->leftInfo & 0xC) - branch->leftValue;
            rStart.nReal = GETCOORD(currLine.start, branch->leftInfo & 0xC) - branch->rightValue;
            rEnd.nReal = GETCOORD(currLine.end, branch->leftInfo & 0xC) - branch->rightValue;

            if (rStart.nInt < 0 && rEnd.nInt < 0) {
                node.type = branch->leftInfo & 3;
                node.index = branch->leftInfo >> 12;
            }
            else if (lStart.nInt >= 0 && lEnd.nInt >= 0) {
                node.type = branch->rightInfo & 3;
                node.index = branch->rightInfo >> 12;
            }
            else if (!((lStart.nInt ^ lEnd.nInt) & 0x80000000) && !((rStart.nInt ^ rEnd.nInt) & 0x80000000)) {
                if (rStart.nInt < rEnd.nInt) {
                    PUSH_NODE_INFO(branch->rightInfo & 3, branch->rightInfo >> 12, currLine.start, currLine.end);
                    node.type = branch->leftInfo & 3;
                    node.index = branch->leftInfo >> 12;
                } else {
                    PUSH_NODE_INFO(branch->leftInfo & 3, branch->leftInfo >> 12, currLine.start, currLine.end);
                    node.type = branch->rightInfo & 3;
                    node.index = branch->rightInfo >> 12;
                }
            }
            else if (((lStart.nInt ^ lEnd.nInt) & 0x80000000) && (rStart.nInt >= 0 && rEnd.nInt >= 0)) {
                RwV3d vTmp = { 0.0f, 0.0f, 0.0f };

                xClumpCollLinePlaneIntersectMacro(&vTmp, &currLine, grad, branch->leftInfo & 0xC, branch->leftValue);

                if (lStart.nInt < 0) {
                    PUSH_NODE_INFO(branch->rightInfo & 3, branch->rightInfo >> 12, currLine.start, currLine.end);
                    node.type = branch->leftInfo & 3;
                    node.index = branch->leftInfo >> 12;
                    currLine.end = vTmp;
                } else {
                    PUSH_NODE_INFO(branch->leftInfo & 3, branch->leftInfo >> 12, vTmp, currLine.end);
                    node.type = branch->rightInfo & 3;
                    node.index = branch->rightInfo >> 12;
                }
            }
            else if (((rStart.nInt ^ rEnd.nInt) & 0x80000000) && (lStart.nInt < 0 && lEnd.nInt < 0)) {
                RwV3d vTmp = { 0.0f, 0.0f, 0.0f };

                xClumpCollLinePlaneIntersectMacro(&vTmp, &currLine, grad, branch->leftInfo & 0xC, branch->rightValue);

                if (rStart.nInt < 0) {
                    PUSH_NODE_INFO(branch->rightInfo & 3, branch->rightInfo >> 12, vTmp, currLine.end);
                    node.type = branch->leftInfo & 3;
                    node.index = branch->leftInfo >> 12;
                } else {
                    PUSH_NODE_INFO(branch->leftInfo & 3, branch->leftInfo >> 12, currLine.start, currLine.end);
                    node.type = branch->rightInfo & 3;
                    node.index = branch->rightInfo >> 12;
                    currLine.end = vTmp;
                }
            } else {
                RwV3d vLeft = { 0.0f, 0.0f, 0.0f }, vRight = { 0.0f, 0.0f, 0.0f };

                xClumpCollLinePlaneIntersectMacro(&vLeft, &currLine, grad, branch->leftInfo & 0xC, branch->leftValue);
                xClumpCollLinePlaneIntersectMacro(&vRight, &currLine, grad, branch->leftInfo & 0xC, branch->rightValue);

                if (lStart.nInt < 0) {
                    PUSH_NODE_INFO(branch->rightInfo & 3, branch->rightInfo >> 12, vRight, currLine.end);
                    node.type = branch->leftInfo & 3;
                    node.index = branch->leftInfo >> 12;
                    currLine.end = vLeft;
                } else {
                    PUSH_NODE_INFO(branch->leftInfo & 3, branch->leftInfo >> 12, vLeft, currLine.end);
                    node.type = branch->rightInfo & 3;
                    node.index = branch->rightInfo >> 12;
                    currLine.end = vRight;
                }
            }
        }
    }

    return tree;
}

xClumpCollBSPTree* xClumpColl_ForAllCapsuleLeafNodeIntersections(xClumpCollBSPTree* tree, RwLine* line, F32 radius, xClumpCollV3dGradient* grad, xClumpCollBSPLeafCB callBack, void* data)
{
    S32 nStack;
    nodeInfo nodeStack[33], node;
    RwLine lineStack[33], currLine;

    radius += 1.0f;

    node.type = tree->branchNodes ? 2 : 1;
    node.index = 0;
    currLine = *line;
    nStack = 0;

    while (nStack >= 0) {
        if (node.type == 1) {
            xClumpCollBSPTriangle* tris = tree->triangles + node.index;
            if (!callBack(tris, data)) {
                return NULL;
            }

            node = nodeStack[nStack];
            currLine = lineStack[nStack];
            nStack--;
        } else {
            RwSplitBits lStart, lEnd;
            RwSplitBits rStart, rEnd;
            xClumpCollBSPBranchNode* branch;

#define PUSH_NODE_INFO(_type, _index, lineStart, lineEnd)                                         \
MACRO_START                                                                                       \
{                                                                                                 \
    nStack++;                                                                                     \
    nodeStack[nStack].type = (_type);                                                             \
    nodeStack[nStack].index = (_index);                                                           \
    lineStack[nStack].start = (lineStart);                                                        \
    lineStack[nStack].end = (lineEnd);                                                            \
}                                                                                                 \
MACRO_STOP

            branch = tree->branchNodes + node.index;

            lStart.nReal = GETCOORD(currLine.start, branch->leftInfo & 0xC) - (branch->leftValue + radius);
            lEnd.nReal = GETCOORD(currLine.end, branch->leftInfo & 0xC) - (branch->leftValue + radius);
            rStart.nReal = GETCOORD(currLine.start, branch->leftInfo & 0xC) - (branch->rightValue - radius);
            rEnd.nReal = GETCOORD(currLine.end, branch->leftInfo & 0xC) - (branch->rightValue - radius);

            if (rStart.nInt < 0 && rEnd.nInt < 0) {
                node.type = branch->leftInfo & 3;
                node.index = branch->leftInfo >> 12;
            }
            else if (lStart.nInt >= 0 && lEnd.nInt >= 0) {
                node.type = branch->rightInfo & 3;
                node.index = branch->rightInfo >> 12;
            }
            else if (!((lStart.nInt ^ lEnd.nInt) & 0x80000000) && !((rStart.nInt ^ rEnd.nInt) & 0x80000000)) {
                if (rStart.nInt < rEnd.nInt) {
                    PUSH_NODE_INFO(branch->rightInfo & 3, branch->rightInfo >> 12, currLine.start, currLine.end);
                    node.type = branch->leftInfo & 3;
                    node.index = branch->leftInfo >> 12;
                } else {
                    PUSH_NODE_INFO(branch->leftInfo & 3, branch->leftInfo >> 12, currLine.start, currLine.end);
                    node.type = branch->rightInfo & 3;
                    node.index = branch->rightInfo >> 12;
                }
            }
            else if (((lStart.nInt ^ lEnd.nInt) & 0x80000000) && (rStart.nInt >= 0 && rEnd.nInt >= 0)) {
                RwV3d vTmp = { 0.0f, 0.0f, 0.0f };

                xClumpCollLinePlaneIntersectMacro(&vTmp, &currLine, grad, branch->leftInfo & 0xC, branch->leftValue);

                if (lStart.nInt < 0) {
                    PUSH_NODE_INFO(branch->rightInfo & 3, branch->rightInfo >> 12, currLine.start, currLine.end);
                    node.type = branch->leftInfo & 3;
                    node.index = branch->leftInfo >> 12;
                    currLine.end = vTmp;
                } else {
                    PUSH_NODE_INFO(branch->leftInfo & 3, branch->leftInfo >> 12, vTmp, currLine.end);
                    node.type = branch->rightInfo & 3;
                    node.index = branch->rightInfo >> 12;
                }
            }
            else if (((rStart.nInt ^ rEnd.nInt) & 0x80000000) && (lStart.nInt < 0 && lEnd.nInt < 0)) {
                RwV3d vTmp = { 0.0f, 0.0f, 0.0f };

                xClumpCollLinePlaneIntersectMacro(&vTmp, &currLine, grad, branch->leftInfo & 0xC, branch->rightValue);

                if (rStart.nInt < 0) {
                    PUSH_NODE_INFO(branch->rightInfo & 3, branch->rightInfo >> 12, vTmp, currLine.end);
                    node.type = branch->leftInfo & 3;
                    node.index = branch->leftInfo >> 12;
                } else {
                    PUSH_NODE_INFO(branch->leftInfo & 3, branch->leftInfo >> 12, currLine.start, currLine.end);
                    node.type = branch->rightInfo & 3;
                    node.index = branch->rightInfo >> 12;
                    currLine.end = vTmp;
                }
            } else {
                RwV3d vLeft = { 0.0f, 0.0f, 0.0f }, vRight = { 0.0f, 0.0f, 0.0f };

                xClumpCollLinePlaneIntersectMacro(&vLeft, &currLine, grad, branch->leftInfo & 0xC, branch->leftValue);
                xClumpCollLinePlaneIntersectMacro(&vRight, &currLine, grad, branch->leftInfo & 0xC, branch->rightValue);

                if (lStart.nInt < 0) {
                    PUSH_NODE_INFO(branch->rightInfo & 3, branch->rightInfo >> 12, vRight, currLine.end);
                    node.type = branch->leftInfo & 3;
                    node.index = branch->leftInfo >> 12;
                    currLine.end = vLeft;
                } else {
                    PUSH_NODE_INFO(branch->leftInfo & 3, branch->leftInfo >> 12, vLeft, currLine.end);
                    node.type = branch->rightInfo & 3;
                    node.index = branch->rightInfo >> 12;
                    currLine.end = vRight;
                }
            }
        }
    }

    return tree;
}

static S32 LeafNodeLinePolyIntersect(xClumpCollBSPTriangle* triangles, void* data)
{
    PolyLineTestParam* isData = (PolyLineTestParam*)data;
    CallBackParam* cbParam = isData->cbParam;

    do {
        RwV3d *v0, *v1, *v2;
        RwReal distance = 0.0f;
        S32 result;

        if (xClumpColl_IsCollidableTriangle(triangles->flags)) {
            v0 = triangles->v.p;
            if (triangles->flags & 0x2) {
                v1 = triangles->v.p + 2;
                v2 = triangles->v.p + 1;
            } else {
                v1 = triangles->v.p + 1;
                v2 = triangles->v.p + 2;
            }

            xClumpCollLineTriangleIntersectMacro(result, &isData->start, &isData->delta, v0, v1, v2, &distance);

            if (result) {
                RpCollisionTriangle collisionTri;

                collisionTri.point = *v0;
                collisionTri.index = (S32)triangles;
                collisionTri.vertices[0] = v0;
                collisionTri.vertices[1] = v1;
                collisionTri.vertices[2] = v2;

                xClumpCollTriangleNormalMacro(&collisionTri.normal, collisionTri.vertices[0], collisionTri.vertices[1], collisionTri.vertices[2]);

                if (!cbParam->u.worldCB(cbParam->intersection, NULL, &collisionTri, distance, cbParam->data)) {
                    return FALSE;
                }
            }
        }
    } while (triangles++->flags & 0x1);

    return TRUE;
}

static S32 LeafNodeSpherePolyIntersect(xClumpCollBSPTriangle* triangles, void* data)
{
    PolyTestParam* isData = (PolyTestParam*)data;
    CallBackParam* cbParam = isData->cbParam;
    TestSphere* testSphere = (TestSphere*)isData->leafTestData;

    do {
        RwV3d *v0, *v1, *v2;
        RwReal distance;
        RpCollisionTriangle collisionTri;

        if (xClumpColl_IsCollidableTriangle(triangles->flags)) {
            v0 = triangles->v.p;
            if (triangles->flags & 0x2) {
                v1 = triangles->v.p + 2;
                v2 = triangles->v.p + 1;
            } else {
                v1 = triangles->v.p + 1;
                v2 = triangles->v.p + 2;
            }

            if (RtIntersectionSphereTriangle(testSphere->sphere, v0, v1, v2, &collisionTri.normal, &distance)) {
                collisionTri.point = *v0;
                collisionTri.index = (S32)triangles;
                collisionTri.vertices[0] = v0;
                collisionTri.vertices[1] = v1;
                collisionTri.vertices[2] = v2;

                distance *= testSphere->recipRadius;

                if (!cbParam->u.worldCB(cbParam->intersection, NULL, &collisionTri, distance, cbParam->data)) {
                    return FALSE;
                }
            }
        }
    } while (triangles++->flags & 0x1);

    return TRUE;
}

static S32 LeafNodeBoxPolyIntersect(xClumpCollBSPTriangle* triangles, void* data)
{
    PolyTestParam* isData = (PolyTestParam*)data;
    CallBackParam* cbParam = isData->cbParam;

    do {
        RwV3d *v0, *v1, *v2;

        if (xClumpColl_IsCollidableTriangle(triangles->flags)) {
            v0 = triangles->v.p;
            if (triangles->flags & 0x2) {
                v1 = triangles->v.p + 2;
                v2 = triangles->v.p + 1;
            } else {
                v1 = triangles->v.p + 1;
                v2 = triangles->v.p + 2;
            }

            if (RtIntersectionBBoxTriangle(&isData->bbox, v0, v1, v2)) {
                RpCollisionTriangle collisionTri;

                collisionTri.point = *v0;
                collisionTri.index = (S32)triangles;
                xClumpCollTriangleNormalMacro(&collisionTri.normal, v0, v1, v2);
                collisionTri.vertices[0] = v0;
                collisionTri.vertices[1] = v1;
                collisionTri.vertices[2] = v2;

                if (!cbParam->u.worldCB(cbParam->intersection, NULL, &collisionTri, 0.0f, cbParam->data)) {
                    return FALSE;
                }
            }
        }
    } while (triangles++->flags & 0x1);

    return TRUE;
}

xClumpCollBSPTree* xClumpColl_ForAllIntersections(xClumpCollBSPTree* tree, RpIntersection* intersection, RpIntersectionCallBackWorldTriangle callBack, void* data)
{
    CallBackParam cbParam;

    cbParam.intersection = intersection;
    cbParam.u.worldCB = callBack;
    cbParam.data = data;

    switch (intersection->type) {
    case rpINTERSECTPOINT:
        return NULL;
    case rpINTERSECTLINE: {
        PolyLineTestParam isData;
        RwLine* line = &intersection->t.line;

        isData.start = intersection->t.line.start;
        RwV3dSub(&isData.delta, &line->end, &line->start);
        isData.cbParam = &cbParam;
        isData.line = *line;

        xClumpCollV3dGradientMacro(&isData.grad, &isData.delta);

        xClumpColl_ForAllLineLeafNodeIntersections(tree, line, &isData.grad, LeafNodeLinePolyIntersect, &isData);

        return tree;
    }
    case rpINTERSECTSPHERE: {
        PolyTestParam isData;
        TestSphere testSphere;

        xClumpCollSphereBBoxMacro(&isData.bbox, &intersection->t.sphere.center, intersection->t.sphere.radius);

        testSphere.sphere = &intersection->t.sphere;
        testSphere.sphere->radius = MAX(1e-5f, testSphere.sphere->radius);
        testSphere.recipRadius = 1.0f / testSphere.sphere->radius;
        isData.leafTestData = &testSphere;
        isData.cbParam = &cbParam;

        xClumpColl_ForAllBoxLeafNodeIntersections(tree, &isData.bbox, LeafNodeSpherePolyIntersect, &isData);

        return tree;
    }
    case rpINTERSECTBOX: {
        PolyTestParam isData;

        isData.bbox = intersection->t.box;
        isData.cbParam = &cbParam;

        xClumpColl_ForAllBoxLeafNodeIntersections(tree, &isData.bbox, LeafNodeBoxPolyIntersect, &isData);

        return tree;
    }
    }

    return NULL;
}

static S32 FastIntersectSphereTriangle(RwSphere* sphere, RwV3d* v0, RwV3d* v1, RwV3d* v2, RwV3d* normal, RwReal* distance, RwV3d* vc);

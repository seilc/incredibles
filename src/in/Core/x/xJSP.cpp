#include "xJSP.h"

#include "xDebug.h"
#include "xGlobals.h"
#include "xString.h"
#include "xScene.h"
#include "iAsync.h"

#include <stdio.h>
#include <string.h>

enum
{
    rwID_HI_BEEF01 = 0xBEEF01,
    rwID_HI_JSPINFO = 0xBEEF02,
    rwID_HI_BEEF03 = 0xBEEF03,
    rwID_HI_BEEF04 = 0xBEEF04
};

struct __rwMark
{
    U32 type;
    U32 length;
    U32 libraryID;
};

struct xJSPExtraInfo
{
    char idtag[4];
    xBox bounds;
};

struct xJSPHeaderEx : xJSPHeader
{
    xJSPExtraInfo extraInfo;
};

struct TempSplitChild
{
    S32 node;
    S32 type;
};

struct TempSplit
{
    TempSplitChild leftChild;
    TempSplitChild rightChild;
    S32 leftCount;
    S32 rightCount;
    S32 axis;
    F32 leftValue;
    F32 rightValue;
};

static RwV3d* sCurrVert;
static U32 sAtomicStartCount;
static RwV3d** sAtomicStartVert;

static void xJSPNodeLightBuild(xJSPHeader* jsp);

static RpAtomic* CountAtomicCB(RpAtomic* atomic, void* data)
{
    sAtomicStartCount++;
    *(U32*)data += atomic->geometry->mesh->totalIndicesInMesh;
    return atomic;
}

static RpMesh* AddMeshCB(RpMesh* mesh, RpMeshHeader*, void* pData)
{
    RwV3d** stripVert = (RwV3d**)pData;
    for (U32 i = 0; i < mesh->numIndices; i++) {
        **stripVert = sCurrVert[mesh->indices[i]];
        (*stripVert)++;
    }
    return mesh;
}

static RpAtomic* AddAtomicCB(RpAtomic* atomic, void* data)
{
    sAtomicStartVert[--sAtomicStartCount] = *(RwV3d**)data;
    sCurrVert = atomic->geometry->morphTarget->verts;
    _rpMeshHeaderForAllMeshes(atomic->geometry->mesh, AddMeshCB, data);
    return atomic;
}

static RpAtomic* AddAtomicPrecalcedVertCB(RpAtomic* atomic, void* data)
{
    sAtomicStartVert[--sAtomicStartCount] = *(RwV3d**)data;
    *(RwV3d**)data += atomic->geometry->mesh->totalIndicesInMesh;
    return atomic;
}

static RpAtomic* ListAtomicCB(RpAtomic* atomic, void* data)
{
    RpAtomic*** aList = (RpAtomic***)data;
    **aList = atomic;
    (*aList)++;
    return atomic;
}

U32 get_matid(xJSPHeader* jsp, U32 old_mat_id, U32 jspAssetID)
{
    char num[100];
    sprintf(num, "%d", old_mat_id);

    U32 matid = 0;
    if (jspAssetID) {
        matid = xStrHashCat(jspAssetID, num);
    } else {
        S32 count = ((xScene*)xglobals->sceneCur)->env->geom->jsp_count;
        xJSPHeader** jsplist = ((xScene*)xglobals->sceneCur)->env->geom->jsp_list;
        for (S32 i = 0; i < count; i++) {
            if (jsp == jsplist[i]) {
                const U32* aidlist = ((xScene*)xglobals->sceneCur)->env->geom->jsp_aid;
                xASSERT_L(327, aidlist[i]);
                matid = xStrHashCat(aidlist[i], num);
            }
        }
    }

    xASSERT_FMT_L(334, matid, "Material lookup failure: old mat ID: 0x%08X JSP: 0x%08X", old_mat_id, jsp);
    return matid;
}

void xJSP_MultiStreamRead(void* data, U32 size, xJSPHeader** jsp, U32 assetID)
{
    S32 i;
    RpClump* clump;
    xClumpCollBSPTree* colltree;
    S32 rwresult;
    xJSPHeaderEx* hdr;

    iAsyncLockRW();

    if (!*jsp) {
        hdr = (xJSPHeaderEx*)RwMallocL(sizeof(xJSPHeaderEx), rwMEMHINTDUR_NADURATION, 371);
        *jsp = hdr;
        xASSERT_L(373, *jsp);
        memset(hdr, 0, sizeof(xJSPHeaderEx));
    } else {
        hdr = (xJSPHeaderEx*)*jsp;
    }

    __rwMark mark;
    __rwMark* mp = (__rwMark*)data;
    U32 _size = sizeof(__rwMark);
    memmove(&mark, mp, _size);
    RwMemNative32(&mark, _size);

    iAsyncUnlockRW();

    if (mark.type == rwID_HI_BEEF01) {
        iAsyncLockRW();

        data = mp + 1;
        colltree = xClumpColl_StaticBufferInit(data, mark.length);

        size -= mark.length + sizeof(__rwMark);
        mp = (__rwMark*)((U8*)data + mark.length);
        memmove(&mark, mp, sizeof(__rwMark));
        RwMemNative32(&mark, sizeof(__rwMark));
        xASSERT_L(409, mark.type == rwID_HI_JSPINFO);

        data = mp + 1;
        xJSPHeader* tmphdr = (xJSPHeader*)data;
        xASSERT_L(412, (tmphdr->idtag[3] == 0) && !strcmp(tmphdr->idtag,"JSP"));
        xASSERT_L(413, (tmphdr->version == 3) || (tmphdr->version == 4) || (tmphdr->version == 5));

        strncpy(hdr->idtag, tmphdr->idtag, sizeof(hdr->idtag));
        hdr->version = tmphdr->version;
        hdr->jspNodeCount = tmphdr->jspNodeCount;
        hdr->colltree = colltree;

        iAsyncUnlockRW();

        if (tmphdr->version < 5) {
            iAsyncLockRW();

            hdr->jspNodeList = (xJSPNodeInfo*)((U8*)data + mark.length) - hdr->jspNodeCount;

            if (size >= sizeof(__rwMark)) {
                mp = (__rwMark*)((U8*)data + mark.length);
                memmove(&mark, mp, sizeof(__rwMark));
                RwMemNative32(&mark, sizeof(__rwMark));
                if (mark.type == rwID_HI_BEEF03) {
                    data = mp + 1;

                    hdr->stripVecCount = *(U32*)data;
                    data = (U32*)data + 1;
                    hdr->stripVecList = (RwV3d*)data;
                    sAtomicStartCount = RpClumpGetNumAtomics(hdr->clump);
                    sAtomicStartVert = (RwV3d**)RwMallocL(sAtomicStartCount * sizeof(RwV3d*), rwMEMHINTDUR_NADURATION, 446);

                    RwV3d* currVec = hdr->stripVecList;
                    RpClumpForAllAtomics(hdr->clump, AddAtomicPrecalcedVertCB, &currVec);
                }
            }

            if (hdr->stripVecCount == 0) {
                hdr->stripVecCount = 0;
                sAtomicStartCount = 0;
                RpClumpForAllAtomics(hdr->clump, CountAtomicCB, &hdr->stripVecCount);

                hdr->stripVecList = (RwV3d*)RwMallocL(hdr->stripVecCount * sizeof(RwV3d), rwMEMHINTDUR_NADURATION, 459);
                sAtomicStartVert = (RwV3d**)RwMallocL(sAtomicStartCount * sizeof(RwV3d*), rwMEMHINTDUR_NADURATION, 460);

                RwV3d* currVec = hdr->stripVecList;
                RpClumpForAllAtomics(hdr->clump, AddAtomicCB, &currVec);
            }

            for (i = 0; i < colltree->numTriangles; i++) {
                colltree->triangles[i].v.p = sAtomicStartVert[colltree->triangles[i].v.i.atomIndex] + colltree->triangles[i].v.i.meshVertIndex;
            }

            RwFree(sAtomicStartVert);
            hdr->nodetree = xJSPNodeTreeBuild(hdr);

            iAsyncUnlockRW();
        } else if (tmphdr->version == 5) {
            iAsyncLockRW();

            hdr->vertDataFlags = tmphdr->vertDataFlags;
            hdr->vertDataStride = tmphdr->vertDataStride;
            hdr->jspNodeList = (xJSPNodeInfo*)(tmphdr + 1);

            xJSPNodeTree* tmpNodeTree = (xJSPNodeTree*)(hdr->jspNodeList + hdr->jspNodeCount);
            hdr->nodetree = (xJSPNodeTree*)RwMallocL(sizeof(xJSPNodeTree), rwMEMHINTDUR_NADURATION, 490);
            *hdr->nodetree = *tmpNodeTree;
            hdr->nodetree->branchNodes = (xJSPNodeTreeBranch*)(tmpNodeTree + 1);
            hdr->nodetree->leafNodes = (xJSPNodeTreeLeaf*)(hdr->nodetree->branchNodes + hdr->nodetree->numBranchNodes);

            U32* stripvecdata = (U32*)(hdr->nodetree->leafNodes + hdr->nodetree->numLeafNodes);
            hdr->stripVecCount = *stripvecdata;
            hdr->stripVecList = (RwV3d*)(stripvecdata + 1);

            for (i = 0; i < colltree->numTriangles; i++) {
                colltree->triangles[i].v.p = &hdr->stripVecList[colltree->triangles[i].v.rawIdx];
            }

            iAsyncUnlockRW();
        }

        iAsyncLockRW();

        __rwMark* mp2 = (__rwMark*)((U8*)data + mark.length);
        __rwMark mark2;
        memmove(&mark2, mp2, sizeof(__rwMark));
        RwMemNative32(&mark2, sizeof(__rwMark));
        if (mark2.type == rwID_HI_BEEF04) {
            mp = mp2;
            mark = mark2;
            data = mp + 1;

            memcpy(&hdr->extraInfo, data, sizeof(xJSPExtraInfo));
            size -= mark.length + sizeof(__rwMark);
            data = (U8*)data + mark.length;
        }

        xASSERT_FMT_L(527, hdr->jspNodeList[0].originalMatIndex != 0xBAADF00D, "JSP %s is bad! Please Reexport!", xSceneID2Name((xScene*)xglobals->sceneCur, assetID));

        for (i = 0; i < colltree->numTriangles; i++) {
            colltree->triangles[i].matIndex = hdr->jspNodeList[colltree->triangles[i].matIndex].originalMatIndex;
        }

        xJSPNodeLightBuild(hdr);

        iAsyncUnlockRW();
    } else {
        iAsyncLockRW();

        RwMemory rwmem;
        rwmem.start = (RwUInt8*)data;
        rwmem.length = size;

        RwStream* stream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &rwmem);
        rwresult = RwStreamFindChunk(stream, rwID_CLUMP, NULL, NULL);
        xASSERT_L(553, rwresult);
        clump = RpClumpStreamRead(stream);
        RwStreamClose(stream, NULL);

        iAsyncUnlockRW();

        iAsyncLockRW();

        if (!hdr->clump) {
            hdr->clump = clump;
        } else {
            S32 i, atomCount = RpClumpGetNumAtomics(clump);
            if (atomCount) {
                RpAtomic** atomList = (RpAtomic**)RwMallocL(atomCount * sizeof(RpAtomic*), rwMEMHINTDUR_NADURATION, 568);
                RpAtomic** atomCurr = atomList;
                xASSERT_L(570, atomList);
                RpClumpForAllAtomics(clump, ListAtomicCB, &atomCurr);
                for (i = atomCount - 1; i >= 0; i--) {
                    RpClumpRemoveAtomic(clump, atomList[i]);
                    RpClumpAddAtomic(hdr->clump, atomList[i]);
                    RpAtomicSetFrame(atomList[i], RpClumpGetFrame(hdr->clump));
                }
                RwFree(atomList);
            }
            RpClumpDestroy(clump);
        }

        iAsyncUnlockRW();
    }
}

void xJSP_Destroy(xJSPHeader* jsp)
{
    xASSERT_L(588, jsp);

    S32 i;
    for (i = 0; i < ((xScene*)xglobals->sceneCur)->env->geom->jsp_count; i++) {
        if (((xScene*)xglobals->sceneCur)->env->geom->jsp_list[i] == jsp) {
            if (((xScene*)xglobals->sceneCur)->env->geom->world) {
                RpWorldDestroy(((xScene*)xglobals->sceneCur)->env->geom->world);
                ((xScene*)xglobals->sceneCur)->env->geom->world = NULL;
                break;
            }
        }
    }

    RwLLLink* cur = rwLinkListGetFirstLLLink(&jsp->clump->atomicList);
    RwLLLink* end = rwLinkListGetTerminator(&jsp->clump->atomicList);
    while (cur != end) {
        RpAtomic* apAtom = rwLLLinkGetData(cur, RpAtomic, inClumpLink);
        rwLinkListInitialize(&apAtom->llWorldSectorsInAtomic);
        cur = rwLLLinkGetNext(cur);
    }

    RpClumpDestroy(jsp->clump);
    RwFree(jsp->colltree);
    RwFree(jsp->nodelight);
    RwFree(jsp->nodetree);

    if (jsp->version < 5) {
        U32* tp = (U32*)jsp->stripVecList;
        U32 test = *(tp - 4);
        RwMemNative32(&test, sizeof(U32));
        if (test != rwID_HI_BEEF03) {
            RwFree(jsp->stripVecList);
        }
    }

    RwFree(jsp);
}

static void xJSPNodeLightBuild(xJSPHeader* jsp)
{
    xJSPNodeLight* nlight;
    RwLLLink* cur;
    RwLLLink* end;
    RpClump* clump;
    RpAtomic* apAtom;

    jsp->nodelight = (xJSPNodeLight*)RwMallocL(jsp->jspNodeCount * sizeof(xJSPNodeLight), rwMEMHINTDUR_NADURATION, 651);
    memset(jsp->nodelight, 0, jsp->jspNodeCount * sizeof(xJSPNodeLight));

    clump = jsp->clump; nlight = jsp->nodelight;
    cur = rwLinkListGetFirstLLLink(&clump->atomicList);
    end = rwLinkListGetTerminator(&clump->atomicList);
    while (cur != end) {
        apAtom = rwLLLinkGetData(cur, RpAtomic, inClumpLink);

        nlight->atomic = apAtom;
        nlight->dummyTie.worldSector = &nlight->dummySector;
        rwLLLinkInitialize(&nlight->dummyTie.lWorldSectorInAtomic);
        rwLinkListAddLLLink(&apAtom->llWorldSectorsInAtomic, &nlight->dummyTie.lWorldSectorInAtomic);
        rwLinkListInitialize(&nlight->dummySector.lightsInWorldSector);

        nlight++; cur = rwLLLinkGetNext(cur);
    }
}

static F32 GetSplitVolumeSum(xJSPNodeTreeLeaf* tleaf, S32 count, S32 axis, F32 coord)
{
    S32 i, numleft = 0, numright = 0;
    RwBBox leftbox, rightbox;

    for (i = 0; i < count; i++) {
        if (GETCOORD(tleaf[i].box.inf, axis) < coord) {
            if (numleft == 0) {
                leftbox = tleaf[i].box;
            } else {
                leftbox.inf.x = xmin(leftbox.inf.x, tleaf[i].box.inf.x);
                leftbox.inf.y = xmin(leftbox.inf.y, tleaf[i].box.inf.y);
                leftbox.inf.z = xmin(leftbox.inf.z, tleaf[i].box.inf.z);
                leftbox.sup.x = xmax(leftbox.sup.x, tleaf[i].box.sup.x);
                leftbox.sup.y = xmax(leftbox.sup.y, tleaf[i].box.sup.y);
                leftbox.sup.z = xmax(leftbox.sup.z, tleaf[i].box.sup.z);
            }
            numleft++;
        }

        if (GETCOORD(tleaf[i].box.sup, axis) > coord || coord == GETCOORD(tleaf[i].box.inf, axis)) {
            if (numright == 0) {
                rightbox = tleaf[i].box;
            } else {
                rightbox.inf.x = xmin(rightbox.inf.x, tleaf[i].box.inf.x);
                rightbox.inf.y = xmin(rightbox.inf.y, tleaf[i].box.inf.y);
                rightbox.inf.z = xmin(rightbox.inf.z, tleaf[i].box.inf.z);
                rightbox.sup.x = xmax(rightbox.sup.x, tleaf[i].box.sup.x);
                rightbox.sup.y = xmax(rightbox.sup.y, tleaf[i].box.sup.y);
                rightbox.sup.z = xmax(rightbox.sup.z, tleaf[i].box.sup.z);
            }
            numright++;
        }
    }

    if (numleft == count || numright == count) {
        return HUGE;
    }

    return (leftbox.sup.x - leftbox.inf.x) * (leftbox.sup.y - leftbox.inf.y) * (leftbox.sup.z - leftbox.inf.z) +
           (rightbox.sup.x - rightbox.inf.x) * (rightbox.sup.y - rightbox.inf.y) * (rightbox.sup.z - rightbox.inf.z);
}

static S32 ChooseSplit_MeanMinSum(xJSPNodeTreeLeaf* tleaf, S32 count, S32& bestaxis, F32& bestcoord)
{
    F32 bestvolume = HUGE;
    S32 i, axis;
    RwV3d mean = { 0.0f, 0.0f, 0.0f };
    F32 testarray[3];
    F32 meancoord, coord, testvol;

    for (i = 0; i < count; i++) {
        mean.x += tleaf[i].box.inf.x + tleaf[i].box.sup.x;
        mean.y += tleaf[i].box.inf.y + tleaf[i].box.sup.y;
        mean.z += tleaf[i].box.inf.z + tleaf[i].box.sup.z;
    }

    mean.x /= 2 * count;
    mean.y /= 2 * count;
    mean.z /= 2 * count;

    for (axis = 0; axis < sizeof(RwV3d); axis += sizeof(F32)) {
        meancoord = GETCOORD(mean, axis);
        testarray[1] = meancoord;
        testarray[0] = -HUGE;
        testarray[2] = HUGE;

        for (i = 0; i < count; i++) {
            coord = GETCOORD(tleaf[i].box.inf, axis);
            if (coord <= meancoord && coord > testarray[0]) {
                testarray[0] = coord;
            }
            if (coord < testarray[2] && coord >= meancoord) {
                testarray[2] = coord;
            }
        }

        for (i = 0; i < 3; i++) {
            testvol = GetSplitVolumeSum(tleaf, count, axis, testarray[i]);
            if (testvol < bestvolume) {
                bestaxis = axis;
                bestvolume = testvol;
                bestcoord = testarray[i];
            }
        }
    }

    return bestvolume != HUGE;
}

static TempSplit* sSplitList;
static TempSplit* sSplitCurr;
static xJSPNodeTreeLeaf* sSplitLeaf;

static void RecurseSplitJSPNode(xJSPNodeTreeLeaf* tleaf, S32 count, TempSplitChild* childptr)
{
    S32 i, bestaxis, lastright;
    F32 bestcoord;
    xJSPNodeTreeLeaf templeaf;
    TempSplit* thisSplit;

    if (count >= 2 && ChooseSplit_MeanMinSum(tleaf, count, bestaxis, bestcoord)) {
        lastright = count;
        for (i = 0; i < lastright; i++) {
            if (0.5f * (GETCOORD(tleaf[i].box.inf, bestaxis) + GETCOORD(tleaf[i].box.sup, bestaxis)) >= bestcoord) {
                lastright--;
                templeaf = tleaf[i];
                tleaf[i] = tleaf[lastright];
                tleaf[lastright] = templeaf;
                i--;
            }
        }

        if (lastright == count || lastright == 0) {
            if (childptr) {
                childptr->node = tleaf - sSplitLeaf;
                childptr->type = 1;
            }
            tleaf->leafCount = count;
        } else {
            sSplitCurr->axis = bestaxis;
            sSplitCurr->rightCount = count - lastright;
            sSplitCurr->leftCount = lastright;
            sSplitCurr->rightValue = HUGE;
            sSplitCurr->leftValue = -HUGE;

            for (i = 0; i < lastright; i++) {
                sSplitCurr->leftValue = xmax(sSplitCurr->leftValue, GETCOORD(tleaf[i].box.sup, bestaxis));
            }
            for (; i < count; i++) {
                sSplitCurr->rightValue = xmin(sSplitCurr->rightValue, GETCOORD(tleaf[i].box.inf, bestaxis));
            }

            if (childptr) {
                childptr->node = sSplitCurr - sSplitList;
                childptr->type = 2;
            }

            thisSplit = sSplitCurr;
            sSplitCurr++;
            RecurseSplitJSPNode(tleaf, lastright, &thisSplit->leftChild);
            RecurseSplitJSPNode(tleaf + lastright, count - lastright, &thisSplit->rightChild);
        }
    } else {
        if (childptr) {
            childptr->node = tleaf - sSplitLeaf;
            childptr->type = 1;
        }
        tleaf->leafCount = count;
    }
}

xJSPNodeTree* xJSPNodeTreeBuild(xJSPHeader* jsp)
{
    RpClump* clump = jsp->clump;
    S32 i, j;
    S32 atomCount = RpClumpGetNumAtomics(clump);
    if (!atomCount) {
        return NULL;
    }

    RpAtomic** atomList = (RpAtomic**)RwMallocL(atomCount * sizeof(RpAtomic*), rwMEMHINTDUR_NADURATION, 914);
    RpAtomic** atomCurr = atomList;
    xASSERT_L(916, atomList);
    RpClumpForAllAtomics(clump, ListAtomicCB, &atomCurr);

    xJSPNodeTreeLeaf* tleaf = (xJSPNodeTreeLeaf*)RwMallocL(atomCount * sizeof(xJSPNodeTreeLeaf), rwMEMHINTDUR_NADURATION, 920);
    atomCurr = atomList;
    for (i = 0; i < atomCount; i++) {
        RpGeometry* geom = (*atomCurr)->geometry;
        S32 numVert = geom->numVertices;
        RwV3d* vert = geom->morphTarget->verts;

        tleaf[i].nodeIndex = i;
        tleaf[i].box.inf.x = HUGE;
        tleaf[i].box.inf.y = HUGE;
        tleaf[i].box.inf.z = HUGE;
        tleaf[i].box.sup.x = -HUGE;
        tleaf[i].box.sup.y = -HUGE;
        tleaf[i].box.sup.z = -HUGE;

        for (j = 0; j < numVert; j++) {
            tleaf[i].box.inf.x = xmin(tleaf[i].box.inf.x, vert[j].x);
            tleaf[i].box.inf.y = xmin(tleaf[i].box.inf.y, vert[j].y);
            tleaf[i].box.inf.z = xmin(tleaf[i].box.inf.z, vert[j].z);
            tleaf[i].box.sup.x = xmax(tleaf[i].box.sup.x, vert[j].x);
            tleaf[i].box.sup.y = xmax(tleaf[i].box.sup.y, vert[j].y);
            tleaf[i].box.sup.z = xmax(tleaf[i].box.sup.z, vert[j].z);
        }

        atomCurr++;
    }

    sSplitList = (TempSplit*)RwMallocL(atomCount * sizeof(TempSplit), rwMEMHINTDUR_NADURATION, 946);
    sSplitLeaf = tleaf;
    sSplitCurr = sSplitList;
    RecurseSplitJSPNode(tleaf, atomCount, NULL);

    xJSPNodeTree* jntree = (xJSPNodeTree*)RwMallocL(sizeof(xJSPNodeTree) + atomCount * sizeof(xJSPNodeTreeLeaf) + (sSplitCurr - sSplitList) * sizeof(xJSPNodeTreeBranch), rwMEMHINTDUR_NADURATION, 959);
    jntree->numBranchNodes = sSplitCurr - sSplitList;
    jntree->branchNodes = (xJSPNodeTreeBranch*)(jntree + 1);
    jntree->numLeafNodes = atomCount;
    jntree->leafNodes = (xJSPNodeTreeLeaf*)(jntree->branchNodes + jntree->numBranchNodes);
    memcpy(jntree->leafNodes, tleaf, atomCount * sizeof(xJSPNodeTreeLeaf));

    for (i = 0; i < jntree->numBranchNodes; i++) {
        jntree->branchNodes[i].leftNode = sSplitList[i].leftChild.node;
        jntree->branchNodes[i].leftType = sSplitList[i].leftChild.type;
        jntree->branchNodes[i].rightNode = sSplitList[i].rightChild.node;
        jntree->branchNodes[i].rightType = sSplitList[i].rightChild.type;
        jntree->branchNodes[i].coord = sSplitList[i].axis;
        jntree->branchNodes[i].leftValue = sSplitList[i].leftValue;
        jntree->branchNodes[i].rightValue = sSplitList[i].rightValue;
    }

    RwFree(atomList);
    RwFree(tleaf);
    RwFree(sSplitList);
    sSplitList = NULL;

    return jntree;
}

xVec3* xJSP_GetTriangleVertData(xJSPHeader* jsp, xClumpCollBSPTriangle* tri, xColor* color, xVec3* normal, F32* uv)
{
    xASSERT_ONCE((tri >= jsp->colltree->triangles) && (tri < jsp->colltree->triangles+jsp->colltree->numTriangles));
    xASSERT_ONCE((tri->v.p >= jsp->stripVecList) && (tri->v.p < jsp->stripVecList+jsp->stripVecCount));

    if (jsp->version < 5) {
        return (xVec3*)tri->v.p;
    }

    S32 vertDataStride = jsp->vertDataStride;
    S32 index = tri->v.p - jsp->stripVecList;
    S8* dataStart = (S8*)(jsp->stripVecList + jsp->stripVecCount);
    S32 vertDataFlags = jsp->vertDataFlags;
    dataStart += index * vertDataStride;

    if (vertDataFlags & 0x1) {
        if (color) {
            color[0] = *(xColor*)dataStart;
            color[1] = *(xColor*)(dataStart + vertDataStride);
            color[2] = *(xColor*)(dataStart + vertDataStride * 2);
        }
        dataStart += 4;
    }

    if (vertDataFlags & 0x2) {
        if (normal) {
            normal[0].x = (1.0f / 127.0f) * dataStart[0];
            normal[0].y = (1.0f / 127.0f) * dataStart[1];
            normal[0].z = (1.0f / 127.0f) * dataStart[2];
            normal[1].x = (1.0f / 127.0f) * dataStart[vertDataStride];
            normal[1].y = (1.0f / 127.0f) * dataStart[vertDataStride + 1];
            normal[1].z = (1.0f / 127.0f) * dataStart[vertDataStride + 2];
            normal[2].x = (1.0f / 127.0f) * dataStart[vertDataStride * 2];
            normal[2].y = (1.0f / 127.0f) * dataStart[vertDataStride * 2 + 1];
            normal[2].z = (1.0f / 127.0f) * dataStart[vertDataStride * 2 + 2];
        }
        dataStart += 4;
    }

    if (vertDataFlags & 0x4) {
        if (uv) {
            uv[0] = (1.0f / 4096.0f) * ((S16*)dataStart)[0];
            uv[1] = (1.0f / 4096.0f) * ((S16*)dataStart)[1];
            uv[2] = (1.0f / 4096.0f) * ((S16*)(dataStart + vertDataStride))[0];
            uv[3] = (1.0f / 4096.0f) * ((S16*)(dataStart + vertDataStride))[1];
            uv[4] = (1.0f / 4096.0f) * ((S16*)(dataStart + vertDataStride * 2))[0];
            uv[5] = (1.0f / 4096.0f) * ((S16*)(dataStart + vertDataStride * 2))[1];
        }
        dataStart += 4;
    }

    if (vertDataFlags & 0x8) {
        if (uv) {
            uv[0] = (1.0f / 4096.0f) * ((F32*)dataStart)[0];
            uv[1] = (1.0f / 4096.0f) * ((F32*)dataStart)[1];
            uv[2] = (1.0f / 4096.0f) * ((F32*)(dataStart + vertDataStride))[0];
            uv[3] = (1.0f / 4096.0f) * ((F32*)(dataStart + vertDataStride))[1];
            uv[4] = (1.0f / 4096.0f) * ((F32*)(dataStart + vertDataStride * 2))[0];
            uv[5] = (1.0f / 4096.0f) * ((F32*)(dataStart + vertDataStride * 2))[1];
        }
    }

    return (xVec3*)tri->v.p;
}

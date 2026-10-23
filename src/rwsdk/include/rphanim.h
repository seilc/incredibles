#ifndef RPHANIM_H
#define RPHANIM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rwcore.h"
#include "rpworld.h"
#include "rtquat.h"
#include "rtanim.h"

typedef struct RpHAnimHierarchy RpHAnimHierarchy;
typedef struct RpHAnimNodeInfo RpHAnimNodeInfo;

struct RpHAnimNodeInfo
{
    RwInt32 nodeID;
    RwInt32 nodeIndex;
    RwInt32 flags;
    RwFrame* pFrame;
};

struct RpHAnimHierarchy
{
    RwInt32 flags;
    RwInt32 numNodes;
    RwMatrix* pMatrixArray;
    void* pMatrixArrayUnaligned;
    RpHAnimNodeInfo* pNodeInfo;
    RwFrame* parentFrame;
    RpHAnimHierarchy* parentHierarchy;
    RwInt32 rootParentOffset;
    RtAnimInterpolator* currentAnim;
};

#endif

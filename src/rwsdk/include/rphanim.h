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
typedef struct RpHAnimKeyFrame RpHAnimKeyFrame;
typedef struct RpHAnimNodeInfo RpHAnimNodeInfo;

struct RpHAnimKeyFrame
{
    RpHAnimKeyFrame* prevFrame;
    RwReal time;
    RtQuat q;
    RwV3d t;
};

struct RpHAnimNodeInfo
{
    RwInt32 nodeID;
    RwInt32 nodeIndex;
    RwInt32 flags;
    RwFrame* pFrame;
};

enum RpHAnimHierarchyFlag
{
    rpHANIMHIERARCHYSUBHIERARCHY = 0x1,
    rpHANIMHIERARCHYNOMATRICES = 0x2,
    rpHANIMHIERARCHYUPDATEMODELLINGMATRICES = 0x1000,
    rpHANIMHIERARCHYUPDATELTMS = 0x2000,
    rpHANIMHIERARCHYLOCALSPACEMATRICES = 0x4000,
    rpHANIMHIERARCHYFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpHAnimHierarchyFlag RpHAnimHierarchyFlag;

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

#define rpHANIMPOPPARENTMATRIX 0x01
#define rpHANIMPUSHPARENTMATRIX 0x02
#define rpHANIMSTDKEYFRAMESIZE sizeof(RpHAnimKeyFrame)

#ifdef __cplusplus
extern "C" {
#endif

RpHAnimHierarchy* RpHAnimFrameGetHierarchy(RwFrame* frame);
RwBool RpHAnimFrameSetHierarchy(RwFrame* frame, RpHAnimHierarchy* hierarchy);
RpHAnimHierarchy* RpHAnimHierarchyCreate(RwInt32 numNodes, RwUInt32* nodeFlags, RwInt32* nodeIDs, RpHAnimHierarchyFlag flags, RwInt32 maxInterpKeyFrameSize);
RwBool RpHAnimPluginAttach(void);

#ifdef __cplusplus
}
#endif

#endif

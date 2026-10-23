#ifndef RTANIM_H
#define RTANIM_H

#include "rwplcore.h"

typedef struct RtAnimAnimation RtAnimAnimation;
typedef struct RtAnimInterpolator RtAnimInterpolator;
typedef struct RtAnimInterpolatorInfo RtAnimInterpolatorInfo;

struct RtAnimInterpolatorInfo
{
    RwInt32 typeID;
    RwInt32 interpKeyFrameSize;
    RwInt32 animKeyFrameSize;
    void (*keyFrameApplyCB)(void*, void*);
    void (*keyFrameBlendCB)(void*, void*, void*, RwReal);
    void (*keyFrameInterpolateCB)(void*, void*, void*, RwReal, void*);
    void (*keyFrameAddCB)(void*, void*, void*);
    void (*keyFrameMulRecipCB)(void*, void*);
    RtAnimAnimation* (*keyFrameStreamReadCB)(RwStream*, RtAnimAnimation*);
    RwInt32 (*keyFrameStreamWriteCB)(RtAnimAnimation*, RwStream*);
    RwInt32 (*keyFrameStreamGetSizeCB)(RtAnimAnimation*);
    RwInt32 customDataSize;
};

struct RtAnimAnimation
{
    RtAnimInterpolatorInfo* interpInfo;
    RwInt32 numFrames;
    RwInt32 flags;
    RwReal duration;
    void* pFrames;
    void* customData;
};

struct RtAnimInterpolator
{
    RtAnimAnimation* pCurrentAnim;
    RwReal currentTime;
    void* pNextFrame;
    RtAnimInterpolator* (*pAnimCallBack)(RtAnimInterpolator*, void*);
    void* pAnimCallBackData;
    RwReal animCallBackTime;
    RtAnimInterpolator* (*pAnimLoopCallBack)(RtAnimInterpolator*, void*);
    void* pAnimLoopCallBackData;
    RwInt32 maxInterpKeyFrameSize;
    RwInt32 currentInterpKeyFrameSize;
    RwInt32 currentAnimKeyFrameSize;
    RwInt32 numNodes;
    RwBool isSubInterpolator;
    RwInt32 offsetInParent;
    RtAnimInterpolator* parentAnimation;
    void (*keyFrameApplyCB)(void*, void*);
    void (*keyFrameBlendCB)(void*, void*, void*, RwReal);
    void (*keyFrameInterpolateCB)(void*, void*, void*, RwReal, void*);
    void (*keyFrameAddCB)(void*, void*, void*);
};

#endif

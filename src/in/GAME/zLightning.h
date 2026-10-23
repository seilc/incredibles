#ifndef ZLIGHTNING_H
#define ZLIGHTNING_H

#include "xMath3.h"
#include "xColor.h"
#include "iSnd.h"
#include "xEnt.h"

struct zLightningParticle
{
    xVec3 pos;
    xVec3 vel;
    xColor col;
    F32 life;
    F32 w;
    F32 h;
    U8 isActive;
    U8 pad[48];
};

struct zLightning
{
    U32 type;
    U32 flags;
    struct
    {
        xVec3 endPoint[2];
        xVec3 endPointB;
        xVec3 direction;
        F32 length;
        F32 scale;
        F32 width;
        F32 endParam[2];
        F32 endVel[2];
        F32 paramSpan[2];
    } func;
    U32 numStrips;
    F32 alphaRamp;
    xColor color;
    F32 time_left;
    F32 time_total;
    zLightning* nextBranch;
    zLightning* prevBranch;
    F32 parentSegment;
    F32 lastParentSegment;
    F32 segmentsPerMeter;
    F32 branchSpeed;
    S32 damage;
    F32 knockBackSpeed;
    RwRaster* mainTexture;
    RwRaster* branchTexture;
    zLightningParticle* particle;
    S32 nextParticle;
    F32 sparkTimer;
    xVec3 collisionPoint;
    F32 genTime;
    U8 randomizeBranchEndPoint;
    U8 collisionEnabled;
    iSndHandle sndHandle;
    xEnt* followStart;
    xVec3 followStartOffset;
    xEnt* followEnd;
    xVec3 followEndOffset;
    void (*renderCB)(zLightning*, RwIm3DVertex*, RwIm3DVertex*, U32);
    void* context;
    F32 weightParam[12];
};

#endif
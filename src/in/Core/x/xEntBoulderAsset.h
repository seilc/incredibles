#ifndef XENTBOULDERASSET_H
#define XENTBOULDERASSET_H

#include "xDynAsset.h"
#include "xSndMgr.h"
#include "xVec3.h"

struct xEntBoulderAsset
{
    F32 gravity;
    F32 mass;
    F32 bounce;
    F32 friction;
    F32 maxVel;
    F32 maxAngVel;
    F32 stickiness;
    F32 bounceDamp;
    U32 flags;
    F32 killtimer;
    U32 hitpoints;
    union
    {
        U32 uSoundGroupHash;
        iSndGroupHandle soundID;
    };
    F32 minSoundVel;
    F32 maxSoundVel;
    F32 fSphereRadius;
    U8 uPad0;
    U8 uPad1;
    U8 uPad2;
    U8 uBoneIndex;
};

struct xBoulderGeneratorAsset : xDynAsset
{
    U32 object;
    xVec3 offset;
    F32 offsetRand;
    xVec3 initvel;
    F32 velAngleRand;
    F32 velMagRand;
    xVec3 rotation;
    U32 flags;
};

#endif

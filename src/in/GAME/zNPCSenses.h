#ifndef ZNPCSENSES_H
#define ZNPCSENSES_H

#include "xEnt.h"
#include "xMath2.h"

namespace zNPC
{
    struct common;

    struct Senses
    {
        xVec3 eyePos;
        xEnt* targetEnt;
        xVec3 targetPos;
        xVec3 targetDir;
        F32 targetDist;
        xVec2 targetDir2d;
        F32 targetDist2d;
        xVec3 aimPos;
        xVec3 aimDir;
        F32 aimDist;
        F32 timeSinceUpdate;
        F32 nextLOSCheckTime;
        bool unk5C;
        bool canSee;
        F32 viewDist;
        F32 viewAngle;
        F32 cosViewAngle;
        U8 checkLOS;
        U8 updateVarsAlways;
        F32 checkLOSMinDelay;
        F32 checkLOSMaxDelay;
        F32 memoryDuration;
        S32 eyeBone;
        F32 npcAlertResponseRange;

        void Setup(common* owner);
        void Reset(common* owner);
        void AlertTargetPos(common* owner, const xVec3& newTargetPos);
        void Update(common* owner, F32 dt);
    };
} // namespace zNPC

#endif
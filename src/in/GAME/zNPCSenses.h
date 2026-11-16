#ifndef ZNPCSENSES_H
#define ZNPCSENSES_H

#include "xVec3.h"
#include "xMath2.h"

struct xEnt;

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
        bool canSee;
        bool canDetect;
        F32 viewDist;
        F32 viewAngle;
        F32 cosViewAngle;
        bool checkLOS;
        bool updateVarsAlways;
        F32 checkLOSMinDelay;
        F32 checkLOSMaxDelay;
        F32 memoryDuration;
        S32 eyeBone;
        F32 npcAlertResponseRange;

        void Setup(common* owner);
        void Reset(common* owner);
        bool HasTarget() const;
        bool CanSeeTarget() const;
        void AlertTargetPos(common* owner, const xVec3& newTargetPos);
        void Update(common* owner, F32 dt);
        void UpdateEyePos(common* owner);
        bool CheckLOS(common* owner);
        void UpdateTargetVars(common* owner);
        bool WorldRayLOS(const xVec3& start, const xVec3& dir, F32 dist) const;
    };
} // namespace zNPC

#endif

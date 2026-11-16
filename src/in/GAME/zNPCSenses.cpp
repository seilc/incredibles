#include "xCommon.h"
#include "zNPCSenses.h"

#include "zNPCCommon.h"
#include "zPlayer.h"
#include "zTurret.h"
#include "zGlobals.h"
#include "zMain.h"
#include "zBase.h"
#include "xScene.h"
#include "xCollide.h"
#include "xRand.h"
#include "iTime.h"

namespace zNPC
{
    void Senses::Setup(common* owner)
    {
        owner->get_parameter("LOSUpdateVarsAlways", &updateVarsAlways, true);
        owner->get_parameter("checkLOS", &checkLOS, true);
        owner->get_parameter("checkLOSMinDelay", &checkLOSMinDelay, 0.2f);
        owner->get_parameter("checkLOSMaxDelay", &checkLOSMaxDelay, 0.5f);
        owner->get_parameter("LOSDistance", &viewDist, 30.0f);
        owner->get_parameter("LOSAngle", &viewAngle, 45.0f);
        viewAngle = (PI / 180.0f) * viewAngle;
        cosViewAngle = icos(viewAngle);
        owner->get_parameter("eyeBone", &eyeBone, -1);
        owner->get_parameter("memoryDuration", &memoryDuration, HUGE);
        owner->get_parameter("npcAlertResponseRange", &npcAlertResponseRange, 50.0f);
        Reset(owner);
    }

    void Senses::Reset(common* owner)
    {
        canDetect = true;
        targetEnt = ZPLAYER;
        targetDist = HUGE;
        targetDist2d = HUGE;
        aimDist = HUGE;
        timeSinceUpdate = HUGE;
        nextLOSCheckTime = iTimeGetGame();

        if (updateVarsAlways) {
            UpdateEyePos(owner);

            if (targetEnt == ZPLAYER) {
                targetPos = ZPLAYER->get_target();
            } else {
                targetPos = *xEntGetCenter(targetEnt);
            }

            targetDir = targetPos - *xEntGetPos(owner);
            targetDir2d.assign(targetDir.x, targetDir.z);
            targetDist = targetDir.safe_normalize(owner->frame->mat.at);
            targetDist2d = targetDir2d.length();

            xVec2 safety = { owner->frame->mat.at.x, owner->frame->mat.at.z };
            targetDir2d.safe_normalize(safety);

            aimPos = targetPos;
            aimDir = aimPos - eyePos;
            aimDist = aimDir.safe_normalize(owner->frame->mat.at);
        }
    }

    bool Senses::HasTarget() const
    {
        return timeSinceUpdate < memoryDuration;
    }

    bool Senses::CanSeeTarget() const
    {
        return canSee;
    }

    void Senses::AlertTargetPos(common* owner, const xVec3& newTargetPos)
    {
        if (newTargetPos.distance(*xEntGetPos(owner)) > npcAlertResponseRange) {
            return;
        }

        targetPos = newTargetPos;
        timeSinceUpdate = 0.0f;
        UpdateEyePos(owner);
        UpdateTargetVars(owner);
    }

    void Senses::Update(common* owner, F32 dt)
    {
        timeSinceUpdate += dt;
        UpdateEyePos(owner);

        if (!CheckLOS(owner)) {
            UpdateTargetVars(owner);
        }
    }

    void Senses::UpdateEyePos(common* owner)
    {
        if (eyeBone >= 0) {
            eyePos = owner->get_bone_world_position(eyeBone);
        } else {
            eyePos = *xEntGetPos(owner);
            eyePos.y += 1.0f;
        }
    }

    bool Senses::CheckLOS(common* owner)
    {
        F32 time = iTimeGetGame();
        bool lastCanSee = canSee;
        canSee = true;

        if ((targetEnt->baseType == eBaseTypePlayer && !((zPlayer*)targetEnt)->IsVisible()) || !canDetect) {
            nextLOSCheckTime = time;
            canSee = false;

            if (!updateVarsAlways) {
                return false;
            }
        }

        xVec3 newTargetPos;
        if (targetEnt == ZPLAYER) {
            newTargetPos = ZPLAYER->get_target();
        } else {
            newTargetPos = *xEntGetCenter(targetEnt);
        }

        xVec3 newTargetDir = newTargetPos - *xEntGetPos(owner);
        xVec2 newTargetDir2d;
        newTargetDir2d.assign(newTargetDir.x, newTargetDir.z);
        F32 newTargetDist = newTargetDir.safe_normalize(owner->frame->mat.at);

        if (newTargetDist > viewDist) {
            nextLOSCheckTime = time;
            canSee = false;

            if (!updateVarsAlways) {
                return false;
            }
        }

        F32 newTargetDist2d = newTargetDir2d.length();
        if (newTargetDist2d > 0.001f) {
            newTargetDir2d.normalize();

            xVec2 facing;
            facing.assign(owner->frame->mat.at.x, owner->frame->mat.at.z);

            F32 len2 = facing.length2();
            if (len2 > 0.001f) {
                facing /= xsqrt(len2);

                if (newTargetDir2d.dot(facing) < cosViewAngle) {
                    nextLOSCheckTime = time;
                    canSee = false;

                    if (!updateVarsAlways) {
                        return false;
                    }
                }
            }
        } else {
            newTargetDir2d.assign(owner->frame->mat.at.x, owner->frame->mat.at.z);
            newTargetDir2d.normalize();
        }

        xVec3 newAimPos = newTargetPos;
        xVec3 newAimDir = newAimPos - eyePos;
        F32 newAimDist = newAimDir.safe_normalize(owner->frame->mat.at);

        if (checkLOS && canSee) {
            if (time < nextLOSCheckTime) {
                canSee = lastCanSee;

                if (!canSee && !updateVarsAlways) {
                    return false;
                }
            } else {
                nextLOSCheckTime = time + xrand_RandomFloatRange(checkLOSMinDelay, checkLOSMaxDelay);
                canSee = WorldRayLOS(eyePos, newAimDir, newAimDist);

                if (canSee) {
                    timeSinceUpdate = 0.0f;
                } else if (!updateVarsAlways) {
                    return false;
                }
            }
        } else if (canSee) {
            timeSinceUpdate = 0.0f;
        }

        targetPos = newTargetPos;
        targetDir = newTargetDir;
        targetDist = newTargetDist;
        targetDir2d = newTargetDir2d;
        targetDist2d = newTargetDist2d;
        aimPos = newAimPos;
        aimDir = newAimDir;
        aimDist = newAimDist;
        return true;
    }

    void Senses::UpdateTargetVars(common* owner)
    {
        targetDir = targetPos - *xEntGetPos(owner);
        targetDist = targetDir.safe_normalize(owner->frame->mat.at);
        eyePos = owner->get_bone_world_position(eyeBone);
        aimPos = targetPos;
        aimDir = aimPos - eyePos;
        aimDist = aimDir.safe_normalize(owner->frame->mat.at);
    }

    bool Senses::WorldRayLOS(const xVec3& start, const xVec3& dir, F32 dist) const
    {
        xRay3 ray;
        ray.origin = start;
        ray.dir = dir;
        ray.min_t = 0.0f;
        ray.max_t = dist;
        ray.flags = XRAY3_USE_MIN | XRAY3_USE_MAX;

        xCollis collisionRecord;
        collisionRecord.flags = 0;
        collisionRecord.dist = HUGE;
        collisionRecord.optr = NULL;
        collisionRecord.mptr = NULL;

        xRayHitsSceneFlags(globals.sceneCur, &ray, &collisionRecord, k_XENT_COLLTYPE_NPC,
                           k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_ENV);

        if (targetEnt == ZPLAYER) {
            if (ZPLAYER->GetTurret() && (collisionRecord.optr == ZPLAYER->GetTurret()->baseEnt ||
                                         collisionRecord.optr == ZPLAYER->GetTurret()->gunEnt)) {
                return true;
            }

            xEnt* grabbed = ZPLAYER->GetCarriedEntity();
            if (grabbed && collisionRecord.optr == grabbed) {
                return true;
            }
        }

        return (collisionRecord.flags & k_HIT_IT) == 0;
    }
} // namespace zNPC

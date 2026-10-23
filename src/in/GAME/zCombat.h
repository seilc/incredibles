#ifndef ZCOMBAT_H
#define ZCOMBAT_H

#include "xBase.h"
#include "xMath3.h"
#include "zHitParameters.h"
#include "xHierarchyBound.h"
#include "zShrapnel.h"
#include "zAnimCache.h"
#include "zCombatEffects.h"
#include "xEnv.h"

struct zAttackTableState
{
    struct hitBoneInfo
    {
        U16 bone;
        xVec3 boneOffset;
        S16 atomic;
    };

    struct effectBone
    {
        U16 bone;
        xVec3* pos;
    };

    U32 state;
    F32 moveDistanceZ;
    F32 moveDistanceY;
    F32 moveTime;
    F32 attackStart;
    F32 attackEnd;
    F32 attackRadius;
    hitBoneInfo hitBone[4];
    S16 damage;
    U16 source;
    U16 effect;
    U16 hitEffect;
    F32 effectStart;
    F32 effectEnd;
    effectBone effectBoneOutside[2];
    effectBone effectBoneInside[2];
    zAnimCacheEntry* bonePositions[2];
    F32 rumbleStartTime;
    U32 rumbleEmitterID;
    U32 shrapID;
    zShrapnelAsset* shrapAsset;
    F32 shrapStartTime;
    F32 velocityUp;
    F32 velocityAway;
    U32 flags;
    F32 holdTime;
    F32 jumpBreakTime;
    F32 crouchBreakTime;
    F32 turnLockStart;
    F32 turnLockStop;
    F32 climaxTime;
    xVec3 climaxOffset;
    F32 drainRate;
    F32 blurStart;
    F32 blurEnd;
    F32 blurLife;
    F32 blurAlpha;
    F32 blurFadeInTime;
    F32 blurFadeOutTime;
    S16 flashAlpha;
    F32 flashTime;
    F32 comboBonus;
    S16 comboType;
    S16 powerBonus;
};

struct zAttackStateRunTimeData
{
    struct ProcEffectData
    {
        xVec3 oldPos;
        xVec3 oldVel;
        xVec3 currPos;
        xVec3 currVel;
    };

    ProcEffectData bonePosVel[2];
};

struct zCombatDamageInfo
{
    S32 flags;
    xBase* from;
    S32 damage;
    zHitSource source;
    zHitTarget target;
    xVec3 knockback;
    xVec3 hitLocation;
    xVec3 direction;

    zCombatDamageInfo()
    {
        flags = 0;
    }
};

struct zCombat
{
    struct sphereInfo
    {
        xSphere sphere;
        xSphere previousSphere;
    };

    enum MoveState
    {
        MS_NONE,
        MS_HOLD,
        MS_SETVEL
    };

    xHierarchyBound bounds;
    U32 lastBoundUpdateTime;
    S16 currentHitPoints;
    S16 maximumHitPoints;
    U16 stateTableSize;
    zAttackTableState* stateTable;
    xAnimState* animationState;
    zAttackTableState* runningAttack;
    F32 runningAttackTimer;
    zAttackStateRunTimeData runningAttackData;
    xEnt* lastNPCDamaged;
    U8 disableMovement;
    U8 runningEffect;
    U8 runningBlur;
    U8 hitting;
    U8 hitEnv;
    U8 hitObjectCount;
    xEnt* hitObjects[24];
    S16 currentDamage;
    S16 lastHitDamage;
    zHitSource lastHitSource;
    zHitTarget lastHitTarget;
    sphereInfo location[4];
    S32 (*envCollisionCB)(xEnt*, xSweptSphere*, xEnv*);
    zCombatEffects::PunchEffect punchEffect;
    U32 effectParam;
    MoveState moveState;
    xVec3 moveStateVelocity;

    void RenderEffectModel(xEnt*);
};

void zCombatDamage(xEnt* ent, const zCombatDamageInfo& damageInfo);
zCombat* zCombatGetFrom(xEnt* ent);

#endif
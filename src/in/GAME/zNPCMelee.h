#ifndef ZNPCMELEE_H
#define ZNPCMELEE_H

#include "zNPCCommon.h"
#include "zNPCBehavior.h"

struct zShrapnelAsset;
struct xFXRibbon;

enum DeathType
{
    eDeathType_Fade,
    eDeathType_Explode
};

namespace zNPC
{
    struct human_throwable : common
    {
        human_throwable() {}
        virtual ~human_throwable() {}
        virtual void scene_setup();
        virtual U32 get_throw_anim_code();
        virtual void culling_changed();
    };

    struct projectile : move
    {
        S32 flags;
        S32 damageToDo;
        F32 default_bound_offset;
        xVec3 initial_velocity;
        F32 bilinear[2];
        xVec3 vel;
        xVec3 centerPos;
        xVec3 centerVel;
        xVec3 rotVec;
        F32 angVel;
        F32 koTimer;
        F32 koTime;
        xVec3 blendFromPos;
        xVec3 blendToPos;
        xQuat blendFromOri;
        xQuat blendToOri;
        F32 blendFactor;
        zShrapnelAsset* deathFrag;
        zShrapnelAsset* hitFrag;
        xFXRibbon* ribbon;
        U32 waistBone;
        DeathType deathType;
        xMat4x3 rootToDraw;

        virtual bool system_event(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                                  xBase* toParamWidget, U32 toParamWidgetID);
        virtual bool damage(zCombatDamageInfo& damageInfo);
        virtual void add_states(xAnimTable* table);
        virtual void add_transitions(xAnimTable* table);
        virtual void exit_state();
        virtual void update(F32 dt);
        virtual void setup();
        virtual void reset();
        virtual bool runnable(F32 dt);
        virtual bool grabbable(GrabType grabType);
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
    };
} // namespace zNPC

#endif

#ifndef ZNPCFLYSHOOTER_H
#define ZNPCFLYSHOOTER_H

#include "zNPCCommon.h"
#include "zNPCBehavior.h"
#include "zNPCBehaviorFX.h"
#include "zNPCFriendly.h"
#include "zNPCLobber.h"
#include "zNPCMelee.h"
#include "zNPCShooter.h"
#include "zNPCVelocipod.h"

namespace zNPC
{
    struct fly_forward : behavior_implementation<common>
    {
        follow_movepoints& follow_movepoints_state;

        fly_forward(follow_movepoints& follow) : follow_movepoints_state(follow)
        {
        }

        virtual const char* getName() { return "fly_foward"; }
        virtual void add_states(xAnimTable* table);
        virtual bool runnable(F32 dt);
        virtual bool grabbable(GrabType grabType);
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
    };

    struct rocket_lob : human_lob
    {
        F32 rocket_turn_speed;
        F32 rocket_speed;

        virtual void setup();
        virtual bool grabbable(GrabType grabType);
        virtual void launch();
    };

    struct flying_idle : idle
    {
        virtual bool grabbable(GrabType grabType) { return grabType != eGrabTypeMrIGroundToss; }
    };

    struct flying_shooter : flying_npc_generic<human_throwable>
    {
        fly_forward fly_foward_state;
        follow_movepoints follow_movepoints_state;
        projectile projectile_state;
        wait_shoot shoot_state;
        bone_container<laser_bone> laser_bones;
        hover_bob bob_state;
        flying_idle idle_state;
        bone_container<blade_bone> blade_bones;
        F32 old_speed;

        flying_shooter();
        virtual void update_npc(F32 dt);
        virtual void setup();
        virtual const char* get_type_name() const { return "human|flying_shooter"; }
        virtual void reset();
    };

    struct flying_rocket : flying_npc_generic<human_throwable>
    {
        fly_forward fly_foward_state;
        follow_movepoints follow_movepoints_state;
        projectile projectile_state;
        rocket_lob rocket_lob_state;
        flying_idle idle_state;
        hover_bob bob_state;
        bone_container<blade_bone> blade_bones;
        F32 old_speed;

        flying_rocket();
        virtual void update_npc(F32 dt);
        virtual void setup();
        virtual const char* get_type_name() const { return "human|flying_rocket"; }
        virtual void reset();
    };

    struct flying_lobber : flying_npc_generic<human_throwable>
    {
        fly_forward fly_foward_state;
        follow_movepoints follow_movepoints_state;
        projectile projectile_state;
        human_lob lob_state;
        flying_idle idle_state;
        hover_bob bob_state;
        bone_container<blade_bone> blade_bones;
        F32 old_speed;

        flying_lobber();
        virtual void update_npc(F32 dt);
        virtual void setup();
        virtual const char* get_type_name() const { return "human|flying_lobber"; }
        virtual void reset();
        virtual U8 get_target_priority() const { return 11; }
    };
} // namespace zNPC

#endif

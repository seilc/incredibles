#ifndef ZNPCSHOOTER_H
#define ZNPCSHOOTER_H

#include "zNPCCommon.h"
#include "zNPCBehavior.h"
#include "zNPCBehaviorFX.h"

namespace zNPC
{
    struct strafe;
    struct stuck;
    struct on_edge;

    struct shoot : move
    {
        U8 has_fired;
        U8 running;
        xVec3 aim_dir;
        strafe* strafe_state;
        group_attack attacker;
        S32 shots_in_group;
        S32 shots_left;
        F32 current_wait_time;
        F32 wait_time;
        F32 attack_radius;
        F32 too_close_radius;
        bone_container<laser_bone>& laser_bones;
        F32 first_shot_timer;
        F32 max_first_shot_timer;
        F32 target_aim;
        F32 aim_speed;
        const stuck* stuck_state;
        const on_edge* edge_state;

        shoot(bone_container<laser_bone>& bones, const stuck* stuck_behavior, const on_edge* edge_behavior)
            : laser_bones(bones), stuck_state(stuck_behavior), edge_state(edge_behavior)
        {
        }

        virtual bool system_event(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                                  xBase* toParamWidget, U32 toParamWidgetID);
        virtual void add_states(xAnimTable* table);
        virtual void add_transitions(xAnimTable* table);
        virtual void exit_state();
        virtual void update(F32 dt);
        virtual void setup();
        virtual void reset();
        virtual bool runnable(F32 dt);
        virtual bool grabbable(GrabType grabType);
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
        virtual void done_attack() {}
    };

    struct wait_shoot : shoot
    {
        F32 volley_timer;
        F32 time_between_volleys;

        wait_shoot(bone_container<laser_bone>& bones, const stuck* stuck_behavior, const on_edge* edge_behavior)
            : shoot(bones, stuck_behavior, edge_behavior)
        {
        }

        virtual void setup();
        virtual void reset();
        virtual bool runnable(F32 dt);
        virtual void done_attack();
    };
} // namespace zNPC

#endif

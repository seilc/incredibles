#ifndef ZNPCROBOTBEHAVIOR_H
#define ZNPCROBOTBEHAVIOR_H

#include "zNPCCommon.h"
#include "zNPCBehaviorFX.h"
#include "xSnd.h"

namespace zNPC
{
    struct static_jump : npc_move
    {
        U32 anim_flags;

        static_jump(U32 flags)
        {
            anim_flags = flags;
        }

        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
        virtual void add_states(xAnimTable* table);
        virtual void update(F32 dt);
        virtual bool runnable(F32 dt);
    };

    struct explode : shrapnel_death
    {
        F32 damage_radius;
        F32 damage_radius_2;
        F32 velocity_up;
        F32 velocity_away;
        S16 damage;
        bool hurt_things;

        virtual bool system_event(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                                  xBase* toParamWidget, U32 toParamWidgetID);
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
        virtual void setup();

        virtual void reset()
        {
            shrapnel_death::reset();
            hurt_things = true;
        }
    };

    struct engine_exhaust : behavior_implementation<common>
    {
        xModelInstance* model[2];
        bool engine_on;
        S8 bones[2];
        xVec3 effect_offset;
        xVec3 effect_pos[2];
        F32 radius;

        ~engine_exhaust();

        virtual void update(F32 dt);
        virtual void setup();
        virtual void render();
        virtual bool runnable(F32) { return engine_on; }
        virtual void on(bool is_on) { engine_on = is_on; }
    };

    struct charge_attack : npc_move
    {
        F32 attack_radius;
        xVec3 desired_dpos;
        xVec3 old_pos;
        U8 attacking;
        U8 entered;
        F32 charge_distance;
        F32 charge_past;
        F32 charge_turn_speed;
        xSndGroupHandle charge_noise_group;

        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
        virtual void exit_state();
        virtual void update(F32 dt);
        virtual void setup();
        virtual bool runnable(F32 dt);

        U32 attack_start(xAnimTransition* tran, xAnimSingle* anim);
        U32 facing_player(xAnimTransition* tran, xAnimSingle* anim);
    };
} // namespace zNPC

#endif

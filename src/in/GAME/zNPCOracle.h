#ifndef ZNPCORACLE_H
#define ZNPCORACLE_H

#include "zNPCCommon.h"
#include "zNPCRobotBehavior.h"
#include "zNPCVelocipod.h"

namespace zNPC
{
    struct oracle_hover_effect : hover_effect
    {
        virtual bool runnable(F32 dt)
        {
            return owner->get_combat()->currentHitPoints > 0 && hover_effect::runnable(dt);
        }
    };

    struct oracle_idle : behavior_implementation<common>
    {
        virtual const char* getName() { return "oracle_idle"; }
        virtual void add_states(xAnimTable* table);
        virtual bool runnable(F32 dt);
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
    };

    struct oracle_charge : charge_attack
    {
        engine_exhaust* engine;
        F32 detonateRadius;
        F32 chargeTime;
        F32 timer;

        oracle_charge(engine_exhaust* exhaust)
        {
            engine = exhaust;
        }

        virtual const char* getName() { return "oracle_charge"; }
        virtual void add_states(xAnimTable* table);
        virtual void add_transitions(xAnimTable* table);
        virtual void exit_state();
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
        virtual void setup();
        virtual void update(F32 dt);

        U32 opened(xAnimTransition* tran, xAnimSingle* anim);

        BEHAVIOR_ANIM_CALLBACK(oracle_charge, common, opened)
        BEHAVIOR_ANIM_CALLBACK(oracle_charge, common, facing_player)
    };

    struct oracle : common
    {
        oracle_hover_effect oracle_hover_effect_state;
        engine_exhaust engine_exhaust_state;
        oracle_idle oracle_idle_state;
        oracle_charge oracle_charge_state;
        explode oracle_explode_state;
        static_jump jump_state;

        oracle();
        virtual const char* get_type_name() const { return "bot|oracle"; }
    };
} // namespace zNPC

#endif

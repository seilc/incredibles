#ifndef ZNPCMINE_H
#define ZNPCMINE_H

#include "zNPCCommon.h"
#include "zNPCFriendly.h"
#include "zNPCRobotBehavior.h"

namespace zNPC
{
    struct initial_anim : behavior_implementation<common>
    {
        F32 timer;

        virtual void add_states(xAnimTable* table);
        virtual void update(F32 dt);
        virtual void setup();
        virtual void reset();
        virtual bool runnable(F32 dt);
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
    };

    struct mine_charge : npc_move
    {
        F32 radius;
        F32 timer;
        F32 wait_time;

        virtual const char* getName() { return "mine_charge"; }
        virtual void add_states(xAnimTable* table);
        virtual void add_transitions(xAnimTable* table);
        virtual void update(F32 dt);
        virtual void setup();
        virtual bool runnable(F32 dt);
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
    };

    struct mine_home : npc_move
    {
        F32 radius2;

        virtual void add_states(xAnimTable* table);
        virtual void update(F32 dt);
        virtual void setup();
        virtual bool runnable(F32 dt);
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
    };

    struct mine_explode : explode
    {
        F32 radius;

        virtual const char* getName() { return "mine_explode"; }
        virtual void setup();
        virtual bool runnable(F32 dt);
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
    };

    struct mine : common
    {
        idle idle_state;
        mine_home mine_home_state;
        mine_charge mine_charge_state;
        initial_anim rising_state;
        mine_explode mine_explode_state;
        F32 rings_emitted;

        mine();
        static void type_setup();
        virtual void update_npc(F32 dt);
        virtual void setup();
        virtual const char* get_type_name() const { return "bot|mine"; }
        virtual void reset();
    };
} // namespace zNPC

#endif

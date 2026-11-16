#ifndef ZNPCLOBBER_H
#define ZNPCLOBBER_H

#include "zNPCCommon.h"
#include "zNPCBehavior.h"

template <class T>
class array;

namespace zNPC
{
    class flash_bone;

    template <class T>
    struct firing_attack_generic : T
    {
        group_attack attacker;
        F32 facing_cos;
        U8 facing_player;

        virtual void update(F32 dt);

        virtual void setup()
        {
            F32 angle;
            owner->get_parameter("FiringCone", &angle, 60.0f);
            angle = (PI / 180.0f) * angle;
            facing_cos = icos(angle / 2.0f);
            facing_player = 0;
            T::setup();
        }

        virtual void reset();
    };

    template <class T>
    struct lob_generic : firing_attack_generic<T>
    {
        bool use_tag;
        char* lob_anim;
        F32 inner_radius;
        F32 outer_radius;
        F32 last_dt;
        F32 delay;
        F32 in_front_distance;
        xVec3 direction;
        array<flash_bone>* flash_bones;
        xModelTag gun_tag;
        S32 gun;
        xVec3 gun_offset;

        lob_generic(array<flash_bone>* bones)
        {
            flash_bones = bones;
        }

        virtual const char* getName() { return "lob"; }
        virtual bool system_event(xBase* from, xBase* to, U32 to_event, const F32* to_param,
                                  xBase* to_param_widget, U32 to_param_widget_id);
        virtual void exit_state();

        virtual void setup()
        {
            xVec3 sentinel = { -1234.5f, -1234.5f, -1234.5f };
            xVec3 pos;
            owner->get_parameter("GunPosition", &pos, sentinel);
            use_tag = !xVec3Equals(&pos, &sentinel);
            if (use_tag) {
                xModelTagSetup(&gun_tag, owner->model, &pos);
            } else {
                owner->get_parameter("GunOffset", &gun_offset, xVec3::m_Null);
                owner->get_parameter("GunBone", &gun, 0);
            }
            owner->get_parameter("LobDelayTime", &delay, 6.0f);
            owner->get_parameter("InFrontDistance", &in_front_distance, 0.0f);
            firing_attack_generic<T>::setup();
        }

        virtual void reset()
        {
            owner->get_parameter("LobDelayTime", &delay, 6.0f);
        }
    };

    struct human_lob : lob_generic<move>
    {
        F32 wait;
        F32 blast_radius;
        S32 bomb_damage;
        F32 flash_time;
        F32 gravity;
        F32 total_flash_time;
        F32 emit_remaining;
        xVec3 bomb_pos_start;
        xVec3 bomb_pos_end;
        F32 lob_timeout;
        F32 retry_wait_time;

        human_lob() : lob_generic<move>(NULL)
        {
        }

        virtual const char* getName() { return "human_lob"; }
        virtual bool system_event(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                                  xBase* toParamWidget, U32 toParamWidgetID);
        virtual void add_states(xAnimTable* table);
        virtual void add_transitions(xAnimTable* table);
        virtual void update(F32 dt);
        virtual void setup();
        virtual void reset();
        virtual bool runnable(F32 dt);
        virtual bool grabbable(GrabType grabType);
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
        virtual void launch();
    };
} // namespace zNPC

#endif

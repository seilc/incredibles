#ifndef ZNPCBEHAVIORFX_H
#define ZNPCBEHAVIORFX_H

#include "zNPCCommon.h"

struct zShrapnelAsset;

namespace zNPC
{
    class shrapnel_death : public behavior_implementation<common>
    {
    public:
        xModelInstance* base_model;
        xModelInstance* destroyed_model;
        zShrapnelAsset* shrapnel;
        xVec3 old_pos[2];
        F32 old_dt[2];
        U8 destroyed_collidable;

        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
        virtual void setup();
        virtual void reset();
        virtual bool runnable(F32 dt);
    };

    class bone_behavior : public behavior_implementation<common>
    {
    public:
        S32 index;

        void set_index(S32 new_index)
        {
            index = new_index;
        }

        static bool is_valid_bone(common* npc, S32 index, const char* name)
        {
            S32 bone;
            return npc->get_parameter(name, index, &bone, -1);
        }
    };

    class firing_bone : public bone_behavior
    {
    public:
        S32 position_bone;
        S32 aim_bone;
        S32 atomic_num;
        union
        {
            xModelTag tag;
            xVec3 bone_offset;
        };
        xVec3 fire_direction;

        void setup_base(const char* prefix);
        xVec3 get_position();
    };

    class muzzel_flash
    {
    public:
        struct config
        {
            U8 on;
            U8 color_r;
            U8 color_g;
            U8 color_b;
            U8 color_a;
            F32 emit_rate;
            F32 life;
            F32 speed;
            F32 size;
            F32 min_rot;
            F32 max_rot;
            F32 fade_time;
        };
    };

    class laser_bone : public firing_bone
    {
    public:
        bool has_muzzel_flash;
        F32 laser_speed;
        F32 laser_length;
        F32 laser_thickness;
        S32 laser_damage;
        F32 laser_shake;
        F32 fire_shake;
        F32 fire_shake_radius;
        F32 max_laser_distance;
        F32 muzzel_flash_emit;
        F32 flash_time;
        F32 muzzel_flash_time;
        F32 flash_delay;
        U32 hitmarktextureid;
        S32 explosion_type;
        xVec3 position;
        xVec3 direction;
        xColor color;
        muzzel_flash::config muzzel_flash_config;

        virtual void update(F32 dt);
        virtual void setup();
        virtual void reset();
        virtual bool runnable(F32 dt) { return true; }

        static bool is_valid_bone(common* npc, S32 index)
        {
            return bone_behavior::is_valid_bone(npc, index, "FireAimBone");
        }
    };

    template <class T>
    class bone_container : public behavior_implementation<common>
    {
    public:
        T* array;
        S32 size_;

        bone_container() {}

        virtual void init()
        {
            owner->remove_behavior(this);

            size_ = 0;
            while (true) {
                if (!T::is_valid_bone(owner, size_ + 1)) {
                    break;
                }
                size_++;
            }

            if (size_ != 0) {
                array = new (eMemStaticType_0, 0) T[size_];
                for (S32 i = 0; i < size(); i++) {
                    owner->add_behavior(&array[i], -1);
                    array[i].set_index(i + 1);
                    array[i].init();
                }
            }
        }

        virtual bool runnable(F32 dt)
        {
            return false;
        }

        S32 size()
        {
            return size_;
        }
    };
} // namespace zNPC

#endif

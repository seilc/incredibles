#ifndef ZNPCVELOCIPOD_H
#define ZNPCVELOCIPOD_H

#include "zNPCCommon.h"
#include "zNPCBehaviorFX.h"
#include "xSndMgr.h"
#include "xString.h"

namespace zAsset
{
    struct spline_path;
} // namespace zAsset

namespace zNPC
{
    struct hover_effect : behavior_implementation<common>
    {
        U8 was_last_over_ground;
        F32 hover_effect_height;
        F32 emit_remaining;
        xVec3 last_ground_pos;

        virtual const char* getName() { return "hover_effect"; }
        virtual void update(F32 dt);
        virtual void setup();
        virtual void reset();
        virtual bool exclusive() { return false; }
        virtual bool runnable(F32 dt);
    };

    struct follow_spline_path : move
    {
        U8 freeze_;
        xVec3 last_spline_end;
        xVec3 last_direction;
        zAsset::spline_path* start_path;
        zAsset::spline_path* path;
        F32 u;
        F32 speed;
        F32 acceleration;
        F32 allow_offset_distance;
        F32 hover_time;
        F32 bank_force;
        F32 speed_force;
        F32 position_blend_v;
        F32 orient_blend_v;
        F32 position_blend_speed;
        F32 orient_blend_speed;
        F32 max_offset;
        F32 offset_blend_k;
        F32 offset_v;
        F32 target_speed;
        xVec3 offset;
        xQuat current_orient;
        enum
        {
            POSITIVE,
            NEGATIVE
        } direction;

        virtual const char* getName() { return "follow_spline_path"; }
        virtual void update(F32 dt);
        virtual void setup();
        virtual void reset();
        virtual bool exclusive() { return false; }
        virtual bool runnable(F32 dt);
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);

        xVec3 get_future_position(F32 time) const;
    };

    struct hover_bob : behavior_implementation<common>
    {
        F32 max_offset;
        F32 max_speed;
        F32 max_angle;
        F32 max_angle_speed;
        F32 target_offset;
        F32 offset;
        F32 velocity;
        F32 velocity_v;
        F32 velocity_blend_k;
        xVec3 angle_velocity;
        F32 angle_velocity_v;
        F32 angle_velocity_blend_k;
        xVec3 target_up;
        xVec3 up;

        virtual void update(F32 dt);
        virtual void setup();
        virtual void reset();
        virtual bool runnable(F32 dt) { return true; }
        virtual void before_anim_matrices(xAnimPlay* play, xQuat* model_quat, xVec3* tran,
                                          S32 boneCount);
    };

    class blade_bone : public bone_behavior
    {
    public:
        U8 enabled;
        S32 bone;
        S32 blade_orientation_bone;
        S32 position_bone;
        S32 orientation_bone;
        F32 speed;
        F32 rotation1;
        F32 rotation2;
        xVec3 axis;
        union
        {
            xVec3 offset;
            xModelTag tag;
        };
        xModelInstance* rotor;

        virtual void update(F32 dt);
        virtual void setup();
        virtual void exit();
        virtual void reset();
        virtual void render();
        virtual bool runnable(F32 dt) { return true; }
        virtual void before_anim_matrices(xAnimPlay* play, xQuat* quat, xVec3* tran,
                                          S32 boneCount);

        static bool is_valid_bone(common* npc, S32 index)
        {
            return bone_behavior::is_valid_bone(npc, index, "BladeBone");
        }
    };

    template <class T>
    struct flying_npc_generic : T
    {
        const behavior_implementation<common>& death_state;
        xSndGroupHandle flying_noise_group;
        iSndHandle flying_noise;
        bool started;

        flying_npc_generic(const behavior_implementation<common>& death) : death_state(death)
        {
        }

        virtual void setup()
        {
            started = false;
            const char* noise_name;
            get_parameter("FlyingNoiseGroup", &noise_name, "FlyingNoise");
            flying_noise_group = xSndMgrGetSoundGroup(xStrHash(noise_name));
            flying_noise = ISNDHANDLE_INVALID;
            T::setup();
        }

        virtual void reset()
        {
            started = false;
            T::reset();
        }

        virtual void update_npc(F32 dt)
        {
            T::update_npc(dt);
            if (commonFlags.active) {
                if (!started) {
                    started = true;
                    if (flying_noise == ISNDHANDLE_INVALID) {
                        flying_noise = xSndMgrPlay(flying_noise_group, xSnd_FOLLOW, NULL, NULL, this, NULL, NULL);
                    }
                } else if (get_current_behavior() == &death_state) {
                    stop_sound();
                }
            }
        }

        void stop_sound()
        {
            if (flying_noise != ISNDHANDLE_INVALID) {
                xSndMgrStop(flying_noise);
                flying_noise = ISNDHANDLE_INVALID;
            }
        }

        virtual void kill(bool still_collide)
        {
            T::kill(still_collide);
            stop_sound();
        }

        virtual void deactivate()
        {
            started = false;
            stop_sound();
            T::deactivate();
        }

        virtual xVec3 get_future_position(F32 time) const
        {
            return frame->mat.pos;
        }

        virtual U8 get_target_priority() const
        {
            return 20;
        }
    };
} // namespace zNPC

#endif

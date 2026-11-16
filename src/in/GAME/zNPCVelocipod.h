#ifndef ZNPCVELOCIPOD_H
#define ZNPCVELOCIPOD_H

#include "zNPCCommon.h"

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
} // namespace zNPC

#endif

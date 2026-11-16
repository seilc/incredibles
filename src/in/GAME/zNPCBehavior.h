#ifndef ZNPCBEHAVIOR_H
#define ZNPCBEHAVIOR_H

#include "zBehavior.h"

struct xMovePoint;

namespace zNPC
{
    struct common;

    struct move : behavior_implementation<common>
    {
        F32 turn_speed;
        F32 move_speed;
        F32 min_speed;
        F32 acceleration;
        F32 speed;

        virtual const char* getName() { return "move"; }
        virtual void setup();
        virtual void reset();
    };

    struct follow_movepoints : behavior_implementation<common>
    {
        U8 orient;
        U8 orient3d;
        U8 stop;
        F32 mass_accel_adjust;
        F32 stop_distance_start;
        F32 speed_force;
        F32 angle_change;
        xMovePoint* movepoint;
        xMovePoint* next_movepoint;
        F32 speed;
        F32 velocity_blend_k;
        F32 orientation_blend_k;
        F32 velocity_v;
        F32 orientation_v;
        F32 arrive_threshold;
        F32 begin_turn_distance;
        F32 bank_force;
        F32 bank;
        F32 bank_v;
        F32 bank_blend_k;
        F32 tilt;
        F32 time;
        F32 pivot_distance;
        xQuat bank_quat;
        xVec3 position;
        xVec3 velocity;
        xVec3 offset;

        virtual bool system_event(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                                  xBase* toParamWidget, U32 toParamWidgetID);
        virtual void update(F32 dt);
        virtual void setup();
        virtual void reset();
        virtual bool runnable(F32 dt);
        virtual xMovePoint* get_next_movepoint();
        virtual void set_start_movepoint();
        virtual void get_orientation(xMat3x3* mat, F32 distance);

        void set_speed(F32 new_speed);

        F32 get_speed() const
        {
            return speed;
        }
    };
} // namespace zNPC

#endif

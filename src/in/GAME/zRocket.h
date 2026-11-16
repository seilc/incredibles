#ifndef ZROCKET_H
#define ZROCKET_H

#include "xMath3.h"
#include "xEnt.h"
#include "zLightWeight.h"

namespace z
{
    class rocket : public lightweight<rocket, 20>
    {
    public:
        F32 turn_speed;
        F32 time;
        xVec3 position;
        xVec3 velocity;
        xEnt* source;
        xModelInstance* model;
        xVec3* target_pos;
        F32 above_target_pos;
        iSndHandle flying_noise;
        F32 emit_remaining;
        F32 jet_emit_remaining;

        rocket();
        rocket(xVec3* target_pos, F32 above_target_pos, xEnt* source, const xVec3& position,
               const xVec3& direction, F32 turn_speed, F32 rocket_speed);
    };

    class JetParticleSystem
    {
    public:
        struct config
        {
            F32 min_life;
            F32 max_life;
            F32 min_size;
            F32 max_size;
            F32 velocity;
            F32 emit_rate;
            F32 gravity;
            F32 slow;
            F32 fade_start;
            U8 r;
            U8 g;
            U8 b;
            F32 system_emit_time;
            F32 glow;
            F32 min_rot;
            F32 max_rot;
            F32 size_delta;
            F32 normal_emit_speed;
            xVec3 start_velocity;
        };

        static config default_config;

        static void setup();
        static void emit(xVec3 position, xVec3 normal, F32& emit_remaining, xVec3 velocity, F32 dt,
                         xMat4x3* mat, config* cfg);
    };
} // namespace z

#endif

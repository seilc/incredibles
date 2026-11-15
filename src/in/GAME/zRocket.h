#ifndef ZROCKET_H
#define ZROCKET_H

#include "xMath3.h"

namespace z
{
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

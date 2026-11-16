#ifndef ZNPCVELOCIPOD_H
#define ZNPCVELOCIPOD_H

#include "zNPCCommon.h"

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
} // namespace zNPC

#endif

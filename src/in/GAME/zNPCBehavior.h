#ifndef ZNPCBEHAVIOR_H
#define ZNPCBEHAVIOR_H

#include "zBehavior.h"

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

        virtual void setup();
        virtual void reset();
    };
} // namespace zNPC

#endif

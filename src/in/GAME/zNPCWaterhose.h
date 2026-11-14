#ifndef ZNPCWATERHOSE_H
#define ZNPCWATERHOSE_H

#include "zNPCBehaviorFX.h"
#include "zWaterhose.h"

namespace zNPC
{
    class water_bone : public firing_bone
    {
    public:
        zWaterhose* waterhose;

        virtual const char* getName() { return "water_bone"; }
        virtual void update(F32 dt);
        virtual void setup();
        virtual void reset();
        virtual void render_extra();
        virtual bool exclusive() { return false; }
        virtual bool runnable(F32);

        void turn_on();
        void turn_off();
    };
} // namespace zNPC

#endif

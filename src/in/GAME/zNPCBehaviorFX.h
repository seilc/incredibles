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
} // namespace zNPC

#endif

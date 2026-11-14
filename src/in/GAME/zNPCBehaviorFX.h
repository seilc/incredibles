#ifndef ZNPCBEHAVIORFX_H
#define ZNPCBEHAVIORFX_H

#include "zNPCCommon.h"

namespace zNPC
{
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

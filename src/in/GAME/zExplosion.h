#ifndef ZEXPLOSION_H
#define ZEXPLOSION_H

#include "xMath3.h"
#include "iSnd.h"

namespace zExplosion
{
    struct presence_volume
    {
        F32 knockback;
        xVec3 center;
        F32 radius;
        F32 damage_player;
        F32 damage_npc;
        F32 damage_other;
    };

    struct trail_emitter
    {
        S32 type;
        xVec3 loc;
        F32 emitted;
        S32 glare_id;
        F32 streak_delay;
        F32 streaks_emitted;
        F32 spark_delay;
        F32 sparks_emitted;
        presence_volume* presence;
        iSndHandle tracking_sound;
    };
} // namespace zExplosion

#endif
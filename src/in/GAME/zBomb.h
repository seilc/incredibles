#ifndef ZBOMB_H
#define ZBOMB_H

#include "types.h"

struct xEnt;
struct xVec3;

void sphere_damage(xEnt* fromEnt, const xVec3& epicenter, F32 blastRadius, S32 player_damage, S32 obj_damage, S32 npc_damage, F32 awayKnockback, F32 upKnockback);

inline void sphere_damage(xEnt* fromEnt, const xVec3& epicenter, F32 blastRadius, S32 player_damage, S32 npc_damage, F32 awayKnockback, F32 upKnockback)
{
    sphere_damage(fromEnt, epicenter, blastRadius, player_damage, npc_damage, npc_damage, awayKnockback, upKnockback);
}

inline void sphere_damage(xEnt* fromEnt, const xVec3& epicenter, F32 blastRadius, S32 damage, F32 awayKnockback, F32 upKnockback)
{
    sphere_damage(fromEnt, epicenter, blastRadius, damage, damage, awayKnockback, upKnockback);
}

#endif

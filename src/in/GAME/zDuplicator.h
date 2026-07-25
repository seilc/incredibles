#ifndef ZDUPLICATOR_H
#define ZDUPLICATOR_H

#include "zNPCCommon.h"

struct zDuplicatorAsset : xBaseAsset
{
    U16 initial_spawn;
    U16 maximum_in_game;
    U16 maximum_to_spawn;
    F32 spawn_rate;
};

struct zDuplicator : xEnt
{
    zNPC::common* avatar;
    bool need_reset;
    bool active;
    S32 npcs_alive;
    S32 npcs_healthy;
    S32 npcs_spawned;
    U32 spawn_event;
    F32 time_till_next_spawn;
    zNPC::common** npc_pool;
    zDuplicatorAsset* duplicator_asset;
};

#endif
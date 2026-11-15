#ifndef ZDUPLICATOR_H
#define ZDUPLICATOR_H

#include "xEnt.h"
#include "zDuplicatorAsset.h"

struct zScene;

namespace zNPC
{
    struct common;
}

struct zDuplicator : xEnt
{
    zDuplicator();
    U32 init(zScene* scene, zDuplicatorAsset* asset, U32 common_idx);
    virtual void update(xScene*, F32 dt);
    virtual void setup();
    virtual void reset();
    void set_position(zNPC::common* npc);
    S32 num_npcs_healthy();
    S32 count_the_bodies();
    void spawn();

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

U32 zDuplicatorQuerySubObjects(const void* asset);

#endif

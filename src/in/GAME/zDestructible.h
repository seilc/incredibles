#ifndef ZDESTRUCTIBLE_H
#define ZDESTRUCTIBLE_H

#include "xSnd.h"

struct xEnt;
struct xModelInstance;
struct xDestructibleAsset;
struct zDestructibleState;

struct zDestructible
{
    xDestructibleAsset* asset;
    U32 id;
    U32 cur_hit_pts;
    U32 curstateidx;
    U32 nstates;
    U32 hit_pts;
    U32 hit_filter;
    U32 launch_flag;
    U32 behaviour;
    U32 base_modelID;
    xSndGroupHandle sg_handle_idle;
    iSndHandle sh_handle_idle;
    xModelInstance* base_model;
    xModelInstance* base_collision;
    zDestructibleState* dstates;
    F32 hit_to_destroy_timer;
    F32 timer;
    F32 respawn_time;
    void(*destroy_notify)(zDestructible*, void*);
    void* notify_context;
    xEnt* root_ent;
    U32 flags;
    bool has_animation;
    bool active;
};

zDestructible* zDestructibleGetFromEntity(xEnt* ent);

#endif

#include "xCommon.h"
#include "zDuplicator.h"

#include "zNPCCommon.h"
#include "zNPCMgr.h"
#include "zNPCTypes.h"
#include "zCombat.h"
#include "zScene.h"
#include "zUIIncredibles.h"
#include "xEvent.h"
#include "xRand.h"
#include "xString.h"

#include <stdio.h>
#include <string.h>

void update_wrapper(xEnt* ent, xScene* xscn, F32 dt)
{
    zDuplicator* duplicator = (zDuplicator*)ent;
    xEntBeginUpdate(duplicator, xscn, dt);
    duplicator->update(xscn, dt);
    xEntEndUpdate(duplicator, xscn, dt);
}

void event_wrapper(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                   U32 toParamWidgetID)
{
    zDuplicator* duplicator = (zDuplicator*)to;

    if (duplicator->avatar) {
        zEntEvent(duplicator->avatar, toEvent, toParam, toParamWidget, toParamWidgetID);
        if (toEvent == eEventEnable) {
            zEntEvent(duplicator->avatar, eEventNPCSetActiveOn, toParam, toParamWidget, toParamWidgetID);
        }
    }

    switch (toEvent) {
    case eEventJumpOnSpawn:
        duplicator->spawn_event = toEvent;
        break;
    case eEventRandomJumpOnSpawn:
        duplicator->spawn_event = toEvent;
        break;
    case eEventDuplicatorActivate:
        duplicator->active = true;
        break;
    case eEventDuplicatorDeactivate:
        duplicator->active = false;
        break;
    default:
        for (S32 i = 0; i < duplicator->duplicator_asset->maximum_in_game; i++) {
            if (duplicator->npc_pool[i]->alive() && from != duplicator->npc_pool[i]) {
                zEntEvent(duplicator, duplicator->npc_pool[i], toEvent, toParam, toParamWidget, toParamWidgetID);
            }
        }
    }
}

zDuplicator::zDuplicator()
{
}

U32 zDuplicator::init(zScene* scene, zDuplicatorAsset* asset, U32 common_idx)
{
    duplicator_asset = asset;
    xEnt::update = update_wrapper;
    eventFunc = event_wrapper;
    need_reset = false;
    npc_pool = (zNPC::common**)xMEMALLOC(asset->maximum_in_game * sizeof(zNPC::common*), 0);

    xEntNPCAssetIN* duplicator_npc = (xEntNPCAssetIN*)(duplicator_asset + 1);
    xEntAsset* entass = (xEntAsset*)(duplicator_npc + 1);
    entass->baseType = eBaseTypeDuplicator;
    xBaseInit(this, entass);
    link = (xLinkAsset*)((U8*)entass + sizeof(xEntAsset) + sizeof(xEntNPCAssetIN));
    entass->baseType = eBaseTypeNPC;
    this->asset = entass;
    entass->baseFlags |= k_XBASE_IS_ENTITY;
    baseFlags |= k_XBASE_IS_ENTITY;

    if (duplicator_npc->npcModel) {
        U32 new_id = xStrHashCat(asset->id, "AVATAR");
        xEntAsset* avatar_asset = (xEntAsset*)xMEMALLOC(sizeof(xEntAsset) + sizeof(xEntNPCAssetIN), 0);
        memcpy(avatar_asset, entass, sizeof(xEntAsset));
        memcpy(avatar_asset + 1, duplicator_npc, sizeof(xEntNPCAssetIN));
        avatar_asset->modelInfoID = ((xEntNPCAssetIN*)(avatar_asset + 1))->npcModel;
        avatar_asset->id = new_id;
        avatar_asset->linkCount = 0;
        avatar = (zNPC::common*)zNPCMgr_createNPCInst(0, avatar_asset);
        zSceneSet(avatar, common_idx);
        xSceneAddEnt(scene, avatar);
        zSceneObjHashtableAdd(avatar->id, avatar);
        common_idx++;
    }

    for (S32 i = 0; i < asset->maximum_in_game; i++) {
        char additional_identifier[10];
        sprintf(additional_identifier, "%d", i);
        U32 new_id = xStrHashCat(asset->id, additional_identifier);
        xEntAsset* unique_asset = (xEntAsset*)xMEMALLOC(sizeof(xEntAsset) + sizeof(xEntNPCAssetIN), 0);
        memcpy(unique_asset, entass, sizeof(xEntAsset) + sizeof(xEntNPCAssetIN));
        unique_asset->id = new_id;
        unique_asset->linkCount = 0;
        zNPC::common* npc = (zNPC::common*)zNPCMgr_createNPCInst(0, unique_asset);
        if (npc) {
            npc->duplicator = this;
            npc_pool[i] = npc;
            zSceneSet(npc, common_idx);
            xSceneAddEnt(scene, npc);
            zSceneObjHashtableAdd(unique_asset->id, npc);
            common_idx++;
        }
    }

    return common_idx;
}

void zDuplicator::setup()
{
    xBaseSetup(this);
    reset();
}

void zDuplicator::reset()
{
    xBaseReset(this, asset);
    active = true;
    need_reset = true;
    npcs_alive = 0;
    npcs_healthy = 0;
    npcs_spawned = 0;
    if (duplicator_asset->spawn_rate) {
        time_till_next_spawn = duplicator_asset->spawn_rate;
    } else {
        time_till_next_spawn = 0.0f;
    }
}

void zDuplicator::set_position(zNPC::common* npc)
{
    if (avatar) {
        ((xMat4x3*)npc->model->Mat)->pos = *xEntGetPos(avatar);
    }
}

void zDuplicator::update(xScene*, F32 dt)
{
    if (!xEntIsEnabled(this) || (avatar && !avatar->alive())) {
        return;
    }

    if (need_reset) {
        need_reset = false;
        for (S32 i = 0; i < duplicator_asset->initial_spawn; i++) {
            spawn();
        }
    }

    if (active) {
        time_till_next_spawn -= dt;
    }

    S32 old_npcs_alive = npcs_alive;
    npcs_alive = count_the_bodies();
    if (npcs_spawned >= duplicator_asset->maximum_to_spawn && npcs_alive <= 0 && old_npcs_alive > 0) {
        zEntEvent(this, this, eEventSpawnedNPCsAllKilled);
    }
    for (S32 i = npcs_alive; i < old_npcs_alive; i++) {
        zEntEvent(this, this, eEventSpawnedNPCKilled);
    }

    S32 old_npcs_healthy = npcs_healthy;
    npcs_healthy = num_npcs_healthy();
    if (npcs_spawned >= duplicator_asset->maximum_to_spawn && npcs_healthy <= 0 && old_npcs_healthy > 0) {
        zEntEvent(this, this, eEventSpawnedNPCsAllNoHealth);
    }
    for (S32 i = npcs_healthy; i < old_npcs_healthy; i++) {
        zEntEvent(this, this, eEventSpawnedNPCNoHealth);
    }

    if (npcs_alive >= duplicator_asset->maximum_in_game) {
        time_till_next_spawn = duplicator_asset->spawn_rate;
    } else if (npcs_spawned < duplicator_asset->maximum_to_spawn || duplicator_asset->maximum_to_spawn == 0) {
        if (time_till_next_spawn < 0.0f) {
            time_till_next_spawn = duplicator_asset->spawn_rate;
            spawn();
        }
        if (npcs_spawned >= duplicator_asset->maximum_to_spawn && duplicator_asset->maximum_to_spawn != 0) {
            zEntEvent(this, this, eEventNPCSpawnDone);
        }
    }
}

S32 zDuplicator::num_npcs_healthy()
{
    U16 count = 0;
    for (S32 i = 0; i < duplicator_asset->maximum_in_game; i++) {
        if (npc_pool[i]->alive()) {
            zCombat* combat = npc_pool[i]->get_combat();
            if (combat && combat->currentHitPoints > 0) {
                count++;
            }
        }
    }
    return count;
}

S32 zDuplicator::count_the_bodies()
{
    U16 count = 0;
    for (S32 i = 0; i < duplicator_asset->maximum_in_game; i++) {
        if (npc_pool[i]->alive()) {
            count++;
        }
    }
    return count;
}

void zDuplicator::spawn()
{
    if (!active) {
        return;
    }

    for (S32 i = 0; i < duplicator_asset->maximum_in_game; i++) {
        if (!npc_pool[i]->alive()) {
            npc_pool[i]->reset();
            npc_pool[i]->revive();
            xEntEnable(npc_pool[i]);
            set_position(npc_pool[i]);

            if (spawn_event == eEventJumpOnSpawn || spawn_event == eEventRandomJumpOnSpawn) {
                xMovePoint* movepoint = npc_pool[i]->movepoint;
                if (spawn_event == eEventRandomJumpOnSpawn && movepoint->asset->numPoints) {
                    S32 index = xrand_RandomChoice(movepoint->asset->numPoints + 1);
                    if (index != movepoint->asset->numPoints) {
                        movepoint = movepoint->nodes[index];
                    }
                }
                npc_pool[i]->system_event(this, npc_pool[i], eEventJumpTo, NULL, movepoint, movepoint->id);
            }

            if (npc_pool[i]->get_type() != NPC_TYPE_MINE) {
                stats::add("Created", 1.0f);
            }

            zEntEvent(this, this, eEventNPCSpawn);
            npcs_spawned++;
            npcs_alive++;
            npcs_healthy++;
            return;
        }
    }
}

U32 zDuplicatorQuerySubObjects(const void* asset)
{
    U32 spawn_count = ((const zDuplicatorAsset*)asset)->maximum_in_game;
    if (((const xEntNPCAssetIN*)((const zDuplicatorAsset*)asset + 1))->npcModel) {
        spawn_count++;
    }
    return spawn_count;
}

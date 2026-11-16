#include "xCommon.h"
#include "zNPCPlayers.h"

#include "zMain.h"
#include "zGlobals.h"
#include "zCutsceneMgr.h"
#include "zHudFamilyMeter.h"
#include "zIncrediBallEffect.h"
#include "zFrozoneIce.h"
#include "zRenderState.h"
#include "zScene.h"
#include "xModel.h"
#include "xEvent.h"

namespace zNPC
{
    void health_hud::reset()
    {
    }

    void health_hud::setup()
    {
    }

    bool health_hud::damage(zCombatDamageInfo& damageInfo)
    {
        zCombat* combat = owner->get_combat();
        ZPLAYER->pFamilyMeter->set_health((F32)(combat->currentHitPoints - damageInfo.damage) /
                                          combat->maximumHitPoints);
        return false;
    }

    incrediball::incrediball()
    {
        add_behavior(&idle_state, -1);
        add_behavior(&follow_state, -1);
        add_behavior(&hud_state, -1);
    }

    void incrediball::regular_setup()
    {
        zCombat* combat = get_combat();
        combat->currentHitPoints = combat->maximumHitPoints = 800;
    }

    void incrediball::setup()
    {
        player::setup();
        regular_setup();
        zIncrediBallSetup();
        effect_mat = g_I3;
    }

    void incrediball::damage(const zCombatDamageInfo& damageInfo)
    {
        player::damage(damageInfo);
        if (!zCutsceneMgrRunning(globals.cmgr) && damageInfo.source == zHS_LASER) {
            xVec3 loc = frame->mat.pos;
            loc -= damageInfo.direction;
            zIncrediBallImpact(loc, 0.15f, -0.5f, false);
        }
    }

    void incrediball::render_inards_callback(void* context)
    {
        ((incrediball*)context)->render_inards();
    }

    void incrediball::render_inards()
    {
        zRenderState(SDRS_AlphaModels);
        S32 old_enabled = xModelBucketEnabled;
        xModelBucketEnabled = 0;
        common::render_npc();
        xModelBucketEnabled = old_enabled;
    }

    void incrediball::update_effect(F32 dt)
    {
        xVec3 dpos = frame->mat.pos - frame->oldmat.pos;
        F32 angle_change = (1.0f / 1.015f) * dpos.length();
        dpos.safe_normalize(g_O3);
        xVec3 right = g_Y3.cross(dpos);
        right.safe_normalize(g_Z3);
        xMat3x3 temp;
        xMat3x3Rot(&temp, &right, 0.15f * angle_change);
        xMat3x3Mul(&effect_mat, &effect_mat, &temp);
        xMat3x3Normalize(&effect_mat, &effect_mat);
        effect_mat.pos = frame->mat.pos;
        effect_mat.pos.y += 0.9f * bound.sph.r;
        zIncrediBallMove(effect_mat);
        xVec3 loc = { effect_mat.pos.x, effect_mat.pos.y - 1.0f, effect_mat.pos.z };
        zIncrediBallImpact(loc, 0.1f, -0.15f, true);
        zIncrediBallUpdate(dt);
    }

    static incrediball* the_ball;

    static void delayed_render_npc()
    {
        zIncrediBallRender(incrediball::render_inards_callback, the_ball);
    }

    void incrediball::render_npc()
    {
        the_ball = this;
        if (!zCutsceneMgrRunning(globals.cmgr)) {
            zSceneSpecialRenderSet(delayed_render_npc);
        }
    }

    void incrediball::update_npc(F32 dt)
    {
        common::update_npc(dt);
        if (!zCutsceneMgrRunning(globals.cmgr)) {
            update_effect(dt);
        }
    }

    bool ice_effect::system_event(xBase*, xBase*, U32 toEvent, const F32*, xBase*, U32)
    {
        switch (toEvent) {
        case eEventVisible:
            zFrozoneIceBridgeStart(false);
            break;
        case eEventInvisible:
            zFrozoneIceBridgeStop();
            break;
        }
        return false;
    }

    void ice_effect::update(F32)
    {
        player* p = (player*)owner;
        zFrozoneIceBridgeMove(p->predict_foot(predict_time), p->frame->mat.pos, false);
    }

    void ice_effect::setup()
    {
        owner->get_parameter("PredictTime", &predict_time, 0.1f);
    }

    void ice_effect::reset()
    {
        zFrozoneIceBridgeStop();
    }

    frozone::frozone()
    {
        add_behavior(&idle_state, -1);
        add_behavior(&follow_state, -1);
        add_behavior(&hud_state, -1);
        add_behavior(&ice_state, -1);
    }

    void frozone::setup()
    {
        player::setup();
        zCombat* combat = get_combat();
        combat->currentHitPoints = combat->maximumHitPoints = 450;
    }
} // namespace zNPC

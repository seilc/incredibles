#include "xCommon.h"
#include "zNPCMine.h"

#include "xutil.h"
#include "zGlobals.h"
#include "zMain.h"
#include "zPlayer.h"
#include "zSplash.h"
#include "zNPCBossOmnidroid.h"

namespace
{
    S32 splash_type = -1;
}

namespace zNPC
{
    void initial_anim::setup()
    {
        timer = 0.0f;
    }

    void initial_anim::reset()
    {
        timer = 0.0f;
    }

    void initial_anim::add_states(xAnimTable* table)
    {
        xAnimTableNewState(table, "IA_T_First01", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL,
                           NULL, NULL);
    }

    void initial_anim::update(F32 dt)
    {
        if (timer < 0.4f && timer + dt >= 0.4f) {
            zSplashEmit(zSplashFindType("Melee Bot Splash"), owner->frame->mat.pos);
        }
        timer += dt;
    }

    void initial_anim::enter_state(const behavior_implementation<common>*)
    {
        set_state("IA_T_First01", 0.15f, NULL);
    }

    bool initial_anim::runnable(F32 dt)
    {
        return timer <= 0.0f || in_state("IA_T_First01", dt, NULL);
    }

    void mine_charge::add_states(xAnimTable* table)
    {
        xAnimTableNewState(table, "MC_T_AttackStart01", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                           NULL, NULL, NULL);
        xAnimTableNewState(table, "MC_L_AttackLoop01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                           NULL, NULL, NULL);
    }

    void mine_charge::add_transitions(xAnimTable* table)
    {
        xAnimTableNewTransition(table, "MC_T_AttackStart01", "MC_L_AttackLoop01", NULL, NULL, 0x10, 0,
                                0.0f, 0.0f, 0, 0, 0.1f, NULL);
    }

    void mine_charge::setup()
    {
        owner->get_parameter("AlertRadius", &radius, 40.0f);
        owner->get_parameter("WaitTime", &wait_time, 2.0f);
        npc_move::setup();
    }

    bool mine_charge::runnable(F32 dt)
    {
        if (owner->get_target_dist() < radius && ZPLAYER->IsSwimming()) {
            return true;
        }
        timer -= dt;
        return timer > 0.0f;
    }

    void mine_charge::enter_state(const behavior_implementation<common>*)
    {
        set_state("MC_T_AttackStart01", 0.15f, NULL);
        timer = wait_time;
    }

    void mine_charge::update(F32 dt)
    {
        move_to(owner->get_target_pos(), dt, false, true, true);
    }

    void mine_home::add_states(xAnimTable* table)
    {
        xAnimTableNewState(table, "MH_L_Return01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL,
                           NULL, NULL);
    }

    void mine_home::update(F32 dt)
    {
        move_to(owner->asset->pos, dt, false, true, true);
    }

    void mine_home::enter_state(const behavior_implementation<common>*)
    {
        set_state("MH_L_Return01", 0.15f, NULL);
    }

    void mine_home::setup()
    {
        npc_move::setup();
        owner->get_parameter("HomeRadius", &radius2, 2.0f);
        radius2 = xsqr(radius2);
    }

    bool mine_home::runnable(F32)
    {
        return xVec3Dist2(&owner->frame->mat.pos, &owner->asset->pos) > radius2;
    }

    void mine_explode::setup()
    {
        owner->get_parameter("AttackRadius", &radius, 4.0f);
        explode::setup();
    }

    void mine_explode::enter_state(const behavior_implementation<common>* prev_behavior)
    {
        if (splash_type != -1) {
            zSplashEmit(splash_type, xEntGetFrame(owner)->pos);
        }
        if (globals.sceneCur->sceneID == IDTAG('N','I','0','3')) {
            hack_ni03_mine_damage_player(xEntGetFrame(owner)->pos);
            hurt_things = false;
        }
        explode::enter_state(prev_behavior);
    }

    bool mine_explode::runnable(F32)
    {
        return owner->get_target_dist() < radius;
    }

    void mine::type_setup()
    {
        splash_type = zSplashFindType("Mine Explode Splash");
    }

    mine::mine()
    {
        add_behavior(&idle_state, -1);
        add_behavior(&mine_home_state, -1);
        add_behavior(&mine_charge_state, -1);
        add_behavior(&rising_state, -1);
        add_behavior(&mine_explode_state, -1);
    }

    void mine::setup()
    {
        common::setup();
        commonFlags.cast_shadow = false;
        commonFlags.not_throw_target = true;
        rings_emitted = 0.0f;
    }

    void mine::update_npc(F32 dt)
    {
        common::update_npc(dt);
        xVec3 velocity = frame->mat.pos - frame->oldmat.pos;
        F32 speed = xsqrt(xsqr(velocity.x) + xsqr(velocity.y) + xsqr(velocity.z));
        if (!xfeq0(speed)) {
            xVec3 pos;
            xVec3Copy(&pos, &frame->mat.pos);
            pos.y += 0.18f;
            zEmitSwimRings(rings_emitted, pos, velocity, speed, dt);
        }
    }

    void mine::reset()
    {
        common::reset();
        commonFlags.cast_shadow = false;
        commonFlags.not_throw_target = true;
        rings_emitted = 0.0f;
    }
} // namespace zNPC

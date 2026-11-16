#include "xCommon.h"
#include "zNPCFlyShooter.h"

#include "zMain.h"
#include "zRocket.h"

namespace zNPC
{
    void fly_forward::enter_state(const behavior_implementation<common>* prev_behavior)
    {
        set_state("FM_L_Follow01", 0.15f, NULL);
    }

    void fly_forward::add_states(xAnimTable* table)
    {
        xAnimTableNewState(table, "FM_L_Follow01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL,
                           NULL, NULL);
    }

    bool fly_forward::runnable(F32 dt)
    {
        return true;
    }

    bool fly_forward::grabbable(GrabType grabType)
    {
        return grabType != eGrabTypeMrIGroundToss;
    }

    void rocket_lob::launch()
    {
        xVec3 rocket_pos;
        if (use_tag) {
            xModelTagEval(owner->model, &gun_tag, &rocket_pos);
        } else {
            rocket_pos = owner->get_bone_world_position_offset(gun, gun_offset);
        }

        direction = owner->frame->mat.at;

        z::rocket new_rocket(&ZPLAYER->frame->mat.pos, 1.0f, owner, rocket_pos, direction,
                             rocket_turn_speed, rocket_speed);
        z::rocket::add(new_rocket);
    }

    void rocket_lob::setup()
    {
        owner->get_parameter("RocketSpeed", &rocket_speed, 5.0f);
        owner->get_parameter("RocketTurnSpeed", &rocket_turn_speed, 2.0f);
        human_lob::setup();
    }

    bool rocket_lob::grabbable(GrabType grabType)
    {
        return grabType != eGrabTypeMrIGroundToss;
    }

    flying_shooter::flying_shooter()
        : flying_npc_generic<human_throwable>(projectile_state),
          fly_foward_state(follow_movepoints_state), shoot_state(laser_bones, NULL, NULL)
    {
        add_behavior(&fly_foward_state, -1);
        add_behavior(&follow_movepoints_state, -1);
        add_behavior(&shoot_state, -1);
        add_behavior(&bob_state, -1);
        add_behavior(&projectile_state, -1);
        add_behavior(&laser_bones, -1);
        add_behavior(&blade_bones, -1);
    }

    void flying_shooter::setup()
    {
        flying_npc_generic<human_throwable>::setup();
        old_speed = 0.0f;
    }

    void flying_shooter::reset()
    {
        flying_npc_generic<human_throwable>::reset();
        old_speed = 0.0f;
    }

    void flying_shooter::update_npc(F32 dt)
    {
        flying_npc_generic<human_throwable>::update_npc(dt);

        if (get_current_behavior() == &shoot_state || get_current_behavior() == &projectile_state) {
            if (follow_movepoints_state.get_speed() > 0.0f) {
                old_speed = follow_movepoints_state.get_speed();
                follow_movepoints_state.set_speed(0.0f);
            }
        } else if (follow_movepoints_state.get_speed() == 0.0f && old_speed > 0.0f) {
            follow_movepoints_state.set_speed(old_speed);
        }
    }

    flying_rocket::flying_rocket()
        : flying_npc_generic<human_throwable>(projectile_state),
          fly_foward_state(follow_movepoints_state)
    {
        add_behavior(&fly_foward_state, -1);
        add_behavior(&follow_movepoints_state, -1);
        add_behavior(&rocket_lob_state, -1);
        add_behavior(&bob_state, -1);
        add_behavior(&projectile_state, -1);
        add_behavior(&blade_bones, -1);
    }

    void flying_rocket::setup()
    {
        flying_npc_generic<human_throwable>::setup();
        old_speed = 0.0f;
    }

    void flying_rocket::reset()
    {
        flying_npc_generic<human_throwable>::reset();
        old_speed = 0.0f;
    }

    void flying_rocket::update_npc(F32 dt)
    {
        flying_npc_generic<human_throwable>::update_npc(dt);

        if (get_current_behavior() == &rocket_lob_state ||
            get_current_behavior() == &projectile_state) {
            if (follow_movepoints_state.get_speed() > 0.0f) {
                old_speed = follow_movepoints_state.get_speed();
                follow_movepoints_state.set_speed(0.0f);
            }
        } else if (follow_movepoints_state.get_speed() == 0.0f && old_speed > 0.0f) {
            follow_movepoints_state.set_speed(old_speed);
        }
    }

    flying_lobber::flying_lobber()
        : flying_npc_generic<human_throwable>(projectile_state),
          fly_foward_state(follow_movepoints_state)
    {
        add_behavior(&fly_foward_state, -1);
        add_behavior(&follow_movepoints_state, -1);
        add_behavior(&bob_state, -1);
        add_behavior(&lob_state, -1);
        add_behavior(&projectile_state, -1);
        add_behavior(&blade_bones, -1);
    }

    void flying_lobber::setup()
    {
        flying_npc_generic<human_throwable>::setup();
        old_speed = 0.0f;
    }

    void flying_lobber::reset()
    {
        flying_npc_generic<human_throwable>::reset();
        old_speed = 0.0f;
    }

    void flying_lobber::update_npc(F32 dt)
    {
        flying_npc_generic<human_throwable>::update_npc(dt);

        if (get_current_behavior() == &lob_state || get_current_behavior() == &projectile_state) {
            if (follow_movepoints_state.get_speed() > 0.0f) {
                old_speed = follow_movepoints_state.get_speed();
                follow_movepoints_state.set_speed(0.0f);
            }
        } else if (follow_movepoints_state.get_speed() == 0.0f && old_speed > 0.0f) {
            follow_movepoints_state.set_speed(old_speed);
        }
    }
} // namespace zNPC

#include "xCommon.h"
#include "zNPCFriendly.h"

#include "zRocket.h"
#include "xEvent.h"

#include "decomp.h"

namespace zNPC
{
    static z::JetParticleSystem::config fire_config;

    speak::speak()
    {
    }

    DECOMP_FORCEFLOAT(0.0f)

    void speak::add_states(xAnimTable* table)
    {
        xAnimTableNewState(table, "SP_L_Talk01", AnimState_Loop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL,
                           NULL, NULL);
    }

    DECOMP_FORCEACTIVE("Talkative", "MaxTalkDistance", "StartConversationDistance", "L1_BUTTON",
                       "fx_glyph_talk", "jawtime = %.2f\n", "jawval = %.2f\n")
    DECOMP_FORCEFLOAT(0.15f)

    void idle::add_states(xAnimTable* table)
    {
        xAnimTableNewState(table, "ID_L_Idle01", AnimState_Loop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL,
                           NULL, NULL);
    }

    void idle::enter_state(const behavior_implementation<common>*)
    {
        set_state("ID_L_Idle01", 0.3f, NULL);
    }

    void buddy_anim::add_states(xAnimTable* table)
    {
        xAnimTableNewState(table, "BA_L_Cycle01", AnimState_Loop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL,
                           NULL, NULL);
        xAnimTableNewState(table, "BA_T_Hit01", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL,
                           NULL, NULL);
    }

    void buddy_anim::add_transitions(xAnimTable* table)
    {
        xAnimTableNewTransition(table, "BA_T_Hit01", "BA_L_Cycle01", NULL, NULL, 0x10, 0, 0.0f, 0.0f,
                                0, 0, 0.1f, NULL);
    }

    void buddy_anim::enter_state(const behavior_implementation<common>*)
    {
        set_state("BA_L_Cycle01", 0.15f, NULL);
    }

    bool buddy_anim::system_event(xBase*, xBase*, U32 toEvent, const F32* toParam, xBase*, U32)
    {
        switch (toEvent) {
        case eEventHit:
            set_state("BA_T_Hit01", toParam[0], NULL);
            break;
        }
        return true;
    }

    friendly::friendly()
    {
        add_behavior(&anim_state, -1);
    }

    void friendly::setup()
    {
        common::setup();
        z::JetParticleSystem::setup();
        jet_offset = 0.1f;
        fire_config = z::JetParticleSystem::default_config;
        fire_config.fade_start = 0.0f;
        fire_config.max_life = 0.1f;
        fire_config.slow = 0.0f;
        fire_config.start_velocity.y = 0.0f;
        fire_config.g = 70;
        fire_config.min_life = 0.2f;
        fire_config.min_size = 0.1f;
        fire_config.normal_emit_speed = 0.6f;
        fire_config.r = 70;
        fire_config.size_delta = 1.0f;
        fire_config.velocity = 1.7f;
        left_emit_remaining = 0.0f;
        right_emit_remaining = 0.0f;
        left_jet_bone = 50;
        right_jet_bone = 54;
    }

    void friendly::reset()
    {
        common::reset();
        left_emit_remaining = 0.0f;
        right_emit_remaining = 0.0f;
    }

    void friendly::update_npc(F32 dt)
    {
        common::update_npc(dt);
        xVec3 left_pos = get_bone_world_position(left_jet_bone);
        xVec3 right_pos = get_bone_world_position(right_jet_bone);
        xVec3 left_dir = get_bone_world_direction(left_jet_bone, -g_Y3);
        xVec3 right_dir = get_bone_world_direction(right_jet_bone, -g_Y3);
        left_pos += left_dir * jet_offset;
        right_pos += right_dir * jet_offset;
        z::JetParticleSystem::emit(left_pos, left_dir, left_emit_remaining, g_O3, dt,
                                   (xMat4x3*)model->Mat, &fire_config);
        z::JetParticleSystem::emit(right_pos, right_dir, right_emit_remaining, g_O3, dt,
                                   (xMat4x3*)model->Mat, &fire_config);
    }
} // namespace zNPC

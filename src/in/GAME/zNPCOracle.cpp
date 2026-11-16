#include "xCommon.h"
#include "zNPCOracle.h"

#include "xDraw.h"

#include <string.h>

namespace zNPC
{
    void oracle_idle::add_states(xAnimTable* table)
    {
        xAnimTableNewState(table, "OI_L_Idle01", AnimState_Loop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL,
                           NULL, NULL);
    }

    void oracle_idle::enter_state(const behavior_implementation<common>*)
    {
        set_state("OI_L_Idle01", 0.15f, NULL);
    }

    bool oracle_idle::runnable(F32)
    {
        return true;
    }

    void oracle_charge::add_transitions(xAnimTable* table)
    {
        xAnimTableNewTransition(table, "OC_L_Ball01", "OC_T_Open01", anfacing_player, NULL, 0, 0,
                                0.0f, 0.0f, 1, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, "OC_T_Open01", "OC_L_Attack01", NULL, anopened, 0x10, 0,
                                0.0f, 0.0f, 1, 0, 0.1f, NULL);
    }

    void oracle_charge::add_states(xAnimTable* table)
    {
        xAnimTableNewState(table, "OC_L_Ball01", AnimState_Loop, 2, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL,
                           NULL, NULL);
        xAnimTableNewState(table, "OC_T_Open01", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL,
                           NULL, NULL);
        xAnimTableNewState(table, "OC_L_Attack01", AnimState_Loop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                           NULL, NULL, NULL);
    }

    U32 oracle_charge::opened(xAnimTransition* tran, xAnimSingle* anim)
    {
        engine->on(true);
        attack_start(tran, anim);
        return 0;
    }

    void oracle_charge::exit_state()
    {
        engine->on(false);
        charge_attack::exit_state();
    }

    void oracle_charge::enter_state(const behavior_implementation<common>* prev_behavior)
    {
        timer = 0.0f;
        if (facing_player(NULL, NULL)) {
            set_state("OC_T_Open01", 0.15f, NULL);
        } else {
            set_state("OC_L_Ball01", 0.15f, NULL);
        }
        charge_attack::enter_state(prev_behavior);
    }

    void oracle_charge::setup()
    {
        owner->get_parameter("DetonateRadius", &detonateRadius, 2.0f);
        owner->get_parameter("ChargeTime", &chargeTime, 1.0f);
        xAnimState* state = owner->model->Anim->Table->StateList;
        // BUG: reads state->Name before testing state
        while (!strstr(state->Name, "OC_T_Open01") && state) {
            state = state->Next;
        }
        if (xfeq0(chargeTime)) {
            state->Speed = state->Data->Duration;
        } else {
            state->Speed = state->Data->Duration / chargeTime;
        }
        charge_attack::setup();
    }

    void oracle_charge::update(F32 dt)
    {
        if (timer < chargeTime) {
            timer += dt;
            xDrawSetColor(g_PIMP_GOLD);
            xDrawSphere(&owner->frame->mat.pos, 2.0f * timer / chargeTime, 0xC0006);
        }
        U8 prev_state = attacking;
        charge_attack::update(dt);
        if (prev_state && !attacking) {
            owner->get_combat()->currentHitPoints = 0;
        }
    }

    oracle::oracle() : oracle_charge_state(&engine_exhaust_state), jump_state(0)
    {
        add_behavior(&oracle_hover_effect_state, -1);
        add_behavior(&engine_exhaust_state, -1);
        add_behavior(&oracle_idle_state, -1);
        add_behavior(&oracle_charge_state, -1);
        add_behavior(&jump_state, -1);
        add_behavior(&oracle_explode_state, -1);
    }
} // namespace zNPC

#ifndef ZNPCPLAYERS_H
#define ZNPCPLAYERS_H

#include "zNPCCommon.h"
#include "zNPCFriendly.h"
#include "zNPCVelocipod.h"

namespace zNPC
{
    struct player : common
    {
        follow_spline_path follow_state;

        virtual void setup()
        {
            common::setup();
            commonFlags.not_throw_target = true;
        }

        virtual void reset()
        {
            common::reset();
            commonFlags.not_throw_target = true;
        }

        virtual void damage(const zCombatDamageInfo& damageInfo)
        {
            if (damageInfo.source != zHS_FIRE) {
                common::damage(damageInfo);
            }
        }

        xVec3 predict_foot(F32 time) const
        {
            return follow_state.get_future_position(time);
        }
    };

    struct health_hud : behavior_implementation<common>
    {
        virtual bool damage(zCombatDamageInfo& damageInfo);
        virtual void setup();
        virtual void reset();
        virtual bool runnable(F32) { return true; }
        virtual bool exclusive() { return false; }
    };

    struct incrediball : player
    {
        idle idle_state;
        health_hud hud_state;
        xMat4x3 effect_mat;

        incrediball();
        virtual void update_npc(F32 dt);
        virtual void setup();
        virtual const char* get_type_name() const { return "incrediball"; }
        inline xVec3 predict(F32 time) const;
        virtual void render_npc();
        virtual void damage(const zCombatDamageInfo& damageInfo);

        void regular_setup();
        static void render_inards_callback(void* context);
        void render_inards();
        void update_effect(F32 dt);
    };

    struct elastigirl : player
    {
        idle idle_state;
        health_hud hud_state;

        elastigirl();
        inline virtual const char* get_type_name() const;
        inline xVec3 predict(F32) const;
    };

    struct ice_effect : behavior_implementation<common>
    {
        F32 predict_time;

        virtual bool system_event(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                                  xBase* toParamWidget, U32 toParamWidgetID);
        virtual void update(F32 dt);
        virtual void setup();
        virtual void reset();
        virtual bool runnable(F32) { return xEntIsVisible(owner) != 0; }
    };

    struct frozone : player
    {
        idle idle_state;
        health_hud hud_state;
        ice_effect ice_state;

        frozone();
        virtual void setup();
        virtual const char* get_type_name() const { return "frozone"; }
        inline xVec3 predict(F32 time) const;
    };
} // namespace zNPC

#endif

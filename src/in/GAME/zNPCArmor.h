#ifndef ZNPCARMOR_H
#define ZNPCARMOR_H

#include "zNPCCommon.h"
#include "xSndMgr.h"

struct zShrapnelAsset;

namespace zNPC
{
    class Armor : public behavior_implementation<common>
    {
    public:
        bool isEnabled;
        bool isFinished;
        bool isExclusive;
        bool startFallBack;
        S32 armorHitPointsInitial;
        S32 armorHitPoints;
        S32 numArmor;
        S32 numArmorRemaining;
        S32 finishCount;
        xSndGroupHandle breakingArmorSound;
        struct
        {
            S32 bone;
            xModelInstance* modelInstance;
            xMat4x3 mat;
            zShrapnelAsset* shrapnelAsset;
            bool isActive;
        } armorInstance[32];
        F32 length;
        xModelTag tag;

        static S32 numArmorNPC;

        static void AddArmor(common* owner);

        virtual const char* getName() { return "Armor"; }
        virtual bool system_event(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                                  xBase* toParamWidget, U32 toParamWidgetID);
        virtual bool damage(zCombatDamageInfo& damageInfo);
        virtual void add_states(xAnimTable* table);
        virtual void add_transitions(xAnimTable* table);
        virtual void update(F32 dt);
        virtual void setup();
        virtual void init();
        virtual void reset();
        virtual void render();
        virtual void render_extra();
        virtual bool exclusive() { return isExclusive; }
        virtual bool runnable(F32 dt);

        void _add_states(xAnimTable* table);
        void _add_transitions(xAnimTable* table);
    };
} // namespace zNPC

#endif

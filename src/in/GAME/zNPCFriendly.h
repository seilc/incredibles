#ifndef ZNPCFRIENDLY_H
#define ZNPCFRIENDLY_H

#include "zNPCCommon.h"
#include "xSnd.h"

struct ztaskbox;

namespace zNPC
{
    struct speak : npc_move
    {
        U8 draw_talk_symbol;
        U8 speaking;
        U8 talkative;
        F32 tmr_cycleAnim;
        F32 tmr_minTalk;
        xSndGroupHandle soundgroup;
        iSndHandle soundhandle;
        void* jawdata;
        F32 jawtime;
        ztaskbox* task;
        F32 max_talk_distance_2;
        F32 speak_distance_2;

        speak(); // UNUSED

        virtual const char* getName() { return "speak"; }
        virtual void add_states(xAnimTable* table); // UNUSED
    };

    struct idle : behavior_implementation<common>
    {
        virtual const char* getName() { return "idle"; }
        virtual void add_states(xAnimTable* table);
        virtual bool runnable(F32) { return true; }
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
    };

    struct buddy_anim : behavior_implementation<common>
    {
        virtual const char* getName() { return "buddy_anim"; }
        virtual bool system_event(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                                  xBase* toParamWidget, U32 toParamWidgetID);
        virtual void add_states(xAnimTable* table);
        virtual void add_transitions(xAnimTable* table);
        virtual bool runnable(F32) { return true; }
        virtual void enter_state(const behavior_implementation<common>* prev_behavior);
    };

    struct friendly : common
    {
        buddy_anim anim_state;
        F32 jet_offset;
        F32 left_emit_remaining;
        F32 right_emit_remaining;
        U8 unk20C[12];
        S32 left_jet_bone;
        S32 right_jet_bone;

        friendly();
        virtual void update_npc(F32 dt);
        virtual void setup();
        virtual const char* get_type_name() const { return "buddy_pine"; }
        virtual void reset();
    };
} // namespace zNPC

#endif

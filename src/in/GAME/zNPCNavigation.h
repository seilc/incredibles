#ifndef ZNPCNAVIGATION_H
#define ZNPCNAVIGATION_H

#include "zNPCNavigationMesh.h"
#include "zNPCBehavior.h"

namespace zNPC
{
    struct common;

    struct jump
    {
        bool active;
        F32 height;
        F32 speed;
        F32 gravity;
        F32 velocity;
        F32 time;
        F32 total_time;
        F32 time_up;
        xVec3 target;
        xVec3 source;
        xVec3 position;
    };

    struct navigate
    {
        F32 stuck_timer;
        bool dpos_apply;
        bool reported_stuck;
        bool last_stuck;
        bool stuck;
        bool arrived;
        bool destination_off_mesh;
        navigation_mesh* full_mesh;
        S32 sub_mesh_index;
        S32 current_triangle;
        circle* circle_root;
        common* npc;
        circle my_circle;
        jump current_jump;
        F32 jump_gravity;
        F32 jump_height;
        xVec3* last_destination;
        bool move_away;

        void reset();
        void collide(F32 dt);
        void apply_dpos(F32 dt);
        void remove();
        void update_stuck(F32 dt);
    };

    struct npc_move : move
    {
        xVec3 last_destination;
        F32 cos_min_move_angle;

        virtual const char* getName() { return "npc_move"; }
        virtual bool system_event(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                                  xBase* toParamWidget, U32 toParamWidgetID);
        virtual void setup();
        virtual void jump_to(xVec3& destination, F32 dt, bool face_dir);
        virtual void move_to(const xVec3& destination, F32 dt, bool move_away, bool face_dir,
                             bool change_anim_speed);
        virtual void move_away_from(const xVec3& point, F32 dt);
        virtual bool physics_move(F32 dt);
        virtual void jump_update(F32 dt, bool face_jump_dir);
        virtual void step_move(const xVec3& direction, F32 distance, F32 dt);
        virtual bool have_arrived() const;
    };

} // namespace zNPC

#endif
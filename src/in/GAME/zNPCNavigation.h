#ifndef ZNPCNAVIGATION_H
#define ZNPCNAVIGATION_H

#include "zNPCNavigationMesh.h"

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

} // namespace zNPC

#endif
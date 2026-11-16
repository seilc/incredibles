#ifndef ZNPCNAVIGATIONMESHDRIVEN_H
#define ZNPCNAVIGATIONMESHDRIVEN_H

#include "xMath3.h"

namespace zNPC
{
    struct circle;
    struct navigation_mesh;

    struct vertex_group
    {
        S32 num_vertices;
        xVec3* local_space_verts;
        xVec3** world_space_verts;
        xMat4x3 to_world;
    };

    struct navigation_mesh_driven
    {
        navigation_mesh* full_mesh;
        S32 sub_mesh_index;
        U8 vert_to_group_map[254];
        S32 num_groups;
        vertex_group* groups;

        void Init(navigation_mesh* full_mesh, S32 sub_mesh_index);
        static void reset();
        static navigation_mesh_driven* Get(navigation_mesh* full_mesh, S32 sub_mesh_index);
        void Init(S32 group_index, const xMat4x3& local_space_origin);
        void UpdateGroup(S32 group_num, const xMat4x3* to_world);
    };

    void get_triangle_uv_pos(F32& u, F32& v, const xVec3* v0, const xVec3* v1, const xVec3* v2, const xVec3& pos);
    void get_center_with_uv(F32 u, F32 v, const xVec3* v0, const xVec3* v1, const xVec3* v2, xVec3& pos);
    void move_npc_circle(circle* npc_circle, const xVec3& circle_pos, const xVec3& entity_pos);
} // namespace zNPC

#endif

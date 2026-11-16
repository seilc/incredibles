#include "xCommon.h"
#include "zNPCNavigationMeshDriven.h"

#include "zNPCNavigationMesh.h"
#include "xMemory.h"

#include <string.h>

#define MAX_DEFORMED_TRI_CIRCLES 10

namespace zNPC
{
    struct circle_location
    {
        circle* npc_circle;
        S32 index;
        F32 circle_u;
        F32 circle_v;
        F32 entity_u;
        F32 entity_v;
    };

    static navigation_mesh_driven* driven_meshes[2];
    static S32 num_driven_meshes;

    void navigation_mesh_driven::Init(navigation_mesh* full_mesh, S32 sub_mesh_index)
    {
        this->full_mesh = full_mesh;
        this->sub_mesh_index = sub_mesh_index;

        xAsset::navigation_mesh::sub_mesh* sub_mesh = &full_mesh->GetAsset()->sub_meshes[sub_mesh_index];

        S32 max_group = 0;
        for (S32 i = 0; i < sub_mesh->num_triangles; i++) {
            S32 group_index = xAsset::GET_GROUP_INDEX(sub_mesh->triangles[i].flags);
            if (group_index > max_group) {
                max_group = group_index;
            }
        }

        num_groups = max_group + 1;
        groups = (vertex_group*)xMEMALLOC(num_groups * sizeof(vertex_group), 0);
        memset(groups, 0, num_groups * sizeof(vertex_group));
        memset(vert_to_group_map, 0, sizeof(vert_to_group_map));

        for (S32 i = 0; i < sub_mesh->num_triangles; i++) {
            S32 group_index = xAsset::GET_GROUP_INDEX(sub_mesh->triangles[i].flags);
            if (group_index != 0) {
                vert_to_group_map[sub_mesh->triangles[i].a] = group_index;
                vert_to_group_map[sub_mesh->triangles[i].b] = group_index;
                vert_to_group_map[sub_mesh->triangles[i].c] = group_index;
            }
        }

        for (S32 i = 0; i < sub_mesh->num_vertices; i++) {
            groups[vert_to_group_map[i]].num_vertices++;
        }

        for (S32 i = 0; i < num_groups; i++) {
            if (groups[i].num_vertices >= 1) {
                groups[i].local_space_verts = (xVec3*)xMEMALLOC(groups[i].num_vertices * sizeof(xVec3), 0);
                groups[i].world_space_verts = (xVec3**)xMEMALLOC(groups[i].num_vertices * sizeof(xVec3*), 0);

                S32 num_verts = 0;
                for (S32 j = 0; j < sub_mesh->num_vertices; j++) {
                    if (vert_to_group_map[j] == i) {
                        groups[i].world_space_verts[num_verts++] = &sub_mesh->vertices[j];
                    }
                }
            }
        }
    }

    void navigation_mesh_driven::reset()
    {
        num_driven_meshes = 0;
    }

    navigation_mesh_driven* navigation_mesh_driven::Get(navigation_mesh* full_mesh, S32 sub_mesh_index)
    {
        for (S32 i = 0; i < num_driven_meshes; i++) {
            if (driven_meshes[i]->full_mesh == full_mesh && driven_meshes[i]->sub_mesh_index == sub_mesh_index) {
                return driven_meshes[i];
            }
        }

        navigation_mesh_driven* driven = (navigation_mesh_driven*)xMEMALLOC(sizeof(navigation_mesh_driven), 0);
        driven->Init(full_mesh, sub_mesh_index);
        driven_meshes[num_driven_meshes] = driven;
        num_driven_meshes++;
        return driven;
    }

    void navigation_mesh_driven::Init(S32 group_index, const xMat4x3& local_space_origin)
    {
        if (group_index < 0 || group_index >= num_groups) {
            return;
        }

        for (S32 i = 0; i < groups[group_index].num_vertices; i++) {
            xMat4x3Tolocal(&groups[group_index].local_space_verts[i], &local_space_origin,
                           groups[group_index].world_space_verts[i]);
        }

        groups[group_index].to_world = local_space_origin;
    }

    void get_triangle_uv_pos(F32& u, F32& v, const xVec3* v0, const xVec3* v1, const xVec3* v2, const xVec3& pos)
    {
        F32 ABx = v1->x - v0->x, ABz = v1->z - v0->z, ACx = v2->x - v0->x, ACz = v2->z - v0->z,
            inner = ABx * ACz - ACx * ABz;

        if (inner < 0.0001f && inner > -0.0001f) {
            u = v = 0.3333f;
            return;
        }

        F32 APx = pos.x - v0->x, APz = pos.z - v0->z;

        if (ABx > 0.0001f) {
            if (ABz > 0.0001f) {
                if (ABx > ABz) {
                    v = (APz * ABx / ABz - APx) / (ACz * ABx / ABz - ACx);
                    u = (APx - v * ACx) / ABx;
                } else {
                    v = (APx * ABz / ABx - APz) / (ACx * ABz / ABx - ACz);
                    u = (APz - v * ACz) / ABz;
                }
            } else if (ABz < -0.0001f) {
                if (ABx > -ABz) {
                    v = (APz * ABx / ABz - APx) / (ACz * ABx / ABz - ACx);
                    u = (APx - v * ACx) / ABx;
                } else {
                    v = (APx * ABz / ABx - APz) / (ACx * ABz / ABx - ACz);
                    u = (APz - v * ACz) / ABz;
                }
            } else {
                v = APz / ACz;
                u = (APx - v * ACx) / ABx;
            }
        } else if (ABx < -0.0001f) {
            if (ABz > 0.0001f) {
                if (-ABx > ABz) {
                    v = (APz * ABx / ABz - APx) / (ACz * ABx / ABz - ACx);
                    u = (APx - v * ACx) / ABx;
                } else {
                    v = (APx * ABz / ABx - APz) / (ACx * ABz / ABx - ACz);
                    u = (APz - v * ACz) / ABz;
                }
            } else if (ABz < -0.0001f) {
                if (ABx < ABz) {
                    v = (APz * ABx / ABz - APx) / (ACz * ABx / ABz - ACx);
                    u = (APx - v * ACx) / ABx;
                } else {
                    v = (APx * ABz / ABx - APz) / (ACx * ABz / ABx - ACz);
                    u = (APz - v * ACz) / ABz;
                }
            } else {
                v = APz / ACz;
                u = (APx - v * ACx) / ABx;
            }
        } else if (ABz > 0.0001f || ABz < -0.0001f) {
            v = APx / ACx;
            u = (APz - v * ACz) / ABz;
        } else {
            u = v = 0.3333f;
        }
    }

    void get_center_with_uv(F32 u, F32 v, const xVec3* v0, const xVec3* v1, const xVec3* v2, xVec3& pos)
    {
        xVec3 AB;
        xVec3Sub(&AB, v1, v0);
        xVec3 BC;
        // BUG: v2 - v1, get_triangle_uv_pos solves for v2 - v0
        xVec3Sub(&BC, v2, v1);

        pos.x = v0->x + u * AB.x + v * BC.x;
        pos.y = v0->y + u * AB.y + v * BC.y;
        pos.z = v0->z + u * AB.z + v * BC.z;
    }

    void move_npc_circle(circle* npc_circle, const xVec3& circle_pos, const xVec3& entity_pos)
    {
        npc_circle->set_center_same_tri(circle_pos);
        (xVec3&)npc_circle->entity->model->Mat->pos = entity_pos;
        npc_circle->entity->frame->mat.pos.x = entity_pos.x;
        npc_circle->entity->frame->mat.pos.y = entity_pos.y;
        npc_circle->entity->frame->mat.pos.z = entity_pos.z;
    }

    void navigation_mesh_driven::UpdateGroup(S32 group_num, const xMat4x3* to_world)
    {
        if (group_num < 0 || group_num >= num_groups) {
            return;
        }

        circle_location deformed_tri_circles[MAX_DEFORMED_TRI_CIRCLES];
        S32 num_deformed_tri_circles = 0;
        xAsset::navigation_mesh::sub_mesh* sub_mesh = &full_mesh->GetAsset()->sub_meshes[sub_mesh_index];

        for (circle* npc_circle = full_mesh->get_first_circle(); npc_circle != NULL;
             npc_circle = full_mesh->get_next_circle(npc_circle)) {
            if (npc_circle->sub_mesh_index == sub_mesh_index) {
                S32 tri_index = npc_circle->tri_index;
                if (vert_to_group_map[sub_mesh->triangles[tri_index].a] == group_num
                        ? (vert_to_group_map[sub_mesh->triangles[tri_index].b] == group_num
                               ? vert_to_group_map[sub_mesh->triangles[tri_index].c] != group_num
                               : true)
                        : (vert_to_group_map[sub_mesh->triangles[tri_index].b] == group_num
                               ? true
                               : vert_to_group_map[sub_mesh->triangles[tri_index].c] == group_num)) {
                    if (num_deformed_tri_circles >= MAX_DEFORMED_TRI_CIRCLES) {
                        break;
                    }

                    deformed_tri_circles[num_deformed_tri_circles].npc_circle = npc_circle;
                    deformed_tri_circles[num_deformed_tri_circles].index = tri_index;

                    const xVec3 *v0, *v1, *v2;
                    full_mesh->get_triangle_verts(sub_mesh_index, tri_index, v0, v1, v2);
                    get_triangle_uv_pos(deformed_tri_circles[num_deformed_tri_circles].circle_u,
                                        deformed_tri_circles[num_deformed_tri_circles].circle_v, v0, v1, v2,
                                        *npc_circle->get_center());
                    get_triangle_uv_pos(deformed_tri_circles[num_deformed_tri_circles].entity_u,
                                        deformed_tri_circles[num_deformed_tri_circles].entity_v, v0, v1, v2,
                                        (xVec3&)npc_circle->entity->model->Mat->pos);
                    num_deformed_tri_circles++;
                }
            }
        }

        for (S32 i = 0; i < groups[group_num].num_vertices; i++) {
            xMat4x3Toworld(groups[group_num].world_space_verts[i], to_world,
                           &groups[group_num].local_space_verts[i]);
        }

        for (S32 i = 0; i < num_deformed_tri_circles; i++) {
            const xVec3 *v0, *v1, *v2;
            full_mesh->get_triangle_verts(sub_mesh_index, deformed_tri_circles[i].index, v0, v1, v2);

            xVec3 circle_pos;
            get_center_with_uv(deformed_tri_circles[i].circle_u, deformed_tri_circles[i].circle_v, v0, v1, v2,
                               circle_pos);
            xVec3 entity_pos;
            get_center_with_uv(deformed_tri_circles[i].entity_u, deformed_tri_circles[i].entity_v, v0, v1, v2,
                               entity_pos);
            move_npc_circle(deformed_tri_circles[i].npc_circle, circle_pos, entity_pos);
        }

        for (circle* npc_circle = full_mesh->get_first_circle(); npc_circle != NULL;
             npc_circle = full_mesh->get_next_circle(npc_circle)) {
            if (npc_circle->sub_mesh_index == sub_mesh_index) {
                if (vert_to_group_map[sub_mesh->triangles[npc_circle->tri_index].a] == group_num &&
                    vert_to_group_map[sub_mesh->triangles[npc_circle->tri_index].b] == group_num &&
                    vert_to_group_map[sub_mesh->triangles[npc_circle->tri_index].c] == group_num) {
                    xVec3 circle_local_space_pos;
                    xMat4x3Tolocal(&circle_local_space_pos, &groups[group_num].to_world, npc_circle->get_center());
                    xVec3 entity_local_space_pos;
                    xMat4x3Tolocal(&entity_local_space_pos, &groups[group_num].to_world,
                                   (xVec3*)&npc_circle->entity->model->Mat->pos);
                    xVec3 circle_world_pos;
                    xMat4x3Toworld(&circle_world_pos, to_world, &circle_local_space_pos);
                    xVec3 entity_world_pos;
                    xMat4x3Toworld(&entity_world_pos, to_world, &entity_local_space_pos);
                    move_npc_circle(npc_circle, circle_world_pos, entity_world_pos);
                }
            }
        }

        xMemoryCopyUpA128(&groups[group_num].to_world, to_world, sizeof(xMat4x3));
    }
} // namespace zNPC

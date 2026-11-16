#ifndef ZNPCNAVIGATIONMESH_H
#define ZNPCNAVIGATIONMESH_H

#include "xNavigationMeshAsset.h"
#include "xEnt.h"

namespace zNPC
{
    struct circle
    {
        circle* base_next;
        F32 base_radius;
        xVec3 center;
        xVec3 velocity;
        circle* next;
        xEnt* entity;
        F32 radius;
        F32 radius_2;
        S32 sub_mesh_index;
        U8 tri_index;

        const xVec3* get_center() const;
        void set_center_same_tri(const xVec3& c);
    };

    struct navigation_mesh
    {
        xAsset::navigation_mesh* asset;
        U8 registered_objects;
        circle* circle_list;

        xAsset::navigation_mesh* GetAsset();
        circle* get_first_circle();
        circle* get_next_circle(circle* c);
        void get_triangle_verts(S32 sub_mesh_index, S32 tri_index, const xVec3*& v0, const xVec3*& v1,
                                const xVec3*& v2);
    };

    inline const xVec3* circle::get_center() const
    {
        return &center;
    }

    inline void circle::set_center_same_tri(const xVec3& c)
    {
        center = c;
    }

    inline xAsset::navigation_mesh* navigation_mesh::GetAsset()
    {
        return asset;
    }

    inline circle* navigation_mesh::get_first_circle()
    {
        return circle_list;
    }

    inline circle* navigation_mesh::get_next_circle(circle* c)
    {
        return c->base_next;
    }

    inline void navigation_mesh::get_triangle_verts(S32 sub_mesh_index, S32 tri_index, const xVec3*& v0,
                                                    const xVec3*& v1, const xVec3*& v2)
    {
        xAsset::navigation_mesh::sub_mesh* sub_mesh = &asset->sub_meshes[sub_mesh_index];
        v0 = &sub_mesh->vertices[sub_mesh->triangles[tri_index].a];
        v1 = &sub_mesh->vertices[sub_mesh->triangles[tri_index].b];
        v2 = &sub_mesh->vertices[sub_mesh->triangles[tri_index].c];
    }
} // namespace zNPC

#endif

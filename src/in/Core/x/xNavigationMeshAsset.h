#ifndef XNAVIGATIONMESHASSET_H
#define XNAVIGATIONMESHASSET_H

#include "xBaseAsset.h"
#include "xMath3.h"

enum { NO_TRIANGLE = 0xFF };

namespace xAsset
{
    class navigation_mesh : public xBaseAsset
    {
    public:
        class sub_mesh
        {
        public:
            struct exit
            {
                S32 exit_triangle_index;
                S32 dest_triangle_index;
                S32 neighbor_mesh_index;
            };

            struct triangle
            {
                U8 a;
                U8 b;
                U8 c;
                U8 flags;
            };

            U8* portal_lookup;
            U8* portal;
            F32* edge_shift;
            exit* exits;
            S32 num_exits;
            xVec3* vertices;
            S32 num_vertices;
            triangle* triangles;
            S32 num_triangles;
            U32* objects;
            S32 num_objects;
            U8* level_two_route_exits;

            S32 get_triangle(const xVec3& point, S32 current_triangle) const;
            bool in_triangle(S32 triangle_index, const xVec3& point) const;
            S32 lookup_next(S32 source, S32 destination) const;

            F32 get_height(S32 triangle, F32 x, F32 z) const;
        };

        S32 num_sub_meshes;
        sub_mesh* sub_meshes;

        void get_triangle(const xVec3& position, S32& sub_mesh_index, S32& tri_index);
    };

    inline F32 navigation_mesh::sub_mesh::get_height(S32 triangle, F32 x, F32 z) const
    {
        if (triangle == NO_TRIANGLE) {
            return 99999.0f;
        }

        xVec3* a = &vertices[triangles[triangle].a];
        xVec3* b = &vertices[triangles[triangle].b];
        xVec3* c = &vertices[triangles[triangle].c];
        F32 Ax = a->x;
        F32 Az = a->z;
        F32 ABx = b->x - Ax;
        F32 ABz = b->z - Az;
        F32 ACx = c->x - Ax;
        F32 ACz = c->z - Az;
        F32 Ny = ABz * ACx - ACz * ABx;

        if (!Ny) {
            return 0.0f;
        }

        F32 iNy = 1.0f / Ny;
        F32 Ay = a->y;
        F32 ABy = b->y - Ay;
        F32 ACy = c->y - Ay;
        F32 Nx = ABy * ACz - ACy * ABz;
        F32 Nz = ABx * ACy - ACx * ABy;

        return (Nx * (Ax - x) + Ay * Ny + Nz * (Az - z)) * iNy;
    }
} // namespace xAsset

#endif

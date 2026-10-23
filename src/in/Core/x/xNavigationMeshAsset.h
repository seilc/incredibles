#ifndef XNAVIGATIONMESHASSET_H
#define XNAVIGATIONMESHASSET_H

#include "xBase.h"
#include "xMath3.h"

namespace xAsset
{
    struct navigation_mesh : xBaseAsset
    {
        struct sub_mesh
        {
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
        };

        S32 num_sub_meshes;
        sub_mesh* sub_meshes;
    };
} // namespace xAsset

#endif
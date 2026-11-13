#include "xNavigationMeshAsset.h"

namespace xAsset
{
    S32 navigation_mesh::sub_mesh::get_triangle(const xVec3& point, S32 current_triangle) const
    {
        if (current_triangle != NO_TRIANGLE) {
            if (in_triangle(current_triangle, point)) {
                F32 triangleHeight = get_height(current_triangle, point.x, point.z);
                if (point.y + 3.0f > triangleHeight && point.y - 3.0f < triangleHeight) {
                    return current_triangle;
                }
            }
            for (S32 i = 0; i < 3; i++) {
                S32 triangle = portal[current_triangle*3+i];
                if (triangle != NO_TRIANGLE)
                    if (in_triangle(portal[current_triangle*3+i], point)) {
                        F32 triangleHeight = get_height(triangle, point.x, point.z);
                        if (point.y + 3.0f > triangleHeight && point.y - 3.0f < triangleHeight) {
                            return triangle;
                        }
                    }
            }
        }

        for (S32 i = 0; i < num_triangles; i++) {
            if (in_triangle(i, point)) {
                F32 triangleHeight = get_height(i, point.x, point.z);
                if (point.y + 3.0f > triangleHeight && point.y - 3.0f < triangleHeight) {
                    return i;
                }
            }
        }

        return NO_TRIANGLE;
    }

    bool navigation_mesh::sub_mesh::in_triangle(S32 triangle_index, const xVec3& point) const
    {
        xVec3* v[3];
        v[0] = &vertices[triangles[triangle_index].a];
        v[1] = &vertices[triangles[triangle_index].b];
        v[2] = &vertices[triangles[triangle_index].c];

        if (((point.x - v[0]->x) * (v[1]->z - v[0]->z) + (point.z - v[0]->z) * (v[0]->x - v[1]->x)) < -0.1f) {
            return false;
        }

        if (((point.x - v[1]->x) * (v[2]->z - v[1]->z) + (point.z - v[1]->z) * (v[1]->x - v[2]->x)) < -0.1f) {
            return false;
        }

        if (((point.x - v[2]->x) * (v[0]->z - v[2]->z) + (point.z - v[2]->z) * (v[2]->x - v[0]->x)) < -0.1f) {
            return false;
        }

        return true;
    }

    S32 navigation_mesh::sub_mesh::lookup_next(S32 source, S32 destination) const
    {
        U32 bitIndex = ((destination * num_triangles + source) << 1);
        U32 byteIndex = (bitIndex >> 3);
        U32 bitShift = (bitIndex & 0x7);

        return (U8)((portal_lookup[byteIndex] >> bitShift) & 0x3);
    }

    void navigation_mesh::get_triangle(const xVec3& position, S32& sub_mesh_index, S32& tri_index)
    {
        for (S32 m = 0; m < num_sub_meshes; m++) {
            tri_index = sub_meshes[m].get_triangle(position, tri_index);
            if (tri_index != NO_TRIANGLE) {
                sub_mesh_index = m;
                return;
            }
        }

        tri_index = NO_TRIANGLE;
        sub_mesh_index = 0;
    }
} // namespace xAsset

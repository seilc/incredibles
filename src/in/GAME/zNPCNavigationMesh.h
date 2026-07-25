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
    };

    struct navigation_mesh
    {
        navigation_mesh* asset;
        U8 registered_objects;
        circle* circle_list;
    };

} // namespace zNPC

#endif
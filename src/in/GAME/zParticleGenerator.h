#ifndef ZPARTICLEGENERATOR_H
#define ZPARTICLEGENERATOR_H

#include "xDynAsset.h"
#include "xMath3.h"

struct zParticleAsset : xDynAsset
{
    struct attach_fixed_data
    {
    };

    struct attach_entity_data
    {
        U32 entity;
        U8 bone;
        U8 pad1;
        U8 pad2;
        U8 pad3;
    };

    struct attach_entity_tag_data
    {
        U32 entity;
        xVec3 tag;
    };

    struct motion_none_data
    {
    };

    struct motion_spiral_data
    {
        U8 flags;
        U8 points;
        U8 pad1;
        U8 pad2;
        F32 radius_inner;
        F32 radius_outer;
        F32 duration;
        F32 frequency;
    };

    struct volume_point_data
    {
    };

    struct volume_sphere_data
    {
        F32 radius;
    };

    struct volume_circle_data
    {
        F32 radius;
        F32 arc_length;
    };

    struct volume_line_data
    {
        U8 flags;
        U8 pad1;
        U8 pad2;
        U8 pad3;
        F32 radius;
        F32 length;
    };

    struct volume_model_data
    {
        U8 flags;
        U8 exclude;
        U8 pad1;
        U8 pad2;
        F32 expand;
    };

    union attach_data
    {
        attach_fixed_data fixed;
        attach_entity_data entity;
        attach_entity_tag_data entity_tag;
    };

    union motion_data
    {
        motion_none_data none;
        motion_spiral_data spiral;
    };

    union volume_data
    {
        volume_point_data point;
        volume_sphere_data sphere;
        volume_circle_data circle;
        volume_line_data line;
        volume_model_data model;
    };

    U8 flags;
    U8 attach_flags;
    U8 motion_flags;
    U8 volume_flags;
    F32 rate;
    U32 texture;
    U8 attach_type;
    U8 motion_type;
    U8 volume_type;
    U8 system_type;
    xVec3 location;
    struct
    {
        F32 yaw;
        F32 pitch;
        F32 roll;
    } axis;
    attach_data attach;
    motion_data motion;
    volume_data volume;
};

struct zParticleGenerator : xBase
{
    zParticleAsset* asset;
    S32 flags;
};

#endif
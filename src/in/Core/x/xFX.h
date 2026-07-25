#ifndef XFX_H
#define XFX_H

#include "containers.h"
#include "xColor.h"

#include <rwcore.h>
#include <rpworld.h>

struct xFXRibbon
{
    struct activity_data
    {
        xFXRibbon* owner;
        activity_data** position;
        tier_queue joints;
        S32 curve_index;
        F32 ilife;
        U32 mtime;
        U32 mlife;
    };

    struct curve_node
    {
        F32 time;
        xColor color;
        F32 scale;
    };

    struct config_type
    {
        F32 life_time;
        U32 blend_src;
        U32 blend_dst;
        F32 pivot;
        S32 flags;
        curve_node* curve;
        S32 curve_size;
        RwRaster* raster;
    };

    F32 texture_offset;
    F32 texture_increment_scale;
    activity_data* act;
    config_type* cfg;
};

void xFX_SceneEnter(RpWorld* world);
void xFX_SceneExit(RpWorld*);
void xFXUpdate(F32 dt);
void xFXanimUV2PSetTexture(RwTexture* texture);

#endif
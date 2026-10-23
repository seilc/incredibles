#ifndef XCAMSUPPORT_H
#define XCAMSUPPORT_H

#include "xMath3.h"

struct xCamCoordPolar
{
    xVec3 origin;
    F32 theta;
    F32 phi;
    F32 dist;
};

struct xCamSupportPath
{
    struct config
    {
        F32 min_segment_dist;
        F32 smooth_factor;
        S32 smooth_passes;
        F32 subdiv_thresh;
    };

    struct path_node
    {
        xVec3 loc;
    };

    config cfg;
    path_node* path;
    S32 power;
    S32 max_size;
    S32 front;
    S32 used;
    F32 dist;
    F32 subdist;
    F32 weights[31];
};

struct xCamSupportLOSBar
{
    struct config
    {
        F32 rail_ymin;
        F32 rail_ymax;
        F32 pivot_bar_radius;
        F32 compress_bar_radius;
        F32 compress_ball_radius;
        F32 compress_dist_min;
    };

    config cfg;
    xVec3 origin;
    xVec3 last_origin;
    xVec3 stern_loc;
    xVec3 last_stern_loc;
    F32 rail_yoffset;
    F32 last_rail_yoffset;
};

#endif
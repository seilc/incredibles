#ifndef XENTDRIVE_H
#define XENTDRIVE_H

#include "xEnt.h"

struct xEntDrive
{
    struct tri_data : xCollis::tri_data
    {
        xVec3 loc;
        F32 yaw;
        xCollis* coll;
        xMat4x3 trioldmat;
    };

    U32 flags;
    F32 otm;
    F32 otmr;
    F32 os;
    F32 tm;
    F32 tmr;
    F32 s;
    xEnt* old_driver;
    xEnt* driver;
    xEnt* driven;
    xVec3 driven_pos_in_old_driver;
    xVec3 driven_pos_in_driver;
    xVec3 last_driven_pos;
    F32 yaw_in_driver;
    xVec3 dloc;
    tri_data tri;
};

#endif
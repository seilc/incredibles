#ifndef ZSLIDECAMERA_H
#define ZSLIDECAMERA_H

#include "xCam.h"

struct zSlideCam : xCam
{
    zSlideCam() {}

    void create();
    void load_cfg();
    void start();
    void setCurrTgtPos(const xVec3& pos, const xVec3& up);
    void setPrevTgtPos(const xVec3& pos, const xVec3& up);
    void update(xScene&, F32 dt);

    const xMat4x3* target;
    struct
    {
        F32 spring_len;
        F32 spring_const;
        F32 damp_const;
        F32 vertical_offset;
    } cfg;
    xVec3 curr_tgt_pos;
    xVec3 prev_tgt_pos;
    xVec3 cam_pos;
};

#endif

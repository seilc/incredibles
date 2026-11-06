#include "zSlideCamera.h"

void zSlideCam::create()
{
    xCam::create();

    coord_type = XCAM_COORD_CART;
    orient_type = XCAM_ORIENT_QUAT;
    cfg_common.priority = 128;
    cfg_common.blend_time = 1.0f;

    load_cfg();
}

void zSlideCam::load_cfg()
{
    cfg.spring_len = 2.5f;
    cfg.spring_const = 3.5f;
    cfg.damp_const = 3.2f;
    cfg.vertical_offset = 2.0f;
}

void zSlideCam::start()
{
    xCam::start();

    prev_tgt_pos = curr_tgt_pos;
    cam_pos = curr_tgt_pos;
}

static xVec3 SpringDamp(const xVec3& cam_pos, const xVec3& curr_tgt_pos, const xVec3& prev_tgt_pos,
                        F32 dt, F32 spring_const, F32 damp_const, F32 spring_len)
{
    xVec3 disp, velocity;

    disp = cam_pos - curr_tgt_pos;
    velocity = (prev_tgt_pos - curr_tgt_pos) * dt;

    F32 forceMag, len = xVec3Length(&disp), dot = xVec3Dot(&disp, &velocity);

    xVec3Normalize(&disp, &disp);

    forceMag = spring_const * (spring_len - len) + damp_const * (dot / len);

    disp *= forceMag * dt;

    xVec3 r;

    xVec3Add(&r, &cam_pos, &disp);

    return r;
}

void zSlideCam::setCurrTgtPos(const xVec3& pos, const xVec3& up)
{
    curr_tgt_pos = pos;
    curr_tgt_pos += up * cfg.vertical_offset;
}

void zSlideCam::setPrevTgtPos(const xVec3& pos, const xVec3& up)
{
    prev_tgt_pos = pos;
    prev_tgt_pos += up * cfg.vertical_offset;
}

void zSlideCam::update(xScene&, F32 dt)
{
    setCurrTgtPos(target->pos, target->up);

    if (xVec3Length(curr_tgt_pos.x - prev_tgt_pos.x, curr_tgt_pos.y - prev_tgt_pos.y,
                    curr_tgt_pos.z - prev_tgt_pos.z) > 0.001f) {
        cam_pos = SpringDamp(cam_pos, curr_tgt_pos, prev_tgt_pos, dt, cfg.spring_const,
                             cfg.damp_const, cfg.spring_len);
    }

    const xVec3 y_axis = { 0.0f, 1.0f, 0.0f };

    mat.up = y_axis;
    mat.at = prev_tgt_pos - cam_pos;

    xVec3Normalize(&mat.at, &mat.at);

    xVec3Cross(&mat.right, &mat.up, &mat.at);
    xVec3Normalize(&mat.right, &mat.right);

    xVec3Cross(&mat.up, &mat.at, &mat.right);
    xVec3Normalize(&mat.up, &mat.up);

    coord.cart = cam_pos;
    mat.pos = coord.cart;

    xQuatFromMat(&orient.quat, &mat);
}
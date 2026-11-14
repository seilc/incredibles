#include "xCommon.h"
#include "zCamFP.h"

#include "xEnt.h"
#include "xGlobals.h"
#include "xModel.h"
#include "zMain.h"
#include "zPlayer.h"

void zCamFP::CopyPosFromPlayer(xEnt* ent)
{
    xMat4x3 mat;
    xMat4x3Mul(&mat, (xMat4x3*)&ent->model->Mat[boneIndex], (xMat4x3*)ent->model->Mat);
    xMat3x3RMulVec(&coord.cart, &mat, &boneOffset);
    xVec3AddTo(&coord.cart, &mat.pos);
}

void zCamFP::CopyYawFromPlayer(xEnt* ent)
{
    orient.euler.yaw = xatan2(ent->model->Mat->at.x, ent->model->Mat->at.z);
}

void zCamFP::CopyToPlayer(xEnt* ent)
{
    xVec3Copy(&ent->frame->mat.right, &mat.right);
    xVec3Init(&ent->frame->mat.up, 0.0f, 1.0f, 0.0f);
    xVec3Cross(&ent->frame->mat.at, &ent->frame->mat.right, &ent->frame->mat.up);
    xMat3x3Copy((xMat3x3*)ent->model->Mat, &ent->frame->mat);
}

void zCamFP::create()
{
    xCam::create();
    coord_type = XCAM_COORD_CART;
    orient_type = XCAM_ORIENT_EULER;
    cfg_common.priority = 0x81;
}

void zCamFP::start()
{
    xCam::start();
    CopyPosFromPlayer(ZPLAYER);
    CopyYawFromPlayer(ZPLAYER);
    orient.euler.pitch = 0.0f;
    cfg_common.priority = 0x81;
}

void zCamFP::stop()
{
    xCam::stop();
    CopyToPlayer(ZPLAYER);
}

void zCamFP::update(xScene&, F32 dt)
{
    CopyPosFromPlayer(ZPLAYER);

    if (fpFlags & 0x1) {
        CopyYawFromPlayer(ZPLAYER);
    }

    orient.euler.yaw -= yawSpeed * analog.offset.x * dt;
    if (orient.euler.yaw > PI) {
        orient.euler.yaw -= 2.0f * PI;
    } else if (orient.euler.yaw < -PI) {
        orient.euler.yaw += 2.0f * PI;
    }

    orient.euler.pitch -= pitchSpeed * analog.offset.y * dt;
    if (orient.euler.pitch < minPitch) {
        orient.euler.pitch = minPitch;
    } else if (orient.euler.pitch > maxPitch) {
        orient.euler.pitch = maxPitch;
    }

    refresh_mat();
}

#ifndef XCAMFOLLOW_H
#define XCAMFOLLOW_H

#include "xCam.h"
#include "xMath2.h"

struct xScene;

struct xCamFollow : xCam
{
    struct config : xCamConfigFollow
    {
        F32 speed_move;
        xVec2 stick_vel;
        xVec2 stick_accel;
        F32 stick_speed;

        struct
        {
            F32 width;
            F32 dist_min;
        } collide;

        struct
        {
            F32 speed;
            F32 dist;
            F32 dist_min;
            F32 delay;
            F32 fade_time;
            F32 vel_min;
            F32 hang_min;
            F32 hang_max;
        } vel_drift;
    };

    config cfg;

    virtual void start(); // UNUSED

private:
    void face_target(); // UNUSED
    void apply_analog_input(F32 dt); // UNUSED
    void apply_spring(F32 dt); // UNUSED
    void collide_inward(xScene& scene, xCamCoordCylinder& coord, const xVec3& face, F32 width); // UNUSED
    void apply_vel_drift(F32 dt); // UNUSED

    const xVec3* target;
    xCamCoordCylinder end_coord;
    xVec3 face;
    xVec3 end_face;
    F32 vel_dist;
    F32 vel_height;
    F32 vel_theta;
    F32 vel_face_height;
    F32 vel_origin_y;
    F32 drift_time;
    F32 vel_stick;
    xVec2 drift_loc;
    F32 drift_vel;
};

#endif

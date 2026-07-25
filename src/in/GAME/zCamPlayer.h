#ifndef ZCAMPLAYER_H
#define ZCAMPLAYER_H

#include "xCam.h"
#include "xCamSupport.h"
#include "xEnt.h"

class zCamPlayer : public xCam
{
public:
    struct target_traits
    {
        xVec3 loc;
        xVec3 vel;
        xVec3 motion;
        xVec3 extra_motion;
    };

    struct basis_traits : xCamCoordPolar
    {
        xVec3 loc;
    };

    struct rest_config
    {
        F32 dist;
        F32 input_center_phi;
        F32 yaw_offset;
        F32 pitch_offset;
    };

    target_traits target_goal;
    target_traits target;
    struct
    {
        F32 phi;
        F32 vel_theta;
        F32 vel_phi;
        F32 dphi;
        F32 pitch_offset;
    } input;
    struct
    {
        F32 input_theta;
        F32 input_pitch_offset;
        F32 restore_dist;
        F32 restore_phi;
        F32 target_xz;
        F32 target_y;
        F32 pitch;
        F32 mount;
        F32 look_xz;
        F32 collide_dist;
        F32 pitch_rest;
        F32 pitch_clamp;
    } blend_vel;
    struct
    {
        F32 dtheta;
        F32 dphi;
        F32 ddist;
    } vel_drift;
    struct
    {
        F32 dist;
        F32 dist_goal;
        F32 dist_vel;
        F32 pitch_offset;
        F32 pitch_offset_goal;
        F32 pitch_offset_vel;
        F32 pitch_offset_speed;
        rest_config start;
        rest_config end;
        rest_config vel;
        rest_config cur;
        rest_config goal;
    } zone;
    struct
    {
        F32 delay_start;
        F32 precollide_theta;
        F32 theta_vel;
    } aggression;
    struct
    {
        xEnt* ent;
        xVec3 center;
        F32 radius;
        F32 margin_angle;
    } secondary;
    struct
    {
        S32 which;
        F32 phi;
        F32 dist;
        F32 theta;
        F32 pitch_offset;
        F32 yaw_offset;
    } start_zone;
    basis_traits basis;
    basis_traits last_basis;
    basis_traits result;
    basis_traits last_result;
    F32 pitch_rest;
    F32 pitch_clamp;
    F32 result_pitch;
    xVec3 look_loc;
    xVec3 losbar_loc;
    xVec3 last_losbar_loc;
    F32 losbar_rail_yoffset;
    F32 last_losbar_rail_yoffset;
    F32 dist_extend;
    F32 dist_extend_vel;
    F32 rest_dist;
    F32 rest_dist_vel;
    F32 rest_phi;
    F32 rest_phi_vel;
    F32 drest_phi;
    F32 look_pitch;
    xCamSupportPath path;
    xCamSupportLOSBar losbar;
    F32 floor_height;
    F32 mount_height;
    F32 mount_offset;
    F32 last_mount_offset;
    bool grounded;
    bool was_colliding;
    xMat4x3 snap_mat;

    void create();
    void start();
    void update(xScene& scene, F32 dt);
    void snap_to(const xMat4x3& mat);

    void set_start_theta(F32 theta)
    {
        start_zone.theta = theta;
        start_zone.which |= 0x4;
    }
};

#endif
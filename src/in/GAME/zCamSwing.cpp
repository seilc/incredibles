#include "xCommon.h"
#include "zCamSwing.h"

#include "xEnt.h"
#include "xGlobals.h"
#include "xMath.h"
#include "xModel.h"
#include "zMain.h"
#include "zPlayer.h"

void zCamSwing::create()
{
    xCam::create();
    coord_type = XCAM_COORD_CYLINDER;
    orient_type = XCAM_ORIENT_EULER;
    cfg_common.priority = 0x81;
    get_target(&coord.cylinder.origin);
    losbar.cfg.rail_ymin = 0.0f;
    losbar.cfg.rail_ymax = 0.0f;
    losbar.cfg.pivot_bar_radius = 0.5f;
    losbar.cfg.compress_bar_radius = 0.2f;
    losbar.cfg.compress_ball_radius = 0.495f;
    losbar.cfg.compress_dist_min = 1.0f;
}

void zCamSwing::get_target(xVec3* target)
{
    xVec3Copy(target, (xVec3*)&ZPLAYER->model->Mat->pos);
    target->y += get_targetHeight();
}

void zCamSwing::refresh_from_mat()
{
    xVec3 target;
    get_target(&target);

    xVec3 offset;
    xVec3Sub(&offset, &mat.pos, &target);
    coord.cylinder.height = offset.y;
    offset.y = 0.0f;
    coord.cylinder.dist = xVec3Length(&offset);
    if (coord.cylinder.dist < 3.0f) {
        coord.cylinder.dist = 3.0f;
    }

    orient.euler.pitch = xatan2(coord.cylinder.height, coord.cylinder.dist) - get_pitchCorrect();
    orient.euler.yaw = coord.cylinder.theta + PI;
    if (orient.euler.yaw > PI) {
        orient.euler.yaw -= 2.0f * PI;
    }

    xVec3Copy(&coord.cylinder.origin, &target);
}

void zCamSwing::refresh_from_my_mat()
{
    xVec3 target;
    get_target(&target);

    xVec3 dir;
    xVec3Sub(&dir, &mat.pos, &target);
    F32 len = xVec3Length(&dir);
    if (len > EPSILON) {
        xVec3SMulBy(&dir, 1.0f / len);
        coord.cylinder.theta = xatan2(dir.x, dir.z);
    }

    refresh_from_mat();
}

void zCamSwing::refresh_from_other_mat()
{
    coord.cylinder.theta = xatan2(-mat.at.x, -mat.at.z);
    refresh_from_mat();
}

void zCamSwing::set_pref_factor(xVec3* hangVec, F32 dt)
{
    F32 len = xVec3Length(hangVec);
    if (len < EPSILON) {
        return;
    }

    xVec3 hang;
    xVec3SMul(&hang, hangVec, 1.0f / len);

    xVec3 dir;
    xVec3Sub(&dir, &mat.pos, &coord.cylinder.origin);
    dir.y = 0.0f;
    len = xVec3Length(&dir);
    if (len < EPSILON) {
        return;
    }

    xVec3SMulBy(&dir, 1.0f / len);
    smooth_pref_factor(xVec3Dot(&hang, &dir), dt);
}

void zCamSwing::smooth_pref_factor(F32 factor, F32 dt)
{
    F32 factorSpeed;
    if (camSwingFlags & k_ZCAMSWING_UNLOCKED) {
        factorSpeed = unlockFactorSpeed;
    } else {
        factorSpeed = lockFactorSpeed;
    }

    if (prefFactor < factor) {
        prefFactor += dt * factorSpeed;
        if (prefFactor > factor) {
            prefFactor = factor;
        }
    } else if (prefFactor > factor) {
        prefFactor -= dt * factorSpeed;
        if (prefFactor < factor) {
            prefFactor = factor;
        }
    }
}

void zCamSwing::get_height_and_dist(zCamSwingPrefs* prefs)
{
    F32 t;
    S32 from, to;
    if (prefFactor > 0.0f) {
        t = prefFactor;
        from = 1;
        to = 2;
    } else {
        t = -prefFactor;
        from = 1;
        to = 0;
    }

    prefs->height = t * pref[to].height;
    prefs->height += (1.0f - t) * pref[from].height;
    prefs->dist = t * pref[to].dist;
    prefs->dist += (1.0f - t) * pref[from].dist;
}

F32 zCamSwing::get_targetHeight()
{
    F32 targetHeight;
    if (prefFactor > 0.0f) {
        targetHeight = prefFactor * pref[2].targetHeight;
        targetHeight += (1.0f - prefFactor) * pref[1].targetHeight;
    } else {
        targetHeight = -prefFactor * pref[0].targetHeight;
        targetHeight += (1.0f + prefFactor) * pref[1].targetHeight;
    }
    return targetHeight;
}

F32 zCamSwing::get_pitchCorrect()
{
    F32 pitchCorrect;
    if (prefFactor > 0.0f) {
        pitchCorrect = prefFactor * pref[2].pitchCorrect;
        pitchCorrect += (1.0f - prefFactor) * pref[1].pitchCorrect;
    } else {
        pitchCorrect = -prefFactor * pref[0].pitchCorrect;
        pitchCorrect += (1.0f + prefFactor) * pref[1].pitchCorrect;
    }
    return pitchCorrect;
}

void zCamSwing::start()
{
    xCam::start();
    prefFactor = 0.0f;
    refresh_from_other_mat();
    heightOffset = coord.cylinder.height - pref[0].height;
    distOffset = coord.cylinder.dist - pref[0].dist;
    refresh_mat();
    losbar.reset(coord.cylinder.origin, mat.pos);
    cfg_common.priority = 0x81;
}

void zCamSwing::update(xScene&, F32 dt)
{
    xVec3 target;
    get_target(&target);

    xVec3 delta;
    xVec3Sub(&delta, &target, &coord.cylinder.origin);
    xVec3Copy(&coord.cylinder.origin, &target);

    coord.cylinder.theta += yawSpeed * analog.offset.x * dt;
    if (coord.cylinder.theta > PI) {
        coord.cylinder.theta -= 2.0f * PI;
    } else if (coord.cylinder.theta < -PI) {
        coord.cylinder.theta += 2.0f * PI;
    }

    zCamSwingPrefs prefs;
    get_height_and_dist(&prefs);

    if (heightOffset < 0.0f) {
        heightOffset += dt * heightSpeed;
        if (heightOffset > 0.0f) {
            heightOffset = 0.0f;
        }
    } else if (heightOffset > 0.0f) {
        heightOffset -= dt * heightSpeed;
        if (heightOffset < 0.0f) {
            heightOffset = 0.0f;
        }
    }
    coord.cylinder.height = prefs.height + heightOffset;

    if (distOffset < 0.0f) {
        distOffset += dt * distSpeed;
        if (distOffset > 0.0f) {
            distOffset = 0.0f;
        }
    } else if (distOffset > 0.0f) {
        distOffset -= dt * distSpeed;
        if (distOffset < 0.0f) {
            distOffset = 0.0f;
        }
    }
    coord.cylinder.dist = prefs.dist + distOffset;

    if (camSwingFlags & k_ZCAMSWING_UNLOCKED) {
        xVec3 offset;
        offset.x = coord.cylinder.dist * isin(coord.cylinder.theta);
        offset.y = 0.0f;
        offset.z = coord.cylinder.dist * icos(coord.cylinder.theta);
        delta.y = 0.0f;

        xVec3 dir;
        xVec3Copy(&dir, &offset);
        xVec3AddScaled(&dir, &delta, -unlockedLag);
        F32 len = xVec3Length(&dir);
        if (len > EPSILON) {
            xVec3SMulBy(&dir, 1.0f / len);
            coord.cylinder.theta = xatan2(dir.x, dir.z);
        }
    }

    orient.euler.yaw = coord.cylinder.theta + PI;
    if (orient.euler.yaw > PI) {
        orient.euler.yaw -= 2.0f * PI;
    }

    refresh_mat();

    losbar.begin_update(coord.cylinder.origin);
    losbar.precollide();
    losbar.stern_loc = mat.pos;
    if (losbar.collide_pivot()) {
        mat.pos = losbar.stern_loc;
    }
    if (losbar.collide_compress()) {
        mat.pos = losbar.stern_loc;
    }

    refresh_from_my_mat();
    heightOffset = coord.cylinder.height - prefs.height;
    distOffset = coord.cylinder.dist - prefs.dist;
    refresh_mat();
}

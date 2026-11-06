#include "xCamFollow.h"

#include "xCollideFast.h"
#include "xCollideSweptSphere.h"
#include "xEnt.h"
#include "xScene.h"

extern xGrid colls_grid;
extern xGrid colls_oso_grid;

static void SweptSphereHitsCamFollowEnt(xScene*, xRay3* ray, xQCData* qcd, xEnt* ent, void* data); // UNUSED

// UNUSED: speculative
void xCamFollow::start()
{
    xCam::start();

    coord.cylinder.origin = *target;
    coord.cylinder.dist = xsqrt(xsqr(mat.pos.x - target->x) + xsqr(mat.pos.z - target->z));
    coord.cylinder.height = mat.pos.y - target->y;
    coord.cylinder.theta = xatan2(mat.pos.x - target->x, mat.pos.z - target->z);
    end_coord = coord.cylinder;

    vel_dist = vel_height = vel_theta = 0.0f;
    vel_face_height = vel_origin_y = 0.0f;
    drift_time = vel_stick = 0.0f;
    drift_loc.assign(target->x, target->z);
    drift_vel = 0.0f;
}

// UNUSED: speculative
void xCamFollow::face_target()
{
    xVec3 heading = *target;
    heading -= coord.cylinder.origin;
    heading.y = 0.0f;

    F32 heading_dist2 = heading.length2();
    if (heading_dist2 < 0.0001f) {
        return;
    }

    heading /= xsqrt(heading_dist2);
    heading.negate();
    end_coord.theta = xatan2(heading.x, heading.z);
}

// UNUSED: speculative
void xCamFollow::apply_analog_input(F32 dt)
{
    F32 sstick = analog.offset.x;
    F32 max_vel_stick = sstick * cfg.stick_vel.x;

    if (vel_stick < max_vel_stick) {
        vel_stick += cfg.stick_accel.x * dt;
        if (vel_stick > max_vel_stick) {
            vel_stick = max_vel_stick;
        }
    } else {
        vel_stick -= cfg.stick_accel.x * dt;
        if (vel_stick < max_vel_stick) {
            vel_stick = max_vel_stick;
        }
    }

    end_coord.theta = xrmod(end_coord.theta + vel_stick * dt);

    xCamConfigFollow::zone_data zone;
    blend3(zone, cfg.zone_below, cfg.zone_rest, cfg.zone_above, analog.offset.y);

    end_coord.dist = zone.offset.z;
    end_coord.height = zone.offset.y;
    end_face = zone.face;
}

// UNUSED: speculative
void xCamFollow::apply_spring(F32 dt)
{
    F32 emove = xDampSpringCoeff(dt, cfg.speed_move);
    spring_interp(coord.cylinder.dist, vel_dist, coord.cylinder.dist, end_coord.dist, dt,
                  cfg.speed_move, emove);
    spring_interp(coord.cylinder.height, vel_height, coord.cylinder.height, end_coord.height, dt,
                  cfg.speed_move, emove);
    spring_interp(coord.cylinder.origin.y, vel_origin_y, coord.cylinder.origin.y, end_coord.origin.y,
                  dt, cfg.speed_move, emove);
    spring_interp(face.y, vel_face_height, face.y, end_face.y, dt, cfg.speed_zone_face);

    F32 theta_diff = xDangleClamp(coord.cylinder.theta - end_coord.theta);
    xDampSpring(theta_diff, vel_theta, dt, cfg.speed_move_orbit);
    coord.cylinder.theta = end_coord.theta + theta_diff;
}

// UNUSED: speculative
void xCamFollow::collide_inward(xScene& scene, xCamCoordCylinder& coord, const xVec3& face, F32 width)
{
    xVec3 loc;
    xSweptSphere sws;

    coord_to_world(loc, coord);

    xVec3 tgtpos;
    tgtpos = face;
    xSweptSpherePrepare(&sws, &tgtpos, &loc, width);

    xRay3 ray;
    ray.origin = tgtpos;
    xVec3Sub(&ray.dir, &loc, &tgtpos);
    ray.max_t = xVec3Length(&ray.dir);
    F32 one_len = 1.0f / ray.max_t;
    xVec3SMul(&ray.dir, &ray.dir, one_len);
    ray.min_t = 0.0f;
    ray.flags = XRAY3_USE_MIN | XRAY3_USE_MAX;

    xRayHitsGrid(&colls_grid, &scene, &ray, SweptSphereHitsCamFollowEnt, &sws.qcd, &sws);
    xRayHitsGrid(&colls_oso_grid, &scene, &ray, SweptSphereHitsCamFollowEnt, &sws.qcd, &sws);

    if (sws.curdist < sws.dist) {
        xVec2 offset;
        offset.x = sws.worldPos.x - coord.origin.x;
        offset.y = sws.worldPos.z - coord.origin.z;
        coord.dist = offset.length();
        if (coord.dist < cfg.collide.dist_min) {
            coord.dist = cfg.collide.dist_min;
        }
    }
}

// UNUSED: speculative
void xCamFollow::apply_vel_drift(F32 dt)
{
    xVec2 B0, B1;
    B0 = drift_loc;
    B1.assign(target->x, target->z);

    xVec2 dB;
    dB.assign(B1.x - B0.x, B1.y - B0.y);
    F32 dist = dB.length();
    F32 vel = dist / dt;

    if (vel < cfg.vel_drift.vel_min) {
        drift_time = 0.0f;
        drift_loc = B1;
        return;
    }

    drift_time += dt;
    if (drift_time < cfg.vel_drift.delay) {
        drift_loc = B1;
        return;
    }

    xVec2 A;
    A.assign(coord.cylinder.origin.x, coord.cylinder.origin.z);
    xVec2& C = drift_loc;

    xVec2 nAC;
    nAC.assign(C.x - A.x, C.y - A.y);
    F32 mAC = nAC.length();
    nAC /= mAC;
    xDampSpring(mAC, drift_vel, dt, cfg.vel_drift.speed);

    xVec2 CB1 = nAC;
    CB1 *= cfg.vel_drift.dist;
    CB1 -= B1;
    F32 mCB1 = CB1.length();
    if (mCB1 > cfg.vel_drift.dist_min) {
        C.x += CB1.x * (vel * dt / mCB1);
        C.y += CB1.y * (vel * dt / mCB1);
    }

    xVec2 CB0 = B0;
    CB0 -= C;
    {
        xVec2 CB1 = B1;
        CB1 -= C;

        F32 crossmag = CB0.x * CB1.y - CB0.y * CB1.x;
        F32 dtheta = crossmag * cfg.vel_drift.speed * dt;
        F32 hang_yaw = nAC.dot(CB1);

        if (hang_yaw < cfg.vel_drift.hang_min || hang_yaw > cfg.vel_drift.hang_max) {
            end_coord.theta += dtheta;
        }
    }
}

// UNUSED: speculative
static void SweptSphereHitsCamFollowEnt(xScene*, xRay3* ray, xQCData* qcd, xEnt* ent, void* data)
{
    xSweptSphere* sws = (xSweptSphere*)data;

    if (!ent->camcollModel || !(ent->chkby & 0x10) || !xQuickCullIsects(qcd, &ent->bound.qcd)) {
        return;
    }

    if (!xEntIsVisible(ent)) {
        return;
    }

    U32 result = 0;

    switch (ent->bound.type) {
    case k_XBOUNDTYPE_SPHERE: {
        F32 oldrad = ent->bound.sph.r;
        ent->bound.sph.r += sws->radius;
        result = xRayHitsSphereFast(ray, &ent->bound.sph);
        ent->bound.sph.r = oldrad;
        break;
    }
    case k_XBOUNDTYPE_BOX: {
        xBox tmpbox;
        tmpbox.upper.x = ent->bound.box.box.upper.x + sws->radius;
        tmpbox.upper.y = ent->bound.box.box.upper.y + sws->radius;
        tmpbox.upper.z = ent->bound.box.box.upper.z + sws->radius;
        tmpbox.lower.x = ent->bound.box.box.lower.x - sws->radius;
        tmpbox.lower.y = ent->bound.box.box.lower.y - sws->radius;
        tmpbox.lower.z = ent->bound.box.box.lower.z - sws->radius;
        result = xRayHitsBoxFast(ray, &tmpbox);
        break;
    }
    case k_XBOUNDTYPE_OBB: {
        xBox tmpbox;
        xRay3 lr;
        xMat3x3 mn;
        F32 ms = xVec3Length(&ent->bound.mat->right);

        xMat3x3Normalize(&mn, ent->bound.mat);
        xMat4x3Tolocal(&lr.origin, ent->bound.mat, &ray->origin);
        xMat3x3Tolocal(&lr.dir, &mn, &ray->dir);
        lr.max_t = ray->max_t / ms;
        lr.min_t = ray->min_t / ms;
        lr.flags = ray->flags;

        tmpbox.upper.x = ent->bound.box.box.upper.x + sws->radius / ms;
        tmpbox.upper.y = ent->bound.box.box.upper.y + sws->radius / ms;
        tmpbox.upper.z = ent->bound.box.box.upper.z + sws->radius / ms;
        tmpbox.lower.x = ent->bound.box.box.lower.x - sws->radius / ms;
        tmpbox.lower.y = ent->bound.box.box.lower.y - sws->radius / ms;
        tmpbox.lower.z = ent->bound.box.box.lower.z - sws->radius / ms;
        result = xRayHitsBoxFast(&lr, &tmpbox);
        break;
    }
    }

    if (result) {
        xModelInstance* collmod = ent->camcollModel;
        xSweptSphereToModel(sws, collmod->Data, collmod->Mat);
    }
}

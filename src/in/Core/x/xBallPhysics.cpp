#include "xCommon.h"
#include "xBallPhysics.h"

#include "xCollideSweptSphere.h"
#include "xEnt.h"
#include "xGlobals.h"
#include "xSndMgr.h"
#include "xSurface.h"
#include "zMain.h"
#include "zPlayer.h"
#include "zScene.h"
#include "zSplash.h"
#include "zSurface.h"

#include <string.h>

void xBallPhysics::reset_physics()
{
    live = true;
    splashed = false;
    ground_normal = xVec3::m_UnitAxisY;
    velocity = xVec3::m_Null;
    rotation_velocity = xVec3::m_Null;
    grounded = false;
    collision_surface = NULL;
    roll_sound = XSNDMGR_INVALID_SND_HANDLE;
    air_time = 0.0f;
    roll_time = 0.0f;
    impact_loc = HUGE;
}

void xBallPhysics::die()
{
    if (live) {
        live = false;
        if (roll_sound != XSNDMGR_INVALID_SND_HANDLE) {
            xSndMgrFadeOff(roll_sound, 0.15f - air_time);
        }
    }
}

bool xBallPhysics::hit_water(const xSweptSphere& sws)
{
    if (collision_surface->moprops && zSurfaceGetProps(collision_surface)->asset &&
        zSurfaceGetProps(collision_surface)->asset->swimmable_surface) {
        if (properties->splash_name) {
            zSplashEmit(zSplashFindType(properties->splash_name), sws.worldContact);
        }
        splash_y = sws.worldContact.y;
        splashed = true;
        return true;
    }
    return false;
}

void xBallPhysics::play_bounce(const xVec3& loc, const xVec3& normal)
{
    if (!properties->bounce_group) {
        return;
    }

    if (air_time < 0.25f) {
        return;
    }

    F32 speed = -velocity.dot(normal);
    if (speed <= properties->bounce_min_speed) {
        return;
    }

    F32 volume;
    if (speed >= properties->bounce_max_speed) {
        volume = 1.0f;
    } else {
        volume = (speed - properties->bounce_min_speed) / (properties->bounce_max_speed - properties->bounce_min_speed);
    }
    xSndMgrPlay(xSndMgrGetSoundGroup(properties->bounce_group), xSnd_RELATIVEVOLUME, &loc, NULL, NULL, &volume, NULL);
}

void xBallPhysics::play_roll(xEnt* ent)
{
    if (!properties->roll_group) {
        return;
    }

    if (!live) {
        if (roll_sound != XSNDMGR_INVALID_SND_HANDLE) {
            xSndMgrFadeOff(roll_sound, 0.15f - air_time);
        }
        return;
    }

    if (collision && roll_sound == XSNDMGR_INVALID_SND_HANDLE) {
        roll_time = 0.0f;
    }

    F32 air = 1.0f - air_time * (1.0f / 0.15f);
    F32 len = velocity.length();
    F32 speed = (len - 1.0f) * (1.0f / 7.0f);
    F32 volume = air * CLAMP(speed, 0.0f, 1.0f);
    if (volume > 0.0f) {
        F32 fade = 20.0f * roll_time;
        if (fade > 1.0f) {
            fade = 1.0f;
        }
        volume *= fade;
        if (roll_sound == XSNDMGR_INVALID_SND_HANDLE) {
            roll_sound = xSndMgrPlay(xSndMgrGetSoundGroup(properties->roll_group), xSnd_RELATIVEVOLUME | xSnd_FOLLOW,
                                     xEntGetCenter(ent), NULL, NULL, &volume, NULL);
        } else {
            xSndMgrSetVolume(roll_sound, volume, true);
        }
    } else if (roll_sound != XSNDMGR_INVALID_SND_HANDLE) {
        xSndMgrStop(roll_sound);
    }
}

void xBallPhysics::update_sounds(xEnt* ent, F32 dt)
{
    if (collision) {
        air_time = 0.0f;
    } else {
        air_time += dt;
    }
    roll_time += dt;
    play_roll(ent);
}

void xBallPhysics::update_rotation(xMat4x3& mat, const xVec3& rot_center, F32 dt)
{
    xVec3 prev_bound_trans = rot_center - mat.pos;
    xVec3 new_bound_trans;
    xMat3x3LMulVec(&new_bound_trans, &mat, &prev_bound_trans);

    xVec3 dpos;
    xVec3ScaleC(dpos, rotation_velocity, dt);
    F32 angle_change = dpos.length() / radius;

    xVec3 direction;
    xVec3Copy(&direction, &dpos);
    direction.safe_normalize(ground_normal);
    if (std::fabsf(ground_normal.dot(direction)) > 0.9995f) {
        return;
    }

    xVec3 right = ground_normal.cross(direction);
    xQuat q;
    xQuatFromAxisAngle(&q, &right, -angle_change);
    xQuat original_rotation;
    xQuatFromMat(&original_rotation, &mat);
    xQuatMul(&q, &original_rotation, &q);
    xQuatToMat(&q, &mat);

    xMat3x3RMulVec(&new_bound_trans, &mat, &new_bound_trans);
    mat.pos += prev_bound_trans - new_bound_trans;
}

void xBallPhysics::update_ent(xEnt* ent, F32 dt)
{
    xVec3 bound_pos;
    xBoundGetCenter(&ent->bound, &bound_pos);
    xVec3 new_pos;
    xVec3Copy(&new_pos, &bound_pos);
    update_rotation(ent->frame->mat, new_pos, dt);
    update_position(ent, new_pos, dt);
    ent->frame->setDpos(new_pos - bound_pos);
}

void xBallPhysics::update_position(xEnt* ent, xVec3& position, F32 dt)
{
    collision = false;
    if (live) {
        if (!use_cheaper_collision) {
            grounded = !splashed && on_ground(ent, position);
        }

        if (!grounded) {
            velocity.y -= properties->gravity * dt;
        } else {
            xVec3 across;
            xVec3Cross(across, ground_normal, xVec3::m_UnitAxisY);
            if (across.length2() > 0.01f * 0.01f) {
                xVec3 downhill;
                xVec3Cross(downhill, ground_normal, across);
                xVec3ScaleC(downhill, properties->gravity * dt);
                xVec3Add(velocity, downhill);
            }

            xVec3 direction = velocity;
            direction.y = 0.0f;
            F32 speed = direction.length();
            if (!xfeq0(speed)) {
                xVec3ScaleC(direction, 1.0f / speed);
                xVec3AddScaled(velocity, direction, -MIN(speed, properties->friction * dt));
            }
        }

        xVec3 collide_pos;
        if (splashed) {
            position.x += velocity.x * dt;
            position.y += velocity.y * dt;
            position.z += velocity.z * dt;
            if (position.y < splash_y - (1.0f + radius)) {
                live = false;
            }
        } else {
            collision = collide(ent, position, collide_pos, velocity * dt, dt);
            if (collision) {
                xVec2 velXZ = { velocity.x, velocity.z };
                if (velXZ.length2() < 0.01f * 0.01f && velocity.y < 0.25f) {
                    position = collide_pos;
                    live = false;
                }
            } else if (velocity.length2() < 0.05f * 0.05f && xVec3Dist2(&impact_loc, &position) < 0.1f * 0.1f) {
                live = false;
                position.y = impact_loc.y;
            }
        }
    }

    update_sounds(ent, dt);
}

bool xBallPhysics::collide(xEnt* ent, xVec3& position, xVec3& collide_pos, const xVec3& dpos, F32)
{
    if (xfeq0(velocity.length2())) {
        return false;
    }

    xSweptSphere sws;
    memset(&sws, 0, sizeof(sws));
    S32 rc = 1;
    xVec3 start;
    xVec3Copy(&start, &position);
    xVec3 end;
    xVec3Add(end, position, dpos);
    S32 depth = 0;

    while (rc) {
        depth++;
        xSweptSpherePrepare(&sws, &start, &end, radius);
        S32 stat = xSweptSphereToStatDyn(&sws, xglobals->sceneCur, ent, k_XENT_COLLTYPE_PC, 0);
        S32 env = xSweptSphereToEnv(&sws, xglobals->sceneCur->env);
        rc = stat | env;

        const xSphere* extra_spheres[6];
        S32 num_spheres = ZPLAYER->GetCollisionSpheres(extra_spheres);
        for (S32 i = 0; i < num_spheres; i++) {
            if (xSweptSphereToSphere(&sws, extra_spheres[i])) {
                rc = 1;
                sws.mptr = ZPLAYER->model;
                sws.oid = ZPLAYER->id;
                sws.optr = ZPLAYER;
                break;
            }
        }

        if (rc) {
            xSweptSphereGetResults(&sws);
            xVec3Copy(&collide_pos, &sws.worldPos);

            xSurface* surf = zSurfaceGetSurface(&sws);
            if (surf) {
                collision_surface = surf;
                if (hit_water(sws)) {
                    break;
                }
            }

            grounded = true;
            if (!collided(sws)) {
                return true;
            }

            xVec3Copy(&ground_normal, &sws.worldNormal);
            xVec3Copy(&impact_loc, &sws.worldPos);
            xVec3ScaleC(rotation_velocity, velocity, properties->rotation_factor);

            if (depth > 3) {
                xVec3Copy(&end, &sws.worldPos);
                break;
            }

            xVec3 new_direction;
            xVec3Sub(new_direction, sws.end, sws.start);
            xVec3Copy(&start, &sws.worldPos);
            F32 result = new_direction.length();
            if (xfeq0(result)) {
                xVec3Copy(&end, &sws.worldPos);
                break;
            }

            if (depth == 1) {
                play_bounce(sws.worldContact, sws.worldNormal);
            }

            xVec3ScaleC(new_direction, 1.0f / result);
            xVec3AddScaled(new_direction, sws.worldNormal, -2.0f * new_direction.dot(sws.worldNormal));
            new_direction.safe_normalize(sws.worldTangent);
            xVec3AddScaled(end, start, new_direction, sws.dist - sws.curdist);
            xVec3SMul(velocity, new_direction, velocity.length());

            F32 vdot = sws.worldNormal.dot(velocity);
            F32 new_vdot = vdot * (1.0f - properties->damp_percent);
            xVec3AddScaled(velocity, sws.worldNormal, new_vdot - MIN(new_vdot, properties->damp) - vdot);
            velocity.x *= properties->horizontal_damp;
            velocity.z *= properties->horizontal_damp;
        } else {
            grounded = false;
        }

        if (use_cheaper_collision) {
            break;
        }
    }

    xVec3Copy(&position, &end);
    return depth > 1 || (use_cheaper_collision && rc);
}

bool xBallPhysics::on_ground(xEnt* ent, const xVec3& position)
{
    xVec3 start = position;
    xVec3 end;
    xVec3Copy(&end, &start);
    end.y -= 0.1f;

    xSweptSphere sws;
    xSweptSpherePrepare(&sws, &start, &end, radius);
    S32 rc = xSweptSphereToStatDyn(&sws, xglobals->sceneCur, ent, k_XENT_COLLTYPE_PC, 0);
    S32 env = xSweptSphereToEnv(&sws, xglobals->sceneCur->env);
    if (rc || env) {
        xSweptSphereGetResults(&sws);

        xSurface* surf = zSurfaceGetSurface(&sws);
        if (surf) {
            collision_surface = surf;
            if (hit_water(sws)) {
                return false;
            }
        }

        xVec3Copy(&ground_normal, &sws.worldNormal);
        xVec3Copy(&impact_loc, &sws.worldPos);
        xVec3ScaleC(rotation_velocity, velocity, properties->rotation_factor);
        return true;
    }
    return false;
}

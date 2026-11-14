#ifndef XBALLPHYSICS_H
#define XBALLPHYSICS_H

#include "xMath3.h"
#include "iSnd.h"

struct xEnt;
struct xSurface;
struct xSweptSphere;

struct xBallPhysicsProperties
{
    F32 damp_percent;
    F32 damp;
    F32 friction;
    F32 horizontal_damp;
    F32 gravity;
    F32 rotation_factor;
    char* splash_name;
    U32 bounce_group;
    U32 roll_group;
    F32 bounce_min_speed;
    F32 bounce_max_speed;
};

struct xBallPhysics
{
    const xBallPhysicsProperties* properties;
    xVec3 velocity;
    xVec3 rotation_velocity;
    xVec3 ground_normal;
    xVec3 impact_loc;
    F32 radius;
    F32 splash_y;
    xSurface* collision_surface;
    iSndHandle roll_sound;
    F32 air_time;
    F32 roll_time;
    bool live;
    bool splashed;
    bool collision;
    bool grounded;

    virtual bool collided(const xSweptSphere&) { return true; }

    bool use_cheaper_collision;

    void reset_physics();
    void die();
    bool hit_water(const xSweptSphere& sws);
    void play_bounce(const xVec3& loc, const xVec3& normal);
    void play_roll(xEnt* ent);
    void update_sounds(xEnt* ent, F32 dt);
    void update_rotation(xMat4x3& mat, const xVec3& rot_center, F32 dt);
    void update_ent(xEnt* ent, F32 dt);
    void update_position(xEnt* ent, xVec3& position, F32 dt);
    bool collide(xEnt* ent, xVec3& position, xVec3& collide_pos, const xVec3& dpos, F32);
    bool on_ground(xEnt* ent, const xVec3& position);
};

#endif

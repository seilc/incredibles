#ifndef ZSHRAPNEL_H
#define ZSHRAPNEL_H

#include "xModel.h"
#include "zShrapnelAsset.h"
#include "zLightning.h"
#include "zExplosion.h"

struct zFragGroup
{
    zFrag* list[21];
};

struct zFragShrapnel
{
    zFragShrapnelAsset* fasset;
};

struct zFragParticle
{
    zFragParticleAsset* fasset;
    U8 no_last_position;
    F32 emit_remaining;
    xVec3 last_position;
};

struct zFragProjectile
{
    zFragProjectileAsset* fasset;
    xModelInstance* model;
    xParabola path;
    F32 angVel;
    F32 t;
    F32 tColl;
    S32 numBounces;
    F32 scale;
    F32 alpha;
    F32 bounceC1;
    F32 bounceC2;
    xVec3 N;
    xVec3 axis;
};

struct zFragLightning
{
    zFragLightningAsset* fasset;
    xModelInstance* startParent;
    xModelInstance* endParent;
    zLightning* lightning;
};

struct zFragSound
{
    zFragSoundAsset* fasset;
    iSndHandle soundID;
};

struct zFragShockwave
{
    zFragShockwaveAsset* fasset;
    F32 currSize;
    F32 currVelocity;
    F32 deltVelocity;
    F32 currSpin;
    F32 deltSpin;
    F32 currColor[4];
    F32 deltColor[4];
};

struct zFragExplosion
{
    zFragExplosionAsset* fasset;
    zExplosion::trail_emitter trail;
    xVec3 initPos;
};

struct zFragDistortion
{
    zFragDistortionAsset* fasset;
    xVec3 loc;
    S32 type;
    F32 repeat_time;
};

struct zFragFire
{
    zFragFireAsset* fasset;
};

union zFragInfo
{
    zFragGroup group;
    zFragShrapnel shrapnel;
    zFragParticle particle;
    zFragProjectile projectile;
    zFragLightning lightning;
    zFragSound sound;
    zFragShockwave shockwave;
    zFragExplosion explosion;
    zFragDistortion distortion;
    zFragFire fire;
};

struct zFrag
{
    zFragType type;
    zFragInfo info;
    F32 delay;
    F32 alivetime;
    F32 lifetime;
    void (*update)(zFrag*, F32);
    void (*destroy)(zFrag*);
    xModelInstance* parent[2];
    zFrag* prev;
    zFrag* next;
};

#endif
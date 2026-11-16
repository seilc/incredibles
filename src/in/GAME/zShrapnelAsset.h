#ifndef ZSHRAPNELASSET_H
#define ZSHRAPNELASSET_H

#include "xModel.h"
#include "xParEmitter.h"
#include "xCurveAsset.h"

struct zFrag;
struct zFragAsset;
struct zShrapnelAsset;

typedef void (*zShrapnelFragCallback)(zFrag*, zFragAsset*);
typedef void (*zShrapnelInitCallback)(zShrapnelAsset*, xModelInstance*, xVec3*, zShrapnelFragCallback);

struct zShrapnelAsset
{
    S32 fassetCount;
    U32 shrapnelID;
    zShrapnelInitCallback initCB;
};

enum zFragType
{
    eFragInactive,
    eFragGroup,
    eFragShrapnel,
    eFragParticle,
    eFragProjectile,
    eFragLightning,
    eFragSound,
    eFragShockwave,
    eFragExplosion,
    eFragDistortion,
    eFragFire,
    eFragCount,
    eFragForceSize = FORCEENUMSIZEINT
};

struct zFragAsset
{
    zFragType type;
    U32 id;
    U32 parentID[2];
    F32 lifetime;
    F32 delay;
};

struct zFragBone
{
    S32 index;
    xVec3 offset;
};

enum zFragLocType
{
    eFragLocBone,
    eFragLocBoneUpdated,
    eFragLocBoneLocal,
    eFragLocBoneLocalUpdated,
    eFragLocTag,
    eFragLocTagUpdated,
    eFragLocCount,
    eFragLocForceSize = FORCEENUMSIZEINT
};

union zFragLocInfo
{
    zFragBone bone;
    xModelTag tag;
};

struct zFragLocation
{
    zFragLocType type;
    zFragLocInfo info;
    F32 rand_radius;
};

struct zFragShrapnelAsset : zFragAsset
{
    U32 shrapnelID;
    zShrapnelAsset* shrapAsset;
};

struct zFragParticleAsset : zFragAsset
{
    zFragLocation source;
    zFragLocation vel;
    xParEmitterCustomSettings emit;
    U32 dummy0;
    void* dummy1;
};

struct zFragProjectileAsset : zFragAsset
{
    U32 modelInfoID;
    RpAtomic* modelFile;
    zFragLocation launch;
    zFragLocation vel;
    zFragLocation velPlusMinus;
    zFragLocation rot;
    zFragLocation rotPlusMinus;
    F32 bounce;
    S32 maxBounces;
    U32 flags;
    U32 childID;
    zShrapnelAsset* child;
    F32 minScale;
    F32 maxScale;
    U32 scaleCurveID;
    xCurveAsset* scaleCurve;
    F32 gravity;
};

struct zFragLightningAsset : zFragAsset
{
    zFragLocation start;
    zFragLocation end;
    U32 startParentID;
    U32 endParentID;
};

struct zFragSoundAsset : zFragAsset
{
    U32 assetID;
    zFragLocation source;
};

struct zFragShockwaveAsset : zFragAsset
{
    U32 modelInfoID;
    F32 birthRadius;
    F32 deathRadius;
    F32 birthVelocity;
    F32 deathVelocity;
    F32 birthSpin;
    F32 deathSpin;
    F32 birthColor[4];
    F32 deathColor[4];
};

struct zFragExplosionAsset : zFragAsset
{
    U32 type;
    zFragLocation loc;
    U32 flags;
};

struct zFragDistortionAsset : zFragAsset
{
    U32 type;
    zFragLocation loc;
    U32 flags;
    F32 radius;
    F32 duration;
    F32 intensity;
    F32 freq;
    F32 repeat_delay;
};

struct zFragFireAsset : zFragAsset
{
    zFragLocation loc;
    U32 flags;
    F32 radius;
    F32 scale;
    F32 fuel;
    F32 heat;
    F32 damage;
    F32 knockback;
};

#endif

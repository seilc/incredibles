#ifndef XENT_H
#define XENT_H

#include "xBase.h"
#include "xCollide.h"
#include "xGrid.h"
#include "xBound.h"

struct xEnt;
struct xScene;

struct xEntAsset : xBaseAsset
{
    U8 flags;
    U8 subtype;
    U8 pflags;
    U8 moreFlags;
    U32 surfaceID;
    xVec3 ang;
    xVec3 pos;
    xVec3 scale;
    F32 redMult;
    F32 greenMult;
    F32 blueMult;
    F32 seeThru;
    F32 seeThruSpeed;
    U32 modelInfoID;
    U32 animListID;
};

struct xEntFrame
{
    xMat4x3 mat;
    xMat4x3 oldmat;
    xVec3 oldvel;
    xRot oldrot;
    xRot drot;
    xRot rot;
    xVec3 dvel;
    xVec3 vel;
    U32 mode;
    xVec3 dpos;

    xVec3& setDpos()
    {
        return dpos;
    }
};

struct xEntCollis
{
    U8 chk;
    U8 pen;
    U8 env_sidx;
    U8 env_eidx;
    U8 npc_sidx;
    U8 npc_eidx;
    U8 dyn_sidx;
    U8 dyn_eidx;
    U8 stat_sidx;
    U8 stat_eidx;
    U8 idx;
    xCollis colls[18];
    void (*post)(xEnt*, xScene*, F32, xEntCollis*);
    U32 (*depenq)(xEnt*, xEnt*, xScene*, F32, xCollis*);
};

struct xEntShadow
{
    xVec3 pos;
    xVec3 vec;
    RpAtomic* shadowModel;
    F32 dst_cast;
    F32 radius[2];
    struct
    {
        S32 flg_castOnOneDFF : 1;
        S32 flg_castOnAllDFF : 1;
        S32 flg_disableEnvCast : 1;
        S32 flg_shadowUnused : 29;
    };
};

struct xFFX;
struct xShadowSimpleCache;

struct xEnt : xBase
{
    struct anim_coll_data
    {
        U32 flags;
        U32 bones;
        xMat4x3 old_mat;
        xMat4x3 new_mat;
        U32 verts_size;
        xVec3* verts;
        xVec3* normals;
    };

    xEntAsset* asset;
    U16 idx;
    U8 flags;
    U8 miscflags;
    U8 subType;
    U8 pflags;
    U16 moreFlags;
    struct
    {
        U8 _isCulled : 2;
        U8 collisionEventReceived : 2;
    };
    U8 driving_count;
    U8 num_ffx;
    U8 collType;
    U8 collLev;
    U8 chkby;
    U8 penby;
    void (*visUpdate)(xEnt*);
    xModelInstance* model;
    xModelInstance* collModel;
    xModelInstance* camcollModel;
    void (*update)(xEnt*, xScene*, F32);
    void (*endUpdate)(xEnt*, xScene*, F32);
    void (*bupdate)(xEnt*, xVec3*);
    void (*move)(xEnt*, xScene*, F32, xEntFrame*);
    void (*render)(xEnt*);
    xEntFrame* frame;
    xEntCollis* collis;
    xGridBound gridb;
    xBound bound;
    void (*transl)(xEnt*, xVec3*, xMat4x3*);
    xFFX* ffx;
    xEnt* driver;
    xEnt* driven;
    S32 driveMode;
    xShadowSimpleCache* simpShadow;
    xEntShadow* entShadow;
    anim_coll_data* anim_coll;
    void* user_data;
};


typedef void (*xEntVisHandler)(xEnt* ent);

void xEntInit(xEnt* ent, xEntAsset* asset);
void xEntGrowGlobalBoundingBox(const xVec3* pos);
void xEntInitForType(xEnt* ent);
void xEntSetup(xEnt* ent);
void xEntReset(xEnt* ent);
void xEntRender(xEnt* ent);
void xEntEndUpdate(xEnt* ent, xScene* sc, F32 dt);
void xEntMove(xEnt* ent, xScene* sc, F32 dt);
void xEntApplyPhysics(xEnt* ent, xScene* sc, F32 dt);
void xEntShow(xEnt* ent);
void xEntHide(xEnt* ent);
xMat4x3* xEntGetFrame(const xEnt* ent);
xVec3* xEntGetPos(const xEnt* ent);

U32 xEntIsEnabled(const xEnt* ent);
U32 xEntIsVisible(const xEnt* ent);
bool xBaseIsEntity(xBase* base);

inline void xEntVisibilityCullOn(xEnt* ent)
{
    ent->flags |= (U8)0x4;

    if (ent->visUpdate)
    {
        ent->visUpdate(ent);
    }
}

inline void xEntVisibilityCullOff(xEnt* ent)
{
    ent->flags &= (U8)~0x4;

    if (ent->visUpdate)
    {
        ent->visUpdate(ent);
    }
}

#endif
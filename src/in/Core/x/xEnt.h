#ifndef XENT_H
#define XENT_H

#include "xBase.h"
#include "xCollide.h"
#include "xGrid.h"
#include "xBound.h"

struct xEnt;
struct xScene;
struct xEntCollis;
struct xEntDriveInfo;

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

    const xVec3& getDpos() const
    {
        return dpos;
    }

    xVec3& setDpos()
    {
        return dpos;
    }

    void setDpos(const xVec3& val)
    {
        dpos = val;
    }
};

typedef void(*xEntCollisPostCallback)(xEnt*, xScene*, F32, xEntCollis*);
typedef U32(*xEntCollisDepenQueryCallback)(xEnt*, xEnt*, xScene*, F32, xCollis*);

// xEntFrame::mode
#define k_XENT_SET_ROT (1 << 4)

// xEnt::flags
#define k_XENT_IS_VISIBLE ((U8)(1 << 0))

// xEnt::pflags
#define k_XENT_IS_MOVING ((U8)(1 << 0))
#define k_XENT_HAS_VELOCITY ((U8)(1 << 1))
#define k_XENT_HAS_GRAVITY ((U8)(1 << 2))
#define k_XENT_HAS_DRAG ((U8)(1 << 3))

// xEnt::collType, chkby, penby; xEntCollis::chk, pen
#define k_XENT_COLLTYPE_NONE ((U8)0)
#define k_XENT_COLLTYPE_TRIG ((U8)(1 << (0)))
#define k_XENT_COLLTYPE_STAT ((U8)(1 << (1)))
#define k_XENT_COLLTYPE_DYN ((U8)(1 << (2)))
#define k_XENT_COLLTYPE_NPC ((U8)(1 << (3)))
#define k_XENT_COLLTYPE_PC ((U8)(1 << (4)))
#define k_XENT_COLLTYPE_ENV ((U8)(1 << (5)))

// xEnt::collLev
#define k_XENT_COLLLEV_FASTEST 0
#define k_XENT_COLLLEV_AABB 1
#define k_XENT_COLLLEV_SPH 2
#define k_XENT_COLLLEV_OBB 3
#define k_XENT_COLLLEV_BOUND 4
#define k_XENT_COLLLEV_MODEL 5

// xEnt::moreFlags
#define k_MORE_FLAGS_ANIM_COLL ((U8)1<<5)
#define k_MORE_FLAGS_HITTABLE ((U8)1<<4)

#define k_XENT_MAX_COLL 18

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
    xCollis colls[k_XENT_MAX_COLL];
    xEntCollisPostCallback post;
    xEntCollisDepenQueryCallback depenq;
};

struct xEntDriveInfo
{
    xMat4x3 driveMat;
    xVec3 centerOffset;
    U32 flags;
};

struct xEntShadow
{
    enum radius_enum
    {
        RADIUS_CACHE,
        RADIUS_RASTER,
        MAX_RADIUS
    };

    xVec3 pos;
    xVec3 vec;
    RpAtomic* shadowModel;
    F32 dst_cast;
    F32 radius[2];
    S32 flg_castOnOneDFF : 1;
    S32 flg_castOnAllDFF : 1;
    S32 flg_disableEnvCast : 1;
    S32 flg_shadowUnused : 29;
};

struct xFFX;
struct xShadowSimpleCache;

typedef void(*xEntVisUpdateCallback)(xEnt*);
typedef void(*xEntUpdateCallback)(xEnt*, xScene*, F32);
typedef void(*xEntBoundUpdateCallback)(xEnt*, xVec3*);
typedef void(*xEntMoveCallback)(xEnt*, xScene*, F32, xEntFrame*);
typedef void(*xEntRenderCallback)(xEnt*);
typedef void(*xEntTranslateCallback)(xEnt*, xVec3*, xMat4x3*);

struct xEnt : xBase
{
    struct anim_coll_data;

    xEntAsset* asset;
    U16 idx;
    U8 flags;
    U8 miscflags;
    U8 subType;
    U8 pflags;
    U16 moreFlags;
    U8 _isCulled : 2;
    U8 collisionEventReceived : 2;
    U8 driving_count : 7;
    U8 driving_reset : 1;
    U8 num_ffx;
    U8 collType;
    U8 collLev;
    U8 chkby;
    U8 penby;
    xEntVisUpdateCallback visUpdate;
    xModelInstance* model;
    xModelInstance* collModel;
    xModelInstance* camcollModel;
    xEntUpdateCallback update;
    xEntUpdateCallback endUpdate;
    xEntBoundUpdateCallback bupdate;
    xEntMoveCallback move;
    xEntRenderCallback render;
    xEntFrame* frame;
    xEntCollis* collis;
    xGridBound gridb;
    xBound bound;
    xEntTranslateCallback transl;
    xFFX* ffx;
    xEnt* driver;
    xEnt* driven;
    xEntDriveInfo* driveInfo;
    xShadowSimpleCache* simpShadow;
    xEntShadow* entShadow;
    anim_coll_data* anim_coll;
    void* user_data;
};

inline xVec3* xEntGetCenter(const xEnt* ent)
{
    return xBoundCenter(&ent->bound);
}

inline xVec3* xEntGetPos(const xEnt* ent)
{
    return &xModelGetFrame(ent->model)->pos;
}

inline U32 xEntIsEnabled(const xEnt* ent)
{
    return xBaseIsEnabled(ent);
}

bool xBaseIsEntity(xBase* base);

inline void xEntVisibilityCullOn(xEnt* ent)
{
    ent->flags |= (U8)0x4;

    if (ent->visUpdate) {
        ent->visUpdate(ent);
    }
}

inline void xEntVisibilityCullOff(xEnt* ent)
{
    ent->flags &= (U8)~0x4;

    if (ent->visUpdate) {
        ent->visUpdate(ent);
    }
}

inline xMat4x3* xEntGetFrame(const xEnt* ent)
{
    return xModelGetFrame(ent->model);
}

struct xEntOpacity
{
    F32 delta_opacity;
    F32 delta_opacity_2;
};

typedef xEnt*(*xEntCollCheckCallback)(xEnt*, xScene*, void*);

void xEntSceneInit();
void xEntSceneExit();
void xEntInit(xEnt* ent, xEntAsset* asset);
void xEntGrowGlobalBoundingBox(const xVec3* pos);
void xEntInitForType(xEnt* ent);
void xEntSetup(xEnt* ent);
void xEntSave(xEnt* ent, xSerial* s);
void xEntLoad(xEnt* ent, xSerial* s);
void xEntReset(xEnt* ent);
void xEntLoadModelFromDisk(xEnt* ent, const char* filename, xMemPool*); // UNUSED
xModelInstance* xEntLoadModel(xEnt* ent, RpAtomic* imodel, xModelInstance* reuse_instance);
void xEntSetupPipeline(xModelInstance* model);
void xEntSetupPipeline(xSurface* surf, RpAtomic* model);
void xEntRestorePipeline(xModelInstance* model);
void xEntRestorePipeline(xSurface*, RpAtomic* model);
void xEntRender(xEnt* ent);
void xEntUpdate(xEnt* ent, xScene* sc, F32 dt);
void xEntBeginUpdate(xEnt* ent, xScene*, F32 dt);
void xEntEndUpdate(xEnt* ent, xScene* sc, F32 dt);
void xEntDefaultBoundUpdate(xEnt* ent, xVec3* pos);
void xEntDefaultTranslate(xEnt* ent, xVec3* dpos, xMat4x3* dmat);
void xEntMotionToMatrix(xEnt* ent, xEntFrame* frame);
void xEntMove(xEnt* ent, xScene* sc, F32 dt);
void xEntApplyPhysics(xEnt* ent, xScene* sc, F32 dt);
void xEntCollide(xEnt* ent, xScene* sc, F32 dt);
void xEntBeginCollide(xEnt* ent, xScene*, F32);
void xEntEndCollide(xEnt* ent, xScene* sc, F32 dt);
void xEntCollCheckEnv(xEnt* p, xScene* sc);
void xEntCollCheckByGrid(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt);
void xEntCollCheckNPCsByGrid(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt);
xEnt* xEntCollCheckOneEntNoDepen(xEnt* ent, xScene* sc, void* data);
void xEntCollideFloor(xEnt* p, xScene* sc, F32 dt);
void xEntCollideCeiling(xEnt* p, xScene* sc, F32 dt);
void xEntCollideWalls(xEnt* p, xScene* sc, F32 dt);
xBox* xEntGetAllEntsBox();
void xEntAnimateCollision(xEnt& ent, bool on);
bool xEntValidType(U8 type);
bool xEntTurnToFace(xEnt* ent, const xVec3* target, F32 speedLimit, F32 dt);
void xEntOpacityInit(xEntOpacity* opacity, xEnt* ent);
void xEntOpacityUpdate(xEntOpacity* opacity, xEnt* ent, F32 dt);
void xEntLoadCheckPoint();
void xEntStoreCheckPoint();

inline void xEntShow(xEnt* ent)
{
    ent->flags |= k_XENT_IS_VISIBLE;
    if (ent->visUpdate) {
        ent->visUpdate(ent);
    }
}

inline void xEntHide(xEnt* ent)
{
    ent->flags &= (U8)~k_XENT_IS_VISIBLE;
    if (ent->visUpdate) {
        ent->visUpdate(ent);
    }
}

inline bool xEntMoves(const xEnt& ent)
{
    return ent.pflags & k_XENT_IS_MOVING || ent.collType & (k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC);
}

inline U32 xEntIsVisible(const xEnt* ent)
{
    return (ent->flags & (k_XENT_IS_VISIBLE | 0x4 | 0x80)) == k_XENT_IS_VISIBLE;
}

#endif

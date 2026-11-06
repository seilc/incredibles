#ifndef XSCENE_H
#define XSCENE_H

#include "xEnt.h"
#include "xEnv.h"
#include "xMemMgr.h"

struct xCollis;
struct xGrid;
struct xModelInstance;
struct xQCData;
struct xRay3;
struct xScene;
struct xSerial;
struct xSphere;

typedef xBase*(*xSceneResolvIDCallback)(U32);
typedef char*(*xSceneBase2NameCallback)(xBase*);
typedef char*(*xSceneID2NameCallback)(U32);
typedef void(*xRayEntCallback)(xScene*, xRay3*, xQCData*, xEnt*, void*);
typedef xEnt*(*xSceneEntCallback)(xEnt*, xScene*, void*);

struct xScene
{
    U32 sceneID;
    U16 flags;
    U16 num_trigs;
    U16 num_stats;
    U16 num_dyns;
    U16 num_npcs;
    U16 num_act_ents;
    F32 gravity;
    F32 drag;
    F32 friction;
    U16 num_ents_allocd;
    U16 num_trigs_allocd;
    U16 num_stats_allocd;
    U16 num_dyns_allocd;
    U16 num_npcs_allocd;
    xEnt** trigs;
    xEnt** stats;
    xEnt** dyns;
    xEnt** npcs;
    xEnt** act_ents;
    xEnv* env;
    xMemPool mempool;
    xSceneResolvIDCallback resolvID;
    xSceneBase2NameCallback base2Name;
    xSceneID2NameCallback id2Name;
};

struct xNearFloorPoly
{
    xBox box;
    xVec3 center;
    xVec3 facingVec;
    F32 facingDot;
    F32 floorDot;
    xVec3 vert[3];
    F32 neardist;
    xVec3 nearpt;
    S32 nearvert;
    S32 nearedge;
    U32 oid;
    void* optr;
    xModelInstance* mptr;
};

extern xScene* g_xSceneCur;

void xSceneInit(xScene* sc, U16 num_trigs, U16 num_stats, U16 num_dyns, U16 num_npcs);
void xSceneExit(xScene* sc);
void xSceneSave(xScene* ent, xSerial* s);
void xSceneLoad(xScene* ent, xSerial* s);
void xSceneSetup(xScene* sc);
void xSceneAddEnt(xScene* sc, xEnt* ent);
xBase* xSceneResolvID(xScene* sc, U32 id);
void xSceneForAllEnts(xScene* sc, xSceneEntCallback func, void* data);
void xRayHitsGrid(xGrid* grid, xScene* sc, xRay3* r, xRayEntCallback rentcb, xQCData* qcr, void* data);
void xRayHitsTikiLandableEnt(xScene*, xRay3* r, xQCData* qcr, xEnt* ent, void* colldata); // UNUSED
void xRayHitsEnt(xScene*, xRay3* r, xQCData* qcr, xEnt* ent, void* colldata);
void xRayHitsScene(xScene* sc, xRay3* r, xCollis* coll);
void xRayHitsSceneFlags(xScene* sc, xRay3* r, xCollis* coll, U8 collType, U8 chk);
void xSphereHitsScene(xScene* sc, const xSphere* sphere, xCollis* coll, U8 collType, U8 chk); // UNUSED
U32 xSceneNearestFloorPoly(xScene* sc, xNearFloorPoly* nfpoly, U8 collType, U8 chk);

#endif
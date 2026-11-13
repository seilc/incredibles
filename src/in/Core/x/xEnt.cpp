#include "xEnt.h"
#include "xMemMgr.h"
#include "xMath3.h"
#include "xEvent.h"
#include "xutil.h"
#include "xGrid.h"
#include "zBase.h"
#include "zDestructible.h"
#include "zPlayer.h"
#include "xGlobals.h"
#include "xSurface.h"
#include "zSurface.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zCollGeom.h"
#include "xCollide.h"
#include "xstransvc.h"
#include "xString.h"
#include "xMath.h"
#include "xserializer.h"
#include "xLinkAsset.h"
#include "xFX.h"
#include "iModel.h"
#include "xShadow.h"
#include "xFFX.h"
#include "zGrid.h"
#include "zMain.h"
#include "zLight.h"

#include "decomp.h"

#include <string.h>
#include <stdio.h>

#define TURRET_HOLDABLE_DEATH_SIMP_ID 0x3159B667 // xStrHash("TURRET_HOLDABLE_DEATH_SIMP")
#define S08_TURRET_HOLDABLE_DEATH_SIMP_ID 0x36A1171F // xStrHash("S08_TURRET_HOLDABLE_DEATH_SIMP")

#define ANIM_COLL_SETUP()                       \
    xModelInstance& model = *ent.model;         \
    xMat4x3& mat = *(xMat4x3*)model.Mat;        \
    xEnt::anim_coll_data& ac = *ent.anim_coll;

char* g_xEnt_FMT = "";
static F32 nsn_angle = PI / 6.0f;
static xBox all_ents_box;
static S32 all_ents_box_init;

namespace
{
    inline void HackHoldableTurret(xEnt* ent)
    {
        if (ent->id == TURRET_HOLDABLE_DEATH_SIMP_ID || ent->id == S08_TURRET_HOLDABLE_DEATH_SIMP_ID) {
            ent->moreFlags &= (U16)~k_MORE_FLAGS_ANIM_COLL;
            ent->collLev = k_XENT_COLLLEV_OBB;
            ent->bound.box.box.upper.assign(1.6f, 8.0f, 1.3f);
            ent->bound.box.box.lower.assign(-1.6f, 0.0f, -1.3f);
        }
    }

    namespace anim_coll
    {
        void reset(xEnt& ent)
        {
            if (!ent.anim_coll) {
                ent.anim_coll = (xEnt::anim_coll_data*)xMEMALLOC(sizeof(xEnt::anim_coll_data), 0);
                ent.anim_coll->flags = 0;
                ent.anim_coll->verts = NULL;
            }

            ANIM_COLL_SETUP();

            if (!(ac.flags & 0x8)) {
                switch (model.BoneCount) {
                case 0:
                    break;
                case 1:
                    ac.flags |= 0x1;
                    ac.old_mat = mat;
                    ac.new_mat = g_I3;
                    break;
                default:
                    ac.flags |= 0x2;
                    ac.old_mat = mat;
                    ac.new_mat = g_I3;

                    xModelAnimCollStart(model);

                    xBox& box = ent.bound.box.box;
                    xVec3 size = box.upper - box.lower;
                    F32 max_size = size.x;
                    if (max_size < size.y) max_size = size.y;
                    if (max_size < size.z) max_size = size.z;
                    max_size += 1.0f;
                    box.upper += max_size;
                    box.lower -= max_size;
                    model.Data->boundingSphere.radius *= 3.0f;
                }
            }
        }

        void refresh(xEnt& ent)
        {
            ANIM_COLL_SETUP();
            xMat4x3& bone_mat = ((xMat4x3*)model.Mat)[1];
            xMat4x3Mul(&mat, &bone_mat, &ac.old_mat);
            ac.new_mat = bone_mat;
            bone_mat = g_I3;
        }

        inline void pre_move(xEnt& ent)
        {
            ANIM_COLL_SETUP();
            ent.frame->mat = mat = ac.old_mat;
        }

        inline void post_move(xEnt& ent)
        {
            ANIM_COLL_SETUP();
            ac.old_mat = ent.frame->mat;
            xMat4x3Mul(&mat, &ac.new_mat, &ac.old_mat);
            ent.frame->mat = mat;
        }
    }
}

void xEntSceneInit()
{
    all_ents_box_init = TRUE;
}

void xEntSceneExit()
{
}

static void xEntAddHittableFlag(xEnt* ent)
{
    if (ent->baseType == eBaseTypeNPC || ent->baseType == eBaseTypeDestructObj ||
        ent->baseType == eBaseTypeButton || ent->baseType == eBaseTypeBoulder ||
        (ent->baseType == eBaseTypePlatform && ent->subType == 12)) {
        ent->moreFlags |= 0x10;
        return;
    }

    for (U32 i = 0; i < ent->linkCount; i++) {
        if (ent->link[i].srcEvent == eEventHit || ent->link[i].srcEvent == eEventHit_Melee ||
            ent->link[i].srcEvent == eEventHit_BubbleBounce || ent->link[i].srcEvent == eEventHit_BubbleBash ||
            ent->link[i].srcEvent == eEventHit_BubbleBowl || ent->link[i].srcEvent == eEventHit_Cruise ||
            ent->link[i].srcEvent == eEventHit_PatrickSlam || ent->link[i].srcEvent == eEventHit_Throw ||
            ent->link[i].srcEvent == eEventHit_PaddleLeft || ent->link[i].srcEvent == eEventHit_PaddleRight ||
            ent->link[i].srcEvent == eEventHitPatrickCartwheel) {
            ent->moreFlags |= 0x10;
            return;
        }
    }
}

static void hack_receive_shadow(xEnt* ent)
{
    static U32 receive_models[15] = {
        0x923EE315,
        0x923EE316,
        0x923EE317,
        0x923EE318,
        0x923EE319,
        0x923EE31A,
        0x923EE31B,
        0x923EE31C,
        0x923EE31D,
        0x923EE31E,
        0x923EE31F,
        0x923EE320,
        0x923EE321,
        0x923EE323,
        0x923EE324,
    };

    const U32* end_receive_models = receive_models + ARRAY_LENGTH(receive_models);

    for (U32* id = receive_models; id != end_receive_models; id++) {
        if (ent->asset->modelInfoID == *id) {
            ent->baseFlags |= k_XBASE_RECEIVES_SHADOWS;
            ent->asset->baseFlags |= k_XBASE_RECEIVES_SHADOWS;
            break;
        }
    }
}

static void xEntAddShadowRecFlag(xEnt* ent)
{
    switch (ent->baseType) {
    case eBaseTypePlatform:
    case eBaseTypeStatic:
    case eBaseTypeButton:
    case eBaseTypeDestructObj:
    case eBaseTypeNPC:
    case eBaseTypeBoulder:
        if (ent->model && (ent->model->Pipe.Flags & (eSRCBLEND_MASK | eDESTBLEND_MASK))) {
            ent->baseFlags &= (U16)~k_XBASE_RECEIVES_SHADOWS;
        }
        break;
    default:
        ent->baseFlags &= (U16)~k_XBASE_RECEIVES_SHADOWS;
    }

    hack_receive_shadow(ent);
}

void xEntInit(xEnt* ent, xEntAsset* asset)
{
    xBaseInit(ent, asset);

    ent->asset = asset;
    ent->update = xEntUpdate;
    ent->bupdate = xEntDefaultBoundUpdate;
    ent->render = xEntRender;
    ent->move = NULL;
    ent->transl = xEntDefaultTranslate;
    ent->flags = asset->flags;
    ent->miscflags = 0;
    ent->moreFlags = asset->moreFlags;
    ent->subType = asset->subtype;
    ent->pflags = asset->pflags;
    ent->ffx = NULL;
    ent->num_ffx = 0;
    ent->driver = NULL;
    ent->driven = NULL;
    ent->model = NULL;
    ent->collModel = NULL;
    ent->camcollModel = NULL;
    ent->frame = NULL;
    ent->collis = NULL;
    ent->simpShadow = NULL;
    ent->entShadow = NULL;
    ent->baseFlags |= k_XBASE_IS_ENTITY;

    xGridBoundInit(&ent->gridb, ent);

    ent->anim_coll = NULL;

    xEntGrowGlobalBoundingBox(&asset->pos);
}

void xEntGrowGlobalBoundingBox(const xVec3* pos)
{
    if (all_ents_box_init) {
        iBoxInitBoundVec(&all_ents_box, pos);
        all_ents_box_init = FALSE;
    } else {
        iBoxBoundVec(&all_ents_box, &all_ents_box, pos);
    }
}

void xEntInitForType(xEnt* ent)
{
    ent->update = xEntUpdate;
    ent->render = xEntRender;

    switch (ent->collType) {
    case k_XENT_COLLTYPE_TRIG:
        ent->pflags &= ~(k_XENT_IS_MOVING | k_XENT_HAS_VELOCITY);
        ent->chkby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        ent->penby = 0;
        break;
    case k_XENT_COLLTYPE_STAT:
        ent->pflags &= ~(k_XENT_IS_MOVING | k_XENT_HAS_VELOCITY);
        ent->chkby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        ent->penby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        break;
    case k_XENT_COLLTYPE_DYN:
        ent->pflags |= k_XENT_IS_MOVING;
        ent->move = NULL;
        ent->frame = (xEntFrame*)xMEMALLOC(sizeof(xEntFrame), 0);
        memset(ent->frame, 0, sizeof(xEntFrame));
        ent->pflags &= (U8)~k_XENT_HAS_VELOCITY;
        ent->chkby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        ent->penby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        break;
    case k_XENT_COLLTYPE_NPC:
        ent->pflags |= k_XENT_IS_MOVING;
        ent->move = NULL;
        ent->pflags |= k_XENT_HAS_VELOCITY;
        ent->chkby = k_XENT_COLLTYPE_PC;
        ent->penby = k_XENT_COLLTYPE_PC;
        break;
    case k_XENT_COLLTYPE_PC:
        ent->pflags |= k_XENT_IS_MOVING;
        ent->move = NULL;
        ent->frame = (xEntFrame*)xMEMALLOC(sizeof(xEntFrame), 0);
        memset(ent->frame, 0, sizeof(xEntFrame));
        ent->pflags |= k_XENT_HAS_VELOCITY;
        ent->chkby = 0;
        ent->penby = 0;
        ent->collis = (xEntCollis*)xMEMALLOC(sizeof(xEntCollis), 0);
        ent->collis->chk = k_XENT_COLLTYPE_TRIG | k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_ENV;
        ent->collis->pen = k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_ENV;
        ent->collis->post = NULL;
        ent->collis->depenq = NULL;
        break;
    }

    if ((ent->moreFlags & k_MORE_FLAGS_ANIM_COLL || ent->flags & 0x2) && !ent->frame) {
        ent->frame = (xEntFrame*)xMEMALLOC(sizeof(xEntFrame), 0);
        memset(ent->frame, 0, sizeof(xEntFrame));
    }

    ent->baseFlags |= k_XBASE_IS_ENTITY;
}

namespace
{
    F32 get_lower_extent(const xBound& bound)
    {
        switch (bound.type) {
        case k_XBOUNDTYPE_SPHERE:
            return bound.sph.r;
        case k_XBOUNDTYPE_BOX:
            return bound.box.center.y - bound.box.box.lower.y;
        case k_XBOUNDTYPE_OBB:
        {
            xMat4x3& mat = *bound.mat;
            if (0.0f == mat.up.x && 0.0f == mat.up.z) {
                F32 y = mat.up.y * bound.box.box.lower.y + mat.pos.y;
                return bound.box.center.y - y;
            }

            xBox abox;
            xBoundGetBox(abox, bound);
            return bound.box.center.y - abox.lower.y;
        }
        }

        return 0.0f;
    }

    bool collide_downward(xVec3& loc, xEnt*& hit, xScene& s, xEnt& ent, F32 max_dist)
    {
        F32 lower_bound_extent = get_lower_extent(ent.bound);
        xVec3& center = *xBoundCenter(&ent.bound);

        xRay3 ray;
        ray.origin = center;
        ray.dir.assign(0.0f, -1.0f, 0.0f);
        ray.min_t = 0.0f;
        ray.max_t = max_dist + lower_bound_extent;
        ray.flags = 0xc00;

        U8 old_bound_type = ent.bound.type;
        F32 old_bound_radius = ent.bound.sph.r;
        ent.bound.type = k_XBOUNDTYPE_SPHERE;
        ent.bound.sph.r = 0.0f;

        xCollis coll;
        center.y = 1000.0f;
        coll.flags = 0x100;
        xRayHitsSceneFlags(&s, &ray, &coll, k_XENT_COLLTYPE_PC, k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_ENV);
        ent.bound.type = old_bound_type;
        ent.bound.sph.r = old_bound_radius;
        center.y = ray.origin.y;

        if (coll.flags & 0x1) {
            loc = ((xMat4x3*)ent.model->Mat)->pos;
            loc.y -= coll.dist - lower_bound_extent;
            hit = (xEnt*)coll.optr;
            return true;
        }

        xSphere* spheres[5];
        S32 count = ZPLAYER->GetCollisionSpheres((const xSphere**)spheres);
        S32 i;
        for (i = 0; i < count; i++) {
            xBound b;
            xBoundFromSphere(b, *spheres[i]);
            xBoundHitsBound(&ent.bound, &b, &coll);
            if (coll.flags & 0x1) {
                break;
            }
        }

        if (i != count) {
            zEntEvent(&ent, eEventHit, 1000000.0f, 0.0f, 0.0f, 0.0f);
            loc = ((xMat4x3*)ent.model->Mat)->pos;
            loc.y -= coll.dist - lower_bound_extent;
            hit = ZPLAYER;
            return true;
        }

        return false;
    }

    void drop_stacked_entity(xEnt& ent);

    void stacked_owner_destroyed(zDestructible*, void* context)
    {
        drop_stacked_entity(*(xEnt*)context);
    }

    void mount_stacked_entity(xEnt& ent, xEnt& driver)
    {
        if (ent.driver)
            ent.driver->driving_count--;
        ent.driver = &driver;
        driver.driving_count++;

        driver.driven = &ent;
        if (driver.pflags & k_XENT_HAS_GRAVITY) {
            driver.driven->pflags = k_XENT_HAS_GRAVITY;
        }

        zDestructible* durst = zDestructibleGetFromEntity(&driver);
        if (durst) {
            durst->destroy_notify = stacked_owner_destroyed;
            durst->notify_context = &ent;
        }
    }

    void drop_driven_stacked_entity(xEnt& ent)
    {
        if (ent.driven && ent.driven->driver) {
            ent.driven->driver->driving_count--;
            ent.driven->driver = NULL;
        }

        if (ent.driven) {
            ent.driven->pflags = k_XENT_HAS_GRAVITY;
            if (ent.driven->driver)
                drop_driven_stacked_entity(*ent.driven->driver);
        }
    }

    void dismount_stacked_entity(xEnt& ent)
    {
        if (!ent.driver) {
            return;
        }

        zDestructible* durst = zDestructibleGetFromEntity(ent.driver);
        if (durst) {
            durst->destroy_notify = NULL;
            durst->notify_context = NULL;
        }

        if (ent.driver->driving_count) {
            --ent.driver->driving_count;
        }

        ent.driver = NULL;

        drop_driven_stacked_entity(ent);
    }

    void setup_stacked_entity(xEnt& ent)
    {
        ent.pflags = k_XENT_HAS_GRAVITY;
    }

    void drop_stacked_entity(xEnt& ent)
    {
        ent.pflags = k_XENT_HAS_GRAVITY;
        dismount_stacked_entity(ent);
    }

    void stop_stacked_entity(xEnt& ent)
    {
        ent.pflags = 0;
    }

    void update_stacked_entity(xScene& s, xEnt& ent, F32 dt)
    {
        if (ent.driver && (ent.driver->pflags & k_XENT_HAS_GRAVITY)) {
            return;
        }

        xEntApplyPhysics(&ent, &s, dt);
        xEntMove(&ent, &s, dt);
        F32 dist = ((xMat4x3*)ent.model->Mat)->pos.y - ent.frame->mat.pos.y;
        if (dist <= 0.0f) {
            return;
        }

        xVec3 hitloc;
        xEnt* hitent;
        if (!collide_downward(hitloc, hitent, s, ent, dist)) {
            return;
        }

        ent.frame->mat.pos.y = hitloc.y;
        stop_stacked_entity(ent);
        if (hitent)
            mount_stacked_entity(ent, *hitent);
    }
}

void xEntSetup(xEnt* ent)
{
    xBaseSetup(ent);

    ent->baseFlags |= k_XBASE_IS_ENTITY;

    if (ent->asset->surfaceID) {
        xSurface* surf = (xSurface*)xSceneResolvID(g_xSceneCur, ent->asset->surfaceID);
        if (surf) {
            surf->type = 1;
            surf->ent = ent;

            xModelInstance* minst = ent->model;
            while (minst) {
                bool isScreen = (ent->model->modelID == xStrHash("monitor_a_half_ci.dff") ||
                                 ent->model->modelID == xStrHash("computer_screen_c_rs.dff") ||
                                 ent->model->modelID == xStrHash("computer_screen_d_rs.dff")) ? true : false;
                if (!isScreen || (minst->Pipe.Flags & eMATFX_MASK)) {
                    minst->Surf = surf;
                }
                minst = minst->Next;
            }
        }
    }

    for (S32 i = 0; i < ent->linkCount; i++) {
        const xLinkAsset* la = &ent->link[i];
        if (la->dstEvent == eEventDrivenby) {
            xEnt* dent = (xEnt*)xSceneResolvID(g_xSceneCur, la->dstAssetID);
            if (dent) {
                ent->driver = dent;
                if ((S32)la->param[0]) {
                    ent->driveInfo = (xEntDriveInfo*)xMEMALLOC(sizeof(xEntDriveInfo), 0);

                    xMat4x3 invMat;
                    RwMatrixUpdate((RwMatrix*)&dent->frame->mat);
                    RwMatrixInvert((RwMatrix*)&invMat, (RwMatrix*)&dent->frame->mat);
                    xMat4x3Mul(&ent->driveInfo->driveMat, (xMat4x3*)ent->model->Mat, &invMat);

                    xMat3x3RMulVec(&ent->driveInfo->centerOffset, &invMat, xEntGetCenter(ent));
                    xVec3AddTo(&ent->driveInfo->centerOffset, &invMat.pos);
                    ent->driveInfo->flags = 1;

                    if (ent->id == xStrHash("S03E_CRANE_CABLE_PLAT_01_DRV")) {
                        ent->driveInfo->flags |= 0x2;
                    }

                    if (ent->id == xStrHash("S03E_CRANE_CABLE_PLAT_01_VIS")) {
                        ent->driveInfo->flags |= 0x8;
                    }
                } else {
                    ent->driveInfo = NULL;
                }

                dent->driving_count++;
            }
        }
    }

    ent->model->RedMultiplier = ent->asset->redMult;
    ent->model->GreenMultiplier = ent->asset->greenMult;
    ent->model->BlueMultiplier = ent->asset->blueMult;
    ent->model->Alpha = ent->asset->seeThru;

    xEntAddHittableFlag(ent);
    xEntAddShadowRecFlag(ent);

    zCollGeom_EntSetup(ent);

    if (ent->model) {
        if (ent->bound.type == k_XBOUNDTYPE_BOX) {
            xBoxForModel(ent->bound.box.box, ent->collModel ? ent->collModel : ent->model, true);
        } else if (ent->bound.type == k_XBOUNDTYPE_OBB) {
            xBoxForModelLocal(ent->bound.box.box, ent->collModel ? ent->collModel : ent->model, true);
        }
    }

    if (ent->moreFlags & k_MORE_FLAGS_ANIM_COLL) {
        anim_coll::reset(*ent);
    }

    HackHoldableTurret(ent);

    if (ent->flags & 0x2) {
        setup_stacked_entity(*ent);
    }
}

void xEntSave(xEnt* ent, xSerial* s)
{
    xBaseSave(ent, s);

    if (ent->flags & k_XENT_IS_VISIBLE)
        s->Write_b1(1);
    else
        s->Write_b1(0);
}

void xEntLoad(xEnt* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    S32 b = 0;
    s->Read_b1(&b);
    if (b)
        xEntShow(ent);
    else
        xEntHide(ent);
}

void xEntReset(xEnt* ent)
{
    xBaseReset(ent, ent->asset);

    ent->baseFlags |= k_XBASE_IS_ENTITY;
    ent->flags = ent->asset->flags;
    ent->miscflags = 0;
    ent->moreFlags = ent->asset->moreFlags;

    xEntAddHittableFlag(ent);
    xEntAddShadowRecFlag(ent);

    xMat4x3 frame;
    if (!ent->driver || !ent->driver->frame || !ent->driveInfo || gInSceneResetOrInit) {
        xMat3x3Euler(&frame, ent->asset->ang.x, ent->asset->ang.y, ent->asset->ang.z);

        xVec3SMulBy(&frame.right, ent->asset->scale.x);
        xVec3SMulBy(&frame.up, ent->asset->scale.y);
        xVec3SMulBy(&frame.at, ent->asset->scale.z);

        xVec3Copy(&frame.pos, &ent->asset->pos);
    } else {
        xMat4x3Mul(&frame, &ent->driveInfo->driveMat, &ent->driver->frame->mat);
        if (ent->driver->driving_reset) {
            ent->driveInfo->flags |= 0x4;
        }
    }

    frame.flags = 0;

    if (ent->model) {
        xModelSetFrame(ent->model, &frame);

        if (ent->collModel) {
            xModelSetFrame(ent->collModel, &frame);
        }

        if (ent->bound.type == k_XBOUNDTYPE_BOX) {
            xBoxForModel(ent->bound.box.box, ent->collModel ? ent->collModel : ent->model, true);
        } else if (ent->bound.type == k_XBOUNDTYPE_OBB) {
            xBoxForModelLocal(ent->bound.box.box, ent->collModel ? ent->collModel : ent->model, true);
        }

        if (ent->moreFlags & k_MORE_FLAGS_ANIM_COLL) {
            anim_coll::reset(*ent);
        }

        HackHoldableTurret(ent);

        xModelInstance* minst = ent->model;
        while (minst) {
            minst->RedMultiplier = ent->asset->redMult;
            minst->GreenMultiplier = ent->asset->greenMult;
            minst->BlueMultiplier = ent->asset->blueMult;
            minst->Alpha = minst->Data->geometry->matList.materials[0]->color.alpha / 255.0f;
            minst->Scale.x = 0.0f;
            minst->Scale.y = 0.0f;
            minst->Scale.z = 0.0f;
            minst = minst->Next;
        }

        minst = ent->model;
        xBox* combinedAnimBound = &minst->combinedAnimBound;
        combinedAnimBound->upper.x = -1e38f;
        while (minst) {
            U16 old_flags = minst->Flags;
            minst->Flags |= 0x1;
            xModelEvalSingle(minst, combinedAnimBound);
            minst->Flags = old_flags;
            minst = minst->Next;
        }
    }

    if (ent->frame) {
        xMat4x3Copy(&ent->frame->mat, &frame);

        if (!ent->driving_count || gInSceneResetOrInit) {
            ent->frame->oldmat = ent->frame->mat;
        } else {
            ent->driving_reset = 1;
        }

        xVec3Copy(&ent->frame->setDpos(), &g_O3);
        xVec3Copy(&ent->frame->dvel, &g_O3);
        xVec3Copy(&ent->frame->vel, &g_O3);
        xVec3Copy(&ent->frame->oldvel, &g_O3);

        xVec3Copy(&ent->frame->rot.axis, &ent->asset->ang);
        ent->frame->rot.angle = 0.0f;
        xRotCopy(&ent->frame->oldrot, &ent->frame->rot);
    }

    if (ent->bupdate && ent->model) {
        ent->bupdate(ent, &((xMat4x3*)ent->model->Mat)->pos);
    }

    if (ent->flags & 0x2) {
        setup_stacked_entity(*ent);
    }

    if (ent->baseType == eBaseTypeStatic && ent->visUpdate) {
        ent->visUpdate(ent);
    }
}

// UNUSED: needed to match (speculative code)
void xEntLoadModelFromDisk(xEnt* ent, const char* filename, xMemPool*)
{
    char fullfilename[80];
    sprintf(fullfilename, "%s.DFF", filename);

    RpAtomic* imodel;
    U32 modelID = xStrHash(fullfilename);
    imodel = (RpAtomic*)xSTFindAsset(modelID, NULL);

    ent->model = xModelInstanceAlloc(imodel, ent, 0, 0, NULL);

    while (imodel = iModelFile_RWMultiAtomic(imodel)) {
        xModelInstanceAttach(xModelInstanceAlloc(imodel, ent, 0x8, 0, NULL), ent->model);
    }
}

DECOMP_FORCEACTIVE("%s.SKA", "ent-table")

xModelInstance* xEntLoadModel(xEnt* ent, RpAtomic* imodel, xModelInstance* reuse_instance)
{
    xModelInstance* model = xModelInstanceAllocReuse(imodel, ent, 0, 0, NULL, reuse_instance);

    while (imodel = iModelFile_RWMultiAtomic(imodel)) {
        xModelInstanceAttach(xModelInstanceAlloc(imodel, ent, 0x8, 0, NULL), model);
    }

    if (ent) {
        ent->model = model;
    }

    return model;
}

DECOMP_FORCEACTIVE("idle", "anim-file", "anim-table")

static void xEntAddToPos(xEnt* ent, const xVec3* dpos)
{
    xVec3AddTo(&ent->frame->mat.pos, dpos);
}

static S32 setMaterialTextureRestore;

inline RwTexture* LocateSurfTexture_TexAnim(zSurfaceProps* surfProp, S32 idx_pass)
{
    RwTexture* txtr = NULL;
    U32 idx_grpItem = surfProp->texanim[idx_pass].group_idx;

    if (surfProp->texanim[idx_pass].txtr_animList) {
        txtr = surfProp->texanim[idx_pass].txtr_animList[idx_grpItem];
    }

    return txtr;
}

void xEntSetupPipeline(xModelInstance* model)
{
    xEntSetupPipeline(model->Surf, model->Data);
}

void xEntSetupPipeline(xSurface* surf, RpAtomic* model)
{
    S32 setPipeline = FALSE;
    setMaterialTextureRestore = FALSE;
    setPipeline = FALSE;

    if (!surf) return;

    if (!surf->moprops) {
        return;
    }

    zSurfaceProps* pp = (zSurfaceProps*)surf->moprops;

    if (pp->texanim_flags & SURF_TEXANIM_ON) {
        S32 tdx = 0;
        RwTexture* texptr = LocateSurfTexture_TexAnim(pp, tdx);
        if (texptr) {
            iModelSetMaterialTexture(model, texptr);
            setMaterialTextureRestore = TRUE;
        }
    }

    {
        RwTexture* texptr = NULL;
        if (pp->texanim_flags & SURF_TEXANIM_ON2) {
            S32 tdx = 1;
            texptr = LocateSurfTexture_TexAnim(pp, 1);
        }
        else if (pp->uvfx_flags & UVANIM_FLAG_ON2) {
            texptr = pp->txtr_matFXDualMap;
        }

        xFXanimUV2PSetTexture(texptr);
        if (texptr) {
            setPipeline = TRUE;
        }
    }

    if (pp->uvfx_flags & (UVANIM_FLAG_ON | UVANIM_FLAG_ON2)) {
        setPipeline = TRUE;
        xFXanimUVSetTranslation(&pp->uvfx[0].trans);
        xFXanimUV2PSetTranslation(&pp->uvfx[1].trans);

        xFXanimUVSetScale(&pp->uvfx[0].scale);
        xFXanimUV2PSetScale(&pp->uvfx[1].scale);

        xFXanimUVSetAngle((PI / 180.0f) * pp->uvfx[0].rot);
        xFXanimUV2PSetAngle((PI / 180.0f) * pp->uvfx[1].rot);
    }

    if (setPipeline) {
        bool dualPass = (pp->uvfx_flags & UVANIM_FLAG_ON2) ? true : false;
        xFXanimUVAtomicSetup(model, dualPass);
    }
}

void xEntRestorePipeline(xModelInstance* model)
{
    xEntRestorePipeline(model->Surf, model->Data);
}

void xEntRestorePipeline(xSurface*, RpAtomic* model)
{
    if (setMaterialTextureRestore) {
        iModelResetMaterial(model);
        setMaterialTextureRestore = FALSE;
    }
}

void xEntRender(xEnt* ent)
{
    if (!ent->model)
        return;
    if (!xEntIsVisible(ent))
        return;

    if (ent->model->Flags & 0x400) {
        return;
    }

    ent->_isCulled = 0;

    if (ent->baseType == eBaseTypePlayer || (ent->baseType == eBaseTypeNPC && !(ent->flags & 0x40))) {
        S32 shadowOutside;
        xVec3 shadVec;
        shadVec.x = ((xMat4x3*)ent->model->Mat)->pos.x;
        shadVec.y = ((xMat4x3*)ent->model->Mat)->pos.y - 10.0f;
        shadVec.z = ((xMat4x3*)ent->model->Mat)->pos.z;
        if (xModelCullGroupPlusShadow(ent->model, 10.0f, &shadowOutside)) {
            if (shadowOutside) {
                ent->_isCulled = 1;
                return;
            }
            goto shadow;
        }
    } else if (xModelCullGroup(ent->model)) {
        ent->_isCulled = 1;
        return;
    }

    xModelRender(ent->model);

shadow:
    if ((ent->baseType == eBaseTypeNPC && ent->entShadow && !(ent->flags & 0x40)) || ent->baseType == eBaseTypePlayer) {
        zLightAddLocal(ent);
        xShadow_ListAdd(ent);
    }
}

void xEntUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    xEntBeginUpdate(ent, sc, dt);

    if (ent->pflags & k_XENT_HAS_VELOCITY) {
        xEntApplyPhysics(ent, sc, dt);
    }

    if (ent->pflags & k_XENT_IS_MOVING) {
        xEntMove(ent, sc, dt);
    }

    xFFXApply(ent, sc, dt);

    if (ent->collis) {
        xEntCollide(ent, sc, dt);
    }

    if ((ent->flags & 0x2) && (ent->pflags & k_XENT_HAS_GRAVITY)) {
        update_stacked_entity(*sc, *ent, dt);
    }

    xEntEndUpdate(ent, sc, dt);
}

void xEntBeginUpdate(xEnt* ent, xScene*, F32 dt)
{
    if (!ent->model)
        return;
    xModelUpdate(ent->model, dt);

    if (!ent->frame)
        return;
    xVec3Copy(&ent->frame->oldvel, &ent->frame->vel);
    ent->frame->oldmat = ent->frame->mat;
    ent->driving_reset = 0;
    xRotCopy(&ent->frame->oldrot, &ent->frame->rot);
    xMat4x3Copy(&ent->frame->mat, xModelGetFrame(ent->model));
    ent->frame->mode = 0;
}

void xEntEndUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    if (!ent->model)
        return;
    if (ent->frame) {
        if (!(ent->frame->mode & 0x20000)) {
            xMat3x3Copy((xMat3x3*)ent->model->Mat, &ent->frame->mat);
        }

        if (!(ent->frame->mode & 0x10000)) {
            xVec3* mpos = &((xMat4x3*)ent->model->Mat)->pos;
            xVec3* upos = &ent->frame->mat.pos;
            xVec3Copy(mpos, upos);
        }

        if (ent->collModel && ent->collModel != ent->model)
            *ent->collModel->Mat = *ent->model->Mat;
        if (ent->camcollModel && ent->camcollModel != ent->model) {
            *ent->camcollModel->Mat = *ent->model->Mat;
        }
    }

    xModelEval(ent->model);

    if (ent->bupdate) {
        ent->bupdate(ent, &((xMat4x3*)ent->model->Mat)->pos);
    }

    if (ent->moreFlags & k_MORE_FLAGS_ANIM_COLL) {
        anim_coll::refresh(*ent);
    }

    if (ent->endUpdate)
        ent->endUpdate(ent, sc, dt);
}

void xEntDefaultBoundUpdate(xEnt* ent, xVec3* pos)
{
    xBound* bound = &ent->bound;

    if (bound->type == k_XBOUNDTYPE_SPHERE) {
        xVec3Copy(&bound->sph.center, pos);
        bound->sph.center.y += 0.7f;
        bound->sph.r = 0.7f;
    } else if (bound->type == k_XBOUNDTYPE_BOX) {
    } else if (bound->type == k_XBOUNDTYPE_OBB) {
        (void)0;
    }

    xBoundUpdate(bound);
    zGridUpdateEnt(ent);
}

void xEntDefaultTranslate(xEnt* ent, xVec3* dpos, xMat4x3* dmat)
{
    if (dmat) {
        if (ent->model) {
            xMat4x3Mul((xMat4x3*)ent->model->Mat, (xMat4x3*)ent->model->Mat, dmat);
        }

        if (ent->frame) {
            xMat4x3Mul(&ent->frame->mat, &ent->frame->mat, dmat);
        }

        xMat4x3Toworld(xEntGetCenter(ent), dmat, xEntGetCenter(ent));
    } else {
        if (ent->model) {
            xVec3AddTo(xEntGetPos(ent), dpos);
        }

        if (ent->frame) {
            xVec3AddTo(&ent->frame->mat.pos, dpos);
        }

        xVec3AddTo(xEntGetCenter(ent), dpos);
    }
}

static void xEntRotationToMatrix(xEntFrame* frame)
{
    if (frame->mode & 0x20) {
        if (frame->mode & 0x400) {
            xVec3AddTo(&frame->rot.axis, &frame->drot.axis);
            xMat3x3Euler(&frame->mat, frame->rot.axis.x, frame->rot.axis.y, frame->rot.axis.z);
        } else {
            frame->rot.angle = xAngleClamp(frame->rot.angle + frame->drot.angle);
            xMat3x3Rot(&frame->mat, &frame->rot.axis, frame->rot.angle);
        }

        frame->drot.angle = 0.0f;
    }
}

void xEntMotionToMatrix(xEnt* ent, xEntFrame* frame)
{
    if (frame->mode & 0x1000) {
        xEntRotationToMatrix(frame);
    }

    if (frame->mode & 0x2) {
        if (frame->mode & 0x800) {
            xMat3x3RMulVec(&frame->setDpos(), &frame->mat, &frame->getDpos());
        }

        xEntAddToPos(ent, &frame->getDpos());
        ent->frame->setDpos().assign(0.0f);
    }

    if (frame->mode & 0x8) {
        if (frame->mode & 0x800) {
            xMat3x3RMulVec(&frame->dvel, &frame->mat, &frame->dvel);
        }

        xVec3AddTo(&frame->vel, &frame->dvel);
        ent->frame->dvel.assign(0.0f);
    }

    if (!(frame->mode & 0x1000)) {
        xEntRotationToMatrix(frame);
    }
}

void xEntMove(xEnt* ent, xScene* sc, F32 dt)
{
    if (ent->moreFlags & k_MORE_FLAGS_ANIM_COLL) {
        anim_coll::pre_move(*ent);
    }

    if (ent->move) {
        ent->move(ent, sc, dt, ent->frame);
    }

    xEntMotionToMatrix(ent, ent->frame);

    if (ent->driver && ent->driver->frame && (!ent->driveInfo || (ent->driveInfo->flags & 0x1))) {
        xEntFrame* dframe = ent->driver->frame;
        if (!ent->driveInfo) {
            xVec3 dpos;
            xVec3Sub(&dpos, &dframe->mat.pos, &dframe->oldmat.pos);
            ent->transl(ent, &dpos, NULL);
        } else {
            if (ent->baseType == eBaseTypeStatic) {
                xMat4x3 newMat;
                xMat4x3Mul(&newMat, &ent->driveInfo->driveMat, &dframe->mat);
                if (ent->model) {
                    if (ent->driveInfo->flags & 0x2) {
                        xVec3Copy(&((xMat4x3*)ent->model->Mat)->pos, &newMat.pos);
                    } else {
                        xMat4x3Copy((xMat4x3*)ent->model->Mat, &newMat);
                    }
                }

                if (ent->frame) {
                    if (ent->driveInfo->flags & 0x2) {
                        xVec3Copy(&ent->frame->mat.pos, &newMat.pos);
                    } else {
                        xMat4x3Copy(&ent->frame->mat, &newMat);
                    }
                }

                xMat3x3RMulVec(xEntGetCenter(ent), &dframe->mat, &ent->driveInfo->centerOffset);
                xVec3AddTo(xEntGetCenter(ent), &dframe->mat.pos);
            } else {
                if (!(ent->driveInfo->flags & 0x4)) {
                    xMat4x3 invOldmat, deltaMat;
                    RwMatrixUpdate((RwMatrix*)&dframe->oldmat);
                    RwMatrixInvert((RwMatrix*)&invOldmat, (RwMatrix*)&dframe->oldmat);
                    xMat4x3Mul(&deltaMat, &invOldmat, &dframe->mat);
                    if (ent->driveInfo->flags & 0x2) {
                        xVec3 startPos, dpos;
                        if (ent->frame)
                            xVec3Copy(&startPos, &ent->frame->mat.pos);
                        else if (ent->model)
                            xVec3Copy(&startPos, &((xMat4x3*)ent->model->Mat)->pos);
                        else
                            xVec3Init(&startPos, 0.0f, 0.0f, 0.0f);
                        xMat3x3RMulVec(&dpos, &deltaMat, &startPos);
                        xVec3AddTo(&dpos, &deltaMat.pos);
                        xVec3SubFrom(&dpos, &startPos);
                        ent->transl(ent, &dpos, NULL);
                    } else {
                        ent->transl(ent, NULL, &deltaMat);
                    }
                }

                ent->driveInfo->flags &= ~0x4;
            }
        }
    }

    if (ent->moreFlags & k_MORE_FLAGS_ANIM_COLL)
        anim_coll::post_move(*ent);
}

void xEntApplyPhysics(xEnt* ent, xScene* sc, F32 dt)
{
    xVec3 dposvel = { 0.0f, 0.0f, 0.0f };

    if (ent->pflags & k_XENT_HAS_GRAVITY && !globals.player.bCheatMode) {
        if (sc->flags & 0x1) {
            ent->frame->vel.y += sc->gravity * dt;
        }
    }

    if (ent->pflags & 0x10) {
        if (sc->flags & 0x2) {
            F32 tfric = 1.0f - sc->friction * dt;
            xVec3SMulBy(&ent->frame->vel, tfric);
        }
    }

    if (ent->pflags & k_XENT_HAS_DRAG) {
        if (sc->flags & 0x4) {
            F32 tdrag = 1.0f - sc->drag * dt;
            xVec3SMulBy(&ent->frame->vel, tdrag);
        }
    }

    xVec3Add(&dposvel, &ent->frame->vel, &ent->frame->oldvel);
    xVec3SMulBy(&dposvel, 0.5f * dt);

    if (dposvel.y < 0.0f) {
        F32 dposXZ = xsqrt(xsqr(dposvel.x) + xsqr(dposvel.z));
        F32 scaleXZ = (dposXZ > 0.00001f) ? (0.63f * (30.0f * dt)) / dposXZ : 0.0f;
        F32 scaleY = (0.63f * (30.0f * dt)) / xabs(dposvel.y);
        if (scaleXZ < 1.0f) {
            dposvel.x *= scaleXZ;
            dposvel.z *= scaleXZ;
        }

        if (scaleY < 1.0f) {
            dposvel.y *= scaleY;
        }
    }

    ent->frame->setDpos() += dposvel;
    ent->frame->mode |= 0x2;
}

void xEntCollide(xEnt* ent, xScene* sc, F32 dt)
{
    if (!ent->model) {
        return;
    }

    if (ent->collis->chk & (k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_ENV)) {
        xEntBeginCollide(ent, sc, dt);
    }

    if (ent->collis->chk & k_XENT_COLLTYPE_NPC) {
        xEntCollCheckNPCsByGrid(ent, sc, xEntCollCheckOneEntNoDepen);
    }

    if (ent->collis->chk & (k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN)) {
        xEntCollCheckByGrid(ent, sc, xEntCollCheckOneEntNoDepen);
    }

    if (ent->collis->chk & k_XENT_COLLTYPE_ENV) {
        xEntCollCheckEnv(ent, sc);
    }

    if (ent->collis->chk & (k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_ENV)) {
        F32 sbr = (ent->bound.type == k_XBOUNDTYPE_SPHERE) ? ent->bound.sph.r : 0.7f;
        if (ent->pflags & 0x80) {
            xCollis* coll = &ent->collis->colls[0];
            if (coll->flags & 0x1) {
                F32 h_dot_n = xVec3Dot(&coll->hdng, &coll->norm);
                if (h_dot_n > 0.0f) {
                    xVec3Inv(&coll->norm, &coll->norm);
                    h_dot_n = -h_dot_n;
                }

                F32 depen_len = h_dot_n * coll->dist + sbr;
                if (depen_len < 0.0f || depen_len > sbr) {
                    depen_len = CLAMP(depen_len, 0.0f, sbr);
                }

                xVec3SMul(&coll->depen, &coll->norm, depen_len);
            }
        }

        if (ent->frame->vel.y <= 0.0f) {
            xEntCollideFloor(ent, sc, dt);
        } else {
            xEntCollideCeiling(ent, sc, dt);
        }

        xEntCollideWalls(ent, sc, dt);

        xEntEndCollide(ent, sc, dt);
    }
}

void xEntBeginCollide(xEnt* ent, xScene*, F32)
{
    if (ent->bupdate) {
        ent->bupdate(ent, &ent->frame->mat.pos);
    }

    for (U8 idx = 0; idx < k_XENT_MAX_COLL; idx++) {
        xCollis* coll = &ent->collis->colls[idx];
        coll->flags = 0x1f00;
        coll->optr = NULL;
        coll->mptr = NULL;
        coll->dist = 1e38f;
    }

    ent->collis->idx = 6;

    ent->collis->stat_sidx = 6;
    ent->collis->stat_eidx = 6;
    ent->collis->dyn_sidx = 6;
    ent->collis->dyn_eidx = 6;
    ent->collis->npc_sidx = 6;
    ent->collis->npc_eidx = 6;
    ent->collis->env_sidx = 6;
    ent->collis->env_eidx = 6;
}

void xEntEndCollide(xEnt* ent, xScene* sc, F32 dt)
{
    if (ent->collis->post) {
        ent->collis->post(ent, sc, dt, ent->collis);
    }
}

void xEntCollCheckEnv(xEnt* p, xScene* sc)
{
    p->collis->env_sidx = p->collis->idx;

    xCollis* coll = &p->collis->colls[p->collis->idx];
    coll->flags = 0x1f00;

    U8 ncolls = xSphereHitsEnv3(&p->bound.sph, sc->env, coll, k_XENT_MAX_COLL - p->collis->idx, PI / 4.0f);

    p->collis->idx = p->collis->idx + ncolls;
    p->collis->env_eidx = p->collis->idx;
}

#define NUM_CELL_OFFS 3

static void xEntCollCheckOneGrid(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt, xGrid* grid)
{
    xGridIterator it;
    xEnt* ent;
    xVec3* pcenter = xEntGetCenter(p);
    S32 px, pz;
    xGridBound* cell = xGridIterFirstCell(grid, pcenter->x, pcenter->y, pcenter->z, px, pz, it);
    while (cell) {
        xBound* cellbound = (xBound*)(cell + 1);
        if (xQuickCullIsects(&p->bound.qcd, &cellbound->qcd)) {
            ent = (xEnt*)cell->data;
            hitIt(ent, sc, p);
        }

        cell = xGridIterNextCell(it);
    }

    static S32 offs[4][NUM_CELL_OFFS][2] = {
        { { -1, 0 }, { -1, -1 }, { 0, -1 } },
        { { 0, -1 }, { 1, -1 }, { 1, 0 } },
        { { 1, 0 }, { 1, 1 }, { 0, 1 } },
        { { 0, 1 }, { -1, 1 }, { -1, 0 } },
    };
    static S32 k;

    xBox clbox;
    F32 clcenterx, clcenterz;
    clbox.lower.x = grid->csizex * px, clbox.lower.z = grid->csizez * pz;
    clbox.lower.x += grid->minx, clbox.lower.z += grid->minz;
    clcenterx = 0.5f * grid->csizex, clcenterz = 0.5f * grid->csizez;
    clcenterx += clbox.lower.x, clcenterz += clbox.lower.z;

    if (pcenter->x < clcenterx) {
        if (pcenter->z < clcenterz) {
            k = 0;
        } else {
            k = 1;
        }
    } else {
        if (pcenter->z < clcenterz) {
            k = 3;
        } else {
            k = 2;
        }
    }

    for (S32 i = 0; i < NUM_CELL_OFFS; i++) {
        S32 _x = px + offs[k][i][1];
        S32 _z = pz + offs[k][i][0];
        cell = xGridIterFirstCell(grid, _x, _z, it);
        while (cell) {
            xBound* cellbound = (xBound*)(cell + 1);
            if (xQuickCullIsects(&p->bound.qcd, &cellbound->qcd)) {
                ent = (xEnt*)cell->data;
                hitIt(ent, sc, p);
            }

            cell = xGridIterNextCell(it);
        }
    }

    cell = xGridIterFirstCell(&grid->other, it);
    while (cell) {
        xBound* cellbound = (xBound*)(cell + 1);
        if (xQuickCullIsects(&p->bound.qcd, &cellbound->qcd)) {
            ent = (xEnt*)cell->data;
            hitIt(ent, sc, p);
        }

        cell = xGridIterNextCell(it);
    }
}

void xEntCollCheckByGrid(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt)
{
    p->collis->stat_sidx = p->collis->idx;
    p->collis->dyn_sidx = p->collis->idx;
    xEntCollCheckOneGrid(p, sc, hitIt, &colls_grid);
    xEntCollCheckOneGrid(p, sc, hitIt, &colls_oso_grid);
    p->collis->stat_eidx = p->collis->idx;
    p->collis->dyn_eidx = p->collis->idx;
}

void xEntCollCheckNPCsByGrid(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt)
{
    p->collis->npc_sidx = p->collis->idx;
    xEntCollCheckOneGrid(p, sc, hitIt, &npcs_grid);
    p->collis->npc_eidx = p->collis->idx;
}

S32 xent_entent;

xEnt* xEntCollCheckOneEntNoDepen(xEnt* ent, xScene*, void* data)
{
    xEnt* p = (xEnt*)data;
    xCollis* coll;

    xent_entent = TRUE;

    U32 modl_coll = FALSE;

    if (p->collis->idx >= 15) {
        xent_entent = FALSE;
        return NULL;
    }

    if (!(ent->chkby & p->collType)) {
        xent_entent = FALSE;
        return ent;
    }

    if (ent == p) {
        xent_entent = FALSE;
        return ent;
    }

    if (ent->id == p->id) {
        if (ent == p && ent->collType == k_XENT_COLLTYPE_NPC) {
            (void)0;
        }

        if (ent->baseType != eBaseTypeBoulder && ent->baseType != eBaseTypeProjBomb) {
            xent_entent = FALSE;
            return ent;
        }
    }

    coll = &p->collis->colls[p->collis->idx];

    if (ent->collLev == k_XENT_COLLLEV_MODEL) {
        if (p->collType & (k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC)) {
            modl_coll = TRUE;
        }
    }

    if (modl_coll) {
        coll->flags = 0;
    } else {
        coll->flags = 0x1f00;
    }

    xBoundHitsBound(&p->bound, &ent->bound, coll);

    if (coll->flags & 0x1) {
        xBound tmp;
        xBound* bptr = &p->bound;
        if (modl_coll) {
            coll->flags = 0x1f00;

            U8 ncolls = 0;
            if (p->bound.type == k_XBOUNDTYPE_SPHERE) {
                xModelInstance* collModel = ent->collModel ? ent->collModel : ent->model;
                ncolls = xSphereHitsModel3(&p->bound.sph, collModel, coll, 15 - p->collis->idx, PI / 4.0f);
            } else if (p->bound.type == k_XBOUNDTYPE_BOX || p->bound.type == k_XBOUNDTYPE_OBB) {
                tmp.type = k_XBOUNDTYPE_SPHERE; xVec3* upper = &p->bound.box.box.upper, *lower = &p->bound.box.box.lower;
                xVec3Add(&tmp.sph.center, upper, lower);
                xVec3SMulBy(&tmp.sph.center, 0.5f);
                tmp.sph.r = 0.167f * (upper->x + upper->y + upper->z - lower->x - lower->y - lower->z);

                xModelInstance* collModel = ent->collModel ? ent->collModel : ent->model;
                ncolls = xSphereHitsModel3(&p->bound.sph, collModel, coll, 15 - p->collis->idx, PI / 4.0f);
            }

            for (U8 idx = 0; idx < ncolls; idx++) {
                coll[idx].optr = ent;
                coll[idx].mptr = ent->model;
                p->collis->idx++;
            }

            xent_entent = FALSE;
            return ent;
        }

        coll->oid = ent->id;
        coll->optr = ent;
        coll->mptr = ent->model;

        p->collis->idx++;

        if ((ent->pflags & 0x20) && ent->bound.type == k_XBOUNDTYPE_SPHERE && bptr->type == k_XBOUNDTYPE_SPHERE && coll->hdng.y < -0.866025f) {
            F32 rsum, dx, dy, dz, hsqr;
            rsum = bptr->sph.r + ent->bound.sph.r, dx = bptr->sph.center.x - ent->bound.sph.center.x, dz = bptr->sph.center.z - ent->bound.sph.center.z;
            dy = bptr->sph.center.y - ent->bound.sph.center.y;
            hsqr = xsqr(rsum) - (xsqr(dx) + xsqr(dz));
            if (hsqr >= 0.0f) {
                coll->depen.x = 0.0f;
                coll->depen.y = xsqrt(hsqr) - dy;
                coll->depen.z = 0.0f;
                coll->dist = 0.7f - coll->depen.y;
                coll->hdng.x = 0.0f;
                coll->hdng.y = -1.0f;
                coll->hdng.z = 0.0f;
            }
        }
    }

    xent_entent = FALSE;
    return ent;
}

void xEntCollideFloor(xEnt* p, xScene* sc, F32 dt)
{
    xCollis* coll = &p->collis->colls[0];
    U8 idx;
    xCollis* ml = coll;
    xVec3 motion;
    F32 mlen;
    S32 stepping = FALSE;
    F32 sbr = (p->bound.type == k_XBOUNDTYPE_SPHERE) ? p->bound.sph.r : 0.7f;

    xVec3Copy(&motion, &p->frame->mat.pos);
    xVec3SubFrom(&motion, &p->frame->oldmat.pos);

    motion.y = 0.0f;
    mlen = xVec3Length(&motion);

    for (idx = 6; idx < p->collis->idx; idx++) {
        xCollis* mf = &p->collis->colls[idx];

        if (!(mf->flags & 0x1)) {
            continue;
        }

        xEnt* fent = (xEnt*)mf->optr;

        if (fent) {
            if (!(fent->collType == k_XENT_COLLTYPE_DYN || fent->collType == k_XENT_COLLTYPE_STAT || ((fent->pflags & 0x20) && 0.0f == mf->hdng.x && 0.0f == mf->hdng.z))) {
                continue;
            }

            if (!(p->collis->depenq ? p->collis->depenq(p, fent, sc, dt, mf) : (fent->collType & p->collis->pen) && (fent->penby & p->collType))) {
                continue;
            }
        } else {
            if (!(p->collis->pen & k_XENT_COLLTYPE_ENV)) {
                continue;
            }
        }

        if (mf->dist < ml->dist)
            if (mf->hdng.y < -icos(PI / 3.0f) && (mf->norm.y > icos(nsn_angle) || p->frame->oldmat.pos.y > p->frame->mat.pos.y + dt * (sc->gravity * dt))) {
                ml = mf;
                stepping = FALSE;
            } else if (mlen > 0.001f && mf->hdng.y < 0.65f / sbr - 1.0f && mf->norm.y > icos(nsn_angle)) {
                stepping = TRUE;
                ml = mf;
            }
    }

    if (ml != coll) {
        F32 flr_dist = ml->dist * xabs(ml->hdng.y);

        *coll = *ml;

        if (flr_dist < sbr) {
            ml->flags |= 0x6;

            if (stepping) {
                p->frame->mat.pos.y += 1.5f * dt;
                p->frame->mat.pos.x += ml->depen.x;
                p->frame->mat.pos.z += ml->depen.z;
            } else {
                p->frame->mat.pos.y += ml->depen.y;
            }

            p->frame->vel.y = 0.0f;
        }
    }
}

void xEntCollideCeiling(xEnt* p, xScene* sc, F32 dt)
{
    xCollis* coll = &p->collis->colls[1];
    U8 idx;
    xCollis* ml = coll;
    F32 sbr = (p->bound.type == k_XBOUNDTYPE_SPHERE) ? p->bound.sph.r : 0.7f;

    for (idx = 6; idx < p->collis->idx; idx++) {
        xCollis* mf = &p->collis->colls[idx];

        if (mf->optr) {
            xEnt* fent = (xEnt*)mf->optr;
            if (!(p->collis->depenq ? p->collis->depenq(p, fent, sc, dt, mf) : (fent->collType & p->collis->pen) && (fent->penby & p->collType))) {
                continue;
            }
        } else {
            if (!(p->collis->pen & k_XENT_COLLTYPE_ENV)) {
                continue;
            }
        }

        if (mf->hdng.y > icos(PI / 4.0f) && mf->dist < ml->dist) {
            ml = mf;
        }
    }

    if (ml != coll) {
        F32 ceil_dist = ml->dist * xabs(ml->hdng.y);

        *coll = *ml;
        ml->flags |= 0xa;

        if (ceil_dist < sbr) {
            p->frame->mat.pos.y -= sbr - ceil_dist;
            p->frame->vel.y = 0.0f;
        }
    }
}

void xEntCollideWalls(xEnt* p, xScene* sc, F32 dt)
{
    xCollis* coll;
    xEnt* cent;
    U8 sidx, eidx, idx;

    F32 sbr = (p->bound.type == k_XBOUNDTYPE_SPHERE) ? p->bound.sph.r : 0.7f;

    if (p->collis->pen & k_XENT_COLLTYPE_NPC) {
        sidx = p->collis->npc_sidx;
        eidx = p->collis->npc_eidx;
        for (idx = sidx; idx < eidx; idx++) {
            coll = &p->collis->colls[idx];
            cent = (xEnt*)coll->optr;
            if (!(coll->flags & 0x2) && coll->dist < sbr && (p->collis->depenq ? p->collis->depenq(p, cent, sc, dt, coll) : cent->penby & p->collType)) {
                if (0.0f != coll->depen.x || 0.0f != coll->depen.z) {
                    coll->depen.y = 0.0f;
                }

                xEntAddToPos(p, &coll->depen);
            }
        }
    }

    if (p->collis->pen & k_XENT_COLLTYPE_DYN) {
        sidx = p->collis->dyn_sidx;
        eidx = p->collis->dyn_eidx;
        for (idx = sidx; idx < eidx; idx++) {
            coll = &p->collis->colls[idx];
            cent = (xEnt*)coll->optr;
            if (!(coll->flags & 0x2) && coll->dist < sbr && (p->collis->depenq ? p->collis->depenq(p, cent, sc, dt, coll) : cent->penby & p->collType)) {
                coll->depen.y = 0.0f;
                xEntAddToPos(p, &coll->depen);
            }
        }
    }

    if (p->collis->pen & k_XENT_COLLTYPE_STAT) {
        sidx = p->collis->stat_sidx;
        eidx = p->collis->stat_eidx;
        for (idx = sidx; idx < eidx; idx++) {
            coll = &p->collis->colls[idx];
            cent = (xEnt*)coll->optr;
            if (!(coll->flags & 0x2) && coll->dist < sbr && (p->collis->depenq ? p->collis->depenq(p, cent, sc, dt, coll) : cent->penby & p->collType)) {
                coll->depen.y = 0.0f;
                xEntAddToPos(p, &coll->depen);
            }
        }
    }

    if (p->collis->pen & k_XENT_COLLTYPE_ENV) {
        sidx = p->collis->env_sidx;
        eidx = p->collis->env_eidx;
        for (idx = sidx; idx < eidx; idx++) {
            coll = &p->collis->colls[idx];
            if (!(coll->flags & 0x2) && coll->dist < sbr) {
                coll->depen.y = 0.0f;
                xEntAddToPos(p, &coll->depen);
            }
        }
    }
}

xBox* xEntGetAllEntsBox()
{
    return &all_ents_box;
}

void xEntAnimateCollision(xEnt& ent, bool on)
{
    if (on && !(ent.moreFlags & k_MORE_FLAGS_ANIM_COLL)) {
        ent.moreFlags |= k_MORE_FLAGS_ANIM_COLL;
        if (!ent.frame) {
            ent.frame = (xEntFrame*)xMEMALLOC(sizeof(xEntFrame), 0);
            memset(ent.frame, 0, sizeof(xEntFrame));
        }

        anim_coll::reset(ent);
    }
    else if (!on && (ent.moreFlags & k_MORE_FLAGS_ANIM_COLL))
        ent.moreFlags &= (U8)~k_MORE_FLAGS_ANIM_COLL;
}

bool xEntValidType(U8 type)
{
    return type == eBaseTypeTrigger || type == eBaseTypeVillain || type == eBaseTypePlayer || type == eBaseTypePickup ||
           type == eBaseTypePlatform || type == eBaseTypeDoor || type == eBaseTypeSavePoint || type == eBaseTypeItem ||
           type == eBaseTypeStatic || type == eBaseTypeDynamic || type == eBaseTypeBubble || type == eBaseTypePendulum ||
           type == eBaseTypeHangable || type == eBaseTypeButton || type == eBaseTypeProj || type == eBaseTypeProjBomb ||
           type == eBaseTypeDestructObj || type == eBaseTypeUI || type == eBaseTypeUIFont || type == eBaseTypeProjectileType ||
           type == eBaseTypeEGenerator || type == eBaseTypeNPC || type == eBaseTypeBoulder || type == eBaseTypeTeleportBox ||
           type == eBaseTypeZipLine;
}

bool xEntTurnToFace(xEnt* ent, const xVec3* target, F32 speedLimit, F32 dt)
{
    xVec3 currentFacing = ent->frame->mat.at;
    xVec3Normalize(&currentFacing, &currentFacing);

    F32 dot = xVec3Dot(&currentFacing, target);
    if (dot < 0.9999f) {
        xVec3 axis;
        xVec3Cross(&axis, &currentFacing, target);
        xVec3Normalize(&axis, &axis);

        F32 angle = xacos(dot);
        F32 maxAngle = speedLimit * dt;
        if (angle > maxAngle) {
            angle = maxAngle;
        }

        if (xVec3Dot(&ent->frame->rot.axis, &axis) < 0.0f) {
            angle = -angle;
        }

        ent->frame->drot.axis.assign(0.0f, 0.0f, 0.0f);
        ent->frame->drot.angle = angle;
        ent->frame->mode |= 0x20;

        return false;
    }

    return true;
}

void xEntOpacityInit(xEntOpacity* opacity, xEnt*)
{
    opacity->delta_opacity = 0.0f;
    opacity->delta_opacity_2 = 0.0f;
}

void xEntOpacityUpdate(xEntOpacity* opacity, xEnt* ent, F32 dt)
{
    opacity->delta_opacity += opacity->delta_opacity_2 * dt;

    if (!ent->model) {
        return;
    }

    ent->model->Alpha += opacity->delta_opacity * dt;
    ent->model->Alpha = CLAMP(ent->model->Alpha, 0.0f, 1.0f);

    if (1.0f != ent->model->Alpha && 0.0f != ent->model->Alpha) {
        ent->model->Pipe.Flags |= 0x20000;
    } else {
        ent->model->Pipe.Flags &= ~0x20000;
    }
}

void xEntLoadCheckPoint()
{
}

void xEntStoreCheckPoint()
{
}

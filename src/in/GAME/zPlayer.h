#ifndef ZPLAYER_H
#define ZPLAYER_H

#include "zEnt.h"
#include "zInteraction.h"
#include "xShadowSimple.h"
#include "xEntDrive.h"
#include "xOneLinerManager.h"

struct jump;
struct zSlideCam;
struct xJSPHeader;
struct xClumpCollBSPTriangle;
struct xModelAssetParam;
struct mblur_data;
struct zTurret;
struct zCombatDamageInfo;
struct xSweptSphere;
struct SphereCollisionResults;

namespace zHud
{
    struct Incredimeter;
    struct BossMeter;
    struct FamilyMeter;
} // namespace zHud

#define ZPLAYER_CUTSCENEREADY 0x8000

struct zPlayer : zEnt
{
    bool Get_cutsceneReady() const;

    void Set_cutsceneReady()
    {
        zPlayerFlags |= ZPLAYER_CUTSCENEREADY;
    }

    void Clear_cutsceneReady();

    virtual void Init(xEntAsset* asset);
    virtual void SceneSetup();
    virtual void BoundUpdate(xVec3* pos) = 0;
    virtual bool AllowInteraction(zInteraction*);
    virtual void AttackHit(bool hitEnv);
    virtual void BeginUpdate(F32 dt);
    virtual void Update(F32 dt);
    virtual void EndUpdate(F32 dt);
    virtual void Move(xScene* s, F32 dt, xEntFrame* frame);
    virtual void Render();
    virtual bool WasRendered() const;
    virtual void HandleEvent(xBase* from, U32 thisEvent, const F32* toParam,
                             xBase* toParamWidget, U32 toParamWidgetID);
    virtual void Reset();
    virtual bool IsDead() const;
    virtual zTurret* GetTurret() const;
    virtual void Exit();
    virtual bool Damage(const zCombatDamageInfo& damageInfo);
    virtual xEnt* GetCarriedEntity();
    virtual S32* GetHeadBones();
    virtual void PostRenderCleanup();
    virtual void RenderCustomUI();
    virtual bool CanTakeDamage() const;
    virtual void LoadCheckPoint();
    virtual void StoreCheckPoint();

    virtual void SetCamera()
    {
    }

    virtual void RenderEffects();
    virtual void SetJump(jump* new_jump);
    virtual void GiveHealth(S32 hitPoints, bool from_pickup);
    virtual void ResetHealth();
    virtual bool NeedsHealth() const;
    virtual xVec3& GetFloorNormal() const;
    virtual xVec3* GetFloorPosition() const;
    virtual const char** GetBoneNames() const;
    virtual bool IsVisible() const;
    virtual void RenderTransparent(F32 alpha);
    virtual bool NeedsIncrediPower() const;
    virtual void GiveIncrediPower(S16 amount, bool from_pickup);
    virtual void TakeIncrediPower(F32 points);
    virtual bool IsSwimming() const;
    virtual void ParseIni();
    virtual void AddStates(xAnimTable* table);
    virtual void AddTransitions(xAnimTable* table);
    virtual bool FindShortestCollision(xSweptSphere& sws, U32& collisionFlags, S32 penby);
    virtual void FindStaticCollisions(const xSphere& sphere);
    virtual bool CollisionResponse(const xVec3& current_dir,
                                   const SphereCollisionResults& scene_collide,
                                   xVec3& response_dpos);
    virtual void RegisterCollision(const SphereCollisionResults& scene_collide);
    virtual bool StayOnGround();
    virtual F32 InitialDepenetrationVelocity();
    virtual void ReactiveAnimationCollided();
    virtual xEnt* get_floor_entity();
    virtual xVec3 get_target();
    virtual F32 GetScreenFadeTime();
    virtual F32 StartScreenFadeAt();
    virtual bool IsInteractioning();
    virtual void SetEnemyIsNear(bool isit);

    xVec3 trigLastFramePos;
    xOneLinerManager* pOneLinerSys;
    zHud::Incredimeter* pIncredimeter;
    zHud::BossMeter* pBossMeter;
    zHud::FamilyMeter* pFamilyMeter;
    xScene* sc;
    xLightKit* lightKit;
    xMat4x3 lastmat;
    S32 brain_id;
    xVec3 lastDeltaPos;
    F32 update_dt;
    F32 last_update_dt;
    xVec3 update_motion;
    xVec3 previous_position;
    jump* currentJump;
    xVec3 predictRotate;
    xVec3 predictTranslate;
    F32 predictAngV;
    xVec3 predictCurrDir;
    F32 predictCurrVel;
    zSlideCam* slideCam;
    xJSPHeader* floor_jsp;
    xClumpCollBSPTriangle* floor_bspTriangle;
    xVec3 floorNorm;
    S32 slope;
    F32 floor_collision_timer;
    F32 floor_distance;
    xEnt* floor_entity;
    U32 floor_oid;
    xSurface* floor_surface;
    xVec3 floor_pos;
    xVec3 floor_normal;
    xSphere extraSpheres[5];
    S32 numExtraSpheres;
    U32 collisionFlags;
    U8 currentIsNearLedge;
    U8 currentIsNearLedgeValid;
    F32 depenetration_velocity;
    F32 fallingTime;
    F32 idleAnimationTimer;
    xEntShadow entShadow_embedded;
    xShadowSimpleCache simpShadow_embedded;
    U32 loaded_assetid;
    xEntDrive drv;
    F32 floorDist[4];
    F32 floorTimer[4];
    xVec3 floorSupp[4];
    U16 parametersSize;
    xModelAssetParam* parameters;
    xEnt* reticleTarget;
    RpAtomic* reticleModel;
    F32 reticleRot;
    F32 reticleAlpha;
    xMat4x3 reticleMat;
    F32 deathTimer;
    U8 deathStartedAnim;
    F32 idleSoundTimer;
    U32 oldSlidePen;
    S32 zPlayerFlags;
    F32 collision_underneath_height;
    F32 collision_underneath_time;
    mblur_data* mblur;
    U32* hitSoundsMap;
    U32 lorezModelID;
    xModelInstance* lorezModel;
    xModelInstance* hirezModel;
};

#endif
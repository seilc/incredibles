#include "xCommon.h"
#include "zNPCSmoke.h"

#include "xCam.h"
#include "xGlobals.h"
#include "xParticleBatch.h"
#include "xRand.h"
#include "xSpacePartitionTree.h"
#include "xString.h"
#include "xstransvc.h"

static const xColor NPCSmokeColor = { 0xFF, 0xFF, 0xFF, 0x28 };

RwTexture* zNPCSmokeSystem::texture;

namespace zNPC
{
    void NPCSmokeBehavior::setup()
    {
        hasStarted = false;
        smokePoints = owner->get_combat()->currentHitPoints * 7 / 10;
        if (!owner->get_parameter("DamageSmokeBone", &bone, 0)) {
            bone = 0;
        }
        if (!owner->get_parameter("DamageSmokeScale", &scale, 0.0f)) {
            scale = 3.5f;
        }
        if (!owner->get_parameter("DamageSmokeParticleLifetime", &lifetime, 0.0f)) {
            lifetime = 5.0f;
        }
        if (!owner->get_parameter("DamageSmokeParticleGravity", &gravity, 0.0f)) {
            gravity = 0.4f;
        }
    }

    void NPCSmokeBehavior::reset()
    {
    }

    bool NPCSmokeBehavior::runnable(F32)
    {
        return owner->get_combat()->currentHitPoints <= smokePoints;
    }

    bool NPCSmokeBehavior::exclusive()
    {
        return false;
    }

    void NPCSmokeBehavior::enter_state(const behavior*)
    {
        if (!hasStarted) {
            NPCSmoke = new zNPCSmokeSystem(scale, lifetime, gravity);
            hasStarted = true;
        }
    }

    void NPCSmokeBehavior::exit()
    {
        delete NPCSmoke;
        hasStarted = false;
    }

    void NPCSmokeBehavior::update(F32 dt)
    {
        xVec3 dir = { 0.0f, 1.0f, 0.0f };
        xVec3 pos;
        xMat4x3Toworld(&pos, (xMat4x3*)owner->model->Mat, (xVec3*)&owner->model->Mat[bone + 1].pos);
        NPCSmoke->update(pos, dir, owner->get_combat()->currentHitPoints, smokePoints, dt);
    }
} // namespace zNPC

void* zNPCSmokeSystem::operator new(size_t size)
{
    return RwMalloc(size, rwMEMHINTDUR_NADURATION);
}

void zNPCSmokeSystem::operator delete(void* mem)
{
    RwFree(mem);
}

zNPCSmokeSystem::zNPCSmokeSystem(F32 _scale, F32 _lifetime, F32 _gravity)
{
    scale = _scale;
    lifetime = _lifetime;
    gravity = _gravity;

    texture = (RwTexture*)xSTFindAsset(xStrHash("ExplosionFlash_Smoke_tan_64"), NULL);
    if (!texture) {
        return;
    }

    Reset(pos, dir);

    xParticleBatchSystem system;
    system.context = this;
    system.update = Update;
    system.order_group = 'HOBO';
    system.order_index = 12;
    system.pool_type = PGT_POS_COLOR_SIZE_ROT;
    system.rs.src_blend = rwBLENDSRCALPHA;
    system.rs.dst_blend = rwBLENDINVSRCALPHA;
    system.rs.flags = 0;
    system.rs.texture = texture;
    system.stride = sizeof(NPCSmokeParticle);
    systemID = xPARTICLEBATCHADDSYSTEM(system);
}

void zNPCSmokeSystem::Reset(xVec3& pos, xVec3& dir)
{
    this->pos = pos;
    this->dir = dir;
    emitRemaining = 0.0f;
    time = 0.1f;
}

void zNPCSmokeSystem::update(xVec3& pos, xVec3& dir, F32 currentHitPoints, F32 smokePoints, F32 dt)
{
    if (!texture || smokePoints <= currentHitPoints) {
        return;
    }

    F32 smokeEmitRate = 10.0f + 90.0f * ((smokePoints - currentHitPoints) / smokePoints);
    F32 timePassed = (1.0f - emitRemaining) / smokeEmitRate;
    emitRemaining += smokeEmitRate * dt;
    S32 emit = emitRemaining;
    while (emit > 0) {
        NPCSmokeParticle* p;
        S32 count = xParticleBatchEmit(systemID, emit, (U8**)&p);
        if (count == 0) {
            emitRemaining = 0.0f;
            return;
        }

        NPCSmokeParticle* end = p + count;
        while (p != end) {
            if (p == end) {
                break;
            }

            p->color = NPCSmokeColor;
            p->size = scale * (0.5f * xurand());
            p->vel.x = 0.4f * (xurand() - 0.5f);
            p->vel.y = 0.4f * (xurand() - 0.5f);
            p->vel.z = 0.4f * (xurand() - 0.5f);
            p->vel += dir * 2.0f;
            p->vel.y = MAX(1.5f, p->vel.y);
            p->rot = PI * xurand();
            p->life = lifetime - timePassed;
            p->pos = pos;
            xVec3AddScaled(p->pos, p->pos, p->vel, timePassed);
            p->pos.y -= 0.5f * gravity * timePassed * timePassed;
            timePassed += 1.0f / smokeEmitRate;
            p++;
        }

        emitRemaining -= count;
        emit -= count;
    }
}

S32 zNPCSmokeSystem::Update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void* context)
{
    zNPCSmokeSystem* system = (zNPCSmokeSystem*)context;
    ptank_pool__pos_color_size_rot& pool = (ptank_pool__pos_color_size_rot&)pool_;
    NPCSmokeParticle* p = (NPCSmokeParticle*)mem;
    NPCSmokeParticle* end = p + count;

    while (p != end) {
        p->vel.y -= system->gravity * dt;
        p->pos += p->vel * dt;
        p->life -= dt;
        if (p->life < 0.0f) {
            end--;
            // BUG: Should be *p = *end
            *p = *(end - 1);
            continue;
        }

        RwSphere sphere;
        sphere.center = p->pos.m_RwV3d;
        sphere.radius = 1.0f;
        if (RwCameraFrustumTestSphere(xglobals->screen->icam, &sphere)) {
            pool.next();
            if (!pool.valid()) {
                break;
            }

            F32 fadeTime = (1.0f - 0.8f) * system->lifetime;
            if (p->life < fadeTime) {
                p->color.a = 40.0f * (p->life / fadeTime);
            } else {
                p->color.a = 40;
            }

            *pool.pos = p->pos;
            pool.SetRot(p->rot);
            *pool.color = p->color;
            pool.size->assign(p->size, p->size);
        }

        p++;
    }

    return end - (NPCSmokeParticle*)mem;
}

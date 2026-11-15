#include "xCommon.h"
#include "zTurretSmoke.h"

#include "xCam.h"
#include "xGlobals.h"
#include "xParticleBatch.h"
#include "xRand.h"
#include "xSpacePartitionTree.h"
#include "xString.h"
#include "xstransvc.h"

RwTexture* zTurretSmokeSystem::texture;

namespace
{
    S32 systemID;
    const xColor turretSmokeColor = { 0xFF, 0xFF, 0xFF, 0x28 };
} // namespace

void zTurretSmokeSystem::setup()
{
    texture = (RwTexture*)xSTFindAsset(xStrHash("ExplosionFlash_Smoke_tan_64"), NULL);
    if (!texture) {
        systemID = -1;
        return;
    }

    xParticleBatchSystem system;
    system.context = NULL;
    system.update = Update;
    system.order_group = 'HOBO';
    system.order_index = 11;
    system.pool_type = PGT_POS_COLOR_SIZE_ROT;
    system.rs.texture = texture;
    system.rs.src_blend = rwBLENDSRCALPHA;
    system.rs.dst_blend = rwBLENDINVSRCALPHA;
    system.rs.flags = 0;
    system.stride = sizeof(TurretSmokeParticle);
    systemID = xPARTICLEBATCHADDSYSTEM(system);
}

zTurretSmokeSystem::zTurretSmokeSystem(xVec3& pos, xVec3& dir) : z::lightweight<zTurretSmokeSystem, 10>(true)
{
    Reset(pos, dir);
}

zTurretSmokeSystem::zTurretSmokeSystem() : z::lightweight<zTurretSmokeSystem, 10>(false)
{
}

void zTurretSmokeSystem::Reset(xVec3& pos, xVec3& dir)
{
    this->pos = pos;
    this->dir = dir;
    emitAccum = 0.0f;
    time = 0.1f;
    emitRemaining = 15.0f;
}

void zTurretSmokeSystem::update(F32 dt)
{
    if (emitRemaining < 0.0f) {
        set_used(false);
        return;
    }

    emitRemaining -= dt;
    if (!texture) {
        return;
    }

    F32 timePassed = (1.0f - emitAccum) / 100.0f;
    emitAccum += 100.0f * dt;
    S32 emit = emitAccum;
    while (emit > 0) {
        TurretSmokeParticle* p;
        S32 count = xParticleBatchEmit(systemID, emit, (U8**)&p);
        if (count == 0) {
            emitAccum = 0.0f;
            return;
        }

        TurretSmokeParticle* end = p + count;
        while (p != end) {
            if (p == end) {
                break;
            }

            p->color = turretSmokeColor;
            p->size = 0.5f * xurand();
            p->vel.x = 0.2f * (xurand() - 0.5f);
            p->vel.y = 0.2f * (xurand() - 0.5f);
            p->vel.z = 0.2f * (xurand() - 0.5f);
            p->vel += dir * 2.0f;
            p->vel.y = MAX(1.5f, p->vel.y);
            p->rot = 2.0f * PI * xurand() - PI;
            p->life = 1.9f - timePassed;
            p->pos = pos;
            xVec3AddScaled(p->pos, p->pos, p->vel, timePassed);
            p->pos.y -= 0.5f * timePassed * timePassed;
            timePassed += 0.01f;
            p++;
        }

        emitAccum -= count;
        emit -= count;
    }
}

S32 zTurretSmokeSystem::Update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void* context)
{
    zTurretSmokeSystem* system = (zTurretSmokeSystem*)context;
    ptank_pool__pos_color_size_rot& pool = (ptank_pool__pos_color_size_rot&)pool_;
    TurretSmokeParticle* p = (TurretSmokeParticle*)mem;
    TurretSmokeParticle* end = p + count;

    while (p != end) {
        p->vel.y -= dt;
        p->pos += p->vel * dt;
        p->life -= dt;
        if (p->life < 0.0f) {
            end--;
            // BUG: Should be *p = *end
            *p = end[-1];
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

            if (p->life < 1.9f * 0.2f) {
                p->color.a = 40.0f * (p->life / (1.9f * 0.2f));
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

    return end - (TurretSmokeParticle*)mem;
}

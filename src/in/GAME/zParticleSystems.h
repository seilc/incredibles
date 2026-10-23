#ifndef ZPARTICLESYSTEMS_H
#define ZPARTICLESYSTEMS_H

#include "zParticleGenerator.h"

class zParticleSystem
{
public:
    struct relative_ordering
    {
        S32 other;
        U8 before_other;
    };

    zParticleSystem(S32 _type, S32 _need)
    {
        type = _type;
        need = _need;
    }

    S32 type;
    S32 need;
    zParticleGenerator** generators;
    S32 generators_size;
    S32 generators_active;

    // clang-format off
    virtual zParticleGenerator* create_generator(zParticleAsset& asset) = 0;
    virtual void destroy_generator(zParticleGenerator*) {}
    virtual void scene_enter() {}
    virtual void scene_exit() {}
    virtual void setup() {}
    virtual void reset() {}
    virtual void update(F32 dt) {}
    virtual void pre_render() {}
    virtual void render() {}

    virtual relative_ordering* get_ordering(S32& size, S32)
    {
        size = 0;
        return NULL;
    }

    virtual S32 get_asset_size() const = 0;
    // clang-format on
};

#endif
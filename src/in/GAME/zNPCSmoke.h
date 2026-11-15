#ifndef ZNPCSMOKE_H
#define ZNPCSMOKE_H

#include "zNPCCommon.h"
#include "xColor.h"
#include "xPtankPool.h"
#include "xVec3.h"

class zNPCSmokeSystem
{
public:
    struct NPCSmokeParticle
    {
        xVec3 pos;
        F32 size;
        xVec3 vel;
        F32 rot;
        xColor color;
        F32 life;
        F32 pad;
    };

    F32 time;

    virtual void update(xVec3& pos, xVec3& dir, F32 currentHitPoints, F32 smokePoints, F32 dt);

    xVec3 pos;
    xVec3 dir;
    S32 systemID;
    F32 emitRemaining;
    F32 scale;
    F32 lifetime;
    F32 gravity;

    static RwTexture* texture;

    void* operator new(size_t size);
    void operator delete(void* mem);
    zNPCSmokeSystem(F32 _scale, F32 _lifetime, F32 _gravity);
    void Reset(xVec3& pos, xVec3& dir);
    static S32 Update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void* context);
};

namespace zNPC
{
    class NPCSmokeBehavior : public behavior_implementation<common>
    {
    public:
        bool hasStarted;
        S32 smokePoints;
        zNPCSmokeSystem* NPCSmoke;
        S32 bone;
        F32 scale;
        F32 lifetime;
        F32 gravity;

        virtual const char* getName() { return "NPCSmokeBehavior"; }
        virtual void enter_state(const behavior*);
        virtual void update(F32 dt);
        virtual void setup();
        virtual void exit();
        virtual void reset();
        virtual bool exclusive();
        virtual bool runnable(F32);
    };
} // namespace zNPC

#endif

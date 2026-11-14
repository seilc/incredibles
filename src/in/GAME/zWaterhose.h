#ifndef ZWATERHOSE_H
#define ZWATERHOSE_H

#include "xMath3.h"
#include "xColor.h"
#include "xPad.h"
#include "xDebugTweak.h"
#include "xSnd.h"

class zWaterhose
{
public:
    struct Particle
    {
        union
        {
            struct
            {
                xVec3 pos;
                xVec3 vel;
                xColor col;
                F32 life;
                F32 lifeAtStart;
                S32 gen;
                F32 w;
                F32 h;
                U8 isActive;
            };
            U8 pad[64];
        };
    };

    struct Layer
    {
        xVec3 pos[20];
        xVec3 vel[20];
        U8 isActive;
        U8 isEnd;
        F32 life;
    };

    struct CrossCardMesh
    {
        xVec3 line[7][8];
        F32 segmentAngle[10];
        F32 segmentLength;
        F32 segmentLengthDelta;
        F32 targetSegmentLength;
    };

    xSndGroupHandle sndGroupHandle;
    iSndHandle sndHandle;
    zWaterhose* prev;
    zWaterhose* next;
    Particle* particles;
    U8 particleMem[32015];
    Layer layer[1];
    CrossCardMesh crossCardMesh;
    S32 nextLayer;
    S32 counter;
    F32 texScroll;
    S32 nextParticle;
    xPad* pad;
    U8 isFirstPass;
    F32 angleLast;
    xVec3 emitterPos;
    xVec3 emitterDir;
    xColor color;
    F32 pitch;
    F32 friction;
    F32 gravity;
    F32 emitTime;
    S32 enabled;
    U8 doRender;
    tweak_callback waterhoseStartCB;
    xMat4x3 emitterMat;
    xMat4x3 emitterMatB;
    F32 emitterAngle;
    U32 count;
    F32 cullFade;
    U8 isShutDown;
    U8 doMgrUpdate;
    U8 doMgrRender;
    U8 isCulled;

    void* operator new(size_t size);
    zWaterhose(bool _doMgrUpdate, bool _doMgrRender);
    void DoMgrUpdate(bool _doMgrUpdate);
    void DoMgrRender(bool _doMgrRender);
    void ShutDown();
    void Update(F32 dt, xMat4x3& mat);
    void Render();
};

#endif

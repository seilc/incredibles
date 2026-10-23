#ifndef XFFX_H
#define XFFX_H

#include "xEnt.h"

struct xFFX
{
    U32 flags;
    void(*doEffect)(xEnt*, xScene*, F32, void*);
    void* fdata;
    xFFX* next;
};

struct xFFXRotMatchState
{
    S32 lgrounded;
    xVec3 lfup;
    xVec3 lfat;
    xVec3 plfat;
    F32 tmr;
    F32 mrate;
    F32 tmatch;
    F32 rrate;
    F32 trelax;
    F32 max_decl;
    xFFXRotMatchState* next;
};

struct xFFXShakeState
{
    xVec3 disp;
    F32 dur;
    F32 freq;
    F32 tmr;
    F32 alpha;
    F32 lval;
    xFFXShakeState* next;
};

void xFFXPoolInit(U32 num_ffx);
xFFX* xFFXAlloc();
void xFFXFree(xFFX* f);
S16 xFFXAddEffect(xEnt* ent, xFFX* f);
U32 xFFXRemoveEffectByFData(xEnt* ent, void* fdata);
void xFFXApply(xEnt* ent, xScene* sc, F32 dt);
void xFFXShakeUpdateEnt(xEnt* ent, xScene* sc, F32 dt, void* fdata);
void xFFXShakePoolInit(U32 num);
xFFXShakeState* xFFXShakeAlloc();
void xFFXShakeFree(xFFXShakeState* s);
void xFFXRotMatchPoolInit(U32 num);

#endif

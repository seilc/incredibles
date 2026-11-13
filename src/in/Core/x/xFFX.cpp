#include "xCommon.h"
#include "xFFX.h"

static U32 psize;
static xFFX* pool;
static xFFX* alist;

void xFFXPoolInit(U32 num_ffx)
{
    psize = num_ffx;
    pool = (xFFX*)xMEMALLOC(sizeof(xFFX) * psize, 0);

    pool->next = NULL;
    for (U32 i = 1; i < psize; i++) {
        pool[i].next = &pool[i-1];
    }

    alist = pool + (psize-1);
}

xFFX* xFFXAlloc()
{
    if (!alist) {
        return NULL;
    }

    xFFX* f = alist;
    alist = alist->next;
    return f;
}

void xFFXFree(xFFX* f)
{
    f->next = alist;
    alist = f;
}

S16 xFFXAddEffect(xEnt* ent, xFFX* f)
{
    f->next = ent->ffx;
    f->flags |= 0x1;

    ent->ffx = f;
    
    return ent->num_ffx++;
}

U32 xFFXRemoveEffectByFData(xEnt* ent, void* fdata)
{
    xFFX** ffxh = &ent->ffx;
    while (*ffxh) {
        if (fdata == (*ffxh)->fdata) {
            xFFX* next = (*ffxh)->next;
            ent->num_ffx--;
            xFFXFree(*ffxh);
            *ffxh = next;
            return TRUE;
        }
        ffxh = &(*ffxh)->next;
    }

    return FALSE;
}

static void xFFXApplyOne(xFFX* ffx, xEnt* ent, xScene* sc, F32 dt)
{
    if (ffx->next) {
        xFFXApplyOne(ffx->next, ent, sc, dt);
    }
    if (ffx->flags & 0x1) {
        ffx->doEffect(ent, sc, dt, ffx->fdata);
    }
}

void xFFXApply(xEnt* ent, xScene* sc, F32 dt)
{
    if (ent->ffx) {
        xFFXApplyOne(ent->ffx, ent, sc, dt);
    }
}

void xFFXShakeUpdateEnt(xEnt* ent, xScene* sc, F32 dt, void* fdata)
{
    xFFXShakeState* ss = (xFFXShakeState*)fdata;
    F32 tnext = ss->tmr + dt;
    xVec3 dpos;
    F32 mag = isin(ss->freq * tnext) * xexp(ss->alpha * tnext);

    if (ss->tmr == 0.0f) {
        ss->lval = 0.0f;
    } else if (ss->tmr >= ss->dur && ss->lval * mag < 0.0f) {
        xFFXRemoveEffectByFData(ent, fdata);
        xFFXShakeFree(ss);
        return;
    }

    xVec3SMul(&dpos, &ss->disp, mag - ss->lval);
    xVec3AddTo(&ent->frame->mat.pos, &dpos);
    ss->lval = mag;
    ss->tmr = tnext;
}

static U32 shake_psize;
static xFFXShakeState* shake_pool;
static xFFXShakeState* shake_alist;

void xFFXShakePoolInit(U32 num)
{
    shake_psize = num;
    shake_pool = (xFFXShakeState*)xMEMALLOC(sizeof(xFFXShakeState) * shake_psize, 0);

    shake_pool[0].next = NULL;
    for (U32 i = 1; i < shake_psize; i++) {
        shake_pool[i].next = &shake_pool[i-1];
    }

    shake_alist = shake_pool + (shake_psize-1);
}

xFFXShakeState* xFFXShakeAlloc()
{
    if (!shake_alist) {
        return NULL;
    }

    xFFXShakeState* s = shake_alist;
    shake_alist = shake_alist->next;
    return s;
}

void xFFXShakeFree(xFFXShakeState* s)
{
    s->next = shake_alist;
    shake_alist = s;
}

static U32 rot_match_psize;
static xFFXRotMatchState* rot_match_pool;
static xFFXRotMatchState* rot_match_alist;

void xFFXRotMatchPoolInit(U32 num)
{
    rot_match_psize = num;
    rot_match_pool = (xFFXRotMatchState*)xMEMALLOC(sizeof(xFFXRotMatchState) * rot_match_psize, 0);

    rot_match_pool->next = NULL;
    for (U32 i = 1; i < rot_match_psize; i++) {
        rot_match_pool[i].next = &rot_match_pool[i-1];
    }

    rot_match_alist = rot_match_pool + (rot_match_psize-1);
}

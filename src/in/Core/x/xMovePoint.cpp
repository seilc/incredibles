#include "xMovePoint.h"

#include "xMemMgr.h"
#include "xScene.h"
#include "xSpline.h"
#include "xRand.h"

void xMovePointInit(xMovePoint* m, xMovePointAsset* asset)
{
    xASSERT_L(26, m);

    xBaseInit(m, asset);

    m->asset = asset;
    m->pos = &m->asset->pos;
    m->on = asset->on;
    m->delay = asset->delay;
    m->spl = NULL;

    if (asset->numPoints > 0) {
        m->nodes = (xMovePoint**)xMEMALLOC(sizeof(xMovePoint*) * asset->numPoints);
    } else {
        m->nodes = NULL;
    }
}

void xMovePointSave(xMovePoint* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT_L(64, ent);
    xASSERT_L(65, s);
}

void xMovePointLoad(xMovePoint* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT_L(83, ent);
    xASSERT_L(84, s);
}

void xMovePointReset(xMovePoint* m)
{
    xASSERT_L(89, m);
    xASSERT_L(90, m->asset);

    xBaseReset(m, m->asset);

    m->on = m->asset->on;
    m->delay = m->asset->delay;
}

void xMovePointSetup(xMovePoint* m, xScene* sc)
{
    U32* id;
    U16 idx;

    xASSERT_L(108, m);
    xASSERT_L(109, sc);
    xASSERT_L(110, m->asset);

    m->node_wt_sum = 0;

    id = (U32*)((U8*)m->asset + sizeof(xMovePointAsset));
    for (idx = 0; idx < m->asset->numPoints; idx++) {
        xASSERT_L(117, m->nodes);

        m->nodes[idx] = (xMovePoint*)xSceneResolvID(sc, id[idx]);
        xASSERT_FMT_L(121, m->nodes[idx], "Move point ID %08X is not found!", id[idx]);

        m->node_wt_sum += m->nodes[idx]->asset->wt;
        m->nodes[idx]->prev = m;
    }
}

void xMovePointSplineSetup(xMovePoint* m)
{
    xMovePoint* w0, *w1, *w2, *w3;
    xVec3 points[2];
    xVec3 p1, p2;

    xASSERT_L(133, m);
    xASSERT_L(134, m->asset);

    if (m->asset->bezIndex != 1) return;
    if (m->spl) return;

    xASSERT_L(138, m->prev);
    xASSERT_L(139, m->nodes);
    xASSERT_L(140, m->nodes[0]);

    w0 = m->prev;
    w1 = m;
    w2 = m->nodes[0];

    points[0] = *w0->pos;
    if (w2->asset->bezIndex > 0) {
        xASSERT_L(151, w2->nodes[0]);
        w3 = w2->nodes[0];
        p1 = *w1->pos;
        p2 = *w2->pos;
        points[1] = *w3->pos;
    } else {
        p1.x = (1/3.f) * w0->pos->x + (2/3.f) * w1->pos->x;
        p1.y = (1/3.f) * w0->pos->y + (2/3.f) * w1->pos->y;
        p1.z = (1/3.f) * w0->pos->z + (2/3.f) * w1->pos->z;
        p2.x = (2/3.f) * w1->pos->x + (1/3.f) * w2->pos->x;
        p2.y = (2/3.f) * w1->pos->y + (1/3.f) * w2->pos->y;
        p2.z = (2/3.f) * w1->pos->z + (1/3.f) * w2->pos->z;
        points[1] = *w2->pos;
    }

    m->spl = xSpline3_Bezier(points, NULL, 2, 0, &p1, &p2);
    xSpline3_ArcInit(m->spl, 20);
}

void xMovePointSplineDestroy(xMovePoint* m)
{
    if (m->spl) {
        m->spl = NULL;
    }
}

F32 xMovePointGetNext(const xMovePoint* m, const xMovePoint* prev, xMovePoint** next, xVec3* hdng)
{
    S32 rnd;
    U16 idx;
    xMovePoint* previousOption;

    xASSERT_L(218, m);

    if (m->asset->numPoints < 1) {
        *next = NULL;
        return 0.0f;
    }

    previousOption = NULL;
    rnd = xrand_RandomInt32() % m->node_wt_sum;

    for (idx = 0; idx < m->asset->numPoints; idx++) {
        *next = m->nodes[idx];
        rnd -= (*next)->asset->wt;
        if (!(*next)->on) {
            *next = NULL;
        } else {
            previousOption = *next;
            if ((*next) == prev) {
                *next = NULL;
            } else if (rnd < 0) {
                break;
            }
        }
    }

    if (!*next) {
        if (previousOption) {
            *next = previousOption;
        } else {
            return 0.0f;
        }
    }

    if (hdng) {
        return xVec3Hdng(hdng, m->pos, (*next)->pos);
    }

    return 0.0f;
}

xVec3* xMovePointGetPos(const xMovePoint* m)
{
    xASSERT_L(275, m);
    return m->pos;
}

S32 xMovePointGetAllLinked(xMovePoint** mps, S32 mps_max, xMovePoint* mp, S32 mps_used)
{
    if (mps_used >= mps_max) return mps_used;

    for (S32 i = 0; i < mps_used; i++) {
        if (mp == mps[i]) return mps_used;
    }

    mps[mps_used++] = mp;

    S32 size = mp->asset->numPoints;
    for (S32 i = 0; i < size; i++) {
        mps_used = xMovePointGetAllLinked(mps, mps_max, mp->nodes[i], mps_used);
    }

    return mps_used;
}

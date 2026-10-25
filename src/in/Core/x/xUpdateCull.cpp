#include "xUpdateCull.h"

#include "xEnt.h"
#include "xGlobals.h"
#include "xCam.h"

#include <string.h>

static void xUpdateCull_Swap(xUpdateCullMgr* m, U32 a, U32 b)
{
    xUpdateCullEnt* cullA = m->mgr[a];
    xUpdateCullEnt* cullB = m->mgr[b];

    xUpdateCullEnt* curr = cullA;
    while (true) {
        curr->index = b;
        curr = curr->nextInGroup;
        if (!curr) break;
        if (curr == cullA) break;
    }
    curr = cullB;
    while (true) {
        curr->index = a;
        curr = curr->nextInGroup;
        if (!curr) break;
        if (curr == cullB) break;
    }

    void* tmpent = m->ent[a];
    m->ent[a] = m->ent[b];
    m->ent[b] = tmpent;

    xUpdateCullEnt* tmpmgr = m->mgr[a];
    m->mgr[a] = m->mgr[b];
    m->mgr[b] = tmpmgr;
}

static void xUpdateCull_MakeActive(xUpdateCullMgr* m, xUpdateCullEnt* cent)
{
    if (cent->index < m->entActive) return;

    if (cent->index != m->entActive) {
        xUpdateCull_Swap(m, cent->index, m->entActive);
    }

    if (m->activateCB) {
        m->activateCB(m->ent[cent->index]);
    }

    m->entActive++;
}

static void xUpdateCull_MakeInactive(xUpdateCullMgr* m, xUpdateCullEnt* cent)
{
    if (cent->index >= m->entActive) return;

    if (cent->index != m->entActive - 1) {
        xUpdateCull_Swap(m, cent->index, m->entActive - 1);
    }

    if (m->deactivateCB) {
        m->deactivateCB(m->ent[cent->index]);
    }

    m->entActive--;
}

U32 xUpdateCull_AlwaysTrueCB(void*, void*)
{
    return 1;
}

U32 xUpdateCull_DistanceSquaredCB(void* ent, void* cbdata)
{
    if (((xEnt*)ent)->baseFlags & k_XBASE_UNK_0x80) {
        return 1;
    }

    FloatAndVoid fandv;
    fandv.v = cbdata;

    xVec3* campos = &xglobals->cam->mat.pos;

    if (!((xEnt*)ent)->model) {
        return (0.0f < fandv.f);
    }

    F32 dist2 = xsqr(((xEnt*)ent)->model->Mat->pos.x - campos->x) +
                xsqr(((xEnt*)ent)->model->Mat->pos.y - campos->y) +
                xsqr(((xEnt*)ent)->model->Mat->pos.z - campos->z);
    return (dist2 < fandv.f);
}

xUpdateCullMgr* xUpdateCull_Init(void** ent, U32 entCount, xGroup** group, U32 groupCount)
{
    void** tempEnt = NULL;
    U32 tempCount = 0;

    if (entCount > 0) {
        tempEnt = (void**)RwMallocL(sizeof(void*) * entCount, rwMEMHINTDUR_NADURATION, 124);

        for (U32 idx = 0; idx < entCount; idx++) {
            if (!(((xEnt*)ent[idx])->baseFlags & k_XBASE_UNK_0x80)) {
                tempEnt[tempCount++] = ent[idx];
            }
        }
    }
    ent = tempEnt;
    entCount = tempCount;
    
    U32 i, j, k, gcnt, entsInThisGroup, entsInGroups, nonEmptyGroups, x;

    entsInGroups = 0;
    nonEmptyGroups = 0;

    bool* inGroupArray = (bool*)RwMallocL(sizeof(bool) * entCount, rwMEMHINTDUR_NADURATION, 141);
    memset(inGroupArray, 0, sizeof(bool) * entCount);

    for (i = 0; i < groupCount; i++) {
        entsInThisGroup = 0;
        gcnt = xGroupGetCount(group[i]);
        for (j = 0; j < gcnt; j++) {
            xBase* base = xGroupGetItemPtr(group[i], j);
            for (k = 0; k < entCount; k++) {
                if (base == ent[k]) {
                    inGroupArray[k] = true;
                    entsInGroups++;
                    entsInThisGroup++;
                }
            }
        }
        if (entsInThisGroup > 0) {
            nonEmptyGroups++;
        }
    }

    U32 mgrCount = entsInGroups;
    for (i = 0; i < entCount; i++) {
        if (!inGroupArray[i]) {
            mgrCount++;
        }
    }

    U32 memsize = sizeof(xUpdateCullMgr) +
                  sizeof(void*) * entCount +
                  sizeof(xUpdateCullEnt*) * entCount +
                  sizeof(xUpdateCullEnt) * mgrCount +
                  sizeof(xUpdateCullGroup) * nonEmptyGroups;
    
    xUpdateCullMgr* m = (xUpdateCullMgr*)xMEMALLOC(memsize);

    m->ent = (void**)(m + 1);
    m->mgr = (xUpdateCullEnt**)(m->ent + entCount);
    m->mgrList = (xUpdateCullEnt*)(m->mgr + entCount);
    m->grpList = (nonEmptyGroups > 0) ? (xUpdateCullGroup*)(m->mgrList + mgrCount) : NULL;
    memset(m->mgr, 0, sizeof(xUpdateCullEnt*) * entCount);
    m->entCount = entCount;
    m->entActive = entCount;
    m->mgrCount = mgrCount;
    m->mgrCurr = 0;
    m->grpCount = nonEmptyGroups;
    m->activateCB = NULL;
    m->deactivateCB = NULL;
    memcpy(m->ent, ent, sizeof(void*) * entCount);
    
    U32 grpIndex = 0;
    U32 mgrIndex = 0;
    for (i = 0; i < groupCount; i++) {
        U32 startIndex = mgrIndex;
        entsInThisGroup = 0;
        gcnt = xGroupGetCount(group[i]);
        for (j = 0; j < gcnt; j++) {
            xBase* base = xGroupGetItemPtr(group[i], j);
            for (k = 0; k < entCount; k++) {
                if (base == ent[k]) {
                    m->mgrList[mgrIndex].index = k;
                    m->mgrList[mgrIndex].groupIndex = grpIndex;
                    m->mgrList[mgrIndex].update_cull_cb = xUpdateCull_AlwaysTrueCB;
                    m->mgrList[mgrIndex].cbdata = NULL;
                    m->mgrList[mgrIndex].nextInGroup = NULL;
                    m->mgr[k] = &m->mgrList[mgrIndex];

                    xUpdateCullEnt** prevPtr = NULL;
                    for (x = 0; x < mgrIndex; x++) {
                        if (k == m->mgrList[x].index) {
                            if (!m->mgrList[mgrIndex].nextInGroup) {
                                m->mgrList[mgrIndex].nextInGroup = &m->mgrList[x];
                            }
                            prevPtr = &m->mgrList[x].nextInGroup;
                        }
                    }
                    if (prevPtr) {
                        *prevPtr = &m->mgrList[mgrIndex];
                    }
                    mgrIndex++;
                    entsInThisGroup++;
                }
            }
        }
        if (entsInThisGroup) {
            m->grpList[grpIndex].active = 1;
            m->grpList[grpIndex].startIndex = startIndex;
            m->grpList[grpIndex].endIndex = mgrIndex - 1;
            m->grpList[grpIndex].groupObject = group[i];
            grpIndex++;
        }
    }

    for (i = 0; i < entCount; i++) {
        if (!inGroupArray[i]) {
            m->mgrList[mgrIndex].index = i;
            m->mgrList[mgrIndex].groupIndex = -1;
            m->mgrList[mgrIndex].update_cull_cb = xUpdateCull_AlwaysTrueCB;
            m->mgrList[mgrIndex].cbdata = NULL;
            m->mgrList[mgrIndex].nextInGroup = NULL;
            m->mgr[i] = &m->mgrList[mgrIndex];
            mgrIndex++;
        }
    }

    xASSERT_L(269, mgrIndex == (S32)m->mgrCount);
    xASSERT_L(270, grpIndex == (S32)m->grpCount);

    for (i = 0; i < entCount; i++) {
        xASSERT_L(272, m->mgr[i]);
    }

    RwFree(inGroupArray);
    if (tempEnt) {
        RwFree(tempEnt);
    }

    return m;
}

void xUpdateCull_Update(xUpdateCullMgr* m, U32 percent_update)
{
    if (m->mgrCount == 0) return;

    S32 numiters = m->mgrCount * percent_update / 100;
    if (numiters <= 0) {
        numiters = 1;
    }

    while (numiters > 0) {
        U32 i;
        xUpdateCullEnt* cent = &m->mgrList[m->mgrCurr];
        U32 result = cent->update_cull_cb(m->ent[cent->index], cent->cbdata);
        if (cent->groupIndex == -1) {
            if (result == 1) {
                xUpdateCull_MakeActive(m, cent);
            } else {
                xUpdateCull_MakeInactive(m, cent);
            }
            m->mgrCurr++;
            numiters--;
        } else {
            xUpdateCullGroup* grp = &m->grpList[cent->groupIndex];
            if (result == 1) {
                if (grp) {
                    if (!grp->active) {
                        grp->active = 1;
                        for (i = grp->startIndex; i <= grp->endIndex; i++) {
                            xUpdateCull_MakeActive(m, &m->mgrList[i]);
                        }
                    }
                    numiters -= grp->endIndex + 1 - m->mgrCurr;
                    m->mgrCurr = grp->endIndex + 1;
                } else {
                    m->mgrCurr++;
                    numiters--;
                }
            } else {
                if (grp && m->mgrCurr == grp->endIndex && grp->active) {
                    grp->active = 0;
                    for (i = grp->startIndex; i <= grp->endIndex; i++) {
                        cent = &m->mgrList[i];
                        for (xUpdateCullEnt* curr = cent->nextInGroup; curr && curr != cent; curr = curr->nextInGroup) {
                            if (m->grpList[curr->groupIndex].active) {
                                goto loc_80101E10;
                            }
                        }
                        xUpdateCull_MakeInactive(m, &m->mgrList[i]);
                        loc_80101E10:;
                    }
                }
                m->mgrCurr++;
                numiters--;
            }
        }
        if (m->mgrCurr >= m->mgrCount) {
            m->mgrCurr = 0;
        }
    }
}

void xUpdateCull_SetCB(xUpdateCullMgr* m, void* entity, xUpdateCullEntCallback cb, void* cbdata)
{
    xASSERT_L(404, m != NULL);
    xASSERT_L(405, entity != NULL);

    for (U32 i = 0; i < m->mgrCount; i++) {
        if (m->ent[m->mgrList[i].index] == entity) {
            m->mgrList[i].update_cull_cb = cb;
            m->mgrList[i].cbdata = cbdata;
        }
    }
}

void xUpdateCull_Reset(xUpdateCullMgr* m)
{
    for (U32 i = m->entActive; i < m->entCount; i++) {
        xUpdateCull_MakeActive(m, m->mgr[i]);
    }
}

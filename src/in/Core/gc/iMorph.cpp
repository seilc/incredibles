#include "xCommon.h"
#include "iMorph.h"

#include "xMemMgr.h"
#include "iModel.h"

#include <rpusrdat.h>
#include <string.h>

struct DirtyMorph
{
    U32 count;
    F32 scale;
    S16 weight[4];
    S16* v_array[4];
};

static RpGeometry* s_geom;
static RpMorphTarget* s_tgt;
static F32* s_alloc;
static F32* s_vTemp;
static F32* s_nTemp;
static S32 s_numV;

static void MorphCommon(RpAtomic* model, RwMatrix* mat, S16** v_array, S16* weight, U32 normals, F32 scale, S32 dorender)
{
    U32 i, a;
    S16 wa[4], *va[4];
    S32 wsum;
    RwV3d *vold = NULL, *nold = NULL;
    RwInt32 lockMode;
    void* temp_scratch = NULL;

    s_geom = RpAtomicGetGeometry(model);
    s_tgt = RpGeometryGetMorphTarget(s_geom, 0);
    s_numV = s_geom->numVertices;
    s_alloc = NULL;
    s_nTemp = NULL;
    normals = normals && (rwObjectGetFlags(s_geom) & rpGEOMETRYNORMALS);
    lockMode = rpGEOMETRYLOCKVERTICES | (normals ? rpGEOMETRYLOCKNORMALS : 0);
    vold = s_tgt->verts;
    if (normals) nold = s_tgt->normals;

    a = 0;
    wsum = 0;
    for (i = 0; i < 4; i++) {
        if (v_array[i] && weight[i]) {
            va[a] = v_array[i], wa[a] = weight[i], wsum += weight[i];
            a++;
        }
    }

    RpUserDataArray* usr = RpGeometryGetUserDataArray(s_geom, 0);
    if (usr) {
        DirtyMorph* dm = (DirtyMorph*)usr->data;

        s_vTemp = (F32*)(dm + 1);
        while ((U32)s_vTemp & 0xF)
            s_vTemp++;
        s_tgt->verts = (RwV3d*)s_vTemp;

        if (normals) {
            s_nTemp = s_vTemp + s_numV * 3;
            while ((U32)s_nTemp & 0xF)
                s_nTemp++;
            s_tgt->normals = (RwV3d*)s_nTemp;
        }

        if (dm->count == a && dm->weight[0] == wa[0] && dm->v_array[0] == va[0] && dm->scale == scale) {
            for (i = 1; i < a; i++) {
                if (dm->weight[i] != wa[i] || va[i] != dm->v_array[i])
                    break;
            }

            if (a == i) {
                if (dorender) {
                    iModelRender(model, mat);
                }
                s_tgt->verts = vold;
                if (nold) s_tgt->normals = nold;
                return;
            }
        }

        dm->count = a;
        dm->scale = scale;
        for (i = 0; i < a; i++) {
            dm->weight[i] = wa[i];
            dm->v_array[i] = v_array[i];
        }

        RpGeometryLock(s_geom, lockMode);
    }

    if (a == 3) {
        va[3] = va[2];
        wa[3] = 0;
    }

    if (!usr) {
        if (s_numV <= 0x4000 / sizeof(RwV3d)) {
            temp_scratch = xMemScratchPush(0x4000, 0);
        }

        if (temp_scratch) {
            s_vTemp = (F32*)temp_scratch;
        } else {
            s_vTemp = (F32*)xMEMPUSHTEMP(s_numV * 3 * sizeof(F32) + 16);
            s_alloc = s_vTemp;
            while ((U32)s_vTemp & 0xF) s_vTemp++;
        }

        if (normals && (rwObjectGetFlags(s_geom) & rpGEOMETRYNORMALS)) {
            if (temp_scratch) {
                s_nTemp = (F32*)((U8*)temp_scratch + 0x2000);
            } else {
                s_nTemp = (F32*)xMEMPUSHTEMP(s_numV * 3 * sizeof(F32) + 16);
                if (!s_alloc) s_alloc = s_nTemp;
                while ((U32)s_nTemp & 0xF) s_nTemp++;
            }
        }
    }

    if (a == 1) FastS16unpack(s_vTemp, va[0], s_numV * 3, scale * wsum);
    else if (a == 2) FastS16weight2(s_vTemp, va, wa, s_numV * 3, scale);
    else FastS16weight4(s_vTemp, va, wa, s_numV * 3, scale);

    if (s_nTemp) {
        scale = 1.0f / (16384.0f * wsum);
        for (i = 0; i < a; i++) va[i] += (s_numV * 3 + 7) & ~7;

        if (a == 1) FastS16unpack(s_nTemp, va[0], s_numV * 3, scale * wsum);
        else if (a == 2) FastS16weight2(s_nTemp, va, wa, s_numV * 3, scale);
        else FastS16weight4(s_nTemp, va, wa, s_numV * 3, scale);
    }

    if (usr) {
        RpGeometryUnlock(s_geom);
        if (dorender) {
            iModelRender(model, mat);
        }

        s_tgt->verts = vold;
        if (normals) s_tgt->normals = nold;
    } else {
        if (dorender) {
            RpGeometryLock(s_geom, lockMode);
            vold = s_tgt->verts;
            s_tgt->verts = (RwV3d*)s_vTemp;
            if (normals) {
                nold = s_tgt->normals;
                s_tgt->normals = (RwV3d*)s_nTemp;
            }
            RpGeometryUnlock(s_geom);

            iModelRender(model, mat);

            s_tgt->verts = vold;
            if (normals) s_tgt->normals = nold;
        }
    }

    if (temp_scratch)
        xMemScratchPop(temp_scratch);
}

void iMorphOptimize(RpAtomic* model, S32 normals)
{
    normals = (normals != 0);

    RpGeometry* geom = RpAtomicGetGeometry(model);
    if (RpGeometryGetUserDataArrayCount(geom) != 0) {
        return;
    }

    RwInt32 numV = RpGeometryGetNumVertices(geom);
    RwInt32 usridx = RpGeometryAddUserDataArray(geom, "MORPHSTATE", rpINTUSERDATA, (normals + 1) * numV * sizeof(RwV3d) + sizeof(DirtyMorph) + 24);
    RpUserDataArray* usr = RpGeometryGetUserDataArray(geom, usridx);
    memset(usr->data, 0, sizeof(DirtyMorph));
}

void iMorphRender(RpAtomic* model, RwMatrix* mat, S16** v_array, S16* weight, U32 normals, F32 scale)
{
    MorphCommon(model, mat, v_array, weight, normals, scale, TRUE);

    if (s_alloc) {
        xMEMPOPTEMP(s_alloc);
        s_alloc = NULL;
    }
}

void FastS16unpack(F32* dest, S16* v, S32 count, F32 scale)
{
    for (S32 i = 0; i < count; i++) {
        dest[i] = scale * v[i];
    }
}

void FastS16weight2(F32* dest, S16** v_array, S16* weight, S32 count, F32 scale)
{
    S32 i;
    S16 *v0 = v_array[0], *v1 = v_array[1];
    F32 w0 = scale * weight[0], w1 = scale * weight[1];

    for (i = 0; i < count; i++) {
        dest[i] = w0 * v0[i] + w1 * v1[i];
    }
}

void FastS16weight4(F32* dest, S16** v_array, S16* weight, S32 count, F32 scale)
{
    S32 i;
    S16 *v0 = v_array[0], *v1 = v_array[1], *v2 = v_array[2], *v3 = v_array[3];
    F32 w0 = scale * weight[0], w1 = scale * weight[1], w2 = scale * weight[2], w3 = scale * weight[3];

    for (i = 0; i < count; i++) {
        dest[i] = w0 * v0[i] + w1 * v1[i] + w2 * v2[i] + w3 * v3[i];
    }
}

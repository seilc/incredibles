#include "xCommon.h"
#include "xDecal.h"

#include "xBound.h"
#include "xCam.h"
#include "xClumpColl.h"
#include "xCollide.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xEnt.h"
#include "xEnv.h"
#include "xFXHighDynamicRange.h"
#include "xGlobals.h"
#include "xGrid.h"
#include "xIM.h"
#include "xJSP.h"
#include "xMath.h"
#include "xMath3.h"
#include "xMemMgr.h"
#include "xModel.h"
#include "xPTankPool.h"
#include "xQuickCull.h"
#include "xRand.h"
#include "xRenderBuffer.h"
#include "xRenderState.h"
#include "xShadow.h"
#include "xstransvc.h"
#include "xString.h"
#include "xutil.h"
#include "xVec3.h"
#include "zEntSimpleObj.h"
#include "zGlobals.h"
#include "zGrid.h"
#include "zPlatform.h"
#include "zMain.h"
#include "zPlayer.h"
#include "zRenderState.h"
#include "zScene.h"
#include "iCamera.h"
#include "iMath3.h"
#include "iModel.h"

#include <rpmatfx.h>
#include <rpskin.h>
#include <stdlib.h>
#include <string.h>


using namespace xDecal;

namespace
{
    namespace tweak
    {
        xTWEAKABLE bool skip_backface = true;
        xTWEAKABLE bool skip_facing_away = false;
        xTWEAKABLE F32 skip_facing_cos = 0.3f;
        xTWEAKABLE F32 zbias = 1.0f;

        namespace debug
        {
            xTWEAKABLE S32 filter_tri = -1;
            xTWEAKABLE bool invert_normals = false;
            xTWEAKABLE bool render_hit_tests = false;
            xTWEAKABLE bool render_volume = false;
            xTWEAKABLE bool render_normals = false;
            xTWEAKABLE bool render_tri_face = false;
            xTWEAKABLE bool render_tri_intersect = false;
            xTWEAKABLE bool render_tri_clipped = false;
            xTWEAKABLE bool render_tri_unclipped = false;

            namespace cast_test
            {
                xTWEAKABLE bool enable = false;
                xTWEAKABLE bool track_camera = true;
                xTWEAKABLE F32 size = 1.0f;
                xTWEAKABLE F32 dist_min = 1.0f;
                xTWEAKABLE F32 dist_max = 10.0f;
                xTWEAKABLE F32 sweep_arc = 0.5235988f;
                xTWEAKABLE F32 sweep_speed = 0.0f;
                xTWEAKABLE S32 blend_mode = 0;
                xTWEAKABLE U32 texture = 0xA588A3D;
            }

            namespace emit_test
            {
                xTWEAKABLE bool cycle = false;
                xTWEAKABLE F32 life = 1.0f;
                xTWEAKABLE F32 size = 1.0f;
                xTWEAKABLE F32 dist_min = 1.0f;
                xTWEAKABLE F32 dist_max = 10.0f;
                xTWEAKABLE S32 blend_mode = 0;
                xTWEAKABLE U32 texture = 0xA588A3D;
            }
        }
    }

    struct owner_group_type
    {
        U32 owner;
        S32 size;
        S32 max_size;
        struct decal_instance* head;
        struct decal_instance* tail;
    };

    enum decal_type
    {
        DECAL_TYPE_ATOMIC,
        DECAL_TYPE_IMMEDIATE,
        DECAL_TYPE_CARD
    };

#define MAX_IMMEDIATE_VERTS 16
#define MAX_IMMEDIATE_TRIS 12

    struct immediate_mode_data
    {
        xVec3 vert[MAX_IMMEDIATE_VERTS];
        xVec2 uv[MAX_IMMEDIATE_VERTS];
        U16 index[MAX_IMMEDIATE_TRIS * 3];
        S32 vert_size;
        S32 index_size;
        immediate_mode_data* next;
    };

    struct decal_instance
    {
        owner_group_type* owner_group;
        decal_instance* group_prev;
        decal_instance* group_next;
        emit_context ec;
        decal_type type;
        F32 age;
        F32 age_rate;
        xEnt* driver_ent;
        RpAtomic* driver_atomic;
        const xMat4x3* driver;
        RwTexture* texture;
        F32 time_start;
        F32 time_scale;
        animix ani;
        xColor color;
        xMat3x2 uvmat;
        union
        {
            U16 blend_mode[2];
            U32 blend_mode_id;
        };
        union
        {
            RpAtomic* atomic;
            immediate_mode_data* immediate;
            S32 card;
        } data;
    };

    typedef pool_list<owner_group_type> owner_group_list;

    decal_instance* decals;
    decal_instance** render_list;
    immediate_mode_data* immediate_buffer;
    immediate_mode_data* immediate_free;
    S32 decals_used;
    bool decals_sorted;
    S32 immediate_indices_used;
    S32 immediate_verts_used;
    owner_group_list owner_groups;
    SDRenderState old_zRenderState;
    const xMat4x3* debug_mat;

    void debug_transform(xVec3* ov, const xVec3* iv, S32 size)
    {
        RwV3dTransformPoints((RwV3d*)ov, (const RwV3d*)iv, size, (const RwMatrix*)debug_mat);
    }

    const S32 CLIP_XLOWER = 0x1;
    const S32 CLIP_XUPPER = 0x2;
    const S32 CLIP_YLOWER = 0x4;
    const S32 CLIP_YUPPER = 0x8;
    const S32 CLIP_ZLOWER = 0x10;
    const S32 CLIP_ZUPPER = 0x20;

    inline S32 get_outcode(const xVec3& v, const xVec3& lower, const xVec3& upper)
    {
        S32 oc = 0;
        if (v.x < lower.x) oc |= CLIP_XLOWER;
        else if (v.x > upper.x) oc |= CLIP_XUPPER;
        if (v.y < lower.y) oc |= CLIP_YLOWER;
        else if (v.y > upper.y) oc |= CLIP_YUPPER;
        if (v.z < lower.z) oc |= CLIP_ZLOWER;
        else if (v.z > upper.z) oc |= CLIP_ZUPPER;
        return oc;
    }

    inline void get_bary_coords(F32& b1, const xVec3& p, const xVec3& t0, const xVec3& t1)
    {
        F32 Ax = t1.x - t0.x, Ay = t1.y - t0.y, Az = t1.z - t0.z;
        F32 Bx = p.x - t0.x, By = p.y - t0.y, Bz = p.z - t0.z;
        F32 AA = Ax * Ax + Ay * Ay + Az * Az;
        F32 AB = Ax * Bx + Ay * By + Az * Bz;
        b1 = xabs(AA) < 0.000001f ? 0.0f : AB / AA;
    }

    inline void get_bary_coords(F32& b1, F32& b2, const xVec3& p, const xVec3& t0, const xVec3& t1, const xVec3& t2)
    {
        F32 Ax = t1.x - t0.x, Ay = t1.y - t0.y, Az = t1.z - t0.z;
        F32 Bx = t2.x - t0.x, By = t2.y - t0.y, Bz = t2.z - t0.z;
        F32 Cx = p.x - t0.x, Cy = p.y - t0.y, Cz = p.z - t0.z;
        F32 AA = Ax * Ax + Ay * Ay + Az * Az;
        F32 BB = Bx * Bx + By * By + Bz * Bz;
        F32 AB = Ax * Bx + Ay * By + Az * Bz;
        F32 AC = Ax * Cx + Ay * Cy + Az * Cz;
        F32 BC = Bx * Cx + By * Cy + Bz * Cz;
        F32 det = AA * BB - AB * AB;

        if (xabs(det) < 0.000001f) {
            if (AA > BB) {
                b1 = xabs(AA) < 0.000001f ? 0.0f : AC / AA;
                b2 = 0.0f;
            } else {
                b1 = 0.0f;
                b2 = xabs(BB) < 0.000001f ? 0.0f : BC / BB;
            }
        } else {
            F32 div = 1.0f / det;
            b1 = div * (AC * BB - AB * BC);
            b2 = div * (BC * AA - AB * AC);
        }
    }

    inline void compute_outcodes(U8* outcode, const xVec3* vert, S32 vert_size, const xBox& box)
    {
        const xVec3* end_vert = vert + vert_size - vert_size % 4;
        while (vert < end_vert) {
            *outcode = get_outcode(*vert, box.lower, box.upper);
            outcode++; vert++;
            *outcode = get_outcode(*vert, box.lower, box.upper);
            outcode++; vert++;
            *outcode = get_outcode(*vert, box.lower, box.upper);
            outcode++; vert++;
            *outcode = get_outcode(*vert, box.lower, box.upper);
            outcode++; vert++;
        }

        end_vert += vert_size % 4;
        while (vert < end_vert) {
            *outcode = get_outcode(*vert, box.lower, box.upper);
            outcode++; vert++;
        }
    }

    struct combo_vertex
    {
        U16 index[3];
        U16 count;
        F32 n;
        F32 m;
    };

    void transform_combo_vertices(xVec3* vert, const combo_vertex* cv, S32 vert_size, const xVec3* src_vert)
    {
        xVec3* end_vert = vert + vert_size;
        for (; vert != end_vert; ++vert, ++cv) {
            const xVec3& s0 = src_vert[cv->index[0]];
            if (cv->count == 1) {
                *vert = s0;
            }
            else if (cv->count == 2) {
                const xVec3& s1 = src_vert[cv->index[1]];
                F32 o = 1.0f - cv->n;
                vert->x = o * s0.x + cv->n * s1.x;
                vert->y = o * s0.y + cv->n * s1.y;
                vert->z = o * s0.z + cv->n * s1.z;
            } else {
                const xVec3& s1 = src_vert[cv->index[1]];
                const xVec3& s2 = src_vert[cv->index[2]];
                F32 o = 1.0f - cv->n - cv->m;
                vert->x = o * s0.x + cv->n * s1.x + cv->m * s2.x;
                vert->y = o * s0.y + cv->n * s1.y + cv->m * s2.y;
                vert->z = o * s0.z + cv->n * s1.z + cv->m * s2.z;
            }
        }
    }

    struct clip_decal_params
    {
        struct
        {
            const xVec3* vert;
            S32 vert_size;
            const U8* outcode;
            xBox box;
            xVec2 size;
            xVec2 uv[2];
        } in;
        struct
        {
            xVec3* vert;
            S32 vert_size;
            S32 max_vert_size;
            xVec2* uv;
        } out;
        xVec3* clip_buffer;
    };

    const S32 MAX_CLIPPED_VERTS = 9;

#define CLIP_POLY(flag, bound, a, b, c, op)                                                       \
    if (out_union & (flag)) {                                                                     \
        F32 _c = arg.in.box.bound.a;                                                              \
        const xVec3* s0 = &cv[cv_offset] + (vsize - 1);                                           \
        const xVec3* s1 = &cv[cv_offset];                                                         \
        const xVec3* ends1 = s1 + vsize;                                                          \
        cv_offset ^= MAX_CLIPPED_VERTS;                                                           \
        xVec3* d = &cv[cv_offset];                                                                \
        while (s1 != ends1) {                                                                     \
            if (s1->a op _c) {                                                                    \
                if (!(s0->a op _c)) {                                                             \
                    F32 s = (_c - s0->a) / (s1->a - s0->a);                                       \
                    d->a = _c;                                                                    \
                    d->b = s * (s1->b - s0->b) + s0->b;                                           \
                    d->c = s * (s1->c - s0->c) + s0->c;                                           \
                    d++;                                                                          \
                }                                                                                 \
                *d = *s1;                                                                         \
                d++;                                                                              \
            } else if (s0->a op _c) {                                                             \
                F32 s = (_c - s0->a) / (s1->a - s0->a);                                           \
                d->a = _c;                                                                        \
                d->b = s * (s1->b - s0->b) + s0->b;                                               \
                d->c = s * (s1->c - s0->c) + s0->c;                                               \
                d++;                                                                              \
            }                                                                                     \
            s0 = s1, ++s1;                                                                        \
        }                                                                                         \
        vsize = d - &cv[cv_offset];                                                               \
    }

    void clip_decal(clip_decal_params& arg)
    {
        const xVec3* iv = arg.in.vert;
        const xVec3* end_iv = iv + arg.in.vert_size;
        xVec3* ov = arg.out.vert;
        xVec3* end_ov = ov + arg.out.max_vert_size - 2;
        xVec2* ouv = arg.out.uv;
        xVec3* cv = arg.clip_buffer;
        const U8* outcode = arg.in.outcode;
        F32 su = arg.in.uv[0].x, sv = arg.in.uv[0].y;
        F32 du = (arg.in.uv[1].x - su) / arg.in.size.x;
        F32 dv = (arg.in.uv[1].y - sv) / arg.in.size.y;
        F32 ux = arg.in.box.upper.x, uy = arg.in.box.upper.y;

        while (iv != end_iv) {
            if (tweak::debug::render_normals) {
                xVec3 tv[3];
                debug_transform(tv, iv, 3);
                xVec3 l1, l2, norm, norm_loc;
                l1.Sub(tv[1], tv[0]);
                l2.Sub(tv[2], tv[1]);
                norm.cross(l1, l2);
                norm.up_normalize();
                norm_loc.Average(tv[0], tv[1], tv[2]);
                xDrawSetColor(g_WHITE);
                xDrawArrow(&norm_loc, &norm, false, 1.0f);
            }

            S32 out_union = outcode[0];
            out_union |= outcode[1];
            out_union |= outcode[2];
            outcode += 3;

            if (out_union == 0) {
                *ov = *iv;
                ouv->x = du * (ux - ov->x) + su;
                ouv->y = dv * (uy - ov->y) + sv;
                iv++; ov++; ouv++;
                *ov = *iv;
                ouv->x = du * (ux - ov->x) + su;
                ouv->y = dv * (uy - ov->y) + sv;
                iv++; ov++; ouv++;
                *ov = *iv;
                ouv->x = du * (ux - ov->x) + su;
                ouv->y = dv * (uy - ov->y) + sv;
                iv++; ov++; ouv++;
            } else {
                S32 cv_offset = 0;
                S32 vsize = 3;

                cv[0] = *iv; iv++;
                cv[1] = *iv; iv++;
                cv[2] = *iv; iv++;

                CLIP_POLY(CLIP_XLOWER, lower, x, y, z, >);
                CLIP_POLY(CLIP_YLOWER, lower, y, x, z, >);
                CLIP_POLY(CLIP_ZLOWER, lower, z, x, y, >);
                CLIP_POLY(CLIP_XUPPER, upper, x, y, z, <);
                CLIP_POLY(CLIP_YUPPER, upper, y, x, z, <);
                CLIP_POLY(CLIP_ZUPPER, upper, z, x, y, <);

                const xVec3* poly_vert = &cv[cv_offset];
                xVec2* poly_uv = (xVec2*)&cv[cv_offset ^ MAX_CLIPPED_VERTS];
                S32 new_verts = (vsize - 2) * 3;

                if (ov + new_verts + 3 > end_ov) {
                    break;
                }

                const xVec3* pv = poly_vert;
                xVec2* puv = poly_uv, *end_puv = puv + vsize;
                for (; puv != end_puv; ++pv, ++puv) {
                    puv->x = du * (ux - pv->x) + su;
                    puv->y = dv * (uy - pv->y) + sv;
                }

                const xVec3* tri_vert1 = poly_vert + 1;
                const xVec3* tri_vert2 = poly_vert + 2;
                const xVec3* end_tri_vert2 = poly_vert + vsize;
                const xVec2* tri_uv1 = poly_uv + 1, *tri_uv2 = poly_uv + 2;
                while (tri_vert2 < end_tri_vert2) {
                    *ov = *poly_vert;
                    *ouv = *poly_uv;
                    ov++; ouv++;
                    *ov = *tri_vert1;
                    *ouv = *tri_uv1;
                    ov++; ouv++;
                    *ov = *tri_vert2;
                    *ouv = *tri_uv2;
                    ov++; ouv++;
                    tri_vert1++; tri_vert2++;
                    tri_uv1++; tri_uv2++;
                }
            }

            if (ov >= end_ov) {
                break;
            }
        }

        arg.out.vert_size = ov - arg.out.vert;
    }

    struct clip_vertex : xVec3
    {
        U8 prev;
        U8 next;
        U8 clipped;
        U8 inner;
    };

    enum { MAX_EMIT_VERTS = 5120 };
    const U32 SCRATCH_SIZE = 0x4000;

#define MAX_ENV_VERTS 1200

#define CLIP_ADD_EXTRA_TRIANGLES 100
#define CLIP_ADD_EXTRA_VERTICES 100

    struct clip_decal_indexed_params
    {
        struct
        {
            const RpTriangle* tri;
            S32 tri_size;
            const xVec3* vert;
            S32 vert_size;
            const U8* outcode;
            xBox box;
            xVec2 size;
            xVec2 uv[2];
        } in;
        struct
        {
            RpTriangle* tri;
            S32 tri_size;
            S32 max_tri_size;
            combo_vertex* vert;
            S32 vert_size;
            S32 max_vert_size;
            xVec2* uv;
        } out;
        struct
        {
            clip_vertex* clip_buffer;
            U16* vert_usage_buffer;
        } buffer;
    };

#define CLIP_POLY_INDEXED(flag, bound, a, b, c, op)                                               \
    if (out_union & (flag)) {                                                                     \
        F32 _c = arg.in.box.bound.a;                                                              \
        const clip_vertex* s0 = &cv[cv_offset] + (vsize - 1);                                     \
        const clip_vertex* s1 = &cv[cv_offset];                                                   \
        const clip_vertex* ends1 = s1 + vsize;                                                    \
        cv_offset ^= MAX_CLIPPED_VERTS;                                                           \
        clip_vertex* d = &cv[cv_offset];                                                          \
        while (s1 != ends1) {                                                                     \
            if (s1->a op _c) {                                                                    \
                if (!(s0->a op _c)) {                                                             \
                    F32 s = (_c - s0->a) / (s1->a - s0->a);                                       \
                    d->a = _c;                                                                    \
                    d->b = s * (s1->b - s0->b) + s0->b;                                           \
                    d->c = s * (s1->c - s0->c) + s0->c;                                           \
                    d->prev = s1->prev;                                                           \
                    d->next = s0->next;                                                           \
                    d->clipped = 1;                                                               \
                    S32 wrap = (d->prev - d->next + 4) % 3;                                       \
                    d->inner = wrap | s0->inner | s1->inner;                                      \
                    d++;                                                                          \
                }                                                                                 \
                *d = *s1;                                                                         \
                d++;                                                                              \
            } else if (s0->a op _c) {                                                             \
                F32 s = (_c - s0->a) / (s1->a - s0->a);                                           \
                d->a = _c;                                                                        \
                d->b = s * (s1->b - s0->b) + s0->b;                                               \
                d->c = s * (s1->c - s0->c) + s0->c;                                               \
                d->prev = s1->prev;                                                               \
                d->next = s0->next;                                                               \
                d->clipped = 1;                                                                   \
                S32 wrap = (d->prev - d->next + 4) % 3;                                           \
                d->inner = wrap | s0->inner | s1->inner;                                          \
                d++;                                                                              \
            }                                                                                     \
            s0 = s1, ++s1;                                                                        \
        }                                                                                         \
        vsize = d - &cv[cv_offset];                                                               \
    }

    void clip_decal_indexed(clip_decal_indexed_params& arg)
    {
        const xVec3* iv = arg.in.vert;
        const RpTriangle* it = arg.in.tri, *end_it = it + arg.in.tri_size;
        RpTriangle* ot = arg.out.tri, *end_ot = ot + arg.out.max_tri_size - 7;
        combo_vertex* ov = arg.out.vert, *end_ov = ov + arg.out.max_vert_size - 8;
        xVec2* ouv = arg.out.uv;
        clip_vertex* cv = arg.buffer.clip_buffer;
        U16* used = arg.buffer.vert_usage_buffer;

        memset(used, 0xff, arg.in.vert_size * sizeof(U16));

        F32 su = arg.in.uv[0].x, sv = arg.in.uv[0].y;
        F32 du = 0.0f;
        if (arg.in.size.x != 0.0f) {
            du = (arg.in.uv[1].x - su) / arg.in.size.x;
        }
        F32 dv = 0.0f;
        if (arg.in.size.y != 0.0f) {
            dv = (arg.in.uv[1].y - sv) / arg.in.size.y;
        }
        F32 ux = arg.in.box.upper.x, uy = arg.in.box.upper.y;

        for (; it != end_it; it++) {
            S32 vi0 = it->vertIndex[0], vi1 = it->vertIndex[1], vi2 = it->vertIndex[2];
            S32 out0 = arg.in.outcode[vi0], out1 = arg.in.outcode[vi1], out2 = arg.in.outcode[vi2];

            S32 out_intersect = out0 & out1 & out2;
            if (out_intersect != 0) {
                continue;
            }

            F32 c1 = (iv[vi1].x - iv[vi0].x) * (iv[vi2].y - iv[vi1].y);
            F32 c2 = (iv[vi1].y - iv[vi0].y) * (iv[vi2].x - iv[vi1].x);
            if (c1 >= c2) {
                continue;
            }

            S32 out_union = out0 | out1 | out2;
            if (out_union == 0) {
                for (S32 i = 0; i < 3; i++) {
                    S32 vi = it->vertIndex[i];
                    U16& stored = used[vi];

                    if (stored != 0xffff)
                        ot->vertIndex[i] = stored;
                    else {
                        ov->index[0] = vi;
                        ov->count = 1;
                        stored = ot->vertIndex[i] = ov - arg.out.vert;
                        ov++;
                        const xVec3& vxy = iv[vi];
                        ouv->x = du * (ux - vxy.x) + su;
                        ouv->y = dv * (uy - vxy.y) + sv;
                        ouv++;
                    }
                }

                ot->matIndex = 0;
                ot++;

                if (ot >= end_ot || ov >= end_ov) {
                    break;
                }
            } else {
                S32 cv_offset = 0;
                S32 vsize = 3;

                (xVec3&)cv[0] = iv[vi0];
                cv[0].prev = 2;
                cv[0].next = 1;
                cv[0].clipped = cv[0].inner = 0;
                (xVec3&)cv[1] = iv[vi1];
                cv[1].prev = 0;
                cv[1].next = 2;
                cv[1].clipped = cv[1].inner = 0;
                (xVec3&)cv[2] = iv[vi2];
                cv[2].prev = 1;
                cv[2].next = 0;
                cv[2].clipped = cv[2].inner = 0;

                CLIP_POLY_INDEXED(CLIP_XLOWER, lower, x, y, z, >);
                CLIP_POLY_INDEXED(CLIP_YLOWER, lower, y, x, z, >);
                CLIP_POLY_INDEXED(CLIP_ZLOWER, lower, z, x, y, >);
                CLIP_POLY_INDEXED(CLIP_XUPPER, upper, x, y, z, <);
                CLIP_POLY_INDEXED(CLIP_YUPPER, upper, y, x, z, <);
                CLIP_POLY_INDEXED(CLIP_ZUPPER, upper, z, x, y, <);

                const clip_vertex* rcv = &cv[cv_offset], *end_rcv = rcv + vsize;
                S32* poly_vert_index = (S32*)&cv[cv_offset ^ MAX_CLIPPED_VERTS];
                S32* pvi = poly_vert_index;
                for (; rcv != end_rcv; ++rcv, ++pvi) {
                    if (!rcv->clipped) {
                        S32 vi = it->vertIndex[3 - (rcv->prev + rcv->next)];
                        U16& stored = used[vi];

                        if (stored != 0xffff) {
                            *pvi = stored;
                            continue;
                        }

                        ov->index[0] = vi;
                        ov->count = 1;
                        stored = ov - arg.out.vert;
                    }
                    else if (!rcv->inner) {
                        ov->index[0] = it->vertIndex[rcv->prev];
                        ov->index[1] = it->vertIndex[rcv->next];
                        ov->count = 2;
                        get_bary_coords(ov->n, *rcv, iv[ov->index[0]], iv[ov->index[1]]);
                    } else {
                        ov->index[0] = vi0;
                        ov->index[1] = vi1;
                        ov->index[2] = vi2;
                        ov->count = 3;
                        get_bary_coords(ov->n, ov->m, *rcv, iv[vi0], iv[vi1], iv[vi2]);
                    }

                    *pvi = ov - arg.out.vert;
                    ov++;
                    ouv->x = du * (ux - rcv->x) + su;
                    ouv->y = dv * (uy - rcv->y) + sv;
                    ouv++;
                }

                U16 fan_center = poly_vert_index[0];
                pvi = poly_vert_index + 1;
                S32* end_pvi = poly_vert_index + vsize - 1;
                for (; pvi < end_pvi; pvi++) {
                    ot->vertIndex[0] = fan_center;
                    ot->vertIndex[1] = pvi[0];
                    ot->vertIndex[2] = pvi[1];
                    ot->matIndex = 0;
                    ot++;
                }
            }
        }

        arg.out.tri_size = ot - arg.out.tri;
        arg.out.vert_size = ov - arg.out.vert;
    }

    void destroy_atomic(RpAtomic* atomic)
    {
        RwFrame* frame = RpAtomicGetFrame(atomic);
        RpAtomicSetFrame(atomic, NULL);
        if (frame) {
            RwFrameDestroy(frame);
        }

        RpGeometry* geom = RpAtomicGetGeometry(atomic);
        RpSkin* skin = RpSkinGeometryGetSkin(geom);
        if (geom->repEntry) {
            RwResourcesFreeResEntry(geom->repEntry);
        }

        if (skin) {
            RpSkinGeometrySetSkin(geom, NULL);
            RpSkinDestroy(skin);
        }

        RpAtomicDestroy(atomic);
    }

    inline void copy_uvmat(RwMatrix& omat, const xMat3x2& imat)
    {
        omat.right.x = imat.right.x;
        omat.right.y = imat.right.y;
        omat.up.x = imat.up.x;
        omat.up.y = imat.up.y;
        omat.pos.x = imat.pos.x;
        omat.pos.y = imat.pos.y;
    }

    RpAtomic* create_decal_atomic(const decal_instance& decal, S32 vert_size, S32 tri_size)
    {
        U32 geom_flags = rpGEOMETRYTEXTURED | rpGEOMETRYPOSITIONS;
        if (decal.ec.flags & emit_context::FLAG_LIT)
            geom_flags |= rpGEOMETRYNORMALS | rpGEOMETRYLIGHT;
        else
            geom_flags |= rpGEOMETRYPRELIT;

        if (decal.ec.flags & emit_context::FLAG_ANIM_COLOR) {
            geom_flags |= rpGEOMETRYMODULATEMATERIALCOLOR;
        }

        RpMaterial* material = RpMaterialCreate();
        RpGeometry* geom = RpGeometryCreate(vert_size, tri_size, geom_flags);
        RpAtomic* atomic = RpAtomicCreate();
        RwFrame* frame = RwFrameCreate();
        if (!material || !geom || !atomic || !frame) {

            if (material) {
                RpMaterialDestroy(material);
            }

            if (geom) {
                RpGeometryDestroy(geom);
            }

            if (atomic) {
                RpAtomicDestroy(atomic);
            }

            if (frame) {
                RwFrameDestroy(frame);
            }

            return NULL;
        }

        RpMaterialSetTexture(material, decal.texture);
        RpAtomicSetFrame(atomic, frame);
        RpAtomicSetGeometry(atomic, geom, 0);

        if (decal.ec.flags & emit_context::FLAG_ANIM_COLOR) {
            RpMaterialSetColor(material, &decal.color.rgba);
        }

        RpTriangle* tri = RpGeometryGetTriangles(geom);
        RpTriangle* end_tri = tri + tri_size;
        while (tri != end_tri) {
            RpGeometryTriangleSetMaterial(geom, tri, material);
            tri++;
        }

        if (!(decal.ec.flags & emit_context::FLAG_LIT)) {
            if (decal.ec.flags & emit_context::FLAG_ANIM_COLOR) {
                memset(geom->preLitLum, 0xFF, vert_size * sizeof(RwRGBA));
            } else {
                xColor* color = (xColor*)geom->preLitLum;
                xColor* end_color = color + vert_size;
                while (color != end_color) {
                    *color = decal.color;
                    color++;
                }
            }
        }

        frame->ltm = (RwMatrix&)g_I3;
        RwMatrixUpdate(&frame->ltm);

        if (decal.ec.flags & emit_context::FLAG_ANIM_UV) {
            RpMatFXAtomicEnableEffects(atomic);
            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTUVTRANSFORM);
            RwMatrix uvmat;
            copy_uvmat(uvmat, decal.uvmat);
            RpMatFXMaterialSetUVTransformMatrices(material, &uvmat, NULL);
        }

        RpGeometryDestroy(geom);
        RpMaterialDestroy(material);
        return atomic;
    }

    inline void transform_points(xVec3* out, const xVec3* in, S32 size, const xMat4x3& mat)
    {
        RwV3dTransformPoints((RwV3d*)out, (const RwV3d*)in, size, (const RwMatrix*)&mat);
    }

    inline void transform_decal_atomic(RpAtomic* atomic, const combo_vertex* vert, const xVec3* src_vert)
    {
        RpGeometry* geom = RpAtomicGetGeometry(atomic);
        xVec3* overt = (xVec3*)RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geom, 0));
        S32 vert_size = RpGeometryGetNumVertices(geom);
        transform_combo_vertices(overt, vert, vert_size, src_vert);
    }

    inline void transform_decal_atomic(RpAtomic* atomic, const combo_vertex* vert, const xVec3* src_vert, const xMat4x3& mat)
    {
        RpGeometry* geom = RpAtomicGetGeometry(atomic);
        xVec3* overt = (xVec3*)RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geom, 0));
        S32 vert_size = RpGeometryGetNumVertices(geom);
        transform_combo_vertices(overt, vert, vert_size, src_vert);
        transform_points(overt, overt, vert_size, mat);
    }

    inline bool transform_decal_atomic_skinned(RpAtomic* atomic, const combo_vertex* cv, RpAtomic* iatomic, F32* weight_buffer)
    {
        RpGeometry* geom = RpAtomicGetGeometry(atomic);
        RpMorphTarget* mt = RpGeometryGetMorphTarget(geom, 0);
        xVec3* vert = (xVec3*)RpMorphTargetGetVertices(mt);
        S32 vert_size = RpGeometryGetNumVertices(geom);

        RwMatrixWeights* weights = (RwMatrixWeights*)xMEMPUSHTEMP(vert_size * sizeof(RwMatrixWeights));
        U32* indices = (U32*)xMEMPUSHTEMP(vert_size * sizeof(U32));
        if (!weights || !indices) {
            if (weights) xMEMPOPTEMP(weights);
            return false;
        }

        memset(weights, 0, vert_size * sizeof(RwMatrixWeights));
        memset(indices, 0, vert_size * sizeof(U32));

        RpGeometry* igeom = RpAtomicGetGeometry(iatomic);
        RpMorphTarget* imt = RpGeometryGetMorphTarget(igeom, 0);
        const xVec3* ivert = (const xVec3*)RpMorphTargetGetVertices(imt);
        RpSkin* iskin = RpSkinGeometryGetSkin(igeom);
        const RwMatrixWeights* iweights = RpSkinGetVertexBoneWeights(iskin);
        const U32* iindices = RpSkinGetVertexBoneIndices(iskin);
        U32 bone_size = RpSkinGetNumBones(iskin);

        memset(weight_buffer, 0, bone_size * sizeof(F32));

        xVec3* end_vert = vert + vert_size;
        RwMatrixWeights* bone_weight = weights;
        U32* bone_index = indices;
        for (; vert != end_vert; ++vert, ++cv, ++bone_weight, ++bone_index) {
            S32 i0 = cv->index[0];
            if (cv->count == 1) {
                *vert = ivert[i0];
                *bone_weight = iweights[i0];
                *bone_index = iindices[i0];
            }
            else if (cv->count == 2) {
                S32 i1 = cv->index[1];
                F32 n = cv->n, o = 1.0f - n;
                const xVec3& s0 = ivert[i0];
                const xVec3& s1 = ivert[i1];
                vert->x = o * s0.x + n * s1.x;
                vert->y = o * s0.y + n * s1.y;
                vert->z = o * s0.z + n * s1.z;

                U32 bi0 = iindices[i0];
                const RwMatrixWeights& wt0 = iweights[i0];
                weight_buffer[bi0 & 0xff] = wt0.w0 * o;
                weight_buffer[(bi0 >> 8) & 0xff] = wt0.w1 * o;
                weight_buffer[(bi0 >> 16) & 0xff] = wt0.w2 * o;
                weight_buffer[(bi0 >> 24) & 0xff] = wt0.w3 * o;

                U32 bi1 = iindices[i1];
                const RwMatrixWeights& wt1 = iweights[i1];
                weight_buffer[bi1 & 0xff] += wt1.w0 * n;
                weight_buffer[(bi1 >> 8) & 0xff] += wt1.w1 * n;
                weight_buffer[(bi1 >> 16) & 0xff] += wt1.w2 * n;
                weight_buffer[(bi1 >> 24) & 0xff] += wt1.w3 * n;

                for (S32 i = 0; i < 4; i++) {
                    F32 highest = 0.0f;
                    S32 highest_index = -1;
                    for (S32 j = 0; j < 4; j++) {
                        if (highest < weight_buffer[(bi0 >> (j * 8)) & 0xff]) {
                            highest = weight_buffer[(bi0 >> (j * 8)) & 0xff];
                            highest_index = (bi0 >> (j * 8)) & 0xff;
                        }
                        if (highest < weight_buffer[(bi1 >> (j * 8)) & 0xff]) {
                            highest = weight_buffer[(bi1 >> (j * 8)) & 0xff];
                            highest_index = (bi1 >> (j * 8)) & 0xff;
                        }
                    }

                    if (highest_index == -1) {
                        break;
                    }

                    weight_buffer[highest_index] = 0.0f;
                    *bone_index |= highest_index << (i * 8);
                    ((F32*)bone_weight)[i] = highest;
                }

                weight_buffer[bi0 & 0xff] = 0.0f;
                weight_buffer[(bi0 >> 8) & 0xff] = 0.0f;
                weight_buffer[(bi0 >> 16) & 0xff] = 0.0f;
                weight_buffer[(bi0 >> 24) & 0xff] = 0.0f;
                weight_buffer[bi1 & 0xff] = 0.0f;
                weight_buffer[(bi1 >> 8) & 0xff] = 0.0f;
                weight_buffer[(bi1 >> 16) & 0xff] = 0.0f;
                weight_buffer[(bi1 >> 24) & 0xff] = 0.0f;
            } else {
                S32 i1 = cv->index[1], i2 = cv->index[2];
                F32 m = cv->m, n = cv->n, o = 1.0f - n - m;
                const xVec3& s0 = ivert[i0];
                const xVec3& s1 = ivert[i1];
                const xVec3& s2 = ivert[i2];
                vert->x = o * s0.x + n * s1.x + m * s2.x;
                vert->y = o * s0.y + n * s1.y + m * s2.y;
                vert->z = o * s0.z + n * s1.z + m * s2.z;

                U32 bi0 = iindices[i0];
                const RwMatrixWeights& wt0 = iweights[i0];
                weight_buffer[bi0 & 0xff] = wt0.w0 * o;
                weight_buffer[(bi0 >> 8) & 0xff] = wt0.w1 * o;
                weight_buffer[(bi0 >> 16) & 0xff] = wt0.w2 * o;
                weight_buffer[(bi0 >> 24) & 0xff] = wt0.w3 * o;

                U32 bi1 = iindices[i1];
                const RwMatrixWeights& wt1 = iweights[i1];
                weight_buffer[bi1 & 0xff] += wt1.w0 * n;
                weight_buffer[(bi1 >> 8) & 0xff] += wt1.w1 * n;
                weight_buffer[(bi1 >> 16) & 0xff] += wt1.w2 * n;
                weight_buffer[(bi1 >> 24) & 0xff] += wt1.w3 * n;

                U32 bi2 = iindices[i2];
                const RwMatrixWeights& wt2 = iweights[i2];
                weight_buffer[bi2 & 0xff] += wt2.w0 * m;
                weight_buffer[(bi2 >> 8) & 0xff] += wt2.w1 * m;
                weight_buffer[(bi2 >> 16) & 0xff] += wt2.w2 * m;
                weight_buffer[(bi2 >> 24) & 0xff] += wt2.w3 * m;

                for (S32 i = 0; i < 4; i++) {
                    F32 highest = 0.0f;
                    S32 highest_index = -1;
                    for (S32 j = 0; j < 4; j++) {
                        if (highest < weight_buffer[(bi0 >> (j * 8)) & 0xff]) {
                            highest = weight_buffer[(bi0 >> (j * 8)) & 0xff];
                            highest_index = (bi0 >> (j * 8)) & 0xff;
                        }
                        if (highest < weight_buffer[(bi1 >> (j * 8)) & 0xff]) {
                            highest = weight_buffer[(bi1 >> (j * 8)) & 0xff];
                            highest_index = (bi1 >> (j * 8)) & 0xff;
                        }
                        if (highest < weight_buffer[(bi2 >> (j * 8)) & 0xff]) {
                            highest = weight_buffer[(bi2 >> (j * 8)) & 0xff];
                            highest_index = (bi2 >> (j * 8)) & 0xff;
                        }
                    }

                    if (highest_index == -1) {
                        break;
                    }

                    weight_buffer[highest_index] = 0.0f;
                    *bone_index |= highest_index << (i * 8);
                    ((F32*)bone_weight)[i] = highest;
                }

                weight_buffer[bi0 & 0xff] = 0.0f;
                weight_buffer[(bi0 >> 8) & 0xff] = 0.0f;
                weight_buffer[(bi0 >> 16) & 0xff] = 0.0f;
                weight_buffer[(bi0 >> 24) & 0xff] = 0.0f;
                weight_buffer[bi1 & 0xff] = 0.0f;
                weight_buffer[(bi1 >> 8) & 0xff] = 0.0f;
                weight_buffer[(bi1 >> 16) & 0xff] = 0.0f;
                weight_buffer[(bi1 >> 24) & 0xff] = 0.0f;
                weight_buffer[bi2 & 0xff] = 0.0f;
                weight_buffer[(bi2 >> 8) & 0xff] = 0.0f;
                weight_buffer[(bi2 >> 16) & 0xff] = 0.0f;
                weight_buffer[(bi2 >> 24) & 0xff] = 0.0f;
            }
        }

        const RwMatrix* isbmat = RpSkinGetSkinToBoneMatrices(iskin);
        RpSkin* skin = RpSkinCreate(vert_size, bone_size, weights, indices, (RwMatrix*)isbmat);
        xMEMPOPTEMP(indices);
        xMEMPOPTEMP(weights);

        if (!skin) {
            return false;
        }

        RpSkinAtomicSetHAnimHierarchy(atomic, RpSkinAtomicGetHAnimHierarchy(iatomic));
        RpSkinGeometrySetSkin(geom, skin);
        RpSkinAtomicSetType(atomic, RpSkinAtomicGetType(iatomic));
        return true;
    }

    inline void copy_atomic_tris(RpAtomic* atomic, const RpTriangle* tri)
    {
        RpGeometry* geom = RpAtomicGetGeometry(atomic);
        RpTriangle* otri = RpGeometryGetTriangles(geom);
        S32 tri_size = RpGeometryGetNumTriangles(geom);
        memcpy(otri, tri, tri_size * sizeof(RpTriangle));
    }

    void complete_decal_atomic(RpAtomic* atomic, const xVec2* uv, RwSphere* o)
    {
        RpGeometry* geom = RpAtomicGetGeometry(atomic);
        RpTriangle* otri = RpGeometryGetTriangles(geom);
        RpMorphTarget* mt = RpGeometryGetMorphTarget(geom, 0);
        xVec2* ouv = (xVec2*)RpGeometryGetVertexTexCoords(geom, rwTEXTURECOORDINATEINDEX0);
        S32 tri_size = RpGeometryGetNumTriangles(geom);
        S32 vert_size = RpGeometryGetNumVertices(geom);

        memcpy(ouv, uv, vert_size * sizeof(xVec2));

        if (o) {
            atomic->boundingSphere = *o;
        } else {
            RwSphere bound;
            RpMorphTargetCalcBoundingSphere(mt, &bound);
            atomic->boundingSphere = bound;
        }

        mt->boundingSphere = atomic->boundingSphere;

        if (RpSkinGeometryGetSkin(geom)) {
            atomic->boundingSphere.radius *= 2.0f;
            mt->boundingSphere.radius = atomic->boundingSphere.radius;
        }

        RpGeometryUnlock(geom);
    }

    inline void set_vert(RwIm3DVertex& vert, const xVec3& loc, F32 u, F32 v, xColor color)
    {
        RwIm3DVertexSetPos(&vert, loc.x, loc.y, loc.z);
        vert.u = u;
        vert.v = v;
        RwIm3DVertexSetRGBA(&vert, color.r, color.g, color.b, color.a);
    }

    void obb_to_capsule(xLine3& line, F32& radius, const xBox& box, const xMat4x3& mat)
    {
        F32 dx = box.upper.x - box.lower.x, dy = box.upper.y - box.lower.y, dz = box.upper.z - box.lower.z;

        line.p1.x = box.lower.x + 0.5f * dx;
        line.p1.y = box.lower.y + 0.5f * dy;
        line.p1.z = box.lower.z;
        xMat4x3Toworld(&line.p1, &mat, &line.p1);
        line.p2.x = line.p1.x + mat.at.x * dz;
        line.p2.y = line.p1.y + mat.at.y * dz;
        line.p2.z = line.p1.z + mat.at.z * dz;

        xVec3 diag;
        xMat4x3Toworld(&diag, &mat, &box.lower);
        diag -= line.p1;
        F32 rd1 = diag.length2();
        xMat4x3Toworld(&diag, &mat, &box.upper);
        diag -= line.p2;
        F32 rd2 = diag.length2();
        radius = rd1 > rd2 ? xsqrt(rd1) : xsqrt(rd2);
    }

    immediate_mode_data* alloc_immediate_data()
    {
        immediate_mode_data* ret = immediate_free;
        if (immediate_free) {
            immediate_free = immediate_free->next;
        }
        return ret;
    }

    void free_immediate_data(immediate_mode_data* im)
    {
        im->next = immediate_free;
        immediate_free = im;
    }

    owner_group_type* find_owner_group(U32 owner)
    {
        owner_group_list::node_type* it = owner_groups.begin();
        owner_group_list::node_type* const end = owner_groups.end();
        for (; it != end; it = it->next) {
            if (it->value.owner == owner) {
                return &it->value;
            }
        }

        return NULL;
    }

    owner_group_type* create_owner_group(U32 owner)
    {
        owner_groups.push_back();
        owner_group_type& owner_group = owner_groups.back();
        owner_group.owner = owner;
        owner_group.size = 0;
        owner_group.max_size = 0;
        owner_group.head = NULL;
        owner_group.tail = NULL;
        return &owner_group;
    }

    void remove_from_owner_group(decal_instance& decal)
    {
        owner_group_type* owner_group = decal.owner_group;
        if (!owner_group) {
            return;
        }

        decal.owner_group = NULL;

        owner_group->size--;

        if (!decal.group_prev)
            owner_group->head = decal.group_next;
        else
            decal.group_prev->group_next = decal.group_next;

        if (!decal.group_next)
            owner_group->tail = decal.group_prev;
        else
            decal.group_next->group_prev = decal.group_prev;

        if (owner_group->size <= 0) {
            owner_groups.erase(owner_groups.get_iterator(owner_group));
        }
    }

    void queue_removal(decal_instance& decal)
    {
        remove_from_owner_group(decal);
        decal.ec.flags &= ~(emit_context::FLAG_PERMANENT | emit_context::FLAG_PAUSE);
        if (decal.ec.accel_aging > 1.0f) {
            decal.age_rate = decal.ec.accel_aging;
        }
    }

    void add_to_owner_group(decal_instance& decal, owner_group_type* owner_group)
    {
        if (!owner_group) {
            if (owner_groups.full()) {
                return;
            }

            owner_group = create_owner_group(decal.ec.owner);
        }

        decal.owner_group = owner_group;
        if (!owner_group) {
            return;
        }

        if (owner_group->max_size < decal.ec.max_owned) {
            owner_group->max_size = decal.ec.max_owned;
        }

        if (!owner_group->tail) {
            owner_group->head = &decal;
            decal.group_prev = NULL;
        } else {
            decal.group_prev = owner_group->tail;
            decal.group_prev->group_next = &decal;
        }

        decal.group_next = NULL;
        owner_group->tail = &decal;
        owner_group->size++;

        if (owner_group->max_size > 0 && owner_group->size > owner_group->max_size) {
            queue_removal(*owner_group->head);
        }
    }

    void relink_in_owner_group(decal_instance& decal)
    {
        if (!decal.owner_group) {
            return;
        }

        if (!decal.group_prev)
            decal.owner_group->head = &decal;
        else
            decal.group_prev->group_next = &decal;

        if (!decal.group_next)
            decal.owner_group->tail = &decal;
        else
            decal.group_next->group_prev = &decal;
    }

#define MAX_DECALS 128

    decal_instance* alloc_decal()
    {
        if (decals_used >= MAX_DECALS) return NULL;

        decal_instance* decal = &decals[decals_used];
        decals_used++;
        decals_sorted = false;
        return decal;
    }

    void free_decal(decal_instance* decal)
    {
        decals_used--;
        decal_instance* move_decal = &decals[decals_used];
        if (decal != move_decal) {
            *decal = *move_decal;
            relink_in_owner_group(*decal);
        }

        decals_sorted = false;
    }

    void refresh_decal(decal_instance& decal)
    {
        if (!(decal.ec.flags & emit_context::FLAG_ANIMATED)) {
            return;
        }

        F32 t = decal.age * decal.time_scale + decal.time_start;
        decal.ec.curve->eval_linear(t, &decal.ani.r);

        decal.ec.flags &= ~emit_context::FLAG_HIDDEN;

        if (decal.ec.flags & emit_context::FLAG_ANIM_COLOR) {
            if (decal.ec.blend_mode == BLEND_MODE_BRIGHT) {
                decal.color.r = (U8)(255.0f * decal.ani.r + 0.5f);
                decal.color.g = (U8)(255.0f * decal.ani.g + 0.5f);
                decal.color.b = (U8)(255.0f * decal.ani.b + 0.5f);
                decal.color.a = (U8)(255.0f * decal.ani.a + 0.5f);
                if ((U32&)decal.color == 0) {
                    decal.ec.flags |= emit_context::FLAG_HIDDEN;
                }

                S32 min_alpha = xFXHighDynamicRangeGetBackgroundGlow();
                if (decal.color.a < min_alpha) {
                    decal.color.a = min_alpha;
                }
            } else {
                decal.color.a = (U8)(255.0f * decal.ani.a + 0.5f);
                if (decal.color.a == 0)
                    decal.ec.flags |= emit_context::FLAG_HIDDEN;
                else {
                    decal.color.r = (U8)(255.0f * decal.ani.r + 0.5f);
                    decal.color.g = (U8)(255.0f * decal.ani.g + 0.5f);
                    decal.color.b = (U8)(255.0f * decal.ani.b + 0.5f);
                }
            }
        }

        if (decal.ec.flags & emit_context::FLAG_ANIM_UV) {
            if (xfeq0(decal.ani.scale_x) || xfeq0(decal.ani.scale_y)) {
                decal.ec.flags |= emit_context::FLAG_HIDDEN;
                return;
            }

            F32 ix = 1.0f / decal.ani.scale_x, iy = 1.0f / decal.ani.scale_y;
            decal.uvmat.right.assign(ix, 0.0f);
            decal.uvmat.up.assign(0.0f, iy);
            decal.uvmat.pos.assign(0.5f * (1.0f - ix), 0.5f * (1.0f - iy));
        }
    }

    decal_instance* create_decal(const emit_context& ec, const xMat4x3* driver, owner_group_type* owner_group)
    {
        decal_instance* decal = alloc_decal();
        if (!decal) return NULL;

        decal->ec = ec;

        decal->driver = driver;
        decal->texture = ec.texture;
        decal->color = g_WHITE;
        decal->age = 0.0f;
        decal->age_rate = 1.0f;
        decal->driver_ent = NULL;
        decal->driver_atomic = NULL;
        decal->uvmat.right.assign(1.0f, 0.0f);
        decal->uvmat.up.assign(0.0f, 1.0f);
        decal->uvmat.pos.assign(0.0f, 0.0f);

        if (!ec.owner) {
            decal->owner_group = NULL;
            decal->group_prev = decal->group_next = NULL;
        } else {
            add_to_owner_group(*decal, owner_group);
        }

        RwTextureSetAddressing(ec.texture, rwTEXTUREADDRESSCLAMP);

        if (decal->ec.curve) {
            decal->ec.flags |= emit_context::FLAG_ANIMATED;
            F32 anistart = decal->ec.curve->start_t();
            F32 aniend = decal->ec.curve->end_t();
            decal->time_start = anistart;
            decal->time_scale = (aniend - anistart) / decal->ec.life;
        }

        static const U16 rwmodes[MAX_BLEND_MODE][2] = {
            { rwBLENDSRCALPHA, rwBLENDINVSRCALPHA },
            { rwBLENDSRCALPHA, rwBLENDONE },
            { rwBLENDONE, rwBLENDONE }
        };

        decal->blend_mode[0] = rwmodes[ec.blend_mode][0];
        decal->blend_mode[1] = rwmodes[ec.blend_mode][1];

        if (ec.blend_mode == BLEND_MODE_BRIGHT) {
            xFXHighDynamicRangeTunePalette(RwTextureGetRaster(ec.texture), 1.0f, true);
        }

        refresh_decal(*decal);
        return decal;
    }

    void destroy_decal(decal_instance& decal)
    {
        remove_from_owner_group(decal);

        switch (decal.type) {
        case DECAL_TYPE_CARD:
            break;
        case DECAL_TYPE_ATOMIC:
            if (decal.ec.destAtomicCB) {
                decal.ec.destAtomicCB(decal.data.atomic);
            }
            destroy_atomic(decal.data.atomic);
            break;
        case DECAL_TYPE_IMMEDIATE:
            free_immediate_data(decal.data.immediate);
            break;
        }
    }

    bool check_skip_emit(const emit_context& ec, owner_group_type* owner_group)
    {
        if (ec.hard_max_owned > 0 && owner_group && owner_group->size >= ec.hard_max_owned) {
            return true;
        }

        return false;
    }

    void set_immediate_indices(U16* index, const RpTriangle* tri, S32 tri_size)
    {
        const RpTriangle* end_tri = tri + tri_size;
        while (tri != end_tri) {
            index[0] = tri->vertIndex[0];
            index[1] = tri->vertIndex[1];
            index[2] = tri->vertIndex[2];
            index += 3;
            tri++;
        }
    }

    S32 compare_decals(const void* e1, const void* e2)
    {
        const decal_instance* d1 = *(const decal_instance**)e1;
        const decal_instance* d2 = *(const decal_instance**)e2;

        if (d1->type != d2->type) return d1->type - d2->type;
        if (d1->texture != d2->texture) {
            return d1->texture - d2->texture;
        }
        if (d1->blend_mode_id != d2->blend_mode_id) {
            return d1->blend_mode_id - d2->blend_mode_id;
        }
        return d1->driver - d2->driver;
    }

    void begin_render()
    {
        xRenderStateSetZBias(tweak::zbias);
        old_zRenderState = zRenderStateCurrent();
        zRenderState(SDRS_Decal);
        iCameraSetFogRenderStates();
    }

    void end_render(const xMat4x3*)
    {
        xRenderStateResetZBias();
        zRenderState(old_zRenderState);
        xFXHighDynamicRangeBrighten(false, false);
    }

    void flush_render_immediate(const xMat4x3* mat)
    {
        if (immediate_indices_used > 0) {
            RwIm3DVertex* verts = gRenderBuffer.m_vertex;
            U16* indices = gRenderBuffer.m_index;
            xIMRenderIndexedLegacy(verts, immediate_verts_used, (const RwMatrix*)mat, rwPRIMTYPETRILIST, indices,
                                   immediate_indices_used);
            immediate_verts_used = immediate_indices_used = 0;
        }
    }

    void begin_render_immediate()
    {
        immediate_verts_used = immediate_indices_used = 0;
    }

    void end_render_immediate(const xMat4x3* mat)
    {
        flush_render_immediate(mat);
    }

    inline void render_decal_atomic(const decal_instance& decal)
    {
        RpGeometry* geom = RpAtomicGetGeometry(decal.data.atomic);
        RpMaterial* material = RpGeometryGetMaterial(geom, 0);

        if (decal.ec.flags & emit_context::FLAG_ANIMATED) {
            RpMaterialSetColor(material, &decal.color.rgba);
        }

        if (decal.ec.flags & emit_context::FLAG_ANIM_UV) {
            RwMatrix uvmat;
            copy_uvmat(uvmat, decal.uvmat);
            RpMatFXMaterialSetUVTransformMatrices(material, &uvmat, NULL);
        }

        if (decal.driver) {
            if (xModelSphereCull(iModelWorldBoundSphere(decal.data.atomic, (RwMatrix*)decal.driver, NULL))) {
                return;
            }

            RpHAnimHierarchy* heir = RpSkinAtomicGetHAnimHierarchy(decal.data.atomic);
            RwMatrix* oldheirmat;
            if (heir) {
                oldheirmat = heir->pMatrixArray;
                heir->pMatrixArray = (RwMatrix*)(decal.driver + 1);
            }

            RwFrame* frame = RpAtomicGetFrame(decal.data.atomic);
            frame->ltm = *(const RwMatrix*)decal.driver;

            RwMatrixUpdate(&frame->ltm);
            RpAtomicRender(decal.data.atomic);

            if (heir) {
                heir->pMatrixArray = oldheirmat;
            }
        } else {
            if (xModelSphereCull(iModelWorldBoundSphere(decal.data.atomic, (RwMatrix*)&g_I3, NULL))) {
                return;
            }

            RpAtomicRender(decal.data.atomic);
        }
    }

    inline void render_decal_immediate(const decal_instance& decal)
    {
        const immediate_mode_data& im = *decal.data.immediate;

        if (immediate_verts_used + im.vert_size > IRENDER_NUM_VERTEX ||
            immediate_indices_used + im.index_size > IRENDER_NUM_INDEX) {
            flush_render_immediate(decal.driver);
        }

        const U16* iindex = im.index;
        const U16* end_iindex = iindex + im.index_size;
        U16* oindex = gRenderBuffer.m_index + immediate_indices_used;
        while (iindex != end_iindex) {
            *oindex = *iindex + immediate_verts_used;
            iindex++, oindex++;
        }

        immediate_indices_used += im.index_size;

        RwIm3DVertex* overt = gRenderBuffer.m_vertex + immediate_verts_used;
        const xVec3* ivert = im.vert;
        const xVec3* end_ivert = ivert + im.vert_size;
        const xVec2* uv = im.uv;
        F32 rx = decal.uvmat.right.x, ry = decal.uvmat.right.y;
        F32 ux = decal.uvmat.up.x, uy = decal.uvmat.up.y;
        F32 px = decal.uvmat.pos.x, py = decal.uvmat.pos.y;
        for (; ivert != end_ivert; ++overt, ++ivert, ++uv) {
            F32 u = px + (rx * uv->x + ux * uv->y);
            F32 v = py + (ry * uv->x + uy * uv->y);
            set_vert(*overt, *ivert, u, v, decal.color);
        }

        immediate_verts_used += im.vert_size;
    }

    struct intersect_env_params
    {
        xVec3* v;
        xVec3* endv;
        U8* outcode;
        xBox box;
        xMat4x3 imat;
        const xMat4x3* mat;
    };

    S32 intersect_env_callback(xClumpCollBSPTriangle* tri, void* data)
    {
        intersect_env_params& arg = *(intersect_env_params*)data;

        S32 pflags = 1;
        for (; pflags & 0x1; pflags = tri->flags, tri++) {
            if (!(tri->flags & 0x8)) {
                continue;
            }

            if (arg.v >= arg.endv) {
                return 0;
            }

            arg.v[0] = (const xVec3&)tri->v.p[0];
            if (tri->flags & 0x2) {
                arg.v[1] = (const xVec3&)tri->v.p[2];
                arg.v[2] = (const xVec3&)tri->v.p[1];
            } else {
                arg.v[1] = (const xVec3&)tri->v.p[1];
                arg.v[2] = (const xVec3&)tri->v.p[2];
            }

            xMat4x3Toworld(&arg.v[0], &arg.imat, &arg.v[0]);
            xMat4x3Toworld(&arg.v[1], &arg.imat, &arg.v[1]);
            xMat4x3Toworld(&arg.v[2], &arg.imat, &arg.v[2]);

            F32 c1 = (arg.v[1].x - arg.v[0].x) * (arg.v[2].y - arg.v[1].y);
            F32 c2 = (arg.v[1].y - arg.v[0].y) * (arg.v[2].x - arg.v[1].x);
            if (c1 >= c2) {
                continue;
            }

            arg.outcode[0] = get_outcode(arg.v[0], arg.box.lower, arg.box.upper);
            arg.outcode[1] = get_outcode(arg.v[1], arg.box.lower, arg.box.upper);
            arg.outcode[2] = get_outcode(arg.v[2], arg.box.lower, arg.box.upper);

            S32 out_intersect = arg.outcode[0] & arg.outcode[1] & arg.outcode[2];
            if (!out_intersect) {
                arg.v += 3;
                arg.outcode += 3;
            }
        }

        return 1;
    }

    void intersect_env(intersect_env_params& arg)
    {
        const xEnv* env = xglobals->sceneCur->env;

        RwLine line;
        F32 radius;
        obb_to_capsule((xLine3&)line, radius, arg.box, *arg.mat);

        RwV3d delta;
        RwV3dSub(&delta, &line.end, &line.start);

        xClumpCollV3dGradient grad;
        xClumpCollV3dGradientMacro(&grad, &delta);

        for (S32 i = 0; i < env->geom->jsp_count && arg.v < arg.endv; i++) {
            if (xEnvIsJSPActive(env, i)) {
                xClumpColl_ForAllCapsuleLeafNodeIntersections(env->geom->jsp_list[i]->colltree, &line, radius, &grad,
                                                              intersect_env_callback, &arg);
            }
        }
    }

    inline bool check_destructible(xEnt& ent)
    {
        switch (ent.baseType) {
        case eBaseTypeStatic:
            return ((zEntSimpleObj&)ent).destructible != NULL;
        case eBaseTypePlatform:
            return ((zPlatform&)ent).destructible != NULL;
        case eBaseTypeRubbleChunk:
            return true;
        }

        return false;
    }

    struct cb_decal_hits_ent
    {
        const xBox& box;
        const xMat4x3& mat;
        const xSphere& o;
        const emit_context& ec;
        S32 check;
        bool skip_destructible;

        cb_decal_hits_ent(const xBox& box, const xMat4x3& mat, const xSphere& o, const emit_context& ec, S32 check,
                          bool skip_destructible)
            : box(box), mat(mat), o(o), ec(ec), check(check), skip_destructible(skip_destructible) {}

        bool operator()(xEnt& ent)
        {
            if (!(ent.collType & check)) {
                return true;
            }
            if (!xEntIsVisible(&ent)) {
                return true;
            }
            if (!ent.model) {
                return true;
            }
            if (skip_destructible && check_destructible(ent)) {
                return true;
            }
            if (!xOBBHitsBound(box, mat, ent.bound)) {
                return true;
            }

            xDecal::emit(ec, mat, *ent.model, xEntMoves(ent), &ent);

            return true;
        }
    };

    struct cb_decal_sphere_hits_ent
    {
        xMat4x3& mat;
        const xSphere& o;
        const emit_context& ec;
        S32 check;
        bool skip_destructible;

        cb_decal_sphere_hits_ent(xMat4x3& mat, const xSphere& o, const emit_context& ec, S32 check, bool skip_destructible)
            : mat(mat), o(o), ec(ec), check(check), skip_destructible(skip_destructible) {}

        bool operator()(xEnt& ent)
        {
            if (!(ent.collType & check)) {
                return true;
            }
            if (!xEntIsVisible(&ent)) {
                return true;
            }
            if (!ent.model) {
                return true;
            }
            if (skip_destructible && check_destructible(ent)) {
                return true;
            }

            xCollis coll;
            coll.dist = HUGE;
            coll.flags = 0x1100;
            xSphereHitsBound(&o, &ent.bound, &coll);
            if (!(coll.flags & k_HIT_IT)) {
                return true;
            }

            xMat3x3LookVec(&mat, &coll.hdng.negate());
            xDecal::emit(ec, mat, *ent.model, xEntMoves(ent), &ent);
            return true;
        }
    };

    inline bool check_driver_hidden(const decal_instance& decal)
    {
        if (!decal.driver_ent) {
            return false;
        }
        if (!xEntIsVisible(decal.driver_ent)) {
            return true;
        }
        if (decal.driver_atomic != decal.driver_ent->model->Data) {
            return true;
        }
        return false;
    }

    inline void debug_init() {}
    inline void debug_update(F32) {}

    inline void debug_render_begin(const xMat4x3&) {}
    inline void debug_render() {}
}

#define MAX_IMMEDIATE 64
#define MAX_OWNER_GROUPS 64

namespace xDecal
{
    char* blend_mode_labels[] = { "Average", "Additive", "Bright (Sun-Shiny)" };
    char* animix_labels[] = { "color \001red", "color \002green", "color \003blue", "color alpha", "scale u", "scale v" };
}

void xDecal::scene_enter()
{
    decals = new (eMemStaticType_0, 0) decal_instance[MAX_DECALS];
    render_list = new (eMemStaticType_0, 0) decal_instance*[MAX_DECALS];
    immediate_buffer = new (eMemStaticType_0, 0) immediate_mode_data[MAX_IMMEDIATE];
    owner_groups.create(MAX_OWNER_GROUPS, NULL);
    debug_init();
    decals_used = 0;
    reset();
}

void xDecal::scene_exit()
{
    decal_instance* decal = decals;
    decal_instance* end_decal = decal + decals_used;
    while (decal != end_decal) {
        destroy_decal(*decal);
        decal++;
    }
}

void xDecal::reset()
{
    decal_instance* decal = decals;
    decal_instance* end_decal = decal + decals_used;
    while (decal != end_decal) {
        destroy_decal(*decal);
        decal++;
    }

    immediate_mode_data* idata = immediate_buffer;
    immediate_mode_data* end_idata = idata + MAX_IMMEDIATE - 1;
    while (idata != end_idata) {
        idata->next = idata + 1;
        idata++;
    }
    idata->next = NULL;
    immediate_free = immediate_buffer;

    owner_groups.reset();
    decals_used = 0;
    decals_sorted = true;
}

void xDecal::update(F32 dt)
{
    debug_update(dt);

    decal_instance* decal = decals;
    decal_instance* end_decal = decal + decals_used;
    for (; decal != end_decal; decal++) {
        if (!(decal->ec.flags & emit_context::FLAG_PERMANENT) && !((decal->ec.flags & emit_context::FLAG_PAUSE) && decal->age >= decal->ec.pause_time)) {
            decal->age += dt * decal->age_rate;

            if ((decal->ec.flags & emit_context::FLAG_PAUSE) && decal->age > decal->ec.pause_time) {
                decal->age = decal->ec.pause_time;
            }

            if (decal->age > decal->ec.life) {
                if (decal->ec.flags & emit_context::FLAG_CYCLE)
                    decal->age -= decal->ec.life;
                else {
                    destroy_decal(*decal);
                    free_decal(decal);
                    end_decal--;
                    decal--;
                    continue;
                }
            }

            refresh_decal(*decal);
        }
    }
}

void xDecal::render_all()
{
    debug_render();

    if (!decals_sorted) {
        decals_sorted = true;
        for (S32 i = 0; i < decals_used; i++) {
            render_list[i] = &decals[i];
        }
        qsort(render_list, decals_used, sizeof(decal_instance*), compare_decals);
    }

    begin_render();

    decal_instance* decal;
    decal_instance** it = render_list;
    decal_instance** const end = it + decals_used;
    U32 blend_mode = (U32)-1;
    while (it != end) {
        decal = *it;
        if (decal->type != DECAL_TYPE_ATOMIC) {
            break;
        }

        if (!(decal->ec.flags & emit_context::FLAG_HIDDEN) && !check_driver_hidden(*decal)) {
            if (blend_mode != decal->blend_mode_id) {
                blend_mode = decal->blend_mode_id;
                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)decal->blend_mode[0]);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)decal->blend_mode[1]);
                xFXHighDynamicRangeBrighten(decal->ec.blend_mode == BLEND_MODE_BRIGHT, false);
            }

            render_decal_atomic(*decal);
        }

        it++;
    }

    begin_render_immediate();

    RwTexture* texture = NULL;
    const xMat4x3* driver = NULL;
    for (; it != end; it++) {
        decal = *it;
        if (!(decal->ec.flags & emit_context::FLAG_HIDDEN) && !check_driver_hidden(*decal)) {
            if (texture != decal->texture || blend_mode != decal->blend_mode_id || driver != decal->driver) {
                flush_render_immediate(driver);
                driver = decal->driver;

                if (texture != decal->texture) {
                    texture = decal->texture;
                    RwRaster* raster = RwTextureGetRaster(texture);
                    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
                }

                if (blend_mode != decal->blend_mode_id) {
                    blend_mode = decal->blend_mode_id;
                    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)decal->blend_mode[0]);
                    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)decal->blend_mode[1]);
                    xFXHighDynamicRangeBrighten(decal->ec.blend_mode == BLEND_MODE_BRIGHT, false);
                }
            }

            render_decal_immediate(*decal);
        }
    }

    end_render_immediate(driver);
    end_render(NULL);
}

void xDecal::set_default_emit_context(emit_context& context)
{
    memset(&context, 0, sizeof(emit_context));
}

void xDecal::emit(const emit_context& ec, const xMat4x3& mat, S32 check_flags)
{
    debug_render_begin(mat);

    xBox box;
    F32 rx = 0.5f * ec.size.x, ry = 0.5f * ec.size.y;
    box.lower.assign(-rx, -ry, 0.0f);
    box.upper.assign(rx, ry, ec.max_dist);

    if (check_flags & k_XENT_COLLTYPE_ENV) {
        emit_env(ec, mat);
    }

    if (check_flags & k_XENT_COLLTYPE_PC) {
        if (xSphereHitsOBB(ZPLAYER->bound.sph.center, ZPLAYER->bound.sph.r, box, mat)) {
            emit(ec, mat, *ZPLAYER->model, true, ZPLAYER);
        }
    }

    if (check_flags & (k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC)) {
        xSphere o;
        iSphereInitBoundOBB(&o, &box, &mat);
        cb_decal_hits_ent cb(box, mat, o, ec, check_flags, (ec.flags & emit_context::FLAG_SKIP_DESTRUCTIBLE) != 0);

        xBound bound;
        bound.type = k_XBOUNDTYPE_BOX;
        xBoxInitBoundOBB(&bound.box.box, &box, &mat);
        xVec3Add(&bound.box.center, &bound.box.box.lower, &bound.box.box.upper);
        xVec3SMul(&bound.box.center, &bound.box.center, 0.5f);
        xQuickCullForBox(&bound.qcd, &bound.box.box);

        if (check_flags & (k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN)) {
            xGridCheckBound(colls_grid, bound, bound.qcd, cb);
            xGridCheckBound(colls_oso_grid, bound, bound.qcd, cb);
        }

        if (check_flags & k_XENT_COLLTYPE_NPC) {
            xGridCheckBound(npcs_grid, bound, bound.qcd, cb);
        }
    }
}

void xDecal::sphere_emit(const emit_context& ec, const xVec3& center, F32 radius, S32 check_flags)
{
    xSphere o;
    o.center = center;
    o.r = radius;

    xMat4x3 mat;
    mat.pos = center;

    xCollis coll;
    xVec3 best_dir = { 0.0f, -1.0f, 0.0f };

    if (check_flags & k_XENT_COLLTYPE_PC) {
        coll.dist = HUGE;
        coll.flags = 0x1100;
        xSphereHitsBound(&o, &ZPLAYER->bound, &coll);
        if ((coll.flags & k_HIT_IT) && coll.dist > 0.0f) {
            xMat3x3LookVec(&mat, &coll.hdng.negate());
            emit(ec, mat, *ZPLAYER->model, true, ZPLAYER);
        }
    }

    if (check_flags & k_XENT_COLLTYPE_ENV) {
        coll.flags = 0x1100;
        coll.dist = HUGE;
        if (xSphereHitsEnv(&o, xglobals->sceneCur->env, &coll) && coll.dist < o.r && coll.dist > 0.0f) {
            xMat3x3LookVec(&mat, &coll.hdng.negate());
            emit_env(ec, mat);
        }
    }

    if (check_flags & (k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC)) {
        cb_decal_sphere_hits_ent cb(mat, o, ec, check_flags, (ec.flags & emit_context::FLAG_SKIP_DESTRUCTIBLE) != 0);

        xBound bound;
        xBoundFromSphere(bound, center, radius);

        if (check_flags & (k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN)) {
            xGridCheckBound(colls_grid, bound, bound.qcd, cb);
            xGridCheckBound(colls_oso_grid, bound, bound.qcd, cb);
        }

        if (check_flags & k_XENT_COLLTYPE_NPC) {
            xGridCheckBound(npcs_grid, bound, bound.qcd, cb);
        }
    }
}

void xDecal::emit(const emit_context& ec, const xMat4x3& mat, const xModelInstance& model, bool moving_model, xEnt* driver_ent)
{
    for (const xModelInstance* m = &model; m; m = m->Next) {
        if (!(m->Flags & ModelInstance_NoRender) && (m->Flags & ModelInstance_Render)) {
            emit(ec, mat, m->Data, (const xMat4x3*)m->Mat, moving_model, driver_ent);
        }
    }
}

void xDecal::emit(const emit_context& ec, const xMat4x3& mat, RpAtomic* model, const xMat4x3* model_mat, bool moving_model,
                  xEnt* driver_ent)
{
    if (decals_used >= MAX_DECALS) {
        return;
    }

    owner_group_type* owner_group = !ec.owner ? NULL : find_owner_group(ec.owner);
    if (check_skip_emit(ec, owner_group)) {
        return;
    }

    RpGeometry* geom = RpAtomicGetGeometry(model);
    RpTriangle* tri = RpGeometryGetTriangles(geom);
    xVec3* vert = (xVec3*)RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geom, 0));
    S32 tri_size = RpGeometryGetNumTriangles(geom);
    S32 vert_size = RpGeometryGetNumVertices(geom);
    if (!vert || !tri) {
        return;
    }

    xMat4x3& mmat = *(xMat4x3*)model_mat;
    xMat4x3 oldmmat = mmat;
    xMat4x3 imat;
    xMat4x3OrthoInv(&imat, &mat);
    xMat4x3Mul(&mmat, &oldmmat, &imat);

    xVec3* decal_vert = (xVec3*)xMEMPUSHTEMP(vert_size * sizeof(xVec3));
    if (!decal_vert) {
        return;
    }

    iModelVertEval(model, 0, vert_size, (RwMatrix*)model_mat, vert, decal_vert);
    mmat = oldmmat;

    U8* temp_scratch = (U8*)xMemScratchPush(SCRATCH_SIZE, 0);
    U8* mem = temp_scratch;
    clip_decal_indexed_params* arg = (clip_decal_indexed_params*)mem;
    mem += sizeof(clip_decal_indexed_params);
    arg->buffer.clip_buffer = (clip_vertex*)mem;
    mem += MAX_CLIPPED_VERTS * 2 * sizeof(clip_vertex);
    arg->in.outcode = mem;
    mem += MAX_EMIT_VERTS * sizeof(U8);
    arg->buffer.vert_usage_buffer = (U16*)mem;
    mem += MAX_EMIT_VERTS * sizeof(U16);
    F32* weight_buffer = (F32*)mem;

    if (vert_size > MAX_EMIT_VERTS) {
        goto cleanup;
    }

    arg->in.tri = tri;
    arg->in.tri_size = tri_size;
    arg->in.vert = decal_vert;
    arg->in.vert_size = vert_size;
    arg->in.size = ec.size;
    arg->in.uv[0] = ec.uv[0];
    arg->in.uv[1] = ec.uv[1];

    F32 rx = 0.5f * ec.size.x, ry = 0.5f * ec.size.y;
    arg->in.box.lower.assign(-rx, -ry, 0.0f);
    arg->in.box.upper.assign(rx, ry, ec.max_dist);

    arg->out.max_tri_size = tri_size * 9 / 8 + CLIP_ADD_EXTRA_TRIANGLES;
    arg->out.max_vert_size = vert_size * 9 / 8 + CLIP_ADD_EXTRA_VERTICES;
    arg->out.tri = (RpTriangle*)xMEMPUSHTEMP(arg->out.max_tri_size * sizeof(RpTriangle));
    arg->out.vert = (combo_vertex*)xMEMPUSHTEMP(arg->out.max_vert_size * sizeof(combo_vertex));
    arg->out.uv = (xVec2*)xMEMPUSHTEMP(arg->out.max_vert_size * sizeof(xVec2));
    if (!arg->out.tri || !arg->out.vert || !arg->out.uv) {
        goto cleanup;
    }

    compute_outcodes((U8*)arg->in.outcode, arg->in.vert, arg->in.vert_size, arg->in.box);
    clip_decal_indexed(*arg);
    if (!arg->out.vert_size || !arg->out.tri_size) {
        goto cleanup;
    }

    bool mountable = !(ec.flags & emit_context::FLAG_NO_MOUNT);
    moving_model = mountable && moving_model;
    bool skinned = mountable && RpSkinGeometryGetSkin(RpAtomicGetGeometry(model));
    const xMat4x3* driver = (skinned || moving_model) ? model_mat : NULL;

    decal_instance* decal = create_decal(ec, driver, owner_group);
    decal->driver_ent = driver_ent;
    decal->driver_atomic = driver_ent->model->Data;

    if (!skinned && arg->out.vert_size <= MAX_IMMEDIATE_VERTS && arg->out.tri_size <= MAX_IMMEDIATE_TRIS && immediate_free) {
        decal->type = DECAL_TYPE_IMMEDIATE;
        immediate_mode_data* im = alloc_immediate_data();
        decal->data.immediate = im;

        im->vert_size = arg->out.vert_size;
        im->index_size = arg->out.tri_size * 3;
        memcpy(im->uv, arg->out.uv, im->vert_size * sizeof(xVec2));
        set_immediate_indices(im->index, arg->out.tri, arg->out.tri_size);

        if (decal->driver) {
            transform_combo_vertices(im->vert, arg->out.vert, arg->out.vert_size, vert);
        } else {
            transform_combo_vertices(im->vert, arg->out.vert, arg->out.vert_size, decal_vert);
            transform_points(im->vert, im->vert, arg->out.vert_size, mat);
        }
    } else {
        decal->type = DECAL_TYPE_ATOMIC;
        decal->data.atomic = create_decal_atomic(*decal, arg->out.vert_size, arg->out.tri_size);
        if (!decal->data.atomic) {
            free_decal(decal);
            goto cleanup;
        }

        if (skinned) {
            decal->ec.flags |= emit_context::FLAG_SKINNED;
            if (!transform_decal_atomic_skinned(decal->data.atomic, arg->out.vert, model, weight_buffer)) {
                destroy_atomic(decal->data.atomic);
                free_decal(decal);
                goto cleanup;
            }
        } else if (decal->driver) {
            transform_decal_atomic(decal->data.atomic, arg->out.vert, vert);
        } else {
            transform_decal_atomic(decal->data.atomic, arg->out.vert, decal_vert, mat);
        }

        copy_atomic_tris(decal->data.atomic, arg->out.tri);

        RwSphere* o = !driver ? NULL : &model->boundingSphere;
        complete_decal_atomic(decal->data.atomic, arg->out.uv, o);

        if (ec.emitAtomicCB) {
            ec.emitAtomicCB(decal->data.atomic, (xMat4x3*)decal->driver);
        }
    }

cleanup:
    if (arg->out.uv) xMEMPOPTEMP(arg->out.uv);
    if (arg->out.vert) xMEMPOPTEMP(arg->out.vert);
    if (arg->out.tri) xMEMPOPTEMP(arg->out.tri);
    xMEMPOPTEMP(decal_vert);
    xMemScratchPop(temp_scratch);
}

void xDecal::emit_env(const emit_context& ec, const xMat4x3& mat)
{
    if (decals_used >= MAX_DECALS) {
        return;
    }

    owner_group_type* owner_group = !ec.owner ? NULL : find_owner_group(ec.owner);
    if (check_skip_emit(ec, owner_group)) {
        return;
    }

    U8* temp_scratch = (U8*)xMemScratchPush(SCRATCH_SIZE, 0);
    U8* mem = temp_scratch;
    intersect_env_params* iarg = (intersect_env_params*)mem;
    mem += sizeof(intersect_env_params);
    clip_decal_params* carg = (clip_decal_params*)mem;
    mem += sizeof(clip_decal_params);
    carg->clip_buffer = (xVec3*)mem;
    mem += MAX_CLIPPED_VERTS * 2 * sizeof(xVec3);
    xVec3* decal_vert = (xVec3*)mem;
    mem += MAX_ENV_VERTS * sizeof(xVec3);
    U8* outcode = mem;

    carg->out.vert = NULL;
    carg->out.uv = NULL;

    iarg->v = decal_vert;
    iarg->endv = decal_vert + MAX_ENV_VERTS - 2;
    iarg->outcode = outcode;
    xMat4x3OrthoInv(&iarg->imat, &mat);
    iarg->mat = &mat;

    F32 rx = 0.5f * ec.size.x, ry = 0.5f * ec.size.y;
    iarg->box.lower.assign(-rx, -ry, 0.0f);
    iarg->box.upper.assign(rx, ry, ec.max_dist);

    intersect_env(*iarg);

    carg->in.vert_size = iarg->v - decal_vert;
    if (carg->in.vert_size <= 0) {
        goto cleanup;
    }

    carg->in.vert = decal_vert;
    carg->in.outcode = outcode;
    carg->in.box = iarg->box;
    carg->in.size = ec.size;
    carg->in.uv[0] = ec.uv[0];
    carg->in.uv[1] = ec.uv[1];

    carg->out.max_vert_size = carg->in.vert_size * 9 / 8 + CLIP_ADD_EXTRA_VERTICES;
    carg->out.vert = (xVec3*)xMEMPUSHTEMP(carg->out.max_vert_size * sizeof(xVec3));
    carg->out.uv = (xVec2*)xMEMPUSHTEMP(carg->out.max_vert_size * sizeof(xVec2));
    if (!carg->out.vert || !carg->out.uv) {
        goto cleanup;
    }

    clip_decal(*carg);
    if (carg->out.vert_size <= 0) {
        goto cleanup;
    }

    decal_instance* decal = create_decal(ec, NULL, owner_group);

    if (carg->out.vert_size <= MAX_IMMEDIATE_VERTS && immediate_free) {
        decal->type = DECAL_TYPE_IMMEDIATE;
        immediate_mode_data* im = alloc_immediate_data();
        decal->data.immediate = im;

        im->vert_size = carg->out.vert_size;
        im->index_size = carg->out.vert_size;
        memcpy(im->uv, carg->out.uv, im->vert_size * sizeof(xVec2));

        U16* index = im->index;
        for (U16 i = 0; i < im->vert_size; ++i, ++index) {
            *index = i;
        }

        transform_points(im->vert, carg->out.vert, im->vert_size, mat);
    } else {
        decal->type = DECAL_TYPE_ATOMIC;
        decal->data.atomic = create_decal_atomic(*decal, carg->out.vert_size, carg->out.vert_size / 3);
        if (!decal->data.atomic) {
            free_decal(decal);
            goto cleanup;
        }

        RpGeometry* geom = RpAtomicGetGeometry(decal->data.atomic);
        RwV3d* avert = RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geom, 0));
        RpTriangle* atri = RpGeometryGetTriangles(geom);

        transform_points((xVec3*)avert, carg->out.vert, carg->out.vert_size, mat);

        for (U16 i = 0; i != carg->out.vert_size;) {
            atri->vertIndex[0] = i; i++;
            atri->vertIndex[1] = i; i++;
            atri->vertIndex[2] = i; i++;
            atri++;
        }

        RpMorphTarget* mt = RpGeometryGetMorphTarget(geom, 0);
        RwSphere o;
        RpMorphTargetCalcBoundingSphere(mt, &o);
        RpMorphTargetSetBoundingSphere(mt, &o);

        complete_decal_atomic(decal->data.atomic, carg->out.uv, NULL);
    }

cleanup:
    if (carg->out.uv) xMEMPOPTEMP(carg->out.uv);
    if (carg->out.vert) xMEMPOPTEMP(carg->out.vert);
    xMemScratchPop(temp_scratch);
}

namespace
{
    inline void lerp(F32& v, F32 frac, F32 v0, F32 v1)
    {
        v = frac * (v1 - v0) + v0;
    }

    inline void lerp(U8& v, F32 frac, U8 v0, U8 v1)
    {
        v = (U8)(0.5f + (frac * ((F32)v1 - (F32)v0) + (F32)v0));
    }

    inline void lerp(xColor& v, F32 frac, xColor v0, xColor v1)
    {
        lerp(v.r, frac, v0.r, v1.r);
        lerp(v.g, frac, v0.g, v1.g);
        lerp(v.b, frac, v0.b, v1.b);
        lerp(v.a, frac, v0.a, v1.a);
    }

    enum { MAX_EMITTERS = 32 };

    xDecalEmitter* active_emitters[MAX_EMITTERS];
    U32 active_emitters_size;

    void register_emitter(xDecalEmitter* emitter)
    {
        if (active_emitters_size >= MAX_EMITTERS) return;

        active_emitters[active_emitters_size] = emitter;
        active_emitters_size++;
    }
}

void xDecalInit()
{
    active_emitters_size = 0;
}

void xDecalUpdate(F32 dt)
{
    for (size_t i = 0; i < active_emitters_size; i++) {
        if (active_emitters[i]->need_update()) {
            active_emitters[i]->update(dt);
        }
    }
}

void xDecalEmitter::init(S32 max_size, const char* debug_name)
{
    register_emitter(this);
    units.init(max_size);
    debug_init(debug_name);
}

void xDecalEmitter::set_default_config()
{
    cfg.flags = 0;
    cfg.life_time = 1.0f;
    cfg.blend_src = rwBLENDSRCALPHA;
    cfg.blend_dst = rwBLENDONE;
    cfg.texture.uv[0].assign(0.0f, 0.0f);
    cfg.texture.uv[1].assign(1.0f, 1.0f);
    cfg.texture.rows = cfg.texture.cols = 1;
    cfg.texture.mode = TM_DEFAULT;
    refresh_config();
}

void xDecalEmitter::set_texture(const char* name) { set_texture(xStrHash(name)); }

void xDecalEmitter::set_texture(U32 id)
{
    set_texture(!id ? (RwTexture*)NULL : (RwTexture*)xSTFindAsset(id, NULL));
}

void xDecalEmitter::set_texture(RwTexture* texture)
{
    this->texture.asset = texture;
    this->texture.prev = 0;
}

void xDecalEmitter::refresh_config()
{
    if (cfg.life_time < 0.001f) cfg.life_time = 0.001f;

    ilife = 1.0f / cfg.life_time;

    if (cfg.texture.rows == 0) cfg.texture.rows = 1;

    if (cfg.texture.cols == 0) cfg.texture.cols = 1;

    cfg.texture.uv[0].x = range_limit(cfg.texture.uv[0].x, 0.0f, 0.99f);
    cfg.texture.uv[0].y = range_limit(cfg.texture.uv[0].y, 0.0f, 0.99f);
    cfg.texture.uv[1].x = range_limit(cfg.texture.uv[1].x, cfg.texture.uv[0].x + 0.01f, 1.0f);
    cfg.texture.uv[1].y = range_limit(cfg.texture.uv[1].y, cfg.texture.uv[0].y + 0.01f, 1.0f);
    texture.units = cfg.texture.rows * cfg.texture.cols;
    texture.size.x = (cfg.texture.uv[1].x - cfg.texture.uv[0].x) / cfg.texture.cols;
    texture.size.y = (cfg.texture.uv[1].y - cfg.texture.uv[0].y) / cfg.texture.rows;
    texture.isize.assign(1.0f / texture.size.x, 1.0f / texture.size.y);
}

void xDecalEmitter::set_curve(const curve_node* curve, size_t size)
{
    curve_size = size;
    this->curve = curve;
    debug_update_curve();
}

namespace
{
    inline F32 choose_greatest(F32 a, F32 b, F32 c)
    {
        if (a < b) a = b;
        if (a < c) a = c;
        return a;
    }
}

void xDecalEmitter::emit(const xMat4x3& mat, S32 texture_index)
{
    if (units.full()) units.pop_back();

    units.push_front();
    unit_data& unit = units.front();
    unit.age = 0.0f;

    if (cfg.flags & FLAG_ORIENT_CAMERA) {
        unit.mat.right.assign(1.0f, 1.0f, 1.0f);
        unit.mat.pos = mat.pos;
        unit.cull_size = 0.5f;
    } else {
        unit.mat = mat;
        unit.cull_size = 0.5f * choose_greatest(unit.mat.right.length(), unit.mat.up.length(), unit.mat.at.length());
    }

    if (texture_index < 0 || texture_index > (S32)texture.units) {
        texture_index = select_texture_unit();
    }

    unit.u = texture_index % cfg.texture.rows;
    unit.v = texture_index / cfg.texture.rows;
}

void xDecalEmitter::emit(const xMat4x3& mat, const xVec3& scale, S32 texture_index)
{
    if (units.full()) units.pop_back();

    units.push_front();
    unit_data& unit = units.front();
    unit.age = 0.0f;

    if (cfg.flags & FLAG_ORIENT_CAMERA) {
        unit.mat.right = scale;
        unit.mat.pos = mat.pos;
        unit.cull_size = 0.5f * choose_greatest(scale.x, scale.y, scale.z);
    } else {
        unit.mat = mat;
        unit.mat.right *= scale.x;
        unit.mat.up *= scale.y;
        unit.mat.at *= scale.z;
        unit.cull_size = 0.5f * choose_greatest(unit.mat.right.length(), unit.mat.up.length(), unit.mat.at.length());
    }

    if (texture_index < 0 || texture_index > (S32)texture.units) {
        texture_index = select_texture_unit();
    }

    unit.u = texture_index % cfg.texture.rows;
    unit.v = texture_index / cfg.texture.rows;
}

void xDecalEmitter::update(F32 dt)
{
    debug_update(dt);

    F32 dage = dt * ilife;

    ptank_pool__color_mat_uv2 pool;
    pool.reset();
    pool.rs.texture = texture.asset;
    pool.rs.src_blend = cfg.blend_src;
    pool.rs.dst_blend = cfg.blend_dst;
    pool.rs.flags = cfg.flags & FLAG_NOZTEST;

    S32 min_alpha;
    if (cfg.flags & FLAG_BRIGHT) {
        pool.rs.flags |= ptank_pool::FLAG_BRIGHT;
        min_alpha = xFXHighDynamicRangeGetBackgroundGlow();
    } else {
        min_alpha = 0;
    }

    curve_index = 0;

    static_queue<unit_data>::iterator it = units.begin();
    while (it != units.end()) {
        unit_data& unit = *it;
        unit.age += dage;
        if (unit.age >= 1.0f) {
            break;
        }

        update_frac(unit);

        const curve_node& node0 = curve[unit.curve_index];
        const curve_node& node1 = curve[unit.curve_index + 1];
        F32 scale;
        lerp(scale, unit.frac, node0.scale, node1.scale);

        (F32&)unit.mat.pad3 = unit.cull_size * scale;
        RwSphere worldsph;
        worldsph.center = (const RwV3d&)unit.mat.pos;
        worldsph.radius = (const F32&)unit.mat.pad3;

        if (RwCameraFrustumTestSphere(xglobals->screen->icam, &worldsph)) {
            pool.next();
            if (!pool.valid()) {
                break;
            }

            get_render_data(unit, scale, *pool.color, *pool.mat, pool.uv[0], pool.uv[1], min_alpha);
        }

        ++it;
    }

    pool.flush();
    units.erase(it, units.end());
}

void xDecalEmitter::update_frac(unit_data& unit)
{
    for (; curve_index < curve_size - 2; curve_index++) {
        if (unit.age >= curve[curve_index].time && unit.age <= curve[curve_index + 1].time) {
            break;
        }
    }

    unit.curve_index = curve_index;
    const curve_node& node0 = curve[curve_index];
    const curve_node& node1 = curve[curve_index + 1];
    F32 itime = 1.0f / (node1.time - node0.time);
    unit.frac = itime * (unit.age - node0.time);
}

void xDecalEmitter::get_render_data(const unit_data& unit, F32 scale, xColor& color, xMat4x3& mat, xVec2& uv0, xVec2& uv1,
                                    S32 min_alpha)
{
    const curve_node& node0 = curve[unit.curve_index];
    const curve_node& node1 = curve[unit.curve_index + 1];
    lerp(color, unit.frac, node0.color, node1.color);

    if (color.a < min_alpha) color.a = min_alpha;

    if (cfg.flags & FLAG_ORIENT_CAMERA) {
        (xMat3x3&)mat = xglobals->cam->mat;
        mat.right *= scale * unit.mat.right.x;
        mat.up *= scale * unit.mat.right.y;
        mat.at *= scale * unit.mat.right.z;
        mat.pos = unit.mat.pos;
    } else {
        mat = unit.mat;
        mat.right *= scale;
        mat.up *= scale;
        mat.at *= scale;
    }

    uv0.x = unit.u * texture.size.x + cfg.texture.uv[0].x;
    uv0.y = unit.v * texture.size.y + cfg.texture.uv[0].y;
    uv1.x = uv0.x + texture.size.x;
    uv1.y = uv0.y + texture.size.y;
}

S32 xDecalEmitter::select_texture_unit()
{
    switch (cfg.texture.mode) {
    case TM_DEFAULT:
        break;
    case TM_RANDOM:
        return (xrand_RandomInt32() >> 13) % texture.units;
    case TM_CYCLE:
        return ++texture.prev % texture.units;
    }

    return 0;
}

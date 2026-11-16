#include "xCommon.h"
#include "xPtankPool.h"

#include "iAsync.h"
#include "iMemMgr.h"
#include "xFXHighDynamicRange.h"
#include "xGlobals.h"
#include "xMemMgr.h"
#include "xutil.h"
#include "zRenderState.h"
#include "zScene.h"

#include <stdlib.h>
#include <string.h>

namespace {
    S32 MAX_PTANKS_COLOR_MAT = 4;
    S32 MAX_PTANKS_COLOR_MAT_UV2 = 8;
    S32 MAX_PTANKS_POS_COLOR_SIZE = 12;
    S32 MAX_PTANKS_POS_COLOR_SIZE_UV2 = 16;
    S32 MAX_PTANKS_POS_COLOR_SIZE_ROT = 12;
    S32 MAX_PTANKS_POS_COLOR_SIZE_ROT_UV2 = 8;
    S32 MAX_PTANKS = MAX_PTANKS_COLOR_MAT + MAX_PTANKS_COLOR_MAT_UV2 + MAX_PTANKS_POS_COLOR_SIZE +
                     MAX_PTANKS_POS_COLOR_SIZE_UV2 + MAX_PTANKS_POS_COLOR_SIZE_ROT +
                     MAX_PTANKS_POS_COLOR_SIZE_ROT_UV2;

    bool inited;
    U32 current_sequence_index;

    struct ptank_context
    {
        ptank_context* next;
        RpAtomic* ptank;
        S32 flags;
        U32 src_blend;
        U32 dst_blend;
        U32 order_group;
        S32 order_index;
        U32 sequence_index;
    };

    struct group_data
    {
        S32 max_size;
        U32 create_flags;
        ptank_context* ptanks;
        ptank_context** buckets;
        S32 size;
        S32 used;
        S32 buckets_used;
    };

    group_data groups[MAX_PGT] = {
        { MAX_PTANKS_COLOR_MAT, rpPTANKDFLAGCNSVTX2TEXCOORDS | rpPTANKDFLAGMATRIX | rpPTANKDFLAGCOLOR },
        { MAX_PTANKS_COLOR_MAT_UV2, rpPTANKDFLAGVTX2TEXCOORDS | rpPTANKDFLAGMATRIX | rpPTANKDFLAGCOLOR },
        { MAX_PTANKS_POS_COLOR_SIZE, rpPTANKDFLAGCNSVTX2TEXCOORDS | rpPTANKDFLAGSIZE | rpPTANKDFLAGCOLOR | rpPTANKDFLAGPOSITION },
        { MAX_PTANKS_POS_COLOR_SIZE_UV2, rpPTANKDFLAGVTX2TEXCOORDS | rpPTANKDFLAGSIZE | rpPTANKDFLAGCOLOR | rpPTANKDFLAGPOSITION },
        { MAX_PTANKS_POS_COLOR_SIZE_ROT, rpPTANKDFLAGCNSVTX2TEXCOORDS | rpPTANKDFLAG2DROTATE | rpPTANKDFLAGSIZE | rpPTANKDFLAGCOLOR | rpPTANKDFLAGPOSITION },
        { MAX_PTANKS_POS_COLOR_SIZE_ROT_UV2, rpPTANKDFLAGVTX2TEXCOORDS | rpPTANKDFLAG2DROTATE | rpPTANKDFLAGSIZE | rpPTANKDFLAGCOLOR | rpPTANKDFLAGPOSITION },
    };

    void sort_buckets(group_data& group)
    {
        memset(group.buckets, 0, group.buckets_used * sizeof(ptank_context*));
        group.buckets_used = 2;
        for (ptank_context* it = group.ptanks, *endp = it + group.size; it != endp; ++it) {
            const RwTexture* texture = RpPTankAtomicGetTexture(it->ptank);
            ptank_context** bucket = group.buckets;
            if (!texture) {
                if (it->flags & ptank_pool::FLAG_NOZTEST) ++bucket;
            } else {
                bucket += 2;
                ptank_context** end = bucket + group.buckets_used - 2;
                for (; bucket < end; ++bucket) {
                    ptank_context* front = *bucket;
                    const RwTexture* btexture = RpPTankAtomicGetTexture(front->ptank);
                    if (texture == btexture &&
                        it->dst_blend == front->dst_blend &&
                        it->src_blend == front->src_blend &&
                        !((it->flags ^ front->flags) & ptank_pool::FLAG_NOZTEST))
                        break;
                }
                if (bucket == end) group.buckets_used++;
            }
            it->next = *bucket;
            *bucket = it;
        }
    }

    void adjust_block_sizes()
    {
        switch (xglobals->sceneCur->sceneID) {
        case IDTAG('N','J','0','2'):
            MAX_PTANKS_COLOR_MAT = 4;
            MAX_PTANKS_COLOR_MAT_UV2 = 8;
            MAX_PTANKS_POS_COLOR_SIZE = 20;
            MAX_PTANKS_POS_COLOR_SIZE_UV2 = 16;
            MAX_PTANKS_POS_COLOR_SIZE_ROT = 12;
            MAX_PTANKS_POS_COLOR_SIZE_ROT_UV2 = 8;
            break;
        default:
            MAX_PTANKS_COLOR_MAT = 4;
            MAX_PTANKS_COLOR_MAT_UV2 = 8;
            MAX_PTANKS_POS_COLOR_SIZE = 12;
            MAX_PTANKS_POS_COLOR_SIZE_UV2 = 16;
            MAX_PTANKS_POS_COLOR_SIZE_ROT = 12;
            MAX_PTANKS_POS_COLOR_SIZE_ROT_UV2 = 8;
            break;
        }

        MAX_PTANKS = MAX_PTANKS_COLOR_MAT + MAX_PTANKS_COLOR_MAT_UV2 + MAX_PTANKS_POS_COLOR_SIZE +
                     MAX_PTANKS_POS_COLOR_SIZE_UV2 + MAX_PTANKS_POS_COLOR_SIZE_ROT +
                     MAX_PTANKS_POS_COLOR_SIZE_ROT_UV2;
    }

    void init_groups()
    {
        adjust_block_sizes();

        groups[PGT_COLOR_MAT].max_size = MAX_PTANKS_COLOR_MAT;
        groups[PGT_COLOR_MAT_UV2].max_size = MAX_PTANKS_COLOR_MAT_UV2;
        groups[PGT_POS_COLOR_SIZE].max_size = MAX_PTANKS_POS_COLOR_SIZE;
        groups[PGT_POS_COLOR_SIZE_UV2].max_size = MAX_PTANKS_POS_COLOR_SIZE_UV2;
        groups[PGT_POS_COLOR_SIZE_ROT].max_size = MAX_PTANKS_POS_COLOR_SIZE_ROT;
        groups[PGT_POS_COLOR_SIZE_ROT_UV2].max_size = MAX_PTANKS_POS_COLOR_SIZE_ROT_UV2;

        S32 total = 0;
        for (group_data* it = groups, *end = it + MAX_PGT; it != end; ++it) {
            total += it->max_size;
        }

        size_t ptanks_size = total * sizeof(ptank_context);
        size_t buckets_size = (total + 2 * MAX_PGT) * sizeof(ptank_context*);
        U8* mem = (U8*)xMEMALLOC(ptanks_size + buckets_size, 0);

        for (group_data* it = groups, *end = it + MAX_PGT; it != end; ++it) {
            it->size = it->used = it->buckets_used = 0;
            it->ptanks = (ptank_context*)mem;
            mem += it->max_size * sizeof(ptank_context);
            it->buckets = (ptank_context**)mem;
            it->buckets_used = it->max_size + 2;
            mem += it->buckets_used * sizeof(ptank_context*);
            sort_buckets(*it);
        }
    }

    RpAtomic* create_ptank(U32 flags)
    {
        flags |= rpPTANKDFLAGARRAY;
        iAsyncLockRWSentry rw_sentry;
        U32 plat_flags = 0;

        RpAtomic* ptank = RpPTankAtomicCreate(BLOCK_SIZE, flags, plat_flags);
        if (!ptank) {
            return NULL;
        }

        RwFrame* frame = RwFrameCreate();
        if (!frame) {
            RpPTankAtomicDestroy(ptank);
            return NULL;
        }

        RwMatrixSetIdentity(RwFrameGetMatrix(frame));
        RpAtomicSetFrame(ptank, frame);
        RpPTankAtomicSetVertexAlpha(ptank, TRUE);

        if (flags & rpPTANKDFLAGCNSVTX2TEXCOORDS) {
            RwTexCoords uv[2] = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
            RpPTankAtomicSetConstantVtx2TexCoords(ptank, uv);
        }

        return ptank;
    }

    void destroy_ptank(RpAtomic* ptank)
    {
        if (ptank) {
            RwFrame* frame = RpAtomicGetFrame(ptank);
            RpAtomicSetFrame(ptank, NULL);
            if (frame) {
                RwFrameDestroy(frame);
            }
            RpPTankAtomicDestroy(ptank);
        }
    }

    void render_ptank(const ptank_context& context, S32& render_flags)
    {
        S32 xor_flags = render_flags ^ context.flags;

        if (xor_flags & 0x1) {
            S32 noztest = context.flags & ptank_pool::FLAG_NOZTEST;
            render_flags = (render_flags & ~0x1) | noztest;
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)!noztest);
        }

        if (xor_flags & 0x2) {
            S32 zwrite = context.flags & ptank_pool::FLAG_ZWRITE;
            render_flags = (render_flags & ~0x2) | zwrite;
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);
        }

        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)!(context.flags & ptank_pool::FLAG_NOZTEST));

        U32 oldtest = xFXHighDynamicRangeBrightenPtank(context.ptank, (context.flags & ptank_pool::FLAG_BRIGHT) != 0, -1);

        RpAtomicRender(context.ptank);

        xFXHighDynamicRangeRestorePtank(context.ptank, oldtest);
    }

    inline S32 create_ptanks(group_data& group, S32 want)
    {
        S32 old_group_size = group.size;
        if (group.size + want > group.max_size) want = group.max_size - group.size;
        ptank_context* head = group.ptanks + group.size, *end = head + want;
        for (ptank_context* it = head; it < end; ++it, ++group.size) {
            it->ptank = create_ptank(group.create_flags);
            if (!it->ptank) {
                break;
            }

            it->flags = ptank_pool::FLAG_NOZTEST;
            it->next = *group.buckets;
            *group.buckets = it;
        }
        return group.size - old_group_size;
    }

    S32 compare_ptanks(const void* e1, const void* e2)
    {
        const ptank_context* p1 = (const ptank_context*)e1;
        const ptank_context* p2 = (const ptank_context*)e2;

        if (p1->order_group != p2->order_group)
            return p1->order_group - p2->order_group;
        if (p1->order_index != p2->order_index) {
            return p1->order_index - p2->order_index;
        }

        if (p1->dst_blend != p2->dst_blend)
            return p2->dst_blend - p1->dst_blend;
        if (p1->src_blend != p2->src_blend) {
            return p1->src_blend - p2->src_blend;
        }

        const RwTexture* tex1 = RpPTankAtomicGetTexture(p1->ptank);
        const RwTexture* tex2 = RpPTankAtomicGetTexture(p2->ptank);
        if (tex1 != tex2) {
            return (S32)tex1 - (S32)tex2;
        }

        return p1->sequence_index - p2->sequence_index;
    }
} // namespace

void xPTankPoolSceneEnter()
{
    inited = true;
    current_sequence_index = 0;
    init_groups();

    for (group_data* it = groups, *end = it + MAX_PGT; it != end; ++it) {
        S32 want = 0.5f + it->max_size;
        create_ptanks(*it, want);
    }
}

void xPTankPoolSceneExit()
{
    inited = false;
    for (group_data* g = groups, *endg = g + MAX_PGT; g != endg; ++g)
        for (ptank_context* p = g->ptanks, *endp = p + g->size; p != endp; ++p)
            destroy_ptank(p->ptank);
}

void xPTankPoolRender()
{
    zRenderState(SDRS_XPTankPool);
    xFXHighDynamicRangeBrighten(true, false);

    ptank_context* ptanks = (ptank_context*)xMEMPUSHTEMP(MAX_PTANKS * sizeof(ptank_context));

    ptank_context* ptank = ptanks;
    for (group_data* g = groups, *endg = g + MAX_PGT; g != endg; ++g) {
        if (g->used > 0) {
            memcpy(ptank, g->ptanks, g->size * sizeof(ptank_context));
            ptank += g->size;

            for (ptank_context* p = g->ptanks, *endp = p + g->size; p != endp; ++p) {
                if (p->flags & 0x10000) {
                    S32 used = RpPTankAtomicGetActiveParticlesCount(p->ptank);
                    p->flags &= ~0x10000;
                }
                else if (!(p->flags & 0x20000)) {
                    RpPTankAtomicSetTexture(p->ptank, NULL);
                    p->flags |= 0x20000;
                }
            }

            g->used = 0;
            sort_buckets(*g);
        }
    }

    S32 render_flags = 0;
    ptank_context* end_ptank = ptank;
    qsort(ptanks, end_ptank - ptanks, sizeof(ptank_context), compare_ptanks);
    for (ptank = ptanks; ptank != end_ptank; ++ptank) {
        if (ptank->flags & 0x10000)
            render_ptank(*ptank, render_flags);
    }

    if (render_flags & 0x1) {
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    }

    if (render_flags & 0x2) {
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    }

    xMEMPOPTEMP(ptanks);

    current_sequence_index = 0;

    xFXHighDynamicRangeBrighten(false, false);
}

void ptank_pool::grab_block(ptank_group_type type)
{
    ptank = NULL;
    group_data& group = groups[type];

    if (group.used >= group.size && create_ptanks(group, 1) < 1) {
        return;
    }

    ptank_context** end = group.buckets + group.buckets_used;

    for (ptank_context** it = group.buckets + 2; it < end; ++it) {
        if (*it) {
            ptank_context*& context = *it;
            const RwTexture* texture = RpPTankAtomicGetTexture(context->ptank);
            if (texture == rs.texture &&
                context->dst_blend == rs.dst_blend &&
                context->src_blend == rs.src_blend &&
                !((context->flags ^ rs.flags) & FLAG_NOZTEST)) {
                if (rs.flags & FLAG_ORDERED) {
                    context->order_group = order_group;
                    context->order_index = order_index;
                } else {
                    context->order_group = 0;
                    context->order_index = 0;
                }

                context->sequence_index = current_sequence_index;
                current_sequence_index++;

                ptank = context->ptank;
                context->flags = (context->flags & 0xFFFF0000) | (rs.flags & 0xFFFF) | 0x10000;
                context = context->next;
                group.used++;
                return;
            }
        }
    }

    for (ptank_context** it = group.buckets; it < end; ++it) {
        if (*it) {
            ptank_context*& context = *it;
            if ((rs.flags & FLAG_NOZTEST) && !(context->flags & FLAG_NOZTEST)) {
                destroy_ptank(context->ptank);
                context->ptank = create_ptank(group.create_flags);
            }

            ptank = context->ptank;
            RpPTankAtomicSetTexture(ptank, rs.texture);
            RpPTankAtomicSetBlendModes(ptank, (RwBlendFunction)rs.src_blend, (RwBlendFunction)rs.dst_blend);

            context->src_blend = rs.src_blend;
            context->dst_blend = rs.dst_blend;

            if (rs.flags & FLAG_ORDERED) {
                context->order_group = order_group;
                context->order_index = order_index;
            } else {
                context->order_group = 0;
                context->order_index = 0;
            }

            context->sequence_index = current_sequence_index;
            current_sequence_index++;

            context->flags = rs.flags | 0x10000;
            context = context->next;
            group.used++;
            return;
        }
    }
}

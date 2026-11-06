#include "xParticleBatch.h"

#include "xGlobals.h"
#include "xMemMgr.h"
#include "xMemory.h"
#include "xutil.h"
#include "zMain.h"

#include <stdlib.h>
#include <string.h>

namespace {

    enum { MAX_SYSTEMS = 128, MAX_SYSTEMS_PER_GROUP = 16 };
#define MAX_BLOCKS 32

    S32 BATCH_BLOCK_SIZE = 0x500;
    S32 BUFFER_SIZE = MAX_BLOCKS * BATCH_BLOCK_SIZE;

    namespace tweak {

        void add_tweaks() {}

    }

    struct block_info
    {
        U8* mem;
        S32 used;
        block_info* next;
    };

    struct internal_system : xParticleBatchSystem
    {
        block_info* block_list;
        S32 blocks_used;
        S32 units_used;
        S32 units_per_block;
        block_info* best_block;
        internal_system* next;
    };

    struct system_bucket
    {
        RwTexture* texture;
        union
        {
            S32 rskey;
            struct
            {
                U8 src_blend;
                U8 dst_blend;
                U16 flags;
            };
        };
        U32 order_group;
        S32 order_index;
        internal_system* systems;
    };

    internal_system systems[MAX_SYSTEMS];
    system_bucket buckets[MAX_PGT][MAX_SYSTEMS];
    block_info* head_free_block;
    U8* block_buffer;
    block_info block_info_buffer[MAX_BLOCKS];
    S32 systems_size;
    S32 buckets_size[MAX_PGT];

    block_info* alloc_block()
    {
        block_info* b = head_free_block;
        head_free_block = head_free_block->next;
        return b;
    }

    void free_block(block_info* b)
    {
        b->next = head_free_block;
        head_free_block = b;
    }

    void create_system(internal_system& isys, const xParticleBatchSystem& sys)
    {
        (xParticleBatchSystem&)isys = sys;
        isys.block_list = 0;
        isys.blocks_used = 0;
        isys.units_used = 0;
        isys.units_per_block = BATCH_BLOCK_SIZE / isys.stride;
        isys.best_block = 0;

        if (!(isys.rs.flags & ptank_pool::FLAG_ORDERED)) {
            isys.order_group = 0;
            isys.order_index = 0;
        }
    }

    struct compare_buckets
    {
        S32 operator()(const void* e1, const void* e2) const
        {
            const system_bucket& b1 = *(const system_bucket*)e1;
            const system_bucket& b2 = *(const system_bucket*)e2;
            return (*this)(*(const system_bucket*)e1, *(const system_bucket*)e2);
        }

        S32 operator()(const system_bucket& b1, const system_bucket& b2) const
        {
            if (b1.texture != b2.texture) {
                return b1.texture - b2.texture;
            }
            S32 ordered1 = b1.flags & ptank_pool::FLAG_ORDERED;
            S32 ordered2 = b1.flags & ptank_pool::FLAG_ORDERED;
            if (ordered1 != ordered2) {
                return ordered1 - ordered2;
            }
            if (b1.order_group != b2.order_group) {
                return b1.order_group - b2.order_group;
            }
            if (b1.order_index != b2.order_index) {
                return b1.order_index - b2.order_index;
            }
            return b1.rskey - b2.rskey;
        }
    };

    void add_to_bucket(internal_system& isys)
    {
        S32& bsize = buckets_size[isys.pool_type];
        system_bucket* const begin_bucket = buckets[isys.pool_type];
        system_bucket* const end_bucket = begin_bucket + bsize;

        system_bucket key;
        key.texture = isys.rs.texture;
        key.src_blend = isys.rs.src_blend;
        key.dst_blend = isys.rs.dst_blend;
        key.flags = isys.rs.flags;
        key.order_group = isys.order_group;
        key.order_index = isys.order_index;

        system_bucket* bucket = binary_find(begin_bucket, end_bucket, key, compare_buckets());

        if (bucket == end_bucket) {
            *bucket = key;
            bucket->systems = 0;
            bsize++;
        }

        isys.next = bucket->systems;
        bucket->systems = &isys;

        if (bucket == end_bucket)
            quicksort(begin_bucket, bsize, sizeof(system_bucket), compare_buckets());
    }

    void remove_from_bucket(internal_system& isys)
    {
        S32& bsize = buckets_size[isys.pool_type];
        system_bucket* const begin_bucket = buckets[isys.pool_type];
        system_bucket* const end_bucket = begin_bucket + bsize;

        system_bucket key;
        key.texture = isys.rs.texture;
        key.src_blend = isys.rs.src_blend;
        key.dst_blend = isys.rs.dst_blend;
        key.flags = isys.rs.flags;
        key.order_group = isys.order_group;
        key.order_index = isys.order_index;

        system_bucket* bucket = binary_find(begin_bucket, end_bucket, key, compare_buckets());

        internal_system *it = bucket->systems, **itp = &bucket->systems;
        for (; it != &isys; itp = &it->next, it = it->next) {
        }

        *itp = it->next;

        if (bucket->systems == 0) {
            bsize--;
            xMemoryCopyUpA32(bucket, bucket + 1, (U8*)end_bucket - (U8*)(bucket + 1));
        }
    }

    void clear_system(internal_system& system)
    {
        block_info* block = system.block_list;
        for (; block; ) {
            block_info* next_block = block->next;
            free_block(block);
            block = next_block;
        }
        system.block_list = 0;
        system.blocks_used = 0;
        system.units_used = 0;
        system.best_block = 0;
    }

    inline block_info* grab_best_block(internal_system& isys)
    {
        if (isys.best_block) {
            return isys.best_block;
        }

        S32 most_used = -1; block_info* b = isys.block_list;
        for (; b; b = b->next) {
            if (b->used < isys.units_per_block && most_used < b->used) {
                isys.best_block = b;
                most_used = b->used;
            }
        }
        if (isys.best_block) {
            return isys.best_block;
        }

        if (!head_free_block)
            return 0;
        isys.best_block = alloc_block();
        isys.best_block->used = 0;
        isys.best_block->next = isys.block_list;
        isys.block_list = isys.best_block;

        isys.blocks_used++;
        return isys.best_block;
    }

    void merge_blocks(internal_system& system)
    {
        block_info *first = system.block_list, *second = first->next;
        block_info *s0, *s1, **ps0;
        if (first->used < second->used) {
            s0 = first; s1 = second; ps0 = &system.block_list;
        } else {
            s0 = second; s1 = first; ps0 = &first->next;
        }

        block_info *b = second->next, **pb = &second->next;
        for (; b; pb = &b->next, b = b->next) {
            if (b->used < s0->used) {
                s1 = s0; s0 = b; ps0 = pb;
            } else if (b->used < s1->used)
                s1 = b;
        }

        S32 total = s0->used + s1->used;
        if (total <= system.units_per_block) {
            U8* tail = s1->mem + s1->used * system.stride;
            memcpy(tail, s0->mem, s0->used * system.stride);
            s1->used += s0->used;
            *ps0 = s0->next;
            free_block(s0);
            system.blocks_used--;
        }
    }

    void refresh_blocks(internal_system& system)
    {
        system.best_block = 0;

        block_info *b = system.block_list, **pb = &system.block_list;
        for (; b; b = *pb) {
            if (b->used <= 0) {
                *pb = b->next;
                free_block(b);
                system.blocks_used--;
            } else {
                pb = &b->next;
            }
        }

        if (system.blocks_used > 1)
            merge_blocks(system);
    }

    template <class T>
    void pre_render_buckets(system_bucket* first, system_bucket* last, F32 dt)
    {
        T pool;
        for (system_bucket* bucket = first; bucket != last; bucket++) {
            internal_system* system = bucket->systems;
            bool started = false;
            for (; system; system = system->next) {
                if (system->units_used <= 0) {
                    continue;
                }

                if (!started) {
                    pool.rs = system->rs;
                    pool.order_group = system->order_group;
                    pool.order_index = system->order_index;
                    pool.reset();
                    started = true;
                }

                bool lost_any = false;
                block_info* block = system->block_list;
                for (; block; block = block->next) {
                    S32 newused = system->update(block->mem, block->used, pool, dt, system->context);
                    if (newused < block->used)
                        lost_any = true;
                    system->units_used += newused - block->used;
                    block->used = newused;
                }
                if (lost_any)
                    refresh_blocks(*system);
            }
            if (started)
                pool.flush();
        }
    }

    inline void validate_system(const internal_system&) {}

    void adjust_batch_size()
    {
        switch (globals.sceneCur->sceneID) {
        case 'FT01':
        case 'HS01':
        case 'LD01':
        case 'NI03':
        case 'NJ02':
        case 'NJ03':
        case 'OM01':
        case 'OM03':
        case 'OM04':
        case 'RS02':
            BATCH_BLOCK_SIZE = 0x1400;
            break;
        default:
            BATCH_BLOCK_SIZE = 0x500;
            break;
        }

        BUFFER_SIZE = MAX_BLOCKS * BATCH_BLOCK_SIZE;
    }

}

void xParticleBatchSceneEnter()
{
    adjust_batch_size();

    block_buffer = (U8*)xMEMALLOC(BUFFER_SIZE, 0);

    systems_size = 0;

    memset(buckets_size, 0, sizeof(buckets_size));

    xParticleBatchReset();

    tweak::add_tweaks();
}

void xParticleBatchSceneExit()
{
}

void xParticleBatchReset()
{
    internal_system *sys = systems, *end_sys = sys + systems_size;
    for (; sys != end_sys; sys++) {
        sys->block_list = 0;
        sys->blocks_used = 0;
        sys->units_used = 0;
        sys->best_block = 0;
    }

    head_free_block = block_info_buffer;
    block_info *b = block_info_buffer, *endb = b + MAX_BLOCKS - 1;
    U8* mem = block_buffer;
    for (; b != endb; b++, mem += BATCH_BLOCK_SIZE) {
        b->mem = mem;
        b->next = b + 1;
    }
    endb->mem = mem;
    endb->next = 0;
}

void xParticleBatchPreRender()
{
    F32 dt = xglobals->update_dt;

    pre_render_buckets<ptank_pool__color_mat>(buckets[PGT_COLOR_MAT], buckets[PGT_COLOR_MAT] + buckets_size[PGT_COLOR_MAT], dt);
    pre_render_buckets<ptank_pool__color_mat_uv2>(buckets[PGT_COLOR_MAT_UV2], buckets[PGT_COLOR_MAT_UV2] + buckets_size[PGT_COLOR_MAT_UV2], dt);
    pre_render_buckets<ptank_pool__pos_color_size>(buckets[PGT_POS_COLOR_SIZE], buckets[PGT_POS_COLOR_SIZE] + buckets_size[PGT_POS_COLOR_SIZE], dt);
    pre_render_buckets<ptank_pool__pos_color_size_uv2>(buckets[PGT_POS_COLOR_SIZE_UV2], buckets[PGT_POS_COLOR_SIZE_UV2] + buckets_size[PGT_POS_COLOR_SIZE_UV2], dt);
    pre_render_buckets<ptank_pool__pos_color_size_rot>(buckets[PGT_POS_COLOR_SIZE_ROT], buckets[PGT_POS_COLOR_SIZE_ROT] + buckets_size[PGT_POS_COLOR_SIZE_ROT], dt);
    pre_render_buckets<ptank_pool__pos_color_size_rot_uv2>(buckets[PGT_POS_COLOR_SIZE_ROT_UV2], buckets[PGT_POS_COLOR_SIZE_ROT_UV2] + buckets_size[PGT_POS_COLOR_SIZE_ROT_UV2], dt);
}

S32 xParticleBatchAddSystem(const xParticleBatchSystem& system)
{
    S32 system_id = systems_size;
    systems_size++;
    internal_system& isys = systems[system_id];
    create_system(isys, system);
    add_to_bucket(isys);
    validate_system(isys);
    return system_id;
}

void xParticleBatchReplaceSystem(S32 system_id, const xParticleBatchSystem& system)
{
    internal_system& isys = systems[system_id];
    if (system.stride != isys.stride) {
        clear_system(isys);
        isys.units_per_block = BATCH_BLOCK_SIZE / isys.stride;
    }

    remove_from_bucket(isys);
    (xParticleBatchSystem&)isys = system;
    add_to_bucket(isys);
    validate_system(isys);
}

S32 xParticleBatchEmit(S32 system_id, S32 amount, U8** mem)
{
    internal_system& isys = systems[system_id];
    block_info* block = grab_best_block(isys);
    if (!block) {
        return 0;
    }

    S32 remain = isys.units_per_block - block->used;
    *mem = block->mem + block->used * isys.stride;
    if (amount >= remain) {
        isys.best_block = 0; amount = remain;
    }

    block->used += amount;
    isys.units_used += amount;
    return amount;
}

void xParticleBatchGroup::create()
{
    elements = new (eMemStaticType_0, 0) U8[MAX_SYSTEMS_PER_GROUP];
    elements_size = 0;
}

namespace {

    inline S32 compare_systems(const xParticleBatchSystem& sys1, const xParticleBatchSystem& sys2)
    {
        if (sys1.pool_type != sys2.pool_type)
            return sys1.pool_type - sys2.pool_type;
        if (sys1.rs.texture != sys2.rs.texture)
            return (S32)sys1.rs.texture - (S32)sys2.rs.texture;
        if (sys1.rs.src_blend != sys2.rs.src_blend)
            return sys1.rs.src_blend - sys2.rs.src_blend;
        if (sys1.rs.dst_blend != sys2.rs.dst_blend)
            return sys1.rs.dst_blend - sys2.rs.dst_blend;
        if (sys1.rs.flags != sys2.rs.flags)
            return sys1.rs.flags - sys2.rs.flags;
        if (sys1.rs.flags & ptank_pool::FLAG_ORDERED) {
            if (sys1.order_group != sys2.order_group)
                return sys1.order_group - sys2.order_group;
            if (sys1.order_index != sys2.order_index)
                return sys1.order_index - sys2.order_index;
        }
        if (sys1.stride != sys2.stride)
            return sys1.stride - sys2.stride;
        if (sys1.update != sys2.update)
            return (S32)sys1.update - (S32)sys2.update;
        return (S32)sys1.context - (S32)sys2.context;
    }

    struct compare_group_systems_functor
    {
        S32 operator()(const xParticleBatchSystem& system, const U8& system_id) const
        {
            const xParticleBatchSystem& isys = systems[system_id];
            return compare_systems(system, isys);
        }
    };

    S32 compare_group_systems_function(const void* e1, const void* e2)
    {
        S32 sysid1 = *(const U8*)e1, sysid2 = *(const U8*)e2;
        const xParticleBatchSystem& isys1 = systems[sysid1];
        const xParticleBatchSystem& isys2 = systems[sysid2];
        return compare_systems(systems[sysid1], systems[sysid2]);
    }

}

S32 xParticleBatchGroup::create_system(const xParticleBatchSystem& system)
{
    const U8* end_elements = elements + elements_size;
    const U8* found = binary_find((const U8*)elements, end_elements, system, compare_group_systems_functor());

    if (found != end_elements)
        return *found;

    S32 system_id = xParticleBatchAddSystem(system);
    elements[elements_size] = system_id;
    elements_size++;

    qsort(elements, elements_size, 1, compare_group_systems_function);

    return system_id;
}

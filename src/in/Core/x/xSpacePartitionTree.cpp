#include "xCommon.h"
#include "xSpacePartitionTree.h"

#include "xMemMgr.h"

namespace {

namespace tweak {

void add_tweaks()
{
}

} // namespace tweak

} // namespace

void xSpacePartitionTree2D::create(F32 minx, F32 miny, F32 maxx, F32 maxy, F32 min_width, F32 max_width, S32 max_entries)
{
    origin.assign(minx, miny);
    size.assign(maxx - minx, maxy - miny);

    grid_shift_x = get_grid_shift(size.x, max_width);
    grid_size_x = 1 << grid_shift_x;
    grid_mask_x = grid_size_x - 1;
    grid_shift_y = get_grid_shift(size.y, max_width);
    grid_size_y = 1 << grid_shift_y;
    grid_mask_y = grid_size_y - 1;

    xVec2 gsize = { size.x / grid_size_x, size.y / grid_size_y };
    cell_size[0] = MAX(gsize.x, gsize.y);
    cell_isize[0] = 1.0f / cell_size[0];
    cell_frac[0] = 1.0f;
    size.assign(cell_size[0] * grid_size_x, cell_size[0] * grid_size_y);

    ilogdepth = -1.0f / xlog(2.0f);
    if (min_width > cell_size[0])
        max_depth = 0;
    else
        max_depth = (S32)(ilogdepth * xlog(min_width / cell_size[0]));
    if (max_depth > MAX_DEPTH) max_depth = MAX_DEPTH;

    F32* cs = cell_size + 1, *end_cs = cs + max_depth;
    F32* ics = cell_isize + 1;
    F32* cf = cell_frac + 1;
    for (; cs != end_cs; ++cs, ++ics, ++cf) {
        *cs = 0.5f * cs[-1];
        *ics = 2.0f * ics[-1];
        *cf = 0.5f * cf[-1];
    }

    node_buffer_size = max_entries * 4;
    node_buffer = new (eMemStaticType_0, 0) entry_node[node_buffer_size];

    entry_buffer_size = max_entries;
    entry_buffer = new (eMemStaticType_0, 0) entry_data[entry_buffer_size];

    cell_buffer_size = (S32)(4.0f * max_entries);
    cell_buffer_size = (cell_buffer_size + 3) / 4 * 4 + grid_size_x * grid_size_y;
    cell_buffer = new (eMemStaticType_0, 0) cell[cell_buffer_size];
    grid = cell_buffer;

    clear();

    tweak::add_tweaks();
}

void xSpacePartitionTree2D::clear()
{
    cell* gc = grid, *end_gc = gc + grid_size_x * grid_size_y;
    for (; gc != end_gc; ++gc)
        create_cell(*gc, INVALID_INDEX, 0);

    free_entries = entry_buffer;
    entry_data* fe = entry_buffer, *end_fe = fe + entry_buffer_size - 1;
    for (; fe != end_fe; ++fe) fe->next_free_entry = fe + 1;
    end_fe->next_free_entry = NULL;

    free_nodes = node_buffer;
    entry_node* fn = node_buffer, *end_fn = fn + node_buffer_size - 1;
    for (; fn != end_fn; ++fn) fn->next_free_node = fn + 1;
    end_fn->next_free_node = NULL;

    S32 grid_size = grid_size_x * grid_size_y;
    free_cells = cell_buffer + grid_size;
    cell* fc = free_cells, *end_fc = fc + cell_buffer_size - grid_size - 4;
    for (; fc != end_fc; fc = fc->next_free_cell)
        fc->next_free_cell = fc + 4;
    end_fc->next_free_cell = NULL;

    free_cells_size = cell_buffer_size - grid_size;
    safety_free_cells_size = (max_depth + 1) * 4;

    current_searchid = 0;
    reset_searchid();
}

void xSpacePartitionTree2D::create_cell(cell& c, U16 parent, S32 cid)
{
    c.parent = parent;
    c.children = INVALID_INDEX;
    c.entries = INVALID_INDEX;
    c.cid = cid;

    if (parent == INVALID_INDEX) {
        c.level = max_depth;
        c.xcode = c.ycode = 0;
    } else {
        cell* const pc = cell_buffer + parent;
        c.level = pc->level - 1;

        S32 xbit = cid & 1, ybit = (cid >> 1) & 1;
        c.xcode = (U8)(pc->xcode | (xbit << c.level));
        c.ycode = (U8)(pc->ycode | (ybit << c.level));
    }
}

void xSpacePartitionTree2D::create_children(cell& c)
{
    cell* const cell_list = cell_buffer;
    cell* cc = alloc_children(), *end_cc = cc + 4;
    c.children = cc - cell_list;
    U16 parent = &c - cell_list;
    for (; cc != end_cc; ++cc)
        create_cell(*cc, parent, cc - cell_list - c.children);
}

void xSpacePartitionTree2D::reset_searchid()
{
    current_searchid = 1;
    entry_data* e = entry_buffer, *end = e + entry_buffer_size;
    for (; e != end; ++e)
        e->searchid = 0;
}

namespace {

struct cb_fp_wrapper
{
    cb_fp_wrapper(xSpacePartitionTree2D::intersect_callback cb, void* context) : cb(cb), context(context) {}

    bool operator()(xSpacePartitionTree2D::entry_data* e) const
    {
        return cb(e, context);
    }

    xSpacePartitionTree2D::intersect_callback cb;
    void* context;
};

} // namespace

// UNUSED: needed to match (speculative code)
void xSpacePartitionTree2D::intersect_point(F32 x, F32 y, intersect_callback cb, void* context)
{
    cb_fp_wrapper fp_wrapper(cb, context);
    intersect_point(x, y, fp_wrapper);
}

void xSpacePartitionTree2D::intersect_rect(F32 x0, F32 y0, F32 x1, F32 y1, intersect_callback cb, void* context)
{
    cb_fp_wrapper fp_wrapper(cb, context);
    intersect_rect(x0, y0, x1, y1, fp_wrapper);
}

void xSpacePartitionTree2D::insert_internal(entry_data* e, F32 x0, F32 y0, F32 x1, F32 y1, F32 width)
{
    F32 scale = cell_isize[0];
    x0 = scale * (x0 - origin.x);
    y0 = scale * (y0 - origin.y);
    x1 = scale * (x1 - origin.x);
    y1 = scale * (y1 - origin.y);
    width = scale * width;

    S32 gx0 = CLAMP((S32)x0, 0, grid_size_x - 1);
    S32 gy0 = CLAMP((S32)y0, 0, grid_size_y - 1);
    S32 gx1 = CLAMP((S32)x1, 0, grid_size_x - 1);
    S32 gy1 = CLAMP((S32)y1, 0, grid_size_y - 1);

    S32 end_depth;
    if (width < 0.0001f) end_depth = max_depth;
    else {
        end_depth = (S32)(ilogdepth * xlog(width));
        if (end_depth > max_depth) end_depth = max_depth;
    }

    e->owner_size = 0;
    for (S32 gy = gy0; gy <= gy1; ++gy)
        for (S32 gx = gx0; gx <= gx1; ++gx)
            insert_internal(e, &grid_cell(gx, gy), end_depth, x0 - gx, y0 - gy, x1 - gx, y1 - gy);

    e->searchid = current_searchid;
}

void xSpacePartitionTree2D::insert_internal(entry_data* e, cell* c, S32 end_depth, F32 x0, F32 y0, F32 x1, F32 y1)
{
    S32 index = 0;
    F32 dxy = cell_frac[1];
    F32 cx0 = 0.0f;
    F32 cy0 = 0.0f;
    F32 cx1 = 1.0f;
    F32 cy1 = 1.0f;
    S32 depth = 0;
    struct { F32 cx0, cy0, cx1, cy1; } bound_stack[MAX_DEPTH];

    cell* const cell_list = cell_buffer;
    entry_node* const node_list = node_buffer;
    entry_data* const entry_list = entry_buffer;

    for (;;) {
descend:
        if (depth == end_depth) {
            e->owner[e->owner_size] = c - cell_list;
            entry_node* n = alloc_node();
            n->entry = e - entry_list;
            n->next = c->entries;
            c->entries = n - node_list;
            ++e->owner_size;
        } else {
            for (; index < 4; ++index) {
                S32 ix = index & 1, iy = index / 2;
                F32 ccx0 = cx0 + dxy * ix, ccy0 = cy0 + dxy * iy, ccx1 = ccx0 + dxy, ccy1 = ccy0 + dxy;

                if (ccx0 > x1 || ccy0 > y1 || ccx1 < x0 || ccy1 < y0)
                    continue;

                if (c->children == INVALID_INDEX)
                    create_children(*c);

                cell* cc = cell_list + c->children + index;

                bound_stack[depth].cx0 = cx0; bound_stack[depth].cy0 = cy0;
                bound_stack[depth].cx1 = cx1; bound_stack[depth].cy1 = cy1;

                c = cc;
                index = 0;
                ++depth; dxy = cell_frac[depth + 1];
                cx0 = ccx0; cy0 = ccy0;
                cx1 = ccx1; cy1 = ccy1;
                goto descend;
            }
        }

        if (depth <= 0)
            return;

        --depth;
        index = c->cid + 1;
        dxy = cell_frac[depth + 1];
        cx0 = bound_stack[depth].cx0; cy0 = bound_stack[depth].cy0; cx1 = bound_stack[depth].cx1; cy1 = bound_stack[depth].cy1;
        c = cell_list + c->parent;
    }
}

void xSpacePartitionTree2D::erase_internal(entry_data* e)
{
    cell* const cell_list = cell_buffer;
    entry_node* const node_list = node_buffer;
    entry_data* const entry_list = entry_buffer;

    U16* owner = e->owner, *end_owner = owner + e->owner_size;
    for (; owner != end_owner; ++owner) {
        cell* c = cell_list + *owner;

        entry_node* prev = NULL, *n = node_list + c->entries;
        U16 key = e - entry_list;
        for (;;) {
            if (n->entry == key) break;
            prev = n;
            n = node_list + n->next;
        }

        if (prev == NULL) c->entries = n->next;
        else prev->next = n->next;
        free_node(n);

        remove_empty(c);
    }
}

void xSpacePartitionTree2D::remove_empty(cell* c)
{
    cell* const cell_list = cell_buffer;

    U16 parent = c->parent;
    while (parent != INVALID_INDEX) {
        cell* pc = cell_list + parent;

        parent = pc->parent;
        cell* bro = cell_list + pc->children, *end_bro = bro + 4;
        for (; bro != end_bro; ++bro) {
            if (bro->entries != INVALID_INDEX || bro->children != INVALID_INDEX)
                return;
        }
        free_children(cell_list + pc->children);
        pc->children = INVALID_INDEX;
    }
}

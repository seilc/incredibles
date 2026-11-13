#ifndef XSPACEPARTITIONTREE_H
#define XSPACEPARTITIONTREE_H

#include "xMath2.h"

// UNUSED: needed to match (speculative code)
static bool line_vs_square(F32 tx0, F32 ty0, F32 tx1, F32 ty1)
{
    F32 tmin, tmax;
    if (tx0 > ty0) tmin = tx0; else tmin = ty0;
    if (tx1 < ty1) tmax = tx1; else tmax = ty1;
    return tmin <= tmax && tmax >= 0.0f && tmin <= 1.0f;
}

#define SPT_VISIT_CELL(c, node_list, entry_list, cb, stop)                                        \
do {                                                                                              \
    for (U16 _i = (c)->entries; _i != INVALID_INDEX;) {                                           \
        entry_node* const _n = (node_list) + _i;                                                  \
        entry_data* const _e = (entry_list) + _n->entry;                                          \
        if (!(cb)(_e)) stop;                                                                      \
        _i = _n->next;                                                                            \
    }                                                                                             \
} while (0)

#define SPT_VISIT_CELL_ONCE(c, node_list, entry_list, searchid, cb, stop)                         \
do {                                                                                              \
    U16 _i = (c)->entries;                                                                        \
    entry_node* _n;                                                                               \
    for (; _i != INVALID_INDEX; _i = _n->next) {                                                  \
        _n = (node_list) + _i;                                                                    \
        entry_data* const _e = (entry_list) + _n->entry;                                          \
        if (_e->searchid != (searchid)) {                                                         \
            _e->searchid = (searchid);                                                            \
            if (!(cb)(_e)) stop;                                                                  \
        }                                                                                         \
    }                                                                                             \
} while (0)

class xSpacePartitionTree2D
{
public:
    enum
    {
        MAX_OWNERS = 4,
        MAX_DEPTH = 8,
        INVALID_INDEX = 0xFFFF
    };

    union entry_data
    {
        entry_data* next_free_entry;
        struct
        {
            void* value;
            S32 type;
            U16 searchid;
            U16 owner_size;
            U16 owner[MAX_OWNERS];
        };
    };

    union entry_node
    {
        entry_node* next_free_node;
        struct
        {
            U16 next;
            U16 entry;
        };
    };

    union cell
    {
        cell* next_free_cell;
        struct
        {
            U16 parent;
            U16 children;
            U16 entries;
            U8 cid;
            U8 level;
            U16 xcode;
            U16 ycode;
        };
    };

    typedef bool (*intersect_callback)(entry_data*, void*);

    void create(F32 minx, F32 miny, F32 maxx, F32 maxy, F32 min_width, F32 max_width, S32 max_entries);
    void clear();
    void intersect_point(F32 x, F32 y, intersect_callback cb, void* context); // UNUSED
    void intersect_rect(F32 x0, F32 y0, F32 x1, F32 y1, intersect_callback cb, void* context);

    template <class T>
    void intersect_point(F32 x, F32 y, T& cb);
    template <class T>
    void intersect_circle(F32 x, F32 y, F32 r, T& cb);
    template <class T>
    void intersect_rect(F32 x0, F32 y0, F32 x1, F32 y1, T& cb);

    entry_data* insert(void* value, S32 type, F32 x, F32 y, F32 r);
    void insert_internal(entry_data* e, F32 x0, F32 y0, F32 x1, F32 y1, F32 width);
    void erase_internal(entry_data* e);

private:
    void create_cell(cell& c, U16 parent, S32 cid);
    void create_children(cell& c);
    void reset_searchid();
    void insert_internal(entry_data* e, cell* c, S32 end_depth, F32 x0, F32 y0, F32 x1, F32 y1);
    void remove_empty(cell* c);

    S32 get_grid_shift(F32 size, F32 max_width) const;
    cell& grid_cell(S32 gx, S32 gy);
    entry_data* alloc_entry();
    entry_node* alloc_node();
    void free_node(entry_node* e);
    cell* alloc_children();
    void free_children(cell* c);

    template <class T>
    bool intersect_rect(cell* c, F32 x0, F32 y0, F32 x1, F32 y1, T& cb);

    xVec2 origin;
    xVec2 size;
    F32 cell_size[MAX_DEPTH + 1];
    F32 cell_isize[MAX_DEPTH + 1];
    F32 cell_frac[MAX_DEPTH + 1];
    S32 max_depth;
    F32 ilogdepth;
    S32 grid_size_x;
    S32 grid_shift_x;
    S32 grid_mask_x;
    S32 grid_size_y;
    S32 grid_shift_y;
    S32 grid_mask_y;
    U16 current_searchid;
    entry_data* free_entries;
    entry_node* free_nodes;
    cell* free_cells;
    S32 free_cells_size;
    S32 safety_free_cells_size;
    cell* grid;
    entry_data* entry_buffer;
    S32 entry_buffer_size;
    entry_node* node_buffer;
    S32 node_buffer_size;
    cell* cell_buffer;
    S32 cell_buffer_size;
};

inline xSpacePartitionTree2D::entry_data* xSpacePartitionTree2D::insert(void* value, S32 type, F32 x, F32 y, F32 r)
{
    entry_data* e = alloc_entry();
    e->value = value;
    e->type = type;
    insert_internal(e, x - r, y - r, x + r, y + r, 2.0f * r);
    return e;
}

template <class T>
void xSpacePartitionTree2D::intersect_point(F32 x, F32 y, T& cb)
{
    x = cell_isize[0] * (x - origin.x); y = cell_isize[0] * (y - origin.y);

    S32 ix = (S32)x, iy = (S32)y;
    if (ix < 0 || ix >= grid_size_x || iy < 0 || iy >= grid_size_y)
        return;

    cell* c = &grid_cell(ix, iy);
    x -= ix; y -= iy;

    F32 code_scale = (F32)(1 << max_depth);
    S32 xcode = (S32)(code_scale * x), ycode = (S32)(code_scale * y);

    cell* const cell_list = cell_buffer;
    entry_node* const node_list = node_buffer;
    entry_data* const entry_list = entry_buffer;

    S32 level = max_depth;

    for (;;) {
        SPT_VISIT_CELL(c, node_list, entry_list, cb, return);

        if (c->children == INVALID_INDEX)
            break;

        --level;
        S32 bit = 1 << level;
        S32 index = ((xcode & bit) >> level) | (((ycode & bit) >> level) << 1);
        c = cell_list + c->children + index;
    }
}

template <class T>
void xSpacePartitionTree2D::intersect_circle(F32 x, F32 y, F32 r, T& cb)
{
    intersect_rect(x - r, y - r, x + r, y + r, cb);
}

template <class T>
void xSpacePartitionTree2D::intersect_rect(F32 x0, F32 y0, F32 x1, F32 y1, T& cb)
{
    ++current_searchid;
    if (current_searchid == 0)
        reset_searchid();

    F32 scale = cell_isize[0];
    x0 = scale * (x0 - origin.x); y0 = scale * (y0 - origin.y);
    x1 = scale * (x1 - origin.x); y1 = scale * (y1 - origin.y);

    S32 gx0 = CLAMP((S32)x0, 0, grid_size_x - 1);
    S32 gy0 = CLAMP((S32)y0, 0, grid_size_y - 1);
    S32 gx1 = CLAMP((S32)x1, 0, grid_size_x - 1);
    S32 gy1 = CLAMP((S32)y1, 0, grid_size_y - 1);

    for (S32 gy = gy0; gy <= gy1; ++gy) {
        for (S32 gx = gx0; gx <= gx1; ++gx) {
            if (!intersect_rect(&grid_cell(gx, gy), x0 - gx, y0 - gy, x1 - gx, y1 - gy, cb))
                break;
        }
    }
}

inline S32 xSpacePartitionTree2D::get_grid_shift(F32 size, F32 max_width) const
{
    S32 gsize = (S32)(size / max_width);
    if (gsize >= 16)
        return 4;

    S32 shift = 0;
    while (gsize > 1) {
        gsize >>= 1; ++shift;
    }
    return shift;
}

inline xSpacePartitionTree2D::cell& xSpacePartitionTree2D::grid_cell(S32 gx, S32 gy)
{
    return grid[gx + (gy << grid_shift_x)];
}

inline xSpacePartitionTree2D::entry_data* xSpacePartitionTree2D::alloc_entry()
{
    entry_data* e = free_entries;
    free_entries = e->next_free_entry;
    return e;
}

inline xSpacePartitionTree2D::entry_node* xSpacePartitionTree2D::alloc_node()
{
    entry_node* e = free_nodes;
    free_nodes = e->next_free_node;
    return e;
}

inline void xSpacePartitionTree2D::free_node(entry_node* e)
{
    e->next_free_node = free_nodes;
    free_nodes = e;
}

inline xSpacePartitionTree2D::cell* xSpacePartitionTree2D::alloc_children()
{
    cell* c = free_cells;
    free_cells = c->next_free_cell;
    free_cells_size -= 4;
    return c;
}

inline void xSpacePartitionTree2D::free_children(cell* c)
{
    c->next_free_cell = free_cells;
    free_cells = c;
    free_cells_size += 4;
}

template <class T>
bool xSpacePartitionTree2D::intersect_rect(cell* c, F32 x0, F32 y0, F32 x1, F32 y1, T& cb)
{
    F32 code_scale = (F32)(1 << max_depth);
    S32 xcode0 = (S32)(code_scale * x0);
    S32 ycode0 = (S32)(code_scale * y0);
    S32 xcode1 = (S32)(code_scale * x1);
    S32 ycode1 = (S32)(code_scale * y1);

    cell* const cell_list = cell_buffer;
    entry_node* const node_list = node_buffer;
    entry_data* const entry_list = entry_buffer;

    S32 diff = (xcode0 ^ xcode1) | (ycode0 ^ ycode1);

    S32 level = max_depth;

    for (;;) {
        --level;
        S32 bit = 1 << level;
        if (diff & bit)
            break;

        SPT_VISIT_CELL(c, node_list, entry_list, cb, return false);

        if (c->children == INVALID_INDEX)
            return true;

        S32 index = ((xcode0 & bit) >> level) | (((ycode0 & bit) >> level) << 1);
        c = cell_list + c->children + index;
    }

    U16 searchid = current_searchid;
    cell* topc = c;
    S32 index = 0;

descend:
    SPT_VISIT_CELL_ONCE(c, node_list, entry_list, searchid, cb, return false);

    for (;;) {
        if (c->children != INVALID_INDEX) {
            for (; index < 4; ++index) {
                cell* cc = cell_list + c->children + index;

                S32 code_width = 1 << cc->level;
                S32 left = cc->xcode;
                S32 right = left + code_width;
                S32 top = cc->ycode;
                S32 bottom = top + code_width;

                if (left <= xcode1 && right > xcode0 && top <= ycode1 && bottom > ycode0) {
                    c = cc;
                    index = 0;
                    goto descend;
                }
            }
        }

        if (c == topc) return true;

        index = c->cid + 1;
        c = cell_list + c->parent;
    }
}

#endif

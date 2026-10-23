#ifndef XSPACEPARTITIONTREE_H
#define XSPACEPARTITIONTREE_H

#include "xMath2.h"

struct xSpacePartitionTree2D
{
    union entry_data
    {
        entry_data* next_free_entry;

        struct
        {
            void* value;
            S32 type;
            U16 searchid;
            U16 owner_size;
            U16 owner[4];
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

    xVec2 origin;
    xVec2 size;
    F32 cell_size[9];
    F32 cell_isize[9];
    F32 cell_frac[9];
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

    void create(F32 minx, F32 miny, F32 maxx, F32 maxy, F32 min_width,
                F32 max_width, S32 max_entries);
    void clear();
};

#endif
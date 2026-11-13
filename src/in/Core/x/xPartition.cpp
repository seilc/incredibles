#include "xCommon.h"
#include "xPartition.h"

#include "xEnv.h"
#include "xMemMgr.h"
#include "xVolume.h"

#include <string.h>

void xPartitionReset()
{
}

static PartLink* PartitionGetFreeLink()
{
    PartLink* link = (PartLink*)xMEMALLOC(sizeof(PartLink), 0);
    return link;
}

static void PartitionSpaceReset(PartSpace* space)
{
    memset(space, 0, sizeof(PartSpace));
}

static void PartitionSpaceInsert(PartSpace* space, void* insert_data)
{
    space->total++;

    PartLink* link = &space->head;
    while (link->next != NULL) {
        link = link->next;
    }

    link->next = PartitionGetFreeLink();
    link->next->data = insert_data;
    link->next->next = NULL;
}

S32 xPartitionGetTrueIdx(Partition* part, S32 x, S32 y, S32 z)
{
    S32 idx = part->total_x * part->total_z * y;
    idx += part->total_x * z;
    idx += x;
    return idx;
}

void xPartitionVolume(Partition* part, xVolume* volume, S32 x_spaces, S32 y_spaces, S32 z_spaces)
{
    memset(part, 0, sizeof(Partition));

    xBound* xb = volume->GetBound();
    xBox* bb = &xb->box.box;

    part->min = bb->lower;
    part->max = bb->upper;

    F32 dx = part->max.x - part->min.x;
    F32 dy = part->max.y - part->min.y;
    F32 dz = part->max.z - part->min.z;

    part->total_x = x_spaces;
    part->total_y = y_spaces;
    part->total_z = z_spaces;

    part->space_dim.x = dx / part->total_x;
    part->space_dim.y = dy / part->total_y;
    part->space_dim.z = dz / part->total_z;

    U32 spaces_size = sizeof(PartSpace) * part->total_x * part->total_y * part->total_z;
    part->space = (PartSpace*)xMEMALLOC(spaces_size, 0);

    for (S32 z = 0; z < z_spaces; z++) {
        for (S32 y = 0; y < y_spaces; y++) {
            for (S32 x = 0; x < x_spaces; x++) {
                S32 idx = xPartitionGetTrueIdx(part, x, y, z);
                PartitionSpaceReset(&part->space[idx]);
            }
        }
    }

    PartitionSpaceReset(&part->global);
}

void xPartitionWorld(Partition* part, xEnv* env, S32 x_spaces, S32 y_spaces, S32 z_spaces)
{
    memset(part, 0, sizeof(Partition));

    xBox* bb = xEnvGetBBox(env);

    part->min = bb->lower;
    part->max = bb->upper;

    F32 dx = part->max.x - part->min.x;
    F32 dy = part->max.y - part->min.y;
    F32 dz = part->max.z - part->min.z;

    part->total_x = x_spaces;
    part->total_y = y_spaces;
    part->total_z = z_spaces;

    part->space_dim.x = dx / part->total_x;
    part->space_dim.y = dy / part->total_y;
    part->space_dim.z = dz / part->total_z;

    U32 spaces_size = sizeof(PartSpace) * part->total_x * part->total_y * part->total_z;
    part->space = (PartSpace*)xMEMALLOC(spaces_size, 0);

    for (S32 z = 0; z < z_spaces; z++) {
        for (S32 y = 0; y < y_spaces; y++) {
            for (S32 x = 0; x < x_spaces; x++) {
                S32 idx = xPartitionGetTrueIdx(part, x, y, z);
                PartitionSpaceReset(&part->space[idx]);
            }
        }
    }

    PartitionSpaceReset(&part->global);
}

S32 xPartitionInsert(Partition* part, void* insert_data, xVec3* insert_pos)
{
    if (insert_pos->x < part->min.x || insert_pos->x > part->max.x || insert_pos->y < part->min.y ||
        insert_pos->y > part->max.y || insert_pos->z < part->min.z || insert_pos->z > part->max.z) {
        PartitionSpaceInsert(&part->global, insert_data);
        return -1;
    }

    S32 idx = xPartitionGetTrueIdx(part, (S32)((insert_pos->x - part->min.x) / part->space_dim.x),
                                   (S32)((insert_pos->y - part->min.y) / part->space_dim.y),
                                   (S32)((insert_pos->z - part->min.z) / part->space_dim.z));
    PartitionSpaceInsert(&part->space[idx], insert_data);
    return idx;
}

S32 xPartitionUpdate(Partition* part, void* data, S32 old_idx, xVec3* current_pos)
{
    S32 cur_idx;
    if (current_pos->x < part->min.x || current_pos->x > part->max.x || current_pos->y < part->min.y ||
        current_pos->y > part->max.y || current_pos->z < part->min.z || current_pos->z > part->max.z) {
        cur_idx = -1;
    } else {
        cur_idx = xPartitionGetTrueIdx(part, (S32)((current_pos->x - part->min.x) / part->space_dim.x),
                                       (S32)((current_pos->y - part->min.y) / part->space_dim.y),
                                       (S32)((current_pos->z - part->min.z) / part->space_dim.z));
    }

    if (old_idx == cur_idx) {
        return cur_idx;
    }

    PartSpace* src = (old_idx == -1) ? &part->global : &part->space[old_idx];
    xPartitionSpaceMove(src, (cur_idx == -1) ? &part->global : &part->space[cur_idx], (U32)data);
    return cur_idx;
}

void xPartitionSpaceMove(PartSpace* src, PartSpace* dest, U32 data)
{
    PartLink* dest_lnk = &dest->head;
    while (dest_lnk->next != NULL) {
        dest_lnk = dest_lnk->next;
    }

    PartLink* src_lnk = &src->head;
    PartLink* src_pre = src_lnk;
    while ((U32)src_lnk->data != data) {
        src_pre = src_lnk;
        src_lnk = src_lnk->next;
    }

    src_pre->next = src_lnk->next;
    dest_lnk->next = src_lnk;
    dest_lnk->next->next = NULL;

    src->total--;
    dest->total++;
}

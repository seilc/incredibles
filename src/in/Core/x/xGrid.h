#ifndef XGRID_H
#define XGRID_H

#include "types.h"

struct xEnt;
struct xQCData;
struct xBound;
struct xBox;
struct xVec3;
struct xGridBound;

struct xGrid
{
    U8 ingrid_id;
    U8 pad[3];
    U16 nx;
    U16 nz;
    F32 minx;
    F32 minz;
    F32 maxx;
    F32 maxz;
    F32 csizex;
    F32 csizez;
    F32 inv_csizex;
    F32 inv_csizez;
    F32 maxr;
    xGridBound** cells;
    xGridBound* other;
    S32 iter_active;
};

struct xGridBound
{
    void* data;
    U16 gx;
    U16 gz;
    U8 oversize;
    bool deleted;
    U8 gpad;
    U8 pad;
    xGrid* grid;
    xGridBound** head;
    xGridBound* next;
};

struct xGridIterator
{
    xGridBound** listhead;
    xGridBound* curcell;
    U32 delfound;
};

typedef S32(*xGridCallback)(xEnt* ent, void* cbdata);

void xGridBoundInit(xGridBound* gridb, void* data);
void xGridInit(xGrid* grid, const xBox* bounds, U16 nx, U16 nz, U8 ingrid_id);
void xGridKill(xGrid* grid);
void xGridEmpty(xGrid* grid);
S32 xGridAddToCell(xGridBound** cell, xGridBound* gridb);
S32 xGridAdd(xGrid* grid, xEnt* ent);
S32 xGridRemove(xGridBound* gridb);
void xGridUpdate(xGrid* grid, xEnt* ent);
void xGridGetCell(xGrid* grid, F32 posx, F32 posy, F32 posz, S32& grx, S32& grz);
xGridBound* xGridIterFirstCell(xGrid* grid, F32 posx, F32 posy, F32 posz, S32& grx, S32& grz, xGridIterator& it);
S32 xGridEntIsTooBig(xGrid* grid, const xEnt* ent);
void xGridCheckPosition(xGrid* grid, xVec3* pos, xQCData* qcd, xGridCallback hitCB, void* cbdata);
xGridBound* xGridIterFirstCell(xGridBound** head, xGridIterator& it);
xGridBound* xGridIterFirstCell(xGrid* grid, S32 grx, S32 grz, xGridIterator& it);
void xGridIterClose(xGridIterator& it);
void xGridCheckBoundSetup(xBox& box, S32& lower_x, S32& lower_z, S32& upper_x, S32& upper_z, xGrid& grid,
                          const xBound& bound);

inline xGridBound* xGridIterNextCell(xGridIterator& it)
{
    if (it.curcell) {
        it.curcell = it.curcell->next;
    }

    while (it.curcell) {
        if (!it.curcell->deleted) {
            return it.curcell;
        }

        it.delfound = TRUE;
        it.curcell = it.curcell->next;
    }

    xGridIterClose(it);
    return NULL;
}

template <class T>
void xGridCheckBound(xGrid& grid, const xBound& bound, const xQCData& qcd, T& cb)
{
    xGridIterator it;
    xBox box;
    S32 lower_x, lower_z, upper_x, upper_z;

    xGridCheckBoundSetup(box, lower_x, lower_z, upper_x, upper_z, grid, bound);

    xGridBound* cell = xGridIterFirstCell(&grid.other, it);
    S32 gx = lower_x, gz = lower_z;

    while (true) {
        while (cell) {
            xBound* cellbound = (xBound*)(cell + 1);
            if (xQuickCullIsects(&qcd, &cellbound->qcd)) {
                if (!cb(*(xEnt*)cell->data)) {
                    xGridIterClose(it);
                    return;
                }
            }
            cell = xGridIterNextCell(it);
        }

        if (gx > upper_x) break;

        cell = xGridIterFirstCell(&grid, gx, gz, it);
        gz++;
        if (gz > upper_z) {
            gz = lower_z;
            gx++;
        }
    }
}

#endif
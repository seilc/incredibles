#ifndef XGRID_H
#define XGRID_H

#include <types.h>

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
    U8 deleted;
    U8 gpad;
    U8 pad;
    xGrid* grid;
    xGridBound** head;
    xGridBound* next;
};

struct xEnt;

S32 xGridAdd(xGrid* grid, xEnt* ent);
S32 xGridRemove(xGridBound* gridb);

#endif
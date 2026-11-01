#include "xGrid.h"

#include "xEnt.h"
#include "xMath.h"
#include "xMemMgr.h"

#include <string.h>

static xGridBound** xGridGetCell(xGrid* grid, const xEnt* ent, S32& grx, S32& grz);

void xGridBoundInit(xGridBound* gridb, void* data)
{
    gridb->data = data;
    gridb->gx = -1;
    gridb->gz = -1;
    gridb->grid = NULL;
    gridb->oversize = 0;
    gridb->head = NULL;
    gridb->next = NULL;
    gridb->gpad = 0xea;
}

void xGridInit(xGrid* grid, const xBox* bounds, U16 nx, U16 nz, U8 ingrid_id)
{
    grid->ingrid_id = ingrid_id;

    grid->nx = nx;
    grid->nz = nz;
    grid->minx = bounds->lower.x;
    grid->minz = bounds->lower.z;
    grid->maxx = bounds->upper.x;
    grid->maxz = bounds->upper.z;
    F32 gsizex = grid->maxx - grid->minx;
    F32 gsizez = grid->maxz - grid->minz;
    grid->csizex = gsizex / nx;
    grid->csizez = gsizez / nz;
    if (xabs(gsizex) <= 0.001f) {
        grid->inv_csizex = 1.0f;
    } else {
        grid->inv_csizex = nx / gsizex;
    }

    if (xabs(gsizez) <= 0.001f) {
        grid->inv_csizez = 1.0f;
    } else {
        grid->inv_csizez = nz / gsizez;
    }

    grid->iter_active = 0;
    grid->maxr = 0.25f * MIN(grid->csizex, grid->csizez);
    grid->cells = (xGridBound**)xMEMALLOC((U32)nx * nz * sizeof(xGridBound*), 0);
    memset(grid->cells, 0, nx * nz * sizeof(xGridBound*));
}

void xGridKill(xGrid* grid)
{
    xGridEmpty(grid);

    grid->cells = NULL;
    grid->iter_active = 0;
}

void xGridEmpty(xGrid* grid)
{
    xGridBound** head, *curr;

    for (S32 x = 0; x < grid->nx; x++) {
        for (S32 z = 0; z < grid->nz; z++) {
            head = grid->cells + z * grid->nx + x;
            curr = *head;
            while (curr) {
                xGridBound* currnext = curr->next;
                xGridBoundInit(curr, curr->data);
                curr = currnext;
            }
            *head = NULL;
        }
    }

    head = &grid->other;
    curr = *head;
    while (curr) {
        xGridBound* currnext = curr->next;
        xGridBoundInit(curr, curr->data);
        curr = currnext;
    }
    *head = NULL;
}

S32 xGridAddToCell(xGridBound** cell, xGridBound* gridb)
{
    if (gridb->head) {
        if (!gridb->grid->iter_active) {
            if (!xGridRemove(gridb)) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    gridb->head = cell;
    gridb->next = *cell;
    *cell = gridb;
    return TRUE;
}

static S32 xGridAdd(xGrid* grid, xGridBound* gridb, S32 x, S32 z)
{
    xGridBound** cell = grid->cells + z * grid->nx + x;
    return xGridAddToCell(cell, gridb);
}

S32 xGridAdd(xGrid* grid, xEnt* ent)
{
    xBound* bound = &ent->bound;
    xVec3* center;
    F32 maxr = grid->maxr;
    if (bound->type == k_XBOUNDTYPE_SPHERE) {
        xSphere* sph = &bound->sph;
        center = &sph->center;
        if (sph->r >= maxr) {
            S32 r = xGridAddToCell(&grid->other, &ent->gridb);
            if (r) {
                ent->gridb.grid = grid;
            }
            return r;
        }
    } else if (bound->type == k_XBOUNDTYPE_OBB) {
        xBBox* bbox = &bound->box;
        center = &bbox->center;

        F32 rx, ry, rz;
        rx = bbox->box.upper.x - bbox->box.lower.x, ry = bbox->box.upper.y - bbox->box.lower.y,
        rz = bbox->box.upper.z - bbox->box.lower.z;
        F32 len2 = xsqr(rx) * (xsqr(bound->mat->right.x) + xsqr(bound->mat->right.y) + xsqr(bound->mat->right.z)) +
                   xsqr(ry) * (xsqr(bound->mat->up.x) + xsqr(bound->mat->up.y) + xsqr(bound->mat->up.z)) +
                   xsqr(rz) * (xsqr(bound->mat->at.x) + xsqr(bound->mat->at.y) + xsqr(bound->mat->at.z));
        if (len2 >= 4.0f * maxr * maxr) {
            S32 r = xGridAddToCell(&grid->other, &ent->gridb);
            if (r) {
                ent->gridb.grid = grid;
            }
            return r;
        }
    } else if (bound->type == k_XBOUNDTYPE_BOX) {
        xBBox* bbox = &bound->box;
        center = &bbox->center;

        F32 rx, rz;
        rx = bbox->box.upper.x - bbox->box.lower.x, rz = bbox->box.upper.z - bbox->box.lower.z;
        F32 len2 = xsqr(rx) + xsqr(rz);
        if (len2 >= 4.0f * maxr * maxr) {
            S32 r = xGridAddToCell(&grid->other, &ent->gridb);
            if (r) {
                ent->gridb.grid = grid;
            }
            return r;
        }
    } else {
        return FALSE;
    }

    F32 cgridx, cgridz;
    cgridx = center->x - grid->minx, cgridz = center->z - grid->minz;
    cgridx *= grid->inv_csizex, cgridz *= grid->inv_csizez;
    S32 x = MIN(grid->nx - 1, MAX(0.0f, cgridx));
    S32 z = MIN(grid->nz - 1, MAX(0.0f, cgridz));

    if (xGridAdd(grid, &ent->gridb, x, z)) {
        ent->gridb.gx = x;
        ent->gridb.gz = z;
        ent->gridb.grid = grid;
        return TRUE;
    }

    return FALSE;
}

S32 xGridRemove(xGridBound* gridb)
{
    if (gridb->head) {
        if (gridb->grid->iter_active) {
            gridb->deleted = TRUE;
            return FALSE;
        }

        xGridBound* cur, **prev;
        prev = gridb->head;
        cur = *prev;
        while (cur && cur != gridb) {
            prev = &cur->next;
            cur = cur->next;
        }

        *prev = cur->next;
        cur->next = NULL;
        cur->head = NULL;
        cur->grid = NULL;
        cur->deleted = FALSE;
        cur->gx = -1;
        cur->gz = -1;
    }

    return TRUE;
}

void xGridUpdate(xGrid* grid, xEnt* ent)
{
    S32 dx, dz;
    xGridGetCell(grid, ent, dx, dz);

    xGridBound* gridb = &ent->gridb;
    if (gridb->oversize != 2 && (dx != gridb->gx || dz != gridb->gz)) {
        xGridBound* cur, **prev;
        prev = gridb->head;
        cur = *prev;
        while (cur && cur != gridb) {
            prev = &cur->next;
            cur = cur->next;
        }

        *prev = cur->next;
        cur->head = NULL;
        cur->next = NULL;
        if (cur != gridb) {
            cur->grid = NULL;
            cur->deleted = FALSE;
            cur->gx = -1;
            cur->gz = -1;
        }

        xGridBound** cell = grid->cells + dz * grid->nx + dx;

        gridb->head = cell;
        gridb->next = *cell;
        gridb->gx = dx;
        gridb->gz = dz;
        *cell = gridb;
    }
}

static xGridBound** xGridGetCell(xGrid* grid, const xEnt* ent, S32& grx, S32& grz)
{
    const xBound* bound = &ent->bound;
    const xVec3* center = xBoundCenter(bound);
    xGridGetCell(grid, center->x, center->y, center->z, grx, grz);
    return grid->cells + grz * grid->nx + grx;
}

void xGridGetCell(xGrid* grid, F32 posx, F32, F32 posz, S32& grx, S32& grz)
{
    F32 pgridx, pgridz;
    pgridx = posx - grid->minx, pgridz = posz - grid->minz;
    pgridx *= grid->inv_csizex, pgridz *= grid->inv_csizez;
    grx = MIN(grid->nx - 1, MAX(0.0f, pgridx));
    grz = MIN(grid->nz - 1, MAX(0.0f, pgridz));
}

xGridBound* xGridIterFirstCell(xGrid* grid, F32 posx, F32 posy, F32 posz, S32& grx, S32& grz, xGridIterator& it)
{
    xGridGetCell(grid, posx, posy, posz, grx, grz);
    return xGridIterFirstCell(grid, grx, grz, it);
}

S32 xGridEntIsTooBig(xGrid* grid, const xEnt* ent)
{
    const xBound* bound = &ent->bound;
    F32 maxr = grid->maxr;
    if (bound->type == k_XBOUNDTYPE_SPHERE) {
        const xSphere* sph = &bound->sph;
        if (sph->r >= maxr) {
            return TRUE;
        }
    } else if (bound->type == k_XBOUNDTYPE_OBB) {
        const xBBox* bbox = &bound->box;

        F32 rx, ry, rz;
        rx = bbox->box.upper.x - bbox->box.lower.x, ry = bbox->box.upper.y - bbox->box.lower.y,
        rz = bbox->box.upper.z - bbox->box.lower.z;
        F32 len2 = xsqr(rx) * (xsqr(bound->mat->right.x) + xsqr(bound->mat->right.y) + xsqr(bound->mat->right.z)) +
                   xsqr(ry) * (xsqr(bound->mat->up.x) + xsqr(bound->mat->up.y) + xsqr(bound->mat->up.z)) +
                   xsqr(rz) * (xsqr(bound->mat->at.x) + xsqr(bound->mat->at.y) + xsqr(bound->mat->at.z));
        if (len2 >= 4.0f * maxr * maxr) {
            return TRUE;
        }
    } else if (bound->type == k_XBOUNDTYPE_BOX) {
        const xBBox* bbox = &bound->box;

        F32 rx, rz;
        rx = bbox->box.upper.x - bbox->box.lower.x, rz = bbox->box.upper.z - bbox->box.lower.z;
        F32 len2 = xsqr(rx) + xsqr(rz);
        if (len2 >= 4.0f * maxr * maxr) {
            return TRUE;
        }
    }

    return FALSE;
}

void xGridCheckPosition(xGrid* grid, xVec3* pos, xQCData* qcd, xGridCallback hitCB, void* cbdata)
{
    xGridIterator it;
    xEnt* ent;
    S32 px, pz;

    xGridBound* cell = xGridIterFirstCell(grid, pos->x, pos->y, pos->z, px, pz, it);
    while (cell) {
        xBound* cellbound = (xBound*)(cell + 1);
        if (xQuickCullIsects(qcd, &cellbound->qcd)) {
            ent = (xEnt*)cell->data;
            if (!hitCB(ent, cbdata)) {
                xGridIterClose(it);
                return;
            }
        }

        cell = xGridIterNextCell(it);
    }

    static S32 offs[4][3][2] = {
        { { -1, 0 }, { -1, -1 }, { 0, -1 } },
        { { 0, -1 }, { 1, -1 }, { 1, 0 } },
        { { 1, 0 }, { 1, 1 }, { 0, 1 } },
        { { 0, 1 }, { -1, 1 }, { -1, 0 } },
    };
    static S32 k;
    xBox clbox;
    F32 clcenterx, clcenterz;

    clbox.lower.x = grid->csizex * px, clbox.lower.z = grid->csizez * pz;
    clbox.lower.x += grid->minx, clbox.lower.z += grid->minz;
    clcenterx = 0.5f * grid->csizex, clcenterz = 0.5f * grid->csizez;
    clcenterx += clbox.lower.x, clcenterz += clbox.lower.z;
    if (pos->x < clcenterx) {
        if (pos->z < clcenterz) {
            k = 0;
        } else {
            k = 1;
        }
    } else {
        if (pos->z < clcenterz) {
            k = 3;
        } else {
            k = 2;
        }
    }

    for (S32 i = 0; i < 3; i++) {
        S32 _x = px + offs[k][i][1];
        if (_x < 0 || _x >= grid->nx) {
            continue;
        }

        S32 _z = pz + offs[k][i][0];
        if (_z < 0 || _z >= grid->nz) {
            continue;
        }

        cell = xGridIterFirstCell(grid, _x, _z, it);
        while (cell) {
            xBound* cellbound = (xBound*)(cell + 1);
            if (xQuickCullIsects(qcd, &cellbound->qcd)) {
                ent = (xEnt*)cell->data;
                if (!hitCB(ent, cbdata)) {
                    xGridIterClose(it);
                    return;
                }
            }

            cell = xGridIterNextCell(it);
        }
    }

    cell = xGridIterFirstCell(&grid->other, it);
    while (cell) {
        xBound* cellbound = (xBound*)(cell + 1);
        if (xQuickCullIsects(qcd, &cellbound->qcd)) {
            ent = (xEnt*)cell->data;
            if (!hitCB(ent, cbdata)) {
                xGridIterClose(it);
                return;
            }
        }

        cell = xGridIterNextCell(it);
    }
}

xGridBound* xGridIterFirstCell(xGridBound** head, xGridIterator& it)
{
    xGridBound* curcell = *head;
    if (!curcell) return NULL;
    it.delfound = FALSE;
    it.listhead = head;

    it.curcell = curcell;
    curcell->grid->iter_active++;
    return curcell;
}

xGridBound* xGridIterFirstCell(xGrid* grid, S32 grx, S32 grz, xGridIterator& it)
{
    if (grx < 0 || grx >= grid->nx) return NULL;
    if (grz < 0 || grz >= grid->nz) return NULL;
    return xGridIterFirstCell(grid->cells + grz * grid->nx + grx, it);
}

void xGridIterClose(xGridIterator& it)
{
    if (it.listhead) {
        S32& iter_active = (*it.listhead)->grid->iter_active;
        iter_active--;
        if (it.delfound && !iter_active) {
            xGridBound* cur, **prev;
            prev = it.listhead;
            cur = *it.listhead;
            while (cur) {
                if (cur->deleted) {
                    *prev = cur->next;
                    cur->next = NULL;
                    cur->head = NULL;
                    cur->grid = NULL;
                    cur->deleted = FALSE;
                    cur->gx = -1;
                    cur->gz = -1;
                    cur = *prev;
                } else {
                    prev = &cur->next;
                    cur = cur->next;
                }
            }
        }

        it.listhead = NULL;
        it.curcell = NULL;
        it.delfound = FALSE;
    }
}

static inline void get_grid_index(S32& ox, S32& oz, const xGrid& grid, F32 x, F32 z)
{
    S32 ix = grid.inv_csizex * (x - grid.minx);
    S32 iz = grid.inv_csizez * (z - grid.minz);
    S32 xmax = grid.nx - 1;
    S32 zmax = grid.nz - 1;
    ox = CLAMP(ix, 0, xmax);
    oz = CLAMP(iz, 0, zmax);
}

void xGridCheckBoundSetup(xBox& box, S32& lower_x, S32& lower_z, S32& upper_x, S32& upper_z, xGrid& grid,
                          const xBound& bound)
{
    xBoundGetBox(box, bound);

    F32 ex = 0.25f * grid.csizex, ez = 0.25f * grid.csizez;
    box.upper.x += ex;
    box.upper.z += ez;
    box.lower.x -= ex;
    box.lower.z -= ez;
    get_grid_index(lower_x, lower_z, grid, box.lower.x, box.lower.z);
    get_grid_index(upper_x, upper_z, grid, box.upper.x, box.upper.z);

    if (lower_z > upper_z)
        lower_x = upper_x + 1;
}

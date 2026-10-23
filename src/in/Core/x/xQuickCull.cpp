#include "xQuickCull.h"

#include "xBound.h"

#include "decomp.h"

xQCControl xqc_def_ctrl;

#if DEBUG
static U32 dbg_regs = 0;
static U32 dbg_comps = 0;
static U32 dbg_culls = 0;
#endif

void xQuickCullInit(xQCControl* ctrl, F32 xmin, F32 ymin, F32 zmin, F32 xmax, F32 ymax, F32 zmax)
{
    xASSERT(54, ctrl);

    ctrl->world_xmin = xmin;
    ctrl->world_ymin = ymin;
    ctrl->world_zmin = zmin;
    ctrl->world_xmax = xmax;
    ctrl->world_ymax = ymax;
    ctrl->world_zmax = zmax;
    ctrl->world_xsz = xmax - xmin;
    ctrl->world_ysz = ymax - ymin;
    ctrl->world_zsz = zmax - zmin;

    if (ctrl->world_xsz <= 0.0f || ctrl->world_ysz <= 0.0f || ctrl->world_zsz <= 0.0f) {
        iprintf("xQuickCullInit: invalid box <%f %f %f><%f %f %f>\n",
                xmin, ymin, zmin, xmax, ymax, zmax);
        ctrl->world_xsz = ctrl->world_ysz = ctrl->world_zsz = 1.0f;
    }

    xASSERT(72, ctrl->world_xsz > 0.0f);
    xASSERT(73, ctrl->world_ysz > 0.0f);
    xASSERT(74, ctrl->world_zsz > 0.0f);

    ctrl->scale_x = 127.0f / ctrl->world_xsz;
    ctrl->scale_y = 127.0f / ctrl->world_ysz;
    ctrl->scale_z = 127.0f / ctrl->world_zsz;
    ctrl->center_x = 0.5f * (xmax + xmin) + 0.5f / ctrl->scale_x;
    ctrl->center_y = 0.5f * (ymax + ymin) + 0.5f / ctrl->scale_y;
    ctrl->center_z = 0.5f * (zmax + zmin) + 0.5f / ctrl->scale_z;
}

void xQuickCullInit(xQCControl* ctrl, const xBox* box)
{
    xASSERT(129, box);
    xQuickCullInit(ctrl,
                   box->lower.x, box->lower.y, box->lower.z,
                   box->upper.x, box->upper.y, box->upper.z);
}

S32 xQuickCullIsects(const xQCData* a, const xQCData* b)
{
    S32 result = !(a->xmin > b->xmax || a->ymin > b->ymax || a->zmin > b->zmax ||
                   b->xmin > a->xmax || b->ymin > a->ymax || b->zmin > a->zmax);
    
#if DEBUG
    if (!result) {
        dbg_culls++;
    }
    dbg_comps++;
#endif

    return result;
}

void xQuickCullForBound(xQCControl* ctrl, xQCData* q, const xBound* b)
{
    xASSERT(184, ctrl);

    if (b->type == k_XBOUNDTYPE_SPHERE) {
        xQuickCullForSphere(ctrl, q, &b->sph);
    } else if (b->type == k_XBOUNDTYPE_BOX) {
        xQuickCullForBox(ctrl, q, &b->box.box);
    } else if (b->type == k_XBOUNDTYPE_CYLINDER) {
        xBox box;
        xBoundGetBox(box, *b);
        xQuickCullForBox(ctrl, q, &box);
    } else if (b->type == k_XBOUNDTYPE_OBB) {
        xQuickCullForOBB(ctrl, q, &b->box.box, b->mat);
    }    
}

#define CLAMP(x, a, b) ((x) < (b) ? (x) : (b)) < (a) ? (a) : ((x) < (b) ? (x) : (b))

static void xQuickCullCellForVec(xQCControl* ctrl, xQCData* c, const xVec3* v)
{
    c->xmin = CLAMP((S32)((v->x - ctrl->center_x) * ctrl->scale_x), -127, 127);
    c->ymin = CLAMP((S32)((v->y - ctrl->center_y) * ctrl->scale_y), -127, 127);
    c->zmin = CLAMP((S32)((v->z - ctrl->center_z) * ctrl->scale_z), -127, 127);
    c->zmin_dup = c->zmin;
    c->xmax = c->xmin;
    c->ymax = c->ymin;
    c->zmax = c->zmin;
    c->zmax_dup = c->zmin_dup;
}

static void xQuickCullCellMerge(xQCData* o, const xQCData* a, const xQCData* b)
{
    o->xmin = xmin(a->xmin, b->xmin);
    o->ymin = xmin(a->ymin, b->ymin);
    o->zmin = xmin(a->zmin, b->zmin);
    o->zmin_dup = o->zmin;
    o->xmax = xmax(a->xmax, b->xmax);
    o->ymax = xmax(a->ymax, b->ymax);
    o->zmax = xmax(a->zmax, b->zmax);
    o->zmax_dup = o->zmax;
}

DECOMP_FORCEACTIVE("ctrl", "q", "v")

void xQuickCullForLine(xQCControl* ctrl, xQCData* q, const xLine3* ln)
{
    xASSERT(544, ctrl);
    xASSERT(545, q);
    xASSERT(546, ln);

    xQCData a;
    xQuickCullCellForVec(ctrl, &a, &ln->p1);

    xQCData b;
    xQuickCullCellForVec(ctrl, &b, &ln->p2);

    xQuickCullCellMerge(q, &a, &b);
}

void xQuickCullForRay(xQCControl* ctrl, xQCData* q, const xRay3* r)
{
    xASSERT(556, ctrl);
    xASSERT(557, q);
    xASSERT(558, r);

    xLine3 ln;

    if (r->flags & XRAY3_USE_MIN) {
        xVec3 delta;
        delta.x = r->dir.x * r->min_t,
            delta.y = r->dir.y * r->min_t,
            delta.z = r->dir.z * r->min_t;
        ln.p1.x = r->origin.x + delta.x,
            ln.p1.y = r->origin.y + delta.y,
            ln.p1.z = r->origin.z + delta.z;
    } else {
        ln.p1.x = r->origin.x,
            ln.p1.y = r->origin.y,
            ln.p1.z = r->origin.z;
    }

    if (r->flags & XRAY3_USE_MAX) {
        F32 len;
        if (r->flags & XRAY3_USE_MIN) {
            len = r->max_t - r->min_t;
        } else {
            len = r->max_t;
        }
        ln.p2.x = r->dir.x * len,
            ln.p2.y = r->dir.y * len,
            ln.p2.z = r->dir.z * len;
    } else {
        ln.p2.x = r->dir.x,
            ln.p2.y = r->dir.y,
            ln.p2.z = r->dir.z;
    }

    ln.p2.x = ln.p1.x + ln.p2.x,
        ln.p2.y = ln.p1.y + ln.p2.y,
        ln.p2.z = ln.p1.z + ln.p2.z;

    xQCData a;
    xQuickCullCellForVec(ctrl, &a, &ln.p1);

    xQCData b;
    xQuickCullCellForVec(ctrl, &b, &ln.p2);

    xQuickCullCellMerge(q, &a, &b);
}

void xQuickCullForSphere(xQCControl* ctrl, xQCData* q, const xSphere* s)
{
    xASSERT(571, ctrl);
    xASSERT(572, q);
    xASSERT(573, s);

    xVec3 v1, v2;
    F32 radius_adjusted = 0.05f + s->r;

    v1.x = -radius_adjusted,
        v1.y = -radius_adjusted,
        v1.z = -radius_adjusted;
    v2.x = radius_adjusted,
        v2.y = radius_adjusted,
        v2.z = radius_adjusted;
    
    v1.x += s->center.x,
        v1.y += s->center.y,
        v1.z += s->center.z;
    v2.x += s->center.x,
        v2.y += s->center.y,
        v2.z += s->center.z;
    
    xQCData a;
    xQuickCullCellForVec(ctrl, &a, &v1);

    xQCData b;
    xQuickCullCellForVec(ctrl, &b, &v2);

    xQuickCullCellMerge(q, &a, &b);
}

void xQuickCullForBox(xQCControl* ctrl, xQCData* q, const xBox* box)
{
    xASSERT(597, ctrl);
    xASSERT(598, q);
    xASSERT(599, box);

    xQCData a;
    xQuickCullCellForVec(ctrl, &a, &box->lower);

    xQCData b;
    xQuickCullCellForVec(ctrl, &b, &box->upper);

    xQuickCullCellMerge(q, &a, &b);
}

void xQuickCullForOBB(xQCControl* ctrl, xQCData* q, const xBox* b, const xMat4x3* m)
{
    xASSERT(609, ctrl);

    xBox worldbox;
    xBoxInitBoundOBB(&worldbox, b, m);

    xQuickCullForBox(ctrl, q, &worldbox);
}

void xQuickCullForEverything(xQCData* q)
{
    q->xmin = -128;
    q->ymin = -128;
    q->zmin = -128;
    q->zmin_dup = -128;
    q->xmax = 127;
    q->ymax = 127;
    q->zmax = 127;
    q->zmax_dup = 127;
}

#if DEBUG
void xQuickCullDebugCB()
{
    xprintf("world_min=<%.3f %.3f %.3f>\n",
            xqc_def_ctrl.world_xmin, xqc_def_ctrl.world_ymin, xqc_def_ctrl.world_zmin);
    xprintf("world_max=<%.3f %.3f %.3f>\n",
            xqc_def_ctrl.world_xmax, xqc_def_ctrl.world_ymax, xqc_def_ctrl.world_zmax);
    xprintf("world_sz=<%.3f %.3f %.3f>\n",
            xqc_def_ctrl.world_xsz, xqc_def_ctrl.world_ysz, xqc_def_ctrl.world_zsz);
    xprintf("block_sz=<%.3f %.3f %.3f>\n",
            xqc_def_ctrl.world_xsz / 254.0f, xqc_def_ctrl.world_ysz / 254.0f, xqc_def_ctrl.world_zsz / 254.0f);
    xprintf("cull ratio: %.4f%% (%d/%d)\n",
            dbg_culls * 100.0f / dbg_comps, dbg_culls, dbg_comps);
}

void xQuickCullDebugUpdate()
{
    dbg_regs = 0;
    dbg_comps = 0;
    dbg_culls = 0;
}

void xQuickCullDebugRegister(const xQCData* q)
{
    xASSERT(716, q);
    dbg_regs++;
}
#endif

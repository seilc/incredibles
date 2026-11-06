#include "xBound.h"

#include "xCollide.h"
#include "iMath3.h"

// UNUSED: speculative
void xBoundMakeSphere(xBound& b, const xVec3& center, F32 radius)
{
    b.type = k_XBOUNDTYPE_SPHERE;
    b.sph.center = center;
    b.sph.r = radius;
    xQuickCullForSphere(&b.qcd, &b.sph);
}

void xBoundUpdate(xBound* b)
{
    if (b->type == k_XBOUNDTYPE_BOX) {
        xBoxCenter(&b->box.center, &b->box.box);
    }
    else if (b->type == k_XBOUNDTYPE_OBB) {
        xBoxCenter(&b->box.center, &b->box.box);
        xMat4x3Toworld(&b->box.center, b->mat, &b->box.center);
    }

    if (b->type != k_XBOUNDTYPE_NONE) {
        xQuickCullForBound(&b->qcd, b);
    }
}

void xBoundGetBox(xBox& box, const xBound& bound)
{
    switch (bound.type) {
    case k_XBOUNDTYPE_SPHERE: {
        const xSphere& o = bound.sph;
        box.upper.assign(o.center.x + o.r, o.center.y + o.r, o.center.z + o.r);
        box.lower.assign(o.center.x - o.r, o.center.y - o.r, o.center.z - o.r);
    }
        break;
    case k_XBOUNDTYPE_BOX:
        box = bound.box.box;
        break;
    case k_XBOUNDTYPE_OBB:
        xBoxInitBoundOBB(&box, &bound.box.box, bound.mat);
        break;
    case k_XBOUNDTYPE_CYLINDER: {
        const xCylinder& c = bound.cyl;
        box.upper.assign(c.center.x + c.r, c.center.y + c.h, c.center.z + c.r);
        box.lower.assign(c.center.x - c.r, c.center.y - c.h, c.center.z - c.r);
    }
        break;
    }
}

void xBoundGetCylinder(xCylinder& o, const xBound& bound)
{
    switch (bound.type) {
    case k_XBOUNDTYPE_SPHERE:
        o.center = bound.sph.center;
        o.h = bound.sph.r;
        o.r = bound.sph.r;
        break;
    case k_XBOUNDTYPE_BOX: {
        o.center = bound.box.center;
        xVec3 diff = bound.box.box.upper - bound.box.center;
        o.h = diff.y;
        diff.y = 0.0f;
        o.r = diff.length();
        break;
    }
    case k_XBOUNDTYPE_OBB: {
        const xMat4x3& mat = *bound.mat;
        xVec3 v = (bound.box.box.upper - bound.box.box.lower) * 0.5f;
        o.r = xsqrt(v.x * v.x * mat.right.length2() + v.z * v.z * mat.at.length2());
        o.center = mat.pos;
        o.center.y += bound.box.box.upper.y - bound.box.box.lower.y;
        o.h = v.y;
    }
        break;
    case k_XBOUNDTYPE_CYLINDER:
        o = bound.cyl;
    }
}

void xBoundGetSphere(xSphere& o, const xBound& bound)
{
    switch (bound.type) {
    case k_XBOUNDTYPE_SPHERE:
        o = bound.sph;
        break;
    case k_XBOUNDTYPE_BOX:
        o.center = bound.box.center;
        o.r = (bound.box.box.upper - bound.box.center).length();
        break;
    case k_XBOUNDTYPE_OBB: {
        const xMat4x3& mat = *bound.mat;
        xVec3 v = (bound.box.box.upper - bound.box.box.lower) * 0.5f;
        o.r = xsqrt(v.x * v.x * mat.right.length2() + v.y * v.y * mat.up.length2() + v.z * v.z * mat.at.length2());
        o.center = bound.box.center;
        break;
    }
    }
}

void xBoundSphereHitsOBB(const xSphere* s, const xBox* b, const xMat4x3* m, xCollis* coll)
{
    xSphereHitsOBB_nu(s, b, m, coll);
}

void xBoundHitsBound(const xBound* a, const xBound* b, xCollis* c)
{
    if (!xQuickCullIsects(&a->qcd, &b->qcd)) {
        c->flags &= ~k_HIT_IT;
        return;
    }

    switch (a->type | (b->type << 3)) {
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_SPHERE << 3):
        xSphereHitsSphere(&a->sph, &b->sph, c);
        break;
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_OBB << 3):
        xBoundSphereHitsOBB(&a->sph, &b->box.box, b->mat, c);
        break;
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_BOX << 3):
        xSphereHitsBox(&a->sph, &b->box.box, c);
        break;
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_SPHERE << 3):
        xBoxHitsSphere(&a->box.box, &b->sph, c);
        break;
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_OBB << 3):
        xBoxHitsObb(&a->box.box, &b->box.box, b->mat, c);
        break;
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_SPHERE << 3):
        xBoundSphereHitsOBB(&b->sph, &a->box.box, a->mat, c);
        break;
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_OBB << 3):
        xObbHitsObb(&a->box.box, a->mat, &b->box.box, b->mat, c);
        break;
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_BOX << 3):
        xBoxHitsObb(&b->box.box, &a->box.box, a->mat, c);
        break;
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_CYLINDER << 3):
        xSphereHitsVCylinder(&a->sph, &b->cyl, c);
        break;
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_BOX << 3):
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_CYLINDER << 3):
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_CYLINDER << 3):
    case k_XBOUNDTYPE_CYLINDER | (k_XBOUNDTYPE_SPHERE << 3):
        xCylinderHitsSphere(&a->cyl, &b->sph, c);
        break;
    }
}

static void xBoundOBBIsectRay(const xBox* b, const xMat4x3* m, const xRay3* r, xIsect* isect)
{
    xRay3 xfr;
    xBox sbox;
    xVec3 scale;
    xMat4x3 mnormal;

    xVec3NormalizeMacro(&mnormal.right, &m->right, &scale.x);
    xVec3NormalizeMacro(&mnormal.up, &m->up, &scale.y);
    xVec3NormalizeMacro(&mnormal.at, &m->at, &scale.z);
    mnormal.pos = m->pos;

    sbox.upper.x = b->upper.x * scale.x, sbox.upper.y = b->upper.y * scale.y, sbox.upper.z = b->upper.z * scale.z;
    sbox.lower.x = b->lower.x * scale.x, sbox.lower.y = b->lower.y * scale.y, sbox.lower.z = b->lower.z * scale.z;

    xMat4x3Tolocal(&xfr.origin, &mnormal, &r->origin);
    xMat3x3Tolocal(&xfr.dir, &mnormal, &r->dir);
    xfr.min_t = r->min_t;
    xfr.max_t = r->max_t;
    xfr.flags = r->flags;

    iBoxIsectRay(&sbox, &xfr, isect);
}

void xRayHitsBound(const xRay3* r, const xBound* b, xCollis* c)
{
    xIsect isect = { 0 };

    if (b->type == k_XBOUNDTYPE_SPHERE) {
        iSphereIsectRay(&b->sph, r, &isect);
    } else if (b->type == k_XBOUNDTYPE_OBB) {
        xBoundOBBIsectRay(&b->box.box, b->mat, r, &isect);
    } else if (b->type == k_XBOUNDTYPE_BOX) {
        iBoxIsectRay(&b->box.box, r, &isect);
    }

    if (isect.penned <= 0.0f) {
        c->flags |= k_HIT_IT;
        c->dist = isect.dist;
    } else {
        c->flags &= ~k_HIT_IT;
    }
}

void xSphereHitsBound(const xSphere* o, const xBound* b, xCollis* c)
{
    switch (b->type) {
    case k_XBOUNDTYPE_SPHERE:
        xSphereHitsSphere(o, &b->sph, c);
        break;
    case k_XBOUNDTYPE_OBB:
        xBoundSphereHitsOBB(o, &b->box.box, b->mat, c);
        break;
    case k_XBOUNDTYPE_BOX:
        xSphereHitsBox(o, &b->box.box, c);
        break;
    }
}

bool xBoundHitsBound(const xBound& a, const xBound& b)
{
    switch (a.type | (b.type << 3)) {
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_SPHERE << 3):
        return xSphereHitsSphere(a.sph, b.sph);
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_OBB << 3):
        return xSphereHitsOBB(a.sph, b.box.box, *b.mat);
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_BOX << 3):
        return xSphereHitsBox(a.sph, b.box.box);
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_SPHERE << 3):
        return xSphereHitsBox(b.sph, a.box.box);
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_BOX << 3):
        return xBoxHitsBox(a.box.box, b.box.box);
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_OBB << 3):
        return xOBBHitsOBB(a.box.box, g_I3, b.box.box, *b.mat);
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_SPHERE << 3):
        return xSphereHitsOBB(b.sph, a.box.box, *a.mat);
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_OBB << 3):
        return xOBBHitsOBB(a.box.box, *a.mat, b.box.box, *b.mat);
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_BOX << 3):
        return xOBBHitsOBB(a.box.box, *a.mat, b.box.box, g_I3);
    default:
        return false;
    }
}

bool xRayHitsBound(const xRay3& r, const xBound& b)
{
    xIsect isect;
    isect.penned = HUGE;

    switch (b.type) {
    case k_XBOUNDTYPE_SPHERE:
        iSphereIsectRay(&b.sph, &r, &isect);
        break;
    case k_XBOUNDTYPE_BOX:
        iBoxIsectRay(&b.box.box, &r, &isect);
        break;
    case k_XBOUNDTYPE_OBB:
        xBoundOBBIsectRay(&b.box.box, b.mat, &r, &isect);
        break;
    }

    return isect.penned <= 0.0f;
}

bool xSphereHitsBound(const xVec3& c, F32 r, const xBound& b)
{
    switch (b.type) {
    case k_XBOUNDTYPE_SPHERE:
        return xSphereHitsSphere(c, r, b.sph.center, b.sph.r);
    case k_XBOUNDTYPE_BOX:
        return xSphereHitsBox(c, r, b.box.box);
    case k_XBOUNDTYPE_OBB:
        return xSphereHitsOBB(c, r, b.box.box, *b.mat);
    default:
        return false;
    }
}

bool xOBBHitsBound(const xBox& box, const xMat4x3& mat, const xBound& b)
{
    switch (b.type) {
    case k_XBOUNDTYPE_SPHERE:
        return xSphereHitsOBB(b.sph.center, b.sph.r, box, mat);
    case k_XBOUNDTYPE_BOX:
        return xOBBHitsOBB(b.box.box, g_I3, box, mat);
    case k_XBOUNDTYPE_OBB:
        return xOBBHitsOBB(b.box.box, *b.mat, box, mat);
    default:
        return false;
    }
}

void xBoundGetCenter(const xBound* b, xVec3* center)
{
    switch (b->type) {
    case k_XBOUNDTYPE_SPHERE:
        *center = b->sph.center;
        break;
    case k_XBOUNDTYPE_BOX:
    case k_XBOUNDTYPE_OBB:
        *center = b->box.center;
        break;
    case k_XBOUNDTYPE_CYLINDER:
        *center = b->cyl.center;
        break;
    }
}

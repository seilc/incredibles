#include "xCollideFast.h"

#include "iMath3.h"

U32 xRayHitsSphereFast(const xRay3* r, const xSphere* s)
{
    xVec3 diff;

    xVec3Sub(&diff, &r->origin, &s->center);

    F32 c = xVec3Dot(&diff, &diff) - s->r * s->r;

    if (c <= 0.0f)
    {
        return 1;
    }

    if ((r->flags & 0x800) && c > (2.0f * s->r + r->max_t) * r->max_t)
    {
        return 0;
    }

    F32 b = xVec3Dot(&diff, &r->dir);

    if (b >= 0.0f)
    {
        return 0;
    }

    return (b * b >= c);
}

U32 xRayHitsBoxFast(const xRay3* r, const xBox* b)
{
    xIsect isx;

    iBoxIsectRay(b, r, &isx);

    return isx.penned <= 0.0f || isx.contained <= 0.0f;
}

U32 xRayHitsCylinderFast(const xRay3* r, const xCylinder* c)
{
    xIsect isx;

    iCylinderIsectRay(c, r, &isx);

    return isx.penned <= 0.0f || isx.contained <= 0.0f;
}

U32 xRayHitsBoundFast(const xRay3* r, const xBound* b)
{
    if (b->type == k_XBOUNDTYPE_SPHERE)
    {
        return xRayHitsSphereFast(r, &b->sph);
    }
    else if (b->type == k_XBOUNDTYPE_OBB)
    {
        xRay3 lr;
        xMat3x3 mn;
        F32 ms = xVec3Length(&b->mat->right);

        xMat3x3Normalize(&mn, b->mat);
        xMat4x3Tolocal(&lr.origin, b->mat, &r->origin);
        xMat3x3Tolocal(&lr.dir, &mn, &r->dir);

        lr.flags = r->flags;
        lr.max_t = r->max_t / ms;
        lr.min_t = r->min_t / ms;

        return xRayHitsBoxFast(&lr, &b->box.box);
    }
    else if (b->type == k_XBOUNDTYPE_BOX)
    {
        return xRayHitsBoxFast(r, &b->box.box);
    }
    else if (b->type == k_XBOUNDTYPE_CYLINDER)
    {
        return xRayHitsCylinderFast(r, &b->cyl);
    }

    return 0;
}
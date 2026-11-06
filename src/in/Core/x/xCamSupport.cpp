#include "xCamSupport.h"

#include "xBound.h"
#include "xCam.h"
#include "xCollide.h"
#include "xCollideFast.h"
#include "xCollideSweptSphere.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xEnt.h"
#include "xGlobals.h"
#include "xGrid.h"
#include "xMath.h"
#include "xMemMgr.h"
#include "xQuickCull.h"
#include "xScene.h"
#include "xutil.h"
#include "zGrid.h"

#include <string.h>

static const S32 MAX_SUBDIV_SIZE = 16;

#define xMATCH_SIGN(x, s)                                                                         \
    if ((s) < 0.0f) { if ((x) > 0.0f) (x) = -(x); }                                               \
    else { if ((x) < 0.0f) (x) = -(x); }

void xCamSupportPath::create(S32 max_size, S32 debug_max_size)
{
    power = 0;
    for (; max_size > 1; max_size >>= 1)
        power++;
    this->max_size = 1 << power;

    U32 alloc_size = (this->max_size + MAX_SUBDIV_SIZE) * sizeof(path_node);
    path = (path_node*)xMEMALLOC(alloc_size, 0);

    debug_init(debug_max_size);
}

void xCamSupportPath::reset(const xVec3& start_loc)
{
    front = used = 0;
    dist = 0.0f;
    subdist = 0.0f;

    path_node* p = path, *endp = path + (max_size + MAX_SUBDIV_SIZE);
    for (; p != endp; ++p) {
        p->loc = start_loc;
    }
}

void xCamSupportPath::refresh_config()
{
    compute_weights(weights, cfg.smooth_passes, cfg.smooth_factor);
    debug_refresh_config();
}

S32 xCamSupportPath::polar_subdivide(const xCamCoordPolar& start, const xCamCoordPolar& end)
{
    S32 path_end = mod_index(front + used);
    S32 vert_used = polar_subdivide(path + path_end, MAX_SUBDIV_SIZE, start, end, cfg.subdiv_thresh);

    S32 subdiv_end = path_end + vert_used;
    if (subdiv_end > max_size) {
        memcpy(path, path + max_size, (subdiv_end - max_size) * sizeof(path_node));
    }

    used += vert_used;
    if (used > max_size) {
        front = mod_index(subdiv_end);
        used = max_size;
        subdist = 0.0f;
    }

    return vert_used;
}

void xCamSupportPath::prune(S32 start, S32)
{
    S32 old_used = used;

    S32 i0 = mod_index(start + front);
    S32 i1 = i0 + 1, i2 = i0 + 2, end_i2 = i0 + mod_index(front + used - i0);
    while (i2 < end_i2) {
        const path_node& A = path[mod_index(i0)];
        path_node& B = path[mod_index(i1)];
        const path_node& C = path[mod_index(i2)];
        F32 dx = B.loc.x - A.loc.x, dy = B.loc.y - A.loc.y, dz = B.loc.z - A.loc.z;
        F32 ABmag = xsqrt(dx * dx + dy * dy + dz * dz);
        dx = C.loc.x - B.loc.x; dy = C.loc.y - B.loc.y; dz = C.loc.z - B.loc.z;
        F32 BCmag = xsqrt(dx * dx + dy * dy + dz * dz);
        if (ABmag + BCmag <= cfg.min_segment_dist) {
            B = C;
            used--;
            dist = -1.0f;
        } else {
            i0++;
            i1++;
            path[mod_index(i1)] = C;
        }
        i2++;
    }
}

void xCamSupportPath::smooth(S32 start, S32 size)
{
    const F32* w;

    static path_node prev_path[33];

    path_node* pp = prev_path + ARRAY_LENGTH(prev_path) - 1, *end_pp = pp - cfg.smooth_passes - 1;
    S32 r = start + front, end_r = front;
    for (; pp > end_pp; --pp, --r) {
        if (r < end_r)
            pp[0] = pp[1];
        else
            pp[0] = path[mod_index(r)];
    }

    S32 i = start + front, end_i = i + size;
    S32 path_end = front + used;
    for (; i < end_i; i++) {
        S32 j, end_j;
        F32 Rx = 0.0f, Ry = 0.0f, Rz = 0.0f;
        path_node& node = path[mod_index(i)];

        j = i - (start + front);
        prev_path[j & 31] = node;
        end_j = j - (cfg.smooth_passes + 1);
        w = weights;
        for (; j > end_j; --j, ++w) {
            const path_node& B = prev_path[j & 31];
            Rx += *w * B.loc.x;
            Ry += *w * B.loc.y;
            Rz += *w * B.loc.z;
        }

        j = i + 1;
        end_j = j + cfg.smooth_passes;
        w = weights + 1;
        for (; j < end_j; ++j, ++w) {
            const path_node& B = path[mod_index(j < path_end ? j : path_end - 1)];
            Rx += *w * B.loc.x;
            Ry += *w * B.loc.y;
            Rz += *w * B.loc.z;
        }

        node.loc.assign(Rx, Ry, Rz);
    }

    if (size > 0) {
        dist = -1.0f;
    }
}

F32 xCamSupportPath::traverse(xVec3& rloc, F32 velmag, F32 dt)
{
    if (dist < 0.0f) {
        refresh_dist();
    }

    if (used <= 1) {
        subdist = 0.0f;
        return 0.0f;
    }

    S32 i = front, end = i + used - 1;
    for (; i < end; i++) {
        const path_node& p0 = path[mod_index(i)];
        const path_node& p1 = path[mod_index(i + 1)];
        F32 dx = p1.loc.x - p0.loc.x, dy = p1.loc.y - p0.loc.y, dz = p1.loc.z - p0.loc.z;
        F32 segdist = xsqrt(dx * dx + dy * dy + dz * dz);

        if (subdist > segdist) {
            subdist = segdist;
        }

        F32 t = velmag < 0.0001f ? HUGE : (segdist - subdist) / velmag;

        if (t <= dt) {
            dt -= t;
            front = mod_index(front + 1);
            used--;
            subdist = 0.0f;

            if (i + 1 < end) {
                const path_node& p2 = path[mod_index(i + 2)];
                F32 ndx = p2.loc.x - p1.loc.x, ndy = p2.loc.y - p1.loc.y, ndz = p2.loc.z - p1.loc.z;
                F32 next_subdist = xsqrt(ndx * ndx + ndy * ndy + ndz * ndz);

                F32 d = segdist * next_subdist;
                if (d > EPSILON) {
                    F32 damp = (dx * ndx + dy * ndy + dz * ndz) / d;
                    if (damp <= 0.0f)
                        velmag = 0.0f;
                    else if (damp < 1.0f)
                        velmag *= damp;
                }
            }
        }
        else if (segdist < 0.0001f) {
            subdist = 0.0f;
            rloc = p0.loc;
            break;
        } else {
            subdist += dt * velmag;
            if (subdist > segdist) {
                subdist = segdist;
            }

            F32 s = subdist / segdist;
            rloc.x = p0.loc.x * (1.0f - s) + p1.loc.x * s;
            rloc.y = p0.loc.y * (1.0f - s) + p1.loc.y * s;
            rloc.z = p0.loc.z * (1.0f - s) + p1.loc.z * s;
            break;
        }
    }

    return velmag;
}

void xCamSupportPath::refresh_dist()
{
    dist = 0.0f;
    for (S32 i = 0; i < used - 1; i++) {
        const xVec3& v0 = path[mod_index(front + i)].loc;
        const xVec3& v1 = path[mod_index(front + i + 1)].loc;
        F32 dx = v1.x - v0.x, dy = v1.y - v0.y, dz = v1.z - v0.z;
        F32 segdist = xsqrt(dx * dx + dy * dy + dz * dz);

        dist += segdist;
    }
}

S32 xCamSupportPath::polar_subdivide(path_node* vert, S32 vert_size, const xCamCoordPolar& start, const xCamCoordPolar& end, F32 thresh)
{
    path_node head, tail;
    xCam::convert(head.loc, start);
    xCam::convert(tail.loc, end);

    S32 vert_used = 1;
    F32 ABx = tail.loc.x - head.loc.x, ABy = tail.loc.y - head.loc.y, ABz = tail.loc.z - head.loc.z;
    F32 best_total = xsqrt(ABx * ABx + ABy * ABy + ABz * ABz);

    if (vert_size == 1) {
        vert[0] = tail;
        return vert_used;
    }

    vert_used++;

    for (;;) {
        F32 x = start.origin.x, y = start.origin.y, z = start.origin.z;
        F32 theta = start.theta, phi = start.phi, dist = start.dist;
        F32 isubdiv = 1.0f / vert_used;
        F32 dx = isubdiv * (end.origin.x - x);
        F32 dy = isubdiv * (end.origin.y - y);
        F32 dz = isubdiv * (end.origin.z - z);
        F32 dtheta = isubdiv * (xrmod(PI + (end.theta - theta)) - PI);
        F32 dphi = isubdiv * (end.phi - phi);
        F32 ddist = isubdiv * (end.dist - dist);

        F32 total = 0.0f;
        path_node* v = vert, *endv = vert + vert_used - 1;
        for (; v != endv; ++v) {
            x += dx; y += dy; z += dz;
            theta += dtheta; phi += dphi; dist += ddist;
            F32 cp = dist * icos(phi);
            v->loc.x = x + cp * isin(theta);
            v->loc.y = y + dist * isin(phi);
            v->loc.z = z + cp * icos(theta);
            const xVec3& A = v <= vert ? head.loc : v[-1].loc;
            F32 ABx = v->loc.x - A.x, ABy = v->loc.y - A.y, ABz = v->loc.z - A.z;
            total += xsqrt(ABx * ABx + ABy * ABy + ABz * ABz);
        }

        *v = tail;

        const xVec3& A = v[-1].loc;
        F32 ABx = v->loc.x - A.x, ABy = v->loc.y - A.y, ABz = v->loc.z - A.z;
        total += xsqrt(ABx * ABx + ABy * ABy + ABz * ABz);

        if (vert_used == vert_size) {
            return vert_used;
        }

        F32 dtotal = total - best_total;
        if (dtotal <= thresh) {
            return vert_used;
        }

        static xTWEAKABLE F32 increase_factor = 1.0f;
        S32 subdiv_increase = range_limit((S32)(increase_factor * xlog(dtotal / thresh)), 2, 10);

        vert_used *= subdiv_increase;
        if (vert_used > vert_size) {
            vert_used = vert_size;
        }

        best_total = total;
    }
}

void xCamSupportPath::compute_weights(F32* w, S32 n, F32 s)
{
    F32 start_efrac = 1.0f;
    for (S32 i = 0; i <= n; i++) {
        w[i] = 0.0f;
        F32 efrac = start_efrac;
        for (S32 j = i; j <= n; j += 2) {
            F32 spow = xpow(s, j);
            F32 ispow = xpow(1.0f - s, n - j);
            F32 pfs = xBinom(n, j);
            F32 scoeff = spow * (pfs * ispow);
            F32 pfe = xBinom(j, (i + 1) / 2 + j / 2);
            F32 ecoeff = pfe * efrac;
            F32 sw = scoeff * ecoeff;
            w[i] += sw;
            efrac *= 0.25f;
        }
        start_efrac *= 0.5f;
    }

    F32 sum = w[0];
    for (S32 i = 0; i < n; i++)
        sum += w[i + 1] + w[i + 1];
    w[0] += 1.0f - sum;
}

namespace {

namespace losbar_tweak {

xTWEAKABLE bool render_coll_pivot = false;
xTWEAKABLE bool render_coll_compress = false;

}

struct ent_cache_entry
{
    xEnt* ent;
};

static const S32 MAX_ENT_CACHE_SIZE = 128;

ent_cache_entry ent_cache[MAX_ENT_CACHE_SIZE];
S32 ent_cache_size;

struct cache_entity_callback
{
    ent_cache_entry* cache;
    xVec3 center;
    F32 radius;

    cache_entity_callback(const xVec3& center, F32 radius) : cache(ent_cache), center(center), radius(radius) {}

    bool operator()(xEnt& ent)
    {
        if (!ent.camcollModel) {
            return true;
        }

        if (!(ent.chkby & 0x1)) {
            if (!(ent.chkby & 0x10)) {
                return true;
            }
            if (!xEntIsVisible(&ent)) {
                return true;
            }
        }

        if (!xSphereHitsBound(center, radius, ent.bound)) {
            return true;
        }

        if (ent_cache_size >= MAX_ENT_CACHE_SIZE)
            return false;

        cache[ent_cache_size].ent = &ent;
        ent_cache_size++;

        return true;
    }
};

// UNUSED: speculative
bool pivot_depen_bar(xVec3& cap_offset, xVec3& impact, xVec3& rot_axis, const xCapsule& cap, F32 impact_t, F32, const xVec3& tohit)
{
    cap_offset.assign(cap.end.x - cap.start.x, cap.end.y - cap.start.y, cap.end.z - cap.start.z);
    impact.assign(cap.start.x + cap_offset.x * impact_t + tohit.x, cap.start.y + cap_offset.y * impact_t + tohit.y, cap.start.z + cap_offset.z * impact_t + tohit.z);
    xVec3 impact_offset = { impact.x - cap.start.x, impact.y - cap.start.y, impact.z - cap.start.z };

    F32 R = cap.r;
    F32 G = impact_offset.length();
    if (G <= R) {
        return false;
    }

    F32 touch_angle = xasin(R / G);
    F32 X0 = xsqrt(G * G - R * R);

    rot_axis.cross(cap_offset, impact_offset);
    F32 crossmag = rot_axis.length();
    rot_axis /= crossmag;
    F32 dotmag = cap_offset.dot(impact_offset) / (X0 * cap_offset.length());
    F32 impact_angle = xacos(dotmag);
    F32 angle = impact_angle - touch_angle;

    xMat3x3 rot_mat;
    xMat3x3Rot(&rot_mat, &rot_axis, angle);
    xMat3x3RMulVec(&cap_offset, &rot_mat, &cap_offset);

    return true;
}

F32 arc_sphere_to_plane(const xMat4x3& circle_mat, F32 Rc, F32 Rs, const xVec3& Nplane, F32 Dplane)
{
    xVec3 N;
    xVec3 point_on_plane = { Nplane.x * Dplane, Nplane.y * Dplane, Nplane.z * Dplane };
    xMat3x3LMulVec(&N, &circle_mat, &Nplane);
    xMat4x3Tolocal(&point_on_plane, &circle_mat, &point_on_plane);
    F32 D = Rs + N.dot(point_on_plane);

    F32 D_Rc = D / Rc;
    F32 Nx2_Ny2 = N.x * N.x + N.y * N.y;

    F32 det = Nx2_Ny2 - D_Rc * D_Rc;
    if (det < 0.0f)
        return 0.0f;
    det = xsqrt(det);
    F32 cost = (N.x * D_Rc + N.y * det) / Nx2_Ny2;
    F32 t = xacos(cost);

    return t;
}

inline bool ray_hits_bound(const xRay3& ray, const xBound& bound, F32 expand)
{
    switch (bound.type) {
    case k_XBOUNDTYPE_SPHERE: {
        const F32 oldr = bound.sph.r;
        ((xBound&)bound).sph.r += expand;
        bool result = xRayHitsSphereFast(&ray, &bound.sph) != 0;
        ((xBound&)bound).sph.r = oldr;
        return result;
    }
    case k_XBOUNDTYPE_BOX: {
        xBox box;
        box.upper.x = bound.box.box.upper.x + expand;
        box.upper.y = bound.box.box.upper.y + expand;
        box.upper.z = bound.box.box.upper.z + expand;
        box.lower.x = bound.box.box.lower.x - expand;
        box.lower.y = bound.box.box.lower.y - expand;
        box.lower.z = bound.box.box.lower.z - expand;
        return xRayHitsBoxFast(&ray, &box) != 0;
    }
    case k_XBOUNDTYPE_OBB: {
        xMat3x3 mat;
        xRay3 local_ray;
        F32 scale = bound.mat->right.length();
        xMat3x3Normalize(&mat, bound.mat);
        xMat4x3Tolocal(&local_ray.origin, bound.mat, &ray.origin);
        xMat3x3Tolocal(&local_ray.dir, &mat, &ray.dir);
        F32 iscale = 1.0f / scale;
        local_ray.max_t = ray.max_t * iscale;
        local_ray.min_t = ray.min_t * iscale;
        local_ray.flags = ray.flags;
        expand *= iscale;
        xBox box;
        box.upper.x = bound.box.box.upper.x + expand;
        box.upper.y = bound.box.box.upper.y + expand;
        box.upper.z = bound.box.box.upper.z + expand;
        box.lower.x = bound.box.box.lower.x - expand;
        box.lower.y = bound.box.box.lower.y - expand;
        box.lower.z = bound.box.box.lower.z - expand;
        return xRayHitsBoxFast(&local_ray, &box) != 0;
    }
    }

    return false;
}

bool capsule_hits_scene(const xCapsule& cap, xCollis& coll)
{
    coll.flags = 0;
    xCapsuleIsectsEnvClosestToLine(&cap, ((xScene*)xglobals->sceneCur)->env, &coll);
    if (coll.dist < 0.001f) {
        return true;
    }

    xBox box; xQCData qcd;
    xBoxFromCapsule(box, cap);
    xQuickCullForBox(&qcd, &box);

    xRay3 ray;
    ray.origin = cap.start;
    ray.dir.assign(cap.end.x - cap.start.x, cap.end.y - cap.start.y, cap.end.z - cap.start.z);
    ray.min_t = 0.0f;
    ray.max_t = ray.dir.length();
    ray.dir /= ray.max_t;
    ray.flags = XRAY3_USE_MIN | XRAY3_USE_MAX;

    ent_cache_entry* ite = ent_cache, *end_ite = ite + ent_cache_size;
    for (; ite != end_ite; ++ite) {
        xEnt& ent = *ite->ent;
        if (!xQuickCullIsects(&qcd, &ent.bound.qcd))
            continue;
        if (!ray_hits_bound(ray, ent.bound, cap.r)) {
            continue;
        }

        xCollis temp_coll;
        temp_coll.flags = 0;
        if (!xCapsuleIsectsModelClosestToLine(&cap, ent.camcollModel, &temp_coll)) {
            continue;
        }

        if (temp_coll.dist < coll.dist) {
            coll = temp_coll;
        }

        if (coll.dist < 0.001f)
            return true;
    }

    return coll.dist < cap.r;
}

bool swept_sphere_hits_scene(xSweptSphere& sws, const xVec3& start, const xVec3& end, F32 radius)
{
    xSweptSpherePrepare(&sws, &start, &end, radius);

    xSweptSphereToEnv(&sws, ((xScene*)xglobals->sceneCur)->env);
    if (sws.curdist <= 0.0f) {
        return true;
    }

    xBox box; xQCData qcd;
    xBoxFromCapsule(box, start, end, radius);
    xQuickCullForBox(&qcd, &box);

    xRay3 ray;
    ray.origin = start;
    ray.dir.assign(end.x - start.x, end.y - start.y, end.z - start.z);
    ray.min_t = 0.0f;
    ray.max_t = ray.dir.length();
    ray.dir /= ray.max_t;
    ray.flags = XRAY3_USE_MIN | XRAY3_USE_MAX;

    ent_cache_entry* ite = ent_cache, *end_ite = ite + ent_cache_size;
    for (; ite != end_ite; ++ite) {
        xEnt& ent = *ite->ent;
        if (!xQuickCullIsects(&qcd, &ent.bound.qcd))
            continue;
        if (!ray_hits_bound(ray, ent.bound, radius)) {
            continue;
        }

        xModelInstance& model = *ent.camcollModel;
        if (!xSweptSphereToModel(&sws, model.Data, model.Mat)) {
            continue;
        }

        if (sws.curdist <= 0.0f)
            return true;
    }

    return sws.curdist < sws.dist;
}

bool sphere_hits_scene(const xSphere& o, xCollis& coll)
{
    coll.dist = HUGE;
    coll.flags |= 0x100;
    xSphereHitsEnv(&o, ((xScene*)xglobals->sceneCur)->env, &coll);
    if (coll.dist <= 0.0f) {
        return true;
    }

    xQCData qcd;
    xQuickCullForSphere(&qcd, &o);

    ent_cache_entry* ite = ent_cache, *end_ite = ite + ent_cache_size;
    for (; ite != end_ite; ++ite) {
        xEnt& ent = *ite->ent;
        if (!xQuickCullIsects(&qcd, &ent.bound.qcd))
            continue;
        if (!xSphereHitsBound(o, ent.bound)) {
            continue;
        }

        xCollis temp_coll;
        temp_coll.flags = coll.flags;
        if (!xSphereHitsModel(&o, ent.camcollModel, &temp_coll)) {
            continue;
        }

        if (temp_coll.dist < coll.dist) {
            coll = temp_coll;
        }

        if (coll.dist <= 0.0f)
            return true;
    }

    return coll.dist < o.r;
}

}

void xCamSupportLOSBar::reset(const xVec3& origin, const xVec3& stern_loc)
{
    rail_yoffset = cfg.rail_ymin;
    this->origin = origin;
    this->stern_loc = stern_loc;
}

void xCamSupportLOSBar::begin_update(const xVec3& origin)
{
    last_origin = this->origin;
    last_rail_yoffset = rail_yoffset;
    last_stern_loc = stern_loc;
    this->origin = origin;

    F32 dy = origin.y - last_origin.y;
    rail_yoffset = range_limit(rail_yoffset - dy, cfg.rail_ymin, cfg.rail_ymax);
}

void xCamSupportLOSBar::precollide()
{
    F32 dx = stern_loc.x - origin.x, dz = stern_loc.z - origin.z;
    F32 xzdist2 = dx * dx + dz * dz;
    F32 dylower = stern_loc.y - (origin.y + cfg.rail_ymin);
    F32 dyupper = stern_loc.y - (origin.y + cfg.rail_ymax);
    F32 rlower = xsqrt(xzdist2 + dylower * dylower);
    F32 rupper = xsqrt(xzdist2 + dyupper * dyupper);
    F32 ylower = origin.y + cfg.rail_ymin - rlower;
    F32 yupper = origin.y + cfg.rail_ymax + rupper;
    F32 ycenter = 0.5f * (ylower + yupper);

    xBound bound;
    bound.type = k_XBOUNDTYPE_SPHERE;
    bound.sph.center.assign(origin.x, ycenter, origin.z);
    bound.sph.r = cfg.pivot_bar_radius + (yupper - ycenter);
    xQuickCullForBound(&bound.qcd, &bound);

    ent_cache_size = 0;
    cache_entity_callback cb(bound.sph.center, bound.sph.r);
    xGridCheckBound(colls_grid, bound, bound.qcd, cb);
    xGridCheckBound(colls_oso_grid, bound, bound.qcd, cb);
}

bool xCamSupportLOSBar::collide_pivot()
{
    xCollis coll;
    coll.flags = 0;

    xCapsule to_cap;
    to_cap.start.assign(origin.x, origin.y + rail_yoffset, origin.z);
    to_cap.end = stern_loc;
    to_cap.r = cfg.pivot_bar_radius;
    if (losbar_tweak::render_coll_pivot)
        xDrawSetColor(g_WHITE);

    if (!capsule_hits_scene(to_cap, coll)) {
        return false;
    }

    F32 to_t = coll.tuv.t, to_dist = coll.dist;
    xVec3 to_tohit = coll.tohit;
    xVec3 to_heading = coll.hdng;
    xVec3 to_impact = {
        to_cap.start.x * (1.0f - to_t) + (to_cap.end.x * to_t + to_tohit.x),
        to_cap.start.y * (1.0f - to_t) + (to_cap.end.y * to_t + to_tohit.y),
        to_cap.start.z * (1.0f - to_t) + (to_cap.end.z * to_t + to_tohit.z)
    };

    if (to_t < 1.0f) {
        F32 new_rail_yoffset;
        xCapsule new_cap = to_cap;

        if (xabs(to_tohit.y) <= 0.001f) {
            if (xabs(rail_yoffset - cfg.rail_ymin) > xabs(rail_yoffset - cfg.rail_ymax))
                new_rail_yoffset = cfg.rail_ymin;
            else
                new_rail_yoffset = cfg.rail_ymax;
            new_cap.start.assign(origin.x, origin.y + new_rail_yoffset, origin.z);
        } else {
            xVec3 N = to_tohit;
            N.normalize(); N.negate();
            F32 D = N.dot(to_impact);

            xMat4x3 vcircle_mat;
            vcircle_mat.right = to_cap.start;
            vcircle_mat.right -= to_cap.end;
            F32 bar_mag = vcircle_mat.right.length();
            vcircle_mat.right /= bar_mag;
            vcircle_mat.up.assign(0.0f, 1.0f, 0.0f);
            xMATCH_SIGN(vcircle_mat.up.y, N.y);
            vcircle_mat.at.cross(vcircle_mat.right, vcircle_mat.up);
            vcircle_mat.at.normalize();
            vcircle_mat.up.cross(vcircle_mat.at, vcircle_mat.right);
            vcircle_mat.pos = to_cap.end;

            F32 Rc = bar_mag * (1.0f - to_t);
            F32 vrot = arc_sphere_to_plane(vcircle_mat, Rc, cfg.pivot_bar_radius, N, D);

            xMATCH_SIGN(vrot, vcircle_mat.up.y);

            F32 oldpitch = xasin(vcircle_mat.right.y);
            F32 newpitch = range_limit(oldpitch + vrot, -89.0f * (PI/180.0f), 89.0f * (PI/180.0f));

            F32 dx = to_cap.start.x - to_cap.end.x, dz = to_cap.start.z - to_cap.end.z;
            F32 xzdist = xsqrt(dx * dx + dz * dz);
            F32 newy = to_cap.end.y + xzdist * itan(newpitch);

            new_cap.start.y = range_limit(newy, origin.y + cfg.rail_ymin, origin.y + cfg.rail_ymax);

            new_rail_yoffset = new_cap.start.y - origin.y;
        }

        if (!capsule_hits_scene(new_cap, coll)) {
            rail_yoffset = new_rail_yoffset;
            return false;
        }

        if (coll.dist > to_dist + EPSILON) {
            to_cap = new_cap;
            to_t = coll.tuv.t;
            to_dist = coll.dist;
            to_tohit = coll.tohit;
            rail_yoffset = new_rail_yoffset;
        }
    }

    xCapsule from_cap;
    F32 from_t, from_dist;
    xVec3 from_tohit;

    if (to_dist >= 0.001f) {
        from_cap = to_cap;
        from_t = to_t;
        from_dist = to_dist;
        from_tohit = to_tohit;
    } else {
        from_cap.start = last_origin;
        from_cap.start.y += last_rail_yoffset;
        from_cap.end = last_stern_loc;
        from_cap.r = cfg.pivot_bar_radius;

        if (capsule_hits_scene(from_cap, coll)) {
            if (coll.dist < 0.001f) {
                return false;
            }
            from_t = coll.tuv.t;
            from_dist = coll.dist;
            from_tohit = coll.tohit;
        } else {
            F32 start_s = 0.0f, end_s = 1.0f;
            S32 tries = 0;
            for (; tries < 8; tries++) {
                F32 s = 0.5f * (start_s + end_s);
                xCapsule cap;
                cap.start = from_cap.start * (1.0f - s) + to_cap.start * s;
                cap.end = from_cap.end * (1.0f - s) + to_cap.end * s;
                cap.r = cfg.pivot_bar_radius;
                if (!capsule_hits_scene(cap, coll))
                    start_s = s;
                else if (coll.dist < 0.001f)
                    end_s = s;
                else {
                    from_cap = cap;
                    from_t = coll.tuv.t;
                    from_dist = coll.dist;
                    from_tohit = coll.tohit;
                    break;
                }
            }

            if (tries >= 8) {
                return false;
            }
        }
    }

    xVec3 cap_offset;
    xVec3 impact = {
        from_cap.start.x * (1.0f - from_t) + (from_cap.end.x * from_t + from_tohit.x),
        from_cap.start.y * (1.0f - from_t) + (from_cap.end.y * from_t + from_tohit.y),
        from_cap.start.z * (1.0f - from_t) + (from_cap.end.z * from_t + from_tohit.z)
    };

    xVec3 N = from_tohit;
    N.normalize(); N.negate();
    F32 D = N.dot(impact);

    cap_offset.assign(from_cap.end.x - from_cap.start.x, from_cap.end.y - from_cap.start.y, from_cap.end.z - from_cap.start.z);
    F32 min_from_t = (cfg.pivot_bar_radius + 0.01f) / cap_offset.length();

    if (from_t >= 1.0f) {
        const xVec3& C = origin;
        xVec3 P = from_cap.end;
        xVec3 CP = { P.x - C.x, P.y - C.y, P.z - C.z };
        xVec3 uCP = CP;
        F32 Rc = uCP.length();
        uCP /= Rc;

        xMat4x3 circle_mat;
        circle_mat.at.cross(from_tohit, CP);
        circle_mat.at.normalize();
        circle_mat.right = uCP;
        circle_mat.up.cross(circle_mat.at, circle_mat.right);
        circle_mat.pos = C;
        F32 max_rot = arc_sphere_to_plane(circle_mat, Rc, cfg.pivot_bar_radius, N, D);

        if (losbar_tweak::render_coll_pivot) {
            xDrawSetColor(g_YELLOW);
            xDrawArrow(&impact, &N, false, 1.0f);
        }

        xMat3x3 rot_mat;
        xMat3x3Rot(&rot_mat, &circle_mat.at, max_rot);
        xMat3x3RMulVec(&cap_offset, &rot_mat, &CP);

        stern_loc = C;
        stern_loc += cap_offset;
    }
    else if (from_t >= min_from_t) {
        cap_offset.x = from_cap.end.x - from_cap.start.x;
        cap_offset.y = from_cap.end.y - from_cap.start.y;
        cap_offset.z = from_cap.end.z - from_cap.start.z;

        if (losbar_tweak::render_coll_pivot) {
            xVec3 tohit0, tohit1;
            xDrawSetColor(g_YELLOW);
            xDrawSetColor(g_RED);
            xDrawSphere(&impact, 0.05f, 0xC0006);
            tohit0 = from_cap.start * (1.0f - from_t) + from_cap.end * from_t;
            tohit1 = tohit0 + from_tohit;
            xDrawLine(&tohit0, &tohit1);
        }

        F32 depen_mul = 1.0f - from_cap.r / from_dist;
        xVec3 depen = from_tohit * depen_mul;
        from_cap.start += depen;
        from_cap.end += depen;

        const xVec3& P = impact, &A = from_cap.start, &B = to_cap.start;
        xVec3 AP = { P.x - A.x, P.y - A.y, P.z - A.z };
        xVec3 BP = { P.x - B.x, P.y - B.y, P.z - B.z };
        xVec3 rot_axis;
        rot_axis.cross(from_tohit, BP);
        rot_axis.normalize();
        F32 R = cfg.pivot_bar_radius;
        F32 R2 = R * R;
        xVec3 PV; PV.cross(BP, rot_axis);
        F32 det = R2 / (BP.length2() - R2);
        PV *= -xsqrt(det);
        xVec3 V = { P.x + PV.x, P.y + PV.y, P.z + PV.z };
        xVec3 BV = { V.x - B.x, V.y - B.y, V.z - B.z };
        F32 offset_mag = cap_offset.length();
        cap_offset = BV;
        cap_offset *= offset_mag / cap_offset.length();

        stern_loc = to_cap.start;
        stern_loc += cap_offset;
    } else {
        return false;
    }

    return true;
}

bool xCamSupportLOSBar::collide_compress()
{
    xSweptSphere sws;
    xVec3 start, end, dir;

    xSphere o;
    o.center = stern_loc;
    o.r = cfg.compress_ball_radius;

    xCollis coll;
    coll.flags = 0x800;
    sphere_hits_scene(o, coll);
    if (losbar_tweak::render_coll_compress && (coll.flags & k_HIT_IT)) {
        xDrawSphere(&o, 0xC0006);
    }

    start.assign(origin.x, origin.y + rail_yoffset, origin.z);
    end = stern_loc;
    dir.assign(end.x - start.x, end.y - start.y, end.z - start.z);
    dir.normalize();
    start.x += dir.x * cfg.compress_dist_min;
    start.y += dir.y * cfg.compress_dist_min;
    start.z += dir.z * cfg.compress_dist_min;
    swept_sphere_hits_scene(sws, start, end, cfg.compress_bar_radius);
    F32 ball_dist = (coll.flags & k_HIT_IT) ? sws.dist + dir.dot(coll.depen) : HUGE;

    if (sws.curdist >= sws.dist && ball_dist >= sws.dist)
        return false;

    F32 dist = sws.curdist;
    if (dist > ball_dist) {
        dist = ball_dist;
    }

    dist -= cfg.pivot_bar_radius - cfg.compress_ball_radius;
    if (dist < 0.0f) {
        dist = 0.0f;
    }

    xVec3AddScaled(stern_loc, start, sws.dir, dist);

    return true;
}

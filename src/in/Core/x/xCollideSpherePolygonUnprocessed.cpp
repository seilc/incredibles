#include "xCollideSpherePolygonUnprocessed.h"

namespace xCollideSpherePolygonUnprocessed {
static F32 normalize(xVec3& v)
{
    F32 len2 = v.length2();
    if (len2 > 0.000001f) {
        F32 length = xsqrt(len2);
        F32 scale = 1.0f / length;
        xVec3ScaleC(v, scale);
        return length;
    }

    return 0.0f;
}
}

static bool ComputeSurface(xPlane& surface, const xVec3& pt0, const xVec3& pt1, const xVec3& pt2)
{
    xVec3 edge0, edge1;
    xVec3Sub(edge0, pt1, pt0);
    xVec3Sub(edge1, pt2, pt1);
    xVec3Cross(surface.norm, edge1, edge0);
    if (xCollideSpherePolygonUnprocessed::normalize(surface.norm) == 0.0f) {
        return false;
    }

    surface.Init(pt0);
    return true;
}

static bool ComputeEdge(xVec3& dir, F32& len, xPlane& plane, const xPlane& surface, const xVec3& pt0, const xVec3 pt1)
{
    xVec3Sub(dir, pt1, pt0);
    len = xCollideSpherePolygonUnprocessed::normalize(dir);
    if (len == 0.0f) {
        return false;
    }

    xVec3Cross(plane.norm, surface.norm, dir);
    plane.Init(pt0);
    return true;
}

static bool TriangleTestSphereToThreePoints(const xSweptSphere& sphere, xCollideSphereToPolygonResults& results, const xVec3& pt0, const xVec3& pt1, const xVec3& pt2);
static bool TriangleTestSphereToTwoPoints(const xSweptSphere& sphere, xCollideSphereToPolygonResults& results, const xVec3& pt0, const xVec3& pt1);

#define TEST_EDGE_PLANE(plane) \
    above_edge_vel = (plane).norm.dot(sphere.dir); \
    above_edge_de = above_edge_vel * sphere.curdist + above_edge_d0; \
    if (above_edge_de >= sphere.radius && above_edge_d0 >= sphere.radius) { \
        return false; \
    }

#define TEST_EDGE_SEGMENT(pt, dir, len) \
    if (xSweptSphereToLineSement(sphere, (pt), (dir), (len), edge_collide) == true) { \
        results.dist = edge_collide.dist; \
        results.contact = edge_collide.contact; \
        results.type = exCOLLIDESPHEREHITTYPE_EDGE; \
        return true; \
    }

#define TEST_EDGE_INTERIOR(pt, dir, len) \
    if (above_edge_d0 < sphere.radius) { \
        xVec3 to_sphere; \
        xVec3Sub(to_sphere, sphere.start, (pt)); \
        F32 along_edge_d0 = to_sphere.dot((dir)); \
        if (along_edge_d0 >= 0.0f && along_edge_d0 <= (len)) { \
            return false; \
        } \
    }

bool TriangleTestSphereOnPlaneUnprocessed(const xPlane& surface, const xVec3& p0, const xVec3& p1, const xVec3& p2, const xSweptSphere& sphere, xCollideSphereToPolygonResults& results, F32 vertical_d0)
{
    xVec3 edge0dir, edge1dir, edge2dir;
    F32 edge0len, edge1len, edge2len;
    xPlane edge0plane, edge1plane, edge2plane;
    F32 above_edge_d0, above_edge_vel, above_edge_de;
    xCollideSphereToLineSegmentResults edge_collide;

    if (!ComputeEdge(edge0dir, edge0len, edge0plane, surface, p0, p1)) {
        return false;
    }

    above_edge_d0 = edge0plane.Dist(sphere.start);
    if (above_edge_d0 > 0.0f) {
        TEST_EDGE_PLANE(edge0plane);
        TEST_EDGE_SEGMENT(p0, edge0dir, edge0len);
        TEST_EDGE_INTERIOR(p0, edge0dir, edge0len);

        if (!ComputeEdge(edge1dir, edge1len, edge1plane, surface, p1, p2)) {
            return false;
        }

        above_edge_d0 = edge1plane.Dist(sphere.start);
        if (above_edge_d0 > 0.0f) {
            TEST_EDGE_PLANE(edge1plane);
            TEST_EDGE_SEGMENT(p1, edge1dir, edge1len);
            TEST_EDGE_INTERIOR(p1, edge1dir, edge1len);

            return TriangleTestSphereToThreePoints(sphere, results, p0, p1, p2);
        }

        if (!ComputeEdge(edge2dir, edge2len, edge2plane, surface, p2, p0)) {
            return false;
        }

        above_edge_d0 = edge2plane.Dist(sphere.start);
        if (above_edge_d0 > 0.0f) {
            TEST_EDGE_PLANE(edge2plane);
            TEST_EDGE_SEGMENT(p2, edge2dir, edge2len);
            TEST_EDGE_INTERIOR(p2, edge2dir, edge2len);

            return TriangleTestSphereToThreePoints(sphere, results, p0, p1, p2);
        }

        return TriangleTestSphereToTwoPoints(sphere, results, p0, p1);
    }

    if (!ComputeEdge(edge1dir, edge1len, edge1plane, surface, p1, p2)) {
        return false;
    }

    above_edge_d0 = edge1plane.Dist(sphere.start);
    if (above_edge_d0 > 0.0f) {
        TEST_EDGE_PLANE(edge1plane);
        TEST_EDGE_SEGMENT(p1, edge1dir, edge1len);
        TEST_EDGE_INTERIOR(p1, edge1dir, edge1len);

        if (!ComputeEdge(edge2dir, edge2len, edge2plane, surface, p2, p0)) {
            return false;
        }

        above_edge_d0 = edge2plane.Dist(sphere.start);
        if (above_edge_d0 > 0.0f) {
            TEST_EDGE_PLANE(edge2plane);
            TEST_EDGE_SEGMENT(p2, edge2dir, edge2len);
            TEST_EDGE_INTERIOR(p2, edge2dir, edge2len);

            return TriangleTestSphereToThreePoints(sphere, results, p0, p1, p2);
        }

        return TriangleTestSphereToTwoPoints(sphere, results, p1, p2);
    }

    if (!ComputeEdge(edge2dir, edge2len, edge2plane, surface, p2, p0)) {
        return false;
    }

    above_edge_d0 = edge2plane.Dist(sphere.start);
    if (above_edge_d0 > 0.0f) {
        TEST_EDGE_PLANE(edge2plane);
        TEST_EDGE_SEGMENT(p2, edge2dir, edge2len);
        TEST_EDGE_INTERIOR(p2, edge2dir, edge2len);

        return TriangleTestSphereToTwoPoints(sphere, results, p0, p2);
    }

    results.dist = 0.0f;
    surface.Project(results.contact, sphere.start, vertical_d0);
    results.type = exCOLLIDESPHEREHITTYPE_INTERIOR;
    return true;
}

bool xSweptSphereToTriangleDoubleSidedUnprocessed(const xSweptSphere& sphere, const xVec3& pt0, const xVec3& pt1, const xVec3& pt2, xCollideSphereToPolygonResults& results, xPlane& surface)
{
    if (!ComputeSurface(surface, pt0, pt1, pt2)) {
        return false;
    }

    F32 vertical_vel = sphere.dir.dot(surface.norm);
    if (vertical_vel < -0.005f) {
        F32 vertical_d0 = surface.Dist(sphere.start);
        if (vertical_d0 < 0.0f) {
            return false;
        }

        F32 vertical_de = vertical_vel * sphere.curdist + vertical_d0;
        if (vertical_de >= sphere.radius) {
            return false;
        }

        if (vertical_d0 <= sphere.radius) {
            return TriangleTestSphereOnPlaneUnprocessed(surface, pt0, pt1, pt2, sphere, results, vertical_d0);
        }

        F32 percent = (vertical_d0 - sphere.radius) / (vertical_d0 - vertical_de);
        F32 to_plane_move_dist = sphere.curdist * percent;

        xSweptSphere at_plane_sphere;
        xVec3AddScaled(at_plane_sphere.start, sphere.start, sphere.dir, to_plane_move_dist);
        at_plane_sphere.curdist = sphere.curdist - to_plane_move_dist;
        at_plane_sphere.dir = sphere.dir;
        at_plane_sphere.radius = sphere.radius;
        if (TriangleTestSphereOnPlaneUnprocessed(surface, pt0, pt1, pt2, at_plane_sphere, results, sphere.radius) == true) {
            results.dist += to_plane_move_dist;
            return true;
        }

        return false;
    } else if (vertical_vel > 0.005f) {
        F32 vertical_d0 = surface.Dist(sphere.start);
        if (vertical_d0 > 0.0f) {
            return false;
        }

        F32 vertical_de = vertical_vel * sphere.curdist + vertical_d0;
        if (vertical_de <= -sphere.radius) {
            return false;
        }

        if (vertical_d0 >= -sphere.radius) {
            return TriangleTestSphereOnPlaneUnprocessed(surface, pt0, pt1, pt2, sphere, results, vertical_d0);
        }

        F32 percent = (-sphere.radius - vertical_d0) / (vertical_de - vertical_d0);
        F32 to_plane_move_dist = sphere.curdist * percent;

        xSweptSphere at_plane_sphere;
        xVec3AddScaled(at_plane_sphere.start, sphere.start, sphere.dir, to_plane_move_dist);
        at_plane_sphere.curdist = sphere.curdist - to_plane_move_dist;
        at_plane_sphere.dir = sphere.dir;
        at_plane_sphere.radius = sphere.radius;
        if (TriangleTestSphereOnPlaneUnprocessed(surface, pt0, pt1, pt2, at_plane_sphere, results, -sphere.radius) == true) {
            results.dist += to_plane_move_dist;
            return true;
        }

        return false;
    }

    return false;
}

bool xSweptSphereToTriangleUnprocessed(const xSweptSphere& sphere, const xVec3& pt0, const xVec3& pt1, const xVec3& pt2, xCollideSphereToPolygonResults& results, xPlane& surface)
{
    if (!ComputeSurface(surface, pt0, pt1, pt2)) {
        return false;
    }

    F32 vertical_vel = sphere.dir.dot(surface.norm);
    if (vertical_vel >= -0.005f) {
        return false;
    }

    F32 vertical_d0 = surface.Dist(sphere.start);
    if (vertical_d0 < 0.0f) {
        return false;
    }

    F32 vertical_de = vertical_vel * sphere.curdist + vertical_d0;
    if (vertical_de >= sphere.radius) {
        return false;
    }

    if (vertical_d0 > sphere.radius) {
        F32 percent = (vertical_d0 - sphere.radius) / (vertical_d0 - vertical_de);
        F32 to_plane_move_dist = sphere.curdist * percent;

        xSweptSphere at_plane_sphere;
        xVec3AddScaled(at_plane_sphere.start, sphere.start, sphere.dir, to_plane_move_dist);
        at_plane_sphere.curdist = sphere.curdist - to_plane_move_dist;
        at_plane_sphere.dir = sphere.dir;
        at_plane_sphere.radius = sphere.radius;
        if (TriangleTestSphereOnPlaneUnprocessed(surface, pt0, pt1, pt2, at_plane_sphere, results, sphere.radius) == true) {
            results.dist += to_plane_move_dist;
            return true;
        }

        return false;
    }

    return TriangleTestSphereOnPlaneUnprocessed(surface, pt0, pt1, pt2, sphere, results, vertical_d0);
}

#define SET_VERTEX_RESULT(collide, pt) \
    results.dist = (collide).dist; \
    results.contact = (pt); \
    results.type = exCOLLIDESPHEREHITTYPE_VERTEX

#define CALC_DIST2(dist, pt) \
    xVec3Sub(to_pt, (pt), sphere.start); \
    (dist) = to_pt.length2()

static bool TriangleTestSphereToThreePoints(const xSweptSphere& sphere, xCollideSphereToPolygonResults& results, const xVec3& pt0, const xVec3& pt1, const xVec3& pt2)
{
    xCollideSphereToPointResults vertex_collide0, vertex_collide1, vertex_collide2;
    xVec3 to_pt;
    F32 dist_to_pt0, dist_to_pt1, dist_to_pt2;

    if (xSweptSphereToPoint(sphere, pt0, vertex_collide0) == true) {
        if (xSweptSphereToPoint(sphere, pt1, vertex_collide1) == true) {
            if (xSweptSphereToPoint(sphere, pt2, vertex_collide2) == true) {
                CALC_DIST2(dist_to_pt0, pt0);
                CALC_DIST2(dist_to_pt1, pt1);
                CALC_DIST2(dist_to_pt2, pt2);

                if (dist_to_pt0 < dist_to_pt1) {
                    if (dist_to_pt1 < dist_to_pt2) {
                        SET_VERTEX_RESULT(vertex_collide0, pt0);
                        return true;
                    }

                    SET_VERTEX_RESULT(vertex_collide2, pt2);
                    return true;
                }

                if (dist_to_pt1 < dist_to_pt2) {
                    SET_VERTEX_RESULT(vertex_collide1, pt1);
                    return true;
                }

                SET_VERTEX_RESULT(vertex_collide2, pt2);
                return true;
            }
        }

        if (xSweptSphereToPoint(sphere, pt2, vertex_collide2) == true) {
            CALC_DIST2(dist_to_pt0, pt0);
            CALC_DIST2(dist_to_pt2, pt2);

            if (dist_to_pt0 < dist_to_pt2) {
                SET_VERTEX_RESULT(vertex_collide0, pt0);
                return true;
            }

            SET_VERTEX_RESULT(vertex_collide2, pt2);
            return true;
        }

        SET_VERTEX_RESULT(vertex_collide0, pt0);
        return true;
    }

    if (xSweptSphereToPoint(sphere, pt1, vertex_collide1) == true) {
        if (xSweptSphereToPoint(sphere, pt2, vertex_collide2) == true) {
            CALC_DIST2(dist_to_pt1, pt1);
            CALC_DIST2(dist_to_pt2, pt2);

            if (dist_to_pt1 < dist_to_pt2) {
                SET_VERTEX_RESULT(vertex_collide1, pt1);
                return true;
            }

            SET_VERTEX_RESULT(vertex_collide2, pt2);
            return true;
        }

        SET_VERTEX_RESULT(vertex_collide1, pt1);
        return true;
    }

    if (xSweptSphereToPoint(sphere, pt2, vertex_collide2) == true) {
        SET_VERTEX_RESULT(vertex_collide2, pt2);
        return true;
    }

    return false;
}

static bool TriangleTestSphereToTwoPoints(const xSweptSphere& sphere, xCollideSphereToPolygonResults& results, const xVec3& pt0, const xVec3& pt1)
{
    xCollideSphereToPointResults vertex_collide0, vertex_collide1;

    if (xSweptSphereToPoint(sphere, pt0, vertex_collide0) == true) {
        if (xSweptSphereToPoint(sphere, pt1, vertex_collide1) == true) {
            xVec3 to_pt;
            F32 dist_to_pt0, dist_to_pt1;
            CALC_DIST2(dist_to_pt0, pt0);
            CALC_DIST2(dist_to_pt1, pt1);

            if (dist_to_pt0 < dist_to_pt1) {
                SET_VERTEX_RESULT(vertex_collide0, pt0);
                return true;
            }

            SET_VERTEX_RESULT(vertex_collide1, pt1);
            return true;
        }

        SET_VERTEX_RESULT(vertex_collide0, pt0);
        return true;
    }

    if (xSweptSphereToPoint(sphere, pt1, vertex_collide1) == true) {
        SET_VERTEX_RESULT(vertex_collide1, pt1);
        return true;
    }

    return false;
}

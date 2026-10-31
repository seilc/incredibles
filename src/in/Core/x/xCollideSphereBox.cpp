#include "xCollideSphereBox.h"

#include "xCollideSpherePolygon.h"

namespace {
    struct xBoxFace
    {
        xPlane surface;
        xCollideProcessedTri::xEdge edges[4];
    };

    inline S32 xSweptSphereToBoxInside(xSweptSphere& sphere, const xBox& box)
    {
        xVec3 box_center;
        xVec3Init(box_center, 0.5f * (box.upper.x + box.lower.x), 0.5f * (box.upper.y + box.lower.y), 0.5f * (box.upper.z + box.lower.z));

        F32 box_radius = xabs(sphere.start.x - box_center.x) + xabs(sphere.start.y - box_center.y) + xabs(sphere.start.z - box_center.z);
        if (box_radius < 0.1f) box_radius = 0.1f;

        return xSweptSphereToSphere(sphere, box_center, box_radius);
    }

    xBoxFace box_faces[6] =
    {
        {
            { { 0.0f, 1.0f, 0.0f }, 0.0f },
            {
                { { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 0.0f }, { { 0.0f, 0.0f, -1.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 0.0f }, { { 1.0f, 0.0f, 0.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, 0.0f }, { { 0.0f, 0.0f, 1.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, 0.0f }, { { -1.0f, 0.0f, 0.0f }, 0.0f } },
            },
        },
        {
            { { 0.0f, 0.0f, 1.0f }, 0.0f },
            {
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.0f }, { { -1.0f, 0.0f, 0.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 0.0f }, { { 0.0f, 1.0f, 0.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, 0.0f }, { { 1.0f, 0.0f, 0.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, 0.0f }, { { 0.0f, -1.0f, 0.0f }, 0.0f } },
            },
        },
        {
            { { 1.0f, 0.0f, 0.0f }, 0.0f },
            {
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 0.0f }, { { 0.0f, -1.0f, 0.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.0f }, { { 0.0f, 0.0f, 1.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, 0.0f }, { { 0.0f, 1.0f, 0.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, 0.0f }, { { 0.0f, 0.0f, -1.0f }, 0.0f } },
            },
        },
        {
            { { 0.0f, -1.0f, 0.0f }, 0.0f },
            {
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 0.0f }, { { -1.0f, 0.0f, 0.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 0.0f }, { { 0.0f, 0.0f, 1.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, 0.0f }, { { 1.0f, 0.0f, 0.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, 0.0f }, { { 0.0f, 0.0f, -1.0f }, 0.0f } },
            },
        },
        {
            { { 0.0f, 0.0f, -1.0f }, 0.0f },
            {
                { { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 0.0f }, { { 0.0f, -1.0f, 0.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.0f }, { { 1.0f, 0.0f, 0.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, 0.0f }, { { 0.0f, 1.0f, 0.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, 0.0f }, { { -1.0f, 0.0f, 0.0f }, 0.0f } },
            },
        },
        {
            { { -1.0f, 0.0f, 0.0f }, 0.0f },
            {
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.0f }, { { 0.0f, 0.0f, -1.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 0.0f }, { { 0.0f, 1.0f, 0.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, 0.0f }, { { 0.0f, 0.0f, 1.0f }, 0.0f } },
                { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, 0.0f }, { { 0.0f, -1.0f, 0.0f }, 0.0f } },
            },
        },
    };

    xCollideProcessedTri::xEdge* box_edges[6][6] =
    {
        { NULL, &box_faces[0].edges[2], &box_faces[0].edges[1], NULL, &box_faces[0].edges[0], &box_faces[0].edges[3] },
        { &box_faces[1].edges[1], NULL, &box_faces[1].edges[2], &box_faces[1].edges[3], NULL, &box_faces[1].edges[0] },
        { &box_faces[2].edges[2], &box_faces[2].edges[1], NULL, &box_faces[2].edges[0], &box_faces[2].edges[3], NULL },
        { NULL, &box_faces[3].edges[1], &box_faces[3].edges[2], NULL, &box_faces[3].edges[3], &box_faces[3].edges[0] },
        { &box_faces[4].edges[2], NULL, &box_faces[4].edges[1], &box_faces[4].edges[0], NULL, &box_faces[4].edges[3] },
        { &box_faces[5].edges[1], &box_faces[5].edges[2], NULL, &box_faces[5].edges[3], &box_faces[5].edges[0], NULL },
    };

    template <S32 A>
    inline void fill(xBoxFace& face, const xBox& box)
    {
    }

    template <>
    inline void fill<0>(xBoxFace& face, const xBox& box)
    {
        face.surface.d = box.upper.y;
        face.edges[0].segment.start.assign(box.lower.x, box.upper.y, box.lower.z);
        face.edges[1].segment.start.assign(box.upper.x, box.upper.y, box.lower.z);
        face.edges[2].segment.start.assign(box.upper.x, box.upper.y, box.upper.z);
        face.edges[3].segment.start.assign(box.lower.x, box.upper.y, box.upper.z);

        face.edges[0].segment.length = face.edges[2].segment.length = box.upper.x - box.lower.x;
        face.edges[1].segment.length = face.edges[3].segment.length = box.upper.z - box.lower.z;

        face.edges[0].edge.d = -box.lower.z;
        face.edges[1].edge.d = box.upper.x;
        face.edges[2].edge.d = box.upper.z;
        face.edges[3].edge.d = -box.lower.x;
    }

    template <>
    inline void fill<1>(xBoxFace& face, const xBox& box)
    {
        face.surface.d = box.upper.z;
        face.edges[0].segment.start.assign(box.lower.x, box.lower.y, box.upper.z);
        face.edges[1].segment.start.assign(box.lower.x, box.upper.y, box.upper.z);
        face.edges[2].segment.start.assign(box.upper.x, box.upper.y, box.upper.z);
        face.edges[3].segment.start.assign(box.upper.x, box.lower.y, box.upper.z);

        face.edges[0].segment.length = face.edges[2].segment.length = box.upper.y - box.lower.y;
        face.edges[1].segment.length = face.edges[3].segment.length = box.upper.y - box.lower.y;

        face.edges[0].edge.d = -box.lower.x;
        face.edges[1].edge.d = box.upper.y;
        face.edges[2].edge.d = box.upper.x;
        face.edges[3].edge.d = -box.lower.y;
    }

    template <>
    inline void fill<2>(xBoxFace& face, const xBox& box)
    {
        face.surface.d = box.upper.x;
        face.edges[0].segment.start.assign(box.upper.x, box.lower.y, box.lower.z);
        face.edges[1].segment.start.assign(box.upper.x, box.lower.y, box.upper.z);
        face.edges[2].segment.start.assign(box.upper.x, box.upper.y, box.upper.z);
        face.edges[3].segment.start.assign(box.upper.x, box.upper.y, box.lower.z);

        face.edges[0].segment.length = face.edges[2].segment.length = box.upper.z - box.lower.z;
        face.edges[1].segment.length = face.edges[3].segment.length = box.upper.y - box.lower.y;

        face.edges[0].edge.d = -box.lower.y;
        face.edges[1].edge.d = box.upper.z;
        face.edges[2].edge.d = box.upper.y;
        face.edges[3].edge.d = -box.lower.z;
    }

    template <>
    inline void fill<3>(xBoxFace& face, const xBox& box)
    {
        face.surface.d = -box.lower.y;
        face.edges[0].segment.start.assign(box.lower.x, box.lower.y, box.lower.z);
        face.edges[1].segment.start.assign(box.lower.x, box.lower.y, box.upper.z);
        face.edges[2].segment.start.assign(box.upper.x, box.lower.y, box.upper.z);
        face.edges[3].segment.start.assign(box.upper.x, box.lower.y, box.lower.z);

        face.edges[0].segment.length = face.edges[2].segment.length = box.upper.z - box.lower.z;
        face.edges[1].segment.length = face.edges[3].segment.length = box.upper.x - box.lower.x;

        face.edges[0].edge.d = -box.lower.x;
        face.edges[1].edge.d = box.upper.z;
        face.edges[2].edge.d = box.upper.x;
        face.edges[3].edge.d = -box.lower.z;
    }

    template <>
    inline void fill<4>(xBoxFace& face, const xBox& box)
    {
        face.surface.d = -box.lower.z;
        face.edges[0].segment.start.assign(box.lower.x, box.lower.y, box.lower.z);
        face.edges[1].segment.start.assign(box.upper.x, box.lower.y, box.lower.z);
        face.edges[2].segment.start.assign(box.upper.x, box.upper.y, box.lower.z);
        face.edges[3].segment.start.assign(box.lower.x, box.upper.y, box.lower.z);

        face.edges[0].segment.length = face.edges[2].segment.length = box.upper.x - box.lower.x;
        face.edges[1].segment.length = face.edges[3].segment.length = box.upper.y - box.lower.y;

        face.edges[0].edge.d = -box.lower.y;
        face.edges[1].edge.d = box.upper.x;
        face.edges[2].edge.d = box.upper.y;
        face.edges[3].edge.d = -box.lower.x;
    }

    template <>
    inline void fill<5>(xBoxFace& face, const xBox& box)
    {
        face.surface.d = -box.lower.x;
        face.edges[0].segment.start.assign(box.lower.x, box.lower.y, box.lower.z);
        face.edges[1].segment.start.assign(box.lower.x, box.upper.y, box.lower.z);
        face.edges[2].segment.start.assign(box.lower.x, box.upper.y, box.upper.z);
        face.edges[3].segment.start.assign(box.lower.x, box.lower.y, box.upper.z);

        face.edges[0].segment.length = face.edges[2].segment.length = box.upper.y - box.lower.y;
        face.edges[1].segment.length = face.edges[3].segment.length = box.upper.z - box.lower.z;

        face.edges[0].edge.d = -box.lower.z;
        face.edges[1].edge.d = box.upper.y;
        face.edges[2].edge.d = box.upper.z;
        face.edges[3].edge.d = -box.lower.y;
    }

    template <S32 A>
    bool check_moving_away(const xVec3& dir);

    template <> inline bool check_moving_away<0>(const xVec3& dir) { return dir.y >= -0.0001f; }

    template <> inline bool check_moving_away<3>(const xVec3& dir) { return dir.y <= 0.0001f; }

    template <> inline bool check_moving_away<2>(const xVec3& dir) { return dir.x >= -0.0001f; }

    template <> inline bool check_moving_away<5>(const xVec3& dir) { return dir.x <= 0.0001f; }

    template <> inline bool check_moving_away<1>(const xVec3& dir) { return dir.z >= -0.0001f; }

    template <> inline bool check_moving_away<4>(const xVec3& dir) { return dir.z <= 0.0001f; }

    template <S32 A>
    S32 xSweptSphereToBoxFaces(xSweptSphere& sphere, const xBox& box)
    {
        fill<A>(box_faces[A], box);

        xCollideSphereToPolygonResults results;
        if (xSweptSphereToPolygon(sphere, box_faces[A].surface, box_faces[A].edges, 4, results) == true) {
            sphere.worldContact = results.contact;
            sphere.curdist = results.dist;
            return TRUE;
        }

        return FALSE;
    }

    template <S32 A, S32 B>
    inline bool check_against_first_side(xSweptSphere& sphere, const xBox& box)
    {
        fill<A>(box_faces[A], box);

        xCollideProcessedTri::xEdge* edge = box_edges[A][B];

        xVec3 to_sphere;
        xVec3Sub(to_sphere, sphere.start, edge->segment.start);

        xVec3 path_perp;
        path_perp.cross(to_sphere, edge->segment.dir);

        return path_perp.dot(sphere.dir) <= 0.0f;
    }

    template <S32 A, S32 B>
    inline S32 xSweptSphereToBoxFaces(xSweptSphere& sphere, const xBox& box)
    {
        if (check_moving_away<A>(sphere.dir) == true) {
            return xSweptSphereToBoxFaces<B>(sphere, box);
        }

        if (check_moving_away<B>(sphere.dir) == true) {
            return xSweptSphereToBoxFaces<A>(sphere, box);
        }

        if (check_against_first_side<A, B>(sphere, box) == true) {
            return xSweptSphereToBoxFaces<A>(sphere, box);
        } else {
            return xSweptSphereToBoxFaces<B>(sphere, box);
        }
    }

    template <S32 A, S32 B, S32 C>
    inline S32 xSweptSphereToBoxFaces(xSweptSphere& sphere, const xBox& box)
    {
        if (check_moving_away<A>(sphere.dir) == true) {
            return xSweptSphereToBoxFaces<B, C>(sphere, box);
        }

        if (check_moving_away<B>(sphere.dir) == true) {
            return xSweptSphereToBoxFaces<A, C>(sphere, box);
        }

        if (check_against_first_side<A, B>(sphere, box) == true) {
            return xSweptSphereToBoxFaces<A, C>(sphere, box);
        } else {
            return xSweptSphereToBoxFaces<B, C>(sphere, box);
        }
    }
}

S32 xSweptSphereToBox(xSweptSphere& sws, const xBox& box)
{
    if (sws.dist <= 0.0f || sws.curdist <= 0.0f) {
        return FALSE;
    }

    if (sws.start.y >= box.upper.y + 0.001f) {
        if (sws.start.x >= box.upper.x + 0.001f) {
            if (sws.start.z >= box.upper.z + 0.001f) {
                return xSweptSphereToBoxFaces<2, 0, 1>(sws, box);
            }

            if (sws.start.z <= box.lower.z - 0.001f) {
                return xSweptSphereToBoxFaces<2, 0, 4>(sws, box);
            }

            return xSweptSphereToBoxFaces<2, 0>(sws, box);
        }

        if (sws.start.x <= box.lower.x - 0.001f) {
            if (sws.start.z >= box.upper.z + 0.001f) {
                return xSweptSphereToBoxFaces<5, 0, 1>(sws, box);
            }

            if (sws.start.z <= box.lower.z - 0.001f) {
                return xSweptSphereToBoxFaces<5, 0, 4>(sws, box);
            }

            return xSweptSphereToBoxFaces<5, 0>(sws, box);
        }

        if (sws.start.z >= box.upper.z + 0.001f) {
            return xSweptSphereToBoxFaces<0, 1>(sws, box);
        }

        if (sws.start.z <= box.lower.z - 0.001f) {
            return xSweptSphereToBoxFaces<0, 4>(sws, box);
        }

        if (check_moving_away<0>(sws.dir) == true) {
            return FALSE;
        }

        return xSweptSphereToBoxFaces<0>(sws, box);
    }

    if (sws.start.y <= box.lower.y - 0.001f) {
        if (sws.start.x >= box.upper.x + 0.001f) {
            if (sws.start.z >= box.upper.z + 0.001f) {
                return xSweptSphereToBoxFaces<2, 3, 1>(sws, box);
            }

            if (sws.start.z <= box.lower.z - 0.001f) {
                return xSweptSphereToBoxFaces<2, 3, 4>(sws, box);
            }

            return xSweptSphereToBoxFaces<2, 3>(sws, box);
        }

        if (sws.start.x <= box.lower.x - 0.001f) {
            if (sws.start.z >= box.upper.z + 0.001f) {
                return xSweptSphereToBoxFaces<5, 3, 1>(sws, box);
            }

            if (sws.start.z <= box.lower.z - 0.001f) {
                return xSweptSphereToBoxFaces<5, 3, 4>(sws, box);
            }

            return xSweptSphereToBoxFaces<5, 3>(sws, box);
        }

        if (sws.start.z >= box.upper.z + 0.001f) {
            return xSweptSphereToBoxFaces<3, 1>(sws, box);
        }

        if (sws.start.z <= box.lower.z - 0.001f) {
            return xSweptSphereToBoxFaces<3, 4>(sws, box);
        }

        if (check_moving_away<3>(sws.dir) == true) {
            return FALSE;
        }

        return xSweptSphereToBoxFaces<3>(sws, box);
    }

    if (sws.start.x >= box.upper.x + 0.001f) {
        if (sws.start.z >= box.upper.z + 0.001f) {
            return xSweptSphereToBoxFaces<2, 1>(sws, box);
        }

        if (sws.start.z <= box.lower.z - 0.001f) {
            return xSweptSphereToBoxFaces<2, 4>(sws, box);
        }

        if (check_moving_away<2>(sws.dir) == true) {
            return FALSE;
        }

        return xSweptSphereToBoxFaces<2>(sws, box);
    }

    if (sws.start.x <= box.lower.x - 0.001f) {
        if (sws.start.z >= box.upper.z + 0.001f) {
            return xSweptSphereToBoxFaces<5, 1>(sws, box);
        }

        if (sws.start.z <= box.lower.z - 0.001f) {
            return xSweptSphereToBoxFaces<5, 4>(sws, box);
        }

        if (check_moving_away<5>(sws.dir) == true) {
            return FALSE;
        }

        return xSweptSphereToBoxFaces<5>(sws, box);
    }

    if (sws.start.z >= box.upper.z + 0.001f) {
        if (check_moving_away<1>(sws.dir) == true) {
            return FALSE;
        }

        return xSweptSphereToBoxFaces<1>(sws, box);
    }

    if (sws.start.z <= box.lower.z - 0.001f) {
        if (check_moving_away<4>(sws.dir) == true) {
            return FALSE;
        }

        return xSweptSphereToBoxFaces<4>(sws, box);
    }

    return xSweptSphereToBoxInside(sws, box);
}

S32 xSweptSphereToOBB(xSweptSphere* sws, const xBox* box, const xMat4x3* mat)
{
    if (sws->dist <= 0.0f || sws->curdist <= 0.0f) {
        return FALSE;
    }

    xVec3 old_start = sws->start, old_end = sws->end, old_dir = sws->dir;
    F32 old_dist = sws->dist;

    F32 box_scale = mat->at.length();

    xMat4x3Tolocal(&sws->start, mat, &old_start);
    xMat4x3Tolocal(&sws->end, mat, &old_end);

    xVec3Sub(sws->dir, sws->end, sws->start);
    sws->dist = xVec3NormalizeSafe(sws->dir);

    if (sws->dist == 0.0f) {
        sws->start = old_start;
        sws->end = old_end;
        sws->dir = old_dir;
        sws->dist = old_dist;
        return FALSE;
    }

    F32 old_radius = sws->radius, old_curdist = sws->curdist;

    sws->radius /= box_scale;
    sws->curdist /= box_scale;

    F32 initial_scaled_curdist = sws->curdist;

    S32 got_collision = xSweptSphereToBox(sws, box);
    if (got_collision) {
        xVec3 collide_point = sws->worldContact;
        xMat4x3Toworld(&sws->worldContact, mat, &collide_point);
    }

    sws->start = old_start;
    sws->end = old_end;
    sws->dir = old_dir;
    sws->radius = old_radius;
    sws->dist = old_dist;

    if (sws->curdist < initial_scaled_curdist) {
        sws->curdist *= box_scale;
    } else {
        sws->curdist = old_curdist;
    }

    return got_collision;
}

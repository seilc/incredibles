#include "xCommon.h"
#include "xCollideSpherePolygon.h"

static bool PolygonTestSphereOnPlane(const xPlane& surface, const xCollideProcessedTri::xEdge* edges, S32 num_edges, const xSweptSphere& sphere, xCollideSphereToPolygonResults& results, F32 vertical_d0);
static bool PolygonTestAH0a(S32 i, F32 closest_dist_squ, const xPlane&, const xCollideProcessedTri::xEdge* edges, S32 num_edges, const xSweptSphere& sphere, xCollideSphereToPolygonResults& results);
static bool PolygonTestAH0b(S32 i, F32 closest_dist_squ, const xPlane&, const xCollideProcessedTri::xEdge* edges, S32 num_edges, const xSweptSphere& sphere, xCollideSphereToPolygonResults& results);
static bool PolygonTestAH0c(S32 i, F32 closest_dist_squ, const xPlane& surface, const xCollideProcessedTri::xEdge* edges, S32 num_edges, const xSweptSphere& sphere, xCollideSphereToPolygonResults& results);
static bool PolygonTestBH0a(S32 i, F32 closest_dist_squ, const xPlane& surface, const xCollideProcessedTri::xEdge* edges, S32 num_edges, const xSweptSphere& sphere, xCollideSphereToPolygonResults& results);

bool xSweptSphereToPoint(const xSweptSphere& sphere, const xVec3& pt, xCollideSphereToPointResults& results)
{
    xVec3 to_pt;
    xVec3Sub(to_pt, pt, sphere.start);

    F32 along_d0 = to_pt.dot(sphere.dir);
    if (along_d0 <= 0.001f) {
        return false;
    }

    xVec3 proj_vec;
    xVec3AddScaled(proj_vec, to_pt, sphere.dir, -along_d0);

    F32 dist_from_line_squ = proj_vec.length2();
    F32 sphere_radius_squ = xsqr(sphere.radius);
    if (dist_from_line_squ >= sphere_radius_squ) {
        return false;
    }

    F32 forward_distance = xsqrt(sphere_radius_squ - dist_from_line_squ);
    if (along_d0 <= forward_distance) {
        results.dist = 0.0f;
        return true;
    }

    F32 along_d_impact = along_d0 - forward_distance;
    if (along_d_impact < sphere.curdist) {
        results.dist = along_d_impact;
        return true;
    }

    return false;
}

bool xSweptSphereToLineSement(const xSweptSphere& sphere, const xVec3& segment_start, const xVec3& segment_dir, F32 segment_length, xCollideSphereToLineSegmentResults& results)
{
    xPlane shear;
    shear.norm.cross(sphere.dir, segment_dir);

    F32 shear_normal_len2 = shear.norm.length2();
    if (shear_normal_len2 < 0.00001f) {
        return false;
    }

    xVec3SMulBy(shear.norm, 1.0f / xsqrt(shear_normal_len2));
    shear.Init(segment_start);

    F32 sphere_height = shear.Dist(sphere.start);
    if (sphere.radius - sphere_height <= 0.0001f || sphere_height + sphere.radius <= 0.0001f) {
        return false;
    }

    xVec3 shear_sphere_center;
    shear.Project(shear_sphere_center, sphere.start, sphere_height);

    xVec3 segment_start_to_sphere;
    xVec3Sub(segment_start_to_sphere, shear_sphere_center, segment_start);

    F32 proj_center_along_seg_d0 = segment_dir.dot(segment_start_to_sphere);
    F32 proj_center_along_seg_de;
    F32 proj_center_along_seg_vel = segment_dir.dot(sphere.dir);
    if (proj_center_along_seg_d0 < 0.0f) {
        if (proj_center_along_seg_vel <= 0.0f) {
            return false;
        }

        proj_center_along_seg_de = proj_center_along_seg_vel * sphere.curdist + proj_center_along_seg_d0;
        if (proj_center_along_seg_de < 0.0f) {
            return false;
        }
    } else if (proj_center_along_seg_d0 > segment_length) {
        if (proj_center_along_seg_vel >= 0.0f) {
            return false;
        }

        proj_center_along_seg_de = proj_center_along_seg_vel * sphere.curdist + proj_center_along_seg_d0;
        if (proj_center_along_seg_de > segment_length) {
            return false;
        }
    } else {
        proj_center_along_seg_de = proj_center_along_seg_vel * sphere.curdist + proj_center_along_seg_d0;
    }

    xVec3 segment_out_normal;
    segment_out_normal.cross(segment_dir, shear.norm);

    F32 proj_center_above_seg_d0 = segment_out_normal.dot(segment_start_to_sphere);
    F32 proj_center_above_seg_de;
    F32 proj_center_above_seg_vel = segment_out_normal.dot(sphere.dir);
    F32 sphere_intersect_radius = xsqrt(xsqr(sphere.radius) - xsqr(sphere_height));
    if (proj_center_above_seg_d0 > sphere_intersect_radius) {
        if (proj_center_above_seg_vel >= 0.0f) {
            return false;
        }

        proj_center_above_seg_de = proj_center_above_seg_vel * sphere.curdist + proj_center_above_seg_d0;
        if (proj_center_above_seg_de > sphere_intersect_radius) {
            return false;
        }
    } else if (proj_center_above_seg_d0 < -sphere_intersect_radius) {
        if (proj_center_above_seg_vel <= 0.0f) {
            return false;
        }

        proj_center_above_seg_de = proj_center_above_seg_vel * sphere.curdist + proj_center_above_seg_d0;
        if (proj_center_above_seg_de < -sphere_intersect_radius) {
            return false;
        }
    } else {
        proj_center_above_seg_de = proj_center_above_seg_vel * sphere.curdist + proj_center_above_seg_d0;
    }

    if (proj_center_along_seg_d0 >= 0.0f && proj_center_along_seg_d0 <= segment_length) {
        if (proj_center_above_seg_d0 >= 0.0001f) {
            if (proj_center_above_seg_d0 <= sphere_intersect_radius) {
                if (proj_center_above_seg_vel >= 0.0f) {
                    return false;
                }

                results.dist = 0.0f;
                xVec3AddScaled(results.contact, segment_start, segment_dir, proj_center_along_seg_d0);
                return true;
            }
        } else if (proj_center_above_seg_d0 <= -0.0001f) {
            if (proj_center_above_seg_d0 >= -sphere_intersect_radius) {
                if (proj_center_above_seg_vel <= 0.0f) {
                    return false;
                }

                results.dist = 0.0f;
                xVec3AddScaled(results.contact, segment_start, segment_dir, proj_center_along_seg_d0);
                return true;
            }
        } else {
            return false;
        }
    } else if (proj_center_above_seg_d0 <= sphere_intersect_radius && proj_center_above_seg_d0 >= -sphere_intersect_radius) {
        return false;
    }

    if (proj_center_above_seg_d0 > 0.0f) {
        F32 travel_total = proj_center_above_seg_d0 - proj_center_above_seg_de;
        if (travel_total < 0.00001f) {
            return false;
        }

        F32 travel_percent = (proj_center_above_seg_d0 - sphere_intersect_radius) / travel_total;
        F32 impact_along_dist = travel_percent * (proj_center_along_seg_de - proj_center_along_seg_d0) + proj_center_along_seg_d0;
        if (impact_along_dist < 0.0f || impact_along_dist > segment_length) {
            return false;
        }

        results.dist = sphere.curdist * travel_percent;
        xVec3AddScaled(results.contact, segment_start, segment_dir, impact_along_dist);
        return true;
    } else {
        F32 travel_total = proj_center_above_seg_de - proj_center_above_seg_d0;
        if (travel_total < 0.00001f) {
            return false;
        }

        F32 travel_percent = (-proj_center_above_seg_d0 - sphere_intersect_radius) / travel_total;
        F32 impact_along_dist = travel_percent * (proj_center_along_seg_de - proj_center_along_seg_d0) + proj_center_along_seg_d0;
        if (impact_along_dist < 0.0f || impact_along_dist > segment_length) {
            return false;
        }

        results.dist = sphere.curdist * travel_percent;
        xVec3AddScaled(results.contact, segment_start, segment_dir, impact_along_dist);
        return true;
    }
}

#define RETURN_VERTEX_TEST(n, test)                                                               \
    xVec3 sphere_to_vertex;                                                                       \
    xVec3Sub(sphere_to_vertex, edges[(n)].segment.start, sphere.start);                           \
    F32 closest_dist_squ = sphere_to_vertex.length2();                                            \
    results.dist = vertex_collide.dist;                                                           \
    results.contact = edges[(n)].segment.start;                                                   \
    return (test)((n), closest_dist_squ, surface, edges, num_edges, sphere, results)

#define UPDATE_CLOSEST_VERTEX(n)                                                                  \
    xVec3 sphere_to_vert;                                                                         \
    xVec3Sub(sphere_to_vert, edges[(n)].segment.start, sphere.start);                             \
    F32 new_dist_squ = sphere_to_vert.length2();                                                  \
    if (new_dist_squ < closest_dist_squ) {                                                        \
        results.dist = vertex_collide.dist;                                                       \
        results.contact = edges[(n)].segment.start;                                               \
        closest_dist_squ = new_dist_squ;                                                          \
    }

bool xSweptSphereToPolygon(const xSweptSphere& sphere, const xPlane& surface, const xCollideProcessedTri::xEdge* edges, S32 num_edges, xCollideSphereToPolygonResults& results)
{
    F32 vertical_vel = sphere.dir.dot(surface.norm);
    if (vertical_vel >= -0.001f) {
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
        if (PolygonTestSphereOnPlane(surface, edges, num_edges, at_plane_sphere, results, sphere.radius) == true) {
            results.dist += to_plane_move_dist;
            return true;
        }

        return false;
    }
    
    return PolygonTestSphereOnPlane(surface, edges, num_edges, sphere, results, vertical_d0);
}

static bool PolygonTestSphereOnPlane(const xPlane& surface, const xCollideProcessedTri::xEdge* edges, S32 num_edges, const xSweptSphere& sphere, xCollideSphereToPolygonResults& results, F32 vertical_d0)
{
    F32 above_edge_d0 = edges[0].edge.Dist(sphere.start);
    F32 above_edge_vel;
    F32 above_edge_de;
    xCollideSphereToLineSegmentResults edge_collide;
    xCollideSphereToPointResults vertex_collide;

    if (above_edge_d0 > 0.0f) {
        above_edge_vel = edges[0].edge.norm.dot(sphere.dir);
        above_edge_de = above_edge_vel * sphere.curdist + above_edge_d0;
        if (above_edge_de >= sphere.radius && above_edge_d0 >= sphere.radius) {
            return false;
        }

        if (xSweptSphereToLineSement(sphere, edges[0].segment, edge_collide) == true) {
            results.dist = edge_collide.dist;
            results.contact = edge_collide.contact;
            results.type = exCOLLIDESPHEREHITTYPE_EDGE;
            return true;
        }

        if (xSweptSphereToPoint(sphere, edges[0].segment.start, vertex_collide) == true) {
            RETURN_VERTEX_TEST(0, PolygonTestAH0c);
        }

        for (S32 i = 1; i < num_edges; i++) {
            above_edge_d0 = edges[i].edge.Dist(sphere.start);
            if (above_edge_d0 <= 0.0f) {
                if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
                    RETURN_VERTEX_TEST(0, PolygonTestBH0a);
                }

                for (i++; i < num_edges; i++) {
                    above_edge_d0 = edges[i].edge.Dist(sphere.start);
                    if (above_edge_d0 <= 0.0f) {
                        continue;
                    }

                    above_edge_vel = edges[i].edge.norm.dot(sphere.dir);
                    above_edge_de = above_edge_vel * sphere.curdist + above_edge_d0;
                    if (above_edge_de >= sphere.radius && above_edge_d0 >= sphere.radius) {
                        return false;
                    }
                    
                    if (xSweptSphereToLineSement(sphere, edges[i].segment, edge_collide) == true) {
                        results.dist = edge_collide.dist;
                        results.contact = edge_collide.contact;
                        results.type = exCOLLIDESPHEREHITTYPE_EDGE;
                        return true;
                    }

                    if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
                        RETURN_VERTEX_TEST(i, PolygonTestAH0b);
                    }

                    for (i++; i < num_edges; i++) {
                        above_edge_d0 = edges[i].edge.Dist(sphere.start);
                        above_edge_vel = edges[i].edge.norm.dot(sphere.dir);
                        above_edge_de = above_edge_vel * sphere.curdist + above_edge_d0;
                        if (above_edge_de >= sphere.radius && above_edge_d0 >= sphere.radius) {
                            return false;
                        }
                        
                        if (xSweptSphereToLineSement(sphere, edges[i].segment, edge_collide) == true) {
                            results.dist = edge_collide.dist;
                            results.contact = edge_collide.contact;
                            results.type = exCOLLIDESPHEREHITTYPE_EDGE;
                            return true;
                        }

                        if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
                            RETURN_VERTEX_TEST(i, PolygonTestAH0b);
                        }
                    }

                    return false;
                }

                return false;
            }

            above_edge_vel = edges[i].edge.norm.dot(sphere.dir);
            above_edge_de = above_edge_vel * sphere.curdist + above_edge_d0;
            if (above_edge_de >= sphere.radius && above_edge_d0 >= sphere.radius) {
                return false;
            }
            
            if (xSweptSphereToLineSement(sphere, edges[i].segment, edge_collide) == true) {
                results.dist = edge_collide.dist;
                results.contact = edge_collide.contact;
                results.type = exCOLLIDESPHEREHITTYPE_EDGE;
                return true;
            }

            if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
                RETURN_VERTEX_TEST(i, PolygonTestAH0c);
            }
        }

        return false;
    }

    for (S32 i = 1; i < num_edges; i++) {
        above_edge_d0 = edges[i].edge.Dist(sphere.start);
        if (above_edge_d0 > 0.0f) {
            above_edge_vel = edges[i].edge.norm.dot(sphere.dir);
            above_edge_de = above_edge_vel * sphere.curdist + above_edge_d0;
            if (above_edge_de >= sphere.radius && above_edge_d0 >= sphere.radius) {
                return false;
            }
            
            if (xSweptSphereToLineSement(sphere, edges[i].segment, edge_collide) == true) {
                results.dist = edge_collide.dist;
                results.contact = edge_collide.contact;
                results.type = exCOLLIDESPHEREHITTYPE_EDGE;
                return true;
            }

            if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
                RETURN_VERTEX_TEST(i, PolygonTestAH0a);
            }

            for (i++; i < num_edges; i++) {
                above_edge_d0 = edges[i].edge.Dist(sphere.start);
                if (above_edge_d0 <= 0.0f) {
                    if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
                        results.dist = vertex_collide.dist;
                        results.contact = edges[i].segment.start;
                        results.type = exCOLLIDESPHEREHITTYPE_VERTEX;
                        return true;
                    }

                    return false;
                }
                
                if (xSweptSphereToLineSement(sphere, edges[i].segment, edge_collide) == true) {
                    results.dist = edge_collide.dist;
                    results.contact = edge_collide.contact;
                    results.type = exCOLLIDESPHEREHITTYPE_EDGE;
                    return true;
                }

                if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
                    RETURN_VERTEX_TEST(i, PolygonTestAH0a);
                }
            }

            if (xSweptSphereToPoint(sphere, edges[0].segment.start, vertex_collide) == true) {
                results.dist = vertex_collide.dist;
                results.contact = edges[0].segment.start;
                results.type = exCOLLIDESPHEREHITTYPE_VERTEX;
                return true;
            }

            return false;
        }
    }

    results.dist = 0.0f;
    surface.Project(results.contact, sphere.start, vertical_d0);
    results.type = exCOLLIDESPHEREHITTYPE_INTERIOR;
    return true;
}

static bool PolygonTestAH0a(S32 i, F32 closest_dist_squ, const xPlane&, const xCollideProcessedTri::xEdge* edges, S32 num_edges, const xSweptSphere& sphere, xCollideSphereToPolygonResults& results)
{
    xCollideSphereToLineSegmentResults edge_collide;
    xCollideSphereToPointResults vertex_collide;

    for (i++; i < num_edges; i++) {
        F32 above_edge_d0 = edges[i].edge.Dist(sphere.start);
        if (above_edge_d0 <= 0.0f) {
            if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
                xVec3 sphere_to_vert;
                xVec3Sub(sphere_to_vert, edges[i].segment.start, sphere.start);
                F32 new_dist_squ = sphere_to_vert.length2();
                if (new_dist_squ < closest_dist_squ) {
                    results.dist = vertex_collide.dist;
                    results.contact = edges[i].segment.start;
                    results.type = exCOLLIDESPHEREHITTYPE_VERTEX;
                    return true;
                }

                results.type = exCOLLIDESPHEREHITTYPE_VERTEX;
                return true;
            }

            results.type = exCOLLIDESPHEREHITTYPE_VERTEX;
            return true;
        }
        
        if (xSweptSphereToLineSement(sphere, edges[i].segment, edge_collide) == true) {
            results.dist = edge_collide.dist;
            results.contact = edge_collide.contact;
            results.type = exCOLLIDESPHEREHITTYPE_EDGE;
            return true;
        }

        if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
            UPDATE_CLOSEST_VERTEX(i);
        }
    }
    
    if (xSweptSphereToPoint(sphere, edges[0].segment.start, vertex_collide) == true) {
        results.dist = vertex_collide.dist;
        results.contact = edges[0].segment.start;
        results.type = exCOLLIDESPHEREHITTYPE_VERTEX;
        return true;
    }

    results.type = exCOLLIDESPHEREHITTYPE_VERTEX;
    return true;
}

static bool PolygonTestAH0b(S32 i, F32 closest_dist_squ, const xPlane&, const xCollideProcessedTri::xEdge* edges, S32 num_edges, const xSweptSphere& sphere, xCollideSphereToPolygonResults& results)
{
    xCollideSphereToLineSegmentResults edge_collide;
    xCollideSphereToPointResults vertex_collide;

    for (i++; i < num_edges; i++) {
        if (xSweptSphereToLineSement(sphere, edges[i].segment, edge_collide) == true) {
            results.dist = edge_collide.dist;
            results.contact = edge_collide.contact;
            results.type = exCOLLIDESPHEREHITTYPE_EDGE;
            return true;
        }

        if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
            UPDATE_CLOSEST_VERTEX(i);
        }
    }

    results.type = exCOLLIDESPHEREHITTYPE_VERTEX;
    return true;
}

static bool PolygonTestAH0c(S32 i, F32 closest_dist_squ, const xPlane& surface, const xCollideProcessedTri::xEdge* edges, S32 num_edges, const xSweptSphere& sphere, xCollideSphereToPolygonResults& results)
{
    xCollideSphereToLineSegmentResults edge_collide;
    xCollideSphereToPointResults vertex_collide;

    for (i++; i < num_edges; i++) {
        F32 above_edge_d0 = edges[i].edge.Dist(sphere.start);
        if (above_edge_d0 <= 0.0f) {
            if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
                UPDATE_CLOSEST_VERTEX(i);
            }

            return PolygonTestBH0a(i, closest_dist_squ, surface, edges, num_edges, sphere, results);
        }
        
        if (xSweptSphereToLineSement(sphere, edges[i].segment, edge_collide) == true) {
            results.dist = edge_collide.dist;
            results.contact = edge_collide.contact;
            results.type = exCOLLIDESPHEREHITTYPE_EDGE;
            return true;
        }

        if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
            UPDATE_CLOSEST_VERTEX(i);
        }
    }
    
    results.type = exCOLLIDESPHEREHITTYPE_VERTEX;
    return true;
}

static bool PolygonTestBH0a(S32 i, F32 closest_dist_squ, const xPlane& surface, const xCollideProcessedTri::xEdge* edges, S32 num_edges, const xSweptSphere& sphere, xCollideSphereToPolygonResults& results)
{
    xCollideSphereToLineSegmentResults edge_collide;
    xCollideSphereToPointResults vertex_collide;

    for (i++; i < num_edges; i++) {
        F32 above_edge_d0 = edges[i].edge.Dist(sphere.start);
        if (above_edge_d0 <= 0.0f) {
            continue;
        }

        if (xSweptSphereToLineSement(sphere, edges[i].segment, edge_collide) == true) {
            results.dist = edge_collide.dist;
            results.contact = edge_collide.contact;
            results.type = exCOLLIDESPHEREHITTYPE_EDGE;
            return true;
        }

        if (xSweptSphereToPoint(sphere, edges[i].segment.start, vertex_collide) == true) {
            UPDATE_CLOSEST_VERTEX(i);
        }

        return PolygonTestAH0b(i, closest_dist_squ, surface, edges, num_edges, sphere, results);
    }

    results.type = exCOLLIDESPHEREHITTYPE_VERTEX;
    return true;
}

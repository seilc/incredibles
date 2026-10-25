#ifndef XCOLLIDESPHEREPOLYGONUNPROCESSED_H
#define XCOLLIDESPHEREPOLYGONUNPROCESSED_H

#include "xCollideSpherePolygon.h"

bool TriangleTestSphereOnPlaneUnprocessed(const xPlane& surface, const xVec3& p0, const xVec3& p1, const xVec3& p2, const xSweptSphere& sphere, xCollideSphereToPolygonResults& results, F32 vertical_d0);
bool xSweptSphereToTriangleDoubleSidedUnprocessed(const xSweptSphere& sphere, const xVec3& pt0, const xVec3& pt1, const xVec3& pt2, xCollideSphereToPolygonResults& results, xPlane& surface);
bool xSweptSphereToTriangleUnprocessed(const xSweptSphere& sphere, const xVec3& pt0, const xVec3& pt1, const xVec3& pt2, xCollideSphereToPolygonResults& results, xPlane& surface);

#endif

#ifndef IMATH3_H
#define IMATH3_H

struct xVec3;
struct xBox;
struct xSphere;
struct xCylinder;
struct xRay3;
struct xIsect;

void iMath3Init();
void iSphereIsectRay(const xSphere* s, const xRay3* r, xIsect* isx);
void iCylinderIsectRay(const xCylinder* c, const xRay3* r, xIsect* isx);
void iBoxIsectRay(const xBox* b, const xRay3* r, xIsect* isx);
void iBoxInitBoundVec(xBox* b, const xVec3* v);
void iBoxBoundVec(xBox* o, const xBox* b, const xVec3* v);

#endif

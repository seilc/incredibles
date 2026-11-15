#ifndef IMATH3_H
#define IMATH3_H

struct xVec3;
struct xVec4;
struct xBox;
struct xBBox;
struct xSphere;
struct xCylinder;
struct xRay3;
struct xIsect;
struct xMat3x3;
struct xMat4x3;

void iMath3Init();
void iSphereIsectVec(const xSphere* s, const xVec3* v, xIsect* isx);
void iSphereIsectRay(const xSphere* s, const xRay3* r, xIsect* isx);
void iSphereIsectSphere(const xSphere* s, const xSphere* p, xIsect* isx);
void iSphereInitBoundVec(xSphere* s, const xVec3* v);
void iSphereInitBoundBox(xSphere* s, const xBox* b); // UNUSED
void iSphereInitBoundOBB(xSphere* s, const xBox* b, const xMat4x3* m);
void iSphereBoundVec(xSphere* o, const xSphere* s, const xVec3* v);
void iSphereBoundSphere(xSphere* o, const xSphere* a, const xSphere* b);
void iCylinderIsectVec(const xCylinder* c, const xVec3* v, xIsect* isx);
void iCylinderIsectRay(const xCylinder* c, const xRay3* r, xIsect* isx);
void iBoxVecDist(const xBox* box, const xVec3* v, xIsect* isx);
void iBoxIsectVec(const xBox* b, const xVec3* v, xIsect* isx);
void iBoxIsectRay(const xBox* b, const xRay3* r, xIsect* isx);
void iBoxIsectSphere(const xBox* box, const xSphere* p, xIsect* isx);
void iBoxInitBoundVec(xBox* b, const xVec3* v);
void iBoxBoundVec(xBox* o, const xBox* b, const xVec3* v);

#endif

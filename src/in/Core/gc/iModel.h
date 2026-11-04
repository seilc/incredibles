#ifndef IMODEL_H
#define IMODEL_H

#include <rwcore.h>
#include <rpworld.h>
#include <rphanim.h>
#include "types.h"

struct xVec3;
struct xQuat;
struct xSphere;
struct xBox;

void iModelInit();
void iModelDeInit();
RpHAnimHierarchy* iModelGetHierarchy(RpAtomic* imodel);
void iModelRender(RpAtomic* model, RwMatrix* mat);
void iModelSetMaterialAlpha(RpAtomic* model, U8 alpha);
void iModelResetMaterial(RpAtomic* model);
void iModelMaterialMul(RpAtomic* model, F32 rm, F32 gm, F32 bm);
void iModelUnload(RpAtomic* userdata);
RpAtomic* iModelFile_RWMultiAtomic(RpAtomic* model);
U32 iModelNumBones(RpAtomic* model);
U32 iModelVertCount(RpAtomic* model);
xSphere* iModelWorldBoundSphere(RpAtomic* model, RwMatrix* mat, xSphere* sphere);
void iModelSetWorldBoundFromBox(RpAtomic* model, xBox* box);
U32 iModelVertEval(RpAtomic* model, U32 index, U32 count, RwMatrix* mat, xVec3* vert, xVec3* dest);
void iModelAnimMatrices(RpAtomic* model, xQuat* quat, xVec3* tran, RwMatrix* mat);
void iModelAnimBoundWorld(xBox* box, xBox* combinedBox, RpAtomic* model, RwMatrix* mat, RwMatrix* bonemat);

#endif
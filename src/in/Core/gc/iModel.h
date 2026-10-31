#ifndef IMODEL_H
#define IMODEL_H

#include <rwcore.h>
#include <rpworld.h>
#include <rphanim.h>
#include "types.h"

RpHAnimHierarchy* iModelGetHierarchy(RpAtomic* imodel);
void iModelRender(RpAtomic* model, RwMatrix* mat);
void iModelSetMaterialAlpha(RpAtomic* model, U8 alpha);
void iModelResetMaterial(RpAtomic* model);
void iModelMaterialMul(RpAtomic* model, F32 rm, F32 gm, F32 bm);

#endif
#ifndef IMORPH_H
#define IMORPH_H

#include "types.h"

#include <rwcore.h>
#include <rpworld.h>

void iMorphOptimize(RpAtomic* model, S32 normals);
void iMorphRender(RpAtomic* model, RwMatrix* mat, S16** v_array, S16* weight, U32 normals, F32 scale);
void FastS16weight2(F32* dest, S16** v_array, S16* weight, S32 count, F32 scale);

#endif

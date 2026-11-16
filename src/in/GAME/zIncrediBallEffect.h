#ifndef ZINCREDIBALLEFFECT_H
#define ZINCREDIBALLEFFECT_H

#include "xMath3.h"

void zIncrediBallSetup();
void zIncrediBallMove(const xMat4x3& mat);
void zIncrediBallUpdate(F32 dt);
void zIncrediBallRender(void (*cb)(void*), void* context);
void zIncrediBallImpact(const xVec3& loc, F32 radius, F32 amount, bool depress);

#endif

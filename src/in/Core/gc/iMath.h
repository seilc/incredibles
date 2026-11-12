#ifndef IMATH_H
#define IMATH_H

#include "types.h"

#define iabs(x) (float)(__fabs((float)(x)))

#define EPSILON 0.00001f


F32 isin(F32 x);
F32 icos(F32 x);
F32 itan(F32 x);

#endif
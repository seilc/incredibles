#ifndef IMATH_H
#define IMATH_H

#include <types.h>
#include <intrin.h>

#define iabs(x) ((F32)__fabs((x)))

#define EPSILON 0.00001f


F32 isin(F32 x);
F32 icos(F32 x);
F32 itan(F32 x);

#endif
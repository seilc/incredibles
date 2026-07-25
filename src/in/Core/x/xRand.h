#ifndef XRAND_H
#define XRAND_H

#include <types.h>

U32 xrand_RandomInt32();

inline U16 xrand_RandomInt16()
{
    return xrand_RandomInt32();
}

inline U32 xrand_RandomChoice(U32 maxCount)
{
    return maxCount * xrand_RandomInt16() >> 16;
}

F32 xurand();

#endif
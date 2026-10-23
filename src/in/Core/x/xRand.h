#ifndef XRAND_H
#define XRAND_H

#include "xDebug.h"

struct xVec3;

enum xRandomChoiceRule
{
    xRandomChoiceRule_ANY_EXCEPT_LAST,
    xRandomChoiceRule_ANY,
    xRandomChoiceRule_ANY_NOT_CHOSEN,
    xRandomChoiceRule_IN_ORDER,
    xRandomChoiceRule_IN_ORDER_RANDOM,
    xRandomChoiceRule_COUNT
};

void init_genrand(U32 s);
void init_by_array(U32* init_key, U32 key_length);
void xrand_RandomInit();
U32 xrand_GenRandInt32();
F32 xrand_GenRandFloat();
U32 xrand_RandomChoice(U32& chosenMask, U8& setBits, U32 maxCount, xRandomChoiceRule rule);
void xRandUniformSpherePoints(xVec3* loc, S32 amount, F32 zmin, F32 zmax);

U16 xrand_RandomInt16();
U32 xrand_RandomInt32();
F32 xrand_RandomFloat();

inline U16 xrand_RandomInt16()
{
    return (U16)xrand_RandomInt32();
}

inline U32 xrand_RandomInt32()
{
    return xrand_GenRandInt32();
}

inline F32 xrand_RandomBipolarUnitFloat()
{
    return 2.0f * xrand_RandomFloat() - 1.0f;
}

inline F32 xrand_RandomFloatRange(F32 fMin, F32 fMax)
{
    F32 fRandUnitFloat = xrand_RandomFloat();
    return fRandUnitFloat * fMax + (1.0f - fRandUnitFloat) * fMin;
}

inline U32 xrand_RandomChoice(U32 nChoice)
{
    xASSERT(124, nChoice <= 0xffff);
    return (nChoice * xrand_RandomInt16()) / 0x10000;
}

inline F32 xurand()
{
    static const F32 scale = 1.0f / 4294967296.0f;
    return scale * xrand_RandomInt32();
}

#endif

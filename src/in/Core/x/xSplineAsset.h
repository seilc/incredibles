#ifndef XSPLINEASSET_H
#define XSPLINEASSET_H

#include "xBase.h"
#include "xMath3.h"

namespace x
{
    struct NURBS : xBaseAsset
    {
        S32 p;
        S32 m;
        S32 n;
        F32* knot;
        xVec3* control;
    };
}; // namespace x

#endif
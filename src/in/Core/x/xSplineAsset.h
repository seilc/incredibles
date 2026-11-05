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

        F32 start();
        void evaluate(F32 u, xVec3& point);
        xVec3 evaluate(F32 u, S32 d);
        bool advance_u(F32 start_u, F32 distance, bool forward, F32& new_u);
    };
}; // namespace x

#endif
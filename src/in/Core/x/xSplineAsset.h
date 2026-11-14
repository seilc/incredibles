#ifndef XSPLINEASSET_H
#define XSPLINEASSET_H

#include "xBaseAsset.h"
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

        F32 approximate_length(S32 divisions); // UNUSED
        S32 find_span(F32 u);
        void find_basis_functions(F32 u, S32 i, F32* N);
        void find_derivative_basis_functions(F32 u, S32 i, S32 order, F32 ders[5][5]);
        F32 start();
        F32 end();
        xVec3 evaluate(F32 u);
        void evaluate(F32 u, xVec3& point);
        xVec3 evaluate(F32 u, S32 d);
        void evaluate(F32 u, S32 d, xVec3& point);
        xVec3 project_point(const xVec3& point, F32* u, F32* distance, F32 guess);
        NURBS(S32 p, S32 knot_count, xVec3* control, F32* knot);
        U8 circle_intersect(F32& intersect_u, xVec3& intersect_vec, F32 begin_u, F32 end_u, const xVec3& circle_center, F32 circle_radius);
        bool advance_u(F32 start_u, F32 distance, bool forward, F32& new_u);
    };
}; // namespace x

#endif

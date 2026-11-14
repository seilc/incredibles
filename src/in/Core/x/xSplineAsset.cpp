#include "xCommon.h"
#include "xSplineAsset.h"

#define MAX_DEPTH ((U32)10)

namespace x
{

// UNUSED: needed to match (speculative code)
F32 NURBS::approximate_length(S32 divisions)
{
    F32 division_size = (end() - start()) / divisions;
    F32 length = 0.0f;

    xVec3 last_point = evaluate(start());

    for (F32 t = division_size + start(); t <= end(); t += division_size) {
        xVec3 point = evaluate(t);
        length += (point - last_point).length();
        last_point = point;
    }

    return length;
}

S32 NURBS::find_span(F32 u)
{
    if (u == knot[n + 1])
        return n;

    S32 low = p;
    S32 high = n + 1;
    S32 mid = (low + high) / 2;

    while (u < knot[mid] || u >= knot[mid + 1]) {
        if (u < knot[mid]) {
            if (high <= mid)
                break;
            high = mid;
        } else {
            if (low >= mid)
                break;
            low = mid;
        }
        mid = (low + high) / 2;
    }

    return mid;
}

void NURBS::find_basis_functions(F32 u, S32 i, F32* N)
{
    N[0] = 1.0f;

    F32 left[5];
    F32 right[5];
    for (S32 j = 1; j <= p; j++) {
        left[j] = u - knot[i + 1 - j];
        right[j] = knot[i + j] - u;
        F32 saved = 0.0f;
        for (S32 r = 0; r < j; r++) {
            F32 temp = N[r] / (right[r + 1] + left[j - r]);
            N[r] = temp * right[r + 1] + saved;
            saved = temp * left[j - r];
        }
        N[j] = saved;
    }
}

void NURBS::find_derivative_basis_functions(F32 u, S32 i, S32 order, F32 ders[5][5])
{
    // My brain hurts.

    F32 ndu[5][5];
    ndu[0][0] = 1.0f;

    F32 left[5];
    F32 right[5];
    S32 j;
    for (j = 1; j <= p; j++) {
        left[j] = u - knot[i + 1 - j];
        right[j] = knot[i + j] - u;
        F32 saved = 0.0f;
        for (S32 r = 0; r < j; r++) {
            ndu[j][r] = right[r + 1] + left[j - r];
            F32 temp = ndu[r][j - 1] / ndu[j][r];
            ndu[r][j] = temp * right[r + 1] + saved;
            saved = temp * left[j - r];
        }
        ndu[j][j] = saved;
    }
    for (j = 0; j <= p; j++) {
        ders[0][j] = ndu[j][p];
    }

    F32 a[2][5];
    S32 r;
    for (r = 0; r <= p; r++) {
        S32 s1 = 0;
        S32 s2 = 1;
        a[0][0] = 1.0f;
        for (S32 k = 1; k <= order; k++) {
            F32 d = 0.0f;
            S32 rk = r - k;
            S32 pk = p - k;
            if (r >= k) {
                a[s2][0] = a[s1][0] / ndu[pk + 1][rk];
                d = a[s2][0] * ndu[rk][pk];
            }
            S32 j1, j2;
            if (rk >= -1)
                j1 = 1;
            else
                j1 = -rk;
            if (r - 1 <= pk)
                j2 = k - 1;
            else
                j2 = p - r;
            for (S32 j = j1; j <= j2; j++) {
                a[s2][j] = (a[s1][j] - a[s1][j - 1]) / ndu[pk + 1][rk + j];
                d += a[s2][j] * ndu[rk + j][pk];
            }
            if (r <= pk) {
                a[s2][k] = -a[s1][k - 1] / ndu[pk + 1][r];
                d += a[s2][k] * ndu[r][pk];
            }
            ders[k][r] = d;

            S32 temp = s1;
            s1 = s2;
            s2 = temp;
        }
    }

    r = p;
    for (S32 k = 1; k <= order; k++) {
        for (S32 j = 0; j <= p; j++) {
            ders[k][j] *= r;
        }
        r *= p - k;
    }
}

F32 NURBS::start()
{
    return knot[p];
}

F32 NURBS::end()
{
    return knot[n + 1];
}

xVec3 NURBS::evaluate(F32 u)
{
    xVec3 point;
    evaluate(u, point);
    return point;
}

void NURBS::evaluate(F32 u, xVec3& point)
{
    S32 span = find_span(u);
    F32 N[5];
    find_basis_functions(u, span, N);

    point = xVec3::m_Null;
    for (S32 i = 0; i <= p; i++) {
        point += control[span - p + i] * N[i];
    }
}

xVec3 NURBS::evaluate(F32 u, S32 d)
{
    xVec3 point;
    evaluate(u, d, point);
    return point;
}

void NURBS::evaluate(F32 u, S32 d, xVec3& point)
{
    u = CLAMP(u, start(), end());
    point = xVec3::m_Null;

    if (d > p) {
        return;
    }

    S32 span = find_span(u);
    F32 ders[5][5];
    find_derivative_basis_functions(u, span, d, ders);

    for (S32 j = 0; j <= p; j++) {
        point += control[span - p + j] * ders[d][j];
    }
}

xVec3 NURBS::project_point(const xVec3& point, F32* u, F32* distance, F32 guess)
{
    F32 length2;
    xVec3 curve_point = { 0.0f, 0.0f, 0.0f };

    if (guess > HUGE/2) {
        length2 = HUGE;

        for (F32 u = start(); u <= end(); u += 5.0f) {
            xVec3 test_point;
            evaluate(u, test_point);
            F32 test_length2 = (point - test_point).length2();
            if (test_length2 < length2) {
                length2 = test_length2;
                guess = u;
                curve_point = test_point;
            }
        }
    } else {
        evaluate(guess, curve_point);
        length2 = (point - curve_point).length2();
    }

    for (S32 depth = 0; depth < MAX_DEPTH; depth++) {
        if (length2 < 0.01f) {
            if (u)
                *u = guess;
            if (distance) {
                *distance = xsqrt(length2);
            }
            return curve_point;
        }

        xVec3 point_to_curve = {
            curve_point.x - point.x,
            curve_point.y - point.y,
            curve_point.z - point.z
        };

        xVec3 curve_tangent;
        evaluate(guess, 1, curve_tangent);

        // 0.015707318f = sin(0.9deg)
        if (xabs(curve_tangent.dot(point_to_curve)) <
            0.015707318f * xabs(point_to_curve.length() * curve_tangent.length())) {
            if (u)
                *u = guess;
            if (distance) {
                *distance = xsqrt(length2);
            }
            return curve_point;
        }

        xVec3 curve_2nd_derivative;
        evaluate(guess, 2, curve_2nd_derivative);

        F32 last_guess = guess;
        F32 denom = curve_2nd_derivative.dot(point_to_curve) + curve_tangent.length2();
        if (denom < 0.25f) {
            guess += curve_tangent.x * (point.x - curve_point.x) + curve_tangent.y * (point.y - curve_point.y) +
                     curve_tangent.z * (point.z - curve_point.z);
        } else {
            guess -= curve_tangent.dot(point_to_curve) / denom;
        }
        guess = CLAMP(guess, start(), end());

        evaluate(guess, curve_point);

        if (xabs(last_guess - guess) < 0.05f) {
            if (u)
                *u = guess;
            if (distance) {
                *distance = xsqrt(length2);
            }
            return curve_point;
        }
    }

    if (u)
        *u = guess;
    if (distance) {
        *distance = xsqrt(length2);
    }
    return curve_point;
}

NURBS::NURBS(S32 p, S32 knot_count, xVec3* control, F32* knot)
    : p(p), m(knot_count - 1), n(m - p - 1), knot(knot), control(control)
{
}

namespace
{
    struct search_end
    {
        F32 u;
        xVec3 pos;
        F32 dist_to_circle;
    };

    void VerifyBounds(NURBS* curve, F32& begin_u, F32& end_u)
    {
        if (begin_u > end_u) {
            F32 temp = begin_u;
            begin_u = end_u;
            end_u = temp;
        }

        if (begin_u < curve->start()) {
            begin_u = curve->start();
        }

        if (end_u > curve->end()) {
            end_u = curve->end();
        }
    }

    U8 BothEndsOutside(NURBS* curve, search_end& begin, search_end& end, const xVec3& circle_center, F32 circle_radius, F32& intersect_u, xVec3& intersect_vec)
    {
        F32 mid_u = begin.u + 0.5f * (end.u - begin.u);

        if (curve->circle_intersect(intersect_u, intersect_vec, begin.u, mid_u, circle_center, circle_radius) == 1) {
            return 1;
        }

        return curve->circle_intersect(intersect_u, intersect_vec, mid_u, end.u, circle_center, circle_radius) == 1;
    }

    void GetNewGuess(NURBS* curve, search_end* inside, search_end* outside, const xVec3& circle_center, F32 circle_radius, search_end* guess)
    {
        guess->u = inside->u + (outside->u - inside->u) * (circle_radius - inside->dist_to_circle) /
                                   (outside->dist_to_circle - inside->dist_to_circle);
        curve->evaluate(guess->u, guess->pos);
        xVec3 diff;
        diff.x = guess->pos.x - circle_center.x;
        diff.y = guess->pos.y - circle_center.y;
        diff.z = guess->pos.z - circle_center.z;
        guess->dist_to_circle = diff.length();
    }
}

U8 NURBS::circle_intersect(F32& intersect_u, xVec3& intersect_vec, F32 begin_u, F32 end_u, const xVec3& circle_center, F32 circle_radius)
{
    VerifyBounds(this, begin_u, end_u);

    search_end begin;
    search_end end;
    begin.u = begin_u;
    end.u = end_u;
    evaluate(begin.u, begin.pos);
    evaluate(end.u, end.pos);
    xVec3 begin_offset;
    begin_offset.x = begin.pos.x - circle_center.x;
    begin_offset.y = begin.pos.y - circle_center.y;
    begin_offset.z = begin.pos.z - circle_center.z;
    xVec3 end_offset;
    end_offset.x = end.pos.x - circle_center.x;
    end_offset.y = end.pos.y - circle_center.y;
    end_offset.z = end.pos.z - circle_center.z;
    begin.dist_to_circle = begin_offset.length();
    end.dist_to_circle = end_offset.length();

    F32 circle_radius_lower = circle_radius - 0.001f;
    F32 circle_radius_upper = circle_radius + 0.001f;

    search_end* inside;
    search_end* outside;
    if (begin.dist_to_circle < circle_radius_lower) {
        inside = &begin;
        if (end.dist_to_circle < circle_radius_lower) {
            return 0;
        }
        if (end.dist_to_circle < circle_radius_upper) {
            intersect_u = end.u;
            intersect_vec = end.pos;
            return 1;
        }
        outside = &end;
    } else if (begin.dist_to_circle > circle_radius_upper) {
        outside = &begin;
        if (end.dist_to_circle > circle_radius_upper) {
            if (begin_offset.dot(end_offset) < 0.0f) {
                return BothEndsOutside(this, begin, end, circle_center, circle_radius, intersect_u, intersect_vec);
            }
            return 0;
        }
        if (end.dist_to_circle > circle_radius_lower) {
            intersect_u = end.u;
            intersect_vec = end.pos;
            return 1;
        }
        inside = &end;
    } else {
        intersect_u = begin.u;
        intersect_vec = begin.pos;
        return 1;
    }

    search_end third;
    search_end* guess = &third;
    GetNewGuess(this, inside, outside, circle_center, circle_radius, guess);

    S32 iterations = 0;
    static S32 total_it = 0;
    while ((guess->dist_to_circle < circle_radius_lower || guess->dist_to_circle > circle_radius_upper) && iterations < 15) {
        iterations++;
        if (guess->dist_to_circle < circle_radius_lower) {
            search_end* temp = inside;
            inside = guess;
            guess = temp;
        } else {
            search_end* temp = outside;
            outside = guess;
            guess = temp;
        }
        GetNewGuess(this, inside, outside, circle_center, circle_radius, guess);
    }

    intersect_u = guess->u;
    intersect_vec = guess->pos;
    return 1;
}

bool NURBS::advance_u(F32 start_u, F32 distance, bool forward, F32& new_u)
{
    F32 spline_start = start();
    F32 spline_end = end();

    if (start_u < spline_start) start_u = spline_start; else if (start_u > spline_end) start_u = spline_end;

    new_u = start_u;

    xVec3 velocity_vec;
    evaluate(start_u, 1, velocity_vec);

    F32 vel_mag = xVec3NormalizeSafe(velocity_vec);
    if (vel_mag == 0.0f) {
        return false;
    }

    if (forward == true) {
        new_u = start_u + distance / vel_mag;
        if (new_u > spline_end) {
            new_u = spline_end;
            return false;
        }
    } else {
        new_u = start_u - distance / vel_mag;
        if (new_u < spline_start) {
            new_u = spline_start;
            return false;
        }
    }

    return true;
}

} // namespace x

#ifndef ZTRANSIENT_H
#define ZTRANSIENT_H

#include "zHitParameters.h"
#include "xBound.h"

struct zFlame;

namespace zTransient
{
    struct instance_data
    {
    };

    struct damage_result
    {
        S32 type;
        F32 damage;
        F32 knockback;
        zHitSource source;
        xBound bound;
        union
        {
            void* object;
            zFlame* flame;
            instance_data* explosion;
        };
    };

    S32 check_damage(damage_result* results, const xBox& box, S32 max_results, S32,
                       S32 type_filter);
} // namespace zTransient

#endif
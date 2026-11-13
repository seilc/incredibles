#ifndef ZREACTIVEANIMATION_H
#define ZREACTIVEANIMATION_H

#include "zBase.h"
#include "zEntSimpleObj.h"
#include "zHitParameters.h"

S32 zReactiveHitTestRay(zEntSimpleObj** hits, S32 max_hits, const xRay3& ray);
void zReactiveDamage(zEntSimpleObj& ent, zHitSource hs, F32 damage);

inline bool zReactiveIsReactive(xEnt& ent)
{
    if (ent.baseType != eBaseTypeStatic) {
        return false;
    }

    zEntSimpleObj& simp = (zEntSimpleObj&)ent;
    return simp.sflags >> 9 & 1;
}

#endif

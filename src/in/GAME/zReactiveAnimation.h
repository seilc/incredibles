#ifndef ZREACTIVEANIMATION_H
#define ZREACTIVEANIMATION_H

#include "zBase.h"
#include "zEntSimpleObj.h"
#include "zHitParameters.h"

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

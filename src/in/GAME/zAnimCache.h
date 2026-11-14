#ifndef ZANIMCACHE_H
#define ZANIMCACHE_H

#include "xAnim.h"

struct zAnimCacheEntry
{
    xVec3* data;
    xAnimFile* animFile;
    U16 bone;
};

struct zAttackTableState;

void zAnimCacheCreateData(zAttackTableState* attackTableStates, U32 attackTableCount,
                          xAnimPlay* animPlay);

#endif
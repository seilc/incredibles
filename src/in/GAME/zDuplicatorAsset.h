#ifndef ZDUPLICATORASSET_H
#define ZDUPLICATORASSET_H

#include "xBaseAsset.h"

struct zDuplicatorAsset : xBaseAsset
{
    U16 initial_spawn;
    U16 maximum_in_game;
    U16 maximum_to_spawn;
    F32 spawn_rate;
};

#endif

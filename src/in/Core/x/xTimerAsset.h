#ifndef XTIMERASSET_H
#define XTIMERASSET_H

#include "xBaseAsset.h"

struct xTimerAsset : xBaseAsset
{
    F32 seconds;
    F32 randomRange;
};

#endif

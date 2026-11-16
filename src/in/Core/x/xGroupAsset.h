#ifndef XGROUPASSET_H
#define XGROUPASSET_H

#include "xBaseAsset.h"

struct xGroupAsset : xBaseAsset
{
    U16 itemCount;
    U16 groupFlags;
};

#endif

#ifndef XBASEASSET_H
#define XBASEASSET_H

#include "types.h"

struct xBaseAsset
{
    U32 id;
    U8 baseType;
    U8 linkCount;
    U16 baseFlags;
};

#endif

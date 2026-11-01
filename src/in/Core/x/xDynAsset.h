#ifndef XDYNASSET_H
#define XDYNASSET_H

#include "xBase.h"
#include "xstransvc.h"
#include "xString.h"

struct xDynAsset : xBaseAsset
{
    U32 type;
    U16 version;
    U16 handle;
};

#endif
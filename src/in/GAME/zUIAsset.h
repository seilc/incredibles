#ifndef ZUIASSET_H
#define ZUIASSET_H

#include "xDynAsset.h"

struct zUIAsset : xDynAsset
{
    F32 x;
    F32 y;
    F32 z;
    F32 width;
    F32 height;
    S32 flags;
    U32 color;
    U32 selectedMotion;
    U32 unselectedMotion;
    U8 brightness;
    U8 pad[3];
    U32 autoMenuUp;
    U32 autoMenuDown;
    U32 autoMenuLeft;
    U32 autoMenuRight;
    U32 custom;
    U32 customWidget;
};

#endif

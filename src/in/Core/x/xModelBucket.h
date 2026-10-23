#ifndef XMODELBUCKET_H
#define XMODELBUCKET_H

#include "xModel.h"

struct xModelBucket
{
    RpAtomic* Data;
    RpAtomic* OriginalData;
    union
    {
        xModelInstance* List;
        xModelBucket** BackRef;
    };
    S32 ClipFlags;
    xModelPipe Pipe;
};

#endif
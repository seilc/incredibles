#ifndef XENTHANGABLEASSET_H
#define XENTHANGABLEASSET_H

#include "types.h"

struct xEntHangableAsset
{
    U32 flags;
    F32 pivotOffset;
    F32 leverArm;
    F32 gravity;
    F32 accel;
    F32 decay;
    F32 grabDelay;
    F32 stopDecel;
};

#endif

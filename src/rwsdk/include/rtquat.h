#ifndef RTQUAT_H
#define RTQUAT_H

#include <math.h>
#include "rwplcore.h"

typedef struct RtQuat RtQuat;

struct RtQuat
{
    RwV3d imag;
    RwReal real;
};

#endif

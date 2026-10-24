#ifndef IFOG_H
#define IFOG_H

#include <rwcore.h>
#include "types.h"

struct iFogParams
{
    RwFogType type;
    F32 start;
    F32 stop;
    F32 density;
    RwRGBA fogcolor;
    RwRGBA bgcolor;
    U8* table;
};

#endif
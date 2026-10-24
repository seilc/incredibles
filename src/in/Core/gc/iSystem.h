#ifndef ISYSTEM_H
#define ISYSTEM_H

#include "types.h"

#ifdef GAMECUBE
U32 iGetDay();
U32 iGetMonth();
#else
U8 iGetDay();
U8 iGetMonth();
#endif

#endif
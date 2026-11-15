#ifndef _FMODGC_H_
#define _FMODGC_H_

#include "fmod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*FSOUND_GC_DVDSTATUSCALLBACK)(int status);

signed char FSOUND_AUXFX_ShutdownEffect(int);
void FSOUND_GC_SetUpdateDvdStatusCallback(FSOUND_GC_DVDSTATUSCALLBACK callback);
void FSOUND_GC_SetDvdStatusCallback(FSOUND_GC_DVDSTATUSCALLBACK callback);

#ifdef __cplusplus
}
#endif

#endif

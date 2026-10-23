#ifndef ZAUTOTWEAK_H
#define ZAUTOTWEAK_H

#include "xDebugTweak.h"

#include <string.h>

#if DEBUG || RELEASE
namespace auto_tweak {

template <class T, class T2>
static inline void add_tweak(const char* name, T* v, T2 vmin, T2 vmax, const tweak_callback* cb, void* context, U32 flags) { xDebugAddTweak(name, v, vmin, vmax, cb, context, flags); }

}

#define zAUTOTWEAK(prefix, name, v, vmin, vmax, cb, context, flags) do { char _buffer[128]; strcpy(_buffer, prefix); strcat(_buffer, name); auto_tweak::add_tweak(_buffer, v, vmin, vmax, cb, context, flags); } while (0)
#else
#define zAUTOTWEAK(prefix, name, v, vmin, vmax, cb, context, flags)
#endif

#endif

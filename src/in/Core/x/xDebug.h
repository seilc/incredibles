#ifndef XDEBUG_H
#define XDEBUG_H

#include "iDebug.h"
#include "xString.h"

#if DEBUG || RELEASE
#define _xSTR(x) #x

#define _xFAIL(line, condstr, ...)                                                                \
{                                                                                                 \
    xDebug_assert2_info(__FUNCTION__, __FILE__, line, condstr);                                   \
    xDebug_assert2(__VA_ARGS__);                                                                  \
    xDebugStackTrace();                                                                           \
    if (xDebugBoing()) iDebugBreak();                                                             \
}

#define _xASSERT(line, cond, condstr, ...)                                                        \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        _xFAIL(line, condstr, __VA_ARGS__);                                                       \
    }                                                                                             \
} while (0)

#define xASSERT(cond) _xASSERT(__LINE__, cond, _xSTR(cond), "%s", #cond)
#define xASSERT_L(line, cond) _xASSERT(line, cond, _xSTR(cond), "%s", #cond)
#define xASSERT_MSG(cond, msg) _xASSERT(__LINE__, cond, _xSTR(cond), "%s", msg)
#define xASSERT_MSG_L(line, cond, msg) _xASSERT(line, cond, _xSTR(cond), "%s", msg)
#define xASSERT_FMT(cond, ...) _xASSERT(__LINE__, cond, #cond, __VA_ARGS__)
#define xASSERT_FMT_L(line, cond, ...) _xASSERT(line, cond, #cond, __VA_ARGS__)

#define xFAIL_L(line) do { _xFAIL(line, "*always*", "%s", "*always*") } while (0)
#define xFAIL_FMT_L(line, ...) do { _xFAIL(line, "*always*", __VA_ARGS__) } while (0)

#define xFAIL_ONCE_L(line, ...)                                                                   \
do {                                                                                              \
    static bool done = false;                                                                     \
    if (!done) {                                                                                  \
        done = true;                                                                              \
        _xFAIL(line, "*once only*", __VA_ARGS__);                                                 \
    }                                                                                             \
} while (0)

#define _xASSERT_ONCE(line, cond, condstr)                                                        \
do {                                                                                              \
    static bool done = false;                                                                     \
    if (!done && !(cond)) {                                                                       \
        xFAIL_FMT_L(line, "%s", condstr);                                                         \
        done = true;                                                                              \
    }                                                                                             \
} while (0)

#define xASSERT_ONCE(cond) _xASSERT_ONCE(__LINE__, cond, #cond)
#define xASSERT_ONCE_L(line, cond) _xASSERT_ONCE(line, cond, #cond)

#define _xASSERT_DESIGN(line, cond, condstr, msg)                                                 \
do {                                                                                              \
    static bool already_asserted = false;                                                         \
    if (!already_asserted && !(cond)) {                                                           \
        already_asserted = true;                                                                  \
        _xFAIL(line, condstr, msg);                                                               \
    }                                                                                             \
} while (0)

#define xASSERT_DESIGN(cond, msg) _xASSERT_DESIGN(__LINE__, cond, _xSTR(cond), msg)
#define xASSERT_DESIGN_L(line, cond, msg) _xASSERT_DESIGN(line, cond, _xSTR(cond), msg)
#else
#define _xFAIL(line, condstr, ...)
#define xASSERT(cond)
#define xASSERT_L(line, cond)
#define xASSERT_MSG(cond, msg)
#define xASSERT_MSG_L(line, cond, msg)
#define xASSERT_FMT(cond, ...)
#define xASSERT_FMT_L(line, cond, ...)
#define xFAIL_L(line)
#define xFAIL_FMT_L(line, ...)
#define xFAIL_ONCE_L(line, ...)
#define xASSERT_ONCE(cond)
#define xASSERT_ONCE_L(line, cond)
#define xASSERT_DESIGN(cond, msg)
#define xASSERT_DESIGN_L(line, cond, msg)
#endif

#define xFAIL() xFAIL_L(__LINE__)
#define xFAIL_FMT(...) xFAIL_FMT_L(__LINE__, __VA_ARGS__)
#define xFAIL_ONCE(...) xFAIL_ONCE_L(__LINE__, __VA_ARGS__)

#define _xFAIL_RETURN_IF(line, cond, condstr, retval)                                             \
do {                                                                                              \
    if ((cond)) {                                                                                 \
        _xFAIL(line, "*always*", "%s", condstr)                                                   \
        return retval;                                                                            \
    }                                                                                             \
} while (0)

#define xFAIL_RETURN_IF(cond) _xFAIL_RETURN_IF(__LINE__, cond, #cond, )
#define xFAIL_RETURN_IF_L(line, cond) _xFAIL_RETURN_IF(line, cond, #cond, )
#define xFAIL_RETURN_VALUE_IF(cond, retval) _xFAIL_RETURN_IF(__LINE__, cond, #cond, (retval))
#define xFAIL_RETURN_VALUE_IF_L(line, cond, retval) _xFAIL_RETURN_IF(line, cond, #cond, (retval))

#define xFIX_NAN_L(line, x)                                                                       \
if ((x) && !((x) > 0.0f) && !((x) < 0.0f)) {                                                      \
    static bool done = false;                                                                     \
    if (!done) {                                                                                  \
        xFAIL_FMT_L(line, "%s", "NAN");                                                           \
        done = true;                                                                              \
    }                                                                                             \
    (x) = 0.0f;                                                                                   \
}

#define xFIX_NAN(x) xFIX_NAN_L(__LINE__, x)

extern U32 gFrameCount;
extern U32 gSceneFrameCount;

void xprintf(const char*, ...);
void xDebugInit();
void xDebugUpdate();
void xDebugExit();
void xDebugTimestampScreen();
void xDrawTextInit(U32, U32);
void xDrawTextRender();

#endif
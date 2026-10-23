#ifndef XDEBUG_H
#define XDEBUG_H

#include "iDebug.h"
#include "xString.h"

#if DEBUG || RELEASE
#define _xSTR(x) #x

#define _xASSERTFAIL(line, condstr, ...)                                                          \
{                                                                                                 \
    xDebug_assert2_info(__FUNCTION__, __FILE__, line, condstr);                                   \
    xDebug_assert2(__VA_ARGS__);                                                                  \
    xDebugStackTrace();                                                                           \
    if (xDebugBoing()) iDebugBreak();                                                             \
}

#define xASSERT(line, cond)                                                                       \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        _xASSERTFAIL(line, _xSTR(cond), "%s", #cond);                                             \
    }                                                                                             \
} while (0)

#define xASSERTMSG(line, cond, msg)                                                               \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        _xASSERTFAIL(line, _xSTR(cond), "%s", msg);                                               \
    }                                                                                             \
} while (0)

#define xASSERTM(line, cond, ...)                                                                 \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        _xASSERTFAIL(line, #cond, __VA_ARGS__);                                                   \
    }                                                                                             \
} while (0)

#define xFAIL(line) do { _xASSERTFAIL(line, "*always*", "%s", "*always*") } while (0)
#define xFAILM(line, ...) do { _xASSERTFAIL(line, "*always*", __VA_ARGS__) } while (0)

#define xFAIL_ONCE_M(line, ...)                                                                   \
do {                                                                                              \
    static bool done = false;                                                                     \
    if (!done) {                                                                                  \
        done = true;                                                                              \
        _xASSERTFAIL(line, "*once only*", __VA_ARGS__);                                           \
    }                                                                                             \
} while (0)

#define xASSERT_ONCE(line, cond)                                                                  \
do {                                                                                              \
    static bool done = false;                                                                     \
    if (!done && !(cond)) {                                                                       \
        xFAILM(line, "%s", #cond);                                                                \
        done = true;                                                                              \
    }                                                                                             \
} while (0)

#define xASSERT_DESIGN_M(line, cond, msg)                                                         \
do {                                                                                              \
    static bool already_asserted = false;                                                         \
    if (!already_asserted && !(cond)) {                                                           \
        already_asserted = true;                                                                  \
        _xASSERTFAIL(line, _xSTR(cond), msg);                                                     \
    }                                                                                             \
} while (0)
#else
#define _xASSERTFAIL(line, condstr, ...)
#define xASSERT(line, cond)
#define xASSERTMSG(line, cond, msg)
#define xASSERTM(line, cond, ...)
#define xFAIL(line)
#define xFAILM(line, ...)
#define xFAIL_ONCE_M(line, ...)
#define xASSERT_ONCE(line, cond)
#define xASSERT_DESIGN_M(line, cond, msg)
#endif

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
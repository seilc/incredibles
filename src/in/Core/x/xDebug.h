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

struct tweak_callback;

struct tweak_info
{
    substr name;
    void* value;
    tweak_callback* cb;
    void* context;
    U8 type;
    U8 value_size;
    U16 flags;
    union
    {
        struct
        {
            S32 value_def;
            S32 value_min;
            S32 value_max;
        } int_context;
        struct
        {
            U32 value_def;
            U32 value_min;
            U32 value_max;
        } uint_context;
        struct
        {
            F32 value_def;
            F32 value_min;
            F32 value_max;
        } float_context;
        struct
        {
            bool value_def;
        } bool_context;
        struct
        {
            U32 value_def;
            U32 labels_size;
            char** labels;
            void* values;
        } select_context;
        struct
        {
            U32 value_def;
            U32 mask;
        } flag_context;
        struct
        {
            U8 pad[16];
        } all_context;
    };
};

struct tweak_callback
{
    void (*on_change)(tweak_info&);
    void (*on_select)(tweak_info&);
    void (*on_unselect)(tweak_info&);
    void (*on_start_edit)(tweak_info&);
    void (*on_stop_edit)(tweak_info&);
    void (*on_expand)(tweak_info&);
    void (*on_collapse)(tweak_info&);
    void (*on_update)(tweak_info&);
    void (*convert_mem_to_tweak)(tweak_info&, void*);
    void (*convert_tweak_to_mem)(tweak_info&, void*);
};

extern U32 gFrameCount;
extern U32 gSceneFrameCount;

void xprintf(const char*, ...);
void xDebugInit();
void xDebugUpdate();
void xDebugExit();
void xDebugTimestampScreen();
void xDebugRemoveTweak(const char*);
void xDrawTextInit(U32, U32);
void xDrawTextRender();

inline void xDebugRemoveTweaks(const char*, bool (*)(const tweak_info&, void*), void*)
{
}

#endif
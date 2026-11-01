#ifndef XDEBUGTWEAK_H
#define XDEBUGTWEAK_H

#include "xString.h"

struct tweak_info;

struct tweak_callback
{
    void(*on_change)(const tweak_info&);
    void(*on_select)(const tweak_info&);
    void(*on_unselect)(const tweak_info&);
    void(*on_start_edit)(const tweak_info&);
    void(*on_stop_edit)(const tweak_info&);
    void(*on_expand)(const tweak_info&);
    void(*on_collapse)(const tweak_info&);
    void(*on_update)(const tweak_info&);
    void(*convert_mem_to_tweak)(const tweak_info&, void*);
    void(*convert_tweak_to_mem)(const tweak_info&, void*);
};

struct tweak_info
{
    substr name;
    void* value;
    const tweak_callback* cb;
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
            const char** labels;
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

inline void xDebugAddTweak(const char* name, const F32* v, F32 vmin, F32 vmax, const tweak_callback* cb, void* context, U32 flags) {}

void xDebugRemoveTweak(const char* name);
inline void xDebugRemoveTweaks(const char*, bool (*)(const tweak_info&, void*), void*) {}

#define xTWEAK(name, v, vmin, vmax, cb, context, flags) xDebugAddTweak(name, v, vmin, vmax, cb, context, flags)

#define xTWEAKABLE const

#endif

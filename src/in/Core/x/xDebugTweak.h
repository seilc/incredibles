#ifndef XDEBUGTWEAK_H
#define XDEBUGTWEAK_H

#include "xString.h"

#include <string.h>

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

    static tweak_callback create_change(void(*on_change)(const tweak_info&));
    static tweak_callback create_expand(void(*on_expand)(const tweak_info&), void(*on_collapse)(const tweak_info&));

    static tweak_callback create_update(void(*on_update)(const tweak_info&))
    {
        tweak_callback cb = {
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            on_update,
            NULL,
            NULL
        };
        return cb;
    }
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

#if DEBUG || RELEASE
void* xDebugTweak_StaticAlloc(U32 size);

void xDebugAddTweak(const char* name, S8* v, S8 vmin, S8 vmax, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, U32* v, U32 vmin, U32 vmax, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, U16* v, U16 vmin, U16 vmax, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, U8* v, U8 vmin, U8 vmax, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, F32* v, F32 vmin, F32 vmax, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddSelectTweak(const char* name, S8* v, const char** labels, const S8* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddSelectTweak(const char* name, U32* v, const char** labels, const U32* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddFlagTweak(const char* name, U32* v, U32 mask, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddBranchTweak(const char* name, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, bool* v, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, const char* message, const tweak_callback* cb, void* context, U32 flags);

void _auto_tweak(const char* prefix, const char* name, S32* v, S32 vmin, S32 vmax, const tweak_callback* cb, void* context, U32 flags, bool process);
void _auto_tweak(const char* prefix, const char* name, U8* v, U8 vmin, U8 vmax, const tweak_callback* cb, void* context, U32 flags, bool process);
void _auto_tweak(const char* prefix, const char* name, F32* v, F32 vmin, F32 vmax, const tweak_callback* cb, void* context, U32 flags, bool process);
void _auto_tweak(const char* prefix, const char* name, bool* v, const tweak_callback* cb, void* context, U32 flags, bool process);

void _auto_tweak_select(const char* prefix, const char* name, U32* v, const char** labels, const U32* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags, bool process);
void _auto_tweak_flag(const char* prefix, const char* name, U32* v, U32 mask, const tweak_callback* cb, void* context, U32 flags);

typedef bool(*xDebugRemoveTweaksCallback)(const tweak_info&, void*);

void xDebugRemoveTweak(const char* name);
void xDebugRemoveTweaks(const substr& name, xDebugRemoveTweaksCallback cb, void* context);

inline void xDebugRemoveTweaks(const char* name, xDebugRemoveTweaksCallback cb, void* context)
{ xDebugRemoveTweaks(substr::create(name, strlen(name)), cb, context); }

inline void _auto_tweak_flag(const char* prefix, const char* name, S32* v, S32 mask, const tweak_callback* cb, void* context, U32 flags)
{ _auto_tweak_flag(prefix, name, (U32*)v, (U32)mask, cb, context, flags); }

template <class T, class T2>
inline void _t_auto_tweak(const char* prefix, const char* name, T* v, T2 vmin, T2 vmax, const tweak_callback* cb, void* context, U32 flags, bool process)
{ _auto_tweak(prefix, name, v, vmin, vmax, cb, context, flags, process); }

template <class T, class T2>
inline void _t_auto_tweak_select(const char* prefix, const char* name, T* v, const char** labels, const T* values, T2 labels_size, const tweak_callback* cb, void* context, U32 flags, bool process)
{ _auto_tweak_select(prefix, name, v, labels, values, labels_size, cb, context, flags, process); }

template <class T, class T2>
inline void _t_auto_tweak_flag(const char* prefix, const char* name, T* v, T2 mask, const tweak_callback* cb, void* context, U32 flags)
{ _auto_tweak_flag(prefix, name, v, mask, cb, context, flags); }

template <>
inline void _t_auto_tweak(const char* prefix, const char* name, bool* v, S32 vmin, S32 vmax, const tweak_callback* cb, void* context, U32 flags, bool process)
{ _auto_tweak(prefix, name, v, cb, context, flags, process); }

#define xTWEAK(name, v, vmin, vmax, cb, context, flags) xDebugAddTweak(name, v, vmin, vmax, cb, context, flags)
#define xTWEAKSELECT(name, v, labels, values, labels_size, cb, context, flags) xDebugAddSelectTweak(name, v, labels, values, labels_size, cb, context, flags)
#define xTWEAKFLAG(name, v, mask, cb, context, flags) xDebugAddFlagTweak(name, v, mask, cb, context, flags)
#define xTWEAKBRANCH(name, cb, context, flags) xDebugAddBranchTweak(name, cb, context, flags)
#define xTWEAKBOOL(name, v, cb, context, flags) xDebugAddTweak(name, v, cb, context, flags)
#define xTWEAKMESSAGE(name, message, cb, context, flags) xDebugAddTweak(name, message, cb, context, flags)
#define xTWEAKREMOVE(name) xDebugRemoveTweak(name)

#define xAUTOTWEAKRANGE(prefix, name, v, vmin, vmax, cb, context, flags, process) _t_auto_tweak(prefix, name, v, vmin, vmax, cb, context, flags, process)
#define xAUTOTWEAKBOOL(prefix, name, v, cb, context, flags, process) _t_auto_tweak(prefix, name, v, 0, 0, cb, context, flags, process)
#define xAUTOTWEAKSELECT(prefix, name, v, labels, values, labels_size, cb, context, flags, process) _t_auto_tweak_select(prefix, name, v, labels, values, labels_size, cb, context, flags, process)
#define xAUTOTWEAKFLAG(prefix, name, v, mask, cb, context, flags) _t_auto_tweak_flag(prefix, name, v, mask, cb, context, flags)

#define xTWEAKABLE
#else
inline void xDebugAddTweak(const char* name, const U32* v, U32 vmin, U32 vmax, const tweak_callback* cb, void* context, U32 flags) {}
inline void xDebugAddTweak(const char* name, const F32* v, F32 vmin, F32 vmax, const tweak_callback* cb, void* context, U32 flags) {}
inline void xDebugAddBranchTweak(const char* name, const tweak_callback* cb, void* context, U32 flags) {}

void xDebugRemoveTweak(const char* name);
inline void xDebugRemoveTweaks(const char*, bool (*)(const tweak_info&, void*), void*) {}

#define xTWEAK(name, v, vmin, vmax, cb, context, flags) xDebugAddTweak(name, v, vmin, vmax, cb, context, flags)
#define xTWEAKSELECT(name, v, labels, values, labels_size, cb, context, flags)
#define xTWEAKFLAG(name, v, mask, cb, context, flags)
#define xTWEAKBRANCH(name, cb, context, flags)
#define xTWEAKBOOL(name, v, cb, context, flags)
#define xTWEAKMESSAGE(name, message, cb, context, flags)
#define xTWEAKREMOVE(name) xDebugRemoveTweak(name)

#define xAUTOTWEAKRANGE(prefix, name, v, vmin, vmax, cb, context, flags, process)
#define xAUTOTWEAKBOOL(prefix, name, v, cb, context, flags, process)
#define xAUTOTWEAKSELECT(prefix, name, v, labels, values, labels_size, cb, context, flags, process)
#define xAUTOTWEAKFLAG(prefix, name, v, mask, cb, context, flags)

#define xTWEAKABLE const
#endif

#endif

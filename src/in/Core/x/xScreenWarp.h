#ifndef XSCREENWARP_H
#define XSCREENWARP_H

#include "xMath2.h"
#include "xMath3.h"

#include <rwcore.h>

namespace xScreenWarp {

    enum type_enum
    {
        TYPE_INVALID = -1,
        TYPE_WARBLE,
        TYPE_LENS,
        TYPE_RIPPLE,
        TYPE_SHOCKWAVE,
        TYPE_HEAT_SHIMMER,
        TYPE_STATIC_LENS,
        MAX_TYPE
    };

    enum
    {
        FLAG_1 = 0x1,
        FLAG_2 = 0x2,
        FLAG_INFINITE = 0x4,
        FLAG_8 = 0x8
    };

    struct warper;

    typedef void(*animate_callback)(xVec2*, warper&, const xVec2&, F32, F32, F32, F32);

    extern const char* common_warp_names[MAX_TYPE + 1];
    extern animate_callback common_warp_animate[MAX_TYPE];

    void scene_enter();
    void scene_exit();
    void reset();
    void update(F32 dt);
    void pre_render();
    void render(RwCamera* cam);
    void clear();
    type_enum find_type(U32 id);
    warper* emit(const xVec3& loc, F32 radius, F32 duration, type_enum type, F32 intensity, F32 freq, S32 flags);
    void modify(warper* warp, const xVec3& loc, F32 radius, F32 duration, type_enum type, F32 intensity, F32 freq, S32 flags);
    void remove(warper* warp);

}

#endif

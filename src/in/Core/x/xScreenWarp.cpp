#include "xScreenWarp.h"

#include "xDebugTweak.h"
#include "xGlobals.h"
#include "xMemMgr.h"
#include "xRand.h"
#include "xString.h"
#include "xutil.h"
#include "iMath.h"

#include <string.h>

using namespace xScreenWarp;

struct xScreenWarp::warper
{
    warper* next;
    warper* prev;
    S32 flags;
    F32 life;
    F32 iduration;
    F32 radius;
    F32 intensity;
    F32 freq;
    type_enum type;
    union
    {
        xVec3 loc3;
        xVec2 loc2;
        const xVec3* ploc3;
        const xVec2* ploc2;
    };
};

namespace {

    namespace tweak {

        xTWEAKABLE F32 radius_cull = 0.1f;
        xTWEAKABLE F32 radius_fade = 0.2f;

        namespace warble {

            xTWEAKABLE F32 mag = 0.1f;

        }

        namespace ripple {

            xTWEAKABLE F32 mag = 0.1f;
            xTWEAKABLE F32 freq = 1.0f;

        }

        namespace shockwave {

            xTWEAKABLE F32 mag = 0.02f;
            xTWEAKABLE F32 freq = 1.5f;
            xTWEAKABLE F32 freq2 = 2.5f;

        }

        namespace heat {

            xTWEAKABLE F32 mag = 0.015f;
            xTWEAKABLE F32 freq = 4.0f;
            xTWEAKABLE F32 dthetax = 25.132742f;
            xTWEAKABLE F32 dthetay = 13.962634f;
            xTWEAKABLE F32 dxcos0 = 0.0f;
            xTWEAKABLE F32 dxsin0 = 1.0f;
            xTWEAKABLE F32 dycos0 = 0.6440866f;
            xTWEAKABLE F32 dysin0 = 0.7649526f;

        }

        namespace debug {

            xTWEAKABLE bool enable = true;

        }

    }

    xVec2* uvmap_buffer;
    F32 uvmap_uoffset;
    F32 uvmap_voffset;
    F32 uvmap_uscale;
    F32 uvmap_vscale;
    F32 uvmap_udelta;
    F32 uvmap_vdelta;
    F32 uvmap_xscale;
    F32 uvmap_yscale;
    F32 uvmap_xdelta;
    F32 uvmap_ydelta;
    U32 common_warp_ids[MAX_TYPE];
    warper warp_buffer[128];
    warper warp_lists[MAX_TYPE];
    warper* warp_stack;
    bool render_this_frame;

    warper* alloc_warp(type_enum type)
    {
        warper* warp = warp_stack;
        warp_stack = warp->next;

        warper& head = warp_lists[type];
        warp->next = head.next;
        warp->prev = &head;
        if (head.next)
            head.next->prev = warp;
        head.next = warp;

        return warp;
    }

    void free_warp(warper* warp)
    {
        if (warp->next)
            warp->next->prev = warp->prev;
        warp->prev->next = warp->next;
        warp->next = warp_stack;
        warp_stack = warp;
    }

    void change_warp_type(warper* warp, type_enum type)
    {
        if (warp->type == type) return;

        if (warp->next) {
            warp->next->prev = warp->prev;
        }
        warp->prev->next = warp->next;

        warper& head = warp_lists[type];
        warp->next = head.next;
        warp->prev = &head;
        if (head.next)
            head.next->prev = warp;
        head.next = warp;

        warp->type = type;
    }

    void set_warp(warper& warp, F32 radius, F32 duration, type_enum type, F32 intensity, F32 freq, S32 flags)
    {
        warp.iduration = 1.0f / duration;
        warp.radius = radius;
        warp.type = type;
        warp.intensity = intensity;
        warp.freq = freq;
        warp.flags = flags;
    }

    warper* create_warp(F32 radius, F32 duration, type_enum type, F32 intensity, F32 freq, S32 flags)
    {
        warper* warp = alloc_warp(type);
        set_warp(*warp, radius, duration, type, intensity, freq, flags);
        warp->life = duration;
        return warp;
    }

    void reset_uvmap(xVec2* uvmap)
    {
        xVec2* uv = uvmap;
        F32 du = uvmap_udelta, dv = uvmap_vdelta;
        F32 u = uvmap_uoffset;
        for (S32 i = 0; i < 17; i++, u += du) {
            F32 v = uvmap_voffset;
            for (S32 j = 0; j < 17; v += dv, uv++, j++)
                uv->assign(u, v);
        }
    }

    bool animate_uvmap(xVec2* uvmap, RwCamera* cam)
    {
        const xMat4x3& view_mat = *(const xMat4x3*)RwCameraGetViewMatrix(cam);

        bool need_render = false;

        for (S32 type = 0; type < MAX_TYPE; type++) {
            animate_callback const animate_cb = common_warp_animate[type];
            for (warper* warp = warp_lists[type].next; warp != NULL; warp = warp->next) {
                xVec2 loc;
                F32 radius, iz;

                if (warp->flags & FLAG_2) {
                    loc = (warp->flags & FLAG_1) ? *warp->ploc2 : warp->loc2;
                    radius = warp->radius;
                    iz = 1.0f;
                } else {
                    xVec3 loc3;

                    if (warp->flags & FLAG_1)
                        xMat4x3Toworld(&loc3, &view_mat, warp->ploc3);
                    else
                        xMat4x3Toworld(&loc3, &view_mat, &warp->loc3);
                    if (loc3.z <= 0.01f) continue;

                    iz = 1.0f / loc3.z;
                    loc.x = loc3.x * iz;
                    loc.y = loc3.y * iz;
                    radius = warp->radius * iz;
                }

                if (radius <= tweak::radius_cull) continue;

                if (loc.x - radius > 1.0f) continue;

                if (loc.x + radius < 0.0f) continue;

                if (loc.y - radius > 1.0f) continue;

                if (loc.y + radius < 0.0f) continue;

                if (!need_render) {
                    reset_uvmap(uvmap);
                    need_render = true;
                }

                F32 intensity;
                if (radius > tweak::radius_fade) {
                    intensity = warp->intensity;
                } else {
                    intensity = warp->intensity * (radius - tweak::radius_cull) * (1.0f / (tweak::radius_fade - tweak::radius_cull));
                }

                loc.x *= uvmap_xscale;
                loc.y *= uvmap_yscale;

                F32 s;
                if (warp->flags & FLAG_8) {
                    s = 0.5f;
                } else {
                    s = warp->life * warp->iduration;
                }

                animate_cb(uvmap, *warp, loc, radius, intensity, iz, s);
            }
        }

        return need_render;
    }

    inline void debug_init()
    {
    }

    inline void debug_update(F32)
    {
    }

    void render_scene_enter();
    void render_scene_exit();
    void render_uvmap(RwCamera*);

}

void xScreenWarp::scene_enter()
{
    uvmap_uscale = 1.0f;
    uvmap_vscale = 1.0f;
    uvmap_xscale = 1.0f;
    uvmap_yscale = (F32)FB_YRES / (F32)FB_XRES;

    render_scene_enter();

    uvmap_udelta = uvmap_uscale / 16.0f;
    uvmap_vdelta = uvmap_vscale / 16.0f;
    uvmap_xdelta = uvmap_xscale / 16.0f;
    uvmap_ydelta = uvmap_yscale / 16.0f;

    debug_init();
    for (S32 i = 0; i < MAX_TYPE; i++)
        common_warp_ids[i] = xStrHash(common_warp_names[i]);

    reset();
}

void xScreenWarp::scene_exit()
{
    render_scene_exit();
}

void xScreenWarp::reset()
{
    clear();
}

void xScreenWarp::update(F32 dt)
{
    if (!tweak::debug::enable) return;

    debug_update(dt);

    for (S32 type = 0; type < MAX_TYPE; type++) {
        for (warper* warp = warp_lists[type].next; warp != NULL;) {
            warper* next_warp = warp->next;
            warp->life -= dt;
            if (warp->flags & FLAG_INFINITE) {
                if (warp->life < 0.0f) {
                    warp->life *= warp->iduration;
                    warp->life -= (S32)warp->life - 1.0f;
                    warp->life /= warp->iduration;
                }
            }
            else if (warp->life <= 0.0f) {
                free_warp(warp);
            }
            warp = next_warp;
        }
    }
}

void xScreenWarp::pre_render()
{
}

void xScreenWarp::render(RwCamera* cam)
{
    if (!tweak::debug::enable) return;

    uvmap_buffer = (xVec2*)xMEMPUSHTEMP((17 * 17 + 1) * sizeof(xVec2));
    if (uvmap_buffer == NULL) return;

    render_this_frame = animate_uvmap(uvmap_buffer, cam);

    if (render_this_frame) {
        render_uvmap(cam);
    }

    xMEMPOPTEMP(uvmap_buffer);
}

void xScreenWarp::clear()
{
    memset(warp_lists, 0, sizeof(warp_lists));
    warper* warp = warp_buffer, *end_warp = warp + ARRAY_LENGTH(warp_buffer) - 1;
    for (; warp != end_warp; warp++)
        warp->next = warp + 1;
    warp->next = NULL;
    warp_stack = warp_buffer;
}

type_enum xScreenWarp::find_type(U32 id)
{
    for (S32 i = 0; i < MAX_TYPE; i++)
        if (common_warp_ids[i] == id)
            return (type_enum)i;
    return TYPE_INVALID;
}

warper* xScreenWarp::emit(const xVec3& loc, F32 radius, F32 duration, type_enum type, F32 intensity, F32 freq, S32 flags)
{
    if (!tweak::debug::enable)
        return NULL;
    if (warp_stack == NULL) {
        return NULL;
    }

    warper* warp = create_warp(radius, duration, type, intensity, freq, flags);

    if (warp->flags & FLAG_1)
        warp->ploc3 = &loc;
    else
        warp->loc3 = loc;

    if (warp->flags & FLAG_INFINITE)
        return warp;
    return NULL;
}

void xScreenWarp::modify(warper* warp, const xVec3& loc, F32 radius, F32 duration, type_enum type, F32 intensity, F32 freq, S32 flags)
{
    change_warp_type(warp, type);
    set_warp(*warp, radius, duration, type, intensity, freq, flags);
    if (warp->life > duration)
        warp->life = xfmod(warp->life, duration);
    if (warp->flags & FLAG_1)
        warp->ploc3 = &loc;
    else
        warp->loc3 = loc;
}

void xScreenWarp::remove(warper* warp)
{
    free_warp(warp);
}

const char* xScreenWarp::common_warp_names[MAX_TYPE + 1] = {
    "Warble",
    "Lens",
    "Ripple",
    "Shockwave",
    "Heat Shimmer",
    "Static Lens",
    "Random"
};

namespace {

    void get_uvmap_range(S32& istart, S32& iend, S32& jstart, S32& jend, F32 x, F32 y, F32 r)
    {
        istart = (S32)(16.0f * (x - r));
        if (istart < 0) istart = 0;
        iend = (S32)(16.0f * (x + r)) + 2;
        if (iend > 16) iend = 16;

        jstart = (S32)(16.0f * (y - r));
        if (jstart < 0) jstart = 0;
        jend = (S32)(16.0f * (y + r)) + 2;
        if (jend > 16) jend = 16;
    }

    void warp_animate_warble(xVec2* uvmap, warper& warp, const xVec2& loc, F32 radius, F32 base_intensity, F32, F32 s)
    {
        const F32 randumag = tweak::warble::mag / 16.0f;
        const F32 randvmag = tweak::warble::mag / 16.0f;

        F32 random_u, random_v;
        if (warp.flags & FLAG_INFINITE) {
            random_u = base_intensity * randumag;
            random_v = base_intensity * randvmag;
        } else {
            s *= base_intensity;
            random_u = s * randumag;
            random_v = s * randvmag;
        }

        random_u *= uvmap_uscale;
        random_v *= uvmap_vscale;
        F32 r2 = radius * radius;
        F32 ir2 = 1.0f / r2;

        S32 istart, iend, jstart, jend;
        get_uvmap_range(istart, iend, jstart, jend, loc.x, loc.y, radius);

        F32 dx = uvmap_xdelta, dy = uvmap_ydelta;
        F32 x = dx * istart;
        xVec2* uv = uvmap + istart * 17 + jstart;
        S32 uvstride = 17 - (jend - jstart);
        for (S32 i = istart; i < iend; x += dx, uv += uvstride, i++) {
            F32 y = dy * jstart;
            for (S32 j = jstart; j < jend; y += dy, uv++, j++) {
                F32 xoff = x - loc.x;
                F32 yoff = y - loc.y;
                F32 dist2 = xoff * xoff + yoff * yoff;
                if (dist2 >= r2) continue;

                F32 intensity = 1.0f - dist2 * ir2;
                uv = uvmap + i * 17 + j;
                uv->x += intensity * (random_u * (2.0f * xurand() - 1.0f));
                uv->y += intensity * (random_v * (2.0f * xurand() - 1.0f));
            }
        }
    }

    void warp_animate_lens(xVec2* uvmap, warper&, const xVec2& loc, F32 radius, F32 base_intensity, F32, F32 s)
    {
        const F32 freq = 2.0f * PI * tweak::shockwave::freq;
        F32 mag = tweak::shockwave::mag * (base_intensity * (s * isin((1.0f - s) * freq)));

        F32 r2 = radius * radius;
        F32 ir2 = 1.0f / r2;
        F32 umag = uvmap_uscale * mag;
        F32 vmag = uvmap_vscale * mag;

        S32 istart, iend, jstart, jend;
        get_uvmap_range(istart, iend, jstart, jend, loc.x, loc.y, radius);

        F32 dx = uvmap_xdelta, dy = uvmap_ydelta;
        F32 x = dx * istart;
        xVec2* uv = uvmap + istart * 17 + jstart;
        S32 uvstride = 17 - (jend - jstart);
        for (S32 i = istart; i < iend; x += dx, uv += uvstride, i++) {
            F32 y = dy * jstart;
            for (S32 j = jstart; j < jend; y += dy, uv++, j++) {
                F32 xoff = x - loc.x;
                F32 yoff = y - loc.y;
                F32 dist2 = xoff * xoff + yoff * yoff;
                if (dist2 >= r2) continue;

                F32 scale = 1.0f - dist2 * ir2;
                uv = uvmap + i * 17 + j;
                uv->x -= umag * (xoff * scale);
                uv->y -= vmag * (yoff * scale);
            }
        }
    }

    void warp_animate_ripple(xVec2* uvmap, warper& warp, const xVec2& loc, F32 radius, F32 base_intensity, F32, F32 s)
    {
        F32 mag;
        if (warp.flags & FLAG_INFINITE) {
            mag = base_intensity * tweak::ripple::mag;
        } else {
            F32 m = 1.0f - 4.0f * (s - 0.5f) * (s - 0.5f);
            mag = tweak::ripple::mag * (m * base_intensity);
        }

        F32 umag = uvmap_uscale * mag;
        F32 vmag = uvmap_vscale * mag;
        F32 freq = warp.freq * (2.0f * PI * tweak::ripple::freq);
        F32 r2 = radius * radius;
        F32 ir2 = 1.0f / r2;

        S32 istart, iend, jstart, jend;
        get_uvmap_range(istart, iend, jstart, jend, loc.x, loc.y, radius);

        F32 dx = uvmap_xdelta, dy = uvmap_ydelta;
        F32 x = dx * istart;
        xVec2* uv = uvmap + istart * 17 + jstart;
        S32 uvstride = 17 - (jend - jstart);
        for (S32 i = istart; i < iend; x += dx, uv += uvstride, i++) {
            F32 y = dy * jstart;
            for (S32 j = jstart; j < jend; y += dy, uv++, j++) {
                F32 xoff = x - loc.x;
                F32 yoff = y - loc.y;
                F32 dist2 = xoff * xoff + yoff * yoff;
                if (dist2 >= r2) continue;

                F32 t = 1.0f - dist2 * ir2;
                F32 scale = t * isin(freq * (t + (1.0f - s)));
                uv = uvmap + i * 17 + j;
                uv->x -= umag * (xoff * scale);
                uv->y -= vmag * (yoff * scale);
            }
        }
    }

    void warp_animate_shockwave(xVec2* uvmap, warper&, const xVec2& loc, F32 radius, F32 base_intensity, F32, F32 s)
    {
        F32 freq = 2.0f * PI * tweak::shockwave::freq;
        F32 freq2 = 2.0f * PI * -tweak::shockwave::freq2;
        F32 mag = tweak::shockwave::mag * (s * base_intensity);
        F32 umag = uvmap_uscale * mag;
        F32 vmag = uvmap_vscale * mag;
        F32 r2 = radius * radius;
        F32 ir = 1.0f / radius;

        S32 istart, iend, jstart, jend;
        get_uvmap_range(istart, iend, jstart, jend, loc.x, loc.y, radius);

        F32 dx = uvmap_xdelta, dy = uvmap_ydelta;
        F32 x = dx * istart;
        xVec2* uv = uvmap + istart * 17 + jstart;
        S32 uvstride = 17 - (jend - jstart);
        for (S32 i = istart; i < iend; x += dx, uv += uvstride, i++) {
            F32 y = dy * jstart;
            for (S32 j = jstart; j < jend; y += dy, uv++, j++) {
                F32 xoff = x - loc.x;
                F32 yoff = y - loc.y;
                F32 dist2 = xoff * xoff + yoff * yoff;
                if (dist2 >= r2 || dist2 <= 0.00001f) continue;

                F32 dist = xsqrt(dist2);
                F32 t = dist * ir;
                F32 theta = freq * t + freq2 * (1.0f - s);
                theta = CLAMP(theta, -PI, PI);
                F32 stheta;
                xsinfast(stheta, theta);
                F32 scale = stheta / dist;

                uv = uvmap + i * 17 + j;
                uv->x -= umag * (xoff * scale);
                uv->y -= vmag * (yoff * scale);
            }
        }
    }

    void warp_animate_heat_shimmer(xVec2* uvmap, warper& warp, const xVec2& loc, F32 radius, F32 base_intensity, F32, F32 s)
    {
        F32 mag = uvmap_uscale * (base_intensity * tweak::heat::mag);
        if (!(warp.flags & FLAG_INFINITE))
            mag *= s;
        F32 start_theta = 2.0f * PI * tweak::heat::freq * (s * warp.freq);

        S32 istart, iend, jstart, jend;
        get_uvmap_range(istart, iend, jstart, jend, loc.x, loc.y, radius);

        F32 dthetax = tweak::heat::dthetax / 16.0f;
        F32 dthetay = tweak::heat::dthetay / 16.0f;
        const F32 dxcos0 = tweak::heat::dxcos0;
        const F32 dxsin0 = tweak::heat::dxsin0;
        const F32 dycos0 = tweak::heat::dycos0;
        const F32 dysin0 = tweak::heat::dysin0;
        F32 phase = xrmod(PI + (start_theta + dthetax * istart + dthetay * jstart)) - PI;
        F32 startcos0 = mag * icos(phase);
        F32 startsin0 = mag * isin(phase);

        F32 ir = 1.0f / radius;
        F32 dx = uvmap_xdelta * ir;
        F32 dy = uvmap_ydelta * ir;
        F32 xstart = ir * (uvmap_xdelta * istart - loc.x);
        F32 ystart = ir * (uvmap_ydelta * jstart - loc.y);
        S32 xsize = iend - istart;
        S32 ysize = jend - jstart;
        xVec2* uv = uvmap + istart * 17 + jstart;
        xVec2* uv_xend = uv + xsize * 17;
        S32 uvstride = 17 - ysize;
        F32 x = xstart;
        for (; uv < uv_xend; x += dx, uv += uvstride) {
            F32 tempcos;
            F32 cos0 = startcos0, sin0 = startsin0;
            F32 y = ystart;
            F32 x2 = x * x;
            xVec2* uv_yend = uv + ysize;
            for (; uv < uv_yend; y += dy, uv++) {
                F32 dist2 = x2 + y * y;
                if (dist2 < 1.0f) {
                    uv->x += (1.0f - dist2) * sin0;
                }

                tempcos = cos0;
                cos0 = dycos0 * cos0 - dysin0 * sin0;
                sin0 = dycos0 * sin0 + dysin0 * tempcos;
            }

            tempcos = startcos0;
            startcos0 = dxcos0 * startcos0 - dxsin0 * startsin0;
            startsin0 = dxcos0 * startsin0 + dxsin0 * tempcos;
        }
    }

    void warp_animate_static_lens(xVec2* uvmap, warper&, const xVec2& loc, F32 radius, F32 base_intensity, F32, F32)
    {
        F32 freq = 2.0f * PI * tweak::shockwave::freq;
        F32 mag = base_intensity * tweak::shockwave::mag;

        F32 r2 = radius * radius;
        F32 ir2 = 1.0f / r2;
        F32 umag = uvmap_uscale * mag;
        F32 vmag = uvmap_vscale * mag;

        S32 istart, iend, jstart, jend;
        get_uvmap_range(istart, iend, jstart, jend, loc.x, loc.y, radius);

        F32 dx = uvmap_xdelta, dy = uvmap_ydelta;
        F32 x = dx * istart;
        xVec2* uv = uvmap + istart * 17 + jstart;
        S32 uvstride = 17 - (jend - jstart);
        for (S32 i = istart; i < iend; x += dx, uv += uvstride, i++) {
            F32 y = dy * jstart;
            for (S32 j = jstart; j < jend; y += dy, uv++, j++) {
                F32 xoff = x - loc.x;
                F32 yoff = y - loc.y;
                F32 dist2 = xoff * xoff + yoff * yoff;
                if (dist2 >= r2) continue;

                F32 scale = 1.0f - dist2 * ir2;
                uv = uvmap + i * 17 + j;
                uv->x -= umag * (xoff * scale);
                uv->y -= vmag * (yoff * scale);
            }
        }
    }

}

animate_callback xScreenWarp::common_warp_animate[MAX_TYPE] = {
    warp_animate_warble,
    warp_animate_lens,
    warp_animate_ripple,
    warp_animate_shockwave,
    warp_animate_heat_shimmer,
    warp_animate_static_lens
};

namespace {

    void render_scene_enter()
    {
        uvmap_uoffset = uvmap_voffset = 0.0f;
    }

    void render_scene_exit()
    {
    }

    void render_uvmap(RwCamera*)
    {
        void* junk_buffer = xMEMPUSHTEMP((FB_XRES * FB_YRES * 2 + 31) / 4);
        void* frame_buffer_raw = xMEMPUSHTEMP(FB_XRES * FB_YRES * 2 + 31);
        if (frame_buffer_raw == NULL) return;

        void* frame_buffer = (void*)(((U32)frame_buffer_raw + 31) & ~31);

        GXSetTexCopySrc(0, 0, FB_XRES, FB_YRES);
        GXSetTexCopyDst(FB_XRES, FB_YRES, GX_TF_RGB565, GX_FALSE);
        GXCopyTex(frame_buffer, GX_FALSE);
        GXPixModeSync();

        Mtx44 mProjection;
        MTXOrtho(mProjection, 0.0f, FB_YRES, 0.0f, FB_XRES, 0.0f, 1.0f);
        GXSetProjection(mProjection, GX_ORTHOGRAPHIC);

        GXSetCurrentMtx(GX_PNMTX0);

        Mtx44 mIdentity;
        MTXIdentity(mIdentity);
        GXLoadPosMtxImm(mIdentity, GX_PNMTX0);

        GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);

        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);

        GXSetNumChans(0);
        RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);

        GXSetCullMode(GX_CULL_NONE);

        GXSetNumTevStages(1);

        GXTexObj tex ATTRIBUTE_ALIGN(16);
        GXInitTexObj(&tex, frame_buffer, FB_XRES, FB_YRES, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
        GXLoadTexObj(&tex, GX_TEXMAP0);

        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ONE, GX_CC_ZERO);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_KONST, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);

        GXSetNumTexGens(1);
        GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

        GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);

        S32 dx = (S32)FB_XRES / 16, dy = (S32)FB_YRES / 16;
        S32 x = 0;
        xVec2* uv0 = uvmap_buffer, *uv1 = uv0 + 17;
        for (S32 i = 0; i < 17; i++, x += dx) {
            GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 34);

            S32 y = 0;
            for (S32 j = 0; j < 17; j++, y += dy) {
                GXPosition2f32(x, y);
                GXTexCoord2f32(uv0->x, uv0->y);
                GXPosition2f32(x + dx, y);
                GXTexCoord2f32(uv1->x, uv1->y);
                uv0++;
                uv1++;
            }
            GXEnd();
        }

        GXDrawDone();
        xMEMPOPTEMP(frame_buffer_raw);
        xMEMPOPTEMP(junk_buffer);
    }

}

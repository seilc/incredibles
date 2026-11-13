#include "xFont.h"

#include "xDebugTweak.h"
#include "xMath.h"
#include "xMath3.h"
#include "xMemMgr.h"
#include "xModel.h"
#include "xModelBucket.h"
#include "xstransvc.h"
#include "xCounter.h"
#include "xTextAsset.h"
#include "xTimer.h"
#include "xutil.h"
#include "zGame.h"
#include "zGlobals.h"
#include "zScene.h"
#include "iTime.h"

#include <rwcore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHARS_PER_CODEPAGE ((U32)144)
#define TEXT_SIZE_UNBOUNDED 0x40000000

namespace {

enum
{
    MAX_FONTS = 4,
    MAX_LAYOUTS = 1,
    MAX_ENTRIES = 16,
    MAX_ARGS = 32,
    MAX_RECUR = 16,
    MAX_DYNAMICS = 64,
    CHAR_VERT_SIZE = 6,
    CHAR_VERT_SIZE_SHADOWED = 12,
    vert_buffer_size = 240
};

struct font_asset
{
    enum
    {
        MAX_SET_SIZE = 160
    };

    U32 tex_id;
    U16 u;
    U16 v;
    U8 du;
    U8 dv;
    U8 line_size;
    U8 baseline;
    struct
    {
        S16 x;
        S16 y;
    } space;
    U32 flags;
    F32 pixel_shear;
    U8 char_set[MAX_SET_SIZE + 1];
    struct
    {
        U8 offset;
        U8 size;
    } char_pos[MAX_SET_SIZE];
};

struct font_data
{
    font_asset* asset;
    size_t index_max;
    U8 char_index[256];
    U8 fixed_width_character;
    F32 iwidth;
    F32 iheight;
    basic_rect<F32> tex_bounds[font_asset::MAX_SET_SIZE];
    basic_rect<F32> bounds[font_asset::MAX_SET_SIZE];
    xVec2 dstfrac[font_asset::MAX_SET_SIZE];
    RwTexture* texture;
    RwRaster* raster;
};

char* default_font_texture[2] = { "font_in", "Font_IN_Futura" };

font_asset default_font_assets[] = {
    { 1, 0, 0, 18, 22, 14, 0, { 0, 0 }, 0x1, 0.0f, "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890?!.,:;'\"+\xFF=/\\_()*&^%#@[]<>-\xDC\xDB\xD9\xC2\xC4\xC0\xCA\xC8\xC9\xCE\xD6\xD4\xC7\xDF\xA9\xAE\xD1\xC5\xC6\xD8\x9C\xBF\xCD\xCC\xA1\xB7\xC3\xD5\xB0\xC1\xD3\xDA\x01\xCF\xCB\xD2" },
    { 0, 0, 0, 18, 22, 14, 0, { 1, 0 }, 0x0, 0.0f, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789?!.,;:+-=/&()%\"'_<>*[]@#{}\xDC\xDB\xD9\xC2\xC4\xC0\xCA\xC8\xC9\xCE\xD6\xD4\xC7\xDF\xFC\xFB\xF9\xE2\xE4\xE0\xEA\xE8\xE9\xEE\xF6\xF4\xE7\xF1\xA9\xAE\xD1\xC5\xC6\xE5\xE6\xD8\xF8\xED" },
    { 1, 148, 232, 6, 8, 18, 0, { 0, 0 }, 0x9, 0.0f, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ,./*+-=:;%<>[]|()_" },
    { 1, 0, 0, 18, 22, 14, 0, { 0, 0 }, 0x1, 3.0f, "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890?!.,:;'\"+\xFF=/\\_()*&^%#@[]<>-\xDC\xDB\xD9\xC2\xC4\xC0\xCA\xC8\xC9\xCE\xD6\xD4\xC7\xDF\xA9\xAE\xD1\xC5\xC6\xD8\x9C\xBF\xCD\xCC\xA1\xB7\xC3\xD5\xB0\xC1\xD3\xDA\x01\xCF\xCB\xD2" }
};

font_data active_fonts[MAX_FONTS];
RwTexture* codepage_textures[16];
size_t active_fonts_size;
size_t codepage_textures_size;
RwIm2DVertex vert_buffer[vert_buffer_size];
size_t vert_buffer_used;
RwReal rcz, nsz;

inline U32 nextCharacter(const char*& stream)
{
    U8 ch1 = *stream++;
    if (ch1 < 0x80) {
        return ch1;
    }

    U8 ch2 = *stream++;
    return ((ch1 & 0x7F) << 7) + (ch2 & 0x7F);
}

inline U32 getCharacter(const char* stream)
{
    return nextCharacter(stream);
}

basic_rect<S32> find_bounds(const xColor* bits, const basic_rect<S32>& r, S32 pitch)
{
    S32 diff = pitch - r.w;
    const xColor* endp = bits + pitch * r.h;
    const xColor* p = bits;
    S32 pmode = (p->r == p->g && p->g == p->b) && p->r >= 240;

    S32 minx = r.x + r.w, maxx = r.x - 1;
    S32 miny = r.y + r.h, maxy = r.y - 1;

    for (S32 y = r.y; p != endp; p += diff, ++y) {
        const xColor* endline = p + r.w;
        for (S32 x = r.x; p != endline; ++p, ++x) {
            if ((pmode && p->a != 0) || (!pmode && p->r != 0)) {
                minx = MIN(x, minx);
                maxx = MAX(x, maxx);
                miny = MIN(y, miny);
                maxy = MAX(y, maxy);
            }
        }
    }

    basic_rect<S32> b;
    b.x = minx;
    b.y = miny;
    b.w = maxx + 1 - minx;
    b.h = maxy + 1 - miny;
    return b;
}

bool reset_font_spacing(font_asset& a)
{
    RwTexture* tex = (RwTexture*)xSTFindAsset(a.tex_id, NULL);
    if (!tex) return false;

    basic_rect<S32> char_bounds;
    char_bounds.w = a.du;
    char_bounds.h = a.dv;

    U8 baseline_count[256];
    memset(baseline_count, 0, sizeof(baseline_count));
    a.baseline = 0;

    RwInt32 width = RwRasterGetWidth(tex->raster);
    RwInt32 height = RwRasterGetHeight(tex->raster);
    RwImage* image = RwImageCreate(width, height, 32);
    if (!image) return false;

    RwImageAllocatePixels(image);
    RwImageSetFromRaster(image, tex->raster);
    const xColor* bits = (const xColor*)RwImageGetPixels(image);

    for (S32 i = 0; a.char_set[i] != 0; i++) {
        if (a.flags & 0x4) {
            char_bounds.x = a.u + char_bounds.w * (i / a.line_size);
            char_bounds.y = a.v + char_bounds.h * (i % a.line_size);
        } else {
            char_bounds.x = a.u + char_bounds.w * (i % a.line_size);
            char_bounds.y = a.v + char_bounds.h * (i / a.line_size);
        }

        const xColor* p = bits + width * char_bounds.y + char_bounds.x;
        basic_rect<S32> r = find_bounds(p, char_bounds, width);

        if (a.flags & 0x8) {
            a.char_pos[i].offset = 0;
            a.char_pos[i].size = char_bounds.w;
        } else {
            a.char_pos[i].offset = r.x - char_bounds.x;
            a.char_pos[i].size = r.w;
        }

        S32 baseline = r.h + (r.y - char_bounds.y) + 1;
        if (++baseline_count[baseline] > baseline_count[a.baseline])
            a.baseline = baseline;
    }

    RwImageDestroy(image);
    return true;
}

basic_rect<F32> get_tex_bounds(const font_data& fd, U8 i)
{
    const font_asset& a = *fd.asset;

    basic_rect<F32> r;
    if (a.flags & 0x4)
        { r.x = i / a.line_size; r.y = i % a.line_size; }
    else
        { r.x = i % a.line_size; r.y = i / a.line_size; }

    r.x = a.u + a.du * r.x + a.char_pos[i].offset;
    r.w = a.char_pos[i].size - 0.5f;
    r.y = a.v + a.dv * r.y;
    r.h = a.dv - 0.5f;
    r.scale(fd.iwidth, fd.iheight);
    return r;
}

basic_rect<F32> get_bounds(const font_data& fd, U8 i)
{
    const font_asset& a = *fd.asset;

    basic_rect<F32> r;
    r.x = 0;
    r.y = -a.baseline / (F32)a.dv;
    r.w = (F32)(a.char_pos[i].size + a.space.x) / (a.du + a.space.x);
    r.h = 1;
    return r;
}

bool init_font_data(font_data& fd)
{
    const font_asset& a = *fd.asset;

    fd.texture = (RwTexture*)xSTFindAsset(a.tex_id, NULL);
    if (!fd.texture) return false;

    RwTextureSetFilterMode(fd.texture, rwFILTERLINEAR);
    fd.raster = RwTextureGetRaster(fd.texture);

    RwInt32 width = RwRasterGetWidth(fd.raster);
    RwInt32 height = RwRasterGetHeight(fd.raster);
    fd.iwidth = 1.0f / width;
    fd.iheight = 1.0f / height;

    memset(fd.char_index, 0xFF, sizeof(fd.char_index));

    fd.index_max = 0;
    for (; a.char_set[fd.index_max] != 0; fd.index_max++) {
        U8 i = fd.index_max;

        U8 c = a.char_set[i];
        fd.char_index[c] = i;

        if (a.flags & 0x1) {
            if (c >= 'A' && c <= 'Z')
                fd.char_index[c + ('a' - 'A')] = i;
            else if (c >= 0xC0 && c <= 0xDF)
                fd.char_index[c + 0x20] = i;
        } else if (a.flags & 0x2) {
            if (c >= 'a' && c <= 'z')
                fd.char_index[c - ('a' - 'A')] = i;
            else if (c >= 0xE0 && c <= 0xFE)
                fd.char_index[c - 0x20] = i;
        }

        fd.tex_bounds[i] = get_tex_bounds(fd, i);
        fd.bounds[i] = get_bounds(fd, i);
        fd.dstfrac[i].x = (F32)a.char_pos[i].size / (a.char_pos[i].size + a.space.x);
        fd.dstfrac[i].y = (F32)a.dv / (a.dv + a.space.y);
    }

    size_t tail_index = fd.index_max;

    if (fd.char_index[' '] == 0xFF) {
        fd.char_index[' '] = tail_index;
        fd.tex_bounds[tail_index].assign(0, 0, 0, 0);
        fd.bounds[tail_index].assign(0, -a.baseline / (F32)a.dv, (a.flags & 0x8) ? 1.0f : 0.5f, 1);
        tail_index++;
    }

    if (fd.char_index[0xA0] == 0xFF) {
        fd.char_index[0xA0] = tail_index;
        fd.tex_bounds[tail_index].assign(0, 0, 0, 0);
        fd.bounds[tail_index].assign(0, -a.baseline / (F32)a.dv, (a.flags & 0x8) ? 1.0f : 0.5f, 1);
        tail_index++;
    }

    if (fd.char_index['\n'] == 0xFF) {
        fd.char_index['\n'] = tail_index;
        fd.tex_bounds[tail_index].assign(0, 0, 0, 0);
        fd.bounds[tail_index].assign(0, -a.baseline / (F32)a.dv, 0, 1);
        tail_index++;
    }

    fd.fixed_width_character = tail_index;
    fd.tex_bounds[fd.fixed_width_character].assign(0, 0, 0, 0);
    fd.bounds[fd.fixed_width_character].assign(0, -a.baseline / (F32)a.dv, 1, 1);
    return true;
}

void start_tex_render(U32 font)
{
    const font_data& fd = active_fonts[font];
    rcz = 1.0f / RwCameraGetNearClipPlane(RwCameraGetCurrentCamera());
    nsz = RwIm2DGetNearScreenZ();
    xfont::set_render_state(fd.raster);
}

void tex_flush()
{
    if (vert_buffer_used) {
        RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, vert_buffer, vert_buffer_used);
        vert_buffer_used = 0;
    }
}

void stop_tex_render()
{
    tex_flush();
    xfont::restore_render_state();
}

inline RwRaster* set_tex_raster(RwRaster* raster)
{
    RwRaster* oldraster;
    RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &oldraster);
    if (raster == oldraster) return raster;
    tex_flush();
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
    return oldraster;
}

inline void set_vert(RwIm2DVertex& vert, F32 x, F32 y, F32 u, F32 v, xColor c)
{
    RwIm2DVertexSetScreenX(&vert, x);
    RwIm2DVertexSetScreenY(&vert, y);
    RwIm2DVertexSetScreenZ(&vert, nsz);
    RwIm2DVertexSetRecipCameraZ(&vert, rcz);
    RwIm2DVertexSetU(&vert, u, rcz);
    RwIm2DVertexSetV(&vert, v, rcz);
    RwIm2DVertexSetIntRGBA(&vert, c.r, c.g, c.b, c.a);
}

void tex_render(const basic_rect<F32>& src, const basic_rect<F32>& dst, const basic_rect<F32>& clip, xColor color)
{
    basic_rect<F32> r = dst, rt = src;
    if (r.empty()) return;

    clip.clip(r, rt);
    if (r.empty()) {
        return;
    }

    if (vert_buffer_used == vert_buffer_size) {
        RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, vert_buffer, vert_buffer_size);
        vert_buffer_used = 0;
    }

    RwIm2DVertex* vert = &vert_buffer[vert_buffer_used];
    r.scale(FB_XRES, FB_YRES);
    set_vert(vert[0], r.x, r.y, rt.x, rt.y, color);
    set_vert(vert[1], r.x, r.y + r.h, rt.x, rt.y + rt.h, color);
    set_vert(vert[2], r.x + r.w, r.y, rt.x + rt.w, rt.y, color);
    vert[3] = vert[2];
    vert[4] = vert[1];
    set_vert(vert[5], r.x + r.w, r.y + r.h, rt.x + rt.w, rt.y + rt.h, color);
    vert_buffer_used += CHAR_VERT_SIZE;
}

void tex_render_shadowed(const basic_rect<F32>& src, const basic_rect<F32>& dst, const basic_rect<F32>& clip, xColor color,
                         xColor shadowcolor, F32 shadowOffsetX, F32 shadowOffsetY, F32 fshear)
{
    basic_rect<F32> r = dst, rt = src;
    clip.clip(r, rt);
    if (r.empty()) {
        return;
    }

    if (vert_buffer_used == vert_buffer_size) {
        RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, vert_buffer, vert_buffer_size);
        vert_buffer_used = 0;
    }

    RwIm2DVertex* vert = &vert_buffer[vert_buffer_used];
    r.scale(FB_XRES, FB_YRES);
    set_vert(vert[6], r.x + fshear, r.y, rt.x, rt.y, color);
    set_vert(vert[7], r.x - fshear, r.y + r.h, rt.x, rt.y + rt.h, color);
    set_vert(vert[8], r.x + r.w + fshear, r.y, rt.x + rt.w, rt.y, color);
    vert[9] = vert[8];
    vert[10] = vert[7];
    set_vert(vert[11], r.x + r.w - fshear, r.y + r.h, rt.x + rt.w, rt.y + rt.h, color);

    vert[0] = vert[6];
    vert[1] = vert[7];
    vert[2] = vert[8];
    vert[3] = vert[9];
    vert[4] = vert[10];
    vert[5] = vert[11];

    vert[0].x += shadowOffsetX;
    vert[0].y += shadowOffsetY;

    vert[1].x += shadowOffsetX;
    vert[1].y += shadowOffsetY;

    vert[2].x += shadowOffsetX;
    vert[2].y += shadowOffsetY;

    vert[3].x += shadowOffsetX;
    vert[3].y += shadowOffsetY;

    vert[4].x += shadowOffsetX;
    vert[4].y += shadowOffsetY;

    vert[5].x += shadowOffsetX;
    vert[5].y += shadowOffsetY;

    RwIm2DVertexSetIntRGBA(&vert[0], shadowcolor.r, shadowcolor.g, shadowcolor.b, shadowcolor.a);

    RwIm2DVertexSetIntRGBA(&vert[1], vert[0].r, vert[0].g, vert[0].b, vert[0].a);
    RwIm2DVertexSetIntRGBA(&vert[2], vert[0].r, vert[0].g, vert[0].b, vert[0].a);
    RwIm2DVertexSetIntRGBA(&vert[3], vert[0].r, vert[0].g, vert[0].b, vert[0].a);
    RwIm2DVertexSetIntRGBA(&vert[4], vert[0].r, vert[0].g, vert[0].b, vert[0].a);
    RwIm2DVertexSetIntRGBA(&vert[5], vert[0].r, vert[0].g, vert[0].b, vert[0].a);
    vert_buffer_used += CHAR_VERT_SIZE_SHADOWED;
}

inline void mchar_render(U32 c, U32 font_index, basic_rect<F32> bounds, const basic_rect<F32>& clip, xColor color,
                         xColor shadowColor, F32 shadowOffsetX, F32 shadowOffsetY)
{
    U32 offset = c % CHARS_PER_CODEPAGE;
    basic_rect<F32> src;
    src.x = (offset % 12) * (21.0f / 256);
    src.y = (offset / 12) * (21.0f / 256);
    src.w = 21.0f / 256;
    src.h = 21.0f / 256;
    const font_data& fd = active_fonts[font_index];
    basic_rect<F32> dest = fd.bounds[fd.fixed_width_character];
    dest.scale(bounds.w, bounds.h);
    dest.x += bounds.x;
    dest.y += bounds.y;

    tex_render_shadowed(src, dest, clip, color, shadowColor, shadowOffsetX, shadowOffsetY, 0.0f);
}

inline void char_render(U8 c, U32 font_index, const basic_rect<F32>& bounds, const basic_rect<F32>& clip, xColor color,
                        xColor shadowColor, F32 shadowOffsetX, F32 shadowOffsetY)
{
    const font_data& fd = active_fonts[font_index];

    U32 i = fd.char_index[c];
    if (i >= fd.index_max) {
        return;
    }

    basic_rect<F32> dest = fd.bounds[i];
    dest.scale(bounds.w, bounds.h);
    dest.x += bounds.x;
    dest.y += bounds.y;
    dest.w *= fd.dstfrac[i].x;
    dest.h *= fd.dstfrac[i].y;

    F32 fShear = fd.asset->pixel_shear * (dest.h * FB_YRES / fd.asset->dv);
    tex_render_shadowed(fd.tex_bounds[i], dest, clip, color, shadowColor, shadowOffsetX, shadowOffsetY, fShear);
}

struct model_cache_entry
{
    U32 id;
    U32 order;
    xModelInstance* model;
};

struct model_pool
{
    RwMatrix mat[8];
    xModelInstance model[8];
};

model_cache_entry model_cache[8];
bool model_cache_inited;

void init_model_cache()
{
    model_cache_inited = true;

    void* data = xMEMALLOC(sizeof(model_pool), 16);
    model_pool& pool = *(model_pool*)data;
    memset(&pool, 0, sizeof(pool));

    for (size_t i = 0; i < ARRAY_LENGTH(model_cache); i++) {
        xModelInstance& model = pool.model[i];
        model_cache_entry& e = model_cache[i];
        e.id = e.order = 0;
        e.model = &model;
        model.Mat = &pool.mat[i];
        model.Flags = 0x1;
        model.BoneCount = 1;
        model.shadowID = 0xDEADBEEF;
    }
}

xModelInstance* load_model(U32 id)
{
    static U32 next_order = 0;
    next_order++;

    size_t oldest = 0;
    for (size_t i = 0; i < ARRAY_LENGTH(model_cache); i++) {
        model_cache_entry& e = model_cache[i];
        if (e.id == id) {
            e.order = next_order;
            return e.model;
        }

        if (e.order < model_cache[oldest].order) oldest = i;
    }

    RpAtomic* mf = (RpAtomic*)xSTFindAsset(id, NULL);
    if (!mf) return NULL;

    model_cache_entry& e = model_cache[oldest];
    e.id = id;
    e.order = next_order;

    xModelInstance& model = *e.model;
    xMat4x3Identity((xMat4x3*)model.Mat);
    model.Data = mf;
    model.Bucket = xModelBucket_GetBuckets(model.Data);
    model.Pipe = model.Bucket ? model.Bucket[0]->Pipe : xModelGetPipe(model.Data);
    return &model;
}

struct
{
    S32 fogenable;
    S32 vertexalphaenable;
    S32 zwriteenable;
    S32 ztestenable;
    S32 cullmode;
    U32 srcblend;
    U32 destblend;
    U32 shademode;
    RwTextureFilterMode filter;
} oldrs;

} // namespace

void xfont::init()
{
    active_fonts_size = 0;

    for (size_t i = 0; i < ARRAY_LENGTH(default_font_assets); i++) {
        font_asset& a = default_font_assets[i];

        if (a.tex_id < ARRAY_LENGTH(default_font_texture)) {
            a.tex_id = xStrHash(default_font_texture[a.tex_id]);
        }

        if (!reset_font_spacing(a)) {
            continue;
        }

        font_data& fd = active_fonts[active_fonts_size];
        fd.asset = &a;
        if (!init_font_data(fd))
            continue;

        active_fonts_size++;
    }

    char filename[20];
    sprintf(filename, "lfont_%i", codepage_textures_size);
    codepage_textures[codepage_textures_size] = (RwTexture*)xSTFindAsset(xStrHash(filename), NULL);
    while (codepage_textures_size < ARRAY_LENGTH(codepage_textures) && codepage_textures[codepage_textures_size]) {
        RwTextureSetFilterMode(codepage_textures[codepage_textures_size], rwFILTERLINEAR);
        codepage_textures_size++;
        sprintf(filename, "lfont_%i", codepage_textures_size);
        codepage_textures[codepage_textures_size] = (RwTexture*)xSTFindAsset(xStrHash(filename), NULL);
    }

    init_model_cache();
}

void xfont::set_render_state(RwRaster* raster)
{
    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &oldrs.fogenable);
    RwRenderStateGet(rwRENDERSTATESRCBLEND, &oldrs.srcblend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND, &oldrs.destblend);
    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &oldrs.vertexalphaenable);
    RwRenderStateGet(rwRENDERSTATESHADEMODE, &oldrs.shademode);
    RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &oldrs.zwriteenable);
    RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &oldrs.ztestenable);
    RwRenderStateGet(rwRENDERSTATECULLMODE, &oldrs.cullmode);
    RwRenderStateGet(rwRENDERSTATETEXTUREFILTER, &oldrs.filter);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEFLAT);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
}

void xfont::restore_render_state()
{
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)oldrs.fogenable);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)oldrs.srcblend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)oldrs.destblend);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)oldrs.vertexalphaenable);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)oldrs.shademode);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)oldrs.zwriteenable);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)oldrs.ztestenable);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)oldrs.filter);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)oldrs.cullmode);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
}

basic_rect<F32> xfont::bounds(U32 c) const
{
    if (c < 0x100)
        return bounds((char)c);

    if (c == 0x3FFF) {
        basic_rect<F32> r = { 0, 0, 0, 0 };
        return r;
    }

    const font_data& fd = active_fonts[id];
    basic_rect<F32> r = fd.bounds[fd.fixed_width_character];
    r.scale(width, height);
    r.w += space;
    return r;
}

basic_rect<F32> xfont::bounds(char c) const
{
    const font_data& fd = active_fonts[id];
    U32 i = fd.char_index[(U8)c];
    if (i == 0xFF) return basic_rect<F32>::m_Null;
    basic_rect<F32> r = fd.bounds[i];
    r.scale(width, height);
    if (r.w > 0)
        r.w += space;
    return r;
}

basic_rect<F32> xfont::bounds(const char* text, size_t text_size) const
{
    size_t size;
    return bounds(text, text_size, HUGE, size);
}

basic_rect<F32> xfont::bounds(const char* text, size_t text_size, F32 max_width, size_t& size) const
{
    const font_data& fd = active_fonts[id];
    basic_rect<F32> r;
    r.x = 0;
    r.y = fd.bounds[0].y * height;
    r.w = 0;
    r.h = fd.bounds[0].h * height;

    if (!text || !text_size) { size = 0; return r; }

    const char* s = text;
    while (s - text < text_size && *s) {
        U32 c = nextCharacter(s);
        if (c == 0x3FFF) continue;

        F32 dx;
        if (c < 0x100) {
            U32 charIndex = fd.char_index[c];
            if (charIndex == 0xFF) continue;
            dx = fd.bounds[charIndex].w * width;
        } else {
            const font_data& fd = active_fonts[id];
            dx = fd.bounds[fd.fixed_width_character].w * width;
        }

        if (r.w + dx > max_width) break;
        r.w += dx + space;
    }

    if (r.w > 0) r.w -= space;
    size = s - text;
    return r;
}

void xfont::start_render() const
{
    start_tex_render(id);
}

void xfont::stop_render() const
{
    stop_tex_render();
}

// UNUSED: needed to match (speculative code)
void xfont::irender(U32 c, F32 x, F32 y) const
{
    basic_rect<F32> bounds = { x, y, width, height };

    if (c >= 0x100) {
        c -= 0x100;
        U32 codePage = c / CHARS_PER_CODEPAGE;
        set_tex_raster(RwTextureGetRaster(codepage_textures[codePage]));
        mchar_render(c, id, bounds, clip, color, shadowColor, shadowOffsetX, shadowOffsetY);
    } else {
        const font_data& fd = active_fonts[id];
        set_tex_raster(fd.raster);
        char_render(c, id, bounds, clip, color, shadowColor, shadowOffsetX, shadowOffsetY);
    }
}

void xfont::irender(const char* text, F32 x, F32 y) const
{
    irender(text, TEXT_SIZE_UNBOUNDED, x, y);
}

void xfont::irender(const char* text, size_t text_size, F32 x, F32 y) const
{
    if (!text) return;

    const font_data& fd = active_fonts[id];
    basic_rect<F32> bounds = { x, y, width, height };

    bool pageCompleted[16];
    memset(pageCompleted, 0, sizeof(pageCompleted));

    for (size_t cpItr = 0; cpItr < codepage_textures_size; cpItr++) {
        bounds.x = x;
        S32 whichCodePage = -1;
        const char* s = text;
        while (s - text < text_size && *s) {
            U32 c = nextCharacter(s);
            if (c == 0x3FFF) {
                continue;
            }

            if (c >= 0x100) {
                c -= 0x100;
                U32 codePage = c / CHARS_PER_CODEPAGE;
                if (whichCodePage == -1 && !pageCompleted[codePage]) {
                    whichCodePage = codePage;
                    set_tex_raster(RwTextureGetRaster(codepage_textures[codePage]));
                }

                if (whichCodePage == codePage) {
                    mchar_render(c, id, bounds, clip, color, shadowColor, shadowOffsetX, shadowOffsetY);
                }

                bounds.x += fd.bounds[fd.fixed_width_character].w * width + space;
            } else {
                U32 charIndex = fd.char_index[c];
                if (charIndex != 0xFF)
                    bounds.x += fd.bounds[charIndex].w * width + space;
            }
        }

        if (whichCodePage == -1) {
            break;
        }
        pageCompleted[whichCodePage] = true;
    }

    bounds.x = x;
    set_tex_raster(fd.raster);
    const char* s = text;
    while (s - text < text_size && *s) {
        U32 c = nextCharacter(s);
        if (c == 0x3FFF) {
            continue;
        }

        if (c >= 0x100) {
            bounds.x += fd.bounds[fd.fixed_width_character].w * width + space;
        } else {
            char_render(c, id, bounds, clip, color, shadowColor, shadowOffsetX, shadowOffsetY);
            U32 charIndex = fd.char_index[c];
            if (charIndex != 0xFF)
                bounds.x += fd.bounds[charIndex].w * width + space;
        }
    }
}

namespace {

// UNUSED: needed to match (speculative code)
xColor read_hex_color(const char* text, xColor c)
{
    size_t read_size;
    U32 v = atox(substr::create(text, 8), read_size);
    if (read_size == 6) {
        c.r = (v >> 16) & 0xFF;
        c.g = (v >> 8) & 0xFF;
        c.b = v & 0xFF;
    } else if (read_size == 8) {
        c.a = v >> 24;
        c.r = (v >> 16) & 0xFF;
        c.g = (v >> 8) & 0xFF;
        c.b = v & 0xFF;
    }
    return c;
}

const substr text_delims = { " \t\n{}=*+:;,", 11 };

size_t parse_split_tag(xtextbox::split_tag& ti)
{
    ti.name.size = ti.action.size = ti.value.size = 0;

    substr s = ti.tag;
    s.text++;
    s.size--;

    ti.name.text = skip_ws(s);
    s.text = find_char(s, text_delims);
    if (!s.text) return 0;

    ti.name.size = s.text - ti.name.text;
    s.size -= ti.name.size;
    ti.action.text = skip_ws(s);
    if (s.size == 0) return 0;

    char c = *s.text;
    if (c == '\0' || c == '{') return 0;
    s.text++; s.size--;
    if (c == '}') return ti.tag.size - s.size;

    ti.action.size = 1;

    ti.value.text = skip_ws(s);
    s.text = find_char(s, '}');
    s.size -= s.text - ti.value.text;
    if (!s.text) return 0;
    ti.value.size = s.text - ti.value.text;
    rskip_ws(ti.value);
    s.text++; s.size--;

    return ti.tag.size - s.size;
}

const char* parse_next_tag_jot(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const char* text, size_t text_size)
{
    xtextbox::split_tag ti = { { text, text_size } };
    size_t size = parse_split_tag(ti);
    if (!size) return NULL;

    a.s.text = text;
    a.s.size = size;
    a.flag.invisible = a.flag.ethereal = true;

    if (icompare(ti.name, substr::create("~", 1)) == 0 || icompare(ti.name, substr::create("reset", 5)) == 0) {
        a.tag = xtextbox::find_format_tag(ti.value);
        if (a.tag && a.tag->reset_tag)
            a.tag->reset_tag(a, tb, ctb, ti);
    } else {
        a.tag = xtextbox::find_format_tag(ti.name);
        if (a.tag && a.tag->parse_tag) {
            a.tag->parse_tag(a, tb, ctb, ti);
        }
    }

    return text + size;
}

const char* parse_next_text_jot(xtextbox::jot& a, const xtextbox& tb, const xtextbox&, const char* text, size_t text_size)
{
    U32 c = getCharacter(text);
    bool hasSpaceChar = false;

    if (xtextbox::get_japanese_word_breaking() && c >= 0x100) {
        if (text_size >= 5 && getCharacter(text + 2) == 0x3FFF)
            hasSpaceChar = true;
        else
            a.flag.japanese_break = true;
    }

    a.s.text = text;
    a.flag.merge = true;
    a.cb = &xtextbox::text_cb;
    a.context = NULL;
    a.context_size = 0;

    if (hasSpaceChar) {
        a.s.size = getCharacter(a.s.text + 4) >= 0x80 ? 6 : 5;
        a.bounds = tb.font.bounds(a.s.text, a.s.size);
    } else {
        a.s.size = c < 0x80 ? 1 : 2;
        a.bounds = tb.font.bounds(c);
    }

    switch (c) {
    case '\n':
        a.flag.line_break = true;
        a.flag.invisible = a.flag.word_break = true;
        break;
    case '\t':
        a.flag.tab = true;
        a.flag.invisible = a.flag.word_break = true;
        break;
    case '-':
        a.flag.word_end = true;
        break;
    case ' ':
        a.flag.invisible = a.flag.word_break = true;
    }

    return a.s.text + a.s.size;
}

const char* parse_next_jot(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const char* text, size_t text_size)
{
    const char* next;
    if (*text != '{' || !(next = parse_next_tag_jot(a, tb, ctb, text, text_size)))
        next = parse_next_text_jot(a, tb, ctb, text, text_size);

    a.flag.merge = a.flag.merge && !(tb.flags & 0x80);
    return next;
}

struct tex_args
{
    RwRaster* raster;
    F32 rot;
    basic_rect<F32> src;
    basic_rect<F32> dst;
    xVec2 off;
    enum
    {
        SCALE_FONT,
        SCALE_SCREEN,
        SCALE_SIZE,
        SCALE_FONT_WIDTH,
        SCALE_FONT_HEIGHT,
        SCALE_SCREEN_WIDTH,
        SCALE_SCREEN_HEIGHT
    } scale;
};

tex_args def_tex_args;

void reset_tex_args(tex_args& ta)
{
    ta.raster = NULL;
    ta.rot = 0;
    ta.src = ta.dst = basic_rect<F32>::m_Unit;
    ta.off.x = ta.off.y = 1;
    ta.scale = tex_args::SCALE_FONT;
}

void load_tex_args(tex_args& ta, const substr& s)
{
    xtextbox::tag_entry_list el = xtextbox::read_tag(s);
    if (!el.size) return;

    const xtextbox::tag_entry* e = el.entries;
    if (e->op == ':' || (e->args_size == 1 && e->args[0].size != 0)) {
        const substr& name = e->args[0];
        U32 id;
        if (name.size == 10 && imemcmp(name.text, "0x", 2) == 0)
            id = atox(substr::create(name.text + 2, 8));
        else
            id = xStrHash(name.text, name.size);

        RwTexture* texture = (RwTexture*)xSTFindAsset(id, NULL);
        if (texture && texture->raster && texture->raster->width > 0 && texture->raster->height > 0 &&
            texture->raster->width <= 0x1000 && texture->raster->height <= 0x1000) {
            ta.raster = RwTextureGetRaster(texture);
        }
    }

    el.entries++;
    el.size--;

    e = xtextbox::find_entry(el, substr::create("rot", 3));
    if (e && e->op == '=' && e->args_size == 1) {
        xtextbox::read_list(*e, &ta.rot, 1);
    }

    e = xtextbox::find_entry(el, substr::create("src", 3));
    if (e && e->op == '=' && e->args_size == 4) {
        xtextbox::read_list(*e, &ta.src.x, 4);
    }

    e = xtextbox::find_entry(el, substr::create("dst", 3));
    if (e && e->op == '=' && e->args_size == 4) {
        xtextbox::read_list(*e, &ta.dst.x, 4);
    }

    e = xtextbox::find_entry(el, substr::create("off", 3));
    if (e && e->op == '=' && e->args_size == 2) {
        xtextbox::read_list(*e, &ta.off.x, 2);
    }

    e = xtextbox::find_entry(el, substr::create("scale", 5));
    if (e && e->op == '=' && e->args_size == 1) {
        if (icompare(e->args[0], substr::create("font", 4)) == 0)
            ta.scale = tex_args::SCALE_FONT;
        else if (icompare(e->args[0], substr::create("screen", 6)) == 0)
            ta.scale = tex_args::SCALE_SCREEN;
        else if (icompare(e->args[0], substr::create("size", 4)) == 0)
            ta.scale = tex_args::SCALE_SIZE;
        else if (icompare(e->args[0], substr::create("font_width", 10)) == 0)
            ta.scale = tex_args::SCALE_FONT_WIDTH;
        else if (icompare(e->args[0], substr::create("font_height", 11)) == 0)
            ta.scale = tex_args::SCALE_FONT_HEIGHT;
        else if (icompare(e->args[0], substr::create("screen_width", 12)) == 0)
            ta.scale = tex_args::SCALE_SCREEN_WIDTH;
        else if (icompare(e->args[0], substr::create("screen_height", 13)) == 0)
            ta.scale = tex_args::SCALE_SCREEN_HEIGHT;
        else
            ta.scale = tex_args::SCALE_FONT;
    }
}

struct model_args
{
    xModelInstance* model;
    xVec3 rot;
    basic_rect<F32> dst;
    xVec2 off;
    enum
    {
        SCALE_FONT,
        SCALE_SCREEN,
        SCALE_SIZE
    } scale;
};

model_args def_model_args;

void reset_model_args(model_args& ma)
{
    ma.model = NULL;
    ma.rot = xVec3::m_Null;
    ma.dst = basic_rect<F32>::m_Unit;
    ma.off.x = ma.off.y = 1;
    ma.scale = model_args::SCALE_FONT;
}

void load_model_args(model_args& ma, const substr& s)
{
    xtextbox::tag_entry_list el = xtextbox::read_tag(s);
    if (!el.size) return;

    const xtextbox::tag_entry* e = el.entries;
    if (e->op == ':' || (e->args_size == 1 && e->args[0].size != 0)) {
        const substr& name = e->args[0];
        U32 id = xStrHash(name.text, name.size);
        ma.model = load_model(id);
    }

    el.entries++;
    el.size--;

    e = xtextbox::find_entry(el, substr::create("rot", 3));
    if (e && e->op == '=' && e->args_size == 3) {
        xtextbox::read_list(*e, &ma.rot.x, 3);
    }

    e = xtextbox::find_entry(el, substr::create("dst", 3));
    if (e && e->op == '=' && e->args_size == 4) {
        xtextbox::read_list(*e, &ma.dst.x, 4);
    }

    e = xtextbox::find_entry(el, substr::create("off", 3));
    if (e && e->op == '=' && e->args_size == 2) {
        xtextbox::read_list(*e, &ma.off.x, 2);
    }

    e = xtextbox::find_entry(el, substr::create("scale", 5));
    if (e && e->op == '=' && e->args_size == 1) {
        if (icompare(e->args[0], substr::create("screen", 6)) == 0)
            ma.scale = model_args::SCALE_SCREEN;
        else
            ma.scale = model_args::SCALE_FONT;
    }
}

void start_layout(const xtextbox&)
{
    reset_tex_args(def_tex_args);
    reset_model_args(def_model_args);
}

void stop_layout(const xtextbox&)
{
}

void start_render(const xtextbox& tb)
{
    tb.font.start_render();
}

void stop_render(const xtextbox& tb)
{
    tb.font.stop_render();
}

struct tl_cache_entry
{
    U32 used;
    iTime last_used;
    xtextbox::layout tl;
};

tl_cache_entry tl_cache[MAX_LAYOUTS];

} // namespace

xtextbox::callback xtextbox::text_cb = { xtextbox::text_render, NULL, NULL };
bool xtextbox::japanese_word_breaking;

void xtextbox::text_render(const jot& j, const xtextbox& tb, F32 x, F32 y)
{
    tb.font.irender(j.s.text, j.s.size, x, y);
}

void xtextbox::set_text(const char* text)
{
    set_text(text, TEXT_SIZE_UNBOUNDED);
}

void xtextbox::set_text(const char* text, size_t text_size)
{
    if (!text || !text_size) { texts_size = 0; text_hash = 0; return; }

    this->text.text = text;
    this->text.size = text_size;
    set_text(&this->text.text, &this->text.size, 1);
}

void xtextbox::set_text(const char** texts, size_t size)
{
    set_text(texts, NULL, size);
}

void xtextbox::set_text(const char** texts, const size_t* text_sizes, size_t size)
{
    texts_size = size;
    text_hash = 0;
    if (!size) return;

    this->texts = texts;
    this->text_sizes = text_sizes;

    if (!text_sizes)
        for (size_t i = 0; i < size; i++) {
            text_hash = text_hash * 131 + xStrHash(texts[i]);
        }
    else
        for (size_t i = 0; i < size; i++)
            text_hash = text_hash * 131 + xStrHash(texts[i], text_sizes[i]);
}

namespace tweaker {
namespace {

void log_cache(bool) {}

} // namespace
} // namespace tweaker

xtextbox::layout& xtextbox::temp_layout(bool cache) const
{
    const iTime decay_time = iTimeFromSec(1.0f);
    iTime cur_time = iTimeGet();
    bool refresh = false;
    size_t index = 0;

    if (cache) {
        for (; index < MAX_LAYOUTS; index++) {
            if (!tl_cache[index].tl.changed(*this)) break;
        }
    } else {
        index = MAX_LAYOUTS;
    }

    tweaker::log_cache(index < MAX_LAYOUTS);

    if (index >= MAX_LAYOUTS) {
        refresh = true;
        S32 min_used = 1000000000;
        index = 0;
        for (size_t i = 0; i < MAX_LAYOUTS; i++) {
            tl_cache_entry& e = tl_cache[i];
            if (cur_time - e.last_used > decay_time) {
                index = i;
                break;
            }

            S32 used = e.used;
            if (e.tl.dynamics_size) used -= 10;
            if (e.tl.jots_size() > 50) used += 10;
            if (used < min_used) {
                index = i;
                min_used = used;
            }
        }
    }

    tl_cache_entry& e = tl_cache[index];
    if (refresh) {
        e.used = 0;
        e.tl.refresh(*this, true);
    } else {
        e.tl.tb = *this;
    }

    if (cache) {
        e.used++;
        e.last_used = cur_time;
    }

    return e.tl;
}

void xtextbox::render(layout& l, S32 begin_jot, S32 end_jot) const
{
    l.render(*this, begin_jot, end_jot);
}

F32 xtextbox::yextent(F32 max, S32& size, const layout& l, S32 begin_jot, S32 end_jot) const
{
    return l.yextent(max, size, begin_jot, end_jot);
}

xtextbox::tag_entry_list xtextbox::read_tag(const substr& s)
{
    static substr arg_buffer[MAX_ARGS];
    static tag_entry entry_buffer[MAX_ENTRIES];

    size_t args_used = 0, entries_used = 0;
    substr it = s;

    const char* d = find_char(it, '{');
    if (d) { it.size -= d + 1 - it.text; it.text = d + 1; }

    substr delims = { "=:+*;}", 6 };
    substr sub_delims = { ",;}", 3 };

    while (it.size) {
        tag_entry& entry = entry_buffer[entries_used];
        entry.args_size = 0;
        entry.op = 0;
        entry.name.text = it.text;

        const char* d = find_char(it, delims);
        entry.name.size = !d ? it.size : d - it.text;
        trim_ws(entry.name);
        if (entry.name.size) entries_used++;
        if (!d || *d == '}') break;

        it.size -= d + 1 - it.text;
        it.text = d + 1;

        if (*d == ';') continue;

        entry.op = *d;
        entry.args = &arg_buffer[args_used];

        while (it.size) {
            substr& arg = arg_buffer[args_used];
            arg.text = it.text;

            const char* d = find_char(it, sub_delims);
            if (!d) {
                arg.size = it.size;
                it.size = 0;
            } else {
                arg.size = d - it.text;
                it.size -= arg.size + 1;
                it.text += arg.size + 1;
            }

            trim_ws(arg);
            if (arg.size) {
                args_used++;
                entry.args_size++;
            }

            if (*d == ';') break;
            if (!d || *d == '}') { it.size = 0; break; }
        }
    }

    tag_entry_list ret = { entry_buffer, entries_used };
    return ret;
}

const xtextbox::tag_entry* xtextbox::find_entry(const tag_entry_list& el, const substr& name)
{
    for (size_t i = 0; i < el.size; i++) {
        const tag_entry& e = el.entries[i];
        if (icompare(name, e.name) == 0) return &e;
    }
    return NULL;
}

size_t xtextbox::read_list(const tag_entry& e, F32* v, size_t vsize)
{
    size_t total = MIN(vsize, e.args_size);
    for (size_t i = 0; i < total; i++)
        v[i] = xatof(e.args[i].text);
    return total;
}

size_t xtextbox::read_list(const tag_entry& e, S32* v, size_t vsize)
{
    size_t total = MIN(vsize, e.args_size);
    for (size_t i = 0; i < total; i++)
        v[i] = atoi(e.args[i].text);
    return total;
}

void xtextbox::clear_layout_cache()
{
    for (size_t index = 0; index < MAX_LAYOUTS; index++)
        tl_cache[index].tl.clear();
}

void xtextbox::layout::refresh(const xtextbox& tb, bool force)
{
    if (!force && !changed(tb)) return;
    this->tb = tb;
    calc(tb, 0);
}

void xtextbox::layout::clear()
{
    _jots_size = _lines_size = context_buffer_size = dynamics_size = 0;
    tb = xtextbox::create();
}

void xtextbox::layout::refresh_end(const xtextbox& tb)
{
    if (this->tb.texts_size <= tb.texts_size) return;
    size_t start_text = this->tb.texts_size;
    this->tb = tb;
    calc(tb, start_text);
}

void xtextbox::layout::trim_line(jot_line& line)
{
    for (S32 i = line.last - 1; i >= (S32)line.first; i--) {
        jot& a = _jots[i];
        if (!a.flag.ethereal) {
            if (a.flag.invisible) {
                erase_jots(i, i + 1);
                line.last--;
            }
            break;
        }
    }

    for (size_t i = line.first; i < line.last; i++) {
        jot& a = _jots[i];
        if (!a.flag.ethereal) {
            if (a.flag.invisible) {
                erase_jots(i, i + 1);
                line.last--;
            }
            break;
        }
    }
}

void xtextbox::layout::erase_jots(size_t first, size_t last)
{
    if (last >= _jots_size) {
        _jots_size = first;
        return;
    }

    size_t offset = last - first;
    _jots_size -= offset;
    for (size_t i = first; i < _jots_size; i++) {
        jot& a = _jots[i];
        a = _jots[i + offset];
    }
}

void xtextbox::layout::merge_line(jot_line& line)
{
    size_t d = line.first;
    for (size_t i = line.first + 1; i != line.last; i++) {
        jot& a1 = _jots[d];
        jot& a2 = _jots[i];
        if (!a1.flag.ethereal && !a2.flag.ethereal && a1.flag.merge && a2.flag.merge && a1.cb == a2.cb) {
            a1.s.size = a2.s.size + (a2.s.text - a1.s.text);
            a1.bounds |= a2.bounds;
            a1.intersect_flags(a2);
        } else {
            d++;
            if (d != i) {
                _jots[d] = a2;
            }
        }
    }

    d++; erase_jots(d, line.last);
    line.last = d;
}

void xtextbox::layout::bound_line(jot_line& line)
{
    line.bounds.w = line.bounds.h = line.baseline = 0;
    for (size_t i = line.first; i != line.last; i++) {
        jot& a = _jots[i];
        if (a.flag.ethereal) continue;
        if (-a.bounds.y > line.baseline) line.baseline = -a.bounds.y;
    }

    for (size_t i = line.first; i != line.last; i++) {
        jot& a = _jots[i];
        if (a.flag.ethereal) continue;
        a.bounds.x = line.bounds.w;
        line.bounds.w += a.bounds.w;
        F32 total_height = line.baseline + a.bounds.y + a.bounds.h;
        if (total_height > line.bounds.h) line.bounds.h = total_height;
    }

    line.page_break = line.last > line.first && _jots[line.last - 1].flag.page_break;
}

bool xtextbox::layout::fit_line(bool isLastLine)
{
    jot_line& line = _lines[_lines_size];

    if (line.bounds.w > tb.bounds.w) {
        switch (tb.flags & 0x30) {
        case 0x10:
            if (line.last > line.first + 1) line.last--;
            break;
        case 0x20:
            merge_line(line);
            bound_line(line);
            return false;
        default:
            S32 i = line.last - 1;
            for (; i > (S32)line.first; i--) {
                if (_jots[i].flag.word_break)
                    { line.last = i + 1; break; }
                if (_jots[i - 1].flag.word_end)
                    { line.last = i; break; }
                if (_jots[i].flag.japanese_break)
                    { line.last = i; break; }
            }

            if (line.last <= line.first) line.last = line.first + 1;
            trim_line(line);
            break;
        }
    }

    if (isLastLine) {
        trim_line(line);
    }
    merge_line(line);
    bound_line(line);
    return true;
}

void xtextbox::layout::next_line()
{
    jot_line& line = _lines[_lines_size];
    _lines_size++;
    jot_line& nline = _lines[_lines_size];
    nline.first = line.last;
    nline.last = _jots_size;
    nline.bounds.x = 0;
    nline.bounds.y = line.bounds.y + line.bounds.h;
    bound_line(nline);
}

void xtextbox::layout::calc(const xtextbox& ctb, size_t start_text)
{
    if (start_text == 0) {
        _jots_size = _lines_size = context_buffer_size = dynamics_size = 0;
    }

    if (!tb.texts_size) return;

    start_layout(ctb);

    jot_line& first_line = _lines[_lines_size];
    first_line.first = 0;
    first_line.bounds.w = 0;
    first_line.bounds.x = 0;
    first_line.bounds.y = 0;
    first_line.baseline = 0;

    struct { const char* s; const char* end; } text_stack[MAX_RECUR];
    size_t text_stack_size = 0;

    size_t text_index = start_text;
    size_t text_size = !tb.text_sizes ? TEXT_SIZE_UNBOUNDED : tb.text_sizes[text_index];
    const char *s = tb.texts[text_index], *end = s + text_size;
    text_index++;

    for (;;) {
        const char* next;

        jot& a = _jots[_jots_size];
        jot_line& line = _lines[_lines_size];
        a.context = &context_buffer[context_buffer_size];
        a.context_size = 0;
        a.reset_flags();
        a.cb = NULL;
        a.tag = NULL;

        if (s == end || !*s) {
            if (text_stack_size) {
                text_stack_size--;
                s = text_stack[text_stack_size].s;
                end = text_stack[text_stack_size].end;
            } else {
                if (text_index >= tb.texts_size) break;

                text_size = !tb.text_sizes ? TEXT_SIZE_UNBOUNDED : tb.text_sizes[text_index];
                s = tb.texts[text_index];
                end = s + text_size;
                text_index++;
            }

            a.flag.invisible = a.flag.ethereal = true;
            a.s = substr::create(NULL, 0);
            _jots_size++;
            continue;
        }

        next = parse_next_jot(a, tb, ctb, s, end - s);

        if (a.context == &context_buffer[context_buffer_size]) {
            context_buffer_size += (a.context_size + 3) & ~3;
        }

        _jots_size++;

        if (a.cb && a.cb->layout_update) {
            a.cb->layout_update(a, tb, ctb);
        }

        if (a.flag.stop) {
            break;
        }

        if (!a.flag.ethereal) {
            a.bounds.x += line.bounds.w;
            line.bounds.w += a.bounds.w;
            if (line.bounds.w >= tb.bounds.w) {
                line.last = _jots_size;
                if (!fit_line(false)) break;
                next_line();
            }
        }

        if (a.flag.line_break || a.flag.page_break) {
            line.last = _jots_size;
            if (!fit_line(false)) break;
            next_line();
        }

        s = next;
        if (a.flag.insert) {
            text_stack[text_stack_size].s = s;
            text_stack[text_stack_size].end = end;
            s = (const char*)a.context;
            end = s + a.context_size;
            text_stack_size++;
        }
    }

    jot_line& last_line = _lines[_lines_size];
    if (last_line.first < _jots_size) {
        last_line.last = _jots_size;
        fit_line(true);
        _lines_size++;
    }

    for (size_t i = 0; i < _jots_size; i++) {
        if (_jots[i].flag.dynamic) {
            dynamics[dynamics_size++] = i;
        }
    }

    stop_layout(ctb);
}

void xtextbox::layout::render(const xtextbox& ctb, S32 begin_jot, S32 end_jot)
{
    if (begin_jot < 0) begin_jot = 0;
    if (end_jot < begin_jot) end_jot = _jots_size;
    if (begin_jot >= end_jot) return;

    tb = ctb;
    start_render(ctb);

    S32 begin_line = 0;

    for (;;) {
        if (begin_line >= (S32)_lines_size) {
            stop_render(ctb);
            return;
        }
        if ((S32)_lines[begin_line].last > begin_jot) break;
        begin_line++;
    }

    for (S32 i = 0; i < begin_jot; i++) {
        jot& j = _jots[i];
        if (j.cb && j.cb->render_update)
            j.cb->render_update(j, tb, ctb);
    }

    F32 top = _lines[begin_line].bounds.y;
    size_t li = begin_line - 1;
    S32 line_last = -1;
    F32 x = 0, y = 0;

    for (S32 i = begin_jot; i < end_jot; i++) {
        if (i >= line_last) {
            li++;
            const jot_line& line = _lines[li];
            x = tb.bounds.x + line.bounds.x;
            line_last = line.last;
            y = tb.bounds.y + line.bounds.y + line.baseline - top;
            y += li * tb.line_space;
            U32 xj = tb.flags & 0x3;

            if (xj == 0x2)
                x += 0.5f * (tb.bounds.w - line.bounds.w);
            else if (xj == 0x1)
                x += tb.bounds.w - line.bounds.w;

            U32 yj = tb.flags & 0xC;
            if (yj == 0x8)
                y += 0.5f * (tb.bounds.h - _lines_size * line.bounds.h - (_lines_size - 1) * tb.line_space);
            else if (yj == 0x4)
                y += tb.bounds.h - _lines_size * line.bounds.h - (_lines_size - 1) * tb.line_space;

            if (line.page_break && end_jot > line_last) end_jot = line_last;
        }

        jot& j = _jots[i];
        if (!j.cb) continue;

        if (j.cb->render_update)
            j.cb->render_update(j, tb, ctb);
        if (!j.flag.ethereal && !j.flag.invisible && j.cb->render)
            j.cb->render(j, tb, x + j.bounds.x, y);
    }

    stop_render(ctb);
}

F32 xtextbox::layout::yextent(F32 max, S32& size, S32 begin_jot, S32 end_jot) const
{
    size = 0;
    if (begin_jot < 0) begin_jot = 0;
    if (end_jot < begin_jot) end_jot = _jots_size;
    if (begin_jot >= end_jot) return 0;

    S32 begin_line = 0;

    for (;;) {
        if (begin_line >= (S32)_lines_size) return 0;
        if ((S32)_lines[begin_line].last > begin_jot) break;
        begin_line++;
    }

    F32 top = _lines[begin_line].bounds.y;
    max += top;
    S32 i = begin_line;

    for (;;) {
        if (i == (S32)_lines_size) break;
        const jot_line& line = _lines[i];
        if (line.bounds.y + line.bounds.h > max) { i--; break; }
        if ((S32)line.last >= end_jot) break;
        if (line.page_break) break;
        i++;
    }

    if (i < begin_line) return 0;

    const jot_line& line = _lines[i];
    size = ((S32)line.last >= end_jot ? end_jot : line.last) - begin_jot;
    return line.bounds.y + line.bounds.h - top;
}

bool xtextbox::layout::changed(const xtextbox& ctb)
{
    U32 flags1 = tb.flags & 0x70, flags2 = ctb.flags & 0x70;
    if (tb.text_hash != ctb.text_hash || tb.font.id != ctb.font.id || tb.font.width != ctb.font.width ||
        tb.font.height != ctb.font.height || tb.font.space != ctb.font.space || tb.bounds.w != ctb.bounds.w ||
        flags1 != flags2 || tb.line_space != ctb.line_space)
        return true;

    for (S32 i = dynamics_size; i > 0;) {
        i--;
        jot& j = _jots[dynamics[i]];
        U32 oldval = xStrHash((const char*)j.context, j.context_size);
        parse_next_jot(j, tb, ctb, j.s.text, j.s.size);
        U32 val = xStrHash((const char*)j.context, j.context_size);
        if (val != oldval)
            return true;
    }

    return false;
}

#define DEFINE_COLOR_TAG(name, field)                                                                                                               \
    void update_tag_##name(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)                                                                   \
    {                                                                                                                                               \
        tb.field = (U8)(255.0f * *(const F32*)&j.context + 0.5f);                                                                                   \
    }                                                                                                                                               \
    void update_tag_reset_##name(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)                                                           \
    {                                                                                                                                               \
        tb.field = ctb.field;                                                                                                                       \
    }                                                                                                                                               \
    void parse_tag_##name(xtextbox::jot& a, const xtextbox& tb, const xtextbox&, const xtextbox::split_tag& ti)                                     \
    {                                                                                                                                               \
        static const xtextbox::callback cb = { NULL, update_tag_##name, update_tag_##name };                                                        \
        F32& v = *(F32*)&a.context;                                                                                                                 \
        if (!ti.value.size || !ti.action.size) return;                                                                                              \
        v = xatof(ti.value.text);                                                                                                                   \
        switch (ti.action.text[0]) {                                                                                                                \
        case '=':                                                                                                                                   \
            break;                                                                                                                                  \
        case '+':                                                                                                                                   \
            v += (1.0f / 255) * tb.field;                                                                                                           \
            break;                                                                                                                                  \
        case '*':                                                                                                                                   \
            v *= (1.0f / 255) * tb.field;                                                                                                           \
            break;                                                                                                                                  \
        default:                                                                                                                                    \
            return;                                                                                                                                 \
        }                                                                                                                                           \
        if (v < 0.0f)                                                                                                                               \
            v = 0.0f;                                                                                                                               \
        else if (v > 1.0f)                                                                                                                          \
            v = 1.0f;                                                                                                                               \
        a.cb = &cb;                                                                                                                                 \
    }                                                                                                                                               \
    void reset_tag_##name(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)                                           \
    {                                                                                                                                               \
        static const xtextbox::callback cb = { NULL, update_tag_reset_##name, update_tag_reset_##name };                                            \
        a.cb = &cb;                                                                                                                                 \
    }

#define DEFINE_FLOAT_TAG(name, field, vmin, vmax, layout, render)                                                                                   \
    void update_tag_##name(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)                                                                   \
    {                                                                                                                                               \
        tb.field = *(const F32*)&j.context;                                                                                                         \
    }                                                                                                                                               \
    void update_tag_reset_##name(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)                                                           \
    {                                                                                                                                               \
        tb.field = ctb.field;                                                                                                                       \
    }                                                                                                                                               \
    void parse_tag_##name(xtextbox::jot& a, const xtextbox& tb, const xtextbox&, const xtextbox::split_tag& ti)                                     \
    {                                                                                                                                               \
        static const xtextbox::callback cb = { NULL, layout ? update_tag_##name : NULL, render ? update_tag_##name : NULL };                        \
        F32& v = *(F32*)&a.context;                                                                                                                 \
        if (!ti.value.size || !ti.action.size) return;                                                                                              \
        v = xatof(ti.value.text);                                                                                                                   \
        switch (ti.action.text[0]) {                                                                                                                \
        case '=':                                                                                                                                   \
            break;                                                                                                                                  \
        case '+':                                                                                                                                   \
            v += tb.field;                                                                                                                          \
            break;                                                                                                                                  \
        case '*':                                                                                                                                   \
            v *= tb.field;                                                                                                                          \
            break;                                                                                                                                  \
        default:                                                                                                                                    \
            return;                                                                                                                                 \
        }                                                                                                                                           \
        if (v < vmin)                                                                                                                               \
            v = vmin;                                                                                                                               \
        else if (v > vmax)                                                                                                                          \
            v = vmax;                                                                                                                               \
        a.cb = &cb;                                                                                                                                 \
    }                                                                                                                                               \
    void reset_tag_##name(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)                                           \
    {                                                                                                                                               \
        static const xtextbox::callback cb = { NULL, layout ? update_tag_reset_##name : NULL, render ? update_tag_reset_##name : NULL };            \
        a.cb = &cb;                                                                                                                                 \
    }

namespace {

DEFINE_COLOR_TAG(alpha, font.color.a)
DEFINE_COLOR_TAG(red, font.color.r)
DEFINE_COLOR_TAG(green, font.color.g)
DEFINE_COLOR_TAG(blue, font.color.b)
DEFINE_COLOR_TAG(salpha, font.shadowColor.a)
DEFINE_COLOR_TAG(sred, font.shadowColor.r)
DEFINE_COLOR_TAG(sgreen, font.shadowColor.g)
DEFINE_COLOR_TAG(sblue, font.shadowColor.b)
DEFINE_FLOAT_TAG(width, font.width, 0.0f, 1.0f, true, true)
DEFINE_FLOAT_TAG(height, font.height, 0.0f, 1.0f, true, true)
DEFINE_FLOAT_TAG(left_indent, left_indent, -1.0f, 1.0f, true, false)
DEFINE_FLOAT_TAG(right_indent, right_indent, -1.0f, 1.0f, true, false)
DEFINE_FLOAT_TAG(tab_stop, tab_stop, 0.0f, 1.0f, true, false)
DEFINE_FLOAT_TAG(xspace, font.space, -1.0f, 1.0f, true, true)
DEFINE_FLOAT_TAG(yspace, line_space, -1.0f, 1.0f, true, false)

void update_tag_reset_all(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb) { tb = ctb; }

void reset_tag_all(xtextbox::jot& j, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)
{
    static const xtextbox::callback cb = { NULL, update_tag_reset_all, update_tag_reset_all };
    j.cb = &cb;
}

void update_tag_color(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
{
    tb.font.color.r = ((const xColor*)&j.context)->r;
    tb.font.color.g = ((const xColor*)&j.context)->g;
    tb.font.color.b = ((const xColor*)&j.context)->b;
}

void update_tag_reset_color(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb) { tb.font.color = ctb.font.color; }

void parse_tag_color(xtextbox::jot& a, const xtextbox& tb, const xtextbox&, const xtextbox::split_tag& ti)
{
    static const xtextbox::callback cb = { NULL, update_tag_color, update_tag_color };

    if (ti.value.size < 6 || !ti.action.size) return;

    xColor& color = *(xColor*)&a.context;
    U32 v = atox(ti.value);
    if (ti.value.size < 8) v = (v & 0xFF000000) | (tb.font.color.a << 24);

    switch (ti.action.text[0]) {
    case '=':
        color.a = v >> 24;
        color.r = (v >> 16) & 0xFF;
        color.g = (v >> 8) & 0xFF;
        color.b = v & 0xFF;
        break;
    case '+': {
        U32 temp = (v >> 24) + tb.font.color.a;
        color.a = MIN(255, temp);
        temp = ((v >> 16) & 0xFF) + tb.font.color.r;
        color.r = MIN(255, temp);
        temp = ((v >> 8) & 0xFF) + tb.font.color.g;
        color.g = MIN(255, temp);
        temp = (v & 0xFF) + tb.font.color.b;
        color.b = MIN(255, temp);
    }
        break;
    case '*':
        color.a = (v >> 24) * tb.font.color.a / 255;
        color.r = ((v >> 16) & 0xFF) * tb.font.color.r / 255;
        color.g = ((v >> 8) & 0xFF) * tb.font.color.g / 255;
        color.b = (v & 0xFF) * tb.font.color.b / 255;
        break;
    default: return;
    }

    a.cb = &cb;
}

void reset_tag_color(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)
{
    static const xtextbox::callback cb = { NULL, update_tag_reset_color, update_tag_reset_color };
    a.cb = &cb;
}

void update_tag_font(const xtextbox::jot& j, xtextbox& tb, const xtextbox&) { tb.font.id = (U32)j.context; }

void update_tag_reset_font(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb) { tb.font.id = ctb.font.id; }

void parse_tag_font(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag& ti)
{
    static const xtextbox::callback cb = { NULL, update_tag_font, update_tag_font };

    if (ti.action.size < 1 || ti.action.text[0] != '=' || !ti.value.size)
        return;

    U32& id = *(U32*)&a.context;
    id = atoi(ti.value.text);
    if (id < active_fonts_size)
        a.cb = &cb;
}

void reset_tag_font(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)
{
    static const xtextbox::callback cb = { NULL, update_tag_reset_font, update_tag_reset_font };
    a.cb = &cb;
}

void update_tag_wrap(const xtextbox::jot& j, xtextbox& tb, const xtextbox&) { tb.flags = (tb.flags & ~0x30) | ((U32)j.context & 0x30); }

void update_tag_reset_wrap(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb) { tb.flags = (tb.flags & ~0x30) | (ctb.flags & 0x30); }

void parse_tag_wrap(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag& ti)
{
    static const xtextbox::callback cb = { NULL, update_tag_wrap, NULL };

    if (ti.action.size < 1 || ti.action.text[0] != '=' || ti.value.size != 4)
        return;

    U32& flags = *(U32*)&a.context;
    if (imemcmp(ti.value.text, "word", 4) == 0) flags = 0x0;
    else if (imemcmp(ti.value.text, "char", 4) == 0) flags = 0x10;
    else if (imemcmp(ti.value.text, "none", 4) == 0) flags = 0x20;
    else return;

    a.cb = &cb;
}

void reset_tag_wrap(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)
{
    static const xtextbox::callback cb = { NULL, update_tag_reset_wrap, NULL };
    a.cb = &cb;
}

void update_tag_xjustify(const xtextbox::jot& j, xtextbox& tb, const xtextbox&) { tb.flags = (tb.flags & ~0x3) | ((U32)j.context & 0x3); }

void update_tag_reset_xjustify(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb) { tb.flags = (tb.flags & ~0x3) | (ctb.flags & 0x3); }

void parse_tag_xjustify(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag& ti)
{
    static const xtextbox::callback cb = { NULL, NULL, update_tag_xjustify };

    if (ti.action.size < 1 || ti.action.text[0] != '=') return;

    U32& flags = *(U32*)&a.context;
    if (icompare(ti.value, substr::create("left", 4)) == 0)
        flags = 0x0;
    else if (icompare(ti.value, substr::create("center", 6)) == 0)
        flags = 0x2;
    else if (icompare(ti.value, substr::create("right", 5)) == 0)
        flags = 0x1;
    else return;

    a.cb = &cb;
}

void reset_tag_xjustify(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)
{
    static const xtextbox::callback cb = { NULL, NULL, update_tag_reset_xjustify };
    a.cb = &cb;
}

void update_tag_yjustify(const xtextbox::jot& j, xtextbox& tb, const xtextbox&) { tb.flags = (tb.flags & ~0xC) | ((U32)j.context & 0xC); }

void update_tag_reset_yjustify(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb) { tb.flags = (tb.flags & ~0xC) | (ctb.flags & 0xC); }

void parse_tag_yjustify(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag& ti)
{
    static const xtextbox::callback cb = { NULL, NULL, update_tag_yjustify };

    if (ti.action.size < 1 || ti.action.text[0] != '=') return;

    U32& flags = *(U32*)&a.context;
    if (icompare(ti.value, substr::create("top", 4)) == 0)
        flags = 0x0;
    else if (icompare(ti.value, substr::create("center", 4)) == 0)
        flags = 0x8;
    else if (icompare(ti.value, substr::create("bottom", 4)) == 0)
        flags = 0x4;
    else return;

    a.cb = &cb;
}

void reset_tag_yjustify(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)
{
    static const xtextbox::callback cb = { NULL, NULL, update_tag_reset_yjustify };
    a.cb = &cb;
}

void parse_tag_open_curly(xtextbox::jot& a, const xtextbox& tb, const xtextbox&, const xtextbox::split_tag& ti)
{
    a.s.text = ti.tag.text;
    a.s.size = 1;
    char c = *a.s.text;
    a.reset_flags();
    a.bounds = tb.font.bounds(c);
    a.cb = tb.cb;
    a.context = NULL;
}

void parse_tag_newline(xtextbox::jot& a, const xtextbox& tb, const xtextbox&, const xtextbox::split_tag&)
{
    a.flag.line_break = true;
    a.flag.ethereal = false;
    a.bounds = tb.font.bounds('\n');
}

void parse_tag_nbsp(xtextbox::jot& a, const xtextbox& tb, const xtextbox&, const xtextbox::split_tag&)
{
    a.reset_flags();
    a.flag.insert = true;
    a.flag.merge = true;
    a.cb = &xtextbox::text_cb;
    a.s = substr::create(" ", 1);
    a.bounds = tb.font.bounds(' ');
}

void parse_tag_tab(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag&) { a.flag.tab = true; }

void parse_tag_word_break(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag&) { a.flag.word_break = true; }

void parse_tag_page_break(xtextbox::jot& a, const xtextbox&, const xtextbox&, const xtextbox::split_tag&) { a.flag.page_break = true; }

struct model_tag_context
{
    xModelInstance* model;
    xVec3 rot;
    basic_rect<F32> dst;
    xSphere o;
};

void render_tag_model(const xtextbox::jot& j, const xtextbox& tb, F32 x, F32 y)
{
    const model_tag_context& mtc = *(const model_tag_context*)j.context;

    basic_rect<F32> dst = mtc.dst; dst.move(x, y);
    xVec3 from = { 0, 0, 1 }, to = { 0, 0, -0.001f };

    xMat4x3 frame;
    xMat3x3Euler(&frame, &mtc.rot);
    F32 ir = mtc.o.r <= 0 ? 1.0f : 0.5f / mtc.o.r;
    F32 scale = 1.001f * ir;
    frame.right *= scale; frame.up *= scale; frame.at *= scale;
    frame.pos = mtc.o.center;
    frame.flags = 0;

    xModelSetFrame(mtc.model, &frame);
    xModelSetMaterialAlpha(mtc.model, tb.font.color.a);

    tex_flush();
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    xModelRender2D(*mtc.model, dst, from, to);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEFLAT);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
}

void parse_tag_model(xtextbox::jot& a, const xtextbox& tb, const xtextbox&, const xtextbox::split_tag& ti)
{
    static const xtextbox::callback cb = { render_tag_model, NULL, NULL };

    load_model_args(def_model_args, ti.tag);
    if (!def_model_args.model) return;

    model_tag_context& mtc = *(model_tag_context*)a.context;
    mtc.model = def_model_args.model;
    mtc.rot = def_model_args.rot; mtc.rot *= PI / 180;
    mtc.dst = def_model_args.dst;

    const xSphere* o = xModelGetLocalSBound(mtc.model);
    mtc.o = *o;

    mtc.dst.y -= def_model_args.off.y;
    a.bounds.assign(0, -def_model_args.off.y, def_model_args.off.x, def_model_args.off.y);

    switch (def_model_args.scale) {
    case model_args::SCALE_SCREEN:
        break;
    case model_args::SCALE_SIZE:
        mtc.dst.scale(mtc.o.r);
        a.bounds.scale(mtc.o.r);
        break;
    default:
        mtc.dst.scale(tb.font.width, tb.font.height);
        a.bounds.scale(tb.font.width, tb.font.height);
        break;
    }

    a.reset_flags();
    a.context_size = sizeof(model_tag_context);
    a.cb = &cb;
}

void reset_tag_model(xtextbox::jot&, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)
{
    reset_model_args(def_model_args);
}

inline xVec2 get_texture_size(RwRaster& raster)
{
    xVec2 ret = { (F32)RwRasterGetWidth(&raster) / SCREEN_WIDTH, (F32)RwRasterGetHeight(&raster) / SCREEN_HEIGHT };
    return ret;
}

struct tex_tag_context
{
    RwRaster* raster;
    F32 rot;
    basic_rect<F32> src;
    basic_rect<F32> dst;
};

void render_tag_tex(const xtextbox::jot& j, const xtextbox& tb, F32 x, F32 y)
{
    const tex_tag_context& ttc = *(const tex_tag_context*)j.context;
    set_tex_raster(ttc.raster);
    basic_rect<F32> dst = ttc.dst; dst.move(x, y);
    tex_render(ttc.src, dst, tb.font.clip, tb.font.color);
}

void parse_tag_tex(xtextbox::jot& a, const xtextbox& tb, const xtextbox&, const xtextbox::split_tag& ti)
{
    static const xtextbox::callback cb = { render_tag_tex, NULL, NULL };

    load_tex_args(def_tex_args, ti.tag);
    if (!def_tex_args.raster) return;

    tex_tag_context& ttc = *(tex_tag_context*)a.context;
    ttc.raster = def_tex_args.raster;
    ttc.rot = def_tex_args.rot;
    ttc.src = def_tex_args.src;
    ttc.dst = def_tex_args.dst;

    ttc.dst.y -= def_tex_args.off.y;
    a.bounds.assign(0, -def_tex_args.off.y, def_tex_args.off.x, def_tex_args.off.y);

    xVec2 scale;
    switch (def_tex_args.scale) {
    case tex_args::SCALE_SCREEN:
        scale.assign(1, 1);
        break;
    case tex_args::SCALE_SIZE:
        scale = get_texture_size(*ttc.raster);
        break;
    case tex_args::SCALE_FONT_WIDTH:
        scale = get_texture_size(*ttc.raster);
        scale.y *= tb.font.width / scale.x;
        scale.x = tb.font.width;
        break;
    case tex_args::SCALE_FONT_HEIGHT:
        scale = get_texture_size(*ttc.raster);
        scale.x *= tb.font.height / scale.y;
        scale.y = tb.font.height;
        break;
    case tex_args::SCALE_SCREEN_WIDTH:
        scale = get_texture_size(*ttc.raster);
        scale.y *= 1.0f / scale.x;
        scale.x = 1.0f;
        break;
    case tex_args::SCALE_SCREEN_HEIGHT:
        scale = get_texture_size(*ttc.raster);
        scale.x *= 1.0f / scale.y;
        scale.y = 1.0f;
        break;
    default:
        scale.assign(tb.font.width, tb.font.height);
        break;
    }

    ttc.dst.scale(scale.x, scale.y);
    a.bounds.scale(scale.x, scale.y);
    a.reset_flags();
    a.context_size = sizeof(tex_tag_context);
    a.cb = &cb;
}

void reset_tag_tex(xtextbox::jot&, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)
{
    reset_tex_args(def_tex_args);
}

void parse_tag_insert(xtextbox::jot& j, const xtextbox&, const xtextbox&, const xtextbox::split_tag& ti)
{
    j.reset_flags();
    j.flag.invisible = j.flag.ethereal = true;
    if (ti.action.size != 1 || ti.action.text[0] != ':' || !ti.value.size)
        return;

    U32 id = xStrHash(ti.value.text, ti.value.size);
    if (!id) return;

    U32 len;
    const char* text = xTextFindString(id, &len);
    if (!text) return;

    j.context = (void*)text;
    j.context_size = len;
    j.flag.insert = true;
}

void parse_tag_insert_hash(xtextbox::jot& j, const xtextbox&, const xtextbox&, const xtextbox::split_tag& ti)
{
    j.reset_flags();
    j.flag.invisible = j.flag.ethereal = true;
    if (ti.action.size != 1 || ti.action.text[0] != ':' || !ti.value.size)
        return;

    U32 id = atox(ti.value);
    if (!id) return;

    U32 len;
    const char* text = xTextFindString(id, &len);
    if (!text) return;

    j.context = (void*)text;
    j.context_size = len;
    j.flag.insert = true;
}

void parse_tag_pop(xtextbox::jot&, const xtextbox&, const xtextbox&, const xtextbox::split_tag& ti)
{
    if (ti.action.size != 1 || ti.action.text[0] != ':' || !ti.value.size) {
        return;
    }
}

void parse_tag_timer(xtextbox::jot& j, const xtextbox&, const xtextbox&, const xtextbox::split_tag& ti)
{
    j.reset_flags();
    j.flag.invisible = j.flag.ethereal = true;
    if (ti.action.size != 1 || ti.action.text[0] != ':' || !ti.value.size)
        return;

    U32 id = xStrHash(ti.value.text, ti.value.size);
    if (!id) return;

    const xTimer* ta = (const xTimer*)zSceneFindObject(id);
    if (!ta) return;

    j.flag.insert = j.flag.dynamic = true;

    char buffer[64];
    U32 sec = (U32)ta->secondsLeft + 1;
    U32 mn = sec / 60;
    if (mn)
        sprintf(buffer, "%d:%02d", mn, sec % 60);
    else
        sprintf(buffer, "%02d", sec);

    char* text = (char*)j.context;
    sprintf(text, "%.*s", 15, buffer);
    j.context_size = 16;
}

void parse_tag_counter(xtextbox::jot& j, const xtextbox&, const xtextbox&, const xtextbox::split_tag& ti)
{
    j.reset_flags();
    j.flag.invisible = j.flag.ethereal = true;
    if (ti.action.size != 1 || ti.action.text[0] != ':' || !ti.value.size)
        return;

    U32 id = xStrHash(ti.value.text, ti.value.size);
    xCounter* counter = (xCounter*)zSceneFindObject(id);
    if (!counter || counter->baseType != eBaseTypeCounter) {
        return;
    }

    char* buffer = (char*)j.context;
    sprintf(buffer, "%d", counter->count);
    j.context_size = strlen(buffer);
}

xtextbox::tag_type format_tags_buffer[2][128] = {
    {
        { { "", 0 }, parse_tag_open_curly, NULL, NULL },
        { { "a", 1 }, parse_tag_alpha, reset_tag_alpha, NULL },
        { { "all", 3 }, NULL, reset_tag_all, NULL },
        { { "alpha", 5 }, parse_tag_alpha, reset_tag_alpha, NULL },
        { { "b", 1 }, parse_tag_blue, reset_tag_blue, NULL },
        { { "blue", 4 }, parse_tag_blue, reset_tag_blue, NULL },
        { { "c", 1 }, parse_tag_color, reset_tag_color, NULL },
        { { "color", 5 }, parse_tag_color, reset_tag_color, NULL },
        { { "counter", 7 }, parse_tag_counter, NULL, NULL },
        { { "f", 1 }, parse_tag_font, reset_tag_font, NULL },
        { { "font", 4 }, parse_tag_font, reset_tag_font, NULL },
        { { "g", 1 }, parse_tag_green, reset_tag_green, NULL },
        { { "green", 5 }, parse_tag_green, reset_tag_green, NULL },
        { { "h", 1 }, parse_tag_height, reset_tag_height, NULL },
        { { "height", 6 }, parse_tag_height, reset_tag_height, NULL },
        { { "i", 1 }, parse_tag_insert, NULL, NULL },
        { { "ih", 2 }, parse_tag_insert_hash, NULL, NULL },
        { { "insert", 6 }, parse_tag_insert, NULL, NULL },
        { { "left_indent", 11 }, parse_tag_left_indent, reset_tag_left_indent, NULL },
        { { "li", 2 }, parse_tag_left_indent, reset_tag_left_indent, NULL },
        { { "model", 5 }, parse_tag_model, reset_tag_model, NULL },
        { { "n", 1 }, parse_tag_newline, NULL, NULL },
        { { "nbsp", 4 }, parse_tag_nbsp, NULL, NULL },
        { { "newline", 7 }, parse_tag_newline, NULL, NULL },
        { { "page_break", 10 }, parse_tag_page_break, NULL, NULL },
        { { "pb", 2 }, parse_tag_page_break, NULL, NULL },
        { { "pop", 3 }, parse_tag_pop, NULL, NULL },
        { { "r", 1 }, parse_tag_red, reset_tag_red, NULL },
        { { "red", 3 }, parse_tag_red, reset_tag_red, NULL },
        { { "ri", 2 }, parse_tag_right_indent, reset_tag_right_indent, NULL },
        { { "right_indent", 12 }, parse_tag_right_indent, reset_tag_right_indent, NULL },
        { { "sa", 2 }, parse_tag_salpha, reset_tag_salpha, NULL },
        { { "salpha", 6 }, parse_tag_salpha, reset_tag_salpha, NULL },
        { { "sb", 2 }, parse_tag_sblue, reset_tag_sblue, NULL },
        { { "sblue", 5 }, parse_tag_sblue, reset_tag_sblue, NULL },
        { { "sg", 2 }, parse_tag_sgreen, reset_tag_sgreen, NULL },
        { { "sgreen", 6 }, parse_tag_sgreen, reset_tag_sgreen, NULL },
        { { "sr", 2 }, parse_tag_sred, reset_tag_sred, NULL },
        { { "sred", 4 }, parse_tag_sred, reset_tag_sred, NULL },
        { { "t", 1 }, parse_tag_tab, NULL, NULL },
        { { "tab", 3 }, parse_tag_tab, NULL, NULL },
        { { "tab_stop", 8 }, parse_tag_tab_stop, reset_tag_tab_stop, NULL },
        { { "tex", 3 }, parse_tag_tex, reset_tag_tex, NULL },
        { { "timer", 5 }, parse_tag_timer, NULL, NULL },
        { { "ts", 2 }, parse_tag_tab_stop, reset_tag_tab_stop, NULL },
        { { "w", 1 }, parse_tag_width, reset_tag_width, NULL },
        { { "wb", 2 }, parse_tag_word_break, NULL, NULL },
        { { "width", 5 }, parse_tag_width, reset_tag_width, NULL },
        { { "word_break", 10 }, parse_tag_word_break, NULL, NULL },
        { { "wrap", 4 }, parse_tag_wrap, reset_tag_wrap, NULL },
        { { "xj", 2 }, parse_tag_xjustify, reset_tag_xjustify, NULL },
        { { "xjustify", 8 }, parse_tag_xjustify, reset_tag_xjustify, NULL },
        { { "xs", 2 }, parse_tag_xspace, reset_tag_xspace, NULL },
        { { "xspace", 6 }, parse_tag_xspace, reset_tag_xspace, NULL },
        { { "yj", 2 }, parse_tag_yjustify, reset_tag_yjustify, NULL },
        { { "yjustify", 8 }, parse_tag_yjustify, reset_tag_yjustify, NULL },
        { { "ys", 2 }, parse_tag_yspace, reset_tag_yspace, NULL },
        { { "yspace", 6 }, parse_tag_yspace, reset_tag_yspace, NULL },
    },
};

xtextbox::tag_type* format_tags = format_tags_buffer[0];
size_t format_tags_size = 58;

} // namespace

void xtextbox::register_tags(const tag_type* t, size_t size)
{
    const tag_type *s1 = format_tags, *s2 = t;
    const tag_type *end1 = s1 + format_tags_size, *end2 = s2 + size;
    tag_type* d = s1 == format_tags_buffer[0] ? format_tags_buffer[1] : format_tags_buffer[0];
    format_tags = d;

    for (; s1 < end1 && s2 < end2; d++) {
        const tag_type &t1 = *s1, &t2 = *s2;
        S32 c = icompare(t1.name, t2.name);
        if (c < 0) { *d = t1; s1++; }
        else if (c > 0) { *d = t2; s2++; }
        else { *d = t2; s1++; s2++; }
    }

    for (; s1 < end1; ++d, ++s1) *d = *s1;
    for (; s2 < end2; ++d, ++s2) *d = *s2;

    format_tags_size = d - format_tags;
}

xtextbox::tag_type* xtextbox::find_format_tag(const substr& s, S32& index)
{
    S32 start = 0, end = format_tags_size;
    while (start != end) {
        index = (start + end) / 2;
        tag_type& t = format_tags[index];
        S32 c = icompare(s, t.name);
        if (c < 0) end = index;
        else if (c > 0) start = index + 1;
        else return &t;
    }

    index = -1;
    return NULL;
}

namespace {

inline void set_rect_vert(RwIm2DVertex& vert, F32 x, F32 y, F32 z, xColor c, F32 rcz)
{
    RwIm2DVertexSetScreenX(&vert, x);
    RwIm2DVertexSetScreenY(&vert, y);
    RwIm2DVertexSetScreenZ(&vert, z);
    RwIm2DVertexSetRecipCameraZ(&vert, rcz);
    RwIm2DVertexSetIntRGBA(&vert, c.r, c.g, c.b, c.a);
}

inline void set_rect_verts(RwIm2DVertex* vert, F32 x, F32 y, F32 w, F32 h, xColor c, F32 rcz, F32 nsz)
{
    set_rect_vert(vert[0], x, y, nsz, c, rcz);
    set_rect_vert(vert[1], x, y + h, nsz, c, rcz);
    set_rect_vert(vert[2], x + w, y, nsz, c, rcz);
    set_rect_vert(vert[3], x + w, y + h, nsz, c, rcz);
}

} // namespace

void render_fill_rect(const basic_rect<F32>& bounds, xColor color)
{
    if (bounds.empty()) return;
    RwReal rcz = 1.0f / RwCameraGetNearClipPlane(RwCameraGetCurrentCamera());
    RwReal nsz = RwIm2DGetNearScreenZ();
    xfont::set_render_state(NULL);

    RwIm2DVertex vert[4];
    basic_rect<F32> r = bounds;
    r.scale(FB_XRES, FB_YRES);
    set_rect_verts(vert, r.x, r.y, r.w, r.h, color, rcz, nsz);
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vert, ARRAY_LENGTH(vert));
    xfont::restore_render_state();
}

S32 textstrlen(const char* string)
{
    S32 ret = 0;
    while (*string) {
        if ((U8)*string >= 0x80)
            string++;
        string++;
        ret++;
    }
    return ret;
}

char* textstrncpy(char* dest, const char* source, S32 chars, char** outDest, const char** outSource)
{
    char* ret = dest;
    while (chars-- > 0) {
        *dest++ = *source;
        if ((U8)*source++ >= 0x80)
            *dest++ = *source++;
    }

    if (outSource)
        *outSource = source;
    if (outDest)
        *outDest = dest;
    *dest = '\0';
    return ret;
}

static void PrintWIPText()
{
    static const char* TXT_WIP = NULL;
    if (!TXT_WIP) {
        TXT_WIP = xTextFindString("TXT_WIP", NULL);
    }

    static F32 wipTimer = 0.0f;
    wipTimer += gRealTimeElapsed;

    static xTWEAKABLE F32 fadeInTime = 0.5f, onTime = 0.5f, fadeOutTime = 0.5f, offTime = 0.05f;

    F32 totalTime = fadeInTime + onTime + fadeOutTime + offTime;
    if (wipTimer > totalTime) {
        wipTimer -= totalTime * (S32)(wipTimer / totalTime);
    }

    if (wipTimer < fadeInTime + onTime + fadeOutTime) {
        xColor color = { 0xFF, 0xFF, 0xFF, 0x80 }, dropcolor = { 0x00, 0x00, 0x00, 0x80 };

        F32 fadePer;
        if (wipTimer < fadeInTime)
            fadePer = wipTimer / fadeInTime;
        else if (wipTimer > fadeInTime + onTime)
            fadePer = fadeOutTime - (wipTimer - fadeInTime - onTime);
        else
            fadePer = 1.0f;

        color.a = dropcolor.a * fadePer;
        dropcolor.a = dropcolor.a * fadePer;

        basic_rect<F32> bounds = { 0, 1.0f - 2.0f * NSCREENY(30), 1.0f - NSCREENX(30), NSCREENY(30) };
        static xtextbox tb = xtextbox::create(xfont::create(0, NSCREENX(20), NSCREENY(20), 0, g_WHITE), bounds, 0x1);

        tb.set_text(TXT_WIP);
        tb.font.shadowColor = dropcolor;
        tb.font.shadowOffsetX = 2.0f;
        tb.font.shadowOffsetY = 2.0f;
        tb.font.color = color;
        tb.bounds = bounds;
        tb.render(true);
    }
}

static void PrintWatermark()
{
    basic_rect<F32> bounds = { 0, 0.05f, 0.95f, 1.5f * NSCREENY(xglobals->watermarkSize) };
    xtextbox tb = xtextbox::create(xfont::create(0, NSCREENX(xglobals->watermarkSize), NSCREENY(xglobals->watermarkSize), 0, g_WHITE), bounds, 0x1);

    tb.set_text(xglobals->watermark);

    xColor color = { 0xFF, 0xFF, 0xFF, xglobals->watermarkAlpha };
    tb.font.color = color;
    xColor dropcolor = { 0x00, 0x00, 0x00, xglobals->watermarkAlpha };
    tb.font.shadowColor = dropcolor;
    tb.font.shadowOffsetX = 2.0f;
    tb.font.shadowOffsetY = 2.0f;
    tb.font.color = color;
    tb.bounds = bounds;
    tb.render(true);
}

void xFontPrintTopText()
{
    if (xglobals->FlashWIP)
        PrintWIPText();
    if (xglobals->watermark[0])
        PrintWatermark();
}

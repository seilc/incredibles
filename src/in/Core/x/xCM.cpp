#include "xCM.h"

#include "xDebug.h"
#include "xEvent.h"
#include "xFont.h"
#include "xPad.h"
#include "xRenderState.h"
#include "xstransvc.h"
#include "zGame.h"
#include "zGlobals.h"
#include "zMenu.h"
#include "zScene.h"

#include <stdint.h>

const U32 CM_ALIGN_CENTER = 0;
const U32 CM_ALIGN_LEFT = 1;
const U32 CM_ALIGN_RIGHT = 2;
const U32 CM_ALIGN_INNER = 3;
const U32 CM_ALIGN_TEXTURE = 4;
const U32 CM_ALIGN_MAX = 5;

struct sxy
{
    F32 x;
    F32 y;
};

struct fade
{
    F32 start;
    F32 end;
};

struct xCreditsData
{
    U32 dummy;
};

struct xCMheader
{
    U32 magic;
    U32 version;
    U32 crdID;
    U32 state;
    F32 total_time;
    U32 total_size;
};

struct xCMcredits
{
    U32 credits_size;
    F32 len;
    U32 flags;
    sxy in;
    sxy out;
    F32 scroll_rate;
    F32 lifetime;
    fade fin;
    fade fout;
    U32 num_presets;
};

struct xCMtextbox
{
    U32 font;
    xColor color;
    sxy char_size;
    sxy char_spacing;
    sxy box;
};

struct xCMtexture
{
    U32 assetID;
    xColor color;
    F32 x;
    F32 y;
    F32 w;
    F32 h;
    RwTexture* texture;
    U32 pad;
};

struct xCMpreset
{
    U16 num;
    U16 align;
    F32 delay;
    F32 innerspace;
    xCMtextbox box[2];
};

struct xCMhunk
{
    U32 hunk_size;
    U32 preset;
    F32 t0;
    F32 t1;
    const char* text1;
    const char* text2;
};

struct xColorUnpack
{
    U8 r, g, b, a;
};

static const char* KEY = "xCMChunkHandler";

static void Decrypt(void* buffer_, U32 size, const void* key_, U32 keySize)
{
    U8* buffer = (U8*)buffer_;
    const U8* key = (const U8*)key_;
    U8 last = 0;
    U32 keyCurrent = 0;

    while (size) {
        *buffer = *buffer ^ last ^ key[keyCurrent];
        last = *buffer;
        buffer++;
        keyCurrent = (keyCurrent + 1) % keySize;
        size--;
    }
}

static void xCMprep(xCreditsData* data)
{
    xCMheader* hdr = (xCMheader*)data;
    if (hdr->magic != 0xBEEEEEEF) {
        return;
    }

    // Fakematch: different declaration order than DWARF
    xCMhunk* hp;
    char* dp;
    xCMcredits* cp;
    xCMpreset* pp;

    dp = (char*)(hdr + 1);
    while (dp - (char*)data < hdr->total_size) {
        cp = (xCMcredits*)dp;
        pp = (xCMpreset*)(cp + 1);
        hp = (xCMhunk*)(pp + cp->num_presets);

        while ((char*)hp - (char*)cp < cp->credits_size) {
            xASSERT(242, hp->preset >= 0 && hp->preset < cp->num_presets);
            xCMpreset* preset = &pp[hp->preset];
            switch (preset->align) {
            case CM_ALIGN_CENTER:
            case CM_ALIGN_LEFT:
            case CM_ALIGN_RIGHT:
            case CM_ALIGN_INNER:
                if ((hdr->state & 0x1) == 0) {
                    if (hp->text1) {
                        hp->text1 = (const char*)((uintptr_t)hp->text1 - (uintptr_t)data);
                    }
                    if (hp->text2) {
                        hp->text2 = (const char*)((uintptr_t)hp->text2 - (uintptr_t)data);
                    }
                } else {
                    if (hp->text1) {
                        hp->text1 = (const char*)((uintptr_t)hp->text1 + (uintptr_t)data);
                    }
                    if (hp->text2) {
                        hp->text2 = (const char*)((uintptr_t)hp->text2 + (uintptr_t)data);
                    }
                }
                break;
            case CM_ALIGN_TEXTURE:
                if (hp->hunk_size > sizeof(xCMhunk)) {
                    hp->text2 = (const char*)(hp + 1);
                    if (preset->box[1].char_spacing.x > 1.01f) {
                        preset->box[1].char_spacing.x = NSCREENX(preset->box[1].char_spacing.x);
                    }
                    if (preset->box[1].char_spacing.y > 1.01f) {
                        preset->box[1].char_spacing.y = NSCREENY(preset->box[1].char_spacing.y);
                    }
                }
                break;
            }

            hp = (xCMhunk*)((char*)hp + hp->hunk_size);
        }

        dp = (char*)hp;
    }
    
    U32 newState = ((hdr->state & 0x1) == 0) ? 1 : 0;
    hdr->state &= ~0x1;
    hdr->state |= newState;
}

static S32 Im2DRenderQuadAlpha(F32 x1, F32 y1, F32 x2, F32 y2, F32 alpha, F32 z, F32 recipCamZ, F32 uvOffset)
{
    RwIm2DVertex vx[4];
    U8 alphaI = 255.0f * alpha;

    RwIm2DVertexSetScreenX(&vx[0], x1);
    RwIm2DVertexSetScreenY(&vx[0], y1);
    RwIm2DVertexSetScreenZ(&vx[0], z);
    RwIm2DVertexSetIntRGBA(&vx[0], 255, 255, 255, alphaI);
    RwIm2DVertexSetU(&vx[0], uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[0], uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[1], x1);
    RwIm2DVertexSetScreenY(&vx[1], y2);
    RwIm2DVertexSetScreenZ(&vx[1], z);
    RwIm2DVertexSetIntRGBA(&vx[1], 255, 255, 255, alphaI);
    RwIm2DVertexSetU(&vx[1], uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[1], 1.0f + uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[2], x2);
    RwIm2DVertexSetScreenY(&vx[2], y1);
    RwIm2DVertexSetScreenZ(&vx[2], z);
    RwIm2DVertexSetIntRGBA(&vx[2], 255, 255, 255, alphaI);
    RwIm2DVertexSetU(&vx[2], 1.0f + uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[2], uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[3], x2);
    RwIm2DVertexSetScreenY(&vx[3], y2);
    RwIm2DVertexSetScreenZ(&vx[3], z);
    RwIm2DVertexSetIntRGBA(&vx[3], 255, 255, 255, alphaI);
    RwIm2DVertexSetU(&vx[3], 1.0f + uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[3], 1.0f + uvOffset, recipCamZ);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vx, 4);

    return 1;
}

static F32 destX = 0.0f;
static F32 destY = 0.0f;
static F32 destWidth = 1.0f;
static F32 destHeight = 1.0f;

static void xCMScaleBounds(basic_rect<F32>& r)
{
    r.x = r.x * destWidth + destX;
    r.y = r.y * destHeight + destY;
    r.w *= destWidth;
    r.h *= destHeight;
}

static void xCMScaleBounds(F32& x0, F32& y0, F32& x1, F32& y1)
{
    x0 = x0 * destWidth + destX;
    y0 = y0 * destHeight + destY;
    x1 = x1 * destWidth + destX;
    y1 = y1 * destHeight + destY;
}

static xColor xCMcolor_scale(xColor color, F32 scale)
{
    xColor tmp = color;
    xColorUnpack tmp2 = *(xColorUnpack*)&tmp;

    F32 r = (F32)tmp2.r;
    F32 g = (F32)tmp2.g;
    F32 b = (F32)tmp2.b;
    F32 a = (F32)tmp2.a;

    a *= scale;

    xColorInit(tmp, (U8)r, (U8)g, (U8)b, (U8)a);

    return tmp;
}

#if DEBUG || RELEASE
U32 dumpit = 0;
#endif

static void xCMdump_header(xCMheader* hdr)
{
    iprintf("header magic:              %08X\n", hdr->magic);
    iprintf("       version:            %08X\n", hdr->version);
    iprintf("       state:              %d\n", hdr->state);
    iprintf("       total_time:         %6.2f\n", hdr->total_time);
    iprintf("       total_size:         %d bytes\n", hdr->total_size);
}

static void xCMdump_credits(xCMcredits* credits)
{
    iprintf("credits credits_size:      %d bytes\n", credits->credits_size);
    iprintf("        len:               %6.2f sec\n", credits->len);
    iprintf("        flags:             %08X\n", credits->flags);
    iprintf("        in:                %6.2f - %6.2f\n", credits->in.x, credits->in.y);
    iprintf("        out:               %6.2f - %6.2f\n", credits->out.x, credits->out.y);
    iprintf("        scroll_rate:       %6.2f pixels/sec\n", credits->scroll_rate);
    iprintf("        lifetime:          %6.2f sec\n", credits->lifetime);
    iprintf("        fin:               %6.2f - %6.2f\n", credits->fin.start, credits->fin.end);
    iprintf("        fout:              %6.2f - %6.2f\n", credits->fout.start, credits->fout.end);
    iprintf("        num_presets:       %d\n", credits->num_presets);
}

static void xCMdump_textbox(xCMtextbox* textbox)
{
    iprintf("textbox font:              %d\n", textbox->font);
    iprintf("        color:             %08X\n", *(U32*)&textbox->color);
    iprintf("        char_size:         %6.2f x %6.2f pixels\n", textbox->char_size.x, textbox->char_size.y);
    iprintf("        char_spacing:      %6.2f x %6.2f pixels\n", textbox->char_spacing.x, textbox->char_spacing.y);
    iprintf("        box:               %6.2f x %6.2f\n", textbox->box.x, textbox->box.y);
}

static void xCMdump_texture(xCMtexture* tex)
{
    iprintf("tex assetID:               %08X\n", tex->assetID);
    iprintf("    color:                 %08X\n", tex->color);
    iprintf("    x,y,w,h:               %6.2f %6.2f %6.2f %6.2f\n", tex->x, tex->y, tex->w, tex->h);
    iprintf("    texture:               %08X\n", tex->texture);
    iprintf("    pad:                   %08X\n", tex->pad);
}

static const char* preset_names[] = {
    "CM_ALIGN_CENTER",
    "CM_ALIGN_LEFT",
    "CM_ALIGN_RIGHT",
    "CM_ALIGN_INNER",
    "CM_ALIGN_TEXTURE"
};

static void xCMdump_preset(xCMpreset* pp)
{
    iprintf("preset num:                %d\n", pp->num);
    iprintf("      align:               %s\n", preset_names[pp->align]);
    iprintf("      delay:               %6.2f sec\n", pp->delay);
    iprintf("      innerspace:          %6.2f\n", pp->innerspace);
    if (pp->align == CM_ALIGN_TEXTURE) {
        xCMdump_texture((xCMtexture*)&pp->box[0]);
    } else if (pp->align == CM_ALIGN_CENTER) {
        xCMdump_textbox(&pp->box[0]);
    } else {
        xCMdump_textbox(&pp->box[0]);
        xCMdump_textbox(&pp->box[1]);
    }
}

static void xCMdump_hunk(xCMhunk* hunk)
{
    iprintf("hunk hunk_size:            %d bytes\n", hunk->hunk_size);
    iprintf("     preset idx:           %d\n", hunk->preset);
    iprintf("     t0,t1:                %6.2f - %6.2f\n", hunk->t0, hunk->t1);
    if (hunk->text1) {
        iprintf("     text1:                [%s]\n", hunk->text1);
    }
    if (hunk->text2) {
        iprintf("     text2:                [%s]\n", hunk->text2);
    }
}

static U32 xCMrender(F32 time, xCreditsData* data)
{
    // Fakematch: different declaration order than DWARF
    xCMhunk* hp;
    xCMheader* hdr;
    char* dp;
    xCMcredits* cp;
    xCMpreset* pp;

    hdr = (xCMheader*)data;

    if (!hdr || hdr->magic != 0xBEEEEEEF) {
        return 0;
    }

    if ((hdr->state & 0x1) == 1) {
        xCMprep(data);
    }

    xprintf("credits time %6.2f\n", time);

#if DEBUG || RELEASE
    if (dumpit) {
        xCMdump_header(hdr);
    }
#endif

    dp = (char*)(hdr + 1);
    while (dp - (char*)data < hdr->total_size) {
        cp = (xCMcredits*)dp;

#if DEBUG || RELEASE
        if (dumpit) {
            xCMdump_credits(cp);
        }
#endif

        pp = (xCMpreset*)(cp + 1);

#if DEBUG || RELEASE
        if (dumpit) {
            for (U32 i = 0; i < cp->num_presets; i++) {
                xCMdump_preset(&pp[i]);
            }
        }
#endif

        hp = (xCMhunk*)(pp + cp->num_presets);
        while ((char*)hp - (char*)cp < cp->credits_size) {
#if DEBUG || RELEASE
            if (dumpit) {
                xCMdump_hunk(hp);
            }
#endif

            if (time >= hp->t0 && time <= hp->t1) {
                xASSERT(1107, hp->t1 - hp->t0 > EPSILON);
                F32 a = (time - hp->t0) / (hp->t1 - hp->t0);

                xASSERT(1111, hp->preset < cp->num_presets);
                xCMpreset* preset = &pp[hp->preset];

                xASSERT(1113, preset->align < CM_ALIGN_MAX);

                F32 nx, ny;
                ny = a * (cp->out.y - cp->in.y) + cp->in.y;

                xASSERT(1124, cp->fin.end - cp->fin.start > EPSILON);
                xASSERT(1125, cp->fout.end - cp->fout.start > EPSILON);

                F32 x0, x1, y0, y1, ca;
                if (a < cp->fin.start || a > cp->fout.end)
                    ca = 0.0f;
                else if (a >= cp->fin.start && a <= cp->fin.end)
                    ca = (a - cp->fin.start) / (cp->fin.end - cp->fin.start);
                else if (a >= cp->fout.start && a <= cp->fout.end)
                    ca = (cp->fout.end - a) / (cp->fout.end - cp->fout.start);
                else
                    ca = 1.0f;

                switch (preset->align) {
                case CM_ALIGN_TEXTURE: {
                    xCMtexture* tex = (xCMtexture*)&preset->box[0];
                    if (!tex->texture) {
                        tex->texture = (RwTexture*)xSTFindAsset(tex->assetID, NULL);
                    }
                    if (tex->texture) {
                        RwRaster* raster = RwTextureGetRaster(tex->texture);
                        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)raster);
                    }

                    nx = tex->x;
                    x0 = nx;
                    y0 = ny;
                    x1 = nx + tex->w;
                    y1 = ny + tex->h;
                    xCMScaleBounds(x0, y0, x1, y1);
                    x0 *= FB_XRES;
                    y0 *= FB_YRES;
                    x1 *= FB_XRES;
                    y1 *= FB_YRES;

                    RwBlendFunction oldSrcBlend, oldDestBlend;
                    RwRenderStateGet(rwRENDERSTATESRCBLEND, (void*)&oldSrcBlend);
                    RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void*)&oldDestBlend);
                    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
                    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

                    Im2DRenderQuadAlpha(x0, y0, x1, y1, ca, 0.0f, 1000000.0f, 0.0f);

                    nx = 0.5f * (1.0f - preset->box[1].box.x);
                    x0 = nx * FB_XRES;
                    y0 = ny * FB_YRES;
                    x1 = (nx + preset->box[1].box.x) * FB_XRES;
                    y1 = (ny + preset->box[1].box.y) * FB_YRES;

                    xColor color = xCMcolor_scale(preset->box[1].color, ca);

                    basic_rect<F32> bounds = { nx, ny, preset->box[1].box.x, preset->box[1].box.y };
                    xCMScaleBounds(bounds);

                    xtextbox tb = xtextbox::create(
                        xfont::create(
                            preset->box[1].font,
                            destWidth * NSCREENX(preset->box[1].char_size.x),
                            destHeight * NSCREENY(preset->box[1].char_size.y),
                            0.0f,
                            color),
                        bounds);
                    
                    tb.font.shadowColor.a = color.a;
                    tb.set_text(hp->text2);
                    tb.render(true);

                    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)oldSrcBlend);
                    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)oldDestBlend);
                    break;
                }
                case CM_ALIGN_CENTER: {
                    nx = 0.5f * (1.0f - preset->box[0].box.x);
                    x0 = nx * FB_XRES;
                    y0 = ny * FB_YRES;
                    x1 = (nx + preset->box[0].box.x) * FB_XRES;
                    y1 = (ny + preset->box[0].box.y) * FB_YRES;

                    xColor color = xCMcolor_scale(preset->box[0].color, ca);

                    basic_rect<F32> bounds = { nx, ny, preset->box[0].box.x, preset->box[0].box.y };
                    xCMScaleBounds(bounds);

                    xtextbox tb = xtextbox::create(
                        xfont::create(
                            preset->box[0].font,
                            destWidth * NSCREENX(preset->box[0].char_size.x),
                            destHeight * NSCREENY(preset->box[0].char_size.y),
                            0.0f,
                            color),
                        bounds);
                    
                    tb.font.shadowColor.a = color.a;
                    tb.set_text(hp->text1);
                    tb.render(true);

                    break;
                }
                case CM_ALIGN_LEFT:
                case CM_ALIGN_RIGHT:
                case CM_ALIGN_INNER: {
                    U32 alignL = 0, alignR = 0;
                    if (preset->align == CM_ALIGN_LEFT) {
                        alignL = alignR = 0x0;
                    } else if (preset->align == CM_ALIGN_RIGHT) {
                        alignL = alignR = 0x1;
                    } else {
                        alignL = 0x1;
                        alignR = 0x0;
                    }

                    {
                        xColor colorL = xCMcolor_scale(preset->box[0].color, ca);

                        basic_rect<F32> bounds = { nx, ny, preset->box[0].box.x, preset->box[0].box.y };
                        xCMScaleBounds(bounds);

                        xtextbox tb = xtextbox::create(
                            xfont::create(
                                preset->box[0].font,
                                destWidth * NSCREENX(preset->box[0].char_size.x),
                                destHeight * NSCREENY(preset->box[0].char_size.y),
                                0.0f,
                                colorL),
                            bounds,
                            alignL);
                        
                        tb.font.shadowColor.a = colorL.a;
                        tb.set_text(hp->text1);
                        tb.render(true);
                    }

                    {
                        xColor colorR = xCMcolor_scale(preset->box[1].color, ca);

                        basic_rect<F32> bounds = { nx, ny, preset->box[1].box.x, preset->box[1].box.y };
                        xCMScaleBounds(bounds);

                        xtextbox tb = xtextbox::create(
                            xfont::create(
                                preset->box[0].font,
                                destWidth * NSCREENX(preset->box[1].char_size.x),
                                destHeight * NSCREENY(preset->box[1].char_size.y),
                                0.0f,
                                colorR),
                            bounds,
                            alignR);
                        
                        tb.font.shadowColor.a = colorR.a;
                        tb.set_text(hp->text2);
                        tb.render(true);
                    }

                    break;
                }
                }
            }

            hp = (xCMhunk*)((char*)hp + hp->hunk_size);
        }

        dp = (char*)hp;
    }

#if DEBUG || RELEASE
    dumpit = 0;
#endif

    return time >= 0.0f && time <= hdr->total_time;
}

F32 dtscale = 1.0f;

static F32 credits_time = 10000.0f;
static xCreditsData* credits_data = NULL;
static U32 credits_parentID = 0;

void xCMupdate(F32 dt)
{
    if (!credits_data) {
        return;
    }

    F32 scale = dtscale;

    if (zGameIsPaused() || zMenuRunning()) {
        if (xglobals->pad0->analog1.y < 0)
            scale *= (xglobals->pad0->analog1.y + 128) / 128.0f;
        else
            scale *= 2.0f * xglobals->pad0->analog1.y / 128.0f + 1.0f;
    }

    credits_time += dt * scale;
    
    xCMheader* hdr = (xCMheader*)credits_data;
    if (credits_time >= hdr->total_time) {
        xCMstop();
    }
}

void xCMrender()
{
    xCMrender(credits_time, credits_data);
}

void xCMstart(xCreditsData* data, F32, xBase* parent)
{
    credits_data = data;

    if (!credits_data) {
        return;
    }

    xCMheader* hdr = (xCMheader*)credits_data;
    if (hdr->state & 0x2) {
        Decrypt(hdr + 1, hdr->total_size - sizeof(xCMheader), KEY, 12);
        hdr->state &= ~0x2;
    }

    credits_time = 0.0f;

    if (parent) {
        credits_parentID = parent->id;
    }
}

void xCMstop()
{
    if (!credits_data) {
        return;
    }

    credits_time = 10000.0f;
    credits_data = NULL;

    if (credits_parentID) {
        zEntEvent(credits_parentID, eEventCreditsEnded);
    }
}

void xCMsetDest(F32 x, F32 y, F32 width, F32 height)
{
    destX = x;
    destY = y;
    destWidth = width;
    destHeight = height;
}

bool xCMisRunning()
{
    return credits_data != NULL;
}

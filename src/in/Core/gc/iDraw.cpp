#include "xCommon.h"
#include "iDraw.h"

#include "xDebugTweak.h"
#include "xMath.h"
#include "zMain.h"

U32 lastSetFBMSK;

void iDrawSetFBMSK(U32 abgr, U32 setMask, bool)
{
    abgr = (abgr & setMask) | (lastSetFBMSK & ~setMask);
    lastSetFBMSK = abgr;
    if (setMask & 0xFF000000) {
        U8 alpha = abgr >> 24;
        if (alpha == 0)
            GXSetAlphaUpdate(GX_TRUE);
        else if (alpha == 0xFF)
            GXSetAlphaUpdate(GX_FALSE);
    }

    if (setMask & 0xFFFFFF) {
        U32 color = abgr & 0xFFFFFF;
        if (color == 0)
            GXSetColorUpdate(GX_TRUE);
        else
            GXSetColorUpdate(GX_FALSE);
    }
}

xTWEAKABLE F32 PAL_OFFX = 40.0f;
xTWEAKABLE F32 PAL_OFFY = 23.0f;

void iDrawSetDisplayOffset(F32 x, F32 y)
{
    if (zMainIsPAL()) {
        F32 nPosX = x * FB_XRES + PAL_OFFX;
        F32 nPosY = y * FB_YRES + PAL_OFFY;

        nPosX = CLAMP(nPosX, 0.0f, 80.0f);
        nPosY = CLAMP(nPosY, 0.0f, 551.0f);

        _RwDlRenderMode->viXOrigin = nPosX;
        _RwDlRenderMode->viYOrigin = nPosY;

        VIConfigure(_RwDlRenderMode);
    }
}

void iDrawBegin() {}
void iDrawEnd() {}

struct GCmemXFB
{
    U8* base;
    U8* addr;
    U8* max;
};

static GCmemXFB xfb;
static void* xfbDisp;
static void* xfbCopy;

void iXFBinit()
{
    RwGameCubeGetXFBs(&xfbDisp, &xfbCopy);
}

void iXFBreset()
{
    iXFBinit();

    xfb.base = (U8*)xfbCopy;

    xfb.max = xfb.base + FB_XRES * FB_YRES * 2;
    xfb.base = (U8*)OSRoundUp32B(xfb.base);
    xfb.addr = xfb.base;
}

void* iXFBalloc(U32 size)
{
    size = OSRoundUp32B(size);
    if (xfb.addr + size > xfb.max)
        return (void*)0xBEEEEEEF;
    U8* addr = xfb.addr;
    xfb.addr += size;
    return addr;
}

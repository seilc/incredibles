#include "water/texcache.h"

#include "types.h"

#define NUM_STANDARD_REGIONS 8
#define NUM_MIPMAP32_REGIONS 2
#define NUM_CI_REGIONS 2
#define NUM_TLUT_REGIONS 8

static GXTexRegion StandardRegions[NUM_STANDARD_REGIONS];
static GXTexRegion Mipmap32Regions[NUM_MIPMAP32_REGIONS];
static GXTexRegion CIRegions[NUM_CI_REGIONS];
static GXTlutRegion TlutRegions[NUM_TLUT_REGIONS];

static GXTexRegion* TexRegionCallback(const GXTexObj* t_obj, GXTexMapID)
{
    GXTexRegion* texRegion = NULL;
    static U32 count = 0;
    static U32 countCI = 0;
    static U32 count32B = 0;

    GXTexFmt fmt = GXGetTexObjFmt(t_obj);

    if (fmt == GX_TF_RGBA8 && GXGetTexObjMipMap(t_obj)) {
        texRegion = &Mipmap32Regions[count32B++ % NUM_MIPMAP32_REGIONS];
    } else {
        if (fmt != GX_TF_C4 && fmt != GX_TF_C8 && fmt != GX_TF_C14X2) {
            texRegion = &StandardRegions[count++ % NUM_STANDARD_REGIONS];
        } else {
            texRegion = &CIRegions[countCI++ % NUM_CI_REGIONS];
        }
    }

    return texRegion;
}

static GXTlutRegion* TlutRegionCallback(u32 tlut_name)
{
    return &TlutRegions[tlut_name];
}

void TexCacheInit()
{
    S32 i;
    U32 lowOffset = 0, highOffset = 0x80000;

    GXSetTexRegionCallback((GXTexRegionCallback)TexRegionCallback);
    GXSetTlutRegionCallback(TlutRegionCallback);

    for (i = 0; i < NUM_STANDARD_REGIONS; i++) {
        GXInitTexCacheRegion(&StandardRegions[i], GX_FALSE, lowOffset, GX_TEXCACHE_32K, highOffset, GX_TEXCACHE_32K);
        lowOffset += 0x8000;
        highOffset += 0x8000;
    }

    for (i = 0; i < NUM_MIPMAP32_REGIONS; i++) {
        GXInitTexCacheRegion(&Mipmap32Regions[i], GX_TRUE, lowOffset, GX_TEXCACHE_32K, highOffset, GX_TEXCACHE_32K);
        lowOffset += 0x10000;
        highOffset += 0x10000;
    }

    for (i = 0; i < NUM_CI_REGIONS; i++) {
        GXInitTexCacheRegion(&CIRegions[i], GX_FALSE, lowOffset, GX_TEXCACHE_32K, lowOffset + 0x8000, GX_TEXCACHE_32K);
        lowOffset += 0x10000;
    }

    for (i = 0; i < NUM_TLUT_REGIONS; i++) {
        GXInitTlutRegion(&TlutRegions[i], highOffset, GX_TLUT_256);
        highOffset += 0x2000;
    }
}

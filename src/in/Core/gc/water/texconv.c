#include "xCommon.h"
#include "water/texconv.h"

#include <rwcore.h>

static RwImage* ImageForwardDiff(RwImage* srcImage, F32 range_scale)
{
    RwImage* dstImage;
    RwRGBA* dst;
    RwUInt8* src;
    RwUInt32 width, height, depth;
    U32 iu, iv;
    S32 vStep, uStep, uWrap, vWrap, peak, scale, diff;

    width = RwImageGetWidth(srcImage);
    height = RwImageGetHeight(srcImage);
    depth = RwImageGetDepth(srcImage);

    switch (depth) {
    case 32:
        vStep = width * 4; uStep = 4;
        break;
    case 8:
        vStep = width; uStep = 1;
        break;
    case 4:
    default:
        return NULL;
    }

    uWrap = uStep * (width - 1);
    vWrap = vStep * (height - 1);

    dstImage = RwImageCreate(width, height, 32);
    RwImageAllocatePixels(dstImage);

    peak = 127;
    src = RwImageGetPixels(srcImage) + uStep - 1;
    dst = (RwRGBA*)RwImageGetPixels(dstImage);
    scale = range_scale * (peak ? 0x7F00 / peak : 0);

    for (iv = 0; iv < width; iv++) {
        for (iu = 0; iu < height; iu++) {
            diff = (iu ? *(src - uStep) : src[uWrap]) - *src;
            dst->alpha = (diff * scale + 0x8000) >> 8;
            diff = (iv ? *(src - vStep) : src[vWrap]) - *src;
            dst->blue = (diff * scale + 0x8000) >> 8;
            dst->red = dst->green = *src;
            src += uStep; dst++;
        }
    }

    return dstImage;
}

RwTexture* TextureDiffBumpRGBA8(RwTexture* tex, F32 range_scale)
{
    RwImage* bumpImage;
    RwImage* diffImage;
    S32 i;

    bumpImage = RwImageCreate(tex->raster->width, tex->raster->height, 32);
    RwImageAllocatePixels(bumpImage);
    RwImageSetFromRaster(bumpImage, tex->raster);
    for (i = 0; i < tex->raster->width * tex->raster->height; i++) {
        bumpImage->cpPixels[i * 4 + 3] = bumpImage->cpPixels[i * 4];
    }

    diffImage = ImageForwardDiff(bumpImage, range_scale);
    RwImageDestroy(bumpImage);
    if (!diffImage) {
        return NULL;
    }

    RwRasterSetFromImage(tex->raster, diffImage);
    RwImageDestroy(diffImage);
    return tex;
}

RwTexture* TextureDiffBumpRGBA8(RwTexture* tex, RwTexture* bump)
{
    RwImage* bumpImage;
    RwImage* diffImage;
    RwRaster* raster;
    RwUInt32 width, height, flags;
    S32 i;

    if (tex->raster->depth < 8) {
        return NULL;
    }

    bumpImage = RwImageCreate(tex->raster->width, tex->raster->height, 32);
    RwImageAllocatePixels(bumpImage);
    RwImageSetFromRaster(bumpImage, tex->raster);
    for (i = 0; i < tex->raster->width * tex->raster->height; i++) {
        bumpImage->cpPixels[i * 4 + 3] = bumpImage->cpPixels[i * 4];
    }

    diffImage = ImageForwardDiff(bumpImage, 1.0f);
    RwImageDestroy(bumpImage);
    if (!diffImage) {
        return NULL;
    }

    RwRasterDestroy(bump->raster);
    width = RwImageGetWidth(diffImage);
    height = RwImageGetHeight(diffImage);
    flags = rwRASTERTYPETEXTURE | rwRASTERFORMAT8888 | rwRASTERFORMATAUTOMIPMAP | rwRASTERFORMATMIPMAP;

    raster = RwRasterCreate(width, height, 32, flags);
    if (!raster) {
        RwImageDestroy(diffImage);
        return NULL;
    }

    RwRasterSetFromImage(raster, diffImage);
    RwImageDestroy(diffImage);
    bump->raster = raster;
    return tex;
}

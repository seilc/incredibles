#ifndef TEXCONV_H
#define TEXCONV_H

#include "types.h"

#include <rwcore.h>

RwTexture* TextureDiffBumpRGBA8(RwTexture* tex, F32 range_scale);
RwTexture* TextureDiffBumpRGBA8(RwTexture* tex, RwTexture* bump);

#endif

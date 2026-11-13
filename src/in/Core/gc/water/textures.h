#ifndef TEXTURES_H
#define TEXTURES_H

#include "types.h"

#include <rwcore.h>

extern RwTexture* iWaterBumpTex;
extern RwTexture* iWaterEnvTex;

S32 TexturesOpen();
void TexturesClose();

#endif

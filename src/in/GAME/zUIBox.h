#ifndef ZUIBOX_H
#define ZUIBOX_H

#include "zUI.h"
#include "xTextureManager.h"

class zUIBox : public zUI
{
private:
    xTextureHandle textures[9];
    F32 curRotation;
};

void zUIBox_Init(xBase& data, xDynAsset& asset, size_t);

#endif
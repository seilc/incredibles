#ifndef ZUIIMAGE_H
#define ZUIIMAGE_H

#include "zUI.h"
#include "xTextureManager.h"

class zUIImage : public zUI
{
private:
    struct ImageState
    {
        F32 rotation;
        F32 offsetU;
        F32 offsetV;
    };

    ImageState currentImage;
    ImageState startMovementImage;
    xTextureHandle texture;
    U32 unk0x94;
};

void zUIImage_Init(xBase& data, xDynAsset& asset, size_t);

#endif
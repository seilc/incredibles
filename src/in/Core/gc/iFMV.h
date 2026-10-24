#ifndef IFMV_H
#define IFMV_H

#include "xFile.h"
#include "xpkrsvc.h"

#include <rwcore.h>
#include <bink.h>

struct MOVIE
{
#if 0
    HBINK hBink;
    RwRaster* pBinkRaster;
    S32 asyncReadKey;
    RwTexture* pBinkTexture;
    U8* pPreBuffer;
    U8* pPreBufferAligned;
    U32 PreBufferSize;
    S32 FileOffset;
    xFile* pFile;
#else
    char unk0x00[0x04];
    HBINK hBink;
    char unk0x08[0x3C];
    RwTexture* pBinkTexture;
    char unk0x48[0x08];
#endif
};

MOVIE* iFMVOpen(PKRAssetTOCInfo& rAssetInfo);
void iFMVClose(MOVIE*& pMovie);
U32 iFMVPlay(const char* filename, U32 buttons, F32 time, U32 uSubtitlesAID,
                bool skippable, bool lockController, bool r8);

#endif
#ifndef XSUBTITLES_H
#define XSUBTITLES_H

#include "xBase.h"

struct xSubtitleLine
{
    F32 fStartTime;
    F32 fStopTime;
    U32 uStringOffset;
};

struct xSubtitlesAsset : xBaseAsset
{
    U16 m_uAssetNumLines;
    U16 m_uAssetByteCount;
    xSubtitleLine m_paSubtitleLines[0];
};

void xSubTitlesInit();
void* xSubTitlesRead(void*, U32, void* indata, U32 insize, U32* outsize);
void xSubTitlesUnloaded(void*, U32);
bool xSubTitlesRender(U32 aid, F32 fTime);
void xSubTitlesSetSize(F32 size);
F32 xSubTitlesGetSize();

#endif
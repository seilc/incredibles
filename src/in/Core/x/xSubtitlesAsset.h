#ifndef XSUBTITLESASSET_H
#define XSUBTITLESASSET_H

#include "xBaseAsset.h"

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
    xSubtitleLine m_paSubtitleLines[];
};

#endif

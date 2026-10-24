#ifndef IFMV_H
#define IFMV_H

#include "types.h"
#include "xpkrsvc.h"
#include <rwcore.h>
#include <dolphin.h>

struct BINK;

struct MOVIE
{
    U32 aid;
    BINK* hBink;
    DVDFileInfo file;
    RwTexture* pBinkTexture;
    U32 uMovieOffsetBytes;
    U32 movie_is_suspended : 1;
    U32 other_movie_flags : 7;
    U32 disk_open_frame : 24;
};

MOVIE* iFMVOpen(PKRAssetTOCInfo& rAssetInfo);
void iFMVClose(MOVIE*& pMovie);
U32 iFMVPlay(const char* filename, U32 buttons, F32 time, U32 uSubtitlesAID,
                bool skippable, bool lockController, bool r8);

#endif

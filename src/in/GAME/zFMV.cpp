#include "xCommon.h"
#include "zFMV.h"

#include "zHashes.h"
#include "zMain.h"
#include "zMenu.h"
#include "xSubTitles.h"
#include "xSndMgr.h"
#include "iFMV.h"
#include "xPad.h"
#include "xutil.h"

// clang-format off
static zFMVFile zFMVFileTable[] =
{
    eFMVFile_NoMovie, "", 0,
    eFMVFile_PixarLOGO, "FMV\\Pixar", 0,
    eFMVFile_DisneyLOGO, "FMV\\Disney", 0,
    eFMVFile_NickLOGO, "FMV\\Nicklogo", 0,
    eFMVFile_THQLOGO, "FMV\\THQLogo", 0,
    eFMVFile_HeavyIronLOGO, "FMV\\HILogo", 0,
    eFMVFile_AttractMode, "FMV\\Attract", 0,
    eFMVFile_FMVOM01, "FMV\\FMVOM01", HASH_FMVOM01_subtitle,
    eFMVFile_FMVOM03, "FMV\\FMVOM03", HASH_FMVOM03_subtitle,
    eFMVFile_FMVOM03a, "FMV\\FMVOM03a", HASH_FMVOM03a_subtitle,
    eFMVFile_FMVLD01, "FMV\\FMVLD01", HASH_FMVLD01_subtitle,
    eFMVFile_FMVLD01a, "FMV\\FMVLD01a", HASH_FMVLD01a_subtitle,
    eFMVFile_FMVNJO2, "FMV\\FMVNJ02", HASH_FMVNJ02_subtitle,
    eFMVFile_FMVNJ03, "FMV\\FMVNJ03", HASH_FMVNJ03_subtitle,
    eFMVFile_FMVNI01, "FMV\\FMVNI01", HASH_FMVNI01_subtitle,
    eFMVFile_FMVNI01a, "FMV\\FMVNI01a", HASH_FMVNI01a_subtitle,
    eFMVFile_FMVCI03, "FMV\\FMVCI03", HASH_FMVCI03_subtitle,
    eFMVFile_FMVCI01a, "FMV\\FMVCI01a", HASH_FMVCI01a_subtitle,
    eFMVFile_FMVFT01, "FMV\\FMVFT01", HASH_FMVFT01_subtitle,
    eFMVFile_FMVFT01a, "FMV\\FMVFT01a", HASH_FMVFT01a_subtitle,
    eFMVFile_FMVFT02, "FMV\\FMVFT02", HASH_FMVFT02_subtitle,
    eFMVFile_FMVFT02a, "FMV\\FMVFT02a", HASH_FMVFT02a_subtitle,
    eFMVFile_FMVFT03a, "FMV\\FMVFT03a", HASH_FMVFT03a_subtitle,
    eFMVFile_FMVFT04, "FMV\\FMVFT04", HASH_FMVFT04_subtitle,
    eFMVFile_FMVFT04a, "FMV\\FMVFT04a", HASH_FMVFT04a_subtitle,
    eFMVFile_FMVRS01, "FMV\\FMVRS01", HASH_FMVRS01_subtitle,
    eFMVFile_FMVRS01a, "FMV\\FMVRS01a", HASH_FMVRS01a_subtitle,
    eFMVFile_FMVRS02a, "FMV\\FMVRS02a", HASH_FMVRS02a_subtitle,
    eFMVFile_FMVHS01a, "FMV\\FMVHS01a", HASH_FMVHS01a_subtitle,
    eFMVFile_FMVHS01b, "FMV\\FMVHS01b", HASH_FMVHS01b_subtitle,
    eFMVFile_FMVNI01b, "FMV\\FMVNI01b", HASH_FMVNI01b_subtitle,
    eFMVFile_FMVNI03a, "FMV\\FMVNI03a", HASH_FMVNI03a_subtitle,
    eFMVFile_FMVHS01, "FMV\\FMVHS01", HASH_FMVHS01_subtitle,
    eFMVFile_FMVNJ01b, "FMV\\FMVNJ01b", HASH_FMVNJ01b_subtitle,
    eFMVFile_FMVNJ01, "FMV\\FMVNJ01", HASH_FMVNJ01_subtitle,
    eFMVFile_D3, "FMV\\D3", 0,
    eFMVFile_PROMO1, "FMV\\PROMO1", HASH_PROMO1_subtitle,
    eFMVFile_PROMO2, "FMV\\PROMO2", HASH_PROMO2_subtitle
};
// clang-format on

zFMVFile* zFMVFileGetFile(eFMVFile fmvId)
{
    for (S32 i = 0; i < ARRAY_LENGTH(zFMVFileTable); i++) {
        if (fmvId == zFMVFileTable[i].fmvCode) {
            return &zFMVFileTable[i];
        }
    }

    return NULL;
}

const char* zFMVFileGetName(eFMVFile fmvId)
{
    zFMVFile* file = zFMVFileGetFile(fmvId);

    if (file == NULL) { // If I do (!file) here the branches get flipped...
        return NULL;
    }

    return file->fileName;
}

U32 zFMVPlay(const char* name, U32 buttons, F32 time, U32 uSubtitlesAID,
                bool skippable, bool lockController, bool displayDemoBlink)
{
    U32 retval = 0;

    if (!globals.noMovies) {
        zMenuPause(true);

        F32 oldSize = xSubTitlesGetSize();

        xSubTitlesSetSize(0.18f);

        xSndEffect currentEffect = xSndMgrGetEffect();

        xSndMgrSetEffect(xSndEffect_NONE);

        retval = iFMVPlay(name, buttons, time, uSubtitlesAID, skippable, lockController, displayDemoBlink);

        xSndMgrSetEffect(currentEffect);
        xSubTitlesSetSize(oldSize);
        xPadUpdate(0, 0.0001f);
        xPadUpdate(0, 0.0001f);
        zMenuPause(false);
    }

    return retval;
}
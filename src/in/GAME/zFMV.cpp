#include "zFMV.h"

#include "zMain.h"
#include "zMenu.h"
#include "xSubTitles.h"
#include "xSndMgr.h"
#include "iFMV.h"
#include "xPad.h"
#include "xutil.h"

#define FMVOM01_SUBTITLE_ID 0xC6472871 // xStrHash("FMVOM01_subtitle")
#define FMVOM03_SUBTITLE_ID 0x6DF46B37 // xStrHash("FMVOM03_subtitle")
#define FMVOM03A_SUBTITLE_ID 0x288395FA // xStrHash("FMVOM03a_subtitle")
#define FMVLD01_SUBTITLE_ID 0x6F4C6D4B // xStrHash("FMVLD01_subtitle")
#define FMVLD01A_SUBTITLE_ID 0xD88CA636 // xStrHash("FMVLD01a_subtitle")
#define FMVNJ02_SUBTITLE_ID 0x7D1F8B72 // xStrHash("FMVNJ02_subtitle")
#define FMVNJ03_SUBTITLE_ID 0x50F62CD5 // xStrHash("FMVNJ03_subtitle")
#define FMVNI01_SUBTITLE_ID 0x0A805494 // xStrHash("FMVNI01_subtitle")
#define FMVNI01A_SUBTITLE_ID 0x441C0091 // xStrHash("FMVNI01a_subtitle")
#define FMVCI03_SUBTITLE_ID 0xEB1C2DFF // xStrHash("FMVCI03_subtitle")
#define FMVCI01A_SUBTITLE_ID 0x66331700 // xStrHash("FMVCI01a_subtitle")
#define FMVFT01_SUBTITLE_ID 0xD7FAD155 // xStrHash("FMVFT01_subtitle")
#define FMVFT01A_SUBTITLE_ID 0x69C9D754 // xStrHash("FMVFT01a_subtitle")
#define FMVFT02_SUBTITLE_ID 0xABD172B8 // xStrHash("FMVFT02_subtitle")
#define FMVFT02A_SUBTITLE_ID 0xD09E6CFD // xStrHash("FMVFT02a_subtitle")
#define FMVFT03A_SUBTITLE_ID 0x377302A6 // xStrHash("FMVFT03a_subtitle")
#define FMVFT04_SUBTITLE_ID 0x537EB57E // xStrHash("FMVFT04_subtitle")
#define FMVFT04A_SUBTITLE_ID 0x9E47984F // xStrHash("FMVFT04a_subtitle")
#define FMVRS01_SUBTITLE_ID 0x40E82326 // xStrHash("FMVRS01_subtitle")
#define FMVRS01A_SUBTITLE_ID 0x1B3AB547 // xStrHash("FMVRS01a_subtitle")
#define FMVRS02A_SUBTITLE_ID 0x820F4AF0 // xStrHash("FMVRS02a_subtitle")
#define FMVHS01A_SUBTITLE_ID 0x517E3E09 // xStrHash("FMVHS01a_subtitle")
#define FMVHS01B_SUBTITLE_ID 0x2554DF6C // xStrHash("FMVHS01b_subtitle")
#define FMVNI01B_SUBTITLE_ID 0x17F2A1F4 // xStrHash("FMVNI01b_subtitle")
#define FMVNI03A_SUBTITLE_ID 0x11C52BE3 // xStrHash("FMVNI03a_subtitle")
#define FMVHS01_SUBTITLE_ID 0xBA7B37BC // xStrHash("FMVHS01_subtitle")
#define FMVNJ01B_SUBTITLE_ID 0x58971FE5 // xStrHash("FMVNJ01b_subtitle")
#define FMVNJ01_SUBTITLE_ID 0xA948EA0F // xStrHash("FMVNJ01_subtitle")
#define PROMO1_SUBTITLE_ID 0xF5B911C5 // xStrHash("PROMO1_subtitle")
#define PROMO2_SUBTITLE_ID 0xC98FB328 // xStrHash("PROMO2_subtitle")

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
    eFMVFile_FMVOM01, "FMV\\FMVOM01", FMVOM01_SUBTITLE_ID,
    eFMVFile_FMVOM03, "FMV\\FMVOM03", FMVOM03_SUBTITLE_ID,
    eFMVFile_FMVOM03a, "FMV\\FMVOM03a", FMVOM03A_SUBTITLE_ID,
    eFMVFile_FMVLD01, "FMV\\FMVLD01", FMVLD01_SUBTITLE_ID,
    eFMVFile_FMVLD01a, "FMV\\FMVLD01a", FMVLD01A_SUBTITLE_ID,
    eFMVFile_FMVNJO2, "FMV\\FMVNJ02", FMVNJ02_SUBTITLE_ID,
    eFMVFile_FMVNJ03, "FMV\\FMVNJ03", FMVNJ03_SUBTITLE_ID,
    eFMVFile_FMVNI01, "FMV\\FMVNI01", FMVNI01_SUBTITLE_ID,
    eFMVFile_FMVNI01a, "FMV\\FMVNI01a", FMVNI01A_SUBTITLE_ID,
    eFMVFile_FMVCI03, "FMV\\FMVCI03", FMVCI03_SUBTITLE_ID,
    eFMVFile_FMVCI01a, "FMV\\FMVCI01a", FMVCI01A_SUBTITLE_ID,
    eFMVFile_FMVFT01, "FMV\\FMVFT01", FMVFT01_SUBTITLE_ID,
    eFMVFile_FMVFT01a, "FMV\\FMVFT01a", FMVFT01A_SUBTITLE_ID,
    eFMVFile_FMVFT02, "FMV\\FMVFT02", FMVFT02_SUBTITLE_ID,
    eFMVFile_FMVFT02a, "FMV\\FMVFT02a", FMVFT02A_SUBTITLE_ID,
    eFMVFile_FMVFT03a, "FMV\\FMVFT03a", FMVFT03A_SUBTITLE_ID,
    eFMVFile_FMVFT04, "FMV\\FMVFT04", FMVFT04_SUBTITLE_ID,
    eFMVFile_FMVFT04a, "FMV\\FMVFT04a", FMVFT04A_SUBTITLE_ID,
    eFMVFile_FMVRS01, "FMV\\FMVRS01", FMVRS01_SUBTITLE_ID,
    eFMVFile_FMVRS01a, "FMV\\FMVRS01a", FMVRS01A_SUBTITLE_ID,
    eFMVFile_FMVRS02a, "FMV\\FMVRS02a", FMVRS02A_SUBTITLE_ID,
    eFMVFile_FMVHS01a, "FMV\\FMVHS01a", FMVHS01A_SUBTITLE_ID,
    eFMVFile_FMVHS01b, "FMV\\FMVHS01b", FMVHS01B_SUBTITLE_ID,
    eFMVFile_FMVNI01b, "FMV\\FMVNI01b", FMVNI01B_SUBTITLE_ID,
    eFMVFile_FMVNI03a, "FMV\\FMVNI03a", FMVNI03A_SUBTITLE_ID,
    eFMVFile_FMVHS01, "FMV\\FMVHS01", FMVHS01_SUBTITLE_ID,
    eFMVFile_FMVNJ01b, "FMV\\FMVNJ01b", FMVNJ01B_SUBTITLE_ID,
    eFMVFile_FMVNJ01, "FMV\\FMVNJ01", FMVNJ01_SUBTITLE_ID,
    eFMVFile_D3, "FMV\\D3", 0,
    eFMVFile_PROMO1, "FMV\\PROMO1", PROMO1_SUBTITLE_ID,
    eFMVFile_PROMO2, "FMV\\PROMO2", PROMO2_SUBTITLE_ID
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
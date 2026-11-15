#include "xCommon.h"
#include "iTRC.h"

#include "xPad.h"
#include "xstransvc.h"
#include "iSystem.h"
#include "zMain.h"

#include <rwcore.h>
#include <dolphin.h>
#include <fmodgc.h>
#include <string.h>

class ROMFont
{
public:
    static void Init();
    static void InitFont();
    static void DestroyFont();
    static void InitDisplay(GXRenderModeObj* mode);
    static void DestroyDisplay();
    static void InitGX();
    static void InitVI();
    static void RenderBegin();
    static void RenderEnd();
    static void SwapBuffers();
    static void DrawCell(S32 x, S32 y);
    static void LoadTex(void* image);
    static s32 DrawString(S32 x, S32 y, char* string);
    static s32 GetWidth(char* string);
    static void DrawTextBox(S32 x, S32 y, S32 width, S32 height, char* string);
    static void DrawBlackScreen();

    static void* mXFBs[2];
    static bool mFontMalloced;
    static OSFontHeader* mFontData;
    static bool mFirstFrame;
    static void* mCurrentFrameBuffer;
    static GXRenderModeObj* mRenderMode;
    static F32 mRomFontScale;
};

bool ROMFont::mFirstFrame = true;
OSFontHeader* ROMFont::mFontData;
void* ROMFont::mXFBs[2];
void* ROMFont::mCurrentFrameBuffer;
GXRenderModeObj* ROMFont::mRenderMode;
bool ROMFont::mFontMalloced;
F32 ROMFont::mRomFontScale = 1.0f;

static char textureBuffer[288] ATTRIBUTE_ALIGN(32);

char* diskNotIdentifiedMessage[eRegionCount] = {
    "Please wait... Checking for the 'The Incredibles' Game Disc.",
    "",
    "",
    "",
    "Bitte warten... Die Unglaublichen-Disc wird gepr\374ft.",
    "",
    "",
    "",
    "Merci de patienter... Recherche du disque Les Indestructibles en cours.",
    "",
    "\202\250\221\322\202\277\202\255\202\276\202\263\202\242\201B Mr.\203C\203\223\203N\203\214\203f\203B\203u\203\213\202\314\203Q\201[\203\200\203f\203B\203X\203N\202\360 \203`\203F\203b\203N\202\265\202\304\202\242\202\334\202\267\201B",
    "",
    "Even wachten... controleren van de spel-disc met The Incredibles.",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

char* diskCoverOpenMessage[eRegionCount] = {
    "The Disc Cover is open. If you want to continue the game, please close the Disc Cover.",
    "",
    "",
    "",
    "Der Disc-Deckel ist ge\366ffnet. Bitte den Disc-Deckel schlie\337en, um mit dem Spiel fortzufahren.",
    "",
    "La tapa est\341 abierta. Si quieres seguir jugando, debes cerrar la tapa.",
    "",
    "Le couvercle est ouvert. Pour continuer \340 jouer, veuillez fermer le couvercle",
    "Il coperchio del disco \350 aperto. Se vuoi proseguire nel gioco, chiudi il coperchio del disco.",
    "\203f\203B\203X\203N\203J\203o\201[\202\252\212J\202\242\202\304\202\242\202\334\202\267\201B \203Q\201[\203\200\202\360\221\261\202\257\202\351\217\352\215\207\202\315\201A \203f\203B\203X\203N\203J\203o\201[\202\360\225\302\202\337\202\304\202\255\202\276\202\263\202\242\201B",
    "",
    "De disc-deksel is open. Sluit de disc-deksel als je verder wilt spelen.",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

char* diskNoDiskMessage[eRegionCount] = {
    "Please insert the 'The Incredibles' Game Disc.",
    "",
    "",
    "",
    "Bitte legen Sie die Die Unglaublichen-Disc ein.",
    "",
    "Coloca el disco de Los Incre\355bles.",
    "",
    "Veuillez ins\351rer le disque Les Indestructibles.",
    "nserisci il disco di gioco Gli Incredibili.",
    "Mr. \203C\203\223\203N\203\214\203f\203B\203u\203\213\202\314\203f\203B\203X\203N\202\360 \203Z\203b\203g\202\265\202\304\202\255\202\276\202\263\202\242\201B",
    "",
    "Plaats de spel-disc met The Incredibles.",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

char* diskWrongDiskMessage[eRegionCount] = {
    "This is not the 'The Incredibles' Game Disc. Please insert the 'The Incredibles' Game Disc.",
    "",
    "",
    "",
    "Bitte legen Sie die \"Die Unglaublichen - The Incredibles\" Disc ein.",
    "",
    "Coloca el disco de Los Incre\355bles.",
    "",
    "Veuillez ins\351rer le disque Les Indestructibles.",
    "Inserisci il disco di gioco Gli Incredibili.",
    "Mr. \203C\203\223\203N\203\214\203f\203B\203u\203\213\202\314\203f\203B\203X\203N\202\360 \203Z\203b\203g\202\265\202\304\202\255\202\276\202\263\202\242\201B",
    "",
    "Plaats de The Incredibles spel-disc.",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

char* diskRetryMessage[eRegionCount] = {
    "The Game Disc could not be read. Please read the Nintendo GameCube\231 Instruction Booklet for more information.",
    "",
    "",
    "",
    "Diese Game Disc kann nicht gelesen werden. Bitte lesen Sie die Bedienungsanleitung des Nintendo GameCube, um weitere Informationen zu erhalten.",
    "",
    "No se puede leer el disco. Consulta el manual de instrucciones de Nintendo GameCube  para obtener m\341s informaci\363n.",
    "",
    "La lecture du disque a \351chou\351. Veuillez vous r\351f\351rer au manuel d'instructions Nintendo GameCube pour de plus amples informations.",
    "Impossibile leggere il disco di gioco. Per ulteriori indicazioni consulta il manuale di istruzioni del Nintendo GameCube.",
    "\203f\203B\203X\203N\202\360\223\307\202\337\202\334\202\271\202\361\202\305\202\265\202\275\201B \202\255\202\355\202\265\202\255\202\315\201A\226{\221\314\202\314\216\346\210\265\220\340\226\276\217\221\202\360 \202\250\223\307\202\335\202\255\202\276\202\263\202\242\201B",
    "",
    "De spel-disc kon niet worden gelezen. Lees de handleiding van de Nintendo GameCube voor meer informatie.",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

char* diskFatalMessage[eRegionCount] = {
    "An error has occurred. Turn the power off and refer to the Nintendo GameCube\231 Instruction Booklet for further instructions.",
    "",
    "",
    "",
    "Ein Fehler ist aufgetreten. Bitte schalten Sie den Nintendo GameCube aus und lesen Sie die Bedienungsanleitung, um weitere Informationen zu erhalten.",
    "",
    "Se ha producido un error. Apaga la consola y consulta el manual de instrucciones de Nintendo GameCube para obtener m\341s informaci\363n.",
    "",
    "Une erreur est survenue. Eteignez la console et r\351f\351rez-vous au manuel d'instructions Nintendo GameCube pour de plus amples informations.",
    "Si \350 verificato un errore. Spegni e consulta il manuale di istruzioni del Nintendo GameCube per ulteriori indicazioni.",
    "\203G\203\211\201[\202\252\224\255\220\266\202\265\202\334\202\265\202\275\201B \226{\221\314\202\314\203p\203\217\201[\203{\203^\203\223\202\360\211\237\202\265\202\304\223d\214\271\202\360OFF\202\311\202\265\201A \226{\221\314\202\314\216\346\210\265\220\340\226\276\217\221\202\314\216w\216\246\202\311\217]\202\301\202\304\202\255\202\276\202\263\202\242\201B",
    "",
    "Er is een fout opgetreden. Zet de Nintendo GameCube uit en raadpleeg de handleiding van de Nintendo GameCube voor nadere instructies.",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

static xRegion GetRegion()
{
    xRegion region = xSTGetLocalizationEnum();
    if (region == eRegionUK) {
        return eRegionUS;
    } else if (region == eRegionUnknown) {
        U8 lang = OSGetLanguage();
        switch (lang) {
        case OS_LANG_ENGLISH:
            if (OSGetFontEncode() == OS_FONT_ENCODE_SJIS) {
                return eRegionJP;
            } else {
                return eRegionUS;
            }
        case OS_LANG_GERMAN:
            return eRegionDE;
        case OS_LANG_FRENCH:
            return eRegionFR;
        case OS_LANG_SPANISH:
            return eRegionES;
        case OS_LANG_ITALIAN:
            return eRegionIT;
        case OS_LANG_DUTCH:
            return eRegionNL;
        default:
            return eRegionUS;
        }
    } else {
        return region;
    }
}

void ROMFont::Init()
{
    mFontData = NULL;
}

void ROMFont::InitFont()
{
    mFontMalloced = false;
    U32 fontDataSize, fontTempSize;
    iRwResourcesEmptyArena();
    if (OSGetFontEncode() == OS_FONT_ENCODE_SJIS) {
        fontDataSize = OS_FONT_DATA_SIZE_SJIS;
        fontTempSize = OS_FONT_ROM_SIZE_SJIS + 32;
    } else {
        fontDataSize = OS_FONT_DATA_SIZE_ANSI;
        fontTempSize = OS_FONT_ROM_SIZE_ANSI + 32;
    }
    RwResEntry* fontRes = RwResourcesAllocateResEntry(NULL, NULL, fontDataSize, NULL);
    if (!fontRes) {
        mFontData = (OSFontHeader*)RwMalloc(fontDataSize, 0);
        mFontMalloced = true;
    } else {
        mFontData = (OSFontHeader*)(fontRes + 1);
    }
    void* fontTemp;
    fontRes = RwResourcesAllocateResEntry(NULL, NULL, fontTempSize, NULL);
    if (!fontRes) {
        fontTemp = RwMalloc(fontTempSize, 0);
    } else {
        fontTemp = fontRes + 1;
    }
    OSLoadFont(mFontData, (void*)OSRoundUp32B(fontTemp));
    if (fontRes) {
        RwResourcesFreeResEntry(fontRes);
    } else {
        RwFree(fontTemp);
    }
}

void ROMFont::DestroyFont()
{
    if (mFontMalloced) {
        memset(mFontData, 0, OS_FONT_DATA_SIZE_ANSI);
        RwFree(mFontData);
    } else {
        RwResEntry* entry = (RwResEntry*)mFontData;
        RwResourcesFreeResEntry(entry - 1);
    }
    mFontData = NULL;
}

void ROMFont::InitDisplay(GXRenderModeObj* mode)
{
    InitFont();
    Mtx44 projection;
    Mtx modelview;
    GXColor Bkcolor = { 0, 0, 0, 0 };
    mRenderMode = mode;
    RwGameCubeGetXFBs(&mXFBs[0], &mXFBs[1]);
    mCurrentFrameBuffer = mXFBs[1];
    InitGX();
    InitVI();
    GXSetCopyClear(Bkcolor, GX_MAX_Z24);
    MTXOrtho(projection, 0.0f, mRenderMode->efbHeight, 0.0f, mRenderMode->fbWidth, 0.0f, -100.0f);
    GXSetProjection(projection, GX_ORTHOGRAPHIC);
    MTXIdentity(modelview);
    GXLoadPosMtxImm(modelview, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
    GXSetNumChans(0);
    GXSetNumTevStages(1);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, 0);
}

void ROMFont::DestroyDisplay()
{
    DestroyFont();
}

void ROMFont::InitGX()
{
    U16 xfbHeight;
    F32 yScale;

    GXSetViewport(0.0f, 0.0f, mRenderMode->fbWidth, mRenderMode->efbHeight, 0.0f, 1.0f);
    GXSetScissor(0, 0, mRenderMode->fbWidth, mRenderMode->efbHeight);
    yScale = GXGetYScaleFactor(mRenderMode->efbHeight, mRenderMode->xfbHeight);
    xfbHeight = GXSetDispCopyYScale(yScale);
    GXSetDispCopySrc(0, 0, mRenderMode->fbWidth, mRenderMode->efbHeight);
    GXSetDispCopyDst(mRenderMode->fbWidth, xfbHeight);
    GXSetCopyFilter(mRenderMode->aa, mRenderMode->sample_pattern, GX_TRUE, mRenderMode->vfilter);
    if (mRenderMode->aa)
        GXSetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    else
        GXSetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
    GXCopyDisp(mCurrentFrameBuffer, GX_TRUE);
    GXSetDispCopyGamma(GX_GM_1_0);
    GXSetCullMode(GX_CULL_NONE);
}

void ROMFont::InitVI()
{
    VISetNextFrameBuffer(mXFBs[0]);
    mCurrentFrameBuffer = mXFBs[1];
    VIFlush();
    VIWaitForRetrace();
    u32 nin = mRenderMode->viTVmode & 1;
    if (nin) {
        VIWaitForRetrace();
    }
}

void ROMFont::RenderBegin()
{
    if (mRenderMode->field_rendering) {
        GXSetViewportJitter(0.0f, 0.0f, mRenderMode->fbWidth, mRenderMode->efbHeight, 0.0f, 1.0f, VIGetNextField());
    } else {
        GXSetViewport(0.0f, 0.0f, mRenderMode->fbWidth, mRenderMode->efbHeight, 0.0f, 1.0f);
    }
    GXInvalidateVtxCache();
    GXInvalidateTexAll();
}

void ROMFont::RenderEnd()
{
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetColorUpdate(GX_TRUE);
    GXCopyDisp(mCurrentFrameBuffer, GX_TRUE);
    GXDrawDone();
    SwapBuffers();
}

void ROMFont::SwapBuffers()
{
    VISetNextFrameBuffer(mCurrentFrameBuffer);
    if (mFirstFrame) {
        VISetBlack(TRUE);
        mFirstFrame = false;
    } else {
        VISetBlack(FALSE);
    }
    VIFlush();
    VIWaitForRetrace();
    mCurrentFrameBuffer = (mCurrentFrameBuffer == mXFBs[0]) ? mXFBs[1] : mXFBs[0];
}

void ROMFont::DrawCell(S32 x, S32 y)
{
    S16 posLeft = x;
    S16 posRight = posLeft + mFontData->cellWidth;
    S16 posTop = y;
    S16 posBottom = posTop + mFontData->cellHeight;
    S16 texLeft = 0;
    S16 texRight = mFontData->cellWidth;
    S16 texTop = 0;
    S16 texBottom = mFontData->cellHeight;

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3s16(posLeft, posTop, 0);
    GXTexCoord2s16(texLeft, texTop);
    GXPosition3s16(posRight, posTop, 0);
    GXTexCoord2s16(texRight, texTop);
    GXPosition3s16(posRight, posBottom, 0);
    GXTexCoord2s16(texRight, texBottom);
    GXPosition3s16(posLeft, posBottom, 0);
    GXTexCoord2s16(texLeft, texBottom);
    GXEnd();
    GXDrawDone();
}

void ROMFont::LoadTex(void* image)
{
    Mtx mtx;
    GXTexObj texObj;

    GXInitTexObj(&texObj, image, 24, 24, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObjLOD(&texObj, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXInvalidateTexAll();
    GXLoadTexObj(&texObj, GX_TEXMAP0);
    MTXScale(mtx, 1.0f / 24.0f, 1.0f / 24.0f, 1.0f);
    GXLoadTexMtxImm(mtx, GX_TEXMTX0, GX_MTX2x4);
    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
}

s32 ROMFont::DrawString(S32 x, S32 y, char* string)
{
    s32 xLetter = x;
    s32 width;

    while (*string) {
        memset(textureBuffer, 0, sizeof(textureBuffer));
        string = OSGetFontTexel(string, textureBuffer, 0, 6, &width);
        DCStoreRange(textureBuffer, sizeof(textureBuffer));
        width = width * mRomFontScale;
        LoadTex(textureBuffer);
        DrawCell(xLetter, y);
        xLetter += width;
    }
    return xLetter;
}

s32 ROMFont::GetWidth(char* string)
{
    s32 cx = 0;
    s32 width;

    while (*string) {
        string = OSGetFontTexel(string, textureBuffer, 0, 6, &width);
        width = width * mRomFontScale;
        cx += width;
    }
    return cx;
}

void ROMFont::DrawTextBox(S32 x, S32 y, S32 width, S32 height, char* string)
{
    char* word;
    char tempChars[256];
    s32 currentX, currentY, widthOfSpace;
    S32 wordWidth;

    if (!string) {
        return;
    }
    RenderBegin();
    currentX = x;
    currentY = y;
    strcpy(tempChars, string);
    word = strtok(tempChars, " ");
    widthOfSpace = GetWidth(" ");
    while (word && y + height > currentY) {
        wordWidth = GetWidth(word);
        if (currentX + wordWidth > x + width) {
            currentX = x;
            currentY += mFontData->cellHeight;
        }
        DrawString(currentX, currentY, word);
        word = strtok(NULL, " ");
        currentX += wordWidth + widthOfSpace;
    }
    RenderEnd();
}

void ROMFont::DrawBlackScreen()
{
    RenderBegin();
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetColorUpdate(GX_TRUE);
    GXCopyDisp(mCurrentFrameBuffer, GX_TRUE);
    GXDrawDone();
    VISetNextFrameBuffer(mCurrentFrameBuffer);
    VISetBlack(TRUE);
    VIFlush();
    VIWaitForRetrace();
    mCurrentFrameBuffer = (mCurrentFrameBuffer == mXFBs[0]) ? mXFBs[1] : mXFBs[0];
}

S32 ResetButton::mWasResetButtonPressed;
bool ResetButton::mResetEnabled = true;
bool ResetButton::mDiscIded;
void (*ResetButton::mSndKill)();
void (*ResetButton::mMovieKillFunction)();

void ResetButton::EnableReset()
{
    mResetEnabled = true;
}

void ResetButton::DisableReset()
{
    mResetEnabled = false;
}

void ResetButton::SetDiscIded(bool value)
{
    mDiscIded = value;
}

void ResetButton::SetSndKillFunction(ResetButtonCallback sndKillFunction)
{
    mSndKill = sndKillFunction;
}

void ResetButton::SetMovieKillFunction(ResetButtonCallback movieKillFunction)
{
    mMovieKillFunction = movieKillFunction;
}

void ResetButton::CheckResetButton()
{
    S32 currentResetState = OSGetResetButtonState();
    if (!currentResetState && mWasResetButtonPressed) {
        ResetSystem();
    }
    mWasResetButtonPressed |= currentResetState;
}

void ResetButton::ResetSystem()
{
    OSThread* curThread = OSGetCurrentThread();
    if (!mResetEnabled) {
        return;
    }
    S32 dvdStatus = DVDGetDriveStatus();
    bool discIDed = DVDCheckDisk();
    bool notIded = !mDiscIded || !discIDed;
    if (!notIded) {
        if (mMovieKillFunction) {
            mMovieKillFunction();
        }
    }
    OSDisableInterrupts();
    iTRCDisk::Shutdown();
    PADRecalibrate(PAD_CHAN0_BIT | PAD_CHAN1_BIT | PAD_CHAN2_BIT | PAD_CHAN3_BIT);
    VISetBlack(TRUE);
    VIFlush();
    VIWaitForRetrace();
    OSEnableInterrupts();
    if (dvdStatus == DVD_STATE_WRONG_DISK || dvdStatus == DVD_STATE_COVER_OPEN || !mDiscIded || !discIDed) {
        if (GCN_DEMO_PARAM) {
            OSResetSystem(OS_RESET_RESTART, 0, FALSE);
        } else {
            OSResetSystem(OS_RESET_HOTRESET, 0, FALSE);
            return;
        }
    }
    if (mSndKill) {
        mSndKill();
    }
    OSResetSystem(OS_RESET_RESTART, 0, FALSE);
}

char iTRCDisk::mMessage[256];
void (*iTRCDisk::mSndSuspend)();
void (*iTRCDisk::mSndResume)();
void (*iTRCDisk::mAsyncLoadSuspend)();
void (*iTRCDisk::mAsyncLoadResume)();
void (*iTRCDisk::mPadStopRumbling)();
void (*iTRCDisk::mMovieSuspendFunction)();
void (*iTRCDisk::mMovieResumeFunction)();
void (*iTRCDisk::mMovieResetCallBack)();
S32 gWasDiskError;
bool iTRCDisk::mInCriticalSection;
bool iTRCDisk::mFModError;
bool iTRCDisk::mInFModLockingCode;
OSThread* iTRCDisk::mMainThread;
s32 iTRCDisk::mMainPriority;
OSThread* iTRCDisk::mFModThread;
s32 iTRCDisk::mFModPriority;
bool iTRCDisk::mDiskErrorInHigherPriorityThread;

void iTRCDisk::Init()
{
    ROMFont::Init();
    gWasDiskError = FALSE;
    mInCriticalSection = false;
    mFModError = false;
    mInFModLockingCode = false;
    FSOUND_GC_SetDvdStatusCallback(FMODCallBack);
    FSOUND_GC_SetUpdateDvdStatusCallback(FMODUpdateDiskError);
    mMainThread = OSGetCurrentThread();
    mMainPriority = OSGetThreadPriority(mMainThread);
}

void iTRCDisk::SetErrorMessage(const char* errorMessage)
{
    strcpy(mMessage, errorMessage);
}

void iTRCDisk::ResetMessage()
{
    memset(mMessage, 0, sizeof(mMessage));
}

void iTRCDisk::SetPadStopRumblingFunction(iTRCDiskCallback padStopRumbing)
{
    mPadStopRumbling = padStopRumbing;
}

void iTRCDisk::SetSndSuspendFunction(iTRCDiskCallback sndSuspendFunction)
{
    mSndSuspend = sndSuspendFunction;
}

void iTRCDisk::SetSndResumeFunction(iTRCDiskCallback sndResumeFunction)
{
    mSndResume = sndResumeFunction;
}

void iTRCDisk::SetAsyncLoadSuspendFunction(iTRCDiskCallback asyncLoadSuspendFunction)
{
    mAsyncLoadSuspend = asyncLoadSuspendFunction;
}

void iTRCDisk::SetAsyncLoadResumeFunction(iTRCDiskCallback asyncLoadResumeFunction)
{
    mAsyncLoadResume = asyncLoadResumeFunction;
}

void iTRCDisk::SetMovieSuspendFunction(iTRCDiskCallback movieSuspendFunction)
{
    mMovieSuspendFunction = movieSuspendFunction;
}

void iTRCDisk::SetMovieResumeFunction(iTRCDiskCallback movieResumeFunction)
{
    mMovieResumeFunction = movieResumeFunction;
}

void iTRCDisk::SetMovieResetCallBack(iTRCDiskCallback movieResetCallBack)
{
    mMovieResetCallBack = movieResetCallBack;
}

bool iTRCDisk::IsDiskIDed()
{
    return DVDCheckDisk();
}

void iTRCDisk::SetInFModLockingCode(bool value)
{
    mInFModLockingCode = value;
}

void iTRCDisk::Shutdown()
{
    mInCriticalSection = false;
    mFModError = false;
    mDiskErrorInHigherPriorityThread = false;
}

void iTRCDisk::DisplayErrorMessage()
{
    if (strcmp(mMessage, "") == 0) {
        return;
    }
    GXCullMode oldCullMode;
    GXGetCullMode(&oldCullMode);
    bool exitRW = false;
    if (!_RwDlRenderMode) {
        iSystemInitRWFailSafe();
        exitRW = true;
    }
    ROMFont::InitDisplay(_RwDlRenderMode);
    S32 dvdStatus = SetDVDState();
    while (dvdStatus != DVD_STATE_END && dvdStatus != DVD_STATE_BUSY) {
        ROMFont::DrawTextBox(25, 170, 452, 100, mMessage);
        if (dvdStatus != DVD_STATE_FATAL_ERROR) {
            ResetButton::CheckResetButton();
        }
        xPadUpdate(0, 0.0f);
        dvdStatus = SetDVDState();
    }
    ResetMessage();
    if (exitRW) {
        iSystemExitRWFailSafe();
        ROMFont::DrawBlackScreen();
        ROMFont::DrawBlackScreen();
    }
    GXSetCullMode(oldCullMode);
    VISetBlack(TRUE);
    VIFlush();
    VIWaitForRetrace();
    gWasDiskError = TRUE;
    ROMFont::DestroyDisplay();
}

S32 iTRCDisk::SetDVDState()
{
    S32 dvdDriveStatus = DVDGetDriveStatus();
    switch (dvdDriveStatus) {
    case DVD_STATE_NO_DISK:
        SetErrorMessage(diskNoDiskMessage[GetRegion()]);
        ResetButton::SetDiscIded(false);
        break;
    case DVD_STATE_WRONG_DISK:
        SetErrorMessage(diskWrongDiskMessage[GetRegion()]);
        ResetButton::SetDiscIded(false);
        break;
    case DVD_STATE_COVER_OPEN:
        SetErrorMessage(diskCoverOpenMessage[GetRegion()]);
        ResetButton::SetDiscIded(false);
        break;
    case DVD_STATE_RETRY:
        SetErrorMessage(diskRetryMessage[GetRegion()]);
        ResetButton::SetDiscIded(false);
        break;
    case DVD_STATE_END:
        break;
    case DVD_STATE_FATAL_ERROR:
        SetErrorMessage(diskFatalMessage[GetRegion()]);
        break;
    default:
        if (DVDCheckDisk()) {
            ResetButton::SetDiscIded(true);
            ResetMessage();
            memset(mMessage, ' ', 10);
        } else {
            SetErrorMessage(diskNotIdentifiedMessage[GetRegion()]);
            dvdDriveStatus = DVD_STATE_COVER_OPEN;
            ResetButton::SetDiscIded(false);
        }
        break;
    }
    return dvdDriveStatus;
}

bool iTRCDisk::CheckDVDAndResetState()
{
    S32 dvdDriveStatus = DVDGetDriveStatus();
    S32 checkDVD = DVDCheckDisk();
    if ((dvdDriveStatus != DVD_STATE_END && dvdDriveStatus != DVD_STATE_BUSY) || mFModError || mDiskErrorInHigherPriorityThread) {
        return true;
    }
    return false;
}

bool iTRCDisk::TRCCheck()
{
    OSCheckActiveThreads();
    OSGetCurrentThread();
    if (!mInCriticalSection) {
        mInCriticalSection = true;
        EnableMovieResetCallBack(false);
    } else {
        while (mInCriticalSection) {
        }
    }
    S32 dvdDriveStatus = SetDVDState();
    S32 checkDVD = DVDCheckDisk();
    bool wasThereAnError = false;
    if (dvdDriveStatus != DVD_STATE_FATAL_ERROR) {
        ResetButton::CheckResetButton();
    }
    if ((dvdDriveStatus != DVD_STATE_END && dvdDriveStatus != DVD_STATE_BUSY) || mFModError || mDiskErrorInHigherPriorityThread) {
        if (mSndSuspend) {
            mSndSuspend();
        }
        if (mPadStopRumbling) {
            mPadStopRumbling();
        }
        if (mMovieSuspendFunction) {
            mMovieSuspendFunction();
        }
        if (mAsyncLoadSuspend) {
            mAsyncLoadSuspend();
        }
        DisplayErrorMessage();
        if (mMovieResumeFunction) {
            mMovieResumeFunction();
        }
        if (mAsyncLoadResume) {
            mAsyncLoadResume();
        }
        if (mSndResume) {
            mSndResume();
        }
        wasThereAnError = true;
        ROMFont::DrawBlackScreen();
        ROMFont::DrawBlackScreen();
        VISetBlack(FALSE);
        VIFlush();
        VIWaitForRetrace();
    }
    mInCriticalSection = false;
    mDiskErrorInHigherPriorityThread = false;
    if (mFModThread && mFModError) {
        OSSetThreadPriority(mFModThread, mFModPriority);
        mFModThread = NULL;
    }
    mFModError = false;
    EnableMovieResetCallBack(true);
    return wasThereAnError;
}

void iTRCDisk::FMODCallBack(S32 status)
{
    if (!mInCriticalSection && status && !mFModError) {
        mFModError = true;
        mFModThread = OSGetCurrentThread();
        mFModPriority = OSGetThreadPriority(mFModThread);
        OSSetThreadPriority(mFModThread, OS_PRIORITY_MAX);
    }
}

bool iTRCDisk::CheckDiskErrorInHigherPriorityThread()
{
    if (!mDiskErrorInHigherPriorityThread) {
        mDiskErrorInHigherPriorityThread = CheckDVDAndResetState();
    }
    return mDiskErrorInHigherPriorityThread;
}

void iTRCDisk::FMODUpdateDiskError(S32)
{
    mFModError = true;
    if (mInFModLockingCode) {
        iTRC_CHECK();
    }
}

void iTRCDisk::EnableMovieResetCallBack(bool enable)
{
    if (enable) {
        OSSetResetCallback(mMovieResetCallBack);
    } else {
        OSSetResetCallback(NULL);
    }
}

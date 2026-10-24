#ifndef __BINKH__
#define __BINKH__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BINK *HBINK;

struct BINKIO;
typedef S32(*BINKIOOPEN)(struct BINKIO* Bnkio, const char* name, U32 flags);
typedef U32(*BINKIOREADHEADER)(struct BINKIO* Bnkio, S32 Offset, void* Dest, U32 Size);
typedef U32(*BINKIOREADFRAME)(struct BINKIO* Bnkio, U32 Framenum, S32 origofs, void* dest, U32 size);
typedef U32(*BINKIOGETBUFFERSIZE)(struct BINKIO* Bnkio, U32 Size);
typedef void(*BINKIOSETINFO)(struct BINKIO* Bnkio, void* Buf, U32 Size, U32 FileSize, U32 simulate);
typedef U32(*BINKIOIDLE)(struct BINKIO* Bnkio);
typedef void(*BINKIOCLOSE)(struct BINKIO* Bnkio);
typedef void(*BINKCBSUSPEND)(struct BINKIO* Bnkio);
typedef S32(*BINKCBTRYSUSPEND)(struct BINKIO* Bnkio);
typedef void(*BINKCBRESUME)(struct BINKIO* Bnkio);
typedef void(*BINKCBIDLE)(struct BINKIO* Bnkio);

struct BINKIO
{
    BINKIOREADHEADER ReadHeader;
    BINKIOREADFRAME ReadFrame;
    BINKIOGETBUFFERSIZE GetBufferSize;
    BINKIOSETINFO SetInfo;
    BINKIOIDLE Idle;
    BINKIOCLOSE Close;
    HBINK bink;
    volatile U32 ReadError;
    volatile U32 DoingARead;
    volatile U32 BytesRead;
    volatile U32 Working;
    volatile U32 TotalTime;
    volatile U32 ForegroundTime;
    volatile U32 IdleTime;
    volatile U32 ThreadTime;
    volatile U32 BufSize;
    volatile U32 BufHighUsed;
    volatile U32 CurBufSize;
    volatile U32 CurBufUsed;
    volatile U8 iodata[128+32];
    BINKCBSUSPEND suspend_callback;
    BINKCBTRYSUSPEND try_suspend_callback;
    BINKCBRESUME resume_callback;
    BINKCBIDLE idle_on_callback;
    volatile U32 callback_control[16];
};

struct BINKSND;
typedef S32(*BINKSNDOPEN)(struct BINKSND* BnkSnd, U32 freq, S32 bits, S32 chans, U32 flags, HBINK bink);
typedef S32(*BINKSNDREADY)(struct BINKSND* BnkSnd);
typedef S32(*BINKSNDLOCK)(struct BINKSND* BnkSnd, U8** addr, U32* len);
typedef S32(*BINKSNDUNLOCK)(struct BINKSND* BnkSnd, U32 filled);
typedef void(*BINKSNDVOLUME)(struct BINKSND* BnkSnd, S32 volume);
typedef void(*BINKSNDPAN)(struct BINKSND* BnkSnd, S32 pan);
typedef void(*BINKSNDMIXBINS)(struct BINKSND* BnkSnd, U32* mix_bins, U32 total);
typedef void(*BINKSNDMIXBINVOLS)(struct BINKSND* BnkSnd, U32* vol_mix_bins, S32* volumes, U32 total);
typedef S32(*BINKSNDONOFF)(struct BINKSND* BnkSnd, S32 status);
typedef S32(*BINKSNDPAUSE)(struct BINKSND* BnkSnd, S32 status);
typedef void(*BINKSNDCLOSE)(struct BINKSND* BnkSnd);
typedef BINKSNDOPEN(*BINKSNDSYSOPEN)(U32 param);

struct BINKSND
{
    BINKSNDREADY Ready;
    BINKSNDLOCK Lock;
    BINKSNDUNLOCK Unlock;
    BINKSNDVOLUME Volume;
    BINKSNDPAN Pan;
    BINKSNDPAUSE Pause;
    BINKSNDONOFF SetOnOff;
    BINKSNDCLOSE Close;
    BINKSNDMIXBINS MixBins;
    BINKSNDMIXBINVOLS MixBinVols;
    U32 sndbufsize;
    U8* sndbuf;
    U8* sndend;
    U8* sndwritepos;
    U8* sndreadpos;
    U32 sndcomp;
    U32 sndamt;
    U32 sndconvert8;
    U32 sndendframe;
    U32 sndprime;
    U32 sndpad;
    U32 BestSizeIn16;
    U32 BestSizeMask;
    U32 SoundDroppedOut;
    S32 NoThreadService;
    S32 OnOff;
    U32 Latency;
    U32 VideoScale;
    U32 freq;
    S32 bits;
    S32 chans;
    U8 snddata[256];
};

typedef struct BINKRECT {
    S32 Left,Top,Width,Height;
} BINKRECT;

#define BINKMAXDIRTYRECTS 8

typedef struct BUNDLEPOINTERS {
    void* typeptr;
    void* type16ptr;
    void* colorptr;
    void* bits2ptr;
    void* motionXptr;
    void* motionYptr;
    void* dctptr;
    void* mdctptr;
    void* patptr;
} BUNDLEPOINTERS;

typedef struct BINK
{
    U32 Width;
    U32 Height;
    U32 Frames;
    U32 FrameNum;
    U32 LastFrameNum;
    U32 FrameRate;
    U32 FrameRateDiv;
    U32 ReadError;
    U32 OpenFlags;
    U32 BinkType;
    U32 Size;
    U32 FrameSize;
    U32 SndSize;
    BINKRECT FrameRects[BINKMAXDIRTYRECTS];
    S32 NumRects;
    U32 PlaneNum;
    void* YPlane[2];
    void* APlane[2];
    U32 YWidth;
    U32 YHeight;
    U32 UVWidth;
    U32 UVHeight;
    void* MaskPlane;
    U32 MaskPitch;
    U32 MaskLength;
    U32 LargestFrameSize;
    U32 InternalFrames;
    S32 NumTracks;
    U32 Highest1SecRate;
    U32 Highest1SecFrame;
    S32 Paused;
    U32 BackgroundThread;
    void* compframe;
    void* preloadptr;
    U32* frameoffsets;
    BINKIO bio;
    U8* ioptr;
    U32 iosize;
    U32 decompwidth;
    U32 decompheight;
    S32* trackindexes;
    U32* tracksizes;
    U32* tracktypes;
    S32* trackIDs;
    U32 numrects;
    U32 playedframes;
    U32 firstframetime;
    U32 startframetime;
    U32 startblittime;
    U32 startsynctime;
    U32 startsyncframe;
    U32 twoframestime;
    U32 entireframetime;
    U32 slowestframetime;
    U32 slowestframe;
    U32 slowest2frametime;
    U32 slowest2frame;
    U32 soundon;
    U32 videoon;
    U32 totalmem;
    U32 timevdecomp;
    U32 timeadecomp;
    U32 timeblit;
    U32 timeopen;
    U32 fileframerate;
    U32 fileframeratediv;
    U32 runtimeframes;
    S32 rtindex;
    U32* rtframetimes;
    U32* rtadecomptimes;
    U32* rtvdecomptimes;
    U32* rtblittimes;
    U32* rtreadtimes;
    U32* rtidlereadtimes;
    U32* rtthreadreadtimes;
    U32 lastblitflags;
    U32 lastdecompframe;
    U32 lastresynctime;
    U32 doresync;
    U32 skipcount;
    U32 toofewskipstomask;
    U32 playingtracks;
    U32 soundskips;
    BINKSND* bsnd;
    U32 skippedlastblit;
    U32 skipped_this_frame;
    U32 skippedblits;
    BUNDLEPOINTERS bunp;
    U32 skipped_in_a_row;
    U32 big_sound_skip_adj;
    U32 big_sound_skip_reduce;
    U32 last_time_almost_empty;
    U32 last_read_count;
    U32 last_sound_count;
    U32 snd_callback_buffer[16];
} BINK;

#define BINKYAINVERT 0x00000800L
#define BINKFRAMERATE 0x00001000L
#define BINKPRELOADALL 0x00002000L
#define BINKSNDTRACK 0x00004000L
#define BINKOLDFRAMEFORMAT 0x00008000L
#define BINKRBINVERT 0x00010000L
#define BINKGRAYSCALE 0x00020000L
#define BINKNOMMX 0x00040000L
#define BINKNOSKIP 0x00080000L
#define BINKALPHA 0x00100000L
#define BINKNOFILLIOBUF 0x00200000L
#define BINKSIMULATE 0x00400000L
#define BINKFILEHANDLE 0x00800000L
#define BINKIOSIZE 0x01000000L
#define BINKIOPROCESSOR 0x02000000L
#define BINKFROMMEMORY 0x04000000L
#define BINKNOTHREADEDIO 0x08000000L

#define BINKSURFACEFAST 0x00000000
#define BINKSURFACESLOW 0x08000000
#define BINKSURFACEDIRECT 0x04000000

#define BINKCOPYALL 0x80000000L
#define BINKCOPY2XH 0x10000000L
#define BINKCOPY2XHI 0x20000000L
#define BINKCOPY2XW 0x30000000L
#define BINKCOPY2XWH 0x40000000L
#define BINKCOPY2XWHI 0x50000000L
#define BINKCOPY1XI 0x60000000L
#define BINKCOPYNOSCALING 0x70000000L

#define BINKSURFACE8P 0
#define BINKSURFACE24 1
#define BINKSURFACE24R 2
#define BINKSURFACE32 3
#define BINKSURFACE32R 4
#define BINKSURFACE32A 5
#define BINKSURFACE32RA 6
#define BINKSURFACE4444 7
#define BINKSURFACE5551 8
#define BINKSURFACE555 9
#define BINKSURFACE565 10
#define BINKSURFACE655 11
#define BINKSURFACE664 12
#define BINKSURFACEYUY2 13
#define BINKSURFACEUYVY 14
#define BINKSURFACEYV12 15
#define BINKSURFACEMASK 15

#define BINKGOTOQUICK 1
#define BINKGOTOQUICKSOUND 2

#define BINKGETKEYPREVIOUS     0
#define BINKGETKEYNEXT         1
#define BINKGETKEYCLOSEST      2
#define BINKGETKEYNOTEQUAL   128

S32 BinkDoFrame(HBINK bnk);
void BinkNextFrame(HBINK bnk);
S32 BinkWait(HBINK bnk);
S32 BinkPause(HBINK bnk, S32 pause);
S32 BinkCopyToBuffer(HBINK bnk, void* dest, S32 destpitch, U32 destheight, U32 destx, U32 desty, U32 flags);
void BinkGoto(HBINK bnk, U32 frame, S32 flags);

#ifdef __cplusplus
}
#endif

#endif

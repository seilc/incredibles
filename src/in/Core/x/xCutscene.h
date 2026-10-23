#ifndef XCUTSCENE_H
#define XCUTSCENE_H

#include "xFile.h"
#include "xCSNNosey.h"
#include "xModel.h"
#include "iSnd.h"

struct xCutsceneAudioTrack
{
    U32 uLeftSoundId;
    U32 uRightSoundId;
    char szLeftSound[28];
    char szRightSound[28];
};

struct xCutsceneInfo
{
    U32 Magic;
    U32 AssetID;
    U32 NumData;
    U32 NumTime;
    U32 MaxModel;
    U32 MaxBufEven;
    U32 MaxBufOdd;
    U32 HeaderSize;
    U32 VisCount;
    U32 VisSize;
    U32 BreakCount;
    U32 pad;
    xCutsceneAudioTrack AudioTracks[32];
};

struct xCutsceneData
{
    U32 DataType;
    U32 AssetID;
    U32 ChunkSize;
    union
    {
        U32 FileOffset;
        void* DataPtr;
    };
};

struct xCutsceneBreak
{
    F32 Time;
    S32 Index;
};

struct xCutsceneTime
{
    F32 StartTime;
    F32 EndTime;
    U32 NumData;
    U32 ChunkIndex;
};

struct xCutsceneModelHack
{
    xModelPipe* pipe;
};

struct xCutscene
{
    xCutsceneInfo* Info;
    xCutsceneData* Data;
    U32* TimeChunkOffs;
    U32* Visibility;
    xCutsceneBreak* BreakList;
    xCutsceneTime* Play;
    xCutsceneTime* Stream;
    U32 Waiting;
    U32 BadReadPause;
    F32 BadReadSpeed;
    void* RawBuf;
    void* AlignBuf;
    F32 Time;
    F32 CamTime;
    U32 PlayIndex;
    U32 Ready;
    S32 DataLoading;
    U32 GotData;
    U32 ShutDownWait;
    F32 PlaybackSpeed;
    U32 Opened;
    xFile File;
    S32 AsyncID;
    void* MemBuf;
    void* MemCurr;
    U32 SndStarted;
    U32 SndNumChannel;
    U32 SndChannelReq[2];
    U32 SndAssetID[2];
    iSndHandle SndHandle[2];
    U32 SubTitlesID;
    XCSNNosey* cb_nosey;
    xCutsceneModelHack* ModelHack;
    bool started;
};

#endif
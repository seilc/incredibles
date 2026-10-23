#ifndef XANIM_H
#define XANIM_H

#include "xMath3.h"
#include "xMemMgr.h"
#include "iSnd.h"

struct xAnimPhysicsData
{
    xVec3* tranTable;
    F32* yawTable;
    S32 tranCount;
};

struct xAnimFile
{
    xAnimFile* Next;
    const char* Name;
    U32 ID;
    U32 FileFlags;
    F32 Duration;
    F32 TimeOffset;
    U16 BoneCount;
    U8 NumAnims[2];
    void** RawData;
    xAnimPhysicsData* PhysicsData;
};

struct xAnimActiveEffect;
struct xAnimSingle;

struct xAnimEffect
{
    xAnimEffect* Next;
    U16 Flags;
    U16 Probability;
    F32 StartTime;
    F32 EndTime;
    U32 (*Callback)(U32, xAnimActiveEffect*, xAnimSingle*, void*);
};

struct xAnimActiveEffect
{
    xAnimEffect* Effect;
    union
    {
        U32 Handle;
        iSndHandle SndHandle;
    };
};

struct xAnimMultiFileBase
{
    U32 Count;
};

struct xAnimMultiFileEntry
{
    U32 ID;
    xAnimFile* File;
};

struct xAnimMultiFile : xAnimMultiFileBase
{
    xAnimMultiFileEntry Files[1];
};

struct xAnimTransitionList;
struct xAnimPlay;

struct xAnimState
{
    xAnimState* Next;
    const char* Name;
    U32 ID;
    U32 Flags;
    U32 UserFlags;
    F32 Speed;
    xAnimFile* Data;
    xAnimEffect* Effects;
    xAnimTransitionList* Default;
    xAnimTransitionList* List;
    F32* BoneBlend;
    F32* TimeSnap;
    F32 FadeRecip;
    U16* FadeOffset;
    void* CallbackData;
    xAnimMultiFile* MultiFile;
    void (*BeforeEnter)(xAnimPlay*, xAnimState*, void*);
    void (*StateCallback)(xAnimState*, xAnimSingle*, void*);
    void (*BeforeAnimMatrices)(xAnimPlay*, xQuat*, xVec3*, S32);
};

struct xAnimTransition
{
    xAnimTransition* Next;
    xAnimState* Dest;
    U32 (*Conditional)(xAnimTransition*, xAnimSingle*, void*);
    U32 (*Callback)(xAnimTransition*, xAnimSingle*, void*);
    U32 Flags;
    U32 UserFlags;
    F32 SrcTime;
    F32 DestTime;
    U16 Priority;
    U16 QueuePriority;
    F32 BlendRecip;
    U16* BlendOffset;
};

struct xAnimTransitionList
{
    xAnimTransitionList* Next;
    xAnimTransition* T;
};

struct xAnimTable
{
    const char* Name;
    xAnimTransition* TransitionList;
    xAnimState* StateList;
    U32 AnimIndex;
    U32 MorphIndex;
    U32 UserFlags;
};

struct xAnimSingle
{
    U32 SingleFlags;
    xAnimState* State;
    F32 Time;
    F32 CurrentSpeed;
    F32 BilinearLerp[2];
    xAnimEffect* Effect;
    U32 ActiveCount;
    F32 LastTime;
    xAnimActiveEffect* ActiveList;
    xAnimPlay* Play;
    xAnimTransition* Sync;
    xAnimTransition* Tran;
    xAnimSingle* Blend;
    F32 BlendFactor;
    xVec3 PhysDisp;
    F32 YawDisp;
    U32 pad[1];
};

struct xModelInstance;

struct xAnimPlay
{
    xAnimPlay* Next;
    U16 NumSingle;
    U16 BoneCount;
    xAnimSingle* Single;
    void* Object;
    xAnimTable* Table;
    xMemPool* Pool;
    xModelInstance* ModelInst;
    void (*BeforeAnimMatrices)(xAnimPlay*, xQuat*, xVec3*, S32);
};

xAnimTransition* xAnimTempTransitionAlloc(const xAnimTransition* source);
xAnimState* xAnimTableGetState(xAnimTable* table, const char* name);
void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, F32 startTime);
void xAnimPlayStartTransition(xAnimPlay* play, xAnimTransition* transition);

#endif
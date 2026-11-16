#ifndef XANIM_H
#define XANIM_H

#include "xMath3.h"
#include "xMemMgr.h"
#include "iSnd.h"

struct xModelInstance;

struct xAnimPhysicsData;
struct xAnimFile;
struct xAnimMultiFileBase;
struct xAnimMultiFileEntry;
struct xAnimMultiFile;
struct xAnimEffect;
struct xAnimActiveEffect;
struct xAnimState;
struct xAnimTransition;
struct xAnimTransitionList;
struct xAnimTable;
struct xAnimSingle;
struct xAnimPlay;

// xAnimFile::FileFlags
enum
{
    AnimFile_Reverse = 0x1000,
    AnimFile_BackAndForth = 0x2000
};

// xAnimState::Flags
enum
{
    AnimState_Loop = 0x10
};

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

typedef U32(*xAnimEffectCallback)(U32, xAnimActiveEffect*, xAnimSingle*, void*);

struct xAnimEffect
{
    xAnimEffect* Next;
    U16 Flags;
    U16 Probability;
    F32 StartTime;
    F32 EndTime;
    xAnimEffectCallback Callback;
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

typedef void(*xAnimBeforeEnterCallback)(xAnimPlay*, xAnimState*, void*);
typedef void(*xAnimStateCallback)(xAnimState*, xAnimSingle*, void*);
typedef void(*xAnimBeforeAnimMatricesCallback)(xAnimPlay*, xQuat*, xVec3*, S32);

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
    xAnimBeforeEnterCallback BeforeEnter;
    xAnimStateCallback StateCallback;
    xAnimBeforeAnimMatricesCallback BeforeAnimMatrices;
};

typedef U32(*xAnimTransitionCallback)(xAnimTransition*, xAnimSingle*, void*);

struct xAnimTransition
{
    xAnimTransition* Next;
    xAnimState* Dest;
    xAnimTransitionCallback Conditional;
    xAnimTransitionCallback Callback;
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
    xAnimBeforeAnimMatricesCallback BeforeAnimMatrices;
};

extern U32 gxAnimUseGrowAlloc;

void xAnimInit();
void xAnimTempTransitionInit(U32 count);
xAnimTransition* xAnimTempTransitionAlloc(const xAnimTransition* source);
void xAnimTempStateInit(U32 count);
xAnimState* xAnimTempStateAlloc(const xAnimState* source);
void xAnimCalcPhysicsData(xAnimFile* afile, xAnimFile* endPose);
xAnimFile* xAnimFileNewBilinear(void** rawData, const char* name, U32 hashed_name_id, U32 flags, xAnimFile** linkedList, U32 numX, U32 numY);
xAnimFile* xAnimFileNew(void* rawData, const char* name, U32 flags, xAnimFile** linkedList);
void xAnimFileSetTime(xAnimFile* data, F32 duration, F32 timeOffset);
void xAnimFileEval(xAnimFile* data, F32 time, F32* bilinear, U32 flags, xVec3* tran, xQuat* quat, F32*);
xAnimEffect* xAnimStateNewEffect(xAnimState* state, U16 flags, U16 probability, F32 startTime, F32 endTime, xAnimEffectCallback callback, U32 userDataSize);
xAnimTable* xAnimTableNew(const char* name, U32 userFlags);
void xAnimDefaultBeforeEnter(xAnimPlay*, xAnimState* state, void*);
xAnimState* xAnimTableNewState(xAnimTable* table, const char* name, U32 flags, U32 userFlags, F32 speed, F32* boneBlend, F32* timeSnap, F32 fadeRecip, U16* fadeOffset, void* callbackData, xAnimBeforeEnterCallback beforeEnter, xAnimStateCallback stateCallback, xAnimBeforeAnimMatricesCallback beforeAnimMatrices);
void xAnimTableNewStateMany(xAnimTable* table, const char* name, S32 howMany, U32 flags, U32 userFlags, F32 speed, F32* boneBlend, F32* timeSnap, F32 fadeRecip, U16* fadeOffset, void* callbackData, xAnimBeforeEnterCallback beforeEnter, xAnimStateCallback stateCallback, xAnimBeforeAnimMatricesCallback beforeAnimMatrices);
xAnimTransition* xAnimTableNewTransition(xAnimTable* table, const char* source, const char* dest, xAnimTransitionCallback conditional, xAnimTransitionCallback callback, U32 flags, U32 userFlags, F32 srcTime, F32 destTime, U16 priority, U16 queuePriority, F32 fBlendTime, U16* blendOffset);
void xAnimTableAddFile(xAnimTable* table, xAnimFile* file, const char* states);
xAnimState* xAnimTableAddFileID(xAnimTable* table, xAnimFile* file, U32 stateID, U32 subStateID, U32 subStateCount);
xAnimState* xAnimTableGetStateID(xAnimTable* table, U32 ID);
xAnimState* xAnimTableGetState(xAnimTable* table, const char* name);
void EffectSingleStart_ForceEffect(xAnimSingle* single, xAnimEffect* force_effect);
void xAnimPhysicsEval(xAnimPhysicsData* phys, F32 time, xVec3* disp, F32* yaw);
void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, F32 startTime);
void xAnimPlaySetup(xAnimPlay* play, void* object, xAnimTable* table, xModelInstance* modelInst);
void xAnimPlayChooseTransition(xAnimPlay* play);
void xAnimPlayStartTransition(xAnimPlay* play, xAnimTransition* transition);
void xAnimPlayUpdate(xAnimPlay* play, F32 timeDelta);
void xAnimPlayEval(xAnimPlay* play);
void xAnimPoolCB(xMemPool* pool, void* data);
void xAnimPoolInit(xMemPool* pool, U32 count, U32 singles, U32 blendFlags, U32 effectMax);
xAnimPlay* xAnimPoolAlloc(xMemPool* pool, void* object, xAnimTable* table, xModelInstance* modelInst);
void xAnimPoolFree(xAnimPlay* play);

inline F32 xAnimFileRawTime(xAnimFile* afile, F32 time)
{
    if ((afile->FileFlags & AnimFile_Reverse) || ((afile->FileFlags & AnimFile_BackAndForth) && time > 0.5f * afile->Duration))
        return afile->TimeOffset + afile->Duration - time;
    return afile->TimeOffset + time;
}

#endif

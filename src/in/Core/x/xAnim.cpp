#include "xAnim.h"

#include "xString.h"
#include "xRand.h"
#include "xModel.h"
#include "xMath.h"
#include "xMorph.h"
#include "iAnim.h"
#include "iAnimSKB.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static xMemPool sxAnimTempTranPool;
static xMemPool sxAnimTempStatePool;
U32 gxAnimUseGrowAlloc;

enum
{
    MAX_SUBST_TRANSITION = 32
};

#define xANIMALLOC(size) (gxAnimUseGrowAlloc ? xMEMGROWALLOC((size)) : xMEMALLOC((size), 0))

#define xAnimFileRawDuration(afile) (((afile)->FileFlags & 0x8000) ? xMorphSeqDuration((xMorphSeqFile*)(afile)->RawData[0]) : iAnimDuration((afile)->RawData[0]))

static bool _xSingleCompare(char ch, char pattern)
{
    switch (pattern) {
    case '+':
    case '*':
    case '?':
        return true;
    case '#':
        return ch >= '0' && ch <= '9';
    }

    return ch == pattern;
}

static bool _xSingleCompare(char ch, const char* pattern)
{
    while (*pattern) {
        if (_xSingleCompare(ch, *pattern))
            return true;
        pattern++;
    }
    return false;
}

static bool _xCharIn(char ch, const char* string)
{
    for (S32 i = 0; string[i]; i++)
        if (ch == string[i])
            return true;
    return false;
}

static bool _xCheckAnimNameInner(const char* name, const char* pattern, S32 patternSize, char* extra, S32* nameOut, S32* extraOut)
{
    const char* startExtra = NULL;
    const char* initialExtra = extra;
    S32 patternCurrent = 0, nameCurrent = 0;

    while (patternCurrent < patternSize) {
        char masterCharacter = pattern[patternCurrent];
        switch (masterCharacter) {
        case '+':
        case '?':
            patternCurrent++;
            if (nameCurrent == 0)
                return false;
            nameCurrent++;
            if (masterCharacter == '?') {
                break;
            }
        case '*':
        {
            S32 check = patternCurrent + 1;
            char nextPattern[128] = { pattern[check] };
            while (nextPattern[0] == '{' || nextPattern[0] == '}' || nextPattern[0] == '<' || nextPattern[0] == '>')
                nextPattern[0] = pattern[++check];
            if (nextPattern[0] == '(') {
                S32 nextPatternCount = 0;
                bool first = true;
                S32 parenCount = 0;
                while (pattern[check] && parenCount >= 0) {
                    if (pattern[check] == '(')
                        parenCount++;
                    else if (pattern[check] == ')')
                        parenCount--;
                    else if (parenCount == 0) {
                        const char* IGNORE_PATTERNS = "{}()<>";
                        if (pattern[check] == '|')
                            first = true;
                        else if (first && !_xCharIn(pattern[check], IGNORE_PATTERNS)) {
                            nextPattern[nextPatternCount++] = pattern[check];
                            first = false;
                        }
                    }
                    check++;
                }
                nextPattern[nextPatternCount] = '\0';
            }
            while (name[nameCurrent] && !_xSingleCompare(name[nameCurrent], nextPattern))
                nameCurrent++;
            patternCurrent++;
            break;
        }
        case '#':
            if (name[nameCurrent] < '0' || name[nameCurrent] > '9')
                return false;
            nameCurrent++;
            while (name[nameCurrent] >= '0' && name[nameCurrent] <= '9')
                nameCurrent++;
            patternCurrent++;
            break;
        case '{':
            startExtra = &name[nameCurrent];
            patternCurrent++;
            break;
        case '}':
        {
            const char* endExtra = &name[nameCurrent];
            S32 length = endExtra - startExtra;
            if (extra) {
                memcpy(extra, startExtra, length);
                extra[length] = '\0';
                extra += length + 1;
                *extra = 1;
            }
            startExtra = NULL;
            patternCurrent++;
            break;
        }
        case '(':
        {
            bool done = false;
            patternCurrent++; const char* current = &pattern[patternCurrent];
            while (*current != ')' && *current) {
                const char* startPattern = current;
                while (*current && *current != ')' && *current != '|') {
                    if (*current == '(') {
                        S32 pc = 1;
                        while (*current && pc > 0) {
                            if (*current == ')')
                                pc--;
                            else if (*current == '(')
                                pc++;
                            current++;
                        }
                        if (*current)
                            current++;
                    } else
                        current++;
                }
                if (current != startPattern) {
                    S32 nameOut = 0, extraOut = 0;
                    bool ret = !done && _xCheckAnimNameInner(&name[nameCurrent], startPattern, current - startPattern, extra, &nameOut, &extraOut);
                    if (ret) {
                        nameCurrent += nameOut;
                        extra += extraOut;
                        done = true;
                    } else if (extra)
                        *extra = 1;
                }
                if (*current == '|') {
                    current++;
                }
            }
            if (*current) {
                current++;
            }
            patternCurrent += current - &pattern[patternCurrent];
            if (!done) {
                return false;
            }
            break;
        }
        case '<':
        {
            patternCurrent++;
            const char* current = &pattern[patternCurrent];
            const char* positive = current;
            while (*current && *current != ';' && *current != '>')
                current++;
            const char* positiveEnd = current;
            const char* negative = NULL;
            const char* negativeEnd = NULL;
            if (*current == ';') {
                current++;
                negative = current;
                while (*current && *current != '>')
                    current++;
                negativeEnd = current;
            }
            S32 nameOut, extraOut;
            bool matched = _xCheckAnimNameInner(&name[nameCurrent], positive, positiveEnd - positive, extra, &nameOut, &extraOut);
            if (matched) {
                if (negative && _xCheckAnimNameInner(&name[nameCurrent], negative, negativeEnd - negative, NULL, NULL, NULL)) {
                    if (extra)
                        *extra = 1;
                    matched = false;
                } else {
                    nameCurrent += nameOut;
                    extra += extraOut;
                }
            }
            if (*current) {
                current++;
            }
            patternCurrent += current - &pattern[patternCurrent];
            if (!matched) {
                return false;
            }
            break;
        }
        case '\0':
            return false;
        default:
            if (name[nameCurrent] != pattern[patternCurrent])
                return false;
            nameCurrent++;
            patternCurrent++;
        }
    }

    if (nameOut)
        *nameOut = nameCurrent;
    if (extraOut)
        *extraOut = extra - initialExtra;
    return true;
}

static bool _xCheckAnimName(const char* name, const char* pattern, char* extra)
{
    *extra = 1;
    S32 patternLength = strlen(pattern);
    S32 sizeOut;
    bool ret = _xCheckAnimNameInner(name, pattern, patternLength, extra, &sizeOut, NULL);
    return ret && name[sizeOut] == '\0';
}

void xAnimInit()
{
    iAnimInit();

    memset(&sxAnimTempTranPool, 0, sizeof(xMemPool));
    memset(&sxAnimTempStatePool, 0, sizeof(xMemPool));
}

void xAnimTempTransitionInit(U32 count)
{
    xMemPoolSetup(&sxAnimTempTranPool, xMEMALLOC(count * sizeof(xAnimTransition), 0), 0, 0, NULL, sizeof(xAnimTransition), count, 0);
}

xAnimTransition* xAnimTempTransitionAlloc(const xAnimTransition* source)
{
    xAnimTransition* tran = (xAnimTransition*)xMemPoolAlloc(&sxAnimTempTranPool);
    if (source)
        memcpy(tran, source, sizeof(xAnimTransition));
    else
        memset(tran, 0, sizeof(xAnimTransition));
    tran->Next = NULL;
    tran->Flags |= 0x2;

    return tran;
}

void xAnimTempStateInit(U32 count)
{
    xMemPoolSetup(&sxAnimTempStatePool, xMEMALLOC(count * sizeof(xAnimState), 0), 0, 0, NULL, sizeof(xAnimState), count, 0);
}

xAnimState* xAnimTempStateAlloc(const xAnimState* source)
{
    xAnimState* state = (xAnimState*)xMemPoolAlloc(&sxAnimTempStatePool);
    if (source)
        memcpy(state, source, sizeof(xAnimState));
    else
        memset(state, 0, sizeof(xAnimState));
    state->Next = NULL;
    state->Flags |= 0x800;

    return state;
}

static F32 CalcRecipBlendMax(U16* timeOffs)
{
    F32 test, max = 0.0f;
    while (*timeOffs != 0xFFFF) {
        test = timeOffs[0] / 1024.0f + (timeOffs[1] == 0 ? 0.0f : 1.0f / (timeOffs[1] / 1024.0f));
        if (test > max) max = test;
        timeOffs += 2;
    }
    if (max == 0.0f) {
        return 0.0f;
    } else {
        return 1.0f / max;
    }
}

static U32 StateHasTransition(xAnimState* state, xAnimTransition* tran)
{
    xAnimTransitionList* curr = state->List;
    while (curr) {
        if (curr->T == tran) return TRUE;
        curr = curr->Next;
    }
    return FALSE;
}

static U32 DefaultHasTransition(xAnimState* state, xAnimTransition* tran, U32* allocCount)
{
    U32 needalloc = 1;

    xAnimTransitionList* curr = state->Default;
    while (curr) {
        if (curr->T == tran) return TRUE;
        if (curr->T->Conditional == tran->Conditional &&
            curr->T->UserFlags == tran->UserFlags)
            needalloc = 0;

        curr = curr->Next;
    }
    *allocCount += needalloc;
    return FALSE;
}

static void _xAnimTransitionListInsert(xAnimTransitionList** list, xAnimTransitionList* toInsert)
{
    if (*list == NULL || toInsert->T->Priority > (*list)->T->Priority ||
        (toInsert->T->Priority == (*list)->T->Priority && toInsert->T->Conditional && !(*list)->T->Conditional)) {
        toInsert->Next = *list;
        *list = toInsert;
    } else {
        xAnimTransitionList* curr = *list;
        while (curr->Next && (curr->Next->T->Priority > toInsert->T->Priority ||
                              (curr->Next->T->Priority == toInsert->T->Priority && (toInsert->T->Conditional ? false : curr->Next->T->Conditional != NULL)))) {
            curr = curr->Next;
        }
        toInsert->Next = curr->Next;
        curr->Next = toInsert;
    }
}

static U32 DefaultOverride(xAnimState* state, xAnimTransition* tran)
{
    xAnimTransitionList *curr = state->Default, *prev;
    while (curr) {
        if (curr->T->Conditional == tran->Conditional && curr->T->UserFlags == tran->UserFlags) {
            if (curr->T->Priority == tran->Priority) {
                curr->T = tran;
            } else if (tran->Priority > curr->T->Priority) {
                if (curr == state->Default) {
                    curr->T = tran;
                } else {
                    prev->Next = curr->Next;
                    curr->T = tran;
                    _xAnimTransitionListInsert(&state->Default, curr);
                }
            }
            return TRUE;
        }
        prev = curr;
        curr = curr->Next;
    }
    return FALSE;
}

static inline void SetTransition(xAnimTransition*& dest, xAnimTransition* tran)
{
    if (dest == tran) {
        return;
    }

    if (dest && (dest->Flags & 0x2))
        xMemPoolFree(&sxAnimTempTranPool, dest);
    dest = tran;
}

static inline void SetState(xAnimState*& dest, xAnimState* state)
{
    if (state && (state->Flags & 0x400)) {
        state = xAnimTempStateAlloc(state);
    }

    if (dest == state) {
        return;
    }

    if (dest && (dest->Flags & 0x800))
        xMemPoolFree(&sxAnimTempStatePool, dest);
    dest = state;
}

static void TransitionTimeInit(xAnimSingle* single, xAnimTransition* tran)
{
    switch (tran->Flags & 0xE0) {
    case 0x20:
    {
        xAnimFile* src = single->State->Data;
        xAnimFile* dest = tran->Dest->Data;
        if ((src->FileFlags ^ dest->FileFlags) & AnimFile_Reverse) {
            single->Time = dest->Duration - single->Time;
        }
        break;
    }
    case 0x40:
    {
        xAnimFile* src = single->State->Data;
        xAnimFile* dest = tran->Dest->Data;
        single->Time *= dest->Duration / src->Duration;
        if ((src->FileFlags ^ dest->FileFlags) & AnimFile_Reverse) {
            single->Time = dest->Duration - single->Time;
        }
        break;
    }
    case 0x80:
    {
        xAnimFile* src = single->State->Data;
        xAnimFile* dest = tran->Dest->Data;
        F32 played = single->Time / src->Duration;
        single->Time = (1.0f - played) * dest->Duration;
        break;
    }
    default:
        single->Time = (tran->Dest->Flags & 0x100) && tran->DestTime == 0.0f ? single->State->Data->Duration * xurand() : tran->DestTime;
    }

    single->LastTime = single->Time;
}

void xAnimCalcPhysicsData(xAnimFile* afile, xAnimFile* endPose)
{
    S32 numAnims = afile->NumAnims[0] * afile->NumAnims[1];
    iAnimSKBHeader* firstHeader = (iAnimSKBHeader*)afile->RawData[0];
    if (firstHeader->ExtractedData) {
        afile->PhysicsData = (xAnimPhysicsData*)firstHeader->ExtractedData;
        return;
    }

    U8 axis[3];
    if (afile->FileFlags & 0x10000) {
        axis[0] = 1;
    } else {
        axis[0] = 0;
    }

    if (afile->FileFlags & 0x20000) {
        axis[1] = 1;
    } else {
        axis[1] = 0;
    }

    if (afile->FileFlags & 0x40000) {
        axis[2] = 1;
    } else {
        axis[2] = 0;
    }

    S32 tranCount = _iAnimSKBNumExtract((iAnimSKBHeader*)afile->RawData[0], 1);

    U32 size = tranCount * sizeof(xVec3) + sizeof(xAnimPhysicsData);
    if (afile->FileFlags & 0x80000) {
        size += tranCount * sizeof(F32);
    }

    xAnimPhysicsData* physDat = (xAnimPhysicsData*)xANIMALLOC(size);
    afile->PhysicsData = physDat;
    physDat->tranTable = (xVec3*)(physDat + 1);

    xVec3* tranTable = physDat->tranTable;
    if (afile->FileFlags & 0x80000) {
        physDat->yawTable = (F32*)(tranTable + tranCount);
    } else {
        physDat->yawTable = NULL;
    }

    F32* yawTable = physDat->yawTable;
    xVec3 tranStart[64];
    xQuat quatStart[64];
    xVec3 tranEnd[64];
    xQuat quatEnd[64];
    iAnimEval(afile->RawData[0], 0.0f, 0, tranStart, quatStart);

    if (!axis[0])
        tranStart[1].x = 0.0f;
    if (!axis[1])
        tranStart[1].y = 0.0f;
    if (!axis[2]) {
        tranStart[1].z = 0.0f;
    }

    if (endPose) {
        iAnimEval(endPose->RawData[0], 0.0f, 0, tranEnd, quatEnd);
    } else {
        iAnimEval(afile->RawData[0], 0.0f, 0, tranEnd, quatEnd);
    }

    if (!axis[0])
        tranEnd[1].x = 0.0f;
    if (!axis[1])
        tranEnd[1].y = 0.0f;
    if (!axis[2]) {
        tranEnd[1].z = 0.0f;
    }

    physDat->tranCount = _iAnimSKBExtractTranslateAxis((iAnimSKBHeader*)afile->RawData[0], 1, tranTable, tranCount, axis);

    if (numAnims > 1) {
        xVec3* tempTable = (xVec3*)xMEMPUSHTEMP(tranCount * sizeof(xVec3));
        for (S32 i = 1; i < numAnims; i++) {
            _iAnimSKBExtractTranslateAxis((iAnimSKBHeader*)afile->RawData[i], 1, tempTable, tranCount, axis);
        }
        xMEMPOPTEMP(tempTable);
    }

    for (S32 i = 1; i < tranCount; i++) {
        xVec3 subOff;
        xVec3Sub(&subOff, &tranEnd[1], &tranStart[1]);
        xVec3SMulBy(&subOff, (F32)i / (F32)(tranCount - 1));
        xVec3AddTo(&subOff, tranTable);
        xVec3Sub(&tranTable[i], &tranTable[i], &subOff);
    }
    xVec3Copy(tranTable, &g_O3);

    for (S32 i = 0; i < numAnims; i++) {
        _iAnimSKBAdjustTranslate((iAnimSKBHeader*)afile->RawData[i], 1, &tranStart[1].x, &tranEnd[1].x);
    }

    if (yawTable) {
        F32 yawStart = QuatToYaw(&quatStart[1]);
        F32 yawEnd = QuatToYaw(&quatEnd[1]);

        physDat->tranCount = _iAnimSKBExtractYaw((iAnimSKBHeader*)afile->RawData[0], 1, yawTable, tranCount);

        if (numAnims > 1) {
            F32* tempYaw = (F32*)xMEMPUSHTEMP(tranCount * sizeof(F32));
            for (S32 i = 1; i < numAnims; i++) {
                _iAnimSKBExtractYaw((iAnimSKBHeader*)afile->RawData[i], 1, tempYaw, tranCount);
            }
            xMEMPOPTEMP(tempYaw);
        }

        for (S32 i = 1; i < tranCount; i++) {
            F32 subOff = i * (yawEnd - yawStart) / (tranCount - 1);
            subOff += yawTable[0];
            yawTable[i] -= subOff;
        }
        yawTable[0] = 0.0f;

        for (S32 i = 0; i < numAnims; i++) {
            _iAnimSKBAdjustYaw((iAnimSKBHeader*)afile->RawData[i], 1, yawStart, yawEnd);
        }
    }

    for (S32 i = 0; i < numAnims; i++) {
        ((iAnimSKBHeader*)afile->RawData[i])->ExtractedData = physDat;
    }
}

xAnimFile* xAnimFileNewBilinear(void** rawData, const char* name, U32 hashed_name_id, U32 flags, xAnimFile** linkedList, U32 numX, U32 numY)
{
    S32 i;
    xAnimFile* afile = (xAnimFile*)xANIMALLOC(numX * numY * sizeof(void*) + sizeof(xAnimFile));

    if (numX > 1 || numY > 1) {
        flags |= 0x4000;
    }

    if (*(U32*)rawData[0] == 'QSPM') {
        flags |= 0x8000;
    }

    if (linkedList) {
        afile->Next = *linkedList;
        *linkedList = afile;
    } else {
        afile->Next = NULL;
    }

    afile->RawData = (void**)(afile + 1);
    for (i = 0; i < (S32)(numX * numY); i++)
        afile->RawData[i] = rawData[i];

    afile->Name = name;
    afile->ID = hashed_name_id;
    afile->FileFlags = flags;
    afile->Duration = (afile->FileFlags & AnimFile_BackAndForth) ?
        2.0f * xAnimFileRawDuration(afile) :
        xAnimFileRawDuration(afile);
    afile->TimeOffset = 0.0f;
    afile->BoneCount = (flags & 0x8000) ? 0 : iAnimBoneCount(rawData[0]);
    afile->NumAnims[0] = numX;
    afile->NumAnims[1] = numY;
    afile->PhysicsData = NULL;

    return afile;
}

xAnimFile* xAnimFileNew(void* rawData, const char* name, U32 flags, xAnimFile** linkedList)
{
    void* temp = &rawData;
    return xAnimFileNewBilinear((void**)temp, name, xStrHash(name), flags, linkedList, 1, 1);
}

void xAnimFileSetTime(xAnimFile* data, F32 duration, F32 timeOffset)
{
    F32 rawDuration = xAnimFileRawDuration(data);
    if (timeOffset > rawDuration - 0.1f) {
        timeOffset = rawDuration - 0.1f;
    }

    data->TimeOffset = timeOffset;
    rawDuration -= timeOffset;
    if (duration > rawDuration)
        duration = rawDuration;
    data->Duration = (data->FileFlags & AnimFile_BackAndForth) ? 2.0f * duration : duration;
}

void xAnimFileEval(xAnimFile* data, F32 time, F32* bilinear, U32 flags, xVec3* tran, xQuat* quat, F32*)
{
    S32 i;
    U32 numBones;

    U32 fileflags = data->FileFlags;
    time = CLAMP(time, 0.0f, data->Duration);
    F32 evalTime = xAnimFileRawTime(data, time);

    if (data->FileFlags & 0x8000) {
        return;
    }

    numBones = (flags & 0x1) ? 1 : data->BoneCount;
    if (flags & 0x2) numBones--;
    if (numBones == 0) {
        return;
    }

    if (bilinear && (fileflags & 0x4000)) {
        F32 bilerp[2], lerp, tmpf;
        U32 biindex[2], biplus[2];
        for (i = 0; i < 2; i++) {
            lerp = CLAMP(bilinear[i], 0.0f, data->NumAnims[i] - 1);
            tmpf = std::floorf(lerp);
            bilerp[i] = lerp - tmpf;
            biindex[i] = tmpf;
            biplus[i] = (biindex[i] + 1 < data->NumAnims[i]) ? biindex[i] + 1 : biindex[i];
        }

        xQuat* q0 = (xQuat*)(g_iAnim_workBuffer + 0x1560);
        xVec3* t0 = (xVec3*)(q0 + 65);
        if (bilerp[0] && bilerp[1]) {
            xQuat* q1 = (xQuat*)(g_iAnim_workBuffer + 0x1c80);
            xVec3* t1 = (xVec3*)(q1 + 65);
            iAnimEval(data->RawData[biindex[0] + biindex[1] * data->NumAnims[0]], evalTime, flags, tran, quat);
            iAnimEval(data->RawData[biplus[0] + biindex[1] * data->NumAnims[0]], evalTime, flags, t0, q0);
            iAnimBlend(bilerp[0], 1.0f, NULL, NULL, numBones, tran, quat, t0, q0, tran, quat);

            iAnimEval(data->RawData[biindex[0] + biplus[1] * data->NumAnims[0]], evalTime, flags, t0, q0);
            iAnimEval(data->RawData[biplus[0] + biplus[1] * data->NumAnims[0]], evalTime, flags, t1, q1);
            iAnimBlend(bilerp[0], 1.0f, NULL, NULL, numBones, t0, q0, t1, q1, t0, q0);

            iAnimBlend(bilerp[1], 1.0f, NULL, NULL, numBones, tran, quat, t0, q0, tran, quat);
        } else if (bilerp[0]) {
            iAnimEval(data->RawData[biindex[0] + biindex[1] * data->NumAnims[0]], evalTime, flags, tran, quat);
            iAnimEval(data->RawData[biplus[0] + biindex[1] * data->NumAnims[0]], evalTime, flags, t0, q0);
            iAnimBlend(bilerp[0], 1.0f, NULL, NULL, numBones, tran, quat, t0, q0, tran, quat);
        } else if (bilerp[1]) {
            iAnimEval(data->RawData[biindex[0] + biindex[1] * data->NumAnims[0]], evalTime, flags, tran, quat);
            iAnimEval(data->RawData[biindex[0] + biplus[1] * data->NumAnims[0]], evalTime, flags, t0, q0);
            iAnimBlend(bilerp[1], 1.0f, NULL, NULL, numBones, tran, quat, t0, q0, tran, quat);
        } else {
            iAnimEval(data->RawData[biindex[0] + biindex[1] * data->NumAnims[0]], evalTime, flags, tran, quat);
        }
    } else {
        iAnimEval(data->RawData[0], evalTime, flags, tran, quat);
    }
}

xAnimEffect* xAnimStateNewEffect(xAnimState* state, U16 flags, U16 probability, F32 startTime, F32 endTime, xAnimEffectCallback callback, U32 userDataSize)
{
    xAnimEffect *curr, **prev, *effect = (xAnimEffect*)xANIMALLOC(userDataSize + sizeof(xAnimEffect));
    effect->Flags = flags;
    effect->Probability = probability;
    effect->StartTime = startTime;
    effect->EndTime = endTime;
    effect->Callback = callback;

    prev = &state->Effects;
    curr = state->Effects;
    while (curr && startTime > curr->StartTime) {
        prev = &curr->Next;
        curr = curr->Next;
    }
    effect->Next = curr;
    *prev = effect;

    return effect;
}

xAnimTable* xAnimTableNew(const char* name, U32 userFlags)
{
    xAnimTable* table = (xAnimTable*)xMEMALLOC(sizeof(xAnimTable), 0);
    table->Name = name;
    table->TransitionList = NULL;
    table->StateList = NULL;
    table->AnimIndex = 0;
    table->MorphIndex = 0;
    table->UserFlags = userFlags;
    return table;
}

void xAnimDefaultBeforeEnter(xAnimPlay*, xAnimState* state, void*)
{
    if (state->MultiFile) {
        U32 entry = rand() % state->MultiFile->Count;
        state->Data = state->MultiFile->Files[entry].File;
    }
}

xAnimState* xAnimTableNewState(xAnimTable* table, const char* name, U32 flags, U32 userFlags, F32 speed, F32* boneBlend, F32* timeSnap, F32 fadeRecip, U16* fadeOffset, void* callbackData, xAnimBeforeEnterCallback beforeEnter, xAnimStateCallback stateCallback, xAnimBeforeAnimMatricesCallback beforeAnimMatrices)
{
    xAnimState* state = (xAnimState*)xANIMALLOC(sizeof(xAnimState));
    if (table->StateList == NULL) {
        state->Next = NULL;
        table->StateList = state;
    } else {
        state->Next = table->StateList->Next;
        table->StateList->Next = state;
    }

    state->Name = name;
    state->ID = xStrHash(name);
    state->Flags = flags;
    state->UserFlags = userFlags;
    state->Speed = speed;
    state->Data = NULL;
    state->Effects = NULL;
    state->Default = NULL;
    state->List = NULL;
    state->BoneBlend = boneBlend;
    state->TimeSnap = timeSnap;
    state->FadeRecip = fadeOffset ? CalcRecipBlendMax(fadeOffset) : fadeRecip;
    state->FadeOffset = fadeOffset;

    state->MultiFile = NULL;

    state->CallbackData = callbackData;
    state->BeforeEnter = beforeEnter;
    state->StateCallback = stateCallback;
    state->BeforeAnimMatrices = beforeAnimMatrices;

    return state;
}

void xAnimTableNewStateMany(xAnimTable* table, const char* name, S32 howMany, U32 flags, U32 userFlags, F32 speed, F32* boneBlend, F32* timeSnap, F32 fadeRecip, U16* fadeOffset, void* callbackData, xAnimBeforeEnterCallback beforeEnter, xAnimStateCallback stateCallback, xAnimBeforeAnimMatricesCallback beforeAnimMatrices)
{
    size_t state_name_len = strlen(name);
    size_t alignedLength = (state_name_len + 4) & ~3;
    for (S32 i = 0; i < howMany; i++) {
        xAnimTableNewState(table, name, flags, userFlags, speed, boneBlend, timeSnap, fadeRecip, fadeOffset, callbackData, beforeEnter, stateCallback, beforeAnimMatrices);

        if (i == howMany - 1) {
            break;
        }

        char* duplicate = (char*)xANIMALLOC(alignedLength);
        strcpy(duplicate, name);

        char* digit = &duplicate[state_name_len - 1];
        while (digit >= duplicate && isdigit(*digit)) {
            (*digit)++;
            if (isdigit(*digit))
                break;
            *digit = '0';
            digit--;
        }

        name = duplicate;
    }
}

static void _xAnimTableAddTransitionHelper(xAnimState* state, xAnimTransition* tran, U32& allocCount, U32& stateCount, xAnimState** stateList)
{
    U32 flags = tran->Flags;

    if (flags & 0x10) {
        if (!DefaultHasTransition(state, tran, &allocCount)) {
            stateList[stateCount] = state;
            stateCount++;
        }
    } else {
        if (!StateHasTransition(state, tran)) {
            stateList[stateCount] = state;
            stateCount++;
            allocCount++;
        }
    }
}

static void _xAnimTableAddTransition(xAnimTable* table, xAnimTransition* tran, const char* source, const char* dest)
{
    U8* buffer;
    xAnimState **stateList, *state;
    U32 i, stateCount, allocCount = 0;
    const char* stateName;
    xAnimTransitionList* tlist = NULL;
    xAnimTransition* substTransitionList[MAX_SUBST_TRANSITION];
    U32 substTransitionCount = 0;

    buffer = (U8*)xMemScratchPush(0x4000, 0);

    stateCount = 0;
    stateList = (xAnimState**)(buffer + 0x400);
    bool hasSubst = false;

    if (dest) {
        for (S32 i = 0; dest[i]; i++)
            if (dest[i] == '@' || dest[i] == '~') {
                hasSubst = true;
                break;
            }
    }

    stateName = xStrTokBuffer(source, " ,\t\n\r", buffer);
    while (stateName) {
        bool isComplex = dest != NULL;
        if (!isComplex) {
            const char* COMPLEX_PATTERNS = "#+*?{}()<>|;";
            for (const char* search = stateName; *search; search++) {
                if (_xCharIn(*search, COMPLEX_PATTERNS)) {
                    isComplex = true;
                    break;
                }
            }
        }

        if (isComplex) {
            xAnimState* state = table->StateList;
            while (state) {
                char extra[128];
                if (_xCheckAnimName(state->Name, stateName, extra)) {
                    if (hasSubst) {
                        char tempName[128];
                        char* tempIterator = tempName;
                        char* extraIterator = extra;
                        bool allowMissingState = false;
                        for (S32 i = 0; dest[i]; i++)
                            if (dest[i] == '@' || dest[i] == '~') {
                                allowMissingState = dest[i] == '~';
                                size_t extraIteratorLength = strlen(extraIterator);
                                strcpy(tempIterator, extraIterator);
                                tempIterator += extraIteratorLength;
                                extraIterator += extraIteratorLength + 1;
                            } else
                                *tempIterator++ = dest[i];
                        *tempIterator = '\0';

                        xAnimState* tempState = xAnimTableGetState(table, tempName);
                        if (allowMissingState && tempState == NULL)
                            goto next;

                        if (substTransitionCount) {
                            xAnimTransition* duplicatedTransition = (xAnimTransition*)xANIMALLOC(sizeof(xAnimTransition));
                            memcpy(duplicatedTransition, tran, sizeof(xAnimTransition));
                            tran = duplicatedTransition;
                        }
                        substTransitionList[substTransitionCount++] = tran;

                        tran->Dest = tempState;
                    }

                    if (tran->Dest != state || (state->Flags & 0x70) == 0x40)
                        _xAnimTableAddTransitionHelper(state, tran, allocCount, stateCount, stateList);
                }
            next:
                state = state->Next;
            }
        } else {
            state = xAnimTableGetState(table, stateName);
            if (state) {
                if (tran->Dest != state || (state->Flags & 0x70) == 0x40) {
                    _xAnimTableAddTransitionHelper(state, tran, allocCount, stateCount, stateList);
                }
            }
        }

        stateName = xStrTokBuffer(NULL, " ,\t\n\r", buffer);
    }

    if (allocCount) {
        tlist = (xAnimTransitionList*)xANIMALLOC(allocCount * sizeof(xAnimTransitionList));
    }

    if (tran->Flags & 0x10) {
        for (i = 0; i < stateCount; i++) {
            xAnimTransition* currentTransition = hasSubst ? substTransitionList[i] : tran;
            if (!DefaultOverride(stateList[i], currentTransition)) {
                tlist->T = currentTransition;
                _xAnimTransitionListInsert(&stateList[i]->Default, tlist);
                tlist++;
            }
        }
    } else {
        for (i = 0; i < stateCount; i++) {
            tlist->T = hasSubst ? substTransitionList[i] : tran;
            _xAnimTransitionListInsert(&stateList[i]->List, tlist);
            tlist++;
        }
    }

    xMemScratchPop(buffer);
}

xAnimTransition* xAnimTableNewTransition(xAnimTable* table, const char* source, const char* dest, xAnimTransitionCallback conditional, xAnimTransitionCallback callback, U32 flags, U32 userFlags, F32 srcTime, F32 destTime, U16 priority, U16 queuePriority, F32 fBlendTime, U16* blendOffset)
{
    xAnimTransition* tran = (xAnimTransition*)xANIMALLOC(sizeof(xAnimTransition));
    F32 fOOBlendTime = (fBlendTime == 0.0f) ? 0.0f : 1.0f / fBlendTime;
    tran->Next = table->TransitionList;
    table->TransitionList = tran;

    bool isComplex = false;
    if (dest == NULL || *dest == '\0') {
        tran->Dest = NULL;
    } else {
        for (S32 i = 0; dest[i]; i++) {
            if (dest[i] == '@' || dest[i] == '~') {
                isComplex = true;
                break;
            }
        }

        tran->Dest = isComplex ? NULL : xAnimTableGetState(table, dest);
    }
    tran->Conditional = conditional;
    tran->Callback = callback;
    tran->Flags = flags;
    tran->UserFlags = userFlags;
    tran->SrcTime = srcTime;
    tran->DestTime = destTime;
    tran->Priority = priority;
    tran->QueuePriority = queuePriority;
    tran->BlendRecip = blendOffset ? CalcRecipBlendMax(blendOffset) : fOOBlendTime;
    tran->BlendOffset = blendOffset;

    _xAnimTableAddTransition(table, tran, source, isComplex ? dest : NULL);
    return tran;
}

void xAnimTableAddFile(xAnimTable* table, xAnimFile* file, const char* states)
{
    U8* buffer = (U8*)xMemScratchPush(0x4000, 0);

    const char* stateName = xStrTokBuffer(states, " ,\t\n\r", buffer);
    while (stateName) {
        xAnimTableAddFileID(table, file, xStrHash(stateName), 0, 0);
        stateName = xStrTokBuffer(NULL, " ,\t\n\r", buffer);
    }
    xMemScratchPop(buffer);
}

xAnimState* xAnimTableAddFileID(xAnimTable* table, xAnimFile* file, U32 stateID, U32 subStateID, U32 subStateCount)
{
    xAnimState* state = xAnimTableGetStateID(table, stateID);
    if (state) {
        bool setFile = true;
        if (subStateID) {
            if (state->MultiFile == NULL) {
                state->MultiFile = (xAnimMultiFile*)xANIMALLOC(subStateCount * sizeof(xAnimMultiFileEntry) + sizeof(xAnimMultiFileBase));
                state->MultiFile->Count = 0;
            }

            U32 current = state->MultiFile->Count;
            state->MultiFile->Files[current].ID = subStateID;
            state->MultiFile->Files[current].File = file;
            state->MultiFile->Count++;
        }

        if (setFile) {
            state->Data = file;
        }

        if (file->FileFlags & 0x8000) {
            table->MorphIndex |= 1 << (state->Flags & 0xF);
        } else {
            table->AnimIndex |= 1 << (state->Flags & 0xF);
        }
    }
    return state;
}

xAnimState* xAnimTableGetStateID(xAnimTable* table, U32 ID)
{
    xAnimState* curr = table->StateList;
    while (curr) {
        if (curr->ID == ID)
            return curr;
        curr = curr->Next;
    }
    return NULL;
}

xAnimState* xAnimTableGetState(xAnimTable* table, const char* name)
{
    return xAnimTableGetStateID(table, xStrHash(name));
}

static void EffectActiveInsert(xAnimSingle* single, xAnimActiveEffect* active)
{
    U32 index = 0, count = single->ActiveCount;
    xAnimActiveEffect* alist = single->ActiveList;

    while (index < count && alist->Effect) { alist++; index++; }

    *alist = *active;
    if (index < count - 1) alist[1].Effect = NULL;
}

static void EffectActiveRemove(xAnimActiveEffect* active, U32 index, U32 count)
{
    if (index == count - 1 || active[1].Effect == NULL) {
        active->Effect = NULL;
        return;
    }

    xAnimActiveEffect* alist = active;
    while (index < count - 1 && alist[1].Effect) { alist++; index++; }

    *active = *alist;
    alist->Effect = NULL;
}

static U32 EffectPlaying(xAnimSingle* single, xAnimEffect* effect)
{
    for (U32 i = 0; i < single->ActiveCount && single->ActiveList[i].Effect; i++)
        if (single->ActiveList[i].Effect == effect)
            return TRUE;
    return FALSE;
}

static void EffectSingleStart(xAnimSingle* single)
{
    xAnimEffect* effect = single->State->Effects;
    xAnimActiveEffect tempActive;
    F32 time = single->Time;

    while (effect && effect->StartTime < time) {
        U32 flags = effect->Flags;
        if (flags & 0x1) {
            if (flags & 0x4) {
                tempActive.Effect = effect;
                tempActive.Handle = effect->Callback(1, &tempActive, single, single->Play->Object);
                if (effect->Flags & 0x2) {
                    if (effect->EndTime <= time) {
                        effect->Callback(3, &tempActive, single, single->Play->Object);
                    } else {
                        EffectActiveInsert(single, &tempActive);
                    }
                } else if (effect->Flags & 0x20) {
                    EffectActiveInsert(single, &tempActive);
                }
            } else if ((flags & 0x2) && !(flags & 0x10) && effect->EndTime > time) {
                tempActive.Effect = effect;
                tempActive.Handle = effect->Callback(1, &tempActive, single, single->Play->Object);
                EffectActiveInsert(single, &tempActive);
            }
        }
        effect = effect->Next;
    }

    single->Effect = effect;
}

void EffectSingleStart_ForceEffect(xAnimSingle* single, xAnimEffect* force_effect)
{
    xAnimEffect* effect = force_effect;
    xAnimActiveEffect tempActive;
    F32 time = single->Time;

    while (effect && effect->StartTime < time) {
        U32 flags = effect->Flags;
        if (flags & 0x1) {
            if (flags & 0x4) {
                tempActive.Effect = effect;
                tempActive.Handle = effect->Callback(1, &tempActive, single, single->Play->Object);
                if (effect->Flags & 0x2) {
                    if (effect->EndTime <= time) {
                        effect->Callback(3, &tempActive, single, single->Play->Object);
                    } else {
                        EffectActiveInsert(single, &tempActive);
                    }
                } else if (effect->Flags & 0x20) {
                    EffectActiveInsert(single, &tempActive);
                }
            } else if ((flags & 0x2) && !(flags & 0x10) && effect->EndTime > time) {
                tempActive.Effect = effect;
                tempActive.Handle = effect->Callback(1, &tempActive, single, single->Play->Object);
                EffectActiveInsert(single, &tempActive);
            }
        }
        effect = effect->Next;
    }

    single->Effect = effect;
}

static void EffectSingleDuration(xAnimSingle* single)
{
    F32 time = single->Time;
    xAnimActiveEffect* alist = single->ActiveList;
    U32 index = 0, count = single->ActiveCount;

    while (index < count && alist->Effect) {
        if (!(alist->Effect->Flags & 0x20) && alist->Effect->EndTime <= time) {
            alist->Effect->Callback(3, alist, single, single->Play->Object);
            EffectActiveRemove(alist, index, count);
        } else {
            alist->Effect->Callback(2, alist, single, single->Play->Object);
            alist++;
            index++;
        }
    }
}

static void EffectSingleRun(xAnimSingle* single)
{
    xAnimEffect* effect = single->Effect;
    xAnimActiveEffect tempActive;
    F32 time = single->Time;

    while (effect && effect->StartTime <= time) {
        U32 flags = effect->Flags;
        if (flags & 0x1) {
            if (!(flags & 0x20) || !EffectPlaying(single, effect)) {
                tempActive.Effect = effect;
                tempActive.Handle = effect->Callback(1, &tempActive, single, single->Play->Object);
                if (flags & 0x2) {
                    if (effect->EndTime <= time) {
                        effect->Callback(3, &tempActive, single, single->Play->Object);
                    } else {
                        EffectActiveInsert(single, &tempActive);
                    }
                } else if (flags & 0x20) {
                    EffectActiveInsert(single, &tempActive);
                }
            }
        }
        effect = effect->Next;
    }
    single->Effect = effect;
}

static void EffectSingleLoop(xAnimSingle* single)
{
    EffectSingleRun(single);

    xAnimActiveEffect* alist = single->ActiveList;
    U32 index = 0, count = single->ActiveCount;

    while (index < count && alist->Effect) {
        if (!(alist->Effect->Flags & 0x20)) {
            alist->Effect->Callback(3, alist, single, single->Play->Object);
            EffectActiveRemove(alist, index, count);
        } else {
            alist++;
            index++;
        }
    }

    xAnimEffect* effect = single->State->Effects;
    while (effect && effect->StartTime < 0.0f) {
        effect = effect->Next;
    }
    single->Effect = effect;
}

static void EffectSingleStop(xAnimSingle* single)
{
    if (single->State == NULL || single->LastTime == -1.0f) {
        return;
    }

    for (U32 i = 0; i < single->ActiveCount && single->ActiveList[i].Effect; i++) {
        if (single->ActiveList[i].Effect->Flags & 0x22)
            single->ActiveList[i].Effect->Callback(3, &single->ActiveList[i], single, single->Play->Object);
    }
    single->ActiveList->Effect = NULL;

    xAnimEffect* effect = single->Effect;
    xAnimActiveEffect tempActive;
    while (effect) {
        if ((effect->Flags & 0x9) == 0x9) {
            tempActive.Effect = effect;
            tempActive.Handle = effect->Callback(1, &tempActive, single, single->Play->Object);
            if (effect->Flags & 0x2) {
                effect->Callback(3, &tempActive, single, single->Play->Object);
            }
        }
        effect = effect->Next;
    }
    single->Effect = NULL;
}

void xAnimPhysicsEval(xAnimPhysicsData* phys, F32 time, xVec3* disp, F32* yaw)
{
    if (phys) {
        if (time < 0.0f) {
            time = 0.0f;
        }

        F32 interp = 30.0f * time;
        S32 index = interp;

        if (index >= phys->tranCount - 1) {
            index = phys->tranCount - 1;
            xVec3Copy(disp, &phys->tranTable[index]);

            if (phys->yawTable) {
                *yaw = phys->yawTable[index];
            } else {
                *yaw = 0.0f;
            }

            return;
        }

        interp -= index;

        xVec3SMul(disp, &phys->tranTable[index + 1], interp);
        xVec3AddScaled(disp, &phys->tranTable[index], 1.0f - interp);

        if (phys->yawTable) {
            *yaw = interp * (phys->yawTable[index + 1] - phys->yawTable[index]) + phys->yawTable[index];
        } else
            *yaw = 0.0f;
    }
}

static void StopUpdate(xAnimSingle* single)
{
    F32 duration = single->State->Data->Duration;
    if (single->Time > duration) {
        single->Time = duration;
        single->CurrentSpeed = 0.0f;
    }
}

static void LoopUpdate(xAnimSingle* single)
{
    F32 duration = single->State->Data->Duration;

    if (single->Time > duration) {
        F32 remainDelta = single->Time - duration;

        single->Time = duration;
        EffectSingleLoop(single);

        if (single->State->Data->FileFlags & 0xF0000) {
            xVec3 disp;
            F32 yaw;
            xAnimPhysicsEval(single->State->Data->PhysicsData, single->Time, &disp, &yaw);
            xVec3AddTo(&single->PhysDisp, &disp);

            if (yaw != 0.0f) {
                xMat3x3 rot;
                xMat3x3RotY(&rot, -yaw);
                xMat3x3RMulVec(&single->PhysDisp, &rot, &single->PhysDisp);
            }

            single->YawDisp += yaw;
            single->SingleFlags |= 0x4;
        }

        single->Time = remainDelta;
    }
}

void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, F32 startTime)
{
    EffectSingleStop(single);
    if (single->Blend) {
        EffectSingleStop(single->Blend);
        SetState(single->Blend->State, NULL);
    }

    SetState(single->State, state);
    if (state == NULL) {
        return;
    }

    single->Time = (state->Flags & 0x100) && startTime == 0.0f ? state->Data->Duration * xurand() : startTime;
    single->CurrentSpeed = state->Speed;
    single->BilinearLerp[0] = 0.0f;
    single->BilinearLerp[1] = 0.0f;
    single->Effect = NULL;
    memset(single->ActiveList, 0, single->ActiveCount * sizeof(xAnimActiveEffect));
    single->LastTime = -1.0f;
    SetTransition(single->Sync, NULL);
    SetTransition(single->Tran, NULL);
    single->BlendFactor = 0.0f;
}

static void SingleUpdate(xAnimSingle* single, F32 timeDelta)
{
    U32 foundBlendstop = FALSE, firstStep = TRUE;
    xAnimTransition* foundTransition = NULL;
    xAnimSingle* bl = NULL;
    F32 tranDelta = 0.0f, blendDelta = 0.0f, singleTime;

    xVec3Init(&single->PhysDisp, 0.0f, 0.0f, 0.0f);
    single->YawDisp = 0.0f;
    single->SingleFlags &= ~0x4;

    if (single->State == NULL) {
        return;
    }

    void* object = single->Play->Object;

    if (single->LastTime == -1.0f) {
        EffectSingleStart(single);
    }

    single->LastTime = single->Time;

    singleTime = timeDelta * single->CurrentSpeed + single->Time;
    if (single->State->Data->FileFlags & 0xF0000) {
        xAnimPhysicsEval(single->State->Data->PhysicsData, single->LastTime, &single->PhysDisp, &single->YawDisp);

        xVec3Inv(&single->PhysDisp, &single->PhysDisp);
        single->YawDisp = -single->YawDisp;
    }

    if (single->Blend && single->Blend->State) {
        bl = single->Blend;
        if (bl->LastTime == -1.0f) {
            EffectSingleStart(bl);
        }

        bl->LastTime = bl->Time;
    }

    F32 duration = single->State->Data->Duration;
    if (single->Sync) {
        if (single->Sync->SrcTime != 0.0f) {
            F32 timeCmp = single->Sync->SrcTime;
            if (timeCmp > duration) timeCmp = duration;
            if (single->LastTime <= timeCmp && singleTime >= timeCmp) {
                tranDelta = (singleTime - timeCmp) / single->CurrentSpeed;
                foundTransition = single->Sync;
                if ((timeDelta -= tranDelta) < 0.0f) timeDelta = 0.0f;
            } else if (single->LastTime <= timeCmp + duration && singleTime >= timeCmp + duration) {
                tranDelta = (singleTime - (timeCmp + duration)) / single->CurrentSpeed;
                foundTransition = single->Sync;
                if ((timeDelta -= tranDelta) < 0.0f) {
                    timeDelta = 0.0f;
                }
            }
        } else {
            if (bl == NULL) {
                tranDelta = timeDelta; foundTransition = single->Sync;
                timeDelta = 0.0f;
            }
        }
    } else {
        if ((single->State->Flags & 0x70) == 0x20 || (single->State->Flags & 0x70) == 0x40) {
            F32 timeCmp = single->State->Default->T->SrcTime;
            if (timeCmp == 0.0f || timeCmp > duration) timeCmp = duration;

            if (singleTime >= timeCmp && (!(single->State->Default->T->Flags & 0x4) || bl == NULL)) {
                xAnimTransitionList* curr = single->State->Default;
                while (curr && curr->T->Conditional && !curr->T->Conditional(curr->T, single, object))
                    curr = curr->Next;

                if (curr == NULL) {
                    fprintf(stderr, "State \"%s\" no default conditionals true!\n", single->State->Name);
                    curr = single->State->Default;
                }

                foundTransition = curr->T;
                if (single->LastTime < timeCmp) {
                    tranDelta = (singleTime - timeCmp) / single->CurrentSpeed;
                    if ((timeDelta -= tranDelta) < 0.0f) timeDelta = 0.0f;
                } else {
                    tranDelta = timeDelta;
                    timeDelta = 0.0f;
                }
            }
        }
    }

    if (single->BlendFactor != 0.0f) {
        F32 recip;
        if (single->Tran)
            recip = single->Tran->BlendRecip;
        else
            recip = single->State->FadeRecip;

        if (recip * (single->BlendFactor + timeDelta) > 1.0f) {
            foundBlendstop = TRUE;
            blendDelta = single->BlendFactor + timeDelta - 1.0f / recip;
            if ((timeDelta -= blendDelta) < 0.0f) timeDelta = 0.0f;
            if (blendDelta < 0.0f) {
                blendDelta = 0.01f;
            }
        }
    }

    do {
        if (!firstStep) {
            if (foundBlendstop) {
                single->BlendFactor = 0.0f;
                if (single->Tran->Flags & 0x8) {
                    single->CurrentSpeed = single->State->Speed;
                }

                if (bl) {
                    EffectSingleStop(bl);
                    SetState(bl->State, NULL);
                    bl = NULL;
                    SetTransition(single->Tran, NULL);
                } else {
                    if (single->Tran) {
                        SetTransition(single->Tran, NULL);
                    } else {
                        EffectSingleStop(single);
                        SetState(single->State, NULL);
                        return;
                    }
                }

                timeDelta = blendDelta;
                foundBlendstop = FALSE;
            } else {
                bool transferredState = false;
                if (bl) {
                    EffectSingleStop(bl);
                    SetState(bl->State, NULL);
                    bl = NULL;
                    single->BlendFactor = 0.0f;
                }

                if (foundTransition->BlendRecip == 0.0f || single->Blend == NULL) {
                    EffectSingleStop(single);
                    SetTransition(single->Tran, NULL);
                } else {
                    bl = single->Blend;
                    bl->State = single->State;
                    transferredState = true;
                    bl->Time = single->Time,
                    bl->CurrentSpeed = single->CurrentSpeed,
                    bl->BilinearLerp[0] = single->BilinearLerp[0],
                    bl->BilinearLerp[1] = single->BilinearLerp[1],
                    bl->Effect = single->Effect,
                    bl->LastTime = single->LastTime;

                    memcpy(bl->ActiveList, single->ActiveList, single->ActiveCount * sizeof(xAnimActiveEffect));
                    single->ActiveList->Effect = NULL;

                    SetTransition(single->Tran, foundTransition);
                    single->BlendFactor = 1e-7f;
                }

                if (single->State->Data->FileFlags & 0xF0000) {
                    xVec3 disp;
                    F32 yaw;
                    xAnimPhysicsEval(single->State->Data->PhysicsData, single->Time, &disp, &yaw);
                    xVec3AddTo(&single->PhysDisp, &disp);

                    if (yaw != 0.0f) {
                        xMat3x3 rot;
                        xMat3x3RotY(&rot, -yaw);
                        xMat3x3RMulVec(&single->PhysDisp, &rot, &single->PhysDisp);
                    }

                    single->YawDisp += yaw;
                    single->SingleFlags |= 0x4;
                }

                TransitionTimeInit(single, foundTransition);

                if (transferredState)
                    single->State = NULL;
                SetState(single->State, foundTransition->Dest);
                single->CurrentSpeed = single->State->Speed,
                single->BilinearLerp[0] = 0.0f,
                single->BilinearLerp[1] = 0.0f;
                SetTransition(single->Sync, NULL);

                if (transferredState && (foundTransition->Flags & 0x8)) {
                    single->CurrentSpeed = 0.0f;
                }

                EffectSingleStart(single);

                if (single->State->BeforeEnter) {
                    single->State->BeforeEnter(single->Play, single->State, single->Play->Object);
                }

                if (foundTransition->Callback) {
                    foundTransition->Callback(foundTransition, single, single->Play->Object);
                }

                timeDelta = tranDelta;
                foundTransition = NULL;
            }
        }

        single->Time += timeDelta * single->CurrentSpeed;
        if (single->BlendFactor != 0.0f) {
            single->BlendFactor += timeDelta;
        }

        if ((single->State->Flags & 0x70) == 0x10) {
            LoopUpdate(single);
        } else {
            StopUpdate(single);
        }

        EffectSingleRun(single);

        if (bl) {
            if ((bl->State->Flags & 0x70) == 0x10) {
                LoopUpdate(bl);
            } else {
                StopUpdate(bl);
            }

            EffectSingleRun(bl);
        }

        firstStep = FALSE;
    } while (foundBlendstop || foundTransition);

    if (single->Tran == NULL && single->BlendFactor == 0.0f && (single->State->Flags & 0x70) == 0x30) {
        if (single->State->Flags & 0x200) {
            if (single->Time >= duration)
                single->BlendFactor = 1e-7f;
        } else {
            if (single->State->FadeRecip * (duration - single->Time) < 1.0f) {
                single->BlendFactor = 1e-7f;
            }
        }
    }

    EffectSingleDuration(single);
    if (bl) EffectSingleDuration(bl);

    if (single->State->Data->FileFlags & 0xF0000) {
        xVec3 disp;
        F32 yaw;
        xAnimPhysicsEval(single->State->Data->PhysicsData, single->Time, &disp, &yaw);

        xVec3AddTo(&single->PhysDisp, &disp);

        if (yaw != 0.0f) {
            xMat3x3 rot;
            xMat3x3RotY(&rot, -yaw);
            xMat3x3RMulVec(&single->PhysDisp, &rot, &single->PhysDisp);
        }

        single->YawDisp += yaw;
        single->SingleFlags |= 0x4;
    }
}

static void SingleEval(xAnimSingle* single, xVec3* tran, xQuat* quat)
{
    if (single->State) {
        if (single->Play->BoneCount > 1) {
            tran++;
            quat++;

            xAnimFileEval(single->State->Data, single->Time, single->BilinearLerp, 0x2, tran, quat, NULL);

            if (single->Blend && single->Blend->State) {
                xQuat* blendquat = (xQuat*)(g_iAnim_workBuffer + 0xe40);
                xVec3* blendtran = (xVec3*)(blendquat + 65);
                xAnimFileEval(single->Blend->State->Data, single->Blend->Time, single->Blend->BilinearLerp, 0x2, blendtran, blendquat, NULL);

                iAnimBlend(single->BlendFactor, single->Tran->BlendRecip, single->Tran->BlendOffset, NULL, single->Play->BoneCount - 1, blendtran, blendquat, tran, quat, tran, quat);
            }
        }
    }
}

void xAnimPlaySetup(xAnimPlay* play, void* object, xAnimTable* table, xModelInstance* modelInst)
{
    play->BoneCount = modelInst->BoneCount;
    play->Object = object;
    play->Table = table;
    play->ModelInst = modelInst;

    modelInst->Anim = play;
    modelInst->Flags |= 0x104;

    if (table->MorphIndex) {
        modelInst->Flags |= 0x80;
    }

    for (S32 i = 0; i < play->NumSingle; i++) {
        play->Single[i].SingleFlags = ((1 << i) & table->MorphIndex) ? 0x8000 : 0x1;
        play->Single[i].State = NULL;
        play->Single[i].Tran = NULL;
        if (play->Single[i].Blend)
            SetState(play->Single[i].Blend->State, NULL);
    }

    xAnimPlaySetState(play->Single, table->StateList, 0.0f);
}

void xAnimPlayChooseTransition(xAnimPlay* play)
{
    U32 i;
    void* object = play->Object;
    static U8* tmpBuffer = g_iAnim_workBuffer;
    xAnimTransition** found = (xAnimTransition**)tmpBuffer;
    memset(found, 0, play->NumSingle * sizeof(xAnimTransition*));

    for (i = 0; i < play->NumSingle; i++) {
        if (play->Single[i].State) {
            xAnimTransitionList* curr = play->Single[i].State->List;

            if (curr == NULL) continue;
            if (curr->T->Conditional == NULL) continue;
            while (curr && (play->Single[i].Sync == NULL || curr->T->Priority > play->Single[i].Sync->QueuePriority)) {
                if (curr->T->Conditional(curr->T, &play->Single[i], object)) {
                    U32 index = curr->T->Dest->Flags & 0xF;

                    found[index] = curr->T;
                    break;
                }
                curr = curr->Next;
            }
        }
    }

    for (i = 0; i < play->NumSingle; i++) {
        if (found[i]) {
            xAnimPlayStartTransition(play, found[i]);
        }
    }
}

void xAnimPlayStartTransition(xAnimPlay* play, xAnimTransition* transition)
{
    U32 index = transition->Dest->Flags & 0xF;

    xAnimSingle* single = &play->Single[index];
    xAnimSingle* bl = single->Blend;

    if (transition->SrcTime != 0.0f || ((transition->Flags & 0x4) && bl && bl->State)) {
        SetTransition(single->Sync, transition);
        return;
    }

    if (bl && bl->State) {
        EffectSingleStop(bl);
        SetState(bl->State, NULL);
        single->BlendFactor = 0.0f;
    }

    bool transferredState = false;
    bool tookOwnership = false;
    if (transition->BlendRecip == 0.0f || single->Blend == NULL) {
        EffectSingleStop(single);
        SetTransition(single->Tran, NULL);
    } else {
        if (single->State) {
            bl->State = single->State;
            transferredState = true;
            bl->Time = single->Time,
            bl->CurrentSpeed = single->CurrentSpeed,
            bl->BilinearLerp[0] = single->BilinearLerp[0],
            bl->BilinearLerp[1] = single->BilinearLerp[1],
            bl->Effect = single->Effect,
            bl->LastTime = single->LastTime;

            memcpy(bl->ActiveList, single->ActiveList, single->ActiveCount * sizeof(xAnimActiveEffect));
            single->ActiveList->Effect = NULL;
        }

        SetTransition(single->Tran, transition);
        single->BlendFactor = 1e-7f;

        tookOwnership = true;
    }

    TransitionTimeInit(single, transition);

    if (transferredState)
        single->State = NULL;
    SetState(single->State, transition->Dest);
    single->CurrentSpeed = single->State->Speed,
    single->BilinearLerp[0] = 0.0f,
    single->BilinearLerp[1] = 0.0f;
    SetTransition(single->Sync, NULL);

    if (transferredState && (transition->Flags & 0x8)) {
        single->CurrentSpeed = 0.0f;
    }

    EffectSingleStart(single);

    if (single->State->BeforeEnter) {
        single->State->BeforeEnter(play, single->State, play->Object);
    }

    if (transition->Callback) {
        transition->Callback(transition, single, single->Play->Object);
    }

    if (!tookOwnership)
        SetTransition(transition, NULL);
}

void xAnimPlayUpdate(xAnimPlay* play, F32 timeDelta)
{
    for (U32 i = 0; i < play->NumSingle; i++) {
        xAnimSingle* single = &play->Single[i];

        SingleUpdate(single, timeDelta);

        if (single->State && single->State->StateCallback)
            single->State->StateCallback(single->State, single, play->Object);
    }
}

void xAnimPlayEval(xAnimPlay* play)
{
    if (play->Single->State == NULL) {
        return;
    }

    U32 i, bone;
    xQuat* quatresult = (xQuat*)g_iAnim_workBuffer;
    xVec3* tranresult = (xVec3*)(quatresult + 65);
    if (play->BoneCount > 1) {
        xQuat* quatblend = (xQuat*)(g_iAnim_workBuffer + 0x720);
        xVec3* tranblend = (xVec3*)(quatblend + 65);

        SingleEval(play->Single, tranresult, quatresult);

        for (i = 1; i < play->NumSingle; i++) {
            xAnimSingle* si = &play->Single[i];
            if (si->State && !(si->SingleFlags & 0x8000)) {
                F32 blendF = 1.0f, blendR = 1.0f;
                U16* blendO = NULL;
                SingleEval(si, tranblend, quatblend);

                if ((!si->Blend || !si->Blend->State) && si->BlendFactor) {
                    if (si->Tran) {
                        blendF = si->BlendFactor;
                        blendR = si->Tran->BlendRecip;
                        blendO = si->Tran->BlendOffset;
                    } else {
                        blendF = -si->BlendFactor;
                        blendR = si->State->FadeRecip;
                        blendO = si->State->FadeOffset;
                    }
                }

                if ((si->SingleFlags & 0x3) == 0x2) {
                    iAnimBlend(blendF, blendR, blendO, si->State->BoneBlend, play->BoneCount - 1, tranblend + 1, quatblend + 1, NULL, NULL, tranblend + 1, quatblend + 1);

                    for (bone = 1; bone < play->BoneCount; bone++) {
                        tranresult[bone].x += tranblend[bone].x;
                        tranresult[bone].y += tranblend[bone].y;
                        tranresult[bone].z += tranblend[bone].z;
                        xQuatMul(&quatresult[bone], &quatresult[bone], &quatblend[bone]);
                    }
                } else {
                    iAnimBlend(blendF, blendR, blendO, si->State->BoneBlend, play->BoneCount - 1, tranresult + 1, quatresult + 1, tranblend + 1, quatblend + 1, tranresult + 1, quatresult + 1);
                }
            }
        }
    }

    memset(tranresult, 0, sizeof(xVec3));
    memset(quatresult, 0, sizeof(xQuat));

    if (play->Single->State->BeforeAnimMatrices)
        play->Single->State->BeforeAnimMatrices(play, quatresult, tranresult, play->BoneCount);
    if (play->BeforeAnimMatrices) {
        play->BeforeAnimMatrices(play, quatresult, tranresult, play->BoneCount);
    }

    iModelAnimMatrices(play->ModelInst->Data, quatresult, tranresult, play->ModelInst->Mat + 1);
}

void xAnimPoolCB(xMemPool* pool, void* data)
{
    S32 i;
    xAnimPlay* clone = (xAnimPlay*)pool->Buffer;
    xAnimPlay* play = (xAnimPlay*)data;
    xAnimSingle *clonesingle, *currsingle = (xAnimSingle*)(play + 1);

    play->NumSingle = clone->NumSingle;
    play->Single = currsingle;
    currsingle += clone->NumSingle;

    for (i = 0; i < clone->NumSingle; i++) {
        play->Single[i].Play = play;
        if (clone->Single[i].Blend) {
            play->Single[i].Blend = currsingle;
            currsingle->Blend = NULL;
            currsingle->Play = play;
            currsingle++;
        } else {
            play->Single[i].Blend = NULL;
        }
    }

    xAnimActiveEffect* curract = (xAnimActiveEffect*)currsingle;

    for (i = 0; i < play->NumSingle; i++) {
        currsingle = &play->Single[i];
        clonesingle = &clone->Single[i];

        while (currsingle) {
            currsingle->ActiveCount = clonesingle->ActiveCount;
            if (clonesingle->ActiveCount) {
                currsingle->ActiveList = curract;
                curract += clonesingle->ActiveCount;
            } else {
                currsingle->ActiveList = NULL;
            }

            clonesingle = clonesingle->Blend;
            currsingle = currsingle->Blend;
        }
    }

    play->Pool = pool;
}

#define XBITCOUNT4(x) (((x) & 1) + (((x) >> 1) & 1) + (((x) >> 2) & 1) + (((x) >> 3) & 1))
#define XBITCOUNT16(x) (XBITCOUNT4(x) + XBITCOUNT4(x >> 4) + XBITCOUNT4(x >> 8) + XBITCOUNT4(x >> 12))

void xAnimPoolInit(xMemPool* pool, U32 count, U32 singles, U32 blendFlags, U32 effectMax)
{
    U32 size, i;

    effectMax += effectMax & 1;
    size = sizeof(xAnimPlay) + (singles + XBITCOUNT16(blendFlags & 0xFFFF & ((1 << singles) - 1))) * (sizeof(xAnimSingle) + effectMax * sizeof(xAnimActiveEffect));
    void* buffer = xMEMALLOC(count * size, 0);

    xAnimPlay* play = (xAnimPlay*)buffer;
    play->NumSingle = singles;
    xAnimSingle* currsingle = (xAnimSingle*)(play + 1);
    play->Single = currsingle;
    currsingle += singles;

    for (i = 0; i < singles; i++) {
        if (blendFlags & (1 << i)) {
            play->Single[i].Blend = currsingle;
            currsingle->Blend = NULL;
            currsingle++;
        } else {
            play->Single[i].Blend = NULL;
        }
    }

    xAnimActiveEffect* curract = (xAnimActiveEffect*)currsingle;

    for (i = 0; i < play->NumSingle; i++) {
        currsingle = &play->Single[i];

        while (currsingle) {
            currsingle->ActiveCount = effectMax;
            if (effectMax) {
                currsingle->ActiveList = curract;
                curract += effectMax;
            } else {
                currsingle->ActiveList = NULL;
            }
            currsingle = currsingle->Blend;
        }
    }

    play->Pool = pool;
    xMemPoolSetup(pool, buffer, 0, 1, xAnimPoolCB, size, count, count / 2);
}

xAnimPlay* xAnimPoolAlloc(xMemPool* pool, void* object, xAnimTable* table, xModelInstance* modelInst)
{
    xAnimPlay* play = (xAnimPlay*)xMemPoolAlloc(pool);
    xAnimPlaySetup(play, object, table, modelInst);
    return play;
}

void xAnimPoolFree(xAnimPlay* play)
{
    for (U32 i = 0; i < play->NumSingle; i++) {
        EffectSingleStop(&play->Single[i]);
        if (play->Single[i].Blend) {
            EffectSingleStop(play->Single[i].Blend);
        }
    }

    if (play->Single->Tran && (play->Single->Tran->Flags & 0x2)) {
        SetTransition(play->Single->Tran, NULL);
    }

    xMemPoolFree(play->Pool, play);
}

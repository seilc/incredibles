#include "xCutscene.h"

#include "xCam.h"
#include "xGlobals.h"
#include "xMath.h"
#include "xMath3.h"
#include "xMemMgr.h"
#include "xModel.h"
#include "xModelBucket.h"
#include "xMorph.h"
#include "xRenderBuffer.h"
#include "xShadow.h"
#include "xSndMgr.h"
#include "xstransvc.h"
#include "xString.h"
#include "xSubTitles.h"
#include "xutil.h"
#include "iAnim.h"
#include "iCutscene.h"
#include "iModel.h"
#include "iMorph.h"
#include "iSnd.h"
#include "zCam.h"
#include "zCameraFly.h"
#include "zCutsceneMgr.h"
#include "zGame.h"

#include <string.h>
#include <cmath>

#define MAX_CUTSCENE_TABLES 16
#define FAKE_MAX 8
#define MAX_WEIGHTS 128

namespace {

    void init_cutscene_cam();
    void cutscene_started(xCutscene* csn);
    void cutscene_stopped(xCutscene* csn);

}

xCutscene sActiveCutscene;

static U32 sCutTocCount[MAX_CUTSCENE_TABLES] = { 0 };
static xCutsceneInfo* sCutTocInfo[MAX_CUTSCENE_TABLES] = { 0 };
static S32 sCutTocMax;
static xModelInstance sCutsceneFakeModel[FAKE_MAX];
static xCutsceneModelHack* sCutsceneModelHackBuf;

void xCutscene_InitBegin()
{
    memset(&sActiveCutscene, 0, sizeof(xCutscene));
    memset(sCutTocCount, 0, sizeof(sCutTocCount));
    memset(sCutTocInfo, 0, sizeof(sCutTocInfo));
    sCutTocMax = 0;
}

void xCutscene_InitTOC(void* toc)
{
    if (!toc) return;

    sCutTocCount[sCutTocMax] = *(U32*)toc;
    sCutTocInfo[sCutTocMax] = (xCutsceneInfo*)((U32*)toc + 1);
    sCutTocMax++;
    if (sCutTocMax == MAX_CUTSCENE_TABLES) {
        sCutTocMax--;
    }
}

void xCutscene_InitEnd()
{
    for (S32 i = 0; i < FAKE_MAX; i++) {
        sCutsceneFakeModel[i].Mat = (RwMatrix*)xMEMALLOC((IANIM_MAX_BONES + 1) * sizeof(RwMatrix), 0);
        sCutsceneFakeModel[i].Bucket = (xModelBucket**)xMEMALLOC(2 * sizeof(xModelBucket*), 0);
        sCutsceneFakeModel[i].Bucket[0] = (xModelBucket*)xMEMALLOC(sizeof(xModelBucket), 0);
        sCutsceneFakeModel[i].Bucket[1] = sCutsceneFakeModel[i].Bucket[0];
        sCutsceneFakeModel[i].RedMultiplier = 1.0f;
        sCutsceneFakeModel[i].GreenMultiplier = 1.0f;
        sCutsceneFakeModel[i].BlueMultiplier = 1.0f;
        sCutsceneFakeModel[i].Alpha = 1.0f;
    }

    S32 maxModels = 0;
    for (S32 tocnum = 0; tocnum < sCutTocMax; tocnum++) {
        xCutsceneInfo* cnfo = sCutTocInfo[tocnum];
        for (U32 cnum = 0; cnum < sCutTocCount[tocnum]; cnum++) {
            maxModels = MAX(maxModels, (S32)cnfo->NumData);
            cnfo = (xCutsceneInfo*)((U8*)cnfo + cnfo->HeaderSize);
        }
    }

    if (maxModels) {
        sCutsceneModelHackBuf = (xCutsceneModelHack*)xMEMALLOC(maxModels * sizeof(xCutsceneModelHack), 0);
    }

    init_cutscene_cam();
}

xCutscene* xCutscene_Create(U32 csnID, U32 subtitlesID)
{
    xCutscene* csn = &sActiveCutscene;
    memset(csn, 0, sizeof(xCutscene));
    csn->PlaybackSpeed = 1.0f;

    for (S32 tocnum = 0; tocnum < sCutTocMax; tocnum++) {
        xCutsceneInfo* cnfo = sCutTocInfo[tocnum];
        for (U32 i = 0; i < sCutTocCount[tocnum]; i++) {
            if (cnfo->AssetID == csnID) {
                U32 maxload = cnfo->MaxBufEven + cnfo->MaxBufOdd;
                if (cnfo->MaxModel > maxload) {
                    maxload = cnfo->MaxModel;
                }

                csn->RawBuf = RwMalloc(maxload + 60, 0);

                csn->AlignBuf = csn->RawBuf;

                while ((U32)csn->AlignBuf & 63)
                    csn->AlignBuf = (U8*)csn->AlignBuf + 4;

                csn->Info = cnfo;
                csn->SubTitlesID = subtitlesID;
                csn->Data = (xCutsceneData*)(cnfo + 1);
                csn->TimeChunkOffs = (U32*)(csn->Data + cnfo->NumData);
                csn->Visibility = csn->TimeChunkOffs + (cnfo->NumTime + 1);
                csn->BreakList = (xCutsceneBreak*)(csn->Visibility + cnfo->VisSize);
                csn->Play = (xCutsceneTime*)csn->AlignBuf;
                csn->Stream = (xCutsceneTime*)((U8*)csn->AlignBuf + cnfo->MaxBufEven);
                csn->started = false;
                csn->ModelHack = sCutsceneModelHackBuf;
                memset(csn->ModelHack, 0, cnfo->NumData * sizeof(xCutsceneModelHack));

                return csn;
            }
            cnfo = (xCutsceneInfo*)((U8*)cnfo + cnfo->HeaderSize);
        }
    }
    return NULL;
}

S32 xCutscene_Destroy(xCutscene* csn)
{
    if (csn->started) {
        cutscene_stopped(csn);
    }

    csn->Ready = 0;

    iSndSetExternalCallback(NULL);

    if (csn->SndStarted) {
        xSndMgrStop(csn->SndHandle[0]);
        if (csn->SndNumChannel == 2) {
            xSndMgrStop(csn->SndHandle[1]);
        }
        csn->SndStarted = 0;
    }

    xSndMgrCutscenePause(-3, false, true);

    xSndMgrUpdate();

    if (csn->Opened) {
        iCSFileClose(csn);
    }

    for (U32 i = 0; i < csn->Info->NumData; i++) {
        if (csn->Data[i].DataType & 0x80000000) {
            if (!csn->Data[i].DataPtr) {
                continue;
            }
            if ((csn->Data[i].DataType & 0x0FFFFFFF) == 6) {
                RwFree(csn->Data[i].DataPtr);
            } else {
                iModelUnload((RpAtomic*)csn->Data[i].DataPtr);
            }
            csn->Data[i].DataType &= 0x0FFFFFFF;
        }
    }

    RwFree(csn->RawBuf);

    memset(csn, 0, sizeof(xCutscene));

    return 1;
}

S32 xCutscene_LoadStart(xCutscene* csn)
{
    if (!iCSFileOpen(csn)) {
        return 0;
    }
    csn->Opened = 1;

    return 1;
}

static F32 xCutsceneConvertBreak(F32 time, xCutsceneBreak* breaklist, U32 breakcount, S32 idx)
{
    for (U32 i = 0; i < breakcount; i++) {
        if (idx == breaklist[i].Index) {
            F32 tdiff = breaklist[i].Time - time;
            if (tdiff > 0.0f && tdiff < 0.033333335f) {
                return breaklist[i].Time - 0.033333335f;
            }
        }
    }
    return time;
}

S32 xCutscene_Update(xCutscene* csn, F32 dt)
{
    if (csn->Ready && !csn->started) {
        cutscene_started(csn);
    }

    if (!csn->SndStarted) {
        if (csn->SndNumChannel) {
            xSndMgrStartStereo(csn->SndHandle[0], csn->SndHandle[1]);
            csn->SndStarted = 1;
        }
    }

    csn->Time += csn->PlaybackSpeed * dt;

    if (gSoak && csn->Time > 2.0f) {
        csn->PlayIndex = csn->Info->NumTime - 1;
    }

    csn->CamTime = xCutsceneConvertBreak(csn->Time, csn->BreakList, csn->Info->BreakCount, -1);

    if (csn->Time > csn->Play->EndTime || csn->BadReadPause) {
        if (csn->PlayIndex == csn->Info->NumTime - 1) {
            csn->Time = csn->Play->EndTime;
            return 0;
        }

        if (csn->BadReadPause && !csn->Waiting) {
            csn->BadReadPause = 0;
            xCutscene_SetSpeed(csn, csn->BadReadSpeed);
        }

        if (csn->CamTime != csn->Time) {
            return 1;
        }

        if (csn->Waiting) {
            csn->Time = csn->Play->EndTime;
            csn->CamTime = xCutsceneConvertBreak(csn->Time, csn->BreakList, csn->Info->BreakCount, -1);
            if (!csn->BadReadPause) {
                csn->BadReadSpeed = csn->PlaybackSpeed;
                xCutscene_SetSpeed(csn, 0.0f);
                csn->BadReadPause = 1;
            }
            return 1;
        }

        xCutsceneTime* oldChunk = csn->Play;
        csn->Play = csn->Stream;
        csn->Stream = oldChunk;
        csn->PlayIndex++;

        if (csn->PlayIndex + 1 < csn->Info->NumTime) {
            iCSFileAsyncRead(csn, csn->Stream, csn->TimeChunkOffs[csn->PlayIndex + 2] - csn->TimeChunkOffs[csn->PlayIndex + 1]);
        }
    }

    return 1;
}

void xCutscene_SetSpeed(xCutscene* csn, F32 speed)
{
    if (csn->BadReadPause) {
        return;
    }

    if (speed > 4.0f) speed = 4.0f;
    if (speed < 0.001f) speed = 0.0f;
    csn->PlaybackSpeed = speed;
}

// RESTORED: speculative
void xCutscene_SetCameraMatrix(xCutscene* csn, xMat4x3* mat)
{
    xCutsceneData* data;
    U32 i, dataIndex;
    F32 camFOV;
    xMat4x3 camMat;

    data = (xCutsceneData*)(csn->Play + 1);
    dataIndex = 0;
    while (dataIndex < csn->Play->NumData) {
        if (data->DataType == 3) {
            xMat3x3 tmpMat;
            xQuat quats[2];
            xQuat qresult;
            F32 invlerp, lerp, tmpXap, tmpFoc;
            S32 frame = std::floorf(30.0f * csn->CamTime);
            U32 count = *(U32*)(data + 1);
            zFlyKey* keys = (zFlyKey*)((U32*)(data + 1) + 1);
            if (frame < keys[0].frame) {
                lerp = 0.0f;
            }
            else if (frame >= keys[count - 1].frame) {
                lerp = 1.0f;
                keys = &keys[count - 2];
            }
            else {
                lerp = 30.0f * csn->CamTime - std::floorf(30.0f * csn->CamTime);
                keys = &keys[frame - keys[0].frame];
            }
            invlerp = 1.0f - lerp;

            for (i = 0; i < ARRAY_LENGTH(quats); i++) {
                tmpMat.right.x = -keys[i].matrix[0];
                tmpMat.right.y = -keys[i].matrix[1];
                tmpMat.right.z = -keys[i].matrix[2];
                tmpMat.up.x = keys[i].matrix[3];
                tmpMat.up.y = keys[i].matrix[4];
                tmpMat.up.z = keys[i].matrix[5];
                tmpMat.at.x = -keys[i].matrix[6];
                tmpMat.at.y = -keys[i].matrix[7];
                tmpMat.at.z = -keys[i].matrix[8];
                xQuatFromMat(&quats[i], &tmpMat);
            }
            xQuatSlerp(&qresult, &quats[0], &quats[1], lerp);
            xQuatToMat(&qresult, &camMat);
            xVec3Lerp(&camMat.pos, (xVec3*)&keys[0].matrix[9], (xVec3*)&keys[1].matrix[9], lerp);

            tmpXap = keys[0].aperture[0] * invlerp + keys[1].aperture[0] * lerp;
            tmpFoc = keys[0].focal * invlerp + keys[1].focal * lerp;
            camFOV = 2.0f * xatan2(12.7f * tmpXap, tmpFoc);
            camFOV *= (180.0f/PI);
        }
        data = (xCutsceneData*)((U8*)data + sizeof(xCutsceneData) + ((data->ChunkSize + 15) & ~15));
        dataIndex++;
    }

    *mat = camMat;
    xglobals->cam->fov = camFOV;
}
// END RESTORED

struct xCutsceneAnimHdr
{
    U32 RootIndex;
    F32 Translate[3];
};

struct xCutsceneMphFrame
{
    F32 Time;
    U32 Flags;
};

struct xCutsceneMphRun
{
    S32 VertIdx;
    S32 VertCt;
};

static void xcsCalcAnimMatrices(RwMatrix* animMat, RpAtomic* model, xCutsceneAnimHdr* ahdr, F32 time, U32 tworoot)
{
    xQuat quatresult[IANIM_MAX_BONES + 1];
    xVec3 tranresult[IANIM_MAX_BONES + 1];
    void* afile = ahdr + 1;
    iAnimEval(afile, time, 0, tranresult, quatresult);

    if (iModelNumBones(model)) {
        xMat4x3Identity((xMat4x3*)animMat);
        animMat->pos.x = ahdr->Translate[0];
        animMat->pos.y = ahdr->Translate[1];
        animMat->pos.z = ahdr->Translate[2];

        if (tworoot) {
            xMat4x3 m1, m2;

            quatresult[1].s = -quatresult[1].s;
            quatresult[2].s = -quatresult[2].s;
            xQuatToMat(&quatresult[1], &m1);
            xQuatToMat(&quatresult[2], &m2);
            m1.pos = tranresult[1];
            m2.pos = tranresult[2];
            xMat4x3Mul(&m1, &m2, &m1);
            tranresult[2].x = 0.0f;
            tranresult[2].y = 0.0f;
            tranresult[2].z = 0.0f;
            quatresult[2].v.x = 0.0f;
            quatresult[2].v.y = 0.0f;
            quatresult[2].v.z = 0.0f;
            quatresult[2].s = 1.0f;
            xQuatFromMat(&quatresult[1], &m1);
            quatresult[1].s = -quatresult[1].s;
            tranresult[1] = m1.pos;
        }

        U32 numbone = iModelNumBones(model), boneidx = 0;
        xQuat* qqq = quatresult;
        xVec3* ttt = tranresult;
        while (boneidx < numbone && boneidx <= ahdr->RootIndex) {
            animMat->pos.x += ttt->x;
            animMat->pos.y += ttt->y;
            animMat->pos.z += ttt->z;
            ttt->x = 0.0f;
            ttt->y = 0.0f;
            ttt->z = 0.0f;
            if (xabs(qqq->s) < 0.9999f)
                break;
            boneidx++;
            qqq++;
            ttt++;
        }

        iModelAnimMatrices(model, quatresult, tranresult, animMat + 1);
    } else {
        xQuatToMat(quatresult, (xMat3x3*)animMat);
        animMat->pos.x = tranresult[0].x + ahdr->Translate[0];
        animMat->pos.y = tranresult[0].y + ahdr->Translate[1];
        animMat->pos.z = tranresult[0].z + ahdr->Translate[2];
    }
}

struct JDeltaRun
{
    S32 VertIdx;
    S32 VertCt;
};

struct JDeltaTarget
{
    S32 skipSize;
    S32 numRuns;
    S32 numVerts;
    F32 scale;
    JDeltaRun runs[1];
};

static void JDeltaEval(RpAtomic* model, void* deltaModel, void* deltaAnim, F32 time)
{
    F32 outweight[MAX_WEIGHTS];
    F32* currweight;
    S32 i;

    S32 numFrames = ((S32*)deltaAnim)[1];
    S32 numWeights = ((S32*)deltaAnim)[2];
    F32* times = (F32*)deltaAnim + 3;
    F32* weights = times + numFrames;

    if (time <= times[0] || numFrames == 1) {
        memcpy(outweight, weights, numWeights * sizeof(F32));
    }
    else if (time >= times[numFrames - 1]) {
        memcpy(outweight, weights + (numFrames - 1) * numWeights, numWeights * sizeof(F32));
    }
    else {
        while (time > times[1]) {
            times++;
            weights += numWeights;
        }
        F32 lerp = (time - times[0]) / (times[1] - times[0]);
        F32 invlerp = 1.0f - lerp;
        for (i = 0; i < numWeights; i++) {
            outweight[i] = invlerp * weights[i] + lerp * weights[i + numWeights];
        }
    }

    RpGeometryLock(model->geometry, rpGEOMETRYLOCKVERTICES);

    RwV3d* outverts;
    S32 numRun;
    JDeltaTarget* dtgt;

    dtgt = (JDeltaTarget*)((S32*)deltaModel + 2);
    numRun = dtgt->numRuns;
    outverts = model->geometry->morphTarget->verts;
    if (dtgt->scale) {
        F32 scale = dtgt->scale;
        S16* svert = (S16*)((U8*)deltaModel + ((((U8*)&dtgt->runs[dtgt->numRuns] - (U8*)deltaModel) + 15) & ~15));
        for (i = 0; i < numRun; i++) {
            S32 j, cmpval = dtgt->runs[i].VertIdx + dtgt->runs[i].VertCt;
            for (j = dtgt->runs[i].VertIdx; j < cmpval; j++) {
                outverts[j].x = scale * svert[0];
                outverts[j].y = scale * svert[1];
                outverts[j].z = scale * svert[2];
                svert += 3;
            }
        }
    } else {
        RwV3d* vert = (RwV3d*)((U8*)deltaModel + ((((U8*)&dtgt->runs[dtgt->numRuns] - (U8*)deltaModel) + 15) & ~15));
        for (i = 0; i < numRun; i++) {
            S32 j, cmpval = dtgt->runs[i].VertIdx + dtgt->runs[i].VertCt;
            for (j = dtgt->runs[i].VertIdx; j < cmpval; j++) {
                outverts[j] = *vert;
                vert++;
            }
        }
    }

    currweight = outweight;
    dtgt = (JDeltaTarget*)((U8*)dtgt + dtgt->skipSize);
    while (numWeights) {
        numRun = dtgt->numRuns;
        if (*currweight) {
            if (dtgt->scale) {
                F32 scale = dtgt->scale * *currweight;
                S16* svert = (S16*)((U8*)deltaModel + ((((U8*)&dtgt->runs[dtgt->numRuns] - (U8*)deltaModel) + 15) & ~15));
                for (i = 0; i < numRun; i++) {
                    S32 j, cmpval = dtgt->runs[i].VertIdx + dtgt->runs[i].VertCt;
                    for (j = dtgt->runs[i].VertIdx; j < cmpval; j++) {
                        outverts[j].x += scale * svert[0];
                        outverts[j].y += scale * svert[1];
                        outverts[j].z += scale * svert[2];
                        svert += 3;
                    }
                }
            } else {
                F32 scale = *currweight;
                RwV3d* vert = (RwV3d*)((U8*)deltaModel + ((((U8*)&dtgt->runs[dtgt->numRuns] - (U8*)deltaModel) + 15) & ~15));
                for (i = 0; i < numRun; i++) {
                    S32 j, cmpval = dtgt->runs[i].VertIdx + dtgt->runs[i].VertCt;
                    for (j = dtgt->runs[i].VertIdx; j < cmpval; j++) {
                        outverts[j].x += scale * vert->x;
                        outverts[j].y += scale * vert->y;
                        outverts[j].z += scale * vert->z;
                        vert++;
                    }
                }
            }
        }
        dtgt = (JDeltaTarget*)((U8*)dtgt + dtgt->skipSize);
        currweight++;
        numWeights--;
    }

    RpGeometryUnlock(model->geometry);
}

struct CutsceneShadowModel
{
    RpAtomic* model;
    RwMatrix* animMat;
    U32 shadowBits;
};

static void CutsceneShadowRender(CutsceneShadowModel* smod)
{
    RpAtomic* model = smod->model;
    U32 bits = smod->shadowBits;

    while (model) {
        if (bits & 1) {
            iModelRender(model, smod->animMat);
        }
        model = iModelFile_RWMultiAtomic(model);
        bits >>= 1;
    }
}

void xCutscene_Render(xCutscene* csn, xEnt**, S32*, F32*)
{
    U32 i, dataIndex, animIndex, mphIndex, visFlags, visIdx, fakeCount;
    U32 tworoot, noshadow;
    xCutsceneData* data;
    xCutsceneData* mphdata;
    RpAtomic* model;
    RpAtomic* shadowModel;
    RwMatrix animMat[IANIM_MAX_BONES + 1];

    visFlags = 0xFFFFFFFF; fakeCount = 0;

    xVec3* camVec = (xVec3*)RwMatrixGetPos(RwFrameGetMatrix(RwCameraGetFrame(RwCameraGetCurrentCamera())));

    xModelPipe* pipeCurr;
    U32 idx = 0;
    U32 hacked_model = 0;
    S32 tableIdx = -1;
    XCSNNosey* nosey = csn->cb_nosey;

    data = (xCutsceneData*)(csn->Play + 1);
    animIndex = 0;
    dataIndex = 0;
    while (dataIndex < csn->Play->NumData) {
        if (data->DataType == 2) {
            F32 breakTime = xCutsceneConvertBreak(csn->CamTime, csn->BreakList, csn->Info->BreakCount, animIndex);

            model = NULL;
            tworoot = 0;
            noshadow = 0;
            pipeCurr = NULL;
            if (!model) {
                for (i = 0; i < csn->Info->NumData; i++) {
                    if ((csn->Data[i].DataType & 0x0FFFFFFF) == 1 && data->AssetID == csn->Data[i].AssetID) {
                        model = (RpAtomic*)csn->Data[i].DataPtr;
                        tworoot = csn->Data[i].DataType & 0x20000000;
                        noshadow = csn->Data[i].DataType & 0x10000000;
                        pipeCurr = csn->ModelHack[i].pipe;
                        break;
                    }
                }
            }

            xBox combinedAnimBound = { 0 };
            combinedAnimBound.upper.x = -1e38f;

            U32 tempSize;
            if (!model) {
                model = (RpAtomic*)xSTFindAsset(data->AssetID, &tempSize);
            }

            if (model) {
                shadowModel = model;
                hacked_model = 0;

                tableIdx = -1;
                idx = 0;

                S32 table_size = zCutsceneMgrGetHackTableSize();
                for (S32 h = 0; h < table_size; h++) {
                    RpAtomic* tmpModel;
                    zCutsceneHack& cutsceneHackTableData = zCutsceneMgrGetHackTableDataByIdx(h);

                    if (model == cutsceneHackTableData.dataPtr && xStrHash(cutsceneHackTableData.cinname) == csn->Info->AssetID) {
                        tableIdx = h; hacked_model = 1;
                        zCutsceneHack& cutsceneHackTableData2 = zCutsceneMgrGetHackTableDataByIdx(tableIdx);
                        tmpModel = (RpAtomic*)cutsceneHackTableData2.atomic_dataPtr[cutsceneHackTableData2.atomic_Idx[idx++]];
                        model = tmpModel;
                        shadowModel = tmpModel;
                        break;
                    }
                }

                visFlags = 0xFFFFFFFF;
                if (csn->Info->VisCount) {
                    U32 viscnt = csn->Info->VisCount;
                    U32* currvis = csn->Visibility;

                    while (viscnt) {
                        U32 objIndex = currvis[0] & 0xFFFF;
                        U32 subIndex = currvis[0] >> 16;
                        U32 frameMin = currvis[1] & 0xFFFF;
                        U32 frameMax = currvis[1] >> 16;

                        if (objIndex == animIndex) {
                            U32 frameIndex = 30.0f * breakTime;
                            if (frameIndex > frameMax - frameMin)
                                frameIndex = frameMax - frameMin;
                            U32 flag = (currvis[(frameIndex >> 5) + 2] >> (frameIndex & 31)) & 1;
                            if (!flag) {
                                visFlags &= ~(1 << subIndex);
                            }
                        }

                        currvis += ((frameMax - frameMin) >> 5) + 3;
                        viscnt--;
                    }
                }

                xcsCalcAnimMatrices(animMat, model, (xCutsceneAnimHdr*)(data + 1), breakTime - csn->Play->StartTime, tworoot);

                if (nosey && (nosey->flg_nosey & 0x2)) {
                    nosey->UpdatedAnimated(model, animMat, animIndex, dataIndex, visFlags);
                }

                U32 shadowBits = 0;
                visIdx = 0;

                while (model) {
                    if (visFlags & (1 << visIdx)) {
                        mphdata = (xCutsceneData*)(csn->Play + 1);
                        zCutsceneHack& cutsceneHackTableData2 = zCutsceneMgrGetHackTableDataByIdx(tableIdx);

                        mphIndex = 0;
                        while (mphIndex < csn->Play->NumData) {
                            U32 morphAnimIndex = mphdata->AssetID & 0xFFFF;
                            U32 morphModelIndex = mphdata->AssetID >> 16;
                            if ((mphdata->DataType == 4 || mphdata->DataType == 7) && morphAnimIndex == animIndex &&
                                ((!hacked_model && morphModelIndex == visIdx) ||
                                 (hacked_model && morphModelIndex == cutsceneHackTableData2.atomic_Idx[visIdx]))) {
                                if (mphdata->DataType == 4) {
                                    U32 numFrame = ((U32*)(mphdata + 1))[0];
                                    U32 numRun = ((U32*)(mphdata + 1))[1];
                                    xCutsceneMphFrame* mphFrame = (xCutsceneMphFrame*)((U32*)(mphdata + 1) + 2);
                                    xCutsceneMphRun* mphRun = (xCutsceneMphRun*)(mphFrame + numFrame);
                                    xMorphTargetFile* mphFile = (xMorphTargetFile*)((U32*)(mphdata + 1) + ((2 + numFrame * 2 + numRun * 2 + 3) & ~3));

                                    S16* v_array[4] = { 0 };
                                    S16 weight[4] = { 0 };

                                    for (i = 1; i < numFrame - 1; i++) {
                                        if (mphFrame[i].Time >= breakTime) break;
                                    }

                                    U32 skipsize = (mphFile->NumVerts * 3 + 7) & ~7;
                                    if (mphFile->Flags & 0x1) {
                                        skipsize *= 2;
                                    }

                                    v_array[0] = (S16*)(mphFile + 1) + skipsize * (i - 1);
                                    v_array[1] = (S16*)(mphFile + 1) + skipsize * i;
                                    weight[1] = 16384.0f * (breakTime - mphFrame[i - 1].Time) / (mphFrame[i].Time - mphFrame[i - 1].Time);
                                    if (weight[1] > 0x4000) weight[1] = 0x4000;
                                    weight[0] = 0x4000 - weight[1];

                                    xVec3* csnTmpArray = (xVec3*)&gRenderArr;
                                    FastS16weight2((F32*)csnTmpArray, v_array, weight, mphFile->NumVerts * 3, (1.0f / 16384.0f) * mphFile->Scale);

                                    RpGeometryLock(model->geometry, rpGEOMETRYLOCKVERTICES);

                                    xVec3* currtmp = csnTmpArray;
                                    xVec3* outv = (xVec3*)model->geometry->morphTarget->verts;
                                    for (i = 0; i < numRun; i++) {
                                        U32 j, cmpval = mphRun[i].VertIdx + mphRun[i].VertCt;
                                        for (j = mphRun[i].VertIdx; j < cmpval; j++) {
                                            outv[j] = *currtmp;
                                            currtmp++;
                                        }
                                    }
                                    RpGeometryUnlock(model->geometry);
                                } else {
                                    void* deltaAnim = mphdata + 1;
                                    void* deltaModel = NULL;

                                    for (i = 0; i < csn->Info->NumData; i++)
                                        if ((csn->Data[i].DataType & 0x0FFFFFFF) == 6 && data->AssetID == csn->Data[i].AssetID) {
                                            deltaModel = csn->Data[i].DataPtr;
                                            break;
                                        }

                                    JDeltaEval(model, deltaModel, deltaAnim, breakTime);
                                }

                                if (pipeCurr && (pipeCurr->Flags & 0xFF00)) {
                                    if (fakeCount < FAKE_MAX) {
                                        sCutsceneFakeModel[fakeCount].Data = model;
                                        sCutsceneFakeModel[fakeCount].BoneCount = iModelNumBones(model);
                                        memcpy(sCutsceneFakeModel[fakeCount].Mat, animMat, sizeof(animMat));
                                        sCutsceneFakeModel[fakeCount].Flags = 0x1;
                                        sCutsceneFakeModel[fakeCount].FadeStart = 100.0f;
                                        sCutsceneFakeModel[fakeCount].FadeEnd = 100.0f;
                                        sCutsceneFakeModel[fakeCount].Bucket[0]->Data = model;
                                        sCutsceneFakeModel[fakeCount].Alpha = model->geometry->matList.materials[0]->color.alpha / 255.0f;
                                        sCutsceneFakeModel[fakeCount].Pipe = *pipeCurr;
                                        sCutsceneFakeModel[fakeCount].LightKit = gLastLightKit;
                                        if (sCutsceneFakeModel[fakeCount].BoneCount) {
                                            iModelAnimBoundWorld(&sCutsceneFakeModel[fakeCount].animBound, &combinedAnimBound, model, animMat, &animMat[1]);
                                        }

                                        xModelBucket_Add(&sCutsceneFakeModel[fakeCount]);
                                        fakeCount++;
                                    }
                                } else {
                                    S32 culled = 0;
                                    if (iModelNumBones(model)) {
                                        xBox worldbox;
                                        iModelAnimBoundWorld(&worldbox, &combinedAnimBound, model, animMat, &animMat[1]);
                                        culled = xModelBoxCull(&worldbox);
                                        iModelSetWorldBoundFromBox(model, &worldbox);
                                    } else {
                                        iModelWorldBoundSphere(model, animMat, NULL);
                                    }

                                    if (!culled) {
                                        iModelRender(model, animMat);
                                    }

                                    if (iModelNumBones(model) >= 0 && !hacked_model) {
                                        shadowBits |= 1 << visIdx;
                                    }
                                }
                                goto next_atomic;
                            }

                            mphdata = (xCutsceneData*)((U8*)mphdata + sizeof(xCutsceneData) + ((mphdata->ChunkSize + 15) & ~15));
                            mphIndex++;
                        }

                        if (pipeCurr && (pipeCurr->Flags & 0xFF00)) {
                            if (fakeCount < FAKE_MAX) {
                                sCutsceneFakeModel[fakeCount].Data = model;
                                sCutsceneFakeModel[fakeCount].BoneCount = iModelNumBones(model);
                                memcpy(sCutsceneFakeModel[fakeCount].Mat, animMat, sizeof(animMat));
                                sCutsceneFakeModel[fakeCount].Flags = 0x1;
                                sCutsceneFakeModel[fakeCount].FadeStart = 1000.0f;
                                sCutsceneFakeModel[fakeCount].FadeEnd = 1000.0f;
                                sCutsceneFakeModel[fakeCount].Bucket[0]->Data = model;
                                sCutsceneFakeModel[fakeCount].Alpha = model->geometry->matList.materials[0]->color.alpha / 255.0f;
                                sCutsceneFakeModel[fakeCount].Pipe = *pipeCurr;
                                sCutsceneFakeModel[fakeCount].LightKit = gLastLightKit;
                                if (sCutsceneFakeModel[fakeCount].BoneCount) {
                                    iModelAnimBoundWorld(&sCutsceneFakeModel[fakeCount].animBound, &combinedAnimBound, model, animMat, &animMat[1]);
                                }

                                xModelBucket_Add(&sCutsceneFakeModel[fakeCount]);
                                fakeCount++;
                            }
                        } else {
                            S32 culled = 0;
                            if (iModelNumBones(model)) {
                                xBox worldbox;
                                iModelAnimBoundWorld(&worldbox, &combinedAnimBound, model, animMat, &animMat[1]);
                                culled = xModelBoxCull(&worldbox);
                                iModelSetWorldBoundFromBox(model, &worldbox);
                            } else {
                                iModelWorldBoundSphere(model, animMat, NULL);
                            }

                            if (!culled) {
                                iModelRender(model, animMat);
                            }

                            if (iModelNumBones(model) >= 0 && !hacked_model) {
                                shadowBits |= 1 << visIdx;
                            }
                        }
                    }

                next_atomic:
                    if (pipeCurr) {
                        pipeCurr++;
                    }
                    visIdx++;
                    if (hacked_model) {
                        zCutsceneHack& cutsceneHackTableData2 = zCutsceneMgrGetHackTableDataByIdx(tableIdx);
                        if (idx < cutsceneHackTableData2.numAtomics)
                            model = (RpAtomic*)cutsceneHackTableData2.atomic_dataPtr[cutsceneHackTableData2.atomic_Idx[idx++]];
                        else
                            model = NULL;
                    } else
                        model = iModelFile_RWMultiAtomic(model);
                }

                if (shadowBits && xVec3Dist(camVec, (xVec3*)&animMat[0].pos) < 25.0f && !noshadow) {
                    CutsceneShadowModel smod;
                    xShadowCache scache;
                    xVec3 center;

                    scache.poly = (xShadowPoly*)xMEMPUSHTEMP(SHADOW_CACHE_POLY_MAX * sizeof(xShadowPoly));

                    center.x = 0.5f * (combinedAnimBound.upper.x + combinedAnimBound.lower.x);
                    center.y = 0.5f * (combinedAnimBound.upper.y + combinedAnimBound.lower.y);
                    center.z = 0.5f * (combinedAnimBound.upper.z + combinedAnimBound.lower.z);
                    F32 fillCacheRadius = xsqrt(xsqr(0.5f * (combinedAnimBound.upper.x - combinedAnimBound.lower.x)) +
                                                xsqr(0.5f * (combinedAnimBound.upper.y - combinedAnimBound.lower.y)) +
                                                xsqr(0.5f * (combinedAnimBound.upper.z - combinedAnimBound.lower.z)));

                    smod.model = shadowModel;
                    smod.animMat = animMat;
                    smod.shadowBits = shadowBits;

                    xLightKit* disabledLightKit = gLastLightKit;
                    xLightKit_Enable(NULL, xglobals->currWorld);

                    static xVec3 shadVec = { 0.0f, -1.0f, 0.0f };
                    xShadowSetLight(&center, &shadVec, 1.0f);
                    xShadowVertical_FillCache(&scache, &center, fillCacheRadius, 6.0f, 0.0871557f);

                    xShadowCameraUpdate(&smod, (xShadowRenderCallback)CutsceneShadowRender, &center, fillCacheRadius, 0);

                    xShadowVertical_DrawCache(&scache, 0.3f, 0.0f, 0, NULL, NULL);

                    xLightKit_Enable(disabledLightKit, xglobals->currWorld);

                    xMEMPOPTEMP(scache.poly);
                }
            }

            animIndex++;
        }

        data = (xCutsceneData*)((U8*)data + sizeof(xCutsceneData) + ((data->ChunkSize + 15) & ~15));
        dataIndex++;
    }

    if (nosey && (nosey->flg_nosey & 0x1)) {
        nosey->CanRenderNow();
    }

    xSubTitlesRender(csn->SubTitlesID, csn->Time);
}

xCutscene* xCutscene_CurrentCutscene()
{
    return &sActiveCutscene;
}

namespace {

    struct xCamCutscene : xCam
    {
        xCutscene* csn;
        xMat4x3 original_mat;

        void create()
        {
            xCam::create();
            coord_type = XCAM_COORD_CART;
            orient_type = XCAM_ORIENT_QUAT;
            cfg_common.priority = 255;
            cfg_common.blend_time = 0.0f;
        }

        void start()
        {
            xCam::start();
            original_mat = mat;
            coord_to_local(coord.cart, mat.pos);
            orient_to_local(orient.quat, mat);
        }

        void stop()
        {
            xCam::stop();
            mat = original_mat;
        }

        void update(xScene&, F32)
        {
            xCutsceneData* data;
            U32 i, dataIndex;

            data = (xCutsceneData*)(csn->Play + 1);
            dataIndex = 0;
            while (dataIndex < csn->Play->NumData) {
                if (data->DataType == 3) {
                    xMat3x3 tmpMat;
                    xQuat quats[2];
                    F32 invlerp, lerp, tmpXap, tmpFoc;
                    S32 frame = std::floorf(30.0f * csn->CamTime);
                    U32 count = *(U32*)(data + 1);
                    zFlyKey* keys = (zFlyKey*)((U32*)(data + 1) + 1);
                    if (frame < keys[0].frame) {
                        lerp = 0.0f;
                    }
                    else if (frame >= keys[count - 1].frame) {
                        lerp = 1.0f;
                        keys = &keys[count - 2];
                    }
                    else {
                        lerp = 30.0f * csn->CamTime - std::floorf(30.0f * csn->CamTime);
                        keys = &keys[frame - keys[0].frame];
                    }
                    invlerp = 1.0f - lerp;

                    for (i = 0; i < ARRAY_LENGTH(quats); i++) {
                        tmpMat.right.x = -keys[i].matrix[0];
                        tmpMat.right.y = -keys[i].matrix[1];
                        tmpMat.right.z = -keys[i].matrix[2];
                        tmpMat.up.x = keys[i].matrix[3];
                        tmpMat.up.y = keys[i].matrix[4];
                        tmpMat.up.z = keys[i].matrix[5];
                        tmpMat.at.x = -keys[i].matrix[6];
                        tmpMat.at.y = -keys[i].matrix[7];
                        tmpMat.at.z = -keys[i].matrix[8];
                        xQuatFromMat(&quats[i], &tmpMat);
                    }
                    xQuatSlerp(&orient.quat, &quats[0], &quats[1], lerp);
                    xQuatToMat(&orient.quat, &mat);
                    xVec3Lerp(&mat.pos, (xVec3*)&keys[0].matrix[9], (xVec3*)&keys[1].matrix[9], lerp);

                    tmpXap = keys[0].aperture[0] * invlerp + keys[1].aperture[0] * lerp;
                    tmpFoc = keys[0].focal * invlerp + keys[1].focal * lerp;
                    fov = 2.0f * xatan2(12.7f * tmpXap, tmpFoc);
                }
                data = (xCutsceneData*)((U8*)data + sizeof(xCutsceneData) + ((data->ChunkSize + 15) & ~15));
                dataIndex++;
            }

            coord.cart = mat.pos;
        }
    };

    xCamCutscene* cutscene_cam;

    void init_cutscene_cam()
    {
        cutscene_cam = new (eMemStaticType_0, 0) xCamCutscene;
        cutscene_cam->create();
    }

    void cutscene_started(xCutscene* csn)
    {
        csn->started = true;
        cutscene_cam->csn = csn;
        zCamAdd(*cutscene_cam, true);
        zCutsceneMgr_Started();
    }

    void cutscene_stopped(xCutscene* csn)
    {
        csn->started = false;
        cutscene_cam->csn = NULL;
        zCamRemove(*cutscene_cam, true);
    }

}

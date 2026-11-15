#include "xCommon.h"
#include "iAnimSKB.h"

#include "xMath.h"
#include "iMath.h"

#include "decomp.h"

#include <cmath>
#include <rtslerp.h>

struct SKAKey {
    F32 quat[4];
    F32 tran[3];
    F32 time;
    U32 prevFrame;
};

struct SKAHeader {
    U32 numFrames;
    U32 flags;
    F32 duration;
};

struct ANMHeader {
    U32 chunkType;
    U32 chunkLength;
    U32 chunkVersion;
    S32 hanimVersion;
    S32 hanimTypeID;
    U32 numFrames;
    S32 flags;
    F32 duration;
};

struct ANMKey {
    F32 time;
    F32 quat[4];
    F32 tran[3];
    U32 prevFrame;
};

void iAnimEvalSKB(iAnimSKBHeader* data, F32 time, U32 flags, xVec3* tran, xQuat* quat)
{
    U32 i, tidx, bcount = data->BoneCount, tcount = data->TimeCount;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    F32* times = (F32*)(keys + data->KeyCount);
    U16* offsets = (U16*)(times + tcount);

    if (time < 0.0f) time = 0.0f;

    if (time > times[tcount - 1]) {
        time = times[tcount - 1];
    }

    tidx = (tcount - 1) & 3;
    while (times[tidx] < time)
        tidx += 4;
    while (tidx && time <= times[tidx])
        tidx--;

    offsets += tidx * bcount;

    if (flags & 0x1) {
        bcount = 1;
    }
    if (flags & 0x2) {
        bcount--;
        offsets++;
    }

    F32 scalex = data->Scale[0];
    F32 scaley = data->Scale[1];
    F32 scalez = data->Scale[2];

    if (tcount == 1) {
        F32 scalex = data->Scale[0];
        F32 scaley = data->Scale[1];
        F32 scalez = data->Scale[2];

        for (i = 0; i < bcount; i++) {
            iAnimSKBKey* k = &keys[i * 2];

            quat->v.x = (1.0f / 32767.0f) * k->Quat[0];
            quat->v.y = (1.0f / 32767.0f) * k->Quat[1];
            quat->v.z = (1.0f / 32767.0f) * k->Quat[2];
            quat->s = (1.0f / 32767.0f) * k->Quat[3];

            tran->x = scalex * k->Tran[0];
            tran->y = scaley * k->Tran[1];
            tran->z = scalez * k->Tran[2];

            quat++;
            tran++;
        }
    } else {
        F32 scalex = data->Scale[0];
        F32 scaley = data->Scale[1];
        F32 scalez = data->Scale[2];
        F32 time1, time2, lerp;

        for (i = 0; i < bcount; i++) {
            iAnimSKBKey* k = &keys[*offsets];
            offsets++;

            time1 = times[k[0].TimeIndex];
            time2 = times[k[1].TimeIndex];
            lerp = (time - time1) / (time2 - time1);

            RtQuat q1, q2;
            RtQuatSlerpCache qcache;

            q1.imag.x = (1.0f / 32767.0f) * k[0].Quat[0];
            q1.imag.y = (1.0f / 32767.0f) * k[0].Quat[1];
            q1.imag.z = (1.0f / 32767.0f) * k[0].Quat[2];
            q1.real = (1.0f / 32767.0f) * k[0].Quat[3];

            q2.imag.x = (1.0f / 32767.0f) * k[1].Quat[0];
            q2.imag.y = (1.0f / 32767.0f) * k[1].Quat[1];
            q2.imag.z = (1.0f / 32767.0f) * k[1].Quat[2];
            q2.real = (1.0f / 32767.0f) * k[1].Quat[3];

            RtQuatSetupSlerpCache(&q1, &q2, &qcache);
            RtQuatSlerp((RtQuat*)quat, &q1, &q2, lerp, &qcache);

            xVec3 t1, t2;

            t1.x = scalex * k[0].Tran[0];
            t1.y = scaley * k[0].Tran[1];
            t1.z = scalez * k[0].Tran[2];

            t2.x = scalex * k[1].Tran[0];
            t2.y = scaley * k[1].Tran[1];
            t2.z = scalez * k[1].Tran[2];

            tran->x = t1.x + lerp * (t2.x - t1.x);
            tran->y = t1.y + lerp * (t2.y - t1.y);
            tran->z = t1.z + lerp * (t2.z - t1.z);

            quat++;
            tran++;
        }
    }
}

F32 iAnimDurationSKB(iAnimSKBHeader* data)
{
    return ((F32*)((iAnimSKBKey*)(data + 1) + data->KeyCount))[data->TimeCount - 1];
}

static S32 CmpTime(const void* a, const void* b); // UNUSED
static S32 CmpBoneTime(const void* a, const void* b); // UNUSED

DECOMP_FORCEFLOAT(32767.0f)
DECOMP_FORCEBLOCK((F32 x) { std::sqrtf(x); })

void _iAnimSKBAdjustTranslate(iAnimSKBHeader* data, U32 bone, F32* starttran, F32* endtran)
{
    S32 ipos;
    U32 i, idx, keyfirst, keylast, kcount = data->KeyCount, bcount = data->BoneCount, tcount = data->TimeCount;
    F32 outScale[3], pos, factor[3], oldmax[3] = { 0.0f, 0.0f, 0.0f }, newmax[3] = { 0.0f, 0.0f, 0.0f }, timefirst,
        timelast;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    F32* times = (F32*)(keys + kcount);
    U16* offsets = (U16*)(times + tcount);

    keyfirst = offsets[bone];
    keylast = offsets[bone + (tcount - 2) * bcount] + 1;

    timefirst = times[0];
    timelast = times[tcount - 1];

    for (i = 0; i < kcount; i++)
        for (idx = 0; idx < 3; idx++)
            if (starttran[idx] || endtran[idx]) {
                pos = data->Scale[idx] * keys[i].Tran[idx];

                if (iabs(pos) > oldmax[idx]) oldmax[idx] = iabs(pos);
                if (i >= keyfirst && i <= keylast) {
                    pos += starttran[idx] + (endtran[idx] - starttran[idx]) * (times[keys[i].TimeIndex] - timefirst) /
                        (timelast - timefirst);
                    if (iabs(pos) > newmax[idx]) newmax[idx] = iabs(pos);
                }
            }

    for (idx = 0; idx < 3; idx++)
        if (starttran[idx] || endtran[idx]) {
            if (newmax[idx] > oldmax[idx])
                outScale[idx] = data->Scale[idx] * newmax[idx] / oldmax[idx];
            else
                outScale[idx] = data->Scale[idx];
            factor[idx] = 1.0f / outScale[idx];
        }

    for (i = 0; i < kcount; i++)
        for (idx = 0; idx < 3; idx++)
            if (starttran[idx] || endtran[idx]) {
                if (i >= keyfirst && i <= keylast) {
                    ipos = factor[idx] * (data->Scale[idx] * keys[i].Tran[idx] +
                        (starttran[idx] + (endtran[idx] - starttran[idx]) * (times[keys[i].TimeIndex] - timefirst) /
                        (timelast - timefirst)));

                    if (ipos < -32767) ipos = -32767;
                    else if (ipos > 32767) ipos = 32767;

                    keys[i].Tran[idx] = ipos;
                } else if (data->Scale[idx] != outScale[idx]) {
                    ipos = data->Scale[idx] * keys[i].Tran[idx] * factor[idx];

                    if (ipos < -32767) ipos = -32767;
                    else if (ipos > 32767) ipos = 32767;

                    keys[i].Tran[idx] = ipos;
                }
            }

    for (idx = 0; idx < 3; idx++)
        if (starttran[idx] || endtran[idx]) {
            data->Scale[idx] = outScale[idx];
        }
}

S32 _iAnimSKBNumExtract(iAnimSKBHeader* data, U32 bone)
{
    U32 i, keyfirst, keylast, kcount = data->KeyCount, bcount = data->BoneCount, tcount = data->TimeCount;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    F32* times = (F32*)(keys + kcount);
    U16* offsets = (U16*)(times + tcount);
    S32 tranFound = 0, lastTime, currTime;

    keyfirst = offsets[bone];
    keylast = offsets[bone + (tcount - 2) * bcount] + 1;

    lastTime = -1;
    for (i = keyfirst; i <= keylast; i++) {
        currTime = 30.0f * times[keys[i].TimeIndex];

        if (lastTime >= 0 && currTime > lastTime + 1) {
            tranFound += currTime - (lastTime + 1);
        }

        if (lastTime != currTime) {
            tranFound++;
        }

        lastTime = currTime;
    }

    return tranFound;
}

S32 _iAnimSKBExtractTranslateAxis(iAnimSKBHeader* data, U32 bone, xVec3* tranArray, S32, U8* axis)
{
    U32 i, j, keyfirst, keylast, kcount = data->KeyCount, bcount = data->BoneCount, tcount = data->TimeCount;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    F32* times = (F32*)(keys + kcount);
    U16* offsets = (U16*)(times + tcount);
    xVec3 currTran;
    xVec3* lastTran;
    S32 tranFound = 0, lastTime, currTime;

    keyfirst = offsets[bone];
    keylast = offsets[bone + (tcount - 2) * bcount] + 1;

    lastTime = -1;
    for (i = keyfirst; i <= keylast; i++) {
        currTime = 30.0f * times[keys[i].TimeIndex];

        if (axis[0])
            currTran.x = keys[i].Tran[0] * data->Scale[0];
        else
            currTran.x = 0.0f;

        if (axis[1])
            currTran.y = keys[i].Tran[1] * data->Scale[1];
        else
            currTran.y = 0.0f;

        if (axis[2])
            currTran.z = keys[i].Tran[2] * data->Scale[2];
        else
            currTran.z = 0.0f;

        if (lastTime >= 0 && currTime > lastTime + 1) {
            lastTran = tranArray - 1;
            for (j = 1; j < currTime - lastTime; j++) {
                F32 lerp;

                lerp = (F32)j / (F32)(currTime - lastTime);
                tranArray->x = lastTran->x + lerp * (currTran.x - lastTran->x);
                tranArray->y = lastTran->y + lerp * (currTran.y - lastTran->y);
                tranArray->z = lastTran->z + lerp * (currTran.z - lastTran->z);
                tranArray++;
                tranFound++;
            }
        }

        if (lastTime != currTime) {
            tranArray->x = currTran.x;
            tranArray->y = currTran.y;
            tranArray->z = currTran.z;
            tranArray++;
            tranFound++;
        }

        lastTime = currTime;

        for (j = 0; j < 3; j++) {
            if (axis[j]) {
                keys[i].Tran[j] = 0;
            }
        }
    }

    return tranFound;
}

F32 QuatToYaw(xQuat* q)
{
    F32 yaw, x, y;

    x = 2.0f * (q->s * q->v.y - q->v.x * q->v.z);
    // BUG: z + z, not z * z
    y = q->s * q->s + q->v.x * q->v.x - q->v.y * q->v.y - (q->v.z + q->v.z);

    yaw = xatan2(x, y);
    return yaw;
}

void YawToQuat(F32 yaw, xQuat* q)
{
    q->s = icos(0.5f * yaw);
    q->v.x = 0.0f;
    q->v.y = isin(0.5f * yaw);
    q->v.z = 0.0f;
}

static void QuatMul(xQuat* o, xQuat* a, xQuat* b)
{
    F32 s = a->s * b->s - a->v.x * b->v.x - a->v.y * b->v.y - a->v.z * b->v.z,
        x = a->s * b->v.x + a->v.x * b->s + a->v.y * b->v.z - a->v.z * b->v.y,
        y = a->s * b->v.y + a->v.y * b->s + a->v.z * b->v.x - a->v.x * b->v.z,
        z = a->s * b->v.z + a->v.z * b->s + a->v.x * b->v.y - a->v.y * b->v.x;

    o->v.x = x;
    o->v.y = y;
    o->v.z = z;
    o->s = s;
}

S32 _iAnimSKBExtractYaw(iAnimSKBHeader* data, U32 bone, F32* yawArray, S32)
{
    U32 i, j, keyfirst, keylast, kcount = data->KeyCount, bcount = data->BoneCount, tcount = data->TimeCount;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    F32* times = (F32*)(keys + kcount);
    U16* offsets = (U16*)(times + tcount);
    xQuat currQuat;
    F32 currYaw, lastYaw, lastCurrYaw = PI;
    S32 yawFound = 0, lastTime, currTime;

    keyfirst = offsets[bone];
    keylast = offsets[bone + (tcount - 2) * bcount] + 1;

    lastTime = -1;
    for (i = keyfirst; i <= keylast; i++) {
        currTime = 30.0f * times[keys[i].TimeIndex];

        currQuat.v.x = (1.0f / 32767.0f) * keys[i].Quat[0];
        currQuat.v.y = (1.0f / 32767.0f) * keys[i].Quat[1];
        currQuat.v.z = (1.0f / 32767.0f) * keys[i].Quat[2];
        currQuat.s = (1.0f / 32767.0f) * keys[i].Quat[3];

        currYaw = QuatToYaw(&currQuat);

        while (currYaw + PI < lastCurrYaw) {
            currYaw += 2.0f * PI;
        }
        while (currYaw - PI > lastCurrYaw) {
            currYaw -= 2.0f * PI;
        }

        if (lastTime >= 0 && currTime > lastTime + 1) {
            lastYaw = yawArray[-1];
            for (j = 1; j < currTime - lastTime; j++) {
                F32 lerp;

                lerp = (F32)j / (F32)(currTime - lastTime);
                *yawArray = lastYaw + lerp * (currYaw - lastYaw);
                yawArray++;
                yawFound++;
            }
        }

        if (lastTime != currTime) {
            *yawArray = currYaw;
            yawArray++;
            yawFound++;
        }

        lastTime = currTime;

        xQuat antiYaw;
        YawToQuat(-currYaw, &antiYaw);
        QuatMul(&currQuat, &currQuat, &antiYaw);
        keys[i].Quat[0] = CLAMP((S32)(32767.0f * currQuat.v.x), -32767.0f, 32767.0f);
        keys[i].Quat[1] = CLAMP((S32)(32767.0f * currQuat.v.y), -32767.0f, 32767.0f);
        keys[i].Quat[2] = CLAMP((S32)(32767.0f * currQuat.v.z), -32767.0f, 32767.0f);
        keys[i].Quat[3] = CLAMP((S32)(32767.0f * currQuat.s), -32767.0f, 32767.0f);

        lastCurrYaw = currYaw;
    }

    return yawFound;
}

void _iAnimSKBAdjustYaw(iAnimSKBHeader* data, U32 bone, F32 yawStart, F32 yawEnd)
{
    U32 i, keyfirst, keylast, kcount = data->KeyCount, bcount = data->BoneCount, tcount = data->TimeCount;
    F32 yaw;
    xQuat quatA, quatB;
    F32 timefirst, timelast;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    F32* times = (F32*)(keys + kcount);
    U16* offsets = (U16*)(times + tcount);

    keyfirst = offsets[bone];
    keylast = offsets[bone + (tcount - 2) * bcount] + 1;
    timefirst = times[0];
    timelast = times[tcount - 1];
    F32 invTimeDiff = 1.0f / (timelast - timefirst);

    for (i = 0; i < kcount; i++) {
        if (i >= keyfirst && i <= keylast) {
            quatA.v.x = (1.0f / 32767.0f) * keys[i].Quat[0];
            quatA.v.y = (1.0f / 32767.0f) * keys[i].Quat[1];
            quatA.v.z = (1.0f / 32767.0f) * keys[i].Quat[2];
            quatA.s = (1.0f / 32767.0f) * keys[i].Quat[3];

            yaw = yawStart + invTimeDiff * ((yawEnd - yawStart) * (times[keys[i].TimeIndex] - timefirst));

            YawToQuat(yaw, &quatB);
            QuatMul(&quatA, &quatA, &quatB);

            keys[i].Quat[0] = CLAMP((S32)(32767.0f * quatA.v.x), -32767.0f, 32767.0f);
            keys[i].Quat[1] = CLAMP((S32)(32767.0f * quatA.v.y), -32767.0f, 32767.0f);
            keys[i].Quat[2] = CLAMP((S32)(32767.0f * quatA.v.z), -32767.0f, 32767.0f);
            keys[i].Quat[3] = CLAMP((S32)(32767.0f * quatA.s), -32767.0f, 32767.0f);
        }
    }
}

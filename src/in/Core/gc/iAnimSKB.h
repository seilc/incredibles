#ifndef IANIMSKB_H
#define IANIMSKB_H

#include "xMath3.h"

struct iAnimSKBHeader {
    U32 Magic;           // 0x0
    void* ExtractedData; // 0x4
    U16 BoneCount;       // 0x8
    U16 TimeCount;       // 0xA
    U16 KeyCount;        // 0xC
    U16 TranCount;       // 0xE
    F32 Scale[3];        // 0x10
};

struct iAnimSKBKey {
    U16 Frame;     // 0x0
    U16 TranIndex; // 0x2
    S16 Quat[3];   // 0x4
};

void iAnimEvalSKB(iAnimSKBHeader* data, F32 time, U32 flags, xVec3* tran, xQuat* quat);
F32 iAnimDurationSKB(iAnimSKBHeader* data);
void _iAnimSKBAdjustTranslate(iAnimSKBHeader* data, U32 bone, F32* starttran, F32* endtran);
S32 _iAnimSKBNumExtract(iAnimSKBHeader* data, U32 bone);
S32 _iAnimSKBExtractTranslateAxis(iAnimSKBHeader* data, U32 bone, xVec3* tranArray, S32 tranCount, U8* axis);
F32 QuatToYaw(xQuat* q);
S32 _iAnimSKBExtractYaw(iAnimSKBHeader* data, U32 bone, F32* yawArray, S32 yawCount);
void _iAnimSKBAdjustYaw(iAnimSKBHeader* data, U32 bone, F32 yawStart, F32 yawEnd);

#endif

#ifndef ISYSTEM_H
#define ISYSTEM_H

#include "types.h"

#include <dolphin.h>

S32 iRwResourcesEmptyArena();
void iVSync();
void my_dsc(U16 token);
void iSystemGetWidthHeight(U32& uWidth, U32& uHeight, bool bPALMode);
void iSystemInit(U32);
void iSystemInitGraphics();
void iSystemExit();
void iSystemRWExit();
void iSystemExitRWFailSafe();
void null_func();
s32 iGetMinute();
s32 iGetHour();
s32 iGetDay();
s32 iGetMonth();
u32 iGetCurrFormattedDate(char* p);
u32 iGetCurrFormattedTime(char* p);
void* iSystemARAMAlloc(U32 numbytes_to_use);
void iSystemARAMFree(void* ptr);
void iSystemARAMInit();
U32 iSystemARAMBasePtrForFMOD();
U32 iSystemARAMSizeForFMOD();
void iSystemInitRWFailSafe();
void iSystemAlarmCallBack(OSAlarm* alarm, OSContext*);
void iSystemSleep(S32 milliSecondsToSleep, U32 cancelAlarmsTag);

#endif

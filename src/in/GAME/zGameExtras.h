#ifndef ZGAMEEXTRAS_H
#define ZGAMEEXTRAS_H

#include "xserializer.h"

extern U32 sCheatTestCheats[16];
extern U32 sCheatSwapCCLR[16];
extern U32 sCheatSwapCCUD[16];
extern U32 sCheatPressed[16];

void zGameExtras_SceneInit();
void zGameExtras_SceneReset();
void zGameExtras_SceneExit();
void zGameExtras_SceneUpdate(F32 dt);
void zGameExtras_NewGameReset();
void zGameExtras_Save(xSerial* xser);
void zGameExtras_Load(xSerial* xser);
void zGameCheatsUpdate(F32 dt);

#endif
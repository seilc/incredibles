#ifndef ZPLAYERBASE_H
#define ZPLAYERBASE_H

#include "zEnt.h"

struct xScene;

void zPlayerUpdate(xEnt* ent, xScene* sc, F32 dt);
void zPlayerRender(zEnt* ent);
void zPlayerReset(zEnt* ent);
void zPlayerMove(xEnt* ent, xScene* s, F32 dt, xEntFrame* frame);
void zPlayerLoadHIP(U32 sceneID);
void zPlayerUnloadHIP();
void zPlayer_StoreCheckPoint(const xVec3& pos, F32 rot, U32 initialCameraID);
void zPlayer_LoadCheckPoint();

#endif
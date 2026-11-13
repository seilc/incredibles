#ifndef XSKYDOME_H
#define XSKYDOME_H

#include "xEnt.h"

void xSkyDome_EmptyRender(xEnt*);
void xSkyDome_Setup();
void xSkyDome_AddEntity(xEnt* ent, S32 sortorder, S32 lockY, S32 lockXZ);
void xSkyDome_Render();
void xSkyDome_GetSettings(xEnt* ent, S32& sortorder, S32& lockY, S32& lockXZ); // UNUSED
void xSkyDome_BeginRenderSingle(); // UNUSED
void xSkyDome_EndRenderSingle(); // UNUSED
void xSkyDome_RenderSingle(xEnt* ent); // UNUSED

inline bool xSkyDome_EntityIsSkyDome(const xEnt* ent)
{
    return ent->render == xSkyDome_EmptyRender;
}

#endif

#ifndef ZCOLLGEOM_H
#define ZCOLLGEOM_H

#include "types.h"

struct xEnt;

U32 zCollGeom_EntSetup(xEnt* ent);
void zCollGeom_CamEnable(xEnt* ent);
void zCollGeom_CamDisable(xEnt* ent);

#endif

#ifndef ZNPCCOMBAT_H
#define ZNPCCOMBAT_H

#include "xEnt.h"

void zNPCCombatInit();
void zNPCCombatReset();
void zNPCCombatSetCombat(xEnt* ent, U32 id, const char* root, U32 atkTblID, U32,
                         U16 hitPoints, F32 scale);

#endif

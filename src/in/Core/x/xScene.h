#ifndef XSCENE_H
#define XSCENE_H

#include "xEnt.h"
#include "xEnv.h"
#include "xMemMgr.h"

struct xScene
{
    U32 sceneID;
    U16 flags;
    U16 num_trigs;
    U16 num_stats;
    U16 num_dyns;
    U16 num_npcs;
    U16 num_act_ents;
    F32 gravity;
    F32 drag;
    F32 friction;
    U16 num_ents_allocd;
    U16 num_trigs_allocd;
    U16 num_stats_allocd;
    U16 num_dyns_allocd;
    U16 num_npcs_allocd;
    xEnt** trigs;
    xEnt** stats;
    xEnt** dyns;
    xEnt** npcs;
    xEnt** act_ents;
    xEnv* env;
    xMemPool mempool;
    xBase* (*resolvID)(U32);
    const char* (*base2Name)(xBase*);
    const char* (*id2Name)(U32);
};

#endif
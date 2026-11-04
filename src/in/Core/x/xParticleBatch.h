#ifndef XPARTICLEBATCH_H
#define XPARTICLEBATCH_H

#include "xPtankPool.h"

typedef S32(*xParticleBatchUpdateCallback)(U8*, S32, ptank_pool&, F32, void*);

struct xParticleBatchSystem
{
    ptank_group_type pool_type;
    ptank_pool::render_state rs;
    U32 order_group;
    S32 order_index;
    S32 stride;
    xParticleBatchUpdateCallback update;
    void* context;
};

class xParticleBatchGroup
{
    U8* elements;
    S32 elements_size;

public:
    void create();
    S32 create_system(const xParticleBatchSystem& system);
};

void xParticleBatchSceneEnter();
void xParticleBatchSceneExit();
void xParticleBatchReset();
void xParticleBatchPreRender();

S32 xParticleBatchAddSystem(const xParticleBatchSystem& system);
void xParticleBatchReplaceSystem(S32 system_id, const xParticleBatchSystem& system);
S32 xParticleBatchEmit(S32 system_id, S32 amount, U8** mem);

#endif

#ifndef RTSKINSP_H
#define RTSKINSP_H

#include "rwcore.h"
#include "rpworld.h"

#ifdef __cplusplus
extern "C" {
#endif

RpAtomic* RtSkinSplitAtomicSplitGeometry(RpAtomic* atomic, RwUInt32 boneLimit);

#ifdef __cplusplus
}
#endif

#endif

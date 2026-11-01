#ifndef ICAMERA_H
#define ICAMERA_H

#include "iFog.h"
#include "xColor.h"

RwCamera* iCameraCreate(S32 width, S32 height, S32 mainGameCamera);
void iCameraDestroy(RwCamera* camera);
void iCameraSetFogParams(iFogParams* fp, F32 time);
RwCamera* RwCameraBeginUpdateWrapper(RwCamera* camera);

#endif

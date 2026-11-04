#ifndef ICAMERA_H
#define ICAMERA_H

#include "iFog.h"
#include "xColor.h"

RwCamera* iCameraCreate(S32 width, S32 height, S32 mainGameCamera);
void iCameraDestroy(RwCamera* camera);
void iCameraSetFogParams(iFogParams* fp, F32 time);
void iCameraSetFog(bool enable);
void iCameraSetOrthoProjection(RwCamera* cam, bool ortho);
RwCamera* RwCameraBeginUpdateWrapper(RwCamera* camera);

#endif

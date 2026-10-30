#ifndef ICAMERA_H
#define ICAMERA_H

#include "iFog.h"
#include "xColor.h"

void iCameraClear(RwCamera* camera, xColor color); // inferred from TSSM GC, Rat MAP
RwCamera* iCameraCreate(S32 width, S32 height, S32 mainGameCamera);
void iCameraDestroy(RwCamera* camera);
F32 iCameraGetFOV(RwCamera* cam); // inferred from TSSM GC, Rat MAP
void iCameraSetFogParams(iFogParams* fp, F32 time);
RwCamera* RwCameraBeginUpdateWrapper(RwCamera* camera);

#endif

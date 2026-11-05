#ifndef ICAMERA_H
#define ICAMERA_H

#include "iFog.h"
#include "xColor.h"

struct xMat4x3;

extern RwCamera* globalCamera;

RwCamera* iCameraCreate(S32 width, S32 height, S32 mainGameCamera);
void iCameraDestroy(RwCamera* camera);
void iCameraUpdatePos(RwCamera* cam, xMat4x3* pos);
void iCamGetViewMatrix(RwCamera* camera, xMat4x3* view_matrix);
void iCameraSetFogParams(iFogParams* fp, F32 time);
void iCameraSetFog(bool enable);
void iCameraSetOrthoProjection(RwCamera* cam, bool ortho);
RwCamera* RwCameraBeginUpdateWrapper(RwCamera* camera);
void iCameraPerformRenderWareHack(RwCamera* camera);

inline RwCamera* RwCameraCreateWrapper()
{
    RwCamera* camera = RwCameraCreate();
    if (camera) {
        iCameraPerformRenderWareHack(camera);
    }
    return camera;
}

#endif

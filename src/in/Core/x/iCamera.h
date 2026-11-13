#ifndef ICAMERA_H
#define ICAMERA_H

#include "iFog.h"
#include "iEnv.h"
#include "iTime.h"

extern RwCamera* globalCamera;

void iCameraSetOrthoProjection(RwCamera* cam, bool ortho);
RwCamera* iCameraCreate(S32 width, S32 height, S32 mainGameCamera);
void iCameraDestroy(RwCamera* camera);
void iCameraBegin(RwCamera* cam, S32);
void iCameraEnd(RwCamera* cam);
void iCameraShowRaster(RwCamera* cam);
void iCameraUpdatePos(RwCamera* cam, xMat4x3* pos);
F32 iCameraGetAspectRatio();
void iCameraSetFOV(RwCamera* cam, F32 fov);
void iCameraAssignEnv(RwCamera* camera, iEnv* env_geom);
void iCamGetViewMatrix(RwCamera* camera, xMat4x3* view_matrix);
void iCameraSetNearFarClip(F32 nearPlane, F32 farPlane);
void iCameraSetFogParams(iFogParams* fp, F32 time);
void iCameraUpdateFog(RwCamera*, iTime t);
void iCameraSetFogRenderStates();
void iCameraSetFog(bool enable);
RwCamera* RwCameraBeginUpdateWrapper(RwCamera* camera);
void UpdateGlobalFrustumPlanes(RwCamera*);
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

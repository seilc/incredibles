#include "xCommon.h"
#include "xSkyDome.h"

#include "xEvent.h"
#include "iCamera.h"

struct SkyDomeInfo
{
    xEnt* ent;
    S32 sortorder;
    S32 lockXZ;
    S32 lockY;
};

#define MAX_SKYDOME_OBJECTS 8

static SkyDomeInfo sSkyList[MAX_SKYDOME_OBJECTS];
static S32 sSkyCount;

void xSkyDome_EmptyRender(xEnt*)
{
}

void xSkyDome_Setup()
{
    sSkyCount = 0;
}

void xSkyDome_AddEntity(xEnt* ent, S32 sortorder, S32 lockY, S32 lockXZ)
{
    if (xSkyDome_EntityIsSkyDome(ent)) {
        return;
    }

    S32 i, j;
    for (i = 0; i < sSkyCount; i++) {
        if (sSkyList[i].ent == ent) {
            return;
        }
    }
    for (i = 0; i < sSkyCount; i++) {
        if (sortorder < sSkyList[i].sortorder) {
            break;
        }
    }
    for (j = sSkyCount-1; j >= i; j--) {
        sSkyList[j+1] = sSkyList[j];
        sSkyList[j+1].ent->user_data = (void*)(j+1);
    }
    sSkyList[i].ent = ent;
    sSkyList[i].sortorder = sortorder;
    sSkyList[i].lockY = lockY;
    sSkyList[i].lockXZ = lockXZ;
    sSkyCount++;
    ent->user_data = (void*)i;
    ent->render = xSkyDome_EmptyRender;
    ent->model->Flags &= (U16)~0x1;
    ent->baseFlags &= (U16)~k_XBASE_RECEIVES_SHADOWS;

    zEntEvent(ent, eEventCollisionOff);
    zEntEvent(ent, eEventCameraCollideOff);
}

void xSkyDome_Render()
{
    RwCamera* cam = RwCameraGetCurrentCamera();
    RwFrame* camframe = RwCameraGetFrame(cam);
    RwMatrix* cammat = RwFrameGetMatrix(camframe);
    RwReal far_clip = RwCameraGetFarClipPlane(cam);

    RwCameraEndUpdate(cam);
    RwCameraSetFarClipPlane(cam, 10000.0f);

    RwRGBA color = { 0, 0, 0, 0 };
    RwCameraClear(cam, &color, (rwCAMERACLEARZ|rwCAMERACLEARSTENCIL));
    RwCameraBeginUpdateWrapper(cam);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);

    for (S32 i = 0; i < sSkyCount; i++) {
        xEnt* ent = sSkyList[i].ent;
        ent->render = xSkyDome_EmptyRender;
        ent->model->Flags &= (U16)~0x1;

        if (!ent->model || !xEntIsVisible(ent)) continue;

        RwV3d oldpos;
        oldpos = ent->model->Mat->pos;

        if (sSkyList[i].lockXZ) {
            ent->model->Mat->pos.x = cammat->pos.x;
            ent->model->Mat->pos.z = cammat->pos.z;
        }
        if (sSkyList[i].lockY) {
            ent->model->Mat->pos.y = cammat->pos.y;
        }

        if (!xModelCullSingle(ent->model)) {
            iModelRender(ent->model->Data, ent->model->Mat);
        }

        ent->model->Mat->pos = oldpos;
    }

    RwCameraEndUpdate(cam);
    RwCameraSetFarClipPlane(cam, far_clip);
    RwCameraBeginUpdateWrapper(cam);
}

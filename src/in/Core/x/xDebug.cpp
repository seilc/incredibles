#include "xCommon.h"
#include "xDebug.h"

#include "xFont.h"
#include "xGlobals.h"

struct xSB
{
    char* buf;
    U32 max;
    char* cur;
    char* disp;
};

U32 gFrameCount;
U32 gSceneFrameCount;

// UNUSED: needed to match (code from Rat GC)
void xSBRender(xSB* sb)
{
    if (!RwCameraGetCurrentCamera()) {
        return;
    }

    xtextbox::log_layout_stats(false);
    static xtextbox tb = xtextbox::create(
        xfont::create(2, NSCREENX(12.0f), NSCREENY(12.0f), NSCREENX(2.0f), g_GRAY80, screen_bounds, g_BLACK, 2.0f, 2.0f),
        screen_bounds, 0, NSCREENY(2.0f), 0.0f, 0.0f, 0.0f);
    tb.font.width = NSCREENX(12.0f);
    tb.font.height = NSCREENY(12.0f);
    tb.bounds = screen_bounds;
    tb.bounds.contract(NSCREENX(28.0f), NSCREENY(80.0f), NSCREENX(28.0f), NSCREENY(0.0f));
    S32 lineCount = 1;
    for (S32 i = 0; sb->disp[i] != '\0'; i++) {
        if (sb->disp[i] == '\n') {
            lineCount++;
            if (lineCount >= 32) {
                sb->disp[i] = '\0';
                break;
            }
        }
    }
    tb.set_text(sb->disp);
    tb.render(false);
    xtextbox::log_layout_stats(false);
}

void xprintf(const char*, ...)
{
}

void xDebugInit()
{
}

void xDebugUpdate()
{
}

void xDebugExit()
{
}

void xDebugTimestampScreen()
{
}

struct xMtx4x3
{
    union
    {
        F32 m_a[16];
        F32 m_aa[4][4];
        xMat3x3 m_xMat3x3;
        xMat4x3 m_xMat4x3;
        RwMatrix m_RwMatrix;
        struct
        {
            xVec3 right;
            S32 flags;
            xVec3 up;
            U32 pad1;
            xVec3 at;
            U32 pad2;
            xVec3 pos;
            U32 pad3;
        };
    };
};

void xDrawTextInit(U32, U32)
{
}

void xDrawTextRender()
{
}

// UNUSED: needed to match (code from Rat GC)
bool xDrawTextComputeScreenPt(xVec2& vUnitScreen, const xVec3& vWorldSpace)
{
    RwCamera* pCam = RwCameraGetCurrentCamera();
    if (!pCam) {
        return false;
    }

    RwReal fNearClip = RwCameraGetNearClipPlane(pCam);
    RwReal fFarClip = RwCameraGetFarClipPlane(pCam);

    xMtx4x3 mtxView;
    mtxView.m_RwMatrix = *RwCameraGetViewMatrix(pCam);

    xVec3 vCameraSpace;
    xVec3 vScreenSpace;
    RwV3dTransformPoints((RwV3d*)&vCameraSpace, (const RwV3d*)&vWorldSpace, 1, &mtxView.m_RwMatrix);
    F32 fRecipZ = 1.0f / vCameraSpace.z;
    vScreenSpace.x = pCam->frameBuffer->width * (vCameraSpace.x * fRecipZ) + pCam->frameBuffer->nOffsetX;
    vScreenSpace.y = pCam->frameBuffer->height * (vCameraSpace.y * fRecipZ) + pCam->frameBuffer->nOffsetY;
    vScreenSpace.z = fRecipZ * pCam->zScale + pCam->zShift;

    vUnitScreen.x = NSCREENX(vScreenSpace.x);
    vUnitScreen.y = NSCREENY(vScreenSpace.y);

    if (vScreenSpace.x < 0.0f || vScreenSpace.y < 0.0f || vUnitScreen.x > 1.0f || vUnitScreen.y > 1.0f ||
        vCameraSpace.z < fNearClip || vCameraSpace.z > fFarClip) {
        return false;
    }

    return true;
}

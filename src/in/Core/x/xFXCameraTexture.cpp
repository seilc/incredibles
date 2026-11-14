#include "xCommon.h"
#include "xFXCameraTexture.h"

#include "iAsync.h"
#include "zMain.h"
#include "xLightKit.h"
#include "iDraw.h"
#include "zRenderState.h"
#include "xMemMgr.h"
#include "iCamera.h"

#include "decomp.h"

#include <string.h>

bool xFXCameraTexture::create(S32 w, S32 h, bool need_zwrite, S32 bpp, S32 zbpp)
{
    iAsyncLockRWSentry rw_sentry;

    cam = 0;
    raster = bgraster = zraster = 0;
    world = 0;

    memset(&bgcolor, 0, sizeof(bgcolor));

    this->w = w;
    this->h = h;

    cam = RwCameraCreateWrapper();
    if (!cam) { destroy(); return false; }

    RwBBox bbox;
    bbox.sup.x = bbox.sup.y = bbox.sup.z = 100000.0f;
    bbox.inf.x = bbox.inf.y = bbox.inf.z = -100000.0f;
    world = RpWorldCreate(&bbox);
    if (!world) { destroy(); return false; }

    RpWorldAddCamera(world, cam);
    RwCameraSetFrame(cam, RwFrameCreate());

    RwV2d vw = { 1.0f, 1.0f };
    RwCameraSetViewWindow(cam, &vw);

    RwCameraSetProjection(cam, rwPERSPECTIVE);
    RwCameraSetNearClipPlane(cam, 0.3f);
    RwCameraSetFarClipPlane(cam, 10000.0f);

    if (!RwCameraGetFrame(cam)) { destroy(); return false; }

    RwUInt32 rasterFlags;

    rasterFlags = rwRASTERTYPECAMERATEXTURE;
    raster = RwRasterCreate(w, h, bpp, rasterFlags);
    if (!raster) { destroy(); return false; }

    if (!RwCameraSetRaster(cam, raster)) {
        destroy(); return false;
    }

    texture = RwTextureCreate(raster);
    if (!texture) {
        destroy(); return false;
    }

    RwTextureSetFilterMode(texture, rwFILTERLINEAR);

    if (need_zwrite) {
        rasterFlags = rwRASTERTYPECAMERATEXTURE | rwRASTERDONTALLOCATE;
        zraster = RwRasterCreate(w, h, zbpp, rasterFlags);
        if (!zraster) { destroy(); return false; }
    }

    max_tw = log2ceil(w);
    max_th = log2ceil(h);

    return true;
}

void xFXCameraTexture::destroy()
{
    if (texture)
        RwTextureDestroy(texture);
    else if (raster)
        RwRasterDestroy(raster);

    if (zraster)
        RwRasterDestroy(zraster);

    if (cam) {
        RwFrame* tmpframe = RwCameraGetFrame(cam);
        if (tmpframe) {
            RwCameraSetFrame(cam, NULL);
            RwFrameDestroy(tmpframe);
        }

        RpWorldRemoveCamera(world, cam);
        RwCameraDestroy(cam);
    }

    if (world)
        RpWorldDestroy(world);

    cam = 0;
    raster = 0;
    world = 0;
}

void xFXCameraTexture::begin_render(S32 clear_flags, U32 fbmsk_write, U32, F32 near_plane, F32 far_plane)
{
    oldworld = globals.currWorld;
    xLightKit_Enable(NULL, oldworld);

    globals.currWorld = world;

    oldcam = RwCameraGetCurrentCamera();
    if (oldcam)
        RwCameraEndUpdate(oldcam);

    old_near_plane = old_far_plane = 0.0f;

    if (near_plane > 0.0f) {
        if (oldcam)
            old_near_plane = RwCameraGetNearClipPlane(oldcam);
        RwCameraSetNearClipPlane(cam, near_plane);
    }

    if (far_plane > 0.0f) {
        if (oldcam)
            old_far_plane = RwCameraGetFarClipPlane(oldcam);
        RwCameraSetFarClipPlane(cam, far_plane);
    }

    if (fbmsk_write != iDrawGetFBMSK())
        iDrawSetFBMSK(fbmsk_write, 0xFFFFFFFF, true);

    RwCameraClear(cam, (RwRGBA*)&bgcolor, clear_flags);

    RwFrameGetLTM(RwCameraGetFrame(cam))->flags |= rwMATRIXTYPEORTHONORMAL;

    RwCameraBeginUpdateWrapper(cam);

    if ((clear_flags & rwCAMERACLEARIMAGE) && bgraster)
        render_background();
}

void xFXCameraTexture::end_render()
{
    xLightKit_Enable(NULL, world);

    globals.currWorld = oldworld;

    RwCameraEndUpdate(cam);

    if (oldcam) {
        if (old_near_plane > 0.0f)
            RwCameraSetNearClipPlane(oldcam, old_near_plane);

        if (old_far_plane > 0.0f)
            RwCameraSetFarClipPlane(oldcam, old_far_plane);

        RwCameraBeginUpdateWrapper(oldcam);
    }
}

void xFXCameraTexture::render_background()
{
    zRenderState(SDRS_Fill);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)bgraster);

    RwIm2DVertex* vert = (RwIm2DVertex*)xMEMPUSHTEMP(sizeof(RwIm2DVertex) * 6);

    set_vert(vert[0], 0, 0, 0, 0);
    set_vert(vert[1], 0, h, 0, 1);
    set_vert(vert[2], w, 0, 1, 0);
    vert[3] = vert[2];
    vert[4] = vert[1];
    set_vert(vert[5], w, h, 1, 1);

    RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, vert, 6);

    xMEMPOPTEMP(vert);
}

DECOMP_FORCEFLOAT(0.5f)

void xFXCameraTexture::setup_subview(RwCamera* src_cam, const xFRect& src_rect, const xFRect& dst_rect)
{
    RwCameraSetNearClipPlane(cam, RwCameraGetNearClipPlane(src_cam));
    RwCameraSetFarClipPlane(cam, RwCameraGetFarClipPlane(src_cam));
    RwCameraSetFogDistance(cam, RwCameraGetFogDistance(src_cam));
    RwCameraSetProjection(cam, RwCameraGetProjection(src_cam));

    xMat4x3 ltm;
    xVec2 view_window, view_offset;
    get_subview(ltm, view_window, view_offset, src_cam, src_rect, dst_rect);

    RwFrameTransform(RwCameraGetFrame(cam), (const RwMatrix*)&ltm, rwCOMBINEREPLACE);
    RwCameraSetViewWindow(cam, (const RwV2d*)&view_window);
    RwCameraSetViewOffset(cam, (const RwV2d*)&view_offset);
}

void xFXCameraTexture::get_subview(xMat4x3& ltm, xVec2& view_window, xVec2& view_offset, RwCamera* src_cam, const xFRect& src_rect, const xFRect& dst_rect) const
{
    RwRaster* src_raster = RwCameraGetRaster(src_cam);
    const xMat4x3& src_ltm = *(const xMat4x3*)RwFrameGetMatrix(RwCameraGetFrame(src_cam));
    const xVec2& src_view_window = *(const xVec2*)RwCameraGetViewWindow(src_cam);
    const xVec2& src_view_offset = *(const xVec2*)RwCameraGetViewOffset(src_cam);
    F32 src_width = src_raster->width;
    F32 src_height = src_raster->height;
    F32 dst_width = raster->width;
    F32 dst_height = raster->height;
    F32 src_iwidth = 1.0f / src_width;
    F32 src_iheight = 1.0f / src_height;
    F32 dst_iwidth = 1.0f / dst_width;
    F32 dst_iheight = 1.0f / dst_height;
    F32 sx = src_rect.x * src_iwidth - 0.5f;
    F32 sy = src_rect.y * src_iheight - 0.5f;
    F32 sw = src_rect.w * src_iwidth;
    F32 sh = src_rect.h * src_iheight;
    F32 cx = sx * src_view_window.x;
    F32 cy = sy * src_view_window.y;
    F32 cw = sw * src_view_window.x;
    F32 ch = sh * src_view_window.y;
    F32 dx = dst_rect.x * dst_iwidth;
    F32 dy = dst_rect.y * dst_iheight;
    F32 dw = dst_rect.w * dst_iwidth;
    F32 dh = dst_rect.h * dst_iheight;
    F32 rw = cw / dw;
    F32 rh = ch / dh;
    F32 rx = cx - dx * rw;
    F32 ry = cy - dy * rh;
    F32 view_center_x = (0.5f * src_rect.w + src_rect.x) * src_iwidth;
    F32 view_center_y = (0.5f * src_rect.h + src_rect.y) * src_iheight;
    F32 mx_correct = -2.0f * src_view_window.x * (view_center_x - 0.5f);
    F32 my_correct = -2.0f * src_view_window.y * (view_center_y - 0.5f);
    F32 rw_correct = src_rect.w * src_view_window.x * src_iwidth;
    F32 rh_correct = src_rect.h * src_view_window.y * src_iheight;
    F32 mx = -2.0f * (0.5f * rw + rx);
    F32 my = -2.0f * (0.5f * rh + ry);

    *(xMat3x3*)&ltm = src_ltm;
    ltm.pos.x = src_ltm.pos.x + ltm.right.x * mx + ltm.up.x * my;
    ltm.pos.y = src_ltm.pos.y + ltm.right.y * mx + ltm.up.y * my;
    ltm.pos.z = src_ltm.pos.z + ltm.right.z * mx + ltm.up.z * my;

    view_window.assign(rw, rh);
    view_offset.assign(src_view_offset.x + mx, src_view_offset.y - my);
}

namespace {

inline bool clip_rect(S32& x, S32& y, S32& w, S32& h, S32 wclip, S32 hclip)
{
    if (w <= 0 || x >= wclip || x + w <= 0 ||
        h <= 0 || y >= hclip || y + h <= 0)
        return false;

    if (x < 0) { w += x; x = 0; }

    if (x + w > wclip) w = wclip - x;

    if (y < 0) { h += y; y = 0; }

    if (y + h > hclip) h = hclip - y;

    return true;
}

}

bool xFXFastRaster::clip_rect(xFRect& dst_rect, RwRaster* dst_raster)
{
    S32 width = dst_raster->width * 16;
    S32 height = dst_raster->height * 16;
    S32 dst_x = 16.0f * dst_rect.x, dst_w = 16.0f * dst_rect.w, dst_y = 16.0f * dst_rect.y;
    S32 dst_h = 16.0f * dst_rect.h;
    S32 x = dst_x;
    S32 y = dst_y;
    S32 w = dst_w;
    S32 h = dst_h;

    if (!::clip_rect(x, y, w, h, width, height))
        return false;

    if (x != dst_x)
        dst_rect.x = 1/16.0f * x;

    if (w != dst_w)
        dst_rect.w = 1/16.0f * w;

    if (y != dst_y)
        dst_rect.y = 1/16.0f * y;

    if (h != dst_h)
        dst_rect.h = 1/16.0f * h;

    return true;
}

bool xFXFastRaster::clip_rect(xFRect& dst_rect, RwRaster* dst_raster, xFRect& src_rect)
{
    F32 oldx = dst_rect.x, oldw = dst_rect.w;
    F32 oldy = dst_rect.y, oldh = dst_rect.h;

    if (!clip_rect(dst_rect, dst_raster))
        return false;

    if (dst_rect.x != oldx)
        src_rect.x += src_rect.w / oldw * (dst_rect.x - oldx);

    if (dst_rect.w != oldw)
        src_rect.w += src_rect.w / oldw * (dst_rect.w - oldw);

    if (dst_rect.y != oldy)
        src_rect.y += src_rect.h / oldh * (dst_rect.y - oldy);

    if (dst_rect.h != oldh)
        src_rect.h += src_rect.h / oldh * (dst_rect.h - oldh);

    return true;
}

void xFXFastRaster::begin_render()
{
}

void xFXFastRaster::end_render()
{
}

void xFXFastRaster::set_source(RwRaster*, tcc_type, tfx_type, filter_type)
{
}

void xFXFastRaster::set_destination(RwRaster* dst, U32 fbmsk)
{
    this->dst = dst;
    this->fbmsk = fbmsk;
}

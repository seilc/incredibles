#include "xModelBlur.h"

#include "xFXCameraTexture.h"
#include "xDebugTweak.h"
#include "xModel.h"
#include "xLightKit.h"
#include "xDraw.h"
#include "xMath.h"
#include "xMemMgr.h"
#include "iDraw.h"
#include "zMain.h"
#include "zRenderState.h"

#define MAX_PATH_NODES 32

namespace {

    bool sModelBlurEnabled = true;

    namespace tweak {

        xTWEAKABLE bool render = true;
        xTWEAKABLE bool render_texture = false;
        xTWEAKABLE S32 min_alpha = 1;

        void add_tweaks()
        {
        }

    }

    struct interpolator
    {
        F32 value;
        F32 start;
        F32 end;
        F32 t;
        F32 trate;

        void reset(F32 value)
        {
            this->value = start = end = value;
            t = trate = 0.0f;
        }

        void go(F32 value, F32 time)
        {
            if (time <= 0.0f || value == this->value) {
                reset(value);
                return;
            }

            start = this->value;
            end = value;
            t = 0.0f;
            trate = 1.0f / time;
        }

        void update(F32 dt)
        {
            if (trate == 0.0f) return;

            t += trate * dt;
            if (t >= 1.0f) {
                value = start = end;
                t = 0.0f;
                trate = 0.0f;
            } else {
                F32 s = t;
                value = (1.0f - s) * start + s * end;
            }
        }
    };

    struct path_node
    {
        bool just_added;
        F32 age;
        xVec3 center;
    };

    struct card_data
    {
        F32 alpha;
        xVec3 center;
    };

    xFXCameraTexture camtex;

    void render_model_now(xModelInstance* model)
    {
        if ((model->Flags & 0x401) == 0x1) {
            bool reset = false;
            xMat3x3 tmpmat;
            if (model->Scale.x) {
                xMat3x3Copy(&tmpmat, (xMat3x3*)model->Mat);

                model->Mat->right.x *= model->Scale.x;
                model->Mat->right.y *= model->Scale.x;
                model->Mat->right.z *= model->Scale.x;

                model->Mat->up.x *= model->Scale.y;
                model->Mat->up.y *= model->Scale.y;
                model->Mat->up.z *= model->Scale.y;

                model->Mat->at.x *= model->Scale.z;
                model->Mat->at.y *= model->Scale.z;
                model->Mat->at.z *= model->Scale.z;
            }

            xModelCullSingle(model);

            xLightKit_Enable(model->LightKit, xglobals->currWorld);

            if (model->RedMultiplier != 1.0f || model->GreenMultiplier != 1.0f || model->BlueMultiplier != 1.0f) {
                reset = true;
                xModelMaterialMul(model, model->RedMultiplier, model->GreenMultiplier, model->BlueMultiplier);
            }

            U8 alpha = (U8)(255.0f * model->Alpha);
            if (alpha != 255) {
                reset = true;
                xModelSetMaterialAlpha(model, alpha);
            }

            iModelRender(model->Data, model->Mat);

            if (reset) {
                xModelResetMaterial(model);
            }

            if (model->Scale.x) {
                xMat3x3Copy((xMat3x3*)model->Mat, &tmpmat);
            }
        }
    }

}

struct xModelBlur::activity_data
{
    S32 index;
    xModelBlur* owner;
    const config_data* cfg;
    xModelInstance* model;
    xVec3 center;
    path_node path[MAX_PATH_NODES];
    card_data cards[MAX_PATH_NODES];
    S32 path_start;
    S32 path_size;
    S32 cards_size;
    interpolator lifetime;
    interpolator alpha;
};

xModelBlur::activity_data* xModelBlur::activity_buffer;
xModelBlur::activity_data* xModelBlur::activities[MAX_ACTIVE];
S32 xModelBlur::activities_used;

void xModelBlur::scene_enter()
{
    camtex.create(128, 128, true, 32, 32);
    camtex.set_background(g_CLEAR);

    activity_buffer = new (eMemStaticType_0, 0) activity_data[MAX_ACTIVE];

    for (S32 i = 0; i < MAX_ACTIVE; i++) {
        activity_data* activity = activity_buffer + i;
        activities[i] = activity;
        activity->index = i;
        activity->owner = NULL;
    }

    activities_used = 0;

    tweak::add_tweaks();
}

void xModelBlur::scene_exit()
{
    camtex.destroy();
}

void xModelBlur::reset_all()
{
    for (S32 i = 0; i < activities_used; i++) {
        if (activities[i]->owner)
            activities[i]->owner->activity = NULL;
    }

    for (S32 i = 0; i < MAX_ACTIVE; i++) {
        activity_data* activity = activity_buffer + i;
        activities[i] = activity;
        activity->index = i;
        activity->owner = NULL;
    }

    activities_used = 0;
}

void xModelBlur::update_all(F32 dt)
{
    activity_data** activity = activities, **end_activity = activity + activities_used;
    while (activity != end_activity) {
        if ((*activity)->cfg->flags & 0x8) {
            activity++;
        }
        else if ((*activity)->owner->update(dt))
            activity++;
        else
            end_activity--;
    }
}

void xModelBlur::render_all()
{
    if (!sModelBlurEnabled) {
        return;
    }

    activity_data** activity = activities, **end_activity = activity + activities_used;
    for (; activity != end_activity; activity++) {
        if ((*activity)->cfg->flags & 0x4) {
            continue;
        }

        (*activity)->owner->render();
    }
}

const xModelBlur::config_data& xModelBlur::get_default_config()
{
    static const config_data cfg = { 0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.1f, 0.0f, 32 };
    return cfg;
}

void xModelBlur::create()
{
    activity = NULL;
}

bool xModelBlur::activate(F32 lifetime, F32 alpha, const config_data& cfg, xModelInstance* model)
{
    if (activity) {
        activity->lifetime.reset(lifetime);
        activity->alpha.reset(alpha);
        return true;
    }

    if (activities_used >= MAX_ACTIVE)
        return false;

    activity = alloc_activity();
    activity->owner = this;
    activity->cfg = &cfg;
    activity->model = model;
    activity->path_start = 0;
    activity->path_size = 0;
    activity->lifetime.reset(lifetime);
    activity->alpha.reset(alpha);
    return true;
}

void xModelBlur::deactivate()
{
    if (!activity) {
        return;
    }

    activity->owner = NULL;
    free_activity(activity);
    activity = NULL;
}

void xModelBlur::move(const xVec3& center)
{
    if (activity->path_size > 1) {
        S32 second_index = (activity->path_start + 1) & (MAX_PATH_NODES - 1);
        const xVec3& A = activity->path[activity->path_start].center;
        const xVec3& B = activity->path[second_index].center;
        F32 ABx = B.x - A.x, ABy = B.y - A.y, ABz = B.z - A.z;
        F32 dist2 = ABx * ABx + ABy * ABy + ABz * ABz, min_dist = activity->cfg->card_dist, min_dist2 = min_dist * min_dist;
        if (dist2 < min_dist2) {
            activity->path_size--;
            activity->path_start = second_index;
        }
    }

    if (activity->path_size >= MAX_PATH_NODES) {
        activity->path_size = MAX_PATH_NODES - 1;
    }

    activity->path_start = (activity->path_start - 1) & (MAX_PATH_NODES - 1);
    activity->path_size++;

    path_node& p = activity->path[activity->path_start];
    p.just_added = true;
    p.age = 0.0f;
    p.center = center;

    activity->center = center;
}

void xModelBlur::set_config(const config_data& cfg)
{
    activity->cfg = &cfg;
}

bool xModelBlur::visible() const
{
    return activity && (activity->lifetime.value > 0.0f || activity->lifetime.end > 0.0f) && (activity->alpha.value > 0.0f || activity->alpha.end > 0.0f);
}

void xModelBlur::set_lifetime(F32 value, F32 time)
{
    activity->lifetime.go(value, time);
}

F32 xModelBlur::get_lifetime_goal() const
{
    return activity->lifetime.end;
}

void xModelBlur::set_alpha(F32 value, F32 time)
{
    activity->alpha.go(value, time);
}

F32 xModelBlur::get_alpha_goal() const
{
    return activity->alpha.end;
}

bool xModelBlur::update(F32 dt)
{
    activity->lifetime.update(dt);
    activity->alpha.update(dt);

    S32 i;
    F32 lifetime = activity->lifetime.value;
    if (lifetime <= 0.0f && activity->lifetime.end <= 0.0f && !(activity->cfg->flags & 0x2)) {
        deactivate();
        return false;
    }

    for (i = 0; i < activity->path_size; i++) {
        path_node& p = activity->path[(i + activity->path_start) & (MAX_PATH_NODES - 1)];
        if (p.just_added) {
            p.just_added = false;
        } else {
            p.age += dt;
            if (p.age >= lifetime) {
                if (i == 0) {
                    activity->path_size = 1;
                    break;
                }

                path_node& pp = activity->path[(i - 1 + activity->path_start) & (MAX_PATH_NODES - 1)];
                if (pp.age >= lifetime) {
                    activity->path_size = i;
                    break;
                }
            }
        }
    }

    F32 card_dist = activity->cfg->card_dist;
    S32 max_card_renders = activity->cfg->max_card_renders;
    S32 end = activity->path_size - 1;
    F32 alpha_scale = activity->cfg->color_alpha * activity->alpha.value / lifetime;
    card_data* cards = activity->cards;
    S32 cards_size = 0;
    F32 path_dist = 0.0f;

    for (i = 1; i < activity->path_size; i++) {
        if (cards_size >= max_card_renders) break;

        path_node& p0 = activity->path[(i - 1 + activity->path_start) & (MAX_PATH_NODES - 1)];
        path_node& p1 = activity->path[(i + activity->path_start) & (MAX_PATH_NODES - 1)];
        const xVec3& A = p0.center;
        const xVec3& B = p1.center;
        F32 ABx = B.x - A.x, ABy = B.y - A.y, ABz = B.z - A.z;
        F32 segment_dist = xsqrt(ABx * ABx + ABy * ABy + ABz * ABz);
        if (segment_dist < 0.001f) {
            path_dist += segment_dist;
            continue;
        }

        F32 idist = 1.0f / segment_dist;
        F32 age0 = p0.age;
        F32 dage = p1.age - p0.age;
        F32 dist = -path_dist;
        path_dist += segment_dist;

        for (; cards_size < max_card_renders; cards_size++) {
            if (path_dist < card_dist) break;

            dist += card_dist;
            path_dist -= card_dist;
            F32 t = dist * idist;
            F32 age = age0 + dage * t;
            if (age >= lifetime) break;

            card_data& card = cards[cards_size];
            card.alpha = alpha_scale * (lifetime - age);
            card.center.assign(A.x + ABx * t, A.y + ABy * t, A.z + ABz * t);
        }
    }

    activity->cards_size = cards_size;

    if (activity->path_size > i + 1) {
        activity->path_size = i + 1;
    }

    return true;
}

xModelBlur::activity_data* xModelBlur::alloc_activity()
{
    activity_data* activity = activities[activities_used];
    activities_used++;
    return activity;
}

void xModelBlur::free_activity(activity_data* activity)
{
    activities_used--;
    activity_data** tail_activity = activities + activities_used;
    if (tail_activity != &activity) {
        activity_data* temp = *tail_activity;
        activities[activity->index] = temp;
        *tail_activity = activity;
        temp->index = activity->index;
        activity->index = activities_used;
    }
}

static U32* FBSaveMem;
static U32* textureMem;

void xModelBlur::render()
{
    if (!tweak::render_texture) {
        if (!tweak::render) return;
        if (activity->cards_size <= 0) {
            return;
        }
    }

    U32 oldFBMSK = iDrawGetFBMSK();

    xLightKit_Enable(NULL, globals.currWorld);

    RwRaster* draw_raster = RwCameraGetRaster(RwCameraGetCurrentCamera());

    const xVec3& center = activity->center;
    xVec3 view_center;
    RwCamera* world_cam = RwCameraGetCurrentCamera();
    const xMat4x3& view_mat = *(const xMat4x3*)RwCameraGetViewMatrix(world_cam);
    xMat4x3Toworld(&view_center, &view_mat, &center);
    F32 rz = 1.0f / view_center.z;
    F32 screen_x = view_center.x * rz;
    F32 screen_y = view_center.y * rz;
    F32 screen_radius = activity->cfg->radius * rz;
    const xVec2& world_view_window = *(const xVec2*)RwCameraGetViewWindow(world_cam);

    xFRect scr_rect;
    scr_rect.w = FB_XRES * screen_radius / world_view_window.x;
    scr_rect.h = FB_YRES * screen_radius / world_view_window.y;
    scr_rect.x = FB_XRES * screen_x - 0.5f * scr_rect.w;
    scr_rect.y = FB_YRES * screen_y - 0.5f * scr_rect.h;

    if (!xFXFastRaster::clip_rect(scr_rect, draw_raster)) {
        return;
    }

    xFRect tex_rect;
    tex_rect.x = 0.0f;
    tex_rect.y = 0.0f;
    tex_rect.w = MIN(scr_rect.w, 128.0f);
    tex_rect.h = MIN(scr_rect.h, 128.0f);

    camtex.setup_subview(world_cam, scr_rect, tex_rect);

    F32 old_projection[7];
    GXGetProjectionv(old_projection);

    camtex.begin_render(0, 0, 0, 0.0f, 0.0f);

    zRenderState(SDRS_BlurAlpha);

    iXFBreset();
    void* FBSaveMemPopAddr = iXFBalloc(0x10040);
    void* textureMemPopAddr = iXFBalloc(0x10040);

    FBSaveMem = (U32*)(((U32)FBSaveMemPopAddr + 31) & ~31);
    textureMem = (U32*)(((U32)textureMemPopAddr + 31) & ~31);

    GXColor clearColor = { 0, 0, 0, 0 };
    GXSetCopyClear(clearColor, GX_MAX_Z24);

    GXSetTexCopySrc(0, 0, 128, 128);
    GXSetTexCopyDst(128, 128, GX_TF_RGBA8, GX_FALSE);
    GXCopyTex(FBSaveMem, GX_TRUE);
    GXPixModeSync();

    for (xModelInstance* m = activity->model; m; m = m->Next) {
        F32 oldalpha = m->Alpha;
        render_model_now(m);
        m->Alpha = oldalpha;
    }

    camtex.end_render();

    if (tex_rect.w >= 1.0f && tex_rect.h >= 1.0f) {
        GXSetTexCopySrc(0, 0, tex_rect.w, tex_rect.h);
        GXSetTexCopyDst(tex_rect.w, tex_rect.h, GX_TF_RGBA8, GX_FALSE);
        GXCopyTex(textureMem, GX_FALSE);
        GXPixModeSync();
    }

    Mtx44 mProjection;
    MTXOrtho(mProjection, 0.0f, FB_YRES, 0.0f, FB_XRES, 0.0f, 1.0f);
    GXSetProjection(mProjection, GX_ORTHOGRAPHIC);
    GXSetCurrentMtx(GX_PNMTX0);

    Mtx44 mIdentity;
    MTXIdentity(mIdentity);
    GXLoadPosMtxImm(mIdentity, GX_PNMTX0);

    GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);

    GXColor colorAmbient = { 255, 255, 255, 255 };
    GXSetChanAmbColor(GX_COLOR0A0, colorAmbient);

    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetNumTexGens(1);
    GXEnableTexOffsets(GX_TEXCOORD0, GX_FALSE, GX_FALSE);

    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);

    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    GXSetNumChans(0);
    RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);

    GXSetCullMode(GX_CULL_NONE);

    GXSetViewport(0.0f, 0.0f, FB_XRES, FB_YRES, 0.0f, 1.0f);
    GXSetScissor(0, 0, FB_XRES, FB_YRES);

    GXTexObj screenBack ATTRIBUTE_ALIGN(16);
    GXInitTexObj(&screenBack, FBSaveMem, 128, 128, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);

    GXSetNumTexGens(1);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXLoadTexObj(&screenBack, GX_TEXMAP0);

    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);

    GXPosition2f32(0.0f, 0.0f);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition2f32(0.0f, 128.0f);
    GXTexCoord2f32(0.0f, 1.0f);
    GXPosition2f32(128.0f, 0.0f);
    GXTexCoord2f32(1.0f, 0.0f);
    GXPosition2f32(128.0f, 128.0f);
    GXTexCoord2f32(1.0f, 1.0f);

    GXEnd();

    iDrawSetFBMSK(oldFBMSK, 0xFFFFFFFF, true);

    if (tweak::render_texture) {
        xDrawCard(camtex.raster, tex_rect, scr_rect, true);
    }

    if (tex_rect.w >= 1.0f && tex_rect.h >= 1.0f) {
        if (tweak::render) {
            F32 xoff0 = view_center.z * (scr_rect.x - FB_XRES * screen_x);
            F32 xoff1 = view_center.z * (scr_rect.x + scr_rect.w - FB_XRES * screen_x);
            F32 yoff0 = view_center.z * (scr_rect.y - FB_YRES * screen_y);
            F32 yoff1 = view_center.z * (scr_rect.y + scr_rect.h - FB_YRES * screen_y);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
            render_cards(tex_rect, xoff0, yoff0, xoff1, yoff1);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
        }
    }

    GXSetProjectionv(old_projection);

    GXDrawDone();

    zRenderState(SDRS_AlphaModels);
}

void xModelBlur::render_cards(const xFRect& src_rect, F32 xoff0, F32 yoff0, F32 xoff1, F32 yoff1)
{
    RwRaster* draw_raster = RwCameraGetRaster(RwCameraGetCurrentCamera());

    xFXFastRaster fr;
    fr.begin_render();
    fr.set_destination(draw_raster, iDrawGetFBMSK());
    fr.set_source(camtex.raster, xFXFastRaster::TCC_DEFAULT, xFXFastRaster::TFX_DEFAULT, xFXFastRaster::FILTER_LINEAR);

    S32 maxu = (S32)src_rect.w - 1;
    S32 maxv = (S32)src_rect.h - 1;

    F32 radius = activity->cfg->radius;
    RwCamera* world_cam = RwCameraGetCurrentCamera();
    const xMat4x3& view_mat = *(const xMat4x3*)RwCameraGetViewMatrix(world_cam);
    const xVec2& vw = *(const xVec2*)RwCameraGetViewWindow(world_cam);
    xFColor color = { activity->cfg->color_red, activity->cfg->color_green, activity->cfg->color_blue, 0.0f };

    F32 max_component = 0.99999f;
    if (color.r > max_component) color.r = max_component;
    if (color.g > max_component) color.g = max_component;
    if (color.b > max_component) color.b = max_component;

    F32 zscale = world_cam->zScale;
    F32 zbias = activity->cfg->zbias * (RwCameraGetFarClipPlane(world_cam) - RwCameraGetNearClipPlane(world_cam)) + world_cam->zShift;

    for (S32 i = 0; i < activity->cards_size; i++) {
        const card_data& card = activity->cards[i];
        color.a = card.alpha;
        if (color.a < tweak::min_alpha / 255.0f) {
            break;
        }

        xVec3 sp;
        xMat4x3Toworld(&sp, &view_mat, &card.center);
        if (sp.z < 0.01f) {
            continue;
        }

        F32 rz = 1.0f / sp.z;
        F32 blitz = rz * zscale + zbias;
        F32 cx = FB_XRES * sp.x;
        F32 cy = FB_YRES * sp.y;
        F32 x0 = rz * (cx + xoff0);
        F32 x1 = rz * (cx + xoff1);
        F32 y0 = rz * (cy + yoff0);
        F32 y1 = rz * (cy + yoff1);

        xFRect clip_src_rect = src_rect;
        xFRect clip_dst_rect = { x0, y0, x1 - x0, y1 - y0 };
        if (!fr.clip_rect(clip_dst_rect, clip_src_rect)) {
            continue;
        }

        if (activity->cfg->flags & 0x10) {
            GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
        } else {
            GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
        }

        GXSetZMode(GX_TRUE, GX_LESS, GX_FALSE);

        GXSetNumChans(1);

        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);

        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

        GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
        GXSetNumTexGens(1);
        GXEnableTexOffsets(GX_TEXCOORD0, GX_FALSE, GX_FALSE);
        GXSetChanCtrl(GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);

        RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);

        GXSetCullMode(GX_CULL_NONE);

        GXTexObj screenBack ATTRIBUTE_ALIGN(16);
        GXInitTexObj(&screenBack, textureMem, src_rect.w, src_rect.h, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);

        GXSetNumTevStages(1);
        GXLoadTexObj(&screenBack, GX_TEXMAP0);

        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);

        GXColor colorAmbient = { 255, 255, 255, 255 };
        GXSetChanAmbColor(GX_COLOR0A0, colorAmbient);

        F32 projVector[7] = { 1.0f, 2.0f / FB_XRES, -1.0f, -2.0f / FB_YRES, 1.0f, -1.0f, -1.0f };
        GXSetProjectionv(projVector);

        F32 posMatrix[3][4] = {
            { 1.0f, 0.0f, 0.0f, 0.5f },
            { 0.0f, 1.0f, 0.0f, 0.5f },
            { 0.0f, 0.0f, -1.0f, 0.0f }
        };
        GXLoadPosMtxImm(posMatrix, GX_PNMTX0);

        GXSetViewport(0.0f, 0.0f, FB_XRES, FB_YRES, 0.0f, 1.0f);
        GXSetScissor(0, 0, FB_XRES, FB_YRES);

        GXSetCurrentMtx(GX_PNMTX0);

        U8 r = (U8)(255.99 * color.r);
        U8 g = (U8)(255.99 * color.g);
        U8 b = (U8)(255.99 * color.b);
        U8 a = (U8)(255.99 * color.a);

        GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);

        F32 blitzGC = rz * zscale + world_cam->zShift - 0.01f * (activity->cfg->zbias / 50.0f);
        GXPosition3f32(clip_dst_rect.x, clip_dst_rect.y, blitzGC);
        GXColor4u8(r, g, b, a);
        GXTexCoord2f32(0.0f, 0.0f);
        GXPosition3f32(clip_dst_rect.x, clip_dst_rect.y + clip_dst_rect.h, blitzGC);
        GXColor4u8(r, g, b, a);
        GXTexCoord2f32(0.0f, 1.0f);
        GXPosition3f32(clip_dst_rect.x + clip_dst_rect.w, clip_dst_rect.y, blitzGC);
        GXColor4u8(r, g, b, a);
        GXTexCoord2f32(1.0f, 0.0f);
        GXPosition3f32(clip_dst_rect.x + clip_dst_rect.w, clip_dst_rect.y + clip_dst_rect.h, blitzGC);
        GXColor4u8(r, g, b, a);
        GXTexCoord2f32(1.0f, 1.0f);

        GXEnd();
    }

    fr.end_render();
}

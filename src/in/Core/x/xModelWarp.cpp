#include "xModelWarp.h"

#include "xFXCameraTexture.h"

namespace
{
    struct render_context
    {
        xMat4x3 uvmat;
        S32 flags;
        xBox box;
        xFColor color;
        RpAtomic* atomic;
        RpAtomic* original_atomic;
        RpAtomicCallBackRender original_render_callback;
    };

    xFXCameraTexture camtex;
    render_context render_contexts[8];
    S32 render_contexts_size = 0;

    namespace tweak
    {
        void add_tweaks()
        {
        }
    } // namespace tweak

    void get_screen_rect(xFRect& rect, const xMat4x3& mat, const xBox& box); // UNUSED

    void destroy_render_context(render_context& context)
    {
        RpAtomicSetRenderCallBack(context.original_atomic, context.original_render_callback);
    }

    U8 write_screen(xFRect& scr_rect, const xFColor& color); // UNUSED
    RpAtomic* render_callback(RpAtomic* atomic); // UNUSED
} // namespace

void xModelWarpSceneEnter()
{
    camtex.create(512, 256, false, 32, 32);
    camtex.set_background(g_CLEAR);

    render_contexts_size = 0;

    tweak::add_tweaks();
}

void xModelWarpSceneExit()
{
    render_context* context = render_contexts, *end_context = context + render_contexts_size;
    for (; context != end_context; context++)
        destroy_render_context(*context);
    render_contexts_size = 0;

    camtex.destroy();
}

void xModelWarpSphereMapUVsByNormals(xVec2* uv, S32 verts_size, const xVec3* norm)
{
    xVec2* end_uv = uv + verts_size;

    for (; uv != end_uv; uv++, norm++) {
        uv->x = xasin(norm->x) * (1/PI) + 0.5f;
        uv->y = -(xasin(norm->y) * (1/PI) + 0.5f);
    }
}
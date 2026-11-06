#include "xHud.h"

#include "xHudAsset.h"
#include "xHudFontMeter.h"
#include "xHudImage.h"
#include "xHudModel.h"
#include "xHudText.h"
#include "xHudUnitMeter.h"
#include "xDebugTweak.h"
#include "xCam.h"
#include "xEvent.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xLightKit.h"
#include "xMath.h"
#include "xMemMgr.h"
#include "xModelAsset.h"
#include "xString.h"
#include "xstransvc.h"
#include "xutil.h"
#include "iCamera.h"
#include "zBase.h"
#include "zEnt.h"
#include "zScene.h"

#include "decomp.h"

namespace xhud {

namespace {

const struct
{
    U8 type;
    U32 size;
} known_types[] = {
    { eBaseTypeHUD_image, sizeof(image_widget::widget_chunk) },
    { eBaseTypeHUD_model, sizeof(model_widget::widget_chunk) },
    { eBaseTypeHUD_unit_meter, sizeof(unit_meter_widget::widget_chunk) },
    { eBaseTypeHUD_font_meter, sizeof(font_meter_widget::widget_chunk) },
    { eBaseTypeHUD_text, sizeof(text_widget::widget_chunk) }
};

}

struct header
{
    holder* next;
};

struct holder
{
    header h;
    U8 data[1];
};

void block_allocator::create(S32 block_size, S32 max_blocks)
{
    this->block_size = ((block_size + 3) & ~3) + sizeof(header);
    this->max_blocks = max_blocks;
    buffer = xMEMALLOC(this->block_size * this->max_blocks, 0);
    reset();
}

void* block_allocator::alloc()
{
    holder* obj = head;
    head = head->h.next;
    return obj->data;
}

void block_allocator::free(void* mem)
{
    holder* obj = (holder*)((U8*)mem - sizeof(header));
    obj->h.next = head;
    head = obj;
}

void block_allocator::reset()
{
    U8* mem = (U8*)buffer, *end_mem = mem + (max_blocks - 1) * block_size;

    for (; mem != end_mem; mem += block_size) {
        ((holder*)mem)->h.next = (holder*)(mem + block_size);
    }

    ((holder*)mem)->h.next = NULL;

    head = (holder*)buffer;
}

block_allocator widget::motive_allocator;

static bool inited = false;

void init()
{
    if (!inited) {
        inited = true;
    } else
        widget::disable_all(true);
}

void destroy()
{
    xTWEAK_REMOVE("HUD");
    widget::disable_all(true);
}

void update(F32 dt)
{
    widget::update_all(dt);
}

void render()
{
    if (inited)
        widget::render_all();
}

widget::widget(const asset& a)
    : a(&a)
    , _motive_top(NULL)
    , _motive_temp(NULL)
    , _motive_temp_tail(NULL)
{
    flag.visible = true;
    flag.enabled = false;
    rc.loc.x = NSCREENX(a.loc.x);
    rc.loc.y = NSCREENY(a.loc.y);
    rc.loc.z = a.loc.z;
    rc.size.x = NSCREENX(a.size.x);
    rc.size.y = NSCREENY(a.size.y);
    rc.r = rc.g = rc.b = rc.a = 1.0f;
    start_rc = rc;
}

struct motive_node
{
    motive m;
    motive_node* next;
};

void widget::init_base(xBase& data, const xBaseAsset& asset, size_t chunk_size)
{
    widget_chunk& wc = (widget_chunk&)data;
    xBaseInit(&wc, &asset);
    wc.eventFunc = cb_dispatch;
    if (wc.linkCount) wc.link = (xLinkAsset*)((U8*)&asset + chunk_size);
}

void widget::destruct() { disable(); }

void widget::presetup()
{
    activity = ACT_NONE;
    add_tweaks();
}

void widget::updater(F32 dt)
{
    if (flag.active) {
        set_timer(dt + get_timer());
    }

    _motive_temp_tail = &_motive_temp;

    for (motive_node** itp = &_motive_top, *it = *itp; it;) {
        if (!it->m.update(*this, dt)) {
            *itp = it->next;
            if (it == _motive_top)
                _motive_top = it->next;
            motive_allocator.free(it);
        } else {
            itp = &it->next;
            if (it->m.hold) break;
        }
        it = *itp;
    }

    if (_motive_temp) {
        *_motive_temp_tail = _motive_top;
        _motive_top = _motive_temp;
        _motive_temp = NULL;
    }

    _motive_temp_tail = NULL;

    if (!_motive_top) activity = ACT_NONE;
}

void widget::dispatcher(xBase*, U32 event, const F32*, xBase*, U32)
{
    switch (event) {
    case eEventEnable:
        enable();
        break;
    case eEventDisable:
        disable();
        break;
    case eEventVisible:
    case eEventFastVisible:
        flag.visible = true;
        break;
    case eEventInvisible:
    case eEventFastInvisible:
        flag.visible = false;
        break;
    case eEventDispatcher_ShowHud:
        show(LEFT, true, false);
        break;
    case eEventDispatcher_HideHud:
        hide(LEFT, true, false);
        break;
    }
}

void widget::scene_enter()
{
    motive_allocator.create(sizeof(motive_node), 100);
}

U32 widget::type() const
{
    static U32 myid = xStrHash(asset::type_name());
    return myid;
}

bool widget::is(U32 id) const
{
    return id == widget::type();
}

void widget::show(move_direction dir, bool clearmotives, bool bSkipShowMotive)
{
    if (clearmotives) {
        clear_motives();
    }

    activity = ACT_SHOW;

    F32 dx = start_rc.loc.x - rc.loc.x;
    F32 dy = start_rc.loc.y - rc.loc.y;
    F32 d2 = dx * dx + dy * dy;

    if (d2 <= 0.0001f * 0.0001f || bSkipShowMotive) {
        rc.loc = start_rc.loc;
        rc.a = start_rc.a;
        return;
    }

    F32 d = xsqrt(d2);
    F32 vx = 10.0f * dx;
    F32 vy = 10.0f * dy;
    F32 a = -(vx * vx + vy * vy) / (2.0f * d);
    F32 ax = a * dx / d;
    F32 ay = a * dy / d;

    switch (dir) {
    case LEFT:
    case RIGHT:
        add_motive(motive(&rc.loc.x, vx, dx, ax, accelerate_motive_update, NULL, 1.0f, false, false, NULL));
        break;
    case TOP:
    case BOTTOM:
        add_motive(motive(&rc.loc.y, vy, dy, ay, accelerate_motive_update, NULL, 1.0f, true, false, NULL));
        break;
    }

    F32 da = start_rc.a - rc.a;
    add_motive(motive(&rc.a, 3.0f * da, da, 0.0f, linear_motive_update, NULL, 1.0f, false, false, NULL));
}

void widget::hide(move_direction d, bool clearmotives, bool bSkipHideMotive)
{
    if (clearmotives) {
        clear_motives();
    }

    activity = ACT_HIDE;

    F32 x = start_rc.loc.x - 0.5f;
    F32 y = start_rc.loc.y - 0.5f;
    F32 sx = start_rc.size.x;
    F32 sy = start_rc.size.y;
    F32 cx = x + 0.5f * sx;
    F32 cy = y + 0.5f * sy;
    F32 tx, ty, tcx, tcy;
    F32 acx = xabs(cx), acy = xabs(cy);
    F32 dist, vx, vy, ex, ey;

    if (xabs(acx + acy) <= 0.0001f) {
        rc.a = 0.0f;
        return;
    }

    if (acx > acy) {
        tcx = cx >= 0.0f ? 0.5f + sx : -0.5f - sx;
        tcy = tcx * cy / cx;
    } else {
        tcy = cy >= 0.0f ? 0.5f + sy : -0.5f - sy;
        tcx = tcy * cx / cy;
    }

    tx = tcx - 0.5f * sx;
    ty = tcy - 0.5f * sy;

    ex = 0.5f + tx - rc.loc.x;
    ey = 0.5f + ty - rc.loc.y;
    dist = xsqrt(ex * ex + ey * ey);
    vx = dist * (5.0f * ex);
    vy = dist * (5.0f * ey);

    switch (d) {
    case LEFT:
    case RIGHT:
        if (bSkipHideMotive)
            rc.loc.x = ex;
        else
            add_motive(motive(&rc.loc.x, 0.0f, ex, 5.0f * vx, accelerate_motive_update, NULL, 2.0f, false, false, NULL));
        break;
    case TOP:
    case BOTTOM:
        if (bSkipHideMotive)
            rc.loc.y = ey;
        else
            add_motive(motive(&rc.loc.y, 0.0f, ey, 5.0f * vy, accelerate_motive_update, NULL, 2.0f, false, false, NULL));
        break;
    }

    if (bSkipHideMotive) {
        rc.a = 0.0f;
        update(0.0f);
    } else
        add_motive(motive(&rc.a, -rc.a, -rc.a, 0.0f, linear_motive_update, NULL, 2.0f, false, false, NULL));
}

namespace {

template <class F>
void for_each(U8 type, U32 size, F f)
{
    zScene& s = *xglobals->sceneCur;
    U8* it = (U8*)s.baseList[type];
    U8* end = it + s.baseCount[type] * size;
    for (; it != end; it += size) {
        widget_chunk* wc = (widget_chunk*)it;
        f(wc->w);
    }
}

struct functor_disable
{
    bool destroy_widgets;

    functor_disable(bool destroy_widgets) : destroy_widgets(destroy_widgets) {}

    void operator()(widget& w)
    {
        w.disable();
        if (destroy_widgets) w.destroy();
    }
};

struct functor_update
{
    F32 dt;

    functor_update(F32 dt) : dt(dt) {}
    void operator()(widget& w) { if (w.enabled()) w.update(dt); }
};

// UNUSED: ported from BFBB GC
void fp_setup(widget& w)
{
    w.setup();
}

void fp_render(widget& w)
{
    if (w.visible()) {
        w.render();
    }
}

void fp_show(widget& w); // UNUSED
void fp_hide(widget& w); // UNUSED

}

void widget::disable_all(bool destroy_widgets)
{
    functor_disable f(destroy_widgets);
    for (size_t i = 0; i < ARRAY_LENGTH(known_types); i++)
        for_each(known_types[i].type, known_types[i].size, f);
}

// UNUSED: ported from BFBB GC
void widget::setup_all()
{
    for (size_t i = 0; i < ARRAY_LENGTH(known_types); i++) {
        for_each(known_types[i].type, known_types[i].size, fp_setup);
    }
}

void widget::update_all(F32 dt)
{
    functor_update f(dt);
    for (size_t i = 0; i < ARRAY_LENGTH(known_types); i++)
        for_each(known_types[i].type, known_types[i].size, f);
}

void widget::render_all()
{
    iCameraSetOrthoProjection(xglobals->screen->icam, true);

    for (size_t i = 0; i < ARRAY_LENGTH(known_types); i++) {
        for_each(known_types[i].type, known_types[i].size, fp_render);
    }

    iCameraSetOrthoProjection(xglobals->screen->icam, false);
}

void widget::add_motive(const motive& m)
{
    motive_node* n = (motive_node*)motive_allocator.alloc();
    new (&n->m) motive(m);

    if (!_motive_temp_tail) {
        n->next = _motive_top;
        _motive_top = n;
    } else {
        if (!_motive_temp) _motive_temp_tail = &n->next;
        n->next = _motive_temp;
        _motive_temp = n;
    }
}

void widget::clear_motives()
{
    activity = ACT_NONE;

    for (motive_node** itp = &_motive_top, *it = *itp; it;) {
        it->m.finish(*this);
        *itp = it->next;
        motive_allocator.free(it);
        it = *itp;
    }
}

bool widget::active()
{
    for (motive_node** itp = &_motive_top, *it = *itp; it;) {
        if (it->m.active()) return true;
        *itp = it->next;
        it = *itp;
    }
    return false;
}

void widget::cb_dispatch(xBase* from, xBase* to, U32 event, const F32* argf, xBase* argw, U32 argwi)
{
    widget_chunk* wc = (widget_chunk*)to;
    widget* w = &wc->w;
    w->dispatch(from, event, argf, argw, argwi);
}

DECOMP_FORCEACTIVE("%d")

xModelInstance* load_model(U32 id)
{
    U32 bufsize;
    xModelAssetInfo* minf = (xModelAssetInfo*)xSTFindAsset(xStrHashCat(id, ".minf"), &bufsize);
    if (minf) return zEntRecurseModelInfo(minf, NULL);

    RpAtomic* imodel = (RpAtomic*)xSTFindAsset(id, &bufsize);
    if (!imodel)
        imodel = (RpAtomic*)xSTFindAsset(xStrHashCat(id, ".dff"), &bufsize);
    if (!imodel)
        return NULL;

    return xModelInstanceAlloc(imodel, NULL, 0, 0, NULL);
}

namespace {

void render_one_model(xModelInstance& m, F32 alpha, const basic_rect<F32>& r, const xVec3& from, const xVec3& to, const xMat4x3& frame)
{
    if (m.LightKit) {
        xLightKit_Enable(m.LightKit, xglobals->currWorld);
    }

    xModelSetMaterialAlpha(&m, (U8)(255.0f * alpha + 0.5f));

    xModelSetFrame(&m, &frame);

    xModelRender2D(m, r, from, to);

    if (m.LightKit) {
        xLightKit_Enable(NULL, xglobals->currWorld);
    }

    xModelSetMaterialAlpha(&m, 255);
}

}

void render_model(xModelInstance& m, const render_context& rc)
{
    basic_rect<F32> r = { rc.loc.x, rc.loc.y, rc.size.x, rc.size.y };
    xVec3 from = { 0.0f, 0.0f, 1.0f };
    xVec3 to = { 0.0f, 0.0f, -rc.loc.z };

    xMat4x3 frame;
    xMat3x3Euler(&frame, rc.rot.x, rc.rot.y, rc.rot.z);
    frame.right *= 1.0f + rc.loc.z;
    frame.up *= 1.0f + rc.loc.z;
    frame.at *= 0.01f;
    frame.pos.x = frame.pos.y = frame.pos.z = 0.0f;
    frame.flags = 0;

    for (xModelInstance* model = &m; model; model = model->Next)
        render_one_model(*model, rc.a, r, from, to, frame);
}

bool linear_motive_update(widget& w, motive& m, F32 dt)
{
    if (!m.update_time(dt)) {
        return false;
    }

    F32 diff = dt * m.delta;
    F32 remaining;

    if (!m.inverse) {
        remaining = m.max_offset - m.offset;
        if ((diff >= 0.0f && diff >= remaining) || (diff < 0.0f && diff <= remaining)) {
            *m.value += remaining;
            m.offset = m.max_offset;
            if (m.fp_finish) {
                m.fp_finish(w, m);
            }
            return false;
        }
        *m.value += diff;
        m.offset += diff;
    } else {
        remaining = m.max_offset - m.offset;
        if (remaining <= 0.0f) {
            *m.value -= remaining;
            m.offset = m.max_offset;
            if (m.fp_finish) {
                m.fp_finish(w, m);
            }
            return false;
        }
        *m.value -= diff;
        m.offset += diff;
    }

    return true;
}

bool accelerate_motive_update(widget&, motive& m, F32 dt)
{
    if (!m.update_time(dt)) {
        return false;
    }

    F32 diff = dt * (0.5f * m.accel * dt) + m.delta * dt;
    m.delta += dt * m.accel;
    F32 remaining = m.max_offset - m.offset;

    if ((diff >= 0.0f && diff >= remaining) || (diff < 0.0f && diff <= remaining)) {
        *m.value += remaining;
        m.offset = m.max_offset;
        return false;
    }

    *m.value += diff;
    m.offset += diff;
    return true;
}

}

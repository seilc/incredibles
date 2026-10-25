#include "xLightVolume.h"

#include "xMemMgr.h"
#include "xDebug.h"
#include "iModel.h"

#define MAX_ACTIVE 64

struct xLightVolume::activity_type
{
    xLightVolume* owner;
};

xLightVolume::activity_type* xLightVolume::activities = NULL;
S32 xLightVolume::activities_used = 0;

void xLightVolume::scene_enter()
{
    activities = new (eMemStaticTypeUnk0, 0) activity_type[MAX_ACTIVE];
    activities_used = 0;
    reset_all();
}

void xLightVolume::scene_exit()
{
}

void xLightVolume::reset_all()
{
    activity_type* act = activities;
    activity_type* end_act = act + activities_used;

    for (; act != end_act; act++) {
        xASSERT(56, act->owner != 0);
        act->owner->activity = 0;
    }

    activities_used = 0;
}

void xLightVolume::create()
{
    activity = 0;
}

bool xLightVolume::activate()
{
    xASSERT(73, activity == 0);

#if DEBUG || RELEASE
    activity_type* act = activities;
    activity_type* end_act = act + activities_used;

    for (; act != end_act; act++) {
        xASSERT(80, (act->owner != this) && "Added light volume more than once.");
    }
#endif

    if (activities_used >= MAX_ACTIVE) {
        return false;
    }

    activity = &activities[activities_used];
    activities_used++;
    activity->owner = this;

    return true;
}

void xLightVolume::deactivate()
{
    xASSERT(98, activity != 0);
    xASSERT(99, (activity >= activities) && (activity < (activities + activities_used)));
    xASSERT(100, activity->owner == this);

    activities_used--;

    activity_type* tail_activity = &activities[activities_used];

    if (activity != tail_activity) {
        *activity = *tail_activity;
        activity->owner->activity = activity;
    }

    activity = 0;
}

void xLightVolumeModel::create()
{
    model = 0;
    color = g_WHITE;
    inside = false;
    single = false;

    xLightVolume::create();
}

void xLightVolumeModel::render()
{
    xASSERT(__LINE__, model != 0);

    if (single) {
        render_single(model, 0);
    } else {
        xModelInstance* m = model;

        while (m) {
            if ((m->Flags & 0x401) == 0x1) {
                render_single(m, 0);
            }

            m = m->Next;
        }
    }
}

void xLightVolumeModel::render_single(xModelInstance* model, U32 stencil_pass)
{
    RpAtomic* atomic = model->Data;
    RpHAnimHierarchy* hierarchy = iModelGetHierarchy(atomic);
    RwMatrix* old_anim_mat;

    if (hierarchy) {
        old_anim_mat = hierarchy->pMatrixArray;
        hierarchy->pMatrixArray = model->Mat + 1;
    }

    xMat4x3& mat = *(xMat4x3*)model->Mat;
    xMat4x3 old_mat;

    if (model->Scale.x != 0.0f) {
        old_mat = mat;
        xMat3x3MulScale(&mat, &mat, &model->Scale);
    }

    RwFrame* frame = RpAtomicGetFrame(atomic);
    frame->ltm = (RwMatrix&)mat;

    if (!xModelCullSingle(model)) {
        render_atomic(atomic, color, inside, stencil_pass);
    }

    if (model->Scale.x != 0.0f) {
        mat = old_mat;
    }

    if (hierarchy) {
        hierarchy->pMatrixArray = old_anim_mat;
    }
}

void xLightVolume::render_all()
{
}

void xLightVolume::render_atomic(RpAtomic*, xColor, bool, U32)
{
}

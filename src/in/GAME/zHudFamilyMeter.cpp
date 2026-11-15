#include "xCommon.h"
#include "zHudFamilyMeter.h"

#include "zScene.h"
#include "xString.h"

namespace zHud
{
    static F32 fMinHealthBar = 0.0f;
    static F32 fMaxHealthBar = 1.0f;

    FamilyMeter::FamilyMeter()
        : hud_element(), m_pHealthImage(NULL), m_pHeadImage(NULL), m_fHealthCurrent(1.0f),
          m_Head(HEAD_NONE)
    {
    }

    void FamilyMeter::reset()
    {
        m_pHealthImage->clear_motives();
        m_pHeadImage->clear_motives();
    }

    void FamilyMeter::setup()
    {
        U32 HealthImageID = xStrHash("FAMILYHEALTH");
        U32 HeadImageID = xStrHash("FAMILYHEAD");
        RwTexCoords upperleft, lowerright;

        m_pHealthImage = (xhud::image_widget*)&((xhud::widget_chunk*)zSceneFindObject(HealthImageID))->w;
        m_pHeadImage = (xhud::image_widget*)&((xhud::widget_chunk*)zSceneFindObject(HeadImageID))->w;

        m_pHealthImage->init_layers(2, 8, 12);

        upperleft.u = 0.0f;
        upperleft.v = 0.01171875f;
        lowerright.u = 1.0f;
        lowerright.v = 0.0859375f;

        m_pHealthImage->set_layer_uvs(0, upperleft, lowerright);
        m_pHealthImage->set_layer_type(0, xhud::image_widget::VARIABLE_RIGHT,
                                       xhud::image_widget::BLEND);
        m_pHealthImage->set_layer_value(0, 1.0f);
        m_pHealthImage->set_layer_alpha(0, 1.0f);

        upperleft.u = 0.0f;
        upperleft.v = 0.12890625f;
        lowerright.u = 1.0f;
        lowerright.v = 0.203125f;

        m_pHealthImage->set_layer_uvs(1, upperleft, lowerright);
        m_pHealthImage->set_layer_type(1, xhud::image_widget::VARIABLE_RIGHT,
                                       xhud::image_widget::BLEND);
        m_pHealthImage->set_layer_value(1, 1.0f);
        m_pHealthImage->set_layer_alpha(1, 1.0f);
        m_pHealthImage->set_layer_minmax(1, fMinHealthBar, fMaxHealthBar);

        m_pHeadImage->init_layers(HEAD_NUM_LAYERS, 12, 18);

        upperleft.u = 0.5234375f;
        upperleft.v = 0.57421875f;
        lowerright.u = 0.7265625f;
        lowerright.v = 1.0f;

        m_pHeadImage->set_layer_uvs(HEAD_FROZONE, upperleft, lowerright);
        m_pHeadImage->set_layer_type(HEAD_FROZONE, xhud::image_widget::OFFSET_CENTERPT,
                                     xhud::image_widget::BLEND);
        m_pHeadImage->set_layer_value(HEAD_FROZONE, 1.0f);
        m_pHeadImage->set_layer_alpha(HEAD_FROZONE, 0.0f);
        m_pHeadImage->set_layer_scale(HEAD_FROZONE, 1.0f, 1.0f);

        upperleft.u = 0.6328125f;
        upperleft.v = 0.203125f;
        lowerright.u = 0.9921875f;
        lowerright.v = 0.5625f;

        m_pHeadImage->set_layer_uvs(HEAD_MRS_I, upperleft, lowerright);
        m_pHeadImage->set_layer_type(HEAD_MRS_I, xhud::image_widget::OFFSET_CENTERPT,
                                     xhud::image_widget::BLEND);
        m_pHeadImage->set_layer_value(HEAD_MRS_I, 1.0f);
        m_pHeadImage->set_layer_alpha(HEAD_MRS_I, 0.0f);
        m_pHeadImage->set_layer_scale(HEAD_MRS_I, 1.0f, 1.0f);

        upperleft.u = 0.03515625f;
        upperleft.v = 0.23046875f;
        lowerright.u = 0.50390625f;
        lowerright.v = 0.69921875f;

        m_pHeadImage->set_layer_uvs(HEAD_DASH_VIOLET, upperleft, lowerright);
        m_pHeadImage->set_layer_type(HEAD_DASH_VIOLET, xhud::image_widget::OFFSET_CENTERPT,
                                     xhud::image_widget::BLEND);
        m_pHeadImage->set_layer_value(HEAD_DASH_VIOLET, 1.0f);
        m_pHeadImage->set_layer_alpha(HEAD_DASH_VIOLET, 0.0f);
        m_pHeadImage->set_layer_scale(HEAD_DASH_VIOLET, 1.0f, 1.0f);

        m_pHealthImage->setup();
        m_pHeadImage->setup();

        reset();
        set_head(HEAD_NONE);
        set_health(1.0f);
    }

    void FamilyMeter::destroy()
    {
    }

    void FamilyMeter::update(F32)
    {
    }

    void FamilyMeter::show()
    {
        m_pHealthImage->show(xhud::widget::TOP, true, true);
        m_pHeadImage->show(xhud::widget::TOP, true, true);
    }

    void FamilyMeter::hide()
    {
        m_pHealthImage->hide(xhud::widget::TOP, true, true);
        m_pHeadImage->hide(xhud::widget::TOP, true, true);
    }

    void FamilyMeter::enable(bool bEnable)
    {
        if (bEnable) {
            m_pHealthImage->enable();
            m_pHeadImage->enable();
            show();
        } else {
            m_pHealthImage->disable();
            m_pHeadImage->disable();
            hide();
        }
    }

    void FamilyMeter::set_head(eFamilyMeterHeadLayer StageCount)
    {
        m_Head = StageCount;

        switch (m_Head) {
        case HEAD_NONE:
            m_pHeadImage->set_layer_alpha(HEAD_FROZONE, 0.0f);
            m_pHeadImage->set_layer_alpha(HEAD_MRS_I, 0.0f);
            m_pHeadImage->set_layer_alpha(HEAD_DASH_VIOLET, 0.0f);
            break;
        case HEAD_FROZONE:
            m_pHeadImage->set_layer_alpha(HEAD_FROZONE, 1.0f);
            m_pHeadImage->set_layer_alpha(HEAD_MRS_I, 0.0f);
            m_pHeadImage->set_layer_alpha(HEAD_DASH_VIOLET, 0.0f);
            fMinHealthBar = 0.08f;
            fMaxHealthBar = 0.94f;
            m_pHealthImage->set_layer_minmax(1, fMinHealthBar, fMaxHealthBar);
            break;
        case HEAD_MRS_I:
            m_pHeadImage->set_layer_alpha(HEAD_FROZONE, 0.0f);
            m_pHeadImage->set_layer_alpha(HEAD_MRS_I, 1.0f);
            m_pHeadImage->set_layer_alpha(HEAD_DASH_VIOLET, 0.0f);
            fMinHealthBar = 0.14f;
            fMaxHealthBar = 0.94f;
            m_pHealthImage->set_layer_minmax(1, fMinHealthBar, fMaxHealthBar);
            break;
        case HEAD_DASH_VIOLET:
            m_pHeadImage->set_layer_alpha(HEAD_FROZONE, 0.0f);
            m_pHeadImage->set_layer_alpha(HEAD_MRS_I, 0.0f);
            m_pHeadImage->set_layer_alpha(HEAD_DASH_VIOLET, 1.0f);
            fMinHealthBar = 0.16f;
            fMaxHealthBar = 0.94f;
            m_pHealthImage->set_layer_minmax(1, fMinHealthBar, fMaxHealthBar);
            break;
        }
    }

    void FamilyMeter::set_health(F32 HealthPercent)
    {
        m_pHealthImage->set_layer_value(1, HealthPercent);
    }
} // namespace zHud

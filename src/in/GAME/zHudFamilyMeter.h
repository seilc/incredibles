#ifndef ZHUDFAMILYMETER_H
#define ZHUDFAMILYMETER_H

#include "zHud.h"
#include "xHudImage.h"

namespace zHud
{
    class FamilyMeter : public hud_element
    {
    public:
        enum eFamilyMeterHeadLayer
        {
            HEAD_NONE = -1,
            HEAD_FROZONE,
            HEAD_MRS_I,
            HEAD_DASH_VIOLET,
            HEAD_NUM_LAYERS
        };

    private:
        xhud::image_widget* m_pHealthImage;
        xhud::image_widget* m_pHeadImage;
        F32 m_fHealthCurrent;
        eFamilyMeterHeadLayer m_Head;

    public:
        FamilyMeter();

        void reset();
        void setup();
        void destroy();
        void update(F32 dt);
        void show();
        void hide();
        void enable(bool bEnable);
        void set_head(eFamilyMeterHeadLayer StageCount);
        void set_health(F32 HealthPercent);
    };
} // namespace zHud

#endif

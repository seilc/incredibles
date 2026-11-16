#ifndef ZHUDDASHMETER_H
#define ZHUDDASHMETER_H

#include "zHud.h"
#include "xHudImage.h"
#include "xHudText.h"

namespace zHud
{
    class DashMeter : public hud_element
    {
    public:
        enum eDashMeterLayer
        {
            BASEBACKGROUND,
            PROGRESS,
            VELOCIPOD,
            CHECKPOINT_NEXT,
            CHECKPOINT_PREV,
            DASH,
            NUM_LAYERS
        };

        static DashMeter* pDashMeter;

    private:
        xhud::image_widget* m_pImageWidget;
        xhud::text_widget* m_pCountDownWidget;
        xhud::text_widget* m_pSpeedMeterWidget;
        xhud::text_widget* m_pLevelPercentWidget;
        xhud::text_widget* m_pMissionTextWidget;
        xhud::text_widget* m_pCheckPointWidget;
        const char* m_SpeedMeterText;
        F32 m_start_scale;
        F32 m_final_scale;
        F32 m_delta_scale;
        F32 m_start_alpha;
        F32 m_end_alpha;
        F32 m_countdown_red_on_time;
        F32 m_countdown_red_fade_time;
        F32 m_countdown_red_total_time;
        xColor m_countdown_red_color;
        F32 m_mission_text_start_time;
        F32 m_mission_text_stop_time;
        F32 m_countdown_flash_start_time;
        F32 m_countdown_flash_stop_time;
        F32 m_countdown_on_time;
        F32 m_countdown_fade_time;
        F32 m_countdown_total_time;
        xColor m_countdown_color;
        F32 m_last_time;
        U8 m_flashing_red;
        bool m_flashing;
        struct
        {
            F32 fMin;
            F32 fCur;
            F32 fMax;
        } m_IconPos[NUM_LAYERS];

    public:
        DashMeter();

        void reset();
        void setup();
        void setCheckpoint();
        void setCountdownFlashing(F32 fTime);
        void setCountdownFlashingRed(F32 fTime);
        void destroy();
        void update(F32 dt);
        void show();
        void hide();
        void setOffset(eDashMeterLayer eIcon, F32 fOffset);
        void setSpeed(U32 uSpeed);
        void setPercentText(U32 uPercent);
        void setTimeRemaining(U32 uHour, U32 uMin, U32 uSec);
    };
} // namespace zHud

#endif

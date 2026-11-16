#include "xCommon.h"
#include "zHudDashMeter.h"

#include "zScene.h"
#include "zGame.h"
#include "zGlobals.h"
#include "zMain.h"
#include "zPlayer.h"
#include "zNPCTypes.h"
#include "xString.h"
#include "xTextAsset.h"

#include <stdio.h>

namespace zHud
{
    DashMeter* DashMeter::pDashMeter;

    DashMeter::DashMeter()
        : hud_element(), m_pImageWidget(NULL), m_start_scale(1.0f), m_final_scale(2.0f), m_delta_scale(1.0f),
          m_start_alpha(255.0f), m_end_alpha(20.0f), m_countdown_red_on_time(1.0f),
          m_countdown_red_fade_time(0.2f), m_countdown_red_total_time(0.075f)
    {
        m_countdown_red_color = g_RED;
        m_mission_text_start_time = 0.3f;
        m_mission_text_stop_time = 5.0f + m_mission_text_start_time;
        m_flashing_red = 1;
        m_flashing = false;
        m_countdown_flash_start_time = 1.0f;
        m_countdown_flash_stop_time = 2.5f;
        m_countdown_on_time = 0.5f;
        m_countdown_fade_time = 0.5f;
        m_countdown_total_time = 0.0f;
        m_countdown_color = g_CLEAR;
        m_last_time = 0.0f;

        m_IconPos[PROGRESS].fMin = 0.0f;
        m_IconPos[PROGRESS].fCur = 0.0f;
        m_IconPos[PROGRESS].fMax = 1.0f;
        m_IconPos[DASH].fMin = 140.0f;
        m_IconPos[DASH].fCur = 0.0f;
        m_IconPos[DASH].fMax = -140.0f;
        m_IconPos[VELOCIPOD].fMin = 130.0f;
        m_IconPos[VELOCIPOD].fCur = 0.0f;
        m_IconPos[VELOCIPOD].fMax = -130.0f;
        m_IconPos[CHECKPOINT_NEXT].fMin = 144.0f;
        m_IconPos[CHECKPOINT_NEXT].fCur = 0.0f;
        m_IconPos[CHECKPOINT_NEXT].fMax = -144.0f;
        m_IconPos[CHECKPOINT_PREV].fMin = 144.0f;
        m_IconPos[CHECKPOINT_PREV].fCur = 0.0f;
        m_IconPos[CHECKPOINT_PREV].fMax = -144.0f;

        pDashMeter = this;
    }

    void DashMeter::reset()
    {
        m_pImageWidget->clear_motives();
        m_pCountDownWidget->clear_motives();
    }

    void DashMeter::setup()
    {
        m_SpeedMeterText = xTextFindString("DASH_HUD_SPEEDMETER", NULL);

        U32 meterID = xStrHash("DashMeter"), countdownTimerID = xStrHash("countdown_timer"),
            levelpercentID = xStrHash("LEVELPERCENT"), speedmeterID = xStrHash("SPEEDMETER"),
            checkpointID = xStrHash("CHECKPOINT"), missionTextID = xStrHash("MISSIONTEXT");
        RwTexCoords upperleft, lowerright;

        m_pCountDownWidget = (xhud::text_widget*)&((xhud::widget_chunk*)zSceneFindObject(countdownTimerID))->w;
        m_pSpeedMeterWidget = (xhud::text_widget*)&((xhud::widget_chunk*)zSceneFindObject(speedmeterID))->w;
        m_pLevelPercentWidget = (xhud::text_widget*)&((xhud::widget_chunk*)zSceneFindObject(levelpercentID))->w;
        m_pCheckPointWidget = (xhud::text_widget*)&((xhud::widget_chunk*)zSceneFindObject(checkpointID))->w;
        m_pMissionTextWidget = (xhud::text_widget*)&((xhud::widget_chunk*)zSceneFindObject(missionTextID))->w;
        m_pImageWidget = (xhud::image_widget*)&((xhud::widget_chunk*)zSceneFindObject(meterID))->w;

        m_pImageWidget->init_layers(NUM_LAYERS, 24, 36);

        upperleft.u = 0.0f;
        upperleft.v = 0.0f;
        lowerright.u = upperleft.u + 0.25f;
        lowerright.v = upperleft.v + 1.0f;

        m_pImageWidget->set_layer_uvs(BASEBACKGROUND, upperleft, lowerright);
        m_pImageWidget->set_layer_type(BASEBACKGROUND, xhud::image_widget::VARIABLE_TOP,
                                       xhud::image_widget::BLEND);
        m_pImageWidget->set_layer_value(BASEBACKGROUND, 1.0f);

        upperleft.u = 0.25f;
        upperleft.v = 0.0f;
        lowerright.u = 0.5f;
        lowerright.v = 1.0f;

        m_pImageWidget->set_layer_uvs(PROGRESS, upperleft, lowerright);
        m_pImageWidget->set_layer_type(PROGRESS, xhud::image_widget::VARIABLE_TOP, xhud::image_widget::BLEND);
        m_pImageWidget->set_layer_value(PROGRESS, 0.0f);

        upperleft.u = 0.5f;
        upperleft.v = 0.5f;
        lowerright.u = 0.75f;
        lowerright.v = 0.75f;

        m_pImageWidget->set_layer_uvs(VELOCIPOD, upperleft, lowerright);
        m_pImageWidget->set_layer_type(VELOCIPOD, xhud::image_widget::OFFSET_CENTERPT,
                                       xhud::image_widget::BLEND);
        m_pImageWidget->set_layer_offset(VELOCIPOD, 0.0f, 0.0f);
        m_pImageWidget->set_layer_alpha(VELOCIPOD, 0.0f);

        upperleft.u = 0.5f;
        upperleft.v = 0.25f;
        lowerright.u = 0.75f;
        lowerright.v = 0.5f;

        m_pImageWidget->set_layer_uvs(CHECKPOINT_NEXT, upperleft, lowerright);
        m_pImageWidget->set_layer_type(CHECKPOINT_NEXT, xhud::image_widget::OFFSET_CENTERPT,
                                       xhud::image_widget::BLEND);
        m_pImageWidget->set_layer_offset(CHECKPOINT_NEXT, 0.0f, 0.0f);

        m_pImageWidget->set_layer_uvs(CHECKPOINT_PREV, upperleft, lowerright);
        m_pImageWidget->set_layer_type(CHECKPOINT_PREV, xhud::image_widget::OFFSET_CENTERPT,
                                       xhud::image_widget::BLEND);
        m_pImageWidget->set_layer_offset(CHECKPOINT_PREV, 0.0f, 0.0f);

        if (ZPLAYER->brain_id == PLAYER_TYPE_DASH_REGULAR) {
            upperleft.u = 0.75f;
            upperleft.v = 0.0f;
            lowerright.u = 1.0f;
            lowerright.v = 0.25f;
        } else {
            upperleft.u = 0.5f;
            upperleft.v = 0.0f;
            lowerright.u = 0.75f;
            lowerright.v = 0.25f;
        }

        m_pImageWidget->set_layer_uvs(DASH, upperleft, lowerright);
        m_pImageWidget->set_layer_type(DASH, xhud::image_widget::OFFSET_CENTERPT, xhud::image_widget::BLEND);
        m_pImageWidget->set_layer_offset(DASH, 0.0f, 0.0f);

        m_pCountDownWidget->setup();
        m_pSpeedMeterWidget->setup();
        m_pLevelPercentWidget->setup();
        m_pImageWidget->setup();
        m_pCheckPointWidget->setup();
        m_pMissionTextWidget->setup();

        m_pCountDownWidget->set_text("00:00:00");
        setSpeed(0);
        m_pLevelPercentWidget->set_text("0%");

        m_pCountDownWidget->enable();
        m_pSpeedMeterWidget->enable();
        m_pLevelPercentWidget->enable();
        m_pImageWidget->enable();
        m_pCheckPointWidget->disable();
        m_pMissionTextWidget->disable();

        reset();
    }

    void DashMeter::setCheckpoint()
    {
        m_pCheckPointWidget->clear_motives();
        m_pCheckPointWidget->enable();
        m_pCheckPointWidget->add_motive(xhud::motive(NULL, m_delta_scale, m_start_scale, m_final_scale,
                                                     m_start_alpha, m_end_alpha, xhud::scale_text_motive_update,
                                                     xhud::scale_text_motive_reset));
    }

    void DashMeter::setCountdownFlashing(F32 fTime)
    {
        m_pCountDownWidget->clear_motives();
        m_pCountDownWidget->add_motive(xhud::motive(fTime, m_countdown_on_time, m_countdown_fade_time,
                                                    m_countdown_total_time, m_countdown_color,
                                                    m_pCountDownWidget->tb.font.color, false,
                                                    xhud::blink_text_motive_update, xhud::blink_text_motive_reset));
    }

    void DashMeter::setCountdownFlashingRed(F32 fTime)
    {
        m_pCountDownWidget->clear_motives();
        m_pCountDownWidget->add_motive(xhud::motive(fTime, m_countdown_red_on_time, m_countdown_red_fade_time,
                                                    m_countdown_red_total_time, m_countdown_red_color,
                                                    m_pCountDownWidget->tb.font.color, true,
                                                    xhud::blink_text_motive_update, xhud::blink_text_motive_reset));
    }

    void DashMeter::destroy()
    {
    }

    void DashMeter::update(F32)
    {
        if (zGameIsPaused()) {
            return;
        }

        m_pImageWidget->set_layer_value(PROGRESS, m_IconPos[PROGRESS].fCur);
        m_pImageWidget->set_layer_offset(DASH, 0.0f, m_IconPos[DASH].fCur);
        m_pImageWidget->set_layer_offset(VELOCIPOD, 0.0f, m_IconPos[VELOCIPOD].fCur);
        m_pImageWidget->set_layer_offset(CHECKPOINT_NEXT, 0.0f, m_IconPos[CHECKPOINT_NEXT].fCur);
        m_pImageWidget->set_layer_offset(CHECKPOINT_PREV, 0.0f, m_IconPos[CHECKPOINT_PREV].fCur);

        if (m_last_time < m_countdown_flash_start_time &&
            globals.player.ControlOnTime > m_countdown_flash_start_time) {
            setCountdownFlashing(m_countdown_flash_stop_time);
        }

        m_last_time = globals.player.ControlOnTime;

        if (globals.player.ControlOnTime > m_mission_text_start_time &&
            globals.player.ControlOnTime < m_mission_text_stop_time) {
            if (m_flashing_red) {
                m_flashing = true;
                m_flashing_red--;
            }
        } else {
            m_flashing = false;
        }

        if (m_flashing) {
            m_pMissionTextWidget->enable();
        } else {
            m_pMissionTextWidget->disable();
        }
    }

    void DashMeter::show()
    {
        m_pImageWidget->show(xhud::widget::RIGHT, true, false);
        m_pCountDownWidget->enable();
        m_pSpeedMeterWidget->enable();
        m_pLevelPercentWidget->enable();
    }

    void DashMeter::hide()
    {
        m_pImageWidget->hide(xhud::widget::RIGHT, true, false);
        m_pImageWidget->update(0.0f);
        m_pCountDownWidget->disable();
        m_pSpeedMeterWidget->disable();
        m_pLevelPercentWidget->disable();
        m_pMissionTextWidget->disable();
        m_pCheckPointWidget->clear_motives();
        m_pCheckPointWidget->disable();
    }

    void DashMeter::setOffset(eDashMeterLayer eIcon, F32 fOffset)
    {
        if (fOffset >= 0.0f) {
            fOffset = CLAMP(fOffset, 0.0f, 1.0f);
            m_IconPos[eIcon].fCur = fOffset * (m_IconPos[eIcon].fMax - m_IconPos[eIcon].fMin) + m_IconPos[eIcon].fMin;
            m_pImageWidget->set_layer_alpha(eIcon, 1.0f);
        } else {
            m_pImageWidget->set_layer_alpha(eIcon, 0.0f);
        }
    }

    void DashMeter::setSpeed(U32 uSpeed)
    {
        char buffer0[12], buffer1[12];

        sprintf(buffer0, "%d\n", uSpeed);
        sprintf(buffer1, m_SpeedMeterText, buffer0);
        m_pSpeedMeterWidget->set_text(buffer1);
    }

    void DashMeter::setPercentText(U32 uPercent)
    {
        char buffer[12];

        sprintf(buffer, "%d%%", MIN(uPercent, 100));
        m_pLevelPercentWidget->set_text(buffer);
    }

    void DashMeter::setTimeRemaining(U32 uHour, U32 uMin, U32 uSec)
    {
        char buffer[16];

        uHour = MIN(uHour, 99);
        uMin = MIN(uMin, 99);
        uSec = MIN(uSec, 99);
        sprintf(buffer, "%02d:%02d:%02d", uHour, uMin, uSec);
        m_pCountDownWidget->set_text(buffer);
    }
} // namespace zHud

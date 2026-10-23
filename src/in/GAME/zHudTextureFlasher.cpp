#include "zHudTextureFlasher.h"

#include "xMath.h"

namespace zHud
{
    void TextureFlasher::setup(xhud::image_widget* pImageWidget, U32 uLayer, F32 onTime,
                               F32 offTime, F32 decayTime)
    {
        m_onTime = onTime;
        m_offTime = -offTime;
        m_offDecayTime = decayTime;
        m_timer = 0.0f;
        m_pImageWidget = pImageWidget;
        m_uFlashLayer = uLayer;
    }

    void TextureFlasher::setCallback(Callback callback, void* context)
    {
        m_context = context;
        m_callback = callback;
    }

    void TextureFlasher::update(F32 dt)
    {
        if (m_callback && !m_callback(m_context, this))
        {
            m_timer = 0.0f;
        }
        else
        {
            m_timer += dt;

            if (m_timer >= m_onTime)
            {
                m_timer = m_offTime;
            }
        }

        if (m_timer < 0.0f)
        {
            F32 timeoff = m_timer - m_offTime;
            F32 fPercentDecay = 1.0f;

            if (m_offDecayTime > 0.0f)
            {
                timeoff /= m_offDecayTime;
                fPercentDecay = MIN(fPercentDecay, timeoff);
            }

            m_pImageWidget->set_layer_alpha(m_uFlashLayer, 1.0f - fPercentDecay);
        }
        else if (m_timer > 0.0f)
        {
            m_pImageWidget->set_layer_alpha(m_uFlashLayer, 1.0f);
        }
        else
        {
            m_pImageWidget->set_layer_alpha(m_uFlashLayer, 0.0f);
        }
    }

    void TextureFlasher::setFlashingParams(F32 offTime, F32 onTime)
    {
        m_offTime = -offTime;
        m_onTime = onTime;

        if (m_timer >= m_onTime)
        {
            m_timer = m_offTime;
        }
        else if (m_timer <= m_offTime)
        {
            m_timer = m_offTime;
        }
    }
} // namespace zHud
#ifndef ZHUDTEXTUREFLASHER_H
#define ZHUDTEXTUREFLASHER_H

#include "xHudImage.h"

namespace zHud
{
    class TextureFlasher
    {
    public:
        typedef bool (*Callback)(void* context, TextureFlasher* flasher);

        void setup(xhud::image_widget* pImageWidget, U32 uLayer, F32 onTime, F32 offTime,
                   F32 decayTime);
        void setCallback(Callback callback, void* context);
        void update(F32 dt);
        void setFlashingParams(F32 offTime, F32 onTime);

    private:
        xhud::image_widget* m_pImageWidget;
        U32 m_uFlashLayer;
        F32 m_timer;
        F32 m_onTime;
        F32 m_offTime;
        F32 m_offDecayTime;
        void* m_context;
        Callback m_callback;
    };
} // namespace zHud

#endif
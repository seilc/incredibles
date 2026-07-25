#ifndef ZHUDDISKINDICATOR_H
#define ZHUDDISKINDICATOR_H

#include "zHud.h"
#include "xHudImage.h"

namespace zHud
{
    class DiskIndicator : public hud_element
    {
    private:
        xhud::image_widget* m_pImageWidget;
        F32 m_fCircleSpeed;
        F32 m_fCircleValue;
        F32 m_fDisplayTimeLeft;

    public:
        DiskIndicator();

        void reset();
        void setup();
        void destroy();
        void spin(F32 fSeconds);
        void update(F32 dt);
        void show();
        void hide();
    };
} // namespace zHud

#endif
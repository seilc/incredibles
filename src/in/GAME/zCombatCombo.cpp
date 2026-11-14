#include "xCommon.h"
#include "zCombatCombo.h"

F32 zCOMBAT_COMBO_THRESHOLD = 0.18f;
zCombatCombo zccombo;

void zCombatCombo::setEnabled(bool value)
{
    enabled = value;
}

bool zCombatCombo::getEnabled()
{
    return enabled;
}

void zCombatCombo::init()
{
    enabled = false;
    elapsed = 0.0f;
    timer = 0.0f;
    combo_time = zCOMBAT_COMBO_THRESHOLD;
    clear_queue();
}

void zCombatCombo::update(F32 dt)
{
    if (timer) {
        timer -= dt;
        if (timer < 0.0f) {
            init();
        }

        for (S32 i = get_size_queue() - 1; i >= 0; i--) {
            zHitStruct* hit = fetch_queue_ptr(i);
            hit->elapsed += dt;
            if (hit->elapsed > zCOMBAT_COMBO_THRESHOLD) {
                get_queue(*hit);
            }
        }
    }
}

void zCombatCombo::add(en_HIT_TYPE type)
{
    timer = combo_time;

    zHitStruct hit;
    hit.type = type;
    hit.elapsed = 0.0f;
    add_queue(hit);
}

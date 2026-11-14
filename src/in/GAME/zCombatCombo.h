#ifndef ZCOMBATCOMBO_H
#define ZCOMBATCOMBO_H

#include "zQueue.h"

enum en_HIT_TYPE
{
    z_HIT_LIGHT,
    z_HIT_HEAVY
};

struct zHitStruct
{
    S32 type;
    F32 elapsed;
};

class zCombatCombo : public zQueue<zHitStruct, 64>
{
public:
    F32 combo_time;
    F32 elapsed;
    F32 timer;
    bool enabled;

    void setEnabled(bool value);
    bool getEnabled();
    void init();
    void update(F32 dt);
    void add(en_HIT_TYPE type);
};

extern zCombatCombo zccombo;

#endif

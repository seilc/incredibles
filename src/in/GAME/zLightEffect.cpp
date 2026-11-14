#include "xCommon.h"
#include "zLightEffect.h"

#include <stdlib.h>

static F32 leGetRandom()
{
    F32 ret = (U16)rand() / 65535.0f;
    return ret;
}

void zLightEffectInitStrobe(zLight*)
{
}

void zLightEffectInitDim(zLight*)
{
}

void zLightEffectInitHalfDim(zLight*)
{
}

void zLightEffectInitRandomCol(zLight*)
{
}

void zLightEffectInitFlicker(zLight* zlight)
{
    *zlight->reg = 0.0f;
    zLightOn(zlight, TRUE);
}

static void EffectFlicker(zLight* zlight, F32 seconds, F32 min, F32 rnd)
{
    iLight* l = &zlight->light;
    F32* reg = zlight->reg;

    *reg -= seconds;
    if (*reg <= 0.0f) {
        *reg = rnd * leGetRandom();
        *reg += min;

        l->color.r = zlight->tasset->lightColor[0] - (0.2f * leGetRandom() + 0.1f);
        l->color.g = zlight->tasset->lightColor[1] - (0.2f * leGetRandom() + 0.1f);
        l->color.b = zlight->tasset->lightColor[2] - (0.2f * leGetRandom() + 0.1f);

        if (l->color.r > 1.0f) {
            l->color.r = 1.0f;
        } else if (l->color.r < 0.0f) {
            l->color.r = 0.0f;
        }

        if (l->color.g > 1.0f) {
            l->color.g = 1.0f;
        } else if (l->color.g < 0.0f) {
            l->color.g = 0.0f;
        }

        if (l->color.b > 1.0f) {
            l->color.b = 1.0f;
        } else if (l->color.b < 0.0f) {
            l->color.b = 0.0f;
        }

        iLightSetColor(l, &l->color);
    }
}

void zLightEffectFlicker(zLight* zlight, F32 seconds)
{
    EffectFlicker(zlight, seconds, 0.05f, 0.2f);
}

void zLightEffectFlickerSlow(zLight* zlight, F32 seconds)
{
    EffectFlicker(zlight, seconds, 0.2f, 0.3f);
}

void zLightEffectFlickerErratic(zLight* zlight, F32 seconds)
{
    EffectFlicker(zlight, seconds, 0.0f, 0.1f);
}

void zLightEffectStrobeSlow(zLight*, F32)
{
}

void zLightEffectStrobe(zLight*, F32)
{
}

void zLightEffectStrobeFast(zLight*, F32)
{
}

void zLightEffectDimSlow(zLight*, F32)
{
}

void zLightEffectDim(zLight*, F32)
{
}

void zLightEffectDimFast(zLight*, F32)
{
}

void zLightEffectHalfDimSlow(zLight*, F32)
{
}

void zLightEffectHalfDim(zLight*, F32)
{
}

void zLightEffectHalfDimFast(zLight*, F32)
{
}

void zLightEffectRandomColSlow(zLight*, F32)
{
}

void zLightEffectRandomCol(zLight*, F32)
{
}

void zLightEffectRandomColFast(zLight*, F32)
{
}

void zLightEffectInitCauldron(zLight* zlight)
{
    *zlight->reg = 0.0f;
    zLightOn(zlight, TRUE);
}

static F32 leBlendToCol(F32 cur, F32 target, F32 step)
{
    if (cur > target) {
        cur -= step;
        if (cur < target) {
            cur = target;
        }
    } else if (cur < target) {
        cur += step;
        if (cur > target) {
            cur = target;
        }
    }
    return cur;
}

void zLightEffectCauldron(zLight* zlight, F32 seconds)
{
    iLight* l = &zlight->light;
    F32* reg = zlight->reg;

    *reg -= seconds;
    if (*reg <= 0.0f) {
        reg[0] = 0.25f;
        reg[1] = 0.5f * leGetRandom();
        reg[2] = 0.4f * leGetRandom() + 0.6f;
        reg[3] = 0.5f * leGetRandom();
    }

    F32 amount = 3.0f * seconds;
    l->color.r = leBlendToCol(l->color.r, reg[1], amount);
    l->color.g = leBlendToCol(l->color.g, reg[2], amount);
    l->color.b = leBlendToCol(l->color.b, reg[3], amount);
    iLightSetColor(l, &l->color);
}

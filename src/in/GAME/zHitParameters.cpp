#include "xCommon.h"
#include "zHitParameters.h"

#include "xString.h"

const zHitSourceMapEntry zHitSourceMap[zHS_COUNT + 1] = {
    { "EVENT", zHS_EVENT },
    { "GENERAL", zHS_GENERAL },
    { "PROJECTILE", zHS_PROJECTILE },
    { "EXPLOSION", zHS_EXPLOSION },
    { "LASER", zHS_LASER },
    { "ENERGY", zHS_ENERGY },
    { "FIRE", zHS_FIRE },
    { "SURFACE", zHS_SURFACE },
    { "MELEE_HIGH", zHS_MELEE_HIGH },
    { "MELEE_MID", zHS_MELEE_MID },
    { "MELEE_LOW", zHS_MELEE_LOW },
    { "MELEE_UP", zHS_MELEE_UP },
    { "MELEE_BACK", zHS_MELEE_BACK },
    { "MELEE_DIZZY", zHS_MELEE_DIZZY },
    { "THROW", zHS_THROW },
    { "WATER", zHS_WATER },
    { "DEATHPLANE", zHS_DEATHPLANE },
    { "INCREDI", zHS_INCREDI },
    { "KNOCKBACK", zHS_KNOCKBACK },
    { "LASERBEAM", zHS_LASERBEAM },
    { NULL, zHS_GENERAL }
};

U32 zHitSourceCCs[zHS_COUNT] = {
    'EV\0\0',
    'GE\0\0',
    'PR\0\0',
    'EX\0\0',
    'LA\0\0',
    'EN\0\0',
    'FI\0\0',
    'SU\0\0',
    'MH\0\0',
    'MM\0\0',
    'ML\0\0',
    'MU\0\0',
    'MB\0\0',
    'MD\0\0',
    'TH\0\0',
    'WA\0\0',
    'DP\0\0',
    'IA\0\0',
    'KB\0\0',
    'LB\0\0'
};

U32 zHitTargetCCs[zHT_COUNT] = {
    '\0\0G\0',
    '\0\0F\0',
    '\0\0B\0',
    '\0\0L\0',
    '\0\0R\0',
    '\0\0E\0',
    '\0\0V\0'
};

S32 getSourceCCIdx(U32 value)
{
    for (S32 i = 0; i < zHS_COUNT; i++) {
        char tag[4];
        tag[0] = (zHitSourceCCs[i] >> 24) & 0xFF;
        tag[1] = (zHitSourceCCs[i] >> 16) & 0xFF;
        tag[2] = (zHitSourceCCs[i] >> 8) & 0xFF;
        tag[3] = zHitSourceCCs[i] & 0xFF;
        if (value == xStrHash(tag))
            return i;
    }
    return -1;
}

#ifndef ZCONFIGURATION_H
#define ZCONFIGURATION_H

#include "types.h"

typedef enum _PLAYER_ENUM
{
    MR_INCREDIBLE_YOUNG,
    MR_INCREDIBLE_DISGUISED,
    MR_INCREDIBLE_OLD_FAT_BLUE,
    MR_INCREDIBLE_OLD_FAT_RED,
    MR_INCREDIBLE_OLD_FIT,
    ELASTI_GIRL_YOUNG,
    ELASTI_GIRL_OLD,
    DASH_REGULAR,
    DASH_COSTUMED,
    INCREDI_BALL,
    VIOLET,
    PLAYER_COUNT
} PLAYER_ENUM;

U32 zConfig_getPlayerEnumIdx(U32 _player_tag);
void zConfig_setScenePlayer_MapEntry_Fixed(U32 sceneID, U32 _player_tag, U32 idx);
void zConfig_setScenePlayer_MapEntry(U32 sceneID, U32 _player_tag);
U32 zConfig_findNextScene(U32 sceneID, U32 numChars, U32& startIdx);
U32 zConfig_getPlayerTag(U32 sceneID, S32 playerIdx);
const char* zConfig_getPlayerModelInfoString(U32 _tag);
U32 zConfig_getPlayerLorezModelID(U32 _tag);
PLAYER_ENUM zConfig_getPlayerEnum(U32 sceneID, S32 playerIdx);

#endif
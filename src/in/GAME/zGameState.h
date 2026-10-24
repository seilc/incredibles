#ifndef ZGAMESTATE_H
#define ZGAMESTATE_H

#include "types.h"

enum eGameMode
{
    eGameMode_Boot,
    eGameMode_Intro,
    eGameMode_Title,
    eGameMode_Start,
    eGameMode_Load,
    eGameMode_Options,
    eGameMode_Save,
    eGameMode_Pause,
    eGameMode_WorldMap,
    eGameMode_MonsterGallery,
    eGameMode_ConceptArtGallery,
    eGameMode_Game,
    eGameMode_Count
};

typedef enum _GameOstrich
{
    eGameOstrich_Loading,
    eGameOstrich_PlayingMovie,
    eGameOstrich_InScene,
    eGameOstrich_Total
} GameOstrich;

// Fake enums, copied from bfbb decomp

enum eGameState
{
    eGameState_FirstTime,
    eGameState_Play,
    eGameState_LoseChance,
    eGameState_GameOver,
    eGameState_GameStats,
    eGameState_SceneSwitch,
    eGameState_Dead,
    eGameState_Exit,
    eGameState_Count
};

enum ePauseState
{
    ePauseState_Pause,
    ePauseState_Options,
    ePauseState_Count
};

enum eSaveState
{
    eSaveState_SelectMemCard,
    eSaveState_SelectSlot,
    eSaveState_Saving,
    eSaveState_Count
};

enum eOptionsState
{
    eOptionsState_Options,
    eOptionsState_Count
};

enum eLoadState
{
    eLoadState_SelectMemCard,
    eLoadState_SelectSlot,
    eLoadState_Loading,
    eLoadState_Count
};

enum eTitleState
{
    eTitleState_Start,
    eTitleState_Attract,
    eTitleState_Count
};

enum eIntroState
{
    eIntroState_Sony,
    eIntroState_Publisher,
    eIntroState_Developer,
    eIntroState_License,
    eIntroState_Count
};

extern S32 gGameState;
extern eGameMode gGameMode;
extern _GameOstrich gGameOstrich;

S32 zGameStateGet();
S32 zGameModeGet();
GameOstrich zGameGetOstrich();
void zGameSetOstrich(GameOstrich o);
S32 zGameStateFindEvent(U32* eventList, S32 eventCount, S32 targetMode, S32 targetEvent,
                           S32* new_mode, S32* new_state);
void zGameStateSwitchEvent(S32 event);
void zGameStateSwitch(S32 theNewState);
void zGameModeSwitch(eGameMode modeNew);

#endif
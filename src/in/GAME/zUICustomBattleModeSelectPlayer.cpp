#include "xCommon.h"
#include "zUICustomBattleModeSelectPlayer.h"

#include "xEvent.h"
#include "xString.h"
#include "xTextAsset.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zConfiguration.h"
#include "zDispatcher.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zUICustomBattleModeSelectScene.h"
#include "zUICustomSysMessage.h"

#include "decomp.h"

#include <stdio.h>
#include <string.h>

struct battle_player_details
{
    U32 tag;
    char filename[5];
    char imagename[5];
};

U32 curr_content_playerID;

namespace {

const battle_player_details battle_player_default[2] = {
    { IDTAG('P','L','Y','4'), "PLY4", "PLY4" },
    { IDTAG('P','L','Y','6'), "PLY6", "PLY6" },
};

bool texture_scene_loaded;

DECOMP_FORCEACTIVE("UI PREV SCREEN HANDLER")

void zSystemMsgUIEnterScene(zUI* from, zUI* ui, U32 sceneID)
{
    pSysMessageManager->Signal(ui, eEventUISysMsgWaitResponse);

    F32 toParams[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    memcpy(toParams, &sceneID, sizeof(U32));

    ((zUICustomSysMessageManager*)pSysMessageManager_Custom)
        ->ShowSystemMessage(ZDSP_elcb_event, eEventUISysMsgWaitResponse, eEventWarpGo, toParams, from,
                            NULL, 0, xTextFindString(xStrHash("UI TXT SYS MSG CHANGE MISSION"), NULL));

    from->Signal(ui, eEventUIFocusOff_Unselect);
}

} // namespace

zUICustomBattleModeSelectPlayerManager::zUICustomBattleModeSelectPlayerManager(zUI* ui) : zUICustom(ui)
{
}

void zUICustomBattleModeSelectPlayerManager::Init()
{
    maxContents = 0;
    displayIndex = 0;
    olddisplayIndex = -1;

    zSceneFindObject(ui->asset->customWidget);

    downloadable_content_available = false;

    removeButton = NULL;
    removeButton = (zUIText*)zSceneFindObject(xStrHash("UI BATTLE MODE REMOVE TEXT"));

    imagePlayer = NULL;
    imagePlayer = (zUIImage*)zSceneFindObject(xStrHash("UI BATTLE MODE PLAYER IMAGE"));
    imagePlayer->visible = false;

    imageArrowLeft = NULL;
    imageArrowLeft = (zUIImage*)zSceneFindObject(xStrHash("UI BATTLE MODE PLAYER IMAGE LEFT"));
    imageArrowLeft->visible = false;

    imageArrowRight = NULL;
    imageArrowRight = (zUIImage*)zSceneFindObject(xStrHash("UI BATTLE MODE PLAYER IMAGE RIGHT"));
    imageArrowRight->visible = false;

    playerDescription = NULL;
    playerDescription = (zUIText*)zSceneFindObject(xStrHash("UI BATTLE PLAYER DESCRIPTION TEXT"));
    playerDescription->visible = false;

    maxContents = 0;
    displayIndex = 0;
}

bool zUICustomBattleModeSelectPlayerManager::HandleEvent(xBase* from, U32 toEvent, const F32* toParam,
                                                         xBase* toParamWidget, U32 toParamWidgetID)
{
    zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);

    switch (toEvent) {
    case eEventUISysMessageActionConfirmed:
        if (texture_scene_loaded) {
            xSTUnLoadScene(IDTAG('U','T','_','T'), 1);
            xSTUnLoadScene(IDTAG('U','T','_','T'), 2);
            imagePlayer->SetTexture(0, false);
            texture_scene_loaded = false;
        }
        break;
    case eEventPadPressStart:
        ui->focus = false;
        ui->lastFocus = false;
        ui->selected = false;
        break;
    case eEventPadPressTriangle:
        if (texture_scene_loaded) {
            xSTUnLoadScene(IDTAG('U','T','_','T'), 1);
            xSTUnLoadScene(IDTAG('U','T','_','T'), 2);
            imagePlayer->SetTexture(0, false);
            texture_scene_loaded = false;
        }
        break;
    case eEventPadPressRight:
        displayIndex++;
        ui->Signal(displayIndex > maxContents - 1 ? eEventFailure : eEventUIAutoMenuRunRight);
        if (displayIndex >= maxContents - 1) {
            displayIndex = maxContents - 1;
            imageArrowRight->visible = false;
        }
        imageArrowLeft->visible = true;
        ShowContent();
        break;
    case eEventPadPressLeft:
        displayIndex--;
        ui->Signal(displayIndex < 0 ? eEventFailure : eEventUIAutoMenuRunLeft);
        if (displayIndex <= 0) {
            displayIndex = 0;
            imageArrowLeft->visible = false;
        }
        imageArrowRight->visible = true;
        ShowContent();
        break;
    case eEventUISelect:
        InitContent();
        ShowContent();
        globals.pad0->pressed = 0;
        olddisplayIndex = -1;
        break;
    case eEventPadPressX: {
        contentDetails& content = battle_players[displayIndex];
        if (content.contentState != TYPE_CORRUPTED && content.contentType != TYPE_NET) {
            U32 tag = IDTAG(content.name[0], content.name[1], content.name[2], content.name[3]);
            zConfig_setScenePlayer_MapEntry_Fixed(curr_content_sceneID, tag, 0);
            curr_content_playerID = tag;
            zSystemMsgUIEnterScene((zUI*)from, ui, curr_content_sceneID);
        }
        break;
    }
    case eEventPadPressDown:
        break;
    case eEventUIMotionOutFinished:
        break;
    case eEventUISignalDeactivateScreen:
        break;
    case eEventUISignalUp:
    case eEventUISignalDown:
        break;
    }

    return true;
}

void zUICustomBattleModeSelectPlayerManager::init_battle_players()
{
    maxContents = 0;
    displayIndex = 0;
    defaultContents = 0;
    local_content_available = false;
    downloadable_content_available = false;
}

void zUICustomBattleModeSelectPlayerManager::get_default_players()
{
    for (U32 i = 0; i < ARRAY_LENGTH(battle_player_default); i++) {
        local_content_available = true;

        contentDetails& content = battle_players[maxContents++];
        content.contentType = TYPE_LOCAL;
        content.contentAccess = TYPE_UNLOCKED;
        content.contentListIdx = -1;
        sprintf(content.name, "%s", battle_player_default[i].filename);
        sprintf(content.imagename, "%s", battle_player_default[i].imagename);
    }

    if (maxContents != 0) {
        imageArrowRight->visible = true;
    }
}

void zUICustomBattleModeSelectPlayerManager::ShowRemoveButton()
{
    if (battle_players[displayIndex].contentType == TYPE_NET) {
        U32 sceneID = globals.sceneCur->sceneID;
        const char* prefix = "BM";
        U8 c0 = sceneID >> 24;
        U8 c1 = sceneID >> 16;
        if (prefix[0] == c0 && prefix[1] == c1) {
            removeButton->visible = false;
        } else {
            removeButton->visible = true;
        }
    } else {
        removeButton->visible = false;
    }
}

void zUICustomBattleModeSelectPlayerManager::ShowContent()
{
    imagePlayer->visible = true;

    if (maxContents == 1) {
        imageArrowLeft->visible = false;
        imageArrowRight->visible = false;
    }

    if (maxContents != 0) {
        UpdateImage();
        ShowRemoveButton();
    }
}

void zUICustomBattleModeSelectPlayerManager::InitContent()
{
    init_battle_players();
    get_default_players();
    displayIndex = 0;
}

void zUICustomBattleModeSelectPlayerManager::PreUpdate(F32 dt)
{
    zUICustom::PreUpdate(dt);
}

void zUICustomBattleModeSelectPlayerManager::UpdateImage()
{
    if (maxContents != 0) {
        playerDescription->SetText("");
        playerDescription->visible = false;
        imagePlayer->SetTexture(0, false);

        if (battle_players[displayIndex].contentType == TYPE_LOCAL) {
            U32 id = xStrHash(battle_players[displayIndex].imagename);
            imagePlayer->SetTexture(id, false);
        } else {
            imagePlayer->SetTexture(0, false);
        }
    }
}

#ifndef ZUICUSTOMBATTLEMODESELECTPLAYER_H
#define ZUICUSTOMBATTLEMODESELECTPLAYER_H

#include "zUICustom.h"
#include "zUIImage.h"
#include "zUIText.h"

enum enCONTENT_TYPE
{
    TYPE_LOCAL,
    TYPE_NET
};

enum enCONTENT_TYPE_ACCESS
{
    TYPE_UNLOCKED,
    TYPE_LOCKED,
    TYPE_UNKNOWN
};

enum enCONTENT_STATE
{
    TYPE_VALID,
    TYPE_CORRUPTED
};

struct contentDetails
{
    char name[128];
    char filename[128];
    char imagename[128];
    char description[128];
    enCONTENT_TYPE contentType;
    enCONTENT_TYPE_ACCESS contentAccess;
    enCONTENT_STATE contentState;
    S32 contentListIdx;
};

class zUICustomBattleModeSelectPlayerManager : public zUICustom
{
public:
    zUICustomBattleModeSelectPlayerManager(zUI* ui);

    void Init();
    bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                     U32 toParamWidgetID);
    void init_battle_players();
    void get_default_players();
    void ShowRemoveButton();
    void ShowContent();
    void InitContent();
    void PreUpdate(F32 dt);
    void UpdateImage();

    bool IsForcePreUpdate()
    {
        return true;
    }

    S32 numItems;
    zUIText* removeButton;
    zUIText* playerDescription;
    xBase* dispatcher;
    zUIImage* imagePlayer;
    zUIImage* imageArrowLeft;
    zUIImage* imageArrowRight;
    contentDetails battle_players[128];
    S32 maxContents;
    S32 defaultContents;
    S32 displayIndex;
    S32 olddisplayIndex;
    bool downloadable_content_available;
    bool local_content_available;
};

extern U32 curr_content_playerID;

#endif

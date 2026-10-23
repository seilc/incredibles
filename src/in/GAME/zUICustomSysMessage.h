#ifndef ZUICUSTOMSYSMESSAGE_H
#define ZUICUSTOMSYSMESSAGE_H

#include "zUICustom.h"
#include "zUIText.h"

class zUICustomSysMessageManager : public zUICustom
{
public:
    zUICustomSysMessageManager(zUI* ui);
    void Init();
    bool IsSignalAllowed(U32 toEvent);
    bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                     U32 toParamWidgetID);
    void ShowSystemMessage(xBaseEventCB cb, U32, U32 _toCBEvent, const F32* _toCBParams,
                           xBase* _toCBto, xBase* _toCBParamWidget, U32 _toCBParamWidgetID,
                           const char* sys_message);

    bool IsSystem(zUI* ptr)
    {
        return (ptr == itemOk || ptr == itemYes || ptr == itemNo || ptr == itemMsg);
    }

private:
    xBaseEventCB cb_function;
    U32 toCBEvent;
    F32 toCBParams[4];
    xBase* toCBto;
    xBase* toCBParamWidget;
    U32 toCBParamWidgetID;
    zUI* fromUI;
    zUIText* itemOk;
    zUIText* itemYes;
    zUIText* itemNo;
    zUIText* itemMsg;
};

extern bool gDontAutoMenu;
extern bool gSysMessageManager_active;
extern zUI* pSysMessageManager;
extern zUICustom* pSysMessageManager_Custom;

#endif
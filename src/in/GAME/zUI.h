#ifndef ZUI_H
#define ZUI_H

#include "xDynAsset.h"
#include "xColor.h"

struct zUICustom;

struct zUIAsset : xDynAsset
{
    F32 x;
    F32 y;
    F32 z;
    F32 width;
    F32 height;
    S32 flags;
    U32 color;
    U32 selectedMotion;
    U32 unselectedMotion;
    U8 brightness;
    U8 pad[3];
    U32 autoMenuUp;
    U32 autoMenuDown;
    U32 autoMenuLeft;
    U32 autoMenuRight;
    U32 custom;
    U32 customWidget;
};

struct zUIMotionAsset : xBaseAsset
{
    U8 cmdCount;
    U8 in;
    U8 pad[2];
    U32 cmdSize;
    F32 totalTime;
    F32 loopTime;
};

struct zUIMotionFrame
{
    F32 offsetX;
    F32 offsetY;
    F32 scaleX;
    F32 scaleY;
    F32 centerScaleX;
    F32 centerScaleY;
    F32 textScaleX;
    F32 textScaleY;
    F32 rotation;
    F32 offsetU;
    F32 offsetV;
    xColor color;
    U8 brightness;
};

class zUI : public xBase
{
    // zUI's member variables are split;
    // some of them are defined before the virtual functions
    // and some of them are defined after.
    // This affects where the vtable pointer is stored.
    // The current split gives the correct vtable offset,
    // so it shouldn't need to be changed.
    // However, it's likely that not everything should be public
    // and we might want to change some things to protected or private
    // (the Do* functions are likely protected and
    // all the member variables are likely private)
public:
    struct State
    {
        F32 x;
        F32 y;
        F32 width;
        F32 height;
        xColor color;
        U8 brightness;
        U8 pad[3];
    };

    zUIAsset* asset;
    State current;
    State startMovement;
    F32 z;
    zUIMotionAsset* selectedMotion;
    zUIMotionAsset* unselectedMotion;
    zUICustom* custom;
    bool visible;
    bool focus;
    bool lastFocus;
    bool selected;
    bool brighten;
    bool hdrPass;
    bool locked;

    zUI();

    zUIAsset* GetAsset() const;
    void HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                     U32 toParamWidgetID);
    void Signal(U32 event);
    void Update(F32 dt);
    void Render() const;
    void Exit();

    virtual U32 GetSortKey() const = 0;
    virtual bool Blends() const;
    virtual bool IsUIText();
    virtual bool IsForcePreUpdate();
    virtual void DoInit();
    virtual void DoSetup();
    virtual void DoReset();
    virtual void DoResetMotion();
    virtual void DoHandleEvent(xBase*, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                               U32);
    virtual void DoRender() const = 0;
    virtual void DoUpdate(F32 dt);
    virtual void DoExit();
    virtual void DoInitMotion();
    virtual void DoApplyMotionFrame(const zUIMotionFrame* frame);
    virtual void FocusOn();
    virtual void FocusOff();
    virtual void Select();
    virtual void Unselect();
    virtual void Visible();
    virtual void Invisible();

    zUIMotionAsset* motion;
    F32 motionTime;
    U8 motionFiredEvent;
    U8 motionLoop;
};

typedef void* (*zUICustomStaticAllocator)(U32 size, void* user);
typedef void (*zUICustomSignalHandler)(zUI* from, U32 dest, U32 event, const F32* param,
                                       xBase* paramWidget, U32 paramWidgetID, void* user);

void zUI_Init(zUI* text, zUIAsset* asset);
void* zUIAllocStaticMemory(U32 size);
void zUISetCustomStaticAllocator(zUICustomStaticAllocator allocator, void* user);
void zUISetCustomSignalHandler(zUICustomSignalHandler handler, void* user);

#endif
#ifndef ZLOADINGSCREEN_H
#define ZLOADINGSCREEN_H

#include "zAsync.h"
#include "zUI.h"

class zLoadingScreen : zAsync::Screen
{
public:
    static void UnSetShowStats();

    void Init();
    void Update(F32 dt);
    void Render();
    void Exit();
    void* CustomAllocate(U32 size);
    void CustomSignal(zUI* from, U32 dest, U32 event, const F32* param,
                      xBase* paramWidget, U32 paramWidgetID);

private:
    zUI* controls[16];
    S32 controlCount;
    void* allocations[32];
    S32 allocationCount;
};

#endif
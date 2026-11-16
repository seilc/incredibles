#include "xCommon.h"
#include "zLoadingScreen.h"

#include "zHashes.h"
#include "zUIMgr.h"
#include "zUIImage.h"
#include "zUIBox.h"
#include "zUIText.h"
#include "zUIController.h"
#include "xGroup.h"
#include "xstransvc.h"
#include "xEvent.h"
#include "xFont.h"

#include <string.h>

static bool show_stats = false;

namespace
{
    void* CustomStaticAllocator(U32 size, void* user)
    {
        return ((zLoadingScreen*)user)->CustomAllocate(size);
    }

    void CustomSignalHandler(zUI* from, U32 dest, U32 event, const F32* param,
                             xBase* paramWidget, U32 paramWidgetID, void* user)
    {
        ((zLoadingScreen*)user)->CustomSignal(from, dest, event, param, paramWidget, paramWidgetID);
    }
} // namespace

void zLoadingScreen::UnSetShowStats()
{
    show_stats = false;
}

void zLoadingScreen::Init()
{
    allocationCount = 0;

    zUISetCustomStaticAllocator(CustomStaticAllocator, this);

    controlCount = 0;

    xGroupAsset* group = (xGroupAsset*)xSTFindAsset(HASH_LOADING_SCREEN, NULL);

    show_stats = true;

    for (S16 i = 0; i < group->itemCount; i++) {
        U32 size;
        xDynAsset* asset = (xDynAsset*)xSTFindAsset(((U32*)(group + 1))[i], &size);

        if (asset) {
            void* test;
            zUI* element = NULL;

            switch (asset->type) {
            case HASH_ui_image:
                test = RwMalloc(sizeof(zUIImage), rwMEMHINTDUR_NADURATION);
                element = (zUI*)test;
                memset(element, 0, sizeof(zUIImage));
                zUIImage_Init(*element, *asset, size);
                break;
            case HASH_ui_box:
                element = (zUI*)RwMalloc(sizeof(zUIBox), rwMEMHINTDUR_NADURATION);
                memset(element, 0, sizeof(zUIBox));
                zUIBox_Init(*element, *asset, size);
                break;
            case HASH_ui_text:
                element = (zUI*)RwMalloc(sizeof(zUIText), rwMEMHINTDUR_NADURATION);
                memset(element, 0, sizeof(zUIText));
                zUIText_Init(*element, *asset, size);
                break;
            case HASH_ui_controller:
                element = (zUI*)RwMalloc(sizeof(zUIController), rwMEMHINTDUR_NADURATION);
                memset(element, 0, sizeof(zUIController));
                zUIController_Init(*element, *asset, size);
                break;
            }

            if (element) {
                controls[controlCount++] = element;
                allocations[allocationCount++] = element;
            }
        }
    }

    zUISetCustomStaticAllocator(NULL, NULL);
    zUISetCustomSignalHandler(CustomSignalHandler, this);
    zUIMgrSortList(controls, controlCount);

    for (S32 i = 0; i < controlCount; i++) {
        controls[i]->HandleEvent(controls[i], eEventVisible, NULL, NULL, 0);
        controls[i]->Signal(eEventVisible);
    }
}

void zLoadingScreen::Update(F32 dt)
{
    for (S32 i = 0; i < controlCount; i++) {
        if (controls[i]->visible) {
            controls[i]->Update(dt);
        }
    }
}

void zLoadingScreen::Render()
{
    for (S32 i = controlCount - 1; i >= 0; i--) {
        if (controls[i]->visible) {
            controls[i]->Render();
        }
    }

    xFontPrintTopText();
}

void zLoadingScreen::Exit()
{
    for (S32 j = 0; j < controlCount; j++) {
        controls[j]->Exit();
    }

    zUISetCustomSignalHandler(NULL, NULL);

    for (S32 i = 0; i < allocationCount; i++) {
        RwFree(allocations[i]);
    }

    allocationCount = 0;
}

void* zLoadingScreen::CustomAllocate(U32 size)
{
    allocations[allocationCount] = RwMalloc(size, rwMEMHINTDUR_NADURATION);

    return allocations[allocationCount++];
}

void zLoadingScreen::CustomSignal(zUI* from, U32 dest, U32 event, const F32* param,
                                  xBase* paramWidget, U32 paramWidgetID)
{
    for (S32 i = 0; i < controlCount; i++) {
        if (dest == controls[i]->GetAsset()->id) {
            controls[i]->HandleEvent(from, event, param, paramWidget, paramWidgetID);
            break;
        }
    }
}
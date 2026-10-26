#include <dolphin.h>
#include <dolphin/ax.h>

#include "__ax.h"

const char* __AXVersion = "<< Dolphin SDK - AX\trelease build: Dec  2 2003 05:10:55 (0x2301) >>";

void AXInit(void) {
    AXInitEx(0);
}

void AXInitEx(u32 outputBufferMode) {
#ifdef DEBUG
    OSReport("Initializing AX\n");
#endif
    OSRegisterVersion(__AXVersion);

    __AXAllocInit();
    __AXVPBInit();
    __AXSPBInit();
    __AXAuxInit();
    __AXClInit();
    __AXOutInit(outputBufferMode);
}

void AXQuit(void) {
#ifdef DEBUG
    OSReport("Shutting down AX\n");
#endif
    __AXAllocQuit();
    __AXVPBQuit();
    __AXSPBQuit();
    __AXAuxQuit();
    __AXClQuit();
    __AXOutQuit();
}

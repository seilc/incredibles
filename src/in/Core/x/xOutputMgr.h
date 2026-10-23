#ifndef XOUTPUTMGR_H
#define XOUTPUTMGR_H

#include "iDebug.h"

enum xOutSeverity
{
    xOutSevIgnore = -1,
    xOutSevCrit = 1,
    xOutSevErr,
    xOutSevWarn,
    xOutSevInfo,
    xOutSevBabble,
    xOutSevForce = FORCEENUMSIZEINT
};

#if DEBUG || RELEASE
#define xWARNINVALID(line, name, cond)                                                              \
do {                                                                                                \
    if (!(cond) && (xOutTrumps((name), xOutSevWarn) || xOutEnabled((name)) && xOutGetSev() >= xOutSevWarn)) {   \
        iprintf(">>> INVALID:" #cond);                                                              \
        iprintf(" in %s in %s(%d)", __FUNCTION__, __FILE__, (line));                                \
        iprintf("\n");                                                                              \
    }                                                                                               \
} while (0)

#define xTRACEFUNCTION(line, name)                                                                  \
do {                                                                                                \
    if (xOutTrumps((name), xOutSevInfo) || (xOutEnabled((name)) && xOutGetSev() >= xOutSevInfo)) {  \
        iprintf("~~ entering %s in %s(%d)", __FUNCTION__, __FILE__, (line));                        \
        iprintf("\n");                                                                              \
    }                                                                                               \
} while (0)

void xOutSetSev(S32 sev);
S32 xOutGetSev();
void xOutEnable(char* name, xOutSeverity trumpsev);
S32 xOutEnabled(char* name);
S32 xOutTrumps(char* name, xOutSeverity usersev);
S32 xOutEnabled(U32 hval);
S32 xOutTrumps(U32 hval, xOutSeverity usersev);
void xOutCrit(const char* fmt, ...);
void xOutErr(char* name, const char* fmt, ...);
void xOutWarn(char* name, const char* fmt, ...);
void xOutInfo(char* name, const char* fmt, ...);
void xOutBabble(char* name, const char* fmt, ...);
#else
#define xWARNINVALID(line, name, cond)
#define xTRACEFUNCTION(line, name)

#define xOutSetSev(sev)
#define xOutGetSev 0
#define xOutEnable(name)
#define xOutEnabled 0
#define xOutTrumps 0
#define xOutCrit(name, ...)
#define xOutErr(name, ...)
#define xOutWarn(name, ...)
#define xOutInfo
#define xOutBabble(name, ...)
#endif

#endif

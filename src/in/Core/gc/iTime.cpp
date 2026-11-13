#include "xCommon.h"
#include "iTime.h"

iTime sStartupTime;

void iTimeInit()
{
    sStartupTime = OSGetTime();
}

void iTimeExit()
{
}

iTime iTimeGet()
{
    iTime curTime = OSGetTime();
    iTime time = curTime - sStartupTime;
    return time;
}

F32 iTimeDiffSec(iTime time)
{
    F32 sec = time;
    return OSTicksToSeconds(sec);
}

F32 iTimeDiffSec(iTime t0, iTime t1)
{
    return iTimeDiffSec(t1 - t0);
}

static F32 sGameTime;

F32 iTimeGetGame()
{
    return sGameTime;
}

void iTimeGameAdvance(F32 elapsed)
{
    sGameTime += elapsed;
}

void iTimeSetGame(F32 time)
{
    sGameTime = time;
}

void iProfileClear(U32)
{
}

void iFuncProfileDump()
{
}

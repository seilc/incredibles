#ifndef ISND_H
#define ISND_H

#include <types.h>

enum iSndHandle
{
};

enum iSndGroupHandle
{
};

#define ISNDHANDLE_INVALID ((iSndHandle)-1)
#define ISND_INVALID_SND_GROUP_HANDLE ((iSndGroupHandle)0xFFFFFFFF)

void iSndSceneExit();

#endif
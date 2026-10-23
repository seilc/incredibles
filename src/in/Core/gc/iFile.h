#ifndef IFILE_H
#define IFILE_H

#include <types.h>

typedef struct tag_iFile
{
    U32 flags;
    char path[128];
    S32 fd;
    S32 offset;
    S32 length;
} iFile;

bool iFileExists(const char* name, S32);
void iFileAsyncService();

#endif
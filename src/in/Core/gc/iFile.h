#ifndef IFILE_H
#define IFILE_H

#include <types.h>

#include <dolphin.h>

typedef struct tag_xFile xFile;

enum xFileDriveType
{
    XFILE_DRIVE_CDVD,
    XFILE_DRIVE_CONSOLE_HD
};

typedef struct tag_iFile
{
    U32 flags;
    char path[128];
    S32 fd;
    DVDFileInfo file;
    void(*cb)(tag_xFile*);
    S32 akey;
    S32 buffer[8];
    S32 real_pos;
} iFile;

U32* iFileLoad(const char* name, U32* buffer, U32* size, S32 flags);
U32 iFileOpen(const char* name, S32 flags, xFile* file);
S32 iFileSeek(xFile* file, S32 offset, S32 whence);
U32 iFileRead(xFile* file, void* buf, U32 size);
S32 iFileReadAsync(xFile* file, void* buf, U32 asize, void(*dcb)(xFile*), S32);
U32 iFileClose(xFile* file);
U32 iFileGetSize(xFile* file);
U32 iFileGetSectorSize(xFileDriveType drive);
bool iFileExists(const char* name, S32);
void iFileAsyncService();

#endif

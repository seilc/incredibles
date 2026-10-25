#ifndef IFILE_H
#define IFILE_H

#include "types.h"

#include <dolphin.h>

#define IFILE_READ (1 << 0)
#define IFILE_WRITE (1 << 1)

typedef struct tag_xFile xFile;
typedef void (*iFileReadAsyncCallback)(xFile*);

enum xFileDriveType
{
    XFILE_DRIVE_CDVD,
    XFILE_DRIVE_CONSOLE_HD
};

enum IFILE_READSECTOR_STATUS
{
    IFILE_RDSTAT_NOOP,
    IFILE_RDSTAT_INPROG,
    IFILE_RDSTAT_DONE,
    IFILE_RDSTAT_FAIL,
    IFILE_RDSTAT_QUEUED,
    IFILE_RDSTAT_EXPIRED
};

typedef struct tag_iFile
{
    U32 flags;
    char path[128];
    S32 fd;
    DVDFileInfo file;
    iFileReadAsyncCallback cb;
    S32 akey;
    S32 buffer[8];
    S32 real_pos;
} iFile;

U32* iFileLoad(const char* name, U32* buffer, U32* size, S32 flags);
U32 iFileOpen(const char* name, S32 flags, xFile* file);
S32 iFileSeek(xFile* file, S32 offset, S32 whence);
U32 iFileRead(xFile* file, void* buf, U32 size);
void iFileAsyncService();
S32 iFileReadAsync(xFile* file, void* buf, U32 asize, iFileReadAsyncCallback dcb, S32);
IFILE_READSECTOR_STATUS iFileReadAsyncStatus(S32 key, S32* bytes_read);
U32 iFileClose(xFile* file);
U32 iFileGetSize(xFile* file);
bool iFileExists(const char* name, S32);
U32 iFileFind(const char* name, S32 fileflags, xFile* file);
void iFileGetInfo(xFile* file, U32* starting_sector, U32* size_in_bytes);
U32 iFileGetSectorSize(xFileDriveType drive);

#endif

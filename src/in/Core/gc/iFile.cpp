#include "xCommon.h"
#include "iFile.h"

#include "xFile.h"
#include "xTRC.h"
#include "iTRC.h"

#include <rwcore.h>
#include <stdio.h>
#include <string.h>

#define MAX_FILE_QUEUE 4

struct file_queue_record
{
    xFile* finfo;
    void* buf;
    U32 read_size;
    U32 cur_read;
    IFILE_READSECTOR_STATUS status;
    iFileReadAsyncCallback done;
    u32 key;
};

static S32 tbuffer[1032];
static S32* buffer32;

void iFileInit()
{
    S32 ba = (S32)tbuffer;
    ba += 31;
    ba >>= 5;
    ba <<= 5;
    buffer32 = (S32*)ba;
}

void iFileExit()
{
}

U32* iFileLoad(const char* name, U32* buffer, U32* size)
{
    char fullpath[IFILE_NAMELEN_MAX];
    xFile file;

    iFileFullPath(name, fullpath);

    // BUG: opens name, not fullpath
    iFileOpen(name, IFILE_FULLPATH, &file);
    U32 fsize = iFileGetSize(&file);

    if (*size) {
    }

    fsize += 31;
    fsize &= ~31;
    if (!buffer) {
        buffer = (U32*)RwMalloc(fsize, 0);
    }
    iFileRead(&file, buffer, fsize);
    if (size) *size = fsize;
    iFileClose(&file);
    return buffer;
}

U32 iFileOpen(const char* name, S32 flags, xFile* file)
{
    iFile* ps = &file->ps;

    if (flags & IFILE_FULLPATH) {
        strcpy(ps->path, name);
    } else {
        iFileFullPath(name, ps->path);
    }

    ps->fd = DVDConvertPathToEntrynum(ps->path);
    if (ps->fd == -1)
        return 1;
    if (!DVDFastOpen(ps->fd, &ps->file)) {
        ps->fd = -1;
        return 1;
    }

    ps->cb = NULL;
    ps->flags = IFILE_OPENED;
    iFileSeek(file, 0, SEEK_SET);
    return 0;
}

S32 iFileSeek(xFile* file, S32 offset, S32 whence)
{
    iFile* ps = &file->ps;
    S32 real_offset;

    switch (whence) {
    case SEEK_SET:
        real_offset = offset;
        break;
    case SEEK_CUR:
        if (DVDGetCommandBlockStatus(&ps->file.cb) == DVD_STATE_BUSY) {
            return -1;
        }
        real_offset = offset + ps->real_pos;
        break;
    case SEEK_END:
        real_offset = ps->file.length - offset;
        if (real_offset < 0) real_offset = 0;
        break;
    default:
        real_offset = offset;
        break;
    }

    ps->real_pos = real_offset;
    return ps->real_pos;
}

U32 iFileSyncAsyncReadActive;

static void ifilereadCB(xFile*)
{
    iFileSyncAsyncReadActive = FALSE;
}

U32 iFileRead(xFile* file, void* buf, U32 size)
{
    iFileSeek(file, file->ps.real_pos, SEEK_SET);
    iFileSyncAsyncReadActive = TRUE;

    for (;;) {
        S32 result = iFileReadAsync(file, buf, size, ifilereadCB, 0);
        if (result != -1) {
            break;
        }

        iTRC_CHECK();
        iFileAsyncService();
    }

    while (iFileSyncAsyncReadActive) {
        iTRC_CHECK();
        iFileAsyncService();
    }
    file->ps.real_pos += size;
    return size;
}

file_queue_record file_queue[MAX_FILE_QUEUE];

static void async_cb(s32 result, DVDFileInfo* finfo)
{
    S32 i = (S32)((DVDCommandBlock*)finfo)->userData;
    file_queue_record* fq = &file_queue[i];
    char* p;
    s32 next_read;
    s32 bytes = -1;

    switch (result) {
    case DVD_RESULT_CANCELED:
        break;
    case DVD_RESULT_FATAL_ERROR:
        xTRCDisk(TRC_DiskFatal);
        return;
    default:
        if (result >= 0) bytes = result;
        break;
    }

    if (bytes < 0) {
        fq->status = IFILE_RDSTAT_FAIL;
        return;
    }

    if (bytes + fq->cur_read >= fq->read_size ||
        bytes + fq->cur_read + fq->finfo->ps.real_pos >= fq->finfo->ps.file.length) {
        fq->status = IFILE_RDSTAT_DONE;
        fq->cur_read = fq->read_size;
        return;
    }

    fq->cur_read += bytes;
    fq->status = IFILE_RDSTAT_INPROG;
    if (fq->read_size - fq->cur_read < 0x8000) {
        next_read = fq->read_size - fq->cur_read;
        next_read += 3;
        next_read &= ~3;
    } else {
        next_read = 0x8000;
    }

    if (next_read + fq->cur_read + fq->finfo->ps.real_pos > fq->finfo->ps.file.length) {
        next_read = fq->finfo->ps.file.length;
        next_read -= fq->finfo->ps.real_pos;
        next_read -= fq->cur_read;
        next_read += 31;
        next_read &= ~31;
    }

    p = (char*)fq->buf;
    p += fq->cur_read;
    DVDReadAsync(&fq->finfo->ps.file, p, next_read, fq->finfo->ps.real_pos + fq->cur_read, async_cb);
}

void iFileAsyncService()
{
    for (S32 i = 0; i < MAX_FILE_QUEUE; i++) {
        file_queue_record& fq = file_queue[i];
        if (fq.done && (fq.status == IFILE_RDSTAT_DONE || fq.status == IFILE_RDSTAT_FAIL)) {
            fq.done(fq.finfo);
            fq.done = NULL;
            fq.finfo->ps.akey = -1;
        }
    }
}

S32 iFileReadAsync(xFile* file, void* buf, U32 asize, iFileReadAsyncCallback dcb, S32)
{
    static u32 fopcount = 1;

    for (S32 i = 0; i < MAX_FILE_QUEUE; i++) {
        file_queue_record& fq = file_queue[i];
        if (!fq.done && fq.status != IFILE_RDSTAT_QUEUED && fq.status != IFILE_RDSTAT_INPROG) {
            s32 read_size = 32;
            u32 key = fopcount++;
            key <<= 2;

            fq.finfo = file;
            fq.buf = buf;
            fq.read_size = asize;
            fq.cur_read = 0;
            fq.status = IFILE_RDSTAT_QUEUED;
            fq.done = dcb;
            fq.key = key + i;
            if (read_size > asize) {
                read_size = asize;
                read_size += 3;
                read_size &= ~3;
            }

            file->ps.file.cb.userData = (void*)i;
            DVDReadAsync(&file->ps.file, buf, read_size, file->ps.real_pos, async_cb);
            file->ps.akey = key + i;
            return (S32)key + i;
        }
    }
    return -1;
}

IFILE_READSECTOR_STATUS iFileReadAsyncStatus(S32 key, S32* bytes_read)
{
    u32 i = (u32)key & 3;

    if (key != file_queue[i].key) {
        return IFILE_RDSTAT_EXPIRED;
    }
    if (bytes_read) {
        *bytes_read = file_queue[i].cur_read;
    }
    return file_queue[i].status;
}

U32 iFileClose(xFile* file)
{
    iFile* ps = &file->ps;

    DVDClose(&ps->file);
    S32 ret = DVDClose(&ps->file);
    if (!ret) {
        return 1;
    }

    ps->flags = 0;
    return 0;
}

U32 iFileGetSize(xFile* file)
{
    iFile* ps = &file->ps;

    return ps->file.length;
}

bool iFileExists(const char* name, S32)
{
    char fullname[IFILE_NAMELEN_MAX];

    iFileFullPath(name, fullname);

    S32 fd = DVDConvertPathToEntrynum(fullname);
    return fd != -1;
}

U32 iFileReadStop()
{
    return DVDCancelAllAsync(NULL);
}

char gHostPath[256];

void iFileFullPath(const char* relname, char* fullname)
{
    const char* inchar = relname;
    char* outchar = fullname;
    while (*inchar) {
        if (*inchar == '\\')
            *outchar = '/';
        else
            *outchar = *inchar;
        inchar++;
        outchar++;
    }
    *outchar = '\0';
}

void iFileSetPath(const char* path)
{
    U32 len = strlen(path);

    printf("iFileSetPath:  \"%s\"\n", path);
    strcpy(gHostPath, path);
    if (len && gHostPath[len - 1] != '/' && gHostPath[len - 1] != '\\') {
        gHostPath[len] = '/';
        gHostPath[len + 1] = '\0';
    }
}

U32 iFileFind(const char* name, S32 fileflags, xFile* file)
{
    return iFileOpen(name, fileflags, file);
}

void iFileGetInfo(xFile* file, U32* starting_sector, U32* size_in_bytes)
{
    iFile* ps = &file->ps;

    if (starting_sector) {
        *starting_sector = ps->file.startAddr;
    }
    if (size_in_bytes) {
        *size_in_bytes = ps->file.length;
    }
}

U32 iFileGetSectorSize(xFileDriveType drive)
{
    switch (drive) {
    case XFILE_DRIVE_CDVD:
        return 32;
    }
    return 0;
}

#ifndef XFILE_H
#define XFILE_H

#include "iFile.h"

#define XFILE_RELNAME_MAX 32

#define XFILE_READ IFILE_READ
#define XFILE_WRITE IFILE_WRITE

enum XFILE_READSECTOR_STATUS
{
    XFILE_RDSTAT_NOOP,
    XFILE_RDSTAT_INPROG,
    XFILE_RDSTAT_DONE,
    XFILE_RDSTAT_FAIL,
    XFILE_RDSTAT_QUEUED,
    XFILE_RDSTAT_EXPIRED
};

typedef struct tag_xFile
{
    char relname[XFILE_RELNAME_MAX];
    iFile ps;
    void* user_data;
} xFile;

inline void xFileSetUserData(xFile* xf, void* p) { xf->user_data = p; }
inline void* xFileGetUserData(xFile* xf) { return xf->user_data; }

inline XFILE_READSECTOR_STATUS xFileReadAsyncStatus(S32 key, S32* amtSoFar)
{
    return (XFILE_READSECTOR_STATUS)iFileReadAsyncStatus(key, amtSoFar);
}

#endif

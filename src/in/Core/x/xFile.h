#ifndef XFILE_H
#define XFILE_H

#include "iFile.h"
#include "xDebug.h"

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
    XFILE_READSECTOR_STATUS x;
    IFILE_READSECTOR_STATUS i;

    xVALIDATE_L(116, (x=XFILE_RDSTAT_INPROG) == (i=IFILE_RDSTAT_INPROG));
    xVALIDATE_L(117, (x=XFILE_RDSTAT_DONE) == (i=IFILE_RDSTAT_DONE));
    xVALIDATE_L(118, (x=XFILE_RDSTAT_FAIL) == (i=IFILE_RDSTAT_FAIL));

    return (XFILE_READSECTOR_STATUS)iFileReadAsyncStatus(key, amtSoFar);
}

#endif

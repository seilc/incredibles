#include "xbinio.h"

#include "xFile.h"
#include "xDebug.h"

#include "decomp.h"

#include <string.h>

#define MAX_LOADINST 8
#define FIOIO_GUESS_ALIGN 2

struct st_BINIO_XTRADATA
{
    char* dbl_buf;
    S32 dblbuf_size;
    S32 dbl_beg;
    S32 dbl_end;
    S32 dbl_amt;
    U32 fpos;
    char* asyn_data;
    S32 asyn_amt;
    S32 asyn_elesize;
    S32 asyn_ismot;
    en_BIO_ASYNC_ERRCODES asyn_status;
    U32 pad[3];
    S32 gcaskey;
};

static U32 g_loadlock = 0xFFFFFF00;
static st_FILELOADINFO g_loadinst[MAX_LOADINST] = {};
static xFile g_xfload[MAX_LOADINST] = {};
static st_BINIO_XTRADATA g_xtraload[MAX_LOADINST] = {};
static st_BINIO_XTRADATA* g_async_context = NULL;

static void LoadDestroy(st_FILELOADINFO* fli);
static S32 SkipBytes(st_FILELOADINFO* fli, S32 fwd);
static S32 ReadSeek(st_FILELOADINFO* fli, S32 pos);
static void SetBuffer(st_FILELOADINFO* fli, char* dblbuffer, S32 bufsize);
static void DiscardBuffer(st_FILELOADINFO* fli);
static S32 ReadRaw(st_FILELOADINFO* fli, void* data, S32 size, S32 count);
static S32 ReadBytes(st_FILELOADINFO* fli, char* data, S32 count);
static S32 ReadMShorts(st_FILELOADINFO* fli, S16* data, S32 count);
static S32 ReadMLongs(st_FILELOADINFO* fli, S32* data, S32 count);
static S32 ReadMFloats(st_FILELOADINFO* fli, F32* data, S32 count);
static S32 ReadMDoubles(st_FILELOADINFO* fli, F64* data, S32 count);
static S32 ReadIShorts(st_FILELOADINFO* fli, S16* data, S32 count);
static S32 ReadILongs(st_FILELOADINFO* fli, S32* data, S32 count);
static S32 ReadIFloats(st_FILELOADINFO* fli, F32* data, S32 count);
static S32 ReadIDoubles(st_FILELOADINFO* fli, F64* data, S32 count);
static S32 AsyncMRead(st_FILELOADINFO* fli, S32 offset, char* data, S32 size, S32 n);
static S32 AsyncIRead(st_FILELOADINFO* fli, S32 offset, char* data, S32 size, S32 n);
static en_BIO_ASYNC_ERRCODES AsyncReadStatus(st_FILELOADINFO* fli);
static void Swap2(char* d, S32 n);
static void Swap4(char* d, S32 n);
static void Swap8(char* d, S32 n);
static xFile* BFD_open(const char* filename, const char* mode, U32 lockid, S32, void* xtradata, S32 fileflags);
static void BFD_close(xFile* bffp, void* xtradata);
static S32 BFD_read(void* data, S32 elesize, S32 elecnt, xFile* bffp, void* xtradata);
static S32 BFD_seek(xFile* bffp, S32 offset, S32 refpos, void* xtradata);
static S32 BFD_getLength(xFile* bffp, void*);
static U32 BFD_startSector(const char* filename, S32 fileflags);
static void BFD_cb_GCP2_readasync(xFile* file);
static S32 BFD_AsyncRead(st_FILELOADINFO* fli, S32 pos, void* data, S32 size, S32 n, S32);
static en_BIO_ASYNC_ERRCODES BFD_AsyncReadStatus(st_FILELOADINFO* fli);

st_FILELOADINFO* xBinioLoadCreate(const char* filename, S32 fileflags)
{
    st_FILELOADINFO* fli = NULL;
    xFile* tmp_fp = NULL;
    S32 i = 0;
    S32 uselock = -1;

    for (i = 0; i < MAX_LOADINST; i++) {
        if (!(g_loadlock & (1 << i))) {
            g_loadlock |= 1 << i;
            fli = &g_loadinst[i];
            uselock = i;
            break;
        }
    }

    xVALIDATE_L(439, fli);

    if (fli) {
        memset(fli, 0, sizeof(st_FILELOADINFO));

        xVALIDATE_L(444, uselock>-1);
        fli->lockid = (uselock < 0) ? 0 : uselock;

        fli->xtradata = &g_xtraload[uselock];

        fli->destroy = LoadDestroy;
        fli->readBytes = ReadBytes;

        fli->readMShorts = ReadMShorts;
        fli->readMLongs = ReadMLongs;
        fli->readMFloats = ReadMFloats;
        fli->readMDoubles = ReadMDoubles;

        fli->readIShorts = ReadIShorts;
        fli->readILongs = ReadILongs;
        fli->readIFloats = ReadIFloats;
        fli->readIDoubles = ReadIDoubles;

        fli->asyncIRead = AsyncIRead;
        fli->asyncMRead = AsyncMRead;
        fli->asyncReadStatus = AsyncReadStatus;

        fli->skipBytes = SkipBytes;
        fli->seekSpot = ReadSeek;

        fli->setDoubleBuf = SetBuffer;
        fli->discardDblBuf = DiscardBuffer;

        fli->error = FIOERR_NONE;
        fli->remain = 0;
        fli->basesector = 0;
        fli->filesize = 0;
        fli->position = 0;

        fli->basesector = 0;

        tmp_fp = BFD_open(filename, "rb", uselock, 0, fli->xtradata, fileflags);
        if (tmp_fp) {
            fli->basesector = BFD_startSector(filename, fileflags);
            fli->privdata = tmp_fp;
            fli->filesize = BFD_getLength(tmp_fp, fli->xtradata);
            fli->remain = fli->filesize;
        } else {
            xWARN("Failed to open read file %s\n", filename);
            LoadDestroy(fli);
            fli = NULL;
        }
    }

    return fli;
}

static void LoadDestroy(st_FILELOADINFO* fli)
{
    xFile* fp = (xFile*)fli->privdata;
    U32 lockid = 0;

    xVALIDATE_L(523, fli);

    if (fp) BFD_close(fp, fli->xtradata);

    lockid = fli->lockid;
    memset(fli, 0, sizeof(st_FILELOADINFO));
    g_loadlock &= ~(1 << lockid);
}

static S32 SkipBytes(st_FILELOADINFO* fli, S32 fwd)
{
    xFile* file = (xFile*)fli->privdata;
    S32 rc = 0;

    if (fli->error != FIOERR_NONE) return 0;

    if (fwd == 0) return 1;
    if (fwd < 0) fwd = 0;

    rc = BFD_seek(file, fli->position + fwd, 0, fli->xtradata);
    xVALIDATE_L(591, !rc);
    if (rc) fli->error = FIOERR_SEEKFAIL;
    else {
        fli->remain -= fwd;
        fli->position += fwd;
        xVALIDATE_L(596, fli->position<=fli->filesize);
        xVALIDATE_L(597, fli->remain>=0);
    }

    xVALIDATE_L(600, fli->error == FIOERR_NONE);
    if (fli->error != FIOERR_NONE) return 0;
    else return 1;
}

static S32 ReadSeek(st_FILELOADINFO* fli, S32 pos)
{
    xFile* file = (xFile*)fli->privdata;
    S32 rc = 0;

    if (fli->error != FIOERR_NONE) return 0;

    xVALIDATE_L(619, pos<=fli->filesize);
    pos = (pos < fli->filesize) ? pos : fli->filesize;

    rc = BFD_seek(file, pos, 0, fli->xtradata);
    xVALIDATE_L(623, !rc);
    if (rc) fli->error = FIOERR_SEEKFAIL;
    else {
        fli->position = pos;
        fli->remain = fli->filesize - pos;
        xVALIDATE_L(628, fli->remain>=0);
    }

    xVALIDATE_L(631, fli->error == FIOERR_NONE);
    if (fli->error != FIOERR_NONE) return 0;
    else return 1;
}

static void SetBuffer(st_FILELOADINFO* fli, char* dblbuffer, S32 bufsize)
{
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)fli->xtradata;
    if (xtra) {
        xtra->dbl_buf = dblbuffer; xtra->dblbuf_size = bufsize; xtra->dbl_beg = 0; xtra->dbl_end = 0; xtra->dbl_amt = 0;
    }
}

static void DiscardBuffer(st_FILELOADINFO* fli)
{
    SetBuffer(fli, NULL, 0);
}

static S32 ReadRaw(st_FILELOADINFO* fli, void* data, S32 size, S32 count)
{
    xFile* file = (xFile*)fli->privdata;
    S32 amt = 0;
    S32 n = 0;

    if (fli->error != FIOERR_NONE) return amt;

    xVALIDATE_L(682, (size * count) <= fli->remain);
    if ((size * count) > fli->remain) amt = fli->remain / size;
    else amt = count;
    xVALIDATE_L(685, amt);
    if (amt) {
        n = BFD_read(data, size, amt, file, fli->xtradata);

        if (n != amt) fli->error = FIOERR_READFAIL;

        fli->remain -= size * amt;
        fli->position += size * amt;
        xVALIDATE_L(697, fli->remain>=0);
        xVALIDATE_L(698, fli->position<=fli->filesize);
    }

    return amt;
}

static S32 ReadBytes(st_FILELOADINFO* fli, char* data, S32 count)
{
    S32 act = 0;
    act = ReadRaw(fli, data, sizeof(char), count);
    return act;
}

static S32 ReadMShorts(st_FILELOADINFO* fli, S16* data, S32 count)
{
    S32 act = 0;
    act = ReadRaw(fli, data, sizeof(S16), count);
    return act;
}

static S32 ReadMLongs(st_FILELOADINFO* fli, S32* data, S32 count)
{
    S32 act = 0;
    act = ReadRaw(fli, data, sizeof(S32), count);
    return act;
}

static S32 ReadMFloats(st_FILELOADINFO* fli, F32* data, S32 count)
{
    S32 act = 0;
    act = ReadRaw(fli, data, sizeof(F32), count);
    return act;
}

static S32 ReadMDoubles(st_FILELOADINFO* fli, F64* data, S32 count)
{
    S32 act = 0;
    act = ReadRaw(fli, data, sizeof(F64), count);
    return act;
}

static S32 ReadIShorts(st_FILELOADINFO* fli, S16* data, S32 count)
{
    S32 act = 0;
    act = ReadRaw(fli, data, sizeof(S16), count);
    Swap2((char*)data, act);
    return act;
}

static S32 ReadILongs(st_FILELOADINFO* fli, S32* data, S32 count)
{
    S32 act = 0;
    act = ReadRaw(fli, data, sizeof(S32), count);
    Swap4((char*)data, act);
    return act;
}

static S32 ReadIFloats(st_FILELOADINFO* fli, F32* data, S32 count)
{
    S32 act = 0;
    act = ReadRaw(fli, data, sizeof(F32), count);
    Swap4((char*)data, act);
    return act;
}

static S32 ReadIDoubles(st_FILELOADINFO* fli, F64* data, S32 count)
{
    S32 act = 0;
    act = ReadRaw(fli, data, sizeof(F64), count);
    Swap8((char*)data, act);
    return act;
}

static S32 AsyncMRead(st_FILELOADINFO* fli, S32 offset, char* data, S32 size, S32 n)
{
    return BFD_AsyncRead(fli, offset, data, size, n, 1);
}

static S32 AsyncIRead(st_FILELOADINFO* fli, S32 offset, char* data, S32 size, S32 n)
{
    return BFD_AsyncRead(fli, offset, data, size, n, 0);
}

static en_BIO_ASYNC_ERRCODES AsyncReadStatus(st_FILELOADINFO* fli)
{
    return BFD_AsyncReadStatus(fli);
}

DECOMP_FORCEACTIVE(DEBUG ? "%s(%d) : (sizeof(S32)==4) in '%s'\n" : 0)

static void Swap2(char* d, S32 n)
{
    char t;

    while (n--) {
        t = d[0];
        d[0] = d[1];
        d[1] = t;

        d += 2;
    }
}

static void Swap4(char* d, S32 n)
{
    char t = 0;

    while (n--) {
        t = d[0];
        d[0] = d[3];
        d[3] = t;

        t = d[1];
        d[1] = d[2];
        d[2] = t;

        d += 4;
    }
}

static void Swap8(char* d, S32 n)
{
    char t = 0;

    while (n--) {
        t = d[0];
        d[0] = d[7];
        d[7] = t;

        t = d[1];
        d[1] = d[6];
        d[6] = t;

        t = d[2];
        d[2] = d[5];
        d[5] = t;

        t = d[3];
        d[3] = d[4];
        d[4] = t;

        d += 8;
    }
}

static void ALT_Swap2(S16* d, S32 n);
static void ALT_Swap4(S32* d, S32 n);
static void ALT_Swap8(S32* d, S32 n);

static xFile* BFD_open(const char* filename, const char* mode, U32 lockid, S32, void* xtradata, S32 fileflags)
{
    xFile* bffp = NULL;
    U32 orc = 0;
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)xtradata;
    S32 xfflg = XFILE_READ;

    xVALIDATE_L(1586, strlen(filename)<(XFILE_RELNAME_MAX-1));

    if (strcmp(mode, "rb") == 0) {
        bffp = &g_xfload[lockid];
        xfflg = XFILE_READ;
    }
    else if (strcmp(mode, "wb") == 0) {
        xWARN("WARNING: BINIO write operation nonsupported\n");
        bffp = NULL;
        xfflg = XFILE_WRITE;
    } else {
        xWARN("Unknown/Unsupported file open state request (%s)\n", mode);
        bffp = NULL;
    }

    xVALIDATE_L(1606, bffp);

    if (bffp) {
        strncpy(bffp->relname, filename, XFILE_RELNAME_MAX - 1);
        bffp->relname[XFILE_RELNAME_MAX - 1] = '\0';

        if (fileflags) {
            xfflg |= fileflags;
        }

        orc = iFileOpen(filename, xfflg, bffp);
        if (orc) {
            xWARN("xFileOpen failed for %s(%d)\n", filename, orc);
            bffp = NULL;
        } else {
            xtra->fpos = 0;
            xtra->dbl_buf = NULL;
            xtra->dblbuf_size = 0;
            xtra->dbl_beg = 0;
            xtra->dbl_end = 0;
            xtra->dbl_amt = 0;
        }
    }

    return bffp;
}

static void BFD_close(xFile* bffp, void* xtradata)
{
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)xtradata;

    xVALIDATE_L(1653, bffp);
    xVALIDATE_L(1654, xtra);
    iFileClose(bffp);

    xtra->fpos = 0;
    xtra->dbl_buf = NULL;
    xtra->dblbuf_size = 0;
    xtra->dbl_beg = 0;
    xtra->dbl_end = 0;
    xtra->dbl_amt = 0;
}

static S32 BFD_read(void* data, S32 elesize, S32 elecnt, xFile* bffp, void* xtradata)
{
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)xtradata;
    char* dest = (char*)data;
    S32 readbeg = 0;
    S32 refill = 0;
    S32 remain = 0;
    S32 actual = 0;
    U32 holdpos = 0;
    S32 toread = 0;
    char* cltoff = NULL;
    char* sectoff = NULL;
    U32 safety = 0;
    U32 numBytes = elesize * elecnt;

    xVALIDATE_L(1718, xtra);
    xVALIDATE_L(1719, dest);
    xVALIDATE_L(1720, numBytes);
    if (!numBytes) return actual;

    if (!xtra->dbl_buf || xtra->dblbuf_size < 1 || (S32)numBytes > xtra->dblbuf_size) {
        xVALIDATE_L(1726, !(xtra->fpos % FIOIO_GUESS_ALIGN));

        xVERBOSE("BINIO:read - direct passthru (code: %d)\n", numBytes);

        iFileSeek(bffp, xtra->fpos, 0);
        actual = iFileRead(bffp, dest, numBytes);
        xVALIDATE_L(1739, actual==(S32)numBytes);

        xtra->fpos += actual;
    } else {
        xVALIDATE_L(1748, xtra->dbl_buf && (xtra->dblbuf_size > 0));

        remain = numBytes;
        xVERBOSE("BINIO:read - from buffer (code: %d)\n", numBytes);

        for (;;) {
            if (xtra->fpos < xtra->dbl_beg) {
                xtra->dbl_beg = xtra->fpos - (xtra->fpos % xtra->dblbuf_size);
                xtra->dbl_end = xtra->dbl_beg + xtra->dblbuf_size;
                xtra->dbl_amt = 0;
                refill = 1;
            }
            else if (xtra->fpos >= xtra->dbl_end) {
                xtra->dbl_beg = xtra->fpos - (xtra->fpos % xtra->dblbuf_size);
                xtra->dbl_end = xtra->dbl_beg + xtra->dblbuf_size;
                xtra->dbl_amt = 0;
                refill = 1;
            }

            if (refill) {
                holdpos = xtra->fpos;
                iFileSeek(bffp, xtra->dbl_beg, 0);
                xtra->fpos = holdpos;

                xtra->dbl_amt = iFileRead(bffp, xtra->dbl_buf, xtra->dblbuf_size);
            }

            readbeg = xtra->fpos - xtra->dbl_beg;
            xVALIDATE_L(1810, readbeg>=0);
            xVALIDATE_L(1811, readbeg<xtra->dbl_amt);

            if (xtra->dbl_amt > readbeg) {
                toread = (remain < xtra->dbl_amt - readbeg) ? remain : xtra->dbl_amt - readbeg;

                cltoff = dest + actual;
                sectoff = xtra->dbl_buf + readbeg;
                memcpy(cltoff, sectoff, toread);

                actual += toread;
                xtra->fpos += toread;
                remain -= toread;
            }

            xVALIDATE_L(1830, actual<=(S32)numBytes);
            if (actual == (S32)numBytes) break;
            else if (xtra->dbl_amt < 1) break;

            xVALIDATE_L(1834, safety < 64);
            if (safety++ > 60000) break;
        }
    }

    return actual / elesize;
}

static S32 BFD_write(void*, S32, S32, xFile*, void*);
static S32 BFD_tell(xFile*, void* xtradata);

static S32 BFD_seek(xFile* bffp, S32 offset, S32 refpos, void* xtradata)
{
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)xtradata;
    S32 rc = 0;

    rc = iFileSeek(bffp, offset, refpos);
    rc = (rc == -1) ? 1 : 0;
    xVALIDATE_L(1873, rc==0);

    xtra->fpos = offset;

    return rc;
}

static S32 BFD_getLength(xFile* bffp, void*)
{
    S32 len = 0;
    len = iFileGetSize(bffp);
    return len;
}

static U32 BFD_startSector(const char* filename, S32 fileflags)
{
    xFile xf = {};
    U32 sect_start = 0;
    U32 fsize = 0;

    U32 rc = iFileFind(filename, fileflags, &xf);
    if (!rc) {
        iFileGetInfo(&xf, &sect_start, &fsize);
    }
    iFileClose(&xf);

    return sect_start;
}

static void BFD_cb_asyncread(S32);

DECOMP_FORCEACTIVE(
    DEBUG ? "%s(%d) : (xtra->asyn_status == BINIO_ASYNC_INPROG) in '%s'\n" : 0,
    (DEBUG || RELEASE) ? "XBIN: Eh? Async callback ... ASS-U-ME success ?!?\n" : 0,
    (DEBUG || RELEASE) ? "XBIN: Wow, IOP callback was actually called!\n" : 0
)

static void BFD_cb_GCP2_readasync(xFile* file)
{
    return;

    st_FILELOADINFO* fli = (st_FILELOADINFO*)xFileGetUserData(file);
    xVALIDATE(fli);
    xFileSetUserData(file, NULL);
}

static S32 BFD_AsyncRead(st_FILELOADINFO* fli, S32 pos, void* data, S32 size, S32 n, S32)
{
    xFile* file = (xFile*)fli->privdata;
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)fli->xtradata;
    S32 result = 0;
    U32 rdsec = 0;
    S32 rc = 0;

    xVALIDATE_L(1995, pos>0);

    if (fli->error != FIOERR_NONE) return 0;
    xVALIDATE_L(1998, g_async_context==NULL);
    if (g_async_context) return 0;

    g_async_context = xtra;

    xtra->asyn_status = BINIO_ASYNC_INPROG;
    xtra->asyn_data = (char*)data;
    xtra->asyn_elesize = size;
    xtra->asyn_amt = n;
    xtra->asyn_ismot = 1;

    rc = iFileSeek(file, pos, 0);
    xVALIDATE_L(2015, rc==pos);

    xFileSetUserData(file, fli);
    rc = iFileReadAsync(file, data, size * n, BFD_cb_GCP2_readasync, 0);
    xVALIDATE_L(2021, rc>=0);
    if (rc < 0) result = 0;
    else {
        xtra->gcaskey = rc;
        result = 1;
    }

    if (!result) {
        g_async_context = NULL;
        xtra->asyn_status = BINIO_ASYNC_NOOP;
        xWARN("XBIN: GCAsync queue failed\n");
    }

    return (rdsec == 0) ? 1 : 0;
}

static en_BIO_ASYNC_ERRCODES BFD_AsyncReadStatus(st_FILELOADINFO* fli)
{
    en_BIO_ASYNC_ERRCODES status = BINIO_ASYNC_NOOP;
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)fli->xtradata;
    XFILE_READSECTOR_STATUS xrdstat = XFILE_RDSTAT_NOOP;

    xVALIDATE_L(2122, g_async_context);
    if (!g_async_context) return status;

    xVALIDATE_L(2125, xtra == g_async_context);
    if (xtra != g_async_context) {
        xWARN("BIO: Non-Owner client requesting async status\n");
        return BINIO_ASYNC_INPROG;
    }

    if (xtra->asyn_status == BINIO_ASYNC_INPROG) {
        S32 amtsofar = 0;
        xrdstat = xFileReadAsyncStatus(xtra->gcaskey, &amtsofar);

        switch (xrdstat) {
        case XFILE_RDSTAT_INPROG:
        case XFILE_RDSTAT_QUEUED:
            xtra->asyn_status = BINIO_ASYNC_INPROG;
            break;
        case XFILE_RDSTAT_DONE:
            xtra->asyn_status = BINIO_ASYNC_DONE;
            break;
        case XFILE_RDSTAT_FAIL:
        case XFILE_RDSTAT_EXPIRED:
        default:
            xtra->asyn_status = BINIO_ASYNC_FAIL;
            break;
        }
    }

    status = xtra->asyn_status;
    if (status == BINIO_ASYNC_FAIL || status == BINIO_ASYNC_DONE) {
        xtra->asyn_status = BINIO_ASYNC_NOOP;
        g_async_context = NULL;
    }

    return status;
}

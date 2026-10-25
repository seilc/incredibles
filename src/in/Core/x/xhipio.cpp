#include "xhipio.h"

#include <string.h>

static HIPLOADDATA* HIPLCreate(const char* filename, char* dblbuf, S32 bufsize, S32 fileflags);
static void HIPLDestroy(HIPLOADDATA* lddata);
static U32 HIPLBaseSector(HIPLOADDATA* lddata);
static S32 HIPLSetBypass(HIPLOADDATA* lddata, S32 enable, S32 use_async);
static void HIPLSetSpot(HIPLOADDATA* lddata, S32 spot);
static U32 HIPLBlockEnter(HIPLOADDATA* lddata);
static void HIPLBlockExit(HIPLOADDATA* lddata);
static S32 HIPLBlockRead(HIPLOADDATA* lddata, void* data, S32 cnt, S32 size);
static S32 HIPLBypassRead(HIPLOADDATA* lddata, void* data, S32 cnt, S32 size);
static S32 HIPLReadAsync(HIPLOADDATA* lddata, S32 pos, char* data, S32 cnt, S32 size);
static READ_ASYNC_STATUS HIPLPollRead(HIPLOADDATA* lddata);
static S32 HIPLReadBytes(HIPLOADDATA* lddata, char* data, S32 cnt);
static S32 HIPLReadShorts(HIPLOADDATA* lddata, S16* data, S32 cnt);
static S32 HIPLReadLongs(HIPLOADDATA* lddata, S32* data, S32 cnt);
static S32 HIPLReadFloats(HIPLOADDATA* lddata, F32* data, S32 cnt);
static S32 HIPLReadString(HIPLOADDATA* lddata, char* buf);

#define MAX_LOADINST 8

static U32 g_loadlock = 0;
static HIPLOADDATA g_hiploadinst[MAX_LOADINST] = {};

// clang-format off
static HIPLOADFUNCS g_map_HIPL_funcmap =
{
    HIPLCreate,
    HIPLDestroy,
    HIPLBaseSector,
    HIPLBlockEnter,
    HIPLBlockExit,
    HIPLReadBytes,
    HIPLReadShorts,
    HIPLReadLongs,
    HIPLReadFloats,
    HIPLReadString,
    HIPLSetBypass,
    HIPLSetSpot,
    HIPLPollRead
};
// clang-format on

HIPLOADFUNCS* get_HIPLFuncs()
{
    return &g_map_HIPL_funcmap;
}

static HIPLOADDATA* HIPLCreate(const char* filename, char* dblbuf, S32 bufsize, S32 fileflags)
{
    HIPLOADDATA* lddata = NULL;
    st_FILELOADINFO* fli;
    HIPLOADBLOCK* tmp_blk;
    S32 i;
    S32 uselock = -1;

    for (i = 0; i < MAX_LOADINST; i++)
    {
        if (!(g_loadlock & (1 << i)))
        {
            g_loadlock |= (1 << i);
            uselock = i;
            lddata = &g_hiploadinst[i];
            break;
        }
    }

    if (lddata)
    {
        memset(lddata, 0, sizeof(HIPLOADDATA));

        lddata->lockid = uselock;
        lddata->top = -1;
        lddata->base_sector = 0;
        lddata->use_async = FALSE;
        lddata->asyn_stat = HIP_RDSTAT_NONE;
        lddata->bypass = FALSE;
        lddata->bypass_recover = -1;
        lddata->pos = 0;
        lddata->readTop = 0;

        for (i = 0; i < MAX_OPENBLK; i++)
        {
            tmp_blk = &lddata->stk[i];
            tmp_blk->endpos = 0;
            tmp_blk->blk_id = 0;
            tmp_blk->blk_remain = 0;
            tmp_blk->flags = 0;
        }

        fli = xBinioLoadCreate(filename, fileflags);

        if (fli)
        {
            lddata->fli = fli;
            lddata->base_sector = fli->basesector;

            if (dblbuf && bufsize > 0)
            {
                fli->setDoubleBuf(fli, dblbuf, bufsize);
            }
        }
        else
        {
            HIPLDestroy(lddata);
            lddata = NULL;
        }
    }

    return lddata;
}

static void HIPLDestroy(HIPLOADDATA* lddata)
{
    S32 lockid;

    if (lddata)
    {
        if (lddata->fli)
        {
            lddata->fli->destroy(lddata->fli);
        }

        lockid = lddata->lockid;

        memset(lddata, 0, sizeof(HIPLOADDATA));

        g_loadlock &= ~(1 << lockid);
    }
}

static U32 HIPLBaseSector(HIPLOADDATA* lddata)
{
    return lddata->base_sector;
}

static S32 HIPLSetBypass(HIPLOADDATA* lddata, S32 enable, S32 use_async)
{
    lddata->fli->discardDblBuf(lddata->fli);

    if (enable && lddata->bypass)
    {
        return FALSE;
    }

    if (!enable && !lddata->bypass)
    {
        return FALSE;
    }

    if (enable)
    {
        lddata->bypass = TRUE;
        lddata->use_async = use_async;
        lddata->bypass_recover = lddata->fli->position;
    }
    else
    {
        lddata->fli->seekSpot(lddata->fli, lddata->bypass_recover);
        lddata->bypass = FALSE;
        lddata->use_async = FALSE;
        lddata->bypass_recover = -1;
    }

    return TRUE;
}

static void HIPLSetSpot(HIPLOADDATA* lddata, S32 spot)
{
    if (lddata->bypass)
    {
        lddata->pos = spot;
        lddata->fli->seekSpot(lddata->fli, spot);
    }
}

static U32 HIPLBlockEnter(HIPLOADDATA* lddata)
{
    HIPLOADBLOCK* top;
    U32 cid = 0;
    S32 size = 0;

    if (lddata->bypass)
    {
        return 0;
    }

    if (lddata->top >= 0 && lddata->stk[lddata->top].blk_remain <= 0)
    {
        return 0;
    }

    if (!HIPLReadLongs(lddata, (S32*)&cid, -1))
    {
        cid = 0;
    }
    else
    {
        HIPLReadLongs(lddata, &size, -1);

        if (lddata->top >= 0)
        {
            lddata->stk[lddata->top].blk_remain -= size;
        }

        top = &lddata->stk[++lddata->top];
        top->blk_id = cid;
        top->blk_remain = size;
        top->endpos = lddata->pos + top->blk_remain + (top->blk_remain & 1);
        top->flags = 0;
    }

    return cid;
}

static void HIPLBlockExit(HIPLOADDATA* lddata)
{
    HIPLOADBLOCK* top;

    if (!lddata->bypass)
    {
        top = &lddata->stk[lddata->top--];

        lddata->fli->skipBytes(lddata->fli, top->endpos - lddata->pos);
        lddata->pos = top->endpos;
    }
}

static S32 HIPLBlockRead(HIPLOADDATA* lddata, void* data, S32 cnt, S32 size)
{
    HIPLOADBLOCK* top;
    S32 got = 0;
    S32 left = 0;
    S32 head = FALSE;

    if (lddata->bypass)
    {
        return 0;
    }

    if (cnt == 0)
    {
        return 0;
    }

    if (lddata->top < 0)
    {
        top = NULL;
    }
    else
    {
        top = &lddata->stk[lddata->top];
        left = top->blk_remain / size;
    }

    if (cnt < 0)
    {
        cnt = -cnt;
        head = TRUE;

        if (top && cnt > left)
        {
            cnt = left;
        }
    }

    if (!head && left < cnt)
    {
        cnt = left;
    }

    if (cnt == 0)
    {
        got = 0;
    }
    else if (size == 1)
    {
        got = lddata->fli->readBytes(lddata->fli, (char*)data, cnt);
    }
    else if (size == 2)
    {
        got = lddata->fli->readMShorts(lddata->fli, (S16*)data, cnt);
    }
    else if (size == 4)
    {
        got = lddata->fli->readMLongs(lddata->fli, (S32*)data, cnt);
    }

    got *= size;

    lddata->pos += got;

    if (top)
    {
        top->blk_remain -= got;
    }

    return got;
}

static S32 HIPLBypassRead(HIPLOADDATA* lddata, void* data, S32 cnt, S32 size)
{
    S32 got = 0;

    if (!lddata->bypass)
    {
        return 0;
    }

    if (lddata->use_async)
    {
        return HIPLReadAsync(lddata, lddata->pos, (char*)data, cnt, size);
    }

    if (cnt == 0)
    {
        return 0;
    }

    if (cnt == 0)
    {
        got = 0;
    }
    else if (size == 1)
    {
        got = lddata->fli->readBytes(lddata->fli, (char*)data, cnt);
    }
    else if (size == 2)
    {
        got = lddata->fli->readMShorts(lddata->fli, (S16*)data, cnt);
    }
    else if (size == 4)
    {
        got = lddata->fli->readMLongs(lddata->fli, (S32*)data, cnt);
    }

    return got * size;
}

static S32 HIPLReadAsync(HIPLOADDATA* lddata, S32 pos, char* data, S32 cnt, S32 size)
{
    S32 got;

    lddata->asyn_stat = HIP_RDSTAT_NONE;

    got = lddata->fli->asyncMRead(lddata->fli, pos, data, cnt, size);

    lddata->asyn_stat = HIP_RDSTAT_INPROG;

    return got;
}

static READ_ASYNC_STATUS HIPLPollRead(HIPLOADDATA* lddata)
{
    READ_ASYNC_STATUS rdstat = HIP_RDSTAT_INPROG;
    en_BIO_ASYNC_ERRCODES pollstat;

    if (!lddata->bypass)
    {
        return HIP_RDSTAT_NOBYPASS;
    }

    if (!lddata->use_async)
    {
        return HIP_RDSTAT_NOASYNC;
    }

    pollstat = lddata->fli->asyncReadStatus(lddata->fli);

    switch (pollstat)
    {
    case BINIO_ASYNC_INPROG:
        rdstat = HIP_RDSTAT_INPROG;
        break;
    case BINIO_ASYNC_DONE:
        rdstat = HIP_RDSTAT_SUCCESS;
        lddata->asyn_stat = HIP_RDSTAT_NONE;
        break;
    case BINIO_ASYNC_FAIL:
        rdstat = HIP_RDSTAT_FAILED;
        lddata->asyn_stat = HIP_RDSTAT_NONE;
        break;
    default:
        lddata->asyn_stat = HIP_RDSTAT_NONE;
        break;
    }

    return rdstat;
}

static S32 HIPLReadBytes(HIPLOADDATA* lddata, char* data, S32 cnt)
{
    if (lddata->bypass)
    {
        return HIPLBypassRead(lddata, data, cnt, 1);
    }
    else
    {
        return HIPLBlockRead(lddata, data, cnt, 1);
    }
}

static S32 HIPLReadShorts(HIPLOADDATA* lddata, S16* data, S32 cnt)
{
    S32 got;

    if (lddata->bypass)
    {
        got = HIPLBypassRead(lddata, data, cnt, 2);
    }
    else
    {
        got = HIPLBlockRead(lddata, data, cnt, 2);
    }

    return got / 2;
}

static S32 HIPLReadLongs(HIPLOADDATA* lddata, S32* data, S32 cnt)
{
    S32 got;

    if (lddata->bypass)
    {
        got = HIPLBypassRead(lddata, data, cnt, 4);
    }
    else
    {
        got = HIPLBlockRead(lddata, data, cnt, 4);
    }

    return got / 4;
}

static S32 HIPLReadFloats(HIPLOADDATA* lddata, F32* data, S32 cnt)
{
    S32 got;

    if (lddata->bypass)
    {
        got = HIPLBypassRead(lddata, data, cnt, 4);
    }
    else
    {
        got = HIPLBlockRead(lddata, data, cnt, 4);
    }

    return got / 4;
}

static S32 HIPLReadString(HIPLOADDATA* lddata, char* buf)
{
    S32 n = 0;
    char pad = '\0';

    if (lddata->bypass)
    {
        while (HIPLBypassRead(lddata, &buf[n], 1, 1))
        {
            if (buf[n] == '\0')
            {
                if ((n & 1) == 0)
                {
                    HIPLBypassRead(lddata, &pad, 1, 1);
                }

                break;
            }

            n++;
        }
    }
    else
    {
        while (HIPLBlockRead(lddata, &buf[n], 1, 1))
        {
            if (buf[n] == '\0')
            {
                if ((n & 1) == 0)
                {
                    HIPLBlockRead(lddata, &pad, 1, 1);
                }

                break;
            }

            n++;
        }
    }

    return n;
}
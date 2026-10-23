#include "xserializer.h"

#include "xordarray.h"
#include "xMemMgr.h"
#include "xMath.h"

#include <string.h>

typedef struct st_XSERIAL_DATA_PRIV
{
    S32 flg_info;
    S32* bitbuf;
    S32 buf_bytcnt;
    SERIAL_CLIENTINFO* cltbuf;
    SERIAL_CLIENTINFO* cltnext;
    XORDEREDARRAY cltlist;
} XSERIAL_DATA_PRIV;

static S32 g_serinit = 0;
static XSERIAL_DATA_PRIV g_xserdata = {};
static S32 g_tbl_onbit[32] = {};
static S32 g_tbl_clear[32] = {};

static void xSER_init_tables();
static void xSER_init_buffers(S32 count, SERIAL_PERCID_SIZE* sizeinfo);
static S32 xSER_ord_compare(void* e1, void* e2);
static S32 xSER_ord_test(const void* key, void* elt);
static SERIAL_CLIENTINFO* XSER_get_client(U32 idtag);
static bool32 xSER_xsgclt_svinfo_ver(void*, XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame);
static bool32 xSER_xsgclt_svproc_ver(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_WRITECONTEXT* wctxt);
static bool32 xSER_xsgclt_ldproc_ver(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_READCONTEXT* rctxt,
                                     U32, S32);
static bool32 xSER_xsgclt_svinfo_clt(void* cltdata, XSAVEGAME_DATA*, S32* cur_space,
                                     S32* max_fullgame);
static bool32 xSER_xsgclt_svproc_clt(void* cltdata, XSAVEGAME_DATA* xsg,
                                     XSAVEGAME_WRITECONTEXT* wctxt);
static bool32 xSER_xsgclt_ldproc_clt(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_READCONTEXT* rctxt,
                                     U32 idtag, S32);
static bool32 xSER_xsgclt_svinfo_fill(void*, XSAVEGAME_DATA*, S32* cur_space,
                                      S32* max_fullgame);
static bool32 xSER_xsgclt_svproc_fill(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_WRITECONTEXT* wctxt);
static bool32 xSER_xsgclt_ldproc_fill(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_READCONTEXT* rctxt,
                                      U32, S32);

S32 xSerialStartup(S32 count, SERIAL_PERCID_SIZE* sizeinfo)
{
    if (!g_serinit++)
    {
        memset(&g_xserdata, 0, sizeof(g_xserdata));

        xSER_init_tables();
        xSER_init_buffers(count, sizeinfo);
    }

    return g_serinit;
}

S32 xSerialShutdown()
{
    return --g_serinit;
}

void xSerialTraverse(xSerialTraverseCallback func)
{
    S32 i;
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    SERIAL_CLIENTINFO* clt;
    xSerial xser;
    bool32 rc;

    for (i = 0; i < xsd->cltlist.cnt; i++)
    {
        clt = (SERIAL_CLIENTINFO*)xsd->cltlist.list[i];

        xser.setClient(clt->idtag);

        rc = func(clt->idtag, &xser);

        if (!rc)
        {
            break;
        }
    }
}

xSerial::~xSerial()
{
}

void xSerial::setClient(U32 idtag)
{
    prepare(idtag);
}

S32 xSerial::Write(char* data, S32 elesize, S32 n)
{
    if (n == 0)
    {
        return 0;
    }

    S32 nbit;

    if (n > 0)
    {
        nbit = n * elesize * 8;
    }
    else
    {
        nbit = -n;
    }

    if (n < 0)
    {
        S32 bidx = 0;
        S32* iptr = (S32*)data;

        for (S32 i = 0; i < nbit; i++)
        {
            wrbit(*iptr & g_tbl_onbit[bidx]);

            bidx++;

            if (bidx == 32)
            {
                bidx = 0;
                iptr++;
            }
        }
    }
    else
    {
        S32 bidx = 0;
        S8* cptr = (S8*)data;

        for (S32 i = 0; i < nbit; i++)
        {
            wrbit(*cptr & g_tbl_onbit[bidx]);

            bidx++;

            if (bidx == 8)
            {
                bidx = 0;
                cptr++;
            }
        }
    }

    return nbit;
}

S32 xSerial::Write_b1(S32 bits)
{
    return Write((char*)&bits, sizeof(S32), -1);
}

S32 xSerial::Write_b1(U32 bits)
{
    return Write((char*)&bits, sizeof(U32), -1);
}

S32 xSerial::Write(U8 data)
{
    return Write((char*)&data, sizeof(U8), 1);
}

S32 xSerial::Write(S16 data)
{
    return Write((char*)&data, sizeof(S16), 1);
}

S32 xSerial::Write(S32 data)
{
    return Write((char*)&data, sizeof(S32), 1);
}

S32 xSerial::Write(U32 data)
{
    return Write((char*)&data, sizeof(U32), 1);
}

S32 xSerial::Write(F32 data)
{
    return Write((char*)&data, sizeof(F32), 1);
}

S32 xSerial::Read(char* buf, S32 elesize, S32 n)
{
    S32 nbit;
    S32* iptr;
    S32 bidx;
    S32 i;

    if (n > 0)
    {
        nbit = n * elesize * 8;
    }
    else
    {
        nbit = -n;
    }

    if (n < 0)
    {
        bidx = 0;
        iptr = (S32*)buf;

        for (i = 0; i < nbit; i++)
        {
            bool32 bitval = rdbit();

            if (bitval)
            {
                *iptr |= g_tbl_onbit[bidx];
            }
            else
            {
                *iptr &= g_tbl_clear[bidx];
            }

            bidx++;

            if (bidx == 32)
            {
                bidx = 0;
                iptr++;
            }
        }
    }
    else
    {
        bidx = 0;
        char* cptr = (char*)buf;

        for (i = 0; i < nbit; i++)
        {
            bool32 bitval = rdbit();

            if (bitval)
            {
                *cptr |= (S8)g_tbl_onbit[bidx];
            }
            else
            {
                *cptr &= (S8)g_tbl_clear[bidx];
            }

            bidx++;

            if (bidx == 8)
            {
                bidx = 0;
                cptr++;
            }
        }
    }

    return nbit;
}

S32 xSerial::Read_b1(S32* bits)
{
    return Read((char*)bits, sizeof(S32), -1);
}

S32 xSerial::Read(U8* buf)
{
    return Read((char*)buf, sizeof(U8), 1);
}

S32 xSerial::Read(S16* buf)
{
    return Read((char*)buf, sizeof(S16), 1);
}

S32 xSerial::Read(S32* buf)
{
    return Read((char*)buf, sizeof(S32), 1);
}

S32 xSerial::Read(U32* buf)
{
    return Read((char*)buf, sizeof(U32), 1);
}

S32 xSerial::Read(F32* buf)
{
    return Read((char*)buf, sizeof(F32), 1);
}

void xSerial::wrbit(bool32 is_on)
{
    SERIAL_CLIENTINFO* clt = ctxtdata;

    if (bittally + 1 > clt->actsize * 8)
    {
        warned = TRUE;
        return;
    }

    clt->membuf[curele] &= g_tbl_clear[bitidx];

    if (is_on)
    {
        clt->membuf[curele] |= g_tbl_onbit[bitidx];
    }

    bitidx++;

    if (bitidx == 32)
    {
        curele++;
        bitidx = 0;
    }

    bittally++;
}

bool32 xSerial::rdbit()
{
    SERIAL_CLIENTINFO* clt = ctxtdata;

    if (bittally + 1 > clt->actsize * 8)
    {
        warned = TRUE;
        return FALSE;
    }

    bool32 bit = ((clt->membuf[curele] & g_tbl_onbit[bitidx]) != 0) ? TRUE : FALSE;

    bitidx++;

    if (bitidx == 32)
    {
        curele++;
        bitidx = 0;
    }

    bittally++;

    return bit;
}

void xSerial::prepare(U32 clientID)
{
    SERIAL_CLIENTINFO* clt = XSER_get_client(clientID);

    idtag = clt->idtag;
    baseoff = clt->trueoff;
    ctxtdata = clt;
    warned = FALSE;
    curele = 0;
    bitidx = 0;
    bittally = 0;
}

void xSerialWipeMainBuffer()
{
    memset(g_xserdata.bitbuf, 0, g_xserdata.buf_bytcnt);
}

static void xSER_init_tables()
{
    for (S32 i = 0; i < 32; i++)
    {
        g_tbl_onbit[i] = 1 << i;
        g_tbl_clear[i] = ~(1 << i);
    }
}

static void xSER_init_buffers(S32 count, SERIAL_PERCID_SIZE* sizeinfo)
{
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    S32 i;
    S32 tally = 0;
    S32 sicnt = 0;
    SERIAL_PERCID_SIZE* sitmp;
    SERIAL_CLIENTINFO* tmp_clt;

    XOrdInit(&g_xserdata.cltlist, count, FALSE);

    g_xserdata.cltbuf = (SERIAL_CLIENTINFO*)xMEMALLOC(count * sizeof(SERIAL_CLIENTINFO));

    // This should be:
    //     memset(g_xserdata.cltbuf, 0, count * sizeof(SERIAL_CLIENTINFO))
    // but it doesn't match
    memset(g_xserdata.cltbuf, 0, count * 16);

    g_xserdata.cltnext = g_xserdata.cltbuf;

    sitmp = sizeinfo;

    while (sitmp->idtag != 0)
    {
        tally += ALIGN(sitmp->needsize, 4);
        sicnt++;
        sitmp++;
    }

    tally += (count - sicnt) * 400;

    xsd->bitbuf = (S32*)xMEMALLOC(tally);
    memset(xsd->bitbuf, 0, tally);

    xsd->buf_bytcnt = tally;

    sitmp = sizeinfo;
    tmp_clt = xsd->cltnext;
    tally = 0;

    while (sitmp->idtag != 0)
    {
        tmp_clt->idtag = sitmp->idtag;
        tmp_clt->trueoff = tally;
        tmp_clt->actsize = ALIGN(sitmp->needsize, 4);
        tmp_clt->membuf = xsd->bitbuf + tally / 4;

        XOrdAppend(&xsd->cltlist, tmp_clt);

        tally += tmp_clt->actsize;
        sitmp++;
        tmp_clt++;
    }

    XOrdSort(&xsd->cltlist, xSER_ord_compare);

    xsd->cltnext = tmp_clt;

    for (i = sicnt; i < count; i++)
    {
        tmp_clt->idtag = 0;
        tmp_clt->trueoff = tally;
        tmp_clt->actsize = 400;
        tmp_clt->membuf = xsd->bitbuf + tally / 4;

        tally += tmp_clt->actsize;
        tmp_clt++;
    }
}

static S32 xSER_ord_compare(void* e1, void* e2)
{
    S32 rc;

    if (((SERIAL_CLIENTINFO*)e1)->idtag < ((SERIAL_CLIENTINFO*)e2)->idtag)
    {
        rc = -1;
    }
    else if (((SERIAL_CLIENTINFO*)e1)->idtag > ((SERIAL_CLIENTINFO*)e2)->idtag)
    {
        rc = 1;
    }
    else
    {
        rc = 0;
    }

    return rc;
}

static S32 xSER_ord_test(const void* key, void* elt)
{
    S32 rc;
    U32 idtag = (U32)key;

    if (idtag < ((SERIAL_CLIENTINFO*)elt)->idtag)
    {
        rc = -1;
    }
    else if (idtag > ((SERIAL_CLIENTINFO*)elt)->idtag)
    {
        rc = 1;
    }
    else
    {
        rc = 0;
    }

    return rc;
}

static SERIAL_CLIENTINFO* XSER_get_client(U32 idtag)
{
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    SERIAL_CLIENTINFO* clt;
    S32 idx = XOrdLookup(&xsd->cltlist, (const void*)idtag, xSER_ord_test);

    if (idx < 0)
    {
        clt = xsd->cltnext;

        xsd->cltnext++;

        clt->idtag = idtag;

        XOrdInsert(&xsd->cltlist, clt, xSER_ord_compare);
    }
    else
    {
        clt = (SERIAL_CLIENTINFO*)xsd->cltlist.list[idx];
    }

    return clt;
}

bool32 xSerial_svgame_register(XSAVEGAME_DATA* sgctxt, SAVEGAME_MODE mode)
{
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    SERIAL_CLIENTINFO* clt;
    S32 i;

    xsd->flg_info &= ~0x1;

    if (mode == XSG_MODE_SAVE)
    {
        xSGAddSaveClient(sgctxt, 'SVID', &g_xserdata, xSER_xsgclt_svinfo_ver,
                         xSER_xsgclt_svproc_ver);

        for (i = 0; i < xsd->cltlist.cnt; i++)
        {
            clt = (SERIAL_CLIENTINFO*)xsd->cltlist.list[i];

            xSGAddSaveClient(sgctxt, clt->idtag, clt, xSER_xsgclt_svinfo_clt,
                             xSER_xsgclt_svproc_clt);
        }

        xSGAddSaveClient(sgctxt, 'SFIL', &g_xserdata, xSER_xsgclt_svinfo_fill,
                         xSER_xsgclt_svproc_fill);
    }
    else if (mode == XSG_MODE_LOAD)
    {
        xSGAddLoadClient(sgctxt, 'SVID', &g_xserdata, xSER_xsgclt_ldproc_ver);
        xSGAddLoadClient(sgctxt, 0, &g_xserdata, xSER_xsgclt_ldproc_clt);
        xSGAddLoadClient(sgctxt, 'SFIL', &g_xserdata, xSER_xsgclt_ldproc_fill);
    }

    return TRUE;
}

static bool32 xSER_xsgclt_svinfo_ver(void*, XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame)
{
    *cur_space = 4;
    *max_fullgame = 4;

    return TRUE;
}

static bool32 xSER_xsgclt_svproc_ver(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_WRITECONTEXT* wctxt)
{
    S32 ver = 1;

    xSGWriteData(xsg, wctxt, &ver, 1);

    return TRUE;
}

static bool32 xSER_xsgclt_ldproc_ver(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_READCONTEXT* rctxt,
                                     U32, S32)
{
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    S32 ver = 0;

    xSGReadData(xsg, rctxt, &ver, 1);

    if (ver != 1)
    {
        xsd->flg_info |= 0x1;
    }

    return TRUE;
}

static bool32 xSER_xsgclt_svinfo_clt(void* cltdata, XSAVEGAME_DATA*, S32* cur_space,
                                     S32* max_fullgame)
{
    SERIAL_CLIENTINFO* clt = (SERIAL_CLIENTINFO*)cltdata;

    *cur_space = clt->actsize;
    *max_fullgame = clt->actsize;

    return TRUE;
}

static bool32 xSER_xsgclt_svproc_clt(void* cltdata, XSAVEGAME_DATA* xsg,
                                     XSAVEGAME_WRITECONTEXT* wctxt)
{
    SERIAL_CLIENTINFO* clt = (SERIAL_CLIENTINFO*)cltdata;

    clt = XSER_get_client(clt->idtag);

    xSGWriteData(xsg, wctxt, (char*)clt->membuf, clt->actsize);

    return TRUE;
}

static bool32 xSER_xsgclt_ldproc_clt(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_READCONTEXT* rctxt,
                                     U32 idtag, S32)
{
    SERIAL_CLIENTINFO* clt;

    if (g_xserdata.flg_info & 0x1)
    {
        return TRUE;
    }

    clt = XSER_get_client(idtag);

    xSGReadData(xsg, rctxt, (char*)clt->membuf, clt->actsize);

    return TRUE;
}

static bool32 xSER_xsgclt_svinfo_fill(void*, XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame)
{
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    S32 i;
    S32 tally = 0;
    S32 size;

    for (i = 0; i < xsd->cltlist.cnt; i++)
    {
        tally += ((SERIAL_CLIENTINFO*)xsd->cltlist.list[i])->actsize;
    }

    size = xsd->buf_bytcnt;
    size -= tally;
    size += 8;

    *cur_space = size;
    *max_fullgame = size;

    return TRUE;
}

static bool32 xSER_xsgclt_svproc_fill(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_WRITECONTEXT* wctxt)
{
    char filbuf[9] = "RyanNeil";

    xSGWriteData(xsg, wctxt, filbuf, 8);

    return TRUE;
}

static bool32 xSER_xsgclt_ldproc_fill(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_READCONTEXT* rctxt,
                                      U32, S32)
{
    char filbuf[9] = {};

    if (g_xserdata.flg_info & 0x1)
    {
        return TRUE;
    }

    xSGReadData(xsg, rctxt, filbuf, 8);

    return TRUE;
}
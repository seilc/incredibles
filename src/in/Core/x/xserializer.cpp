#include "xserializer.h"

#include "xordarray.h"
#include "xDebug.h"
#include "xMemMgr.h"
#include "xutil.h"

#include <string.h>

#define XSER_BITOP_DEPTH 32
#define XSER_BITOP_ELESIZE 4

typedef struct st_XSERIAL_DATA_PRIV
{
    S32 flg_info;
    S32* bitbuf;
    S32 buf_bytcnt;
    SERIAL_CLIENTINFO* cltbuf;
    SERIAL_CLIENTINFO* cltnext;
    st_XORDEREDARRAY cltlist;
} XSERIAL_DATA_PRIV;

static S32 g_serinit;
static XSERIAL_DATA_PRIV g_xserdata = { 0 };
static S32 g_tbl_onbit[XSER_BITOP_DEPTH] = { 0 };
static S32 g_tbl_clear[XSER_BITOP_DEPTH] = { 0 };

static void xSER_init_tables();
static void xSER_init_buffers(S32 count, SERIAL_PERCID_SIZE* sizeinfo);
static S32 xSER_ord_compare(void* e1, void* e2);
static S32 xSER_ord_test(const void* key, void* elt);
static SERIAL_CLIENTINFO* XSER_get_client(U32 idtag);
static S32 xSER_xsgclt_svinfo_ver(void*, XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame);
static S32 xSER_xsgclt_svproc_ver(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_WRITECONTEXT* wctxt);
static S32 xSER_xsgclt_ldproc_ver(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_READCONTEXT* rctxt, U32, S32);
static S32 xSER_xsgclt_svinfo_clt(void* cltdata, XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame);
static S32 xSER_xsgclt_svproc_clt(void* cltdata, XSAVEGAME_DATA* xsg, XSAVEGAME_WRITECONTEXT* wctxt);
static S32 xSER_xsgclt_ldproc_clt(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_READCONTEXT* rctxt, U32 idtag, S32 maxsize);
static S32 xSER_xsgclt_svinfo_fill(void*, XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame);
static S32 xSER_xsgclt_svproc_fill(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_WRITECONTEXT* wctxt);
static S32 xSER_xsgclt_ldproc_fill(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_READCONTEXT* rctxt, U32, S32);

S32 xSerialStartup(S32 count, SERIAL_PERCID_SIZE* sizeinfo)
{
    if (g_serinit++ == 0) {
        memset(&g_xserdata, 0, sizeof(g_xserdata));

        xSER_init_tables();
        xSER_init_buffers(count, sizeinfo);

        xVERBOSE("XSER: Startup ... %d sandboxes\n", count);
    }

    return g_serinit;
}

S32 xSerialShutdown()
{
    if (--g_serinit == 0)
        xVERBOSE("XSER: Shutdown\n");

    xVALIDATE_L(85, g_serinit>=0);
    return g_serinit;
}

void xSerialTraverse(xSerialTraverseCallback func)
{
    S32 i = 0;
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    SERIAL_CLIENTINFO* clt = NULL;
    xSerial xser;
    S32 rc = 0;

    xVALIDATE_L(98, func);

    for (i = 0; i < xsd->cltlist.cnt; i++) {
        clt = (SERIAL_CLIENTINFO*)xsd->cltlist.list[i];
        xVALIDATE_L(103, clt);

        xser.setClient(clt->idtag);

        rc = func(clt->idtag, &xser);
        if (!rc)
            break;
    }
}

xSerial::~xSerial()
{
    F32 pct = 0.0f;

    xVALIDATE_L(130, this->ctxtdata);
    if (this->ctxtdata) {
        pct = (F32)this->bittally / (this->ctxtdata->actsize * 8);
        pct *= 100.0f;
    }
}

void xSerial::setClient(U32 idtag)
{
    prepare(idtag);
}

S32 xSerial::Write(char* data, S32 elesize, S32 n)
{
    xVALIDATE_L(167, n!=0);
    if (n == 0)
        return 0;

    S32 nbit, numbytes;
    if (n > 0) {
        numbytes = n * elesize;
        nbit = numbytes * 8;
    } else {
        nbit = -n;
        numbytes = (nbit + 7) / 8;
        numbytes = (numbytes + 3) & ~3;
        xVALIDATE_L(194, elesize==4);
        xVALIDATE_MSG_L(209, numbytes <= 4, "XSER: Supports >4bytes w/ _bn; just unexpected");
    }
    xVALIDATE_L(212, nbit>0);

    if (n < 0) {
        S32 bidx = 0;
        S32* iptr = (S32*)data;
        for (S32 i = 0; i < nbit; i++) {
            S32 bitval = *iptr & g_tbl_onbit[bidx];

            wrbit(bitval);

            bidx++;
            if (bidx == XSER_BITOP_DEPTH) {
                bidx = 0;
                iptr++;
                numbytes -= 4;
            }
            xVALIDATE_L(250, numbytes>=0);
        }
    } else {
        xVALIDATE_L(254, (elesize==1)||(elesize==2)||(elesize==4));
        S32 bidx = 0;
        char* cptr = data;
        for (S32 i = 0; i < nbit; i++) {
            S32 bitval = *cptr & g_tbl_onbit[bidx];

            wrbit(bitval);

            bidx++;
            if (bidx == 8) {
                bidx = 0;
                cptr++;
                numbytes--;
            }
            xVALIDATE_L(276, numbytes>=0);
        }
    }

    return nbit;
}

// RESTORED: from TSSM GC
S32 xSerial::Write_bn(U32 bits, S32 n)
{
    return Write((char*)&bits, 4, -n);
}
// END RESTORED

S32 xSerial::Write_b1(S32 bits)
{
    return Write((char*)&bits, 4, -1);
}

S32 xSerial::Write_b1(U32 bits)
{
    return Write((char*)&bits, 4, -1);
}

// RESTORED: from BFBB GC
S32 xSerial::Write_b7(U32 bits)
{
    return Write((char*)&bits, 4, -7);
}
// END RESTORED

S32 xSerial::Write(U8 data)
{
    return Write((char*)&data, 1, 1);
}

S32 xSerial::Write(S16 data)
{
    return Write((char*)&data, 2, 1);
}

S32 xSerial::Write(S32 data)
{
    return Write((char*)&data, 4, 1);
}

S32 xSerial::Write(U32 data)
{
    return Write((char*)&data, 4, 1);
}

S32 xSerial::Write(F32 data)
{
    return Write((char*)&data, 4, 1);
}

S32 xSerial::Read(char* buf, S32 elesize, S32 n)
{
    S32 nbit = 0;
    S32 numbytes = 0;
    S32* iptr = NULL;
    S32 bidx = 0;
    S32 i = 0;

    if (n > 0) {
        numbytes = n * elesize;
        nbit = numbytes * 8;
    } else {
        nbit = -n;
        numbytes = (nbit + 7) / 8;
        numbytes = (numbytes + 3) & ~3;
        xVALIDATE_MSG_L(508, numbytes <= 4, "Properly supports >4byte read from _bn; just unexpected");
    }
    xVALIDATE_L(515, nbit>0);

    if (n < 0) {
        bidx = 0;
        iptr = (S32*)buf;
        for (i = 0; i < nbit; i++) {
            S32 bitval = rdbit();

            if (bitval)
                *iptr |= g_tbl_onbit[bidx];
            else
                *iptr &= g_tbl_clear[bidx];

            bidx++;
            if (bidx == XSER_BITOP_DEPTH) {
                bidx = 0;
                iptr++;
                numbytes -= 4;
            }
            xVALIDATE_L(542, numbytes>=0);
        }
    } else {
        bidx = 0;
        char* cptr = buf;
        for (i = 0; i < nbit; i++) {
            S32 bitval = rdbit();

            if (bitval)
                *cptr |= (char)g_tbl_onbit[bidx];
            else
                *cptr &= (char)g_tbl_clear[bidx];

            bidx++;
            if (bidx == 8) {
                bidx = 0;
                cptr++;
                numbytes--;
            }
            xVALIDATE_L(568, numbytes>=0);
        }
    }

    return nbit;
}

// RESTORED: from TSSM GC
S32 xSerial::Read_bn(U32* bits, S32 n)
{
    return Read((char*)bits, 4, -n);
}
// END RESTORED

S32 xSerial::Read_b1(S32* bits)
{
    return Read((char*)bits, 4, -1);
}

// RESTORED: from TSSM GC, Rat GC
S32 xSerial::Read_b1(U32* bits)
{
    return Read((char*)bits, 4, -1);
}
// END RESTORED

// RESTORED: from BFBB GC
S32 xSerial::Read_b7(U32* bits)
{
    return Read((char*)bits, 4, -7);
}
// END RESTORED

S32 xSerial::Read(U8* buf)
{
    return Read((char*)buf, 1, 1);
}

S32 xSerial::Read(S16* buf)
{
    return Read((char*)buf, 2, 1);
}

S32 xSerial::Read(S32* buf)
{
    return Read((char*)buf, 4, 1);
}

S32 xSerial::Read(U32* buf)
{
    return Read((char*)buf, 4, 1);
}

S32 xSerial::Read(F32* buf)
{
    return Read((char*)buf, 4, 1);
}

void xSerial::wrbit(S32 is_on)
{
    SERIAL_CLIENTINFO* clt = this->ctxtdata;

    xVALIDATE_L(767, clt->idtag == this->idtag);

    if (this->bittally + 1 > clt->actsize * 8) {
        xASSERT_FMT_L(785, this->warned, "XSER: Client %s playing in someone else's sandbox\nPersistent data will be lost.", xUtil_idtag2string(this->idtag, 0));
        this->warned = TRUE;
        return;
    }

    clt->membuf[this->curele] &= g_tbl_clear[this->bitidx];
    if (is_on)
        clt->membuf[this->curele] |= g_tbl_onbit[this->bitidx];

    this->bitidx++;
    if (this->bitidx == XSER_BITOP_DEPTH) {
        this->curele++;
        this->bitidx = 0;
    }
    xVALIDATE_L(811, this->bitidx < XSER_BITOP_DEPTH);

    this->bittally++;
}

S32 xSerial::rdbit()
{
    SERIAL_CLIENTINFO* clt = this->ctxtdata;
    S32 is_on = FALSE;

    xVALIDATE_L(828, clt->idtag == this->idtag);

    if (this->bittally + 1 > clt->actsize * 8) {
        if (!this->warned)
            xWARN("XSER: Client %s playing in someone else's sandbox\n", xUtil_idtag2string(this->idtag, 0));
        this->warned = TRUE;
        return is_on;
    }

    is_on = (clt->membuf[this->curele] & g_tbl_onbit[this->bitidx]) ? TRUE : FALSE;

    this->bitidx++;
    if (this->bitidx == XSER_BITOP_DEPTH) {
        this->curele++;
        this->bitidx = 0;
    }
    xVALIDATE_L(867, this->bitidx < XSER_BITOP_DEPTH);

    this->bittally++;

    return is_on;
}

void xSerial::prepare(U32 idtag)
{
    SERIAL_CLIENTINFO* clt = NULL;

    clt = XSER_get_client(idtag);

    this->idtag = clt->idtag;
    this->baseoff = clt->trueoff;
    this->ctxtdata = clt;

    this->warned = FALSE;
    this->curele = 0;
    this->bitidx = 0;
    this->bittally = 0;
}

void xSerialWipeMainBuffer()
{
    XSERIAL_DATA_PRIV* xser = &g_xserdata;

    xVALIDATE_MSG_L(912, xser->bitbuf && xser->buf_bytcnt, "XSER: No buffer?!? Was Startup called?");

    memset(xser->bitbuf, 0, xser->buf_bytcnt);
}

static void xSER_init_tables()
{
    S32 i = 0;

    for (i = 0; i < XSER_BITOP_DEPTH; i++) {
        g_tbl_onbit[i] = 1 << i;
        g_tbl_clear[i] = ~(1 << i);
    }
}

static void xSER_init_buffers(S32 count, SERIAL_PERCID_SIZE* sizeinfo)
{
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    S32 i = 0;
    S32 tally = 0;

    S32 sicnt = 0;
    SERIAL_PERCID_SIZE* sitmp = NULL;
    SERIAL_CLIENTINFO* tmp_clt = NULL;
    size_t s;

    xVALIDATE_L(958, count>0);

    xVALIDATE_L(961, (s=sizeof(S32)) == XSER_BITOP_ELESIZE);

    XOrdInit(&g_xserdata.cltlist, count, 0);
    xsd->cltbuf = (SERIAL_CLIENTINFO*)xMEMALLOC((U32)count * sizeof(SERIAL_CLIENTINFO));
    memset(xsd->cltbuf, 0, count * sizeof(SERIAL_CLIENTINFO));
    xsd->cltnext = xsd->cltbuf;

    sitmp = sizeinfo;
    while (sitmp->idtag) {
        tally += (sitmp->needsize + 3) & ~3;
        sicnt++;
        sitmp++;
    }

    xVALIDATE_L(984, (tally%4)==0);

    tally += (count - sicnt) * 400;

    xVERBOSE("XSER: Sandbox total: %d\n", tally);

    xsd->bitbuf = (S32*)xMEMALLOC(tally);
    memset(xsd->bitbuf, 0, tally);

    xsd->buf_bytcnt = tally;

    tmp_clt = xsd->cltnext;
    sitmp = sizeinfo;
    tally = 0;
    while (sitmp->idtag) {
        xVERBOSE("XSER: Registering custom client '%s'\n", xUtil_idtag2string(sitmp->idtag, 0));

        tmp_clt->idtag = sitmp->idtag;
        tmp_clt->trueoff = tally;
        tmp_clt->actsize = (sitmp->needsize + 3) & ~3;

        tmp_clt->membuf = &xsd->bitbuf[tally / 4];

        XOrdAppend(&xsd->cltlist, tmp_clt);

        tally += tmp_clt->actsize;

        sitmp++;
        tmp_clt++;
    }

    XOrdSort(&xsd->cltlist, xSER_ord_compare);

    xsd->cltnext = tmp_clt;

    for (i = sicnt; i < count; i++) {
        tmp_clt->idtag = 0;
        tmp_clt->trueoff = tally;
        tmp_clt->actsize = 400;

        tmp_clt->membuf = &xsd->bitbuf[tally / 4];

        tally += tmp_clt->actsize;
        tmp_clt++;
    }

    xVALIDATE_L(1060, tally <= xsd->buf_bytcnt);
    xVALIDATE_L(1062, tally == xsd->buf_bytcnt);
}

static S32 xSER_ord_compare(void* e1, void* e2)
{
    S32 rc = 0;

    SERIAL_CLIENTINFO* clt_a = (SERIAL_CLIENTINFO*)e1;
    SERIAL_CLIENTINFO* clt_b = (SERIAL_CLIENTINFO*)e2;

    if (clt_a->idtag < clt_b->idtag)
        rc = -1;
    else if (clt_a->idtag > clt_b->idtag)
        rc = 1;
    else
        rc = 0;

    return rc;
}

static S32 xSER_ord_test(const void* key, void* elt)
{
    S32 rc = 0;

    U32 idtag = (U32)key;
    SERIAL_CLIENTINFO* clt = (SERIAL_CLIENTINFO*)elt;

    if (idtag < clt->idtag)
        rc = -1;
    else if (idtag > clt->idtag)
        rc = 1;
    else
        rc = 0;

    return rc;
}

static SERIAL_CLIENTINFO* XSER_get_client(U32 idtag)
{
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    SERIAL_CLIENTINFO* clt = NULL;
    S32 idx = -1;

    idx = XOrdLookup(&xsd->cltlist, (void*)idtag, xSER_ord_test);
    if (idx < 0) {
        xVERBOSE("XSER: Adding client '%s'\n", xUtil_idtag2string(idtag, 0));

        clt = xsd->cltnext; xsd->cltnext++;

        xVALIDATE_L(1120, clt->idtag==0);
        clt->idtag = idtag;

        xVALIDATE_L(1124, clt->actsize>0);
        xVALIDATE_L(1125, clt->membuf);
        XOrdInsert(&xsd->cltlist, clt, xSER_ord_compare);
    } else {
        xVERBOSE("XSER: Found existing client '%s'\n", xUtil_idtag2string(idtag, 0));
        clt = (SERIAL_CLIENTINFO*)xsd->cltlist.list[idx];
        xVALIDATE_L(1132, clt->membuf);
    }

    xVALIDATE_L(1135, clt);
    xVALIDATE_L(1136, clt->idtag==idtag);

    xVALIDATE_L(1144, xsd->cltnext < (xsd->cltbuf + (xsd->cltlist.max)));

    return clt;
}

S32 xSerial_svgame_register(XSAVEGAME_DATA* sgctxt, SAVEGAME_MODE mode)
{
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    SERIAL_CLIENTINFO* clt = NULL;
    S32 i = 0;

    xsd->flg_info &= ~1;

    if (mode == XSG_MODE_SAVE) {
        xSGAddSaveClient(sgctxt, 'SVID', &g_xserdata, xSER_xsgclt_svinfo_ver, xSER_xsgclt_svproc_ver);

        for (i = 0; i < xsd->cltlist.cnt; i++) {
            clt = (SERIAL_CLIENTINFO*)xsd->cltlist.list[i];
            xSGAddSaveClient(sgctxt, clt->idtag, clt, xSER_xsgclt_svinfo_clt, xSER_xsgclt_svproc_clt);
        }

        xSGAddSaveClient(sgctxt, 'SFIL', &g_xserdata, xSER_xsgclt_svinfo_fill, xSER_xsgclt_svproc_fill);
    }
    else if (mode == XSG_MODE_LOAD) {
        xSGAddLoadClient(sgctxt, 'SVID', &g_xserdata, xSER_xsgclt_ldproc_ver);
        xSGAddLoadClient(sgctxt, 0, &g_xserdata, xSER_xsgclt_ldproc_clt);
        xSGAddLoadClient(sgctxt, 'SFIL', &g_xserdata, xSER_xsgclt_ldproc_fill);
    }
    else
        xWARN("XSER: Unknown savegame mode %d\n", mode);

    return TRUE;
}

static S32 xSER_xsgclt_svinfo_ver(void*, XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame)
{
    *cur_space = 4;
    *max_fullgame = 4;

    return TRUE;
}

static S32 xSER_xsgclt_svproc_ver(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_WRITECONTEXT* wctxt)
{
    S32 ver = 1;

    xSGWriteData(xsg, wctxt, &ver, 1);

    return TRUE;
}

static S32 xSER_xsgclt_ldproc_ver(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_READCONTEXT* rctxt, U32, S32)
{
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    S32 ver = 0;

    xSGReadData(xsg, rctxt, &ver, 1);

    if (ver != 1)
        xsd->flg_info |= 1;

    return TRUE;
}

static S32 xSER_xsgclt_svinfo_clt(void* cltdata, XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame)
{
    SERIAL_CLIENTINFO* clt = (SERIAL_CLIENTINFO*)cltdata;
    xVALIDATE_L(1253, clt);

    *cur_space = clt->actsize;
    *max_fullgame = clt->actsize;

    return TRUE;
}

static S32 xSER_xsgclt_svproc_clt(void* cltdata, XSAVEGAME_DATA* xsg, XSAVEGAME_WRITECONTEXT* wctxt)
{
    SERIAL_CLIENTINFO* clt = (SERIAL_CLIENTINFO*)cltdata;
    xVALIDATE_L(1267, clt);

    clt = XSER_get_client(clt->idtag);

    xSGWriteData(xsg, wctxt, (char*)clt->membuf, clt->actsize);

    return TRUE;
}

static S32 xSER_xsgclt_ldproc_clt(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_READCONTEXT* rctxt, U32 idtag, S32 maxsize)
{
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    SERIAL_CLIENTINFO* clt = NULL;
    xVALIDATE_L(1289, idtag);

    if (xsd->flg_info & 1)
        return TRUE;

    clt = XSER_get_client(idtag);

    xVALIDATE_L(1302, clt->actsize <= maxsize);
    xSGReadData(xsg, rctxt, (char*)clt->membuf, clt->actsize);

    return TRUE;
}

static S32 xSER_xsgclt_svinfo_fill(void*, XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame)
{
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;

    SERIAL_CLIENTINFO* clt = NULL;
    S32 i = 0;
    S32 tally = 0;
    S32 size = 0;

    xVALIDATE_L(1320, xsd);

    tally = 0;
    for (i = 0; i < xsd->cltlist.cnt; i++) {
        clt = (SERIAL_CLIENTINFO*)xsd->cltlist.list[i];
        tally += clt->actsize;
    }

    size = xsd->buf_bytcnt;
    xVALIDATE_L(1334, tally<=size);

    size -= tally;
    xVERBOSE("XSER: Reserving %d bytes in SaveGame\n", size);

    *cur_space = size + 8;
    *max_fullgame = size + 8;

    return TRUE;
}

static S32 xSER_xsgclt_svproc_fill(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_WRITECONTEXT* wctxt)
{
    char filbuf[9] = "RyanNeil";

    xSGWriteData(xsg, wctxt, filbuf, 8);

    return TRUE;
}

static S32 xSER_xsgclt_ldproc_fill(void*, XSAVEGAME_DATA* xsg, XSAVEGAME_READCONTEXT* rctxt, U32, S32)
{
    XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    char filbuf[9] = { 0 };

    if (xsd->flg_info & 1)
        return TRUE;

    xSGReadData(xsg, rctxt, filbuf, 8);
    xVALIDATE_L(1374, !memcmp(filbuf,"RyanNeil",8));

    return TRUE;
}

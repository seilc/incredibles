#include "xRMemData.h"

#include "xMemMgr.h"
#include "xDebug.h"
#include "xOutputMgr.h"

#include <string.h>

void* RyzMemData::operator new(size_t amt, S32, RyzMemGrow* growCtxt)
{
    void* mem;

    S32 dogrow = TRUE;

    if (!growCtxt)
    {
        dogrow = FALSE;
    }
    else if (!growCtxt->IsEnabled())
    {
        dogrow = FALSE;
    }

    if (dogrow)
    {
        mem = xMEMGROWALLOC(amt);
    }
    else
    {
        mem = xMEMALLOC(amt, 0);
    }

    memset(mem, 0, 4);

    return mem;
}

void RyzMemData::operator delete(void*)
{
    return;
}

// RESTORED: from BFBB GC, TSSM GC
RyzMemGrow* RyzMemGrow::Resume(xBase*)
{
    xASSERT_MSG(this->ptr_last, "Resume has bad last info");

    this->amt = this->amt_last;
    this->ptr = this->ptr_last;
    this->user = this->user_last;
    this->flg_grow = 0x3;

    xOutInfo("RyzMemData",
             "RMemGrow (0x%08X [0x%08X]) base 0x%08X resume\n",
             this->user,
             this->user ? this->user->id : 0,
             this->ptr);

    return this;
}

void RyzMemGrow::Done()
{
    xOutInfo("RyzMemData",
             "RMemGrow (0x%08X [0x%08X]) base 0x%08X end\n",
             this->user,
             this->user ? this->user->id : 0,
             this->ptr);

    xASSERT(this->flg_grow & RMEM_GROW_ENABLED);
    xASSERT(this->amt==RMEM_GROWSTUB_SIZE);
    xASSERT(this->ptr);

    this->amt_last = this->amt;
    this->ptr_last = this->ptr;
    this->user_last = this->user;

    this->amt = 0;
    this->ptr = NULL;
    this->user = NULL;
    this->flg_grow = 0;
}
// END RESTORED

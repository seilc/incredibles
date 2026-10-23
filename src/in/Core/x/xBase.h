#ifndef XBASE_H
#define XBASE_H

#include "xLinkAsset.h"
#include "xserializer.h"

struct xBase;

typedef void (*xBaseEventCB)(xBase*, xBase*, U32, const F32*, xBase*, U32);

struct xBase
{
    U32 id;
    U8 baseType;
    U8 linkCount;
    U16 baseFlags;
    xLinkAsset* link;
    xBaseEventCB eventFunc;
};

struct xBaseAsset
{
    U32 id;
    U8 baseType;
    U8 linkCount;
    U16 baseFlags;
};

#define XBASE_ENABLED 0x1
#define XBASE_PERSISTENT 0x2
#define XBASE_VALID 0x4
#define XBASE_UNK10 0x10
#define XBASE_IS_ENT 0x20
#define k_XBASE_UNK_0x80 ((U16)(1 << 7))

void xBaseInit(xBase* xb, const xBaseAsset* asset);
void xBaseSetup(xBase*);
void xBaseSave(xBase* ent, xSerial* s);
void xBaseLoad(xBase* ent, xSerial* s);
void xBaseReset(xBase* xb, xBaseAsset* asset);

inline bool xBaseIsEnabled(const xBase* xb)
{
    return xb->baseFlags & XBASE_ENABLED;
}

inline void xBaseEnable(xBase* xb)
{
    xb->baseFlags |= XBASE_ENABLED;
}

inline void xBaseDisable(xBase* xb)
{
    xb->baseFlags &= ~XBASE_ENABLED;
}

inline void xBaseValidate(xBase* xb)
{
    xb->baseFlags |= XBASE_VALID;
}

#endif
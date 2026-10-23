#ifndef XBASE_H
#define XBASE_H

#include "xBaseAsset.h"
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

#define k_XBASE_IS_ENABLED ((U16)(1 << 0))
#define k_XBASE_IS_PERSISTENT ((U16)(1 << 1))
#define k_XBASE_IS_VALID ((U16)(1 << 2))
#define k_XBASE_IS_VISIBLE_IN_CUTSCENES ((U16)(1 << 3))
#define k_XBASE_RECEIVES_SHADOWS ((U16)(1 << 4))
#define k_XBASE_UNK_0x80 ((U16)(1 << 7))

#define k_XBASE_IS_ENTITY ((U16)(1 << 5))
#define k_XBASE_IS_NPC ((U16)(1 << 8))

void xBaseInit(xBase* xb, const xBaseAsset* asset);
void xBaseSetup(xBase*);
void xBaseSave(xBase* ent, xSerial* s);
void xBaseLoad(xBase* ent, xSerial* s);
void xBaseReset(xBase* xb, xBaseAsset* asset);

inline bool xBaseIsEnabled(const xBase* xb)
{
    return xb->baseFlags & k_XBASE_IS_ENABLED;
}

inline void xBaseEnable(xBase* xb)
{
    xb->baseFlags |= k_XBASE_IS_ENABLED;
}

inline void xBaseDisable(xBase* xb)
{
    xb->baseFlags &= (U16)~k_XBASE_IS_ENABLED;
}

inline void xBaseValidate(xBase* xb)
{
    xb->baseFlags |= k_XBASE_IS_VALID;
}

#endif
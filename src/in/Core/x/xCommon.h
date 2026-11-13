#ifndef XCOMMON_H
#define XCOMMON_H

#include "iCommon.h"
#include "xDebug.h"
#include "xMath3.h"
#include "xMemMgr.h"

template <class X, class Y>
static inline void FLAG_REMOVE(X& x, Y y)
{
    x = x & (X)~y;
}

#endif

#ifndef ZCONDASSET_H
#define ZCONDASSET_H

#include "xBase.h"

struct zCondAsset : xBaseAsset
{
    U32 constNum;
    U32 expr1;
    U32 op;
    U32 value_asset;
};

// zCondAsset::op
#define ZCOND_EQ 0
#define ZCOND_GT 1
#define ZCOND_LT 2
#define ZCOND_GE 3
#define ZCOND_LE 4
#define ZCOND_NE 5

#endif

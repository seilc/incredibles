#ifndef _MSL_FLOAT_H
#define _MSL_FLOAT_H

#ifdef __cplusplus
extern "C" {
#endif

int __float_max[];

#define FLT_MAX (*(float*)__float_max)

#ifdef __cplusplus
}
#endif

#endif

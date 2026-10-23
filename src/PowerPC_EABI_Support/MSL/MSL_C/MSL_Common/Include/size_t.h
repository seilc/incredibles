#ifndef _MSL_SIZE_T_H
#define _MSL_SIZE_T_H

#ifdef __MWERKS__
typedef __typeof__(sizeof(0)) size_t;
#else
typedef unsigned long size_t;
#endif

#endif

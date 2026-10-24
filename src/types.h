#ifndef TYPES_H
#define TYPES_H

typedef signed char S8;
typedef signed short S16;
typedef signed int S32;
typedef signed long long S64;
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef float F32;
typedef double F64;

#define FALSE 0
#define TRUE !FALSE
#define NULL 0L

#ifdef __MWERKS__
#define ALIGN(a) __attribute__ ((aligned(a)))
#else
#define ALIGN(a)
#endif

#define FORCEENUMSIZEINT ((S32)((~((U32)0))>>1))

#endif

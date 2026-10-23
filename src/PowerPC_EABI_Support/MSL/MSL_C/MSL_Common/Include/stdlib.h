#ifndef _MSL_STDLIB_H
#define _MSL_STDLIB_H

#include <size_t.h>

#ifdef __cplusplus
extern "C" {
#endif

extern double atof(const char*);
extern int atoi(const char*);

extern void* calloc(size_t, size_t);
extern void free(void*);
extern void* malloc(size_t);
extern void* realloc(void*, size_t);

extern void qsort(void*, size_t, size_t, int (*)(const void*, const void*));

#ifdef __cplusplus
}
#endif

#endif

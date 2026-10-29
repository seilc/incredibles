#ifndef _MSL_STRING_H
#define _MSL_STRING_H

#include <size_t.h>
#include <null.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void* memset(void*, int, size_t);
extern void* memcpy(void*, const void*, size_t);
extern void* memmove(void*, const void*, size_t);
extern int memcmp(const void*, const void*, size_t);
extern size_t strlen(const char*);
extern char* strcpy(char*, const char*);
extern char* strncpy(char*, const char*, size_t);
extern char* strcat(char*, const char*);
extern char* strncat(char*, const char*, size_t);
extern int strcmp(const char*, const char*);
extern int strncmp(const char*, const char*, size_t);
extern int stricmp(const char*, const char*);
extern int strcmpi(const char*, const char*);
extern char* strrchr(const char*, int);
extern char* strtok(char*, const char*);
extern char* strstr(const char*, const char*);

#ifdef __cplusplus
}
#endif

#endif

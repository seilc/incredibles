#ifndef XSTRING_H
#define XSTRING_H

#include "types.h"

#include <stddef.h>

struct substr
{
    const char* text;
    size_t size;

    static substr create(const char* text, size_t size) { substr s = { text, size }; return s; }
};

U32 xStrHash(const char* str);
U32 xStrHash(const char* s, size_t size);
U32 xStrHashCat(U32 prefix, const char* str);
char* xStrTok(char* string, const char* control, char** nextoken);
U32 xStrTokCount(const char* string, const char* control, U32* maxsize);
const char* xStrTokBuffer(const char* string, const char* control, void* buffer);
const char* xStristr(const char* str1, const char* str2);
S32 xStricmp(const char* string1, const char* string2);
S32 xMemicmp(const void* string1, const void* string2, size_t chars);
char* xStrupr(char* string);
S32 xStrParseFloatList(F32* dest, const char* strbuf, S32 max);
F32 xStrParseFloat(const char* x);
S32 imemcmp(const void* d1, const void* d2, size_t size);
S32 icompare(const substr& s1, const substr& s2);
U32 atox(const substr& s, size_t& read_size);
const char* find_char(const substr& s, const substr& cs);

#endif

#include "xString.h"

#include "xDebug.h"
#include "xOutputMgr.h"
#include "xMath.h"

#include <stdlib.h>

U32 xStrHash(const char* str)
{
    U32 i = 0;
    for (; *str != '\0'; ++str) {
        char c = *str;
        c = c - (c & (c >> 1) & 0x20);
        i = i * 131 + c;
    }
    return i;
}

U32 xStrHash(const char* s, size_t size)
{
    xASSERT_L(83, (size == 0) || (s != 0));

    U32 value = 0;
    for (size_t i = 0; i < size && *s != '\0'; ++i, ++s) {
        char c = *s;
        c = c - (c & (c >> 1) & 0x20);
        value = value * 131 + c;
    }
    return value;
}

U32 xStrHashCat(U32 prefix, const char* str)
{
    xWARN_INVALID_L(105, "UTIL", str);

    U32 i = prefix;
    for (; *str != '\0'; ++str) {
        char c = *str;
        c = c - (c & (c >> 1) & 0x20);
        i = i * 131 + c;
    }
    return i;
}

char* xStrTok(char* string, const char* control, char** nextoken)
{
    U8* str;
    const U8* ctrl = (const U8*)control;
    U8 map[32];

    for (S32 count = 0; count < 32; ++count) {
        map[count] = 0;
    }
    do {
        map[*ctrl >> 3] |= (U8)(1 << (*ctrl & 0x7));
    } while (*ctrl++);

    if (string) {
        str = (U8*)string;
    } else {
        str = (U8*)*nextoken;
    }
    while ((map[*str >> 3] & (1 << (*str & 0x7))) && *str) {
        ++str;
    }

    string = (char*)str;

    while (*str) {
        if (map[*str >> 3] & (1 << (*str & 0x7))) {
            *str = '\0';
            ++str;
            break;
        }
        ++str;
    }

    *nextoken = (char*)str;

    if (string == (char*)str) {
        return NULL;
    }
    
    return string;
}

U32 xStrTokCount(const char* string, const char* control, U32* maxsize)
{
    U32 num = 0;
    U32 max = 0;
    U32 test = 0;
    const U8* str;
    const U8* ctrl = (const U8*)control;
    U8 map[32];

    for (S32 count = 0; count < 32; ++count) {
        map[count] = 0;
    }
    do {
        map[*ctrl >> 3] |= (U8)(1 << (*ctrl & 0x7));
    } while (*ctrl++);
    
    str = (const U8*)string;

    while (true) {
        while ((map[*str >> 3] & (1 << (*str & 0x7))) && *str) {
            ++str;
        }

        string = (const char*)str;

        test = 0;
        while (*str) {
            if (map[*str >> 3] & (1 << (*str & 0x7))) {
                ++str;
                break;
            }
            ++test;
            ++str;
        }

        if (string == (const char*)str) {
            if (maxsize) {
                *maxsize = max;
            }

            return num;
        }

        if (test > max) {
            max = test;
        }

        ++num;
    }
}

const char* xStrTokBuffer(const char* string, const char* control, void* buffer)
{
    char** nextoken = (char**)buffer;
    char* dest = (char*)(nextoken + 1);
    const U8* str;
    const U8* ctrl = (const U8*)control;
    U8 map[32];

    for (S32 count = 0; count < 32; ++count) {
        map[count] = 0;
    }
    do {
        map[*ctrl >> 3] |= (U8)(1 << (*ctrl & 0x7));
    } while (*ctrl++);

    if (string) {
        str = (const U8*)string;
    } else {
        str = (const U8*)*nextoken;
    }
    while ((map[*str >> 3] & (1 << (*str & 0x7))) && *str) {
        ++str;
    }

    string = (const char*)str;

    while (*str) {
        if (map[*str >> 3] & (1 << (*str & 0x7))) {
            ++str;
            break;
        }

        *dest = *str;
        ++dest;
        ++str;
    }

    *dest = '\0';
    *nextoken = (char*)str;

    if (string == (const char*)str) {
        return NULL;
    }
    
    return (const char*)(nextoken + 1);
}

#define xToUpper(c) (((c) >= 'a' && (c) <= 'z') ? (c) - 32 : (c))

const char* xStristr(const char* str1, const char* str2)
{
    char* cp = (char*)str1;
    
    if (*str2 == '\0') {
        return str1;
    }

    while (*cp) {
        char* s1 = cp;
        char* s2 = (char*)str2;
        for (; *s1 && *s2 && (xToUpper(*s1) - xToUpper(*s2)) == 0; s1++, s2++) {}
        if (*s2 == '\0') {
            return cp;
        }
        ++cp;
    }

    return NULL;
}

S32 xStricmp(const char* string1, const char* string2)
{
    S32 result = 0;

    while (xToUpper(*string1) == xToUpper(*string2) && result == 0) {
        if (*string1 == '\0' || *string2 == '\0') {
            result = 1;
            continue;
        }
        ++string1;
        ++string2;
    }

    result = 0;
    if (*string1 != *string2) {
        result = 1;
        if (xToUpper(*string1) < xToUpper(*string2)) {
            result = -1;
        }
    }

    return result;
}

S32 xMemicmp(const void* string1, const void* string2, size_t chars)
{
    const char* str1 = (const char*)string1;
    const char* str2 = (const char*)string2;

    while (chars--) {
        S32 ret = xToUpper(*str2) - xToUpper(*str1);
        if (ret != 0) {
            return ret;
        }
        ++str1;
        ++str2;
    }
    
    return 0;
}

char* xStrupr(char* string)
{
    char* p = string;
    while (*p) {
        *p = xToUpper(*p);
        ++p;
    }
    return string;
}

S32 xStrParseFloatList(F32* dest, const char* strbuf, S32 max)
{
    char* str = (char*)strbuf;
    if (str == NULL) {
        return 0;
    }
    
    S32 index = 0;

    while (*str && index < max) {
        while (*str == '\t' ||
               *str == ' ' ||
               *str == '+' ||
               *str == '[' ||
               *str == ']' ||
               *str == '{' ||
               *str == '}' ||
               *str == '(' ||
               *str == ')' ||
               *str == ',' ||
               *str == ':' ||
               *str == ';') {
            ++str;
        }

        if (*str == '\0') {
            return index;
        }

        S32 digits;
        S32 negate;
        char tmpc;
        char* numstart;

        if (*str == '-') {
            negate = 1;
            ++str;
            while (*str == '\t' || *str == ' ') {
                ++str;
            }
        } else {
            negate = 0;
        }

        numstart = str;
        digits = 0;

        while ((*str >= '0' && *str <= '9') ||
               *str == '.' ||
               *str == 'e' ||
               *str == 'E' ||
               *str == 'f') {
            if (*str >= '0' && *str <= '9') {
                ++digits;
            }
            ++str;
        }

        if (digits == 0) {
            return index;
        }

        tmpc = *str;
        *str = '\0';

        dest[index] = xatof(numstart);

        if (negate) {
            dest[index] = -dest[index];
        }

        *str = tmpc;

        ++index;
    }

    return index;
}

namespace {
    inline S32 tolower(S32 c)
    {
        return (c | (c >> 1) & 0x20);
    }

    inline S32 tolower(char c)
    {
        return tolower((S32)c);
    }
}

S32 imemcmp(const void* d1, const void* d2, size_t size)
{
    const char* s1 = (const char*)d1;
    const char* s2 = (const char*)d2;

    for (size_t i = 0; i < size; ++i, ++s1, ++s2) {
        S32 c1 = tolower(*s1);
        S32 c2 = tolower(*s2);
        if (c1 != c2) {
            return c1 - c2;
        }
    }

    return 0;
}

S32 icompare(const substr& s1, const substr& s2)
{
    size_t len = xmin(s1.size, s2.size);
    S32 c = imemcmp(s1.text, s2.text, len);
    if (c != 0) {
        return c;
    }

    if (s1.size == s2.size) {
        return 0;
    }

    if (s1.size < s2.size) {
        return -1;
    }

    return 1;
}

U32 atox(const substr& s, size_t& read_size)
{
    const char* p = s.text;
    size_t size = s.size;

    if (p == NULL) {
        return 0;
    }

    U32 total = 0;

    if (size > 8) {
        size = 8;
    }

    for (read_size = 0; read_size < size; ++read_size, ++p) {
        char c = *p;
        U32 v;
        if (c >= '0' && c <= '9') {
            v = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            v = c - ('a' - 10);
        } else if (c >= 'A' && c <= 'F') {
            v = c - ('A' - 10);
        } else {
            break;
        }
        total = (total << 4) + v;
    }

    return total;
}

const char* find_char(const substr& s, const substr& cs)
{
    if (s.text == NULL || cs.text == NULL) {
        return NULL;
    }

    const char* p = s.text;
    const char* d = cs.text;

    switch (cs.size) {
    case 0:
        break;
    case 1:
        for (S32 i = s.size; i > 0 && *p != '\0'; --i, ++p) {
            char c = *p;
            if (c == d[0]) {
                return p;
            }
        }
        break;
    case 2:
        for (S32 i = s.size; i > 0 && *p != '\0'; --i, ++p) {
            char c = *p;
            if (c == d[0] ||
                c == d[1]) {
                return p;
            }
        }
        break;
    case 3:
        for (S32 i = s.size; i > 0 && *p != '\0'; --i, ++p) {
            char c = *p;
            if (c == d[0] ||
                c == d[1] ||
                c == d[2]) {
                return p;
            }
        }
        break;
    case 4:
        for (S32 i = s.size; i > 0 && *p != '\0'; --i, ++p) {
            char c = *p;
            if (c == d[0] ||
                c == d[1] ||
                c == d[2] ||
                c == d[3]) {
                return p;
            }
        }
        break;
    case 5:
        for (S32 i = s.size; i > 0 && *p != '\0'; --i, ++p) {
            char c = *p;
            if (c == d[0] ||
                c == d[1] ||
                c == d[2] ||
                c == d[3] ||
                c == d[4]) {
                return p;
            }
        }
        break;
    case 6:
        for (S32 i = s.size; i > 0 && *p != '\0'; --i, ++p) {
            char c = *p;
            if (c == d[0] ||
                c == d[1] ||
                c == d[2] ||
                c == d[3] ||
                c == d[4] ||
                c == d[5]) {
                return p;
            }
        }
        break;
    case 7:
        for (S32 i = s.size; i > 0 && *p != '\0'; --i, ++p) {
            char c = *p;
            if (c == d[0] ||
                c == d[1] ||
                c == d[2] ||
                c == d[3] ||
                c == d[4] ||
                c == d[5] ||
                c == d[6]) {
                return p;
            }
        }
        break;
    case 8:
        for (S32 i = s.size; i > 0 && *p != '\0'; --i, ++p) {
            char c = *p;
            if (c == d[0] ||
                c == d[1] ||
                c == d[2] ||
                c == d[3] ||
                c == d[4] ||
                c == d[5] ||
                c == d[6] ||
                c == d[7]) {
                return p;
            }
        }
        break;
    case 9:
        for (S32 i = s.size; i > 0 && *p != '\0'; --i, ++p) {
            char c = *p;
            if (c == d[0] ||
                c == d[1] ||
                c == d[2] ||
                c == d[3] ||
                c == d[4] ||
                c == d[5] ||
                c == d[6] ||
                c == d[7] ||
                c == d[8]) {
                return p;
            }
        }
        break;
    case 10:
        for (S32 i = s.size; i > 0 && *p != '\0'; --i, ++p) {
            char c = *p;
            if (c == d[0] ||
                c == d[1] ||
                c == d[2] ||
                c == d[3] ||
                c == d[4] ||
                c == d[5] ||
                c == d[6] ||
                c == d[7] ||
                c == d[8] ||
                c == d[9]) {
                return p;
            }
        }
        break;
    case 11:
        for (S32 i = s.size; i > 0 && *p != '\0'; --i, ++p) {
            char c = *p;
            if (c == d[0] ||
                c == d[1] ||
                c == d[2] ||
                c == d[3] ||
                c == d[4] ||
                c == d[5] ||
                c == d[6] ||
                c == d[7] ||
                c == d[8] ||
                c == d[9] ||
                c == d[10]) {
                return p;
            }
        }
        break;
    default:
        for (S32 i = s.size; i > 0 && *p != '\0'; --i, ++p) {
            for (const char* s = cs.text; *s != '\0'; ++s) {
                if (*p == *s) {
                    return p;
                }
            }
        }
        break;
    }

    return NULL;
}

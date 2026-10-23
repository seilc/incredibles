#ifndef XINI_H
#define XINI_H

#include "types.h"

struct xIniValue
{
    const char* tok;
    const char* val;
};

struct xIniSection
{
    const char* sec;
    S32 first;
    S32 count;
};

struct xIniFile
{
    S32 NumValues;
    S32 NumSections;
    xIniValue* Values;
    xIniSection* Sections;
    void* mem;
    char name[256];
    char pathname[256];
};

xIniFile* xIniParse(char* buf, S32 len);
void xIniDestroy(xIniFile* ini);
S32 xIniGetIndex(xIniFile* ini, const char* tok);
S32 xIniGetInt(xIniFile* ini, const char* tok, S32 def);
F32 xIniGetFloat(xIniFile* ini, const char* tok, F32 def);
const char* xIniGetString(xIniFile* ini, const char* tok, const char* def);
S32 xIniGetIndex(xIniFile* ini, const char* tok, S32 idx);
const char* xIniGetString(xIniFile* ini, const char* tok, const char* def, S32& idx);

#endif

#include "xIni.h"

#include "xDebug.h"
#include "xString.h"

#include <cstring>
#include <stdlib.h>
#include <rwcore.h>

static char* TrimWhitespace(char* str)
{
    xASSERT_L(16, str);

    while (*str == ' ' || *str == '\t') str++;
    if (*str == '\0') return str;

    char* c = str + (strlen(str) - 1);
    while (*c == ' ' || *c == '\t') c--;
    c[1] = '\0';

    return str;
}

xIniFile* xIniParse(char* buf, S32 len)
{
    S32 valueAlloc, sectionAlloc, i;
    S32 ccr = 1;
    S32 clf = 1;
    S32 copen = 0;
    S32 lastCRLF = -1;
    char* c, *tok, *val, *lastLine, *line, *ltoken;
    xIniFile* ini;

    for (i = 0; i < len; i++) {
        switch (buf[i]) {
        case '\n':
            lastCRLF = i;
            clf++;
            break;
        case '\r':
            lastCRLF = i;
            ccr++;
            break;
        case '[':
            copen++;
            break;
        }
    }

    if (clf > ccr) ccr = clf;

    sectionAlloc = copen;
    valueAlloc = ccr;

    ini = (xIniFile*)RwMallocL(sizeof(xIniFile)
                             + valueAlloc * sizeof(xIniValue)
                             + sectionAlloc * sizeof(xIniSection)
                             + (len - lastCRLF),
                               rwMEMHINTDUR_NADURATION, 57);

    ini->mem = NULL;
    ini->NumValues = 0;
    ini->NumSections = 0;
    ini->Values = (xIniValue*)(ini + 1);
    ini->Sections = (xIniSection*)(ini->Values + valueAlloc);

    lastLine = (char*)(ini->Sections + sectionAlloc);
    strncpy(lastLine, buf + (lastCRLF + 1), len - (lastCRLF + 1));
    lastLine[len - (lastCRLF + 1)] = '\0';

    if (lastCRLF >= 0) {
        buf[lastCRLF] = '\0';
    } else {
        buf[0] = '\0';
    }

    line = xStrTok(buf, "\n\r", &ltoken);
    if (!line) {
        line = xStrTok(lastLine, "\n\r", &ltoken);
        lastLine = NULL;
    }

    while (line) {
        line = TrimWhitespace(line);
        if (line[0] != '#' && line[0] != '\0') {
            if (line[0] == '[') {
                c = std::strstr(line, "]");
                if (c) {
                    *c = '\0';
                    c = TrimWhitespace(line + 1);
                    if (*c != '\0') {
                        ini->Sections[ini->NumSections].sec = c;
                        ini->Sections[ini->NumSections].first = ini->NumValues;
                        ini->Sections[ini->NumSections].count = 0;
                        ini->NumSections++;
                    }
                }
            } else {
                c = std::strstr(line, "=");
                if (c) {
                    *c = '\0';
                    tok = TrimWhitespace(line);
                    if (*tok != '\0') {
                        line = c + 1;
                        c = std::strstr(line, "#");
                        if (c) *c = '\0';
                        val = TrimWhitespace(line);
                        ini->Values[ini->NumValues].tok = tok;
                        ini->Values[ini->NumValues].val = val;
                        ini->NumValues++;
                        if (ini->NumSections != 0) {
                            ini->Sections[ini->NumSections-1].count++;
                        }
                    }
                }
            }
        }

        line = xStrTok(NULL, "\n\r", &ltoken);
        if (!line && lastLine) {
            line = xStrTok(lastLine, "\n\r", &ltoken);
            lastLine = NULL;
        }
    }

    return ini;
}

void xIniDestroy(xIniFile* ini)
{
    RwFree(ini->mem);
    RwFree(ini);
}

S32 xIniGetIndex(xIniFile* ini, const char* tok)
{
    for (S32 i = 0; i < ini->NumValues; i++) {
        if (xStricmp(ini->Values[i].tok, tok) == 0) {
            return i;
        }
    }
    return -1;
}

S32 xIniGetInt(xIniFile* ini, const char* tok, S32 def)
{
    S32 index = xIniGetIndex(ini, tok);
    if (index == -1) {
        return def;
    }
    return atoi(ini->Values[index].val);
}

F32 xIniGetFloat(xIniFile* ini, const char* tok, F32 def)
{
    S32 index = xIniGetIndex(ini, tok);
    if (index == -1) {
        return def;
    }
    return atof(ini->Values[index].val);
}

const char* xIniGetString(xIniFile* ini, const char* tok, const char* def)
{
    S32 index = xIniGetIndex(ini, tok);
    if (index == -1) {
        return def;
    }
    return ini->Values[index].val;
}

S32 xIniGetIndex(xIniFile* ini, const char* tok, S32 idx)
{
    for (S32 i = idx; i < ini->NumValues; i++) {
        if (xStricmp(ini->Values[i].tok, tok) == 0) {
            return i;
        }
    }
    return -1;
}

const char* xIniGetString(xIniFile* ini, const char* tok, const char* def, S32& idx)
{
    idx = xIniGetIndex(ini, tok, idx);
    if (idx == -1) {
        return def;
    }
    return ini->Values[idx++].val;
}

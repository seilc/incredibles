#ifndef ZCINEFX_H
#define ZCINEFX_H

#include "xFX.h"
#include "xMath3.h"

struct zCutsceneMgr;
struct zLightning;
struct zParEmitter;
struct xEnt;

struct NCINLyt
{
    zLightning* lyt_zap;
};

struct NCINStreak
{
    U32 streakID;
};

struct NCINArc
{
    zLightning* lightning;
    xVec3 endPos;
};

struct NCINPar
{
    zParEmitter* emitter;
};

struct NCINEnts
{
    xEnt* ent[4];
};

struct NCINCustom
{
    F32 f[1];
    xVec3 v[2];
    S32 n[3];
};

struct NCINMat
{
    RwMatrix* mat;
};

struct NCINRibbon
{
    xFXRibbon* ribbon;
};

union NCINData
{
    NCINLyt lytdata;
    NCINStreak streakdata;
    NCINArc arcdata;
    NCINPar pardata;
    NCINEnts entdata;
    NCINCustom customdata;
    NCINMat matdata;
    NCINRibbon ribbondata;
};

struct NCINRibbonInput
{
    xFXRibbon::config_type ribcfg;
};

union NCINInput
{
    void* whateverMakesTheCompilerHappyIsFineWithMe;
    void* fxparm;
    const char* str_fx;
    S32 int_val;
    F32 flt_val;
    NCINRibbonInput* ribbon;
};

struct NCINEntry
{
    struct TypeInfo
    {
        union
        {
            S32 fxTypeID;
            S32 usrTypeID;
        };
        F32 tym_beg;
        F32 tym_end;
        const char* twk_name;
    };

    struct Callbacks
    {
        void (*fxupd)(zCutsceneMgr*, NCINEntry*, S32);
        void (*fxanim)(zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrix*, U32, U32);
        void (*fxrend)(zCutsceneMgr*, NCINEntry*);
        void (*tweaks)(const char*, NCINEntry*);
    };

    struct IndexInput
    {
        S32 idx_anim;
        S32 idx_bone;
        S32 idx_boneA;
        S32 idx_boneB;
    };

    struct PosInput
    {
        xVec3 pos_A[2];
        xVec3 pos_B[2];
    };

    TypeInfo info;
    Callbacks cb;
    IndexInput index;
    PosInput basic;
    NCINInput input;
    void* userFXData;
    NCINData fxdata;
    S32 flg_stat;
};

#endif
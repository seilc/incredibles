#ifndef _FMODGC_H_
#define _FMODGC_H_

#include "fmod.h"

typedef struct FSOUND_GCADPCM_HDR
{
    unsigned int num_samples;
    unsigned int num_adpcm_nibbles;
    unsigned int sample_rate;
    unsigned short loop_flag;
    unsigned short format;
    unsigned int sa;
    unsigned int ea;
    unsigned int ca;
    unsigned short coef[16];
    unsigned short gain;
    unsigned short pred_scale;
    unsigned short yn1;
    unsigned short yn2;
    unsigned short loop_pred_scale;
    unsigned short loop_yn1;
    unsigned short loop_yn2;
    unsigned short pad[11];
} FSOUND_GCADPCM_HDR;

typedef struct FSOUND_GCADPCMINFO
{
    signed short coef[16];
    unsigned short gain;
    unsigned short pred_scale;
    signed short yn1;
    signed short yn2;
    unsigned short loop_pred_scale;
    signed short loop_yn1;
    signed short loop_yn2;
} FSOUND_GCADPCMINFO;

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*FSOUND_GC_DVDSTATUSCALLBACK)(int status);

signed char FSOUND_SetAramBlock(unsigned int, unsigned int);

signed char FSOUND_AUXFX_SetLevels(int channel, int, int);
signed char FSOUND_AUXFX_SetReverbHi(int, float, float, float, float, float, float);
signed char FSOUND_AUXFX_SetReverbHiDpl2(float, float, float, float, float);
signed char FSOUND_AUXFX_ShutdownEffect(int);

void FSOUND_GC_Sample_SetGCADPCMInfo(FSOUND_SAMPLE* sptr, FSOUND_GCADPCMINFO* info);
signed char FSOUND_GC_Stream_SetGCADPCMLoopContext(FSOUND_STREAM* stream);

void FSOUND_GC_SetUpdateDvdStatusCallback(FSOUND_GC_DVDSTATUSCALLBACK callback);
void FSOUND_GC_SetDvdStatusCallback(FSOUND_GC_DVDSTATUSCALLBACK callback);

#ifdef __cplusplus
}
#endif

#endif

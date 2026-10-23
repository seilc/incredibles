#ifndef XONELINERPLAYER_H
#define XONELINERPLAYER_H

#include "iSnd.h"

class xOneLinerManager;

struct BasicInform
{
};

enum ePlayerType
{
    eALWAYS,
    eCOUNTER,
    eCHECKER,
    eTESTER,
    ePLAYER_TYPE_SIZE
};

struct testerPlayer
{
    S32 firstParam;
    F32 secondParam;
};

class xOneLiner
{
private:
    U32 m_soundGroupNameHash;
    F32 m_fSoundStartDelay;
    F32 m_fTimeSpan;
    F32 m_fTimeLastPlayed;
    U32 m_uNumPlays;
    F32 m_fDelayBetweenPlays;
    F32 m_fProbability;
    F32 m_fDefaultDuration;
    F32 m_fLastDuration;
    U32 m_uMaxPlays;
    iSndGroupHandle m_soundGroupHandle;
    xOneLinerManager* m_pOLManager;
    S16 m_eventType;
    S16 m_bPlaysInMusicChannel;
    BasicInform* m_pData;
    ePlayerType m_playerType;
    testerPlayer m_testerData;
};

#endif
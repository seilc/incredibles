#ifndef XONELINERPLAYER_H
#define XONELINERPLAYER_H

#include "xSndMgr.h"

class xOneLinerManager;

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

class BasicInform
{
public:
    virtual void Inform(testerPlayer& data, F32 fDuration, void* uniqueness) = 0;
    virtual bool Age(F32 fHowLong) = 0;
    virtual bool Test(testerPlayer& data) = 0;
};

class xOneLiner
{
public:
    bool Init();
    bool UpdatePlaySound(F32 currentTime, F32 fRollsPerSecond);

    void Inform(F32 fDuration, void* uniqueness)
    {
        m_pData->Inform(m_testerData, fDuration, uniqueness);
        m_fLastDuration = fDuration;
    }

    bool Age(F32 fHowLong)
    {
        return m_pData->Age(fHowLong);
    }

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
    xSndGroupHandle m_soundGroupHandle;
    xOneLinerManager* m_pOLManager;
    S16 m_eventType;
    S16 m_bPlaysInMusicChannel;
    BasicInform* m_pData;
    ePlayerType m_playerType;
    testerPlayer m_testerData;
};

#endif
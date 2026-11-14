#ifndef XONELINERMANAGER_H
#define XONELINERMANAGER_H

#include "xSndMgr.h"

class xOneLiner;

class xOneLinerManager
{
public:
    void Init(U16 eventCount);
    void Inform(S32 event, void* pEventSpecific, F32 fOverrideDuration);
    bool SystemIsDisabled();
    void PlaySound(S32 index, F32 currentTime);
    void play();
    void playAsMusic();
    void PlayAudio();

private:
    xOneLiner* m_aOneLinerPlayers;
    S32 m_iNextSound;
    xSndGroupHandle m_currentSoundID;
    F32 m_pauseBetweenSounds;
    F32 m_lastUpdateTime;
    F32 m_lastPlayedTime;
    F32 m_queuedSoundTimer;
    U16 m_NumEventTypes;
    U16 m_NumOLPlayers;
    U16 m_nextSoundPriority;
    U16 m_priorityFilter;
};

#endif

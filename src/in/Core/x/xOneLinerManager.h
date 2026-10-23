#ifndef XONELINERMANAGER_H
#define XONELINERMANAGER_H

#include "xOneLinerPlayer.h"

class xOneLinerManager
{
public:
    void Inform(S32 event, void* pEventSpecific, F32 fOverrideDuration);

private:
    xOneLiner* m_aOneLinerPlayers;
    S32 m_iNextSound;
    iSndGroupHandle m_currentSoundID;
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
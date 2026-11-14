#include "xCommon.h"
#include "xOneLinerManager.h"

#include "xOneLinerPlayer.h"
#include "xOneLinerAsset.h"
#include "xSndMgr.h"
#include "xstransvc.h"
#include "xRand.h"
#include "iTime.h"
#include "zMain.h"
#include "zMusic.h"

#include <string.h>
#include <stdlib.h>

static S32 compare(const void* p1, const void* p2)
{
    if (((xOneLiner*)p1)->m_eventType < ((xOneLiner*)p2)->m_eventType) {
        return -1;
    }
    if (((xOneLiner*)p1)->m_eventType == ((xOneLiner*)p2)->m_eventType) {
        return 0;
    }
    return 1;
}

void xOneLinerManager::Init(U16 eventCount)
{
    m_NumEventTypes = eventCount;
    m_lastUpdateTime = iTimeGetGame();
    m_iNextSound = -1;
    m_currentSoundID = ISND_INVALID_SND_GROUP_HANDLE;
    m_queuedSoundTimer = 0.0f;
    m_priorityFilter = 0;

    S32 count = xSTAssetCountByType('ONEL');
    for (S32 i = 0; i < count; i++) {
        char* buffer = (char*)xSTFindAssetByType('ONEL', i, NULL);
        m_NumOLPlayers += ((xOneLinerAsset*)buffer)->oneLinerPlayerCount;
    }

    m_aOneLinerPlayers = (xOneLiner*)xMEMALLOC(m_NumOLPlayers * sizeof(xOneLiner), 0);
    m_NumOLPlayers = 0;

    for (S32 i = 0; i < count; i++) {
        char* buffer = (char*)xSTFindAssetByType('ONEL', i, NULL);
        xOneLinerAsset* pAsset = (xOneLinerAsset*)buffer;
        xOneLiner* pLiner = &m_aOneLinerPlayers[m_NumOLPlayers];
        memcpy(pLiner, &pAsset->player, pAsset->oneLinerPlayerCount * sizeof(xOneLiner));
        for (U32 j = 0; j < pAsset->oneLinerPlayerCount; j++) {
            pLiner[j].Init();
        }
        m_NumOLPlayers += pAsset->oneLinerPlayerCount;
    }

    for (U32 i = 0; i < m_NumOLPlayers; i++) {
        m_aOneLinerPlayers[i].m_pOLManager = this;
        m_aOneLinerPlayers[i].m_soundGroupHandle = xSndMgrGetSoundGroup(m_aOneLinerPlayers[i].m_soundGroupNameHash);
        m_aOneLinerPlayers[i].m_fTimeLastPlayed = -m_aOneLinerPlayers[i].m_fDelayBetweenPlays;
    }

    qsort(m_aOneLinerPlayers, m_NumOLPlayers, sizeof(xOneLiner), compare);
}

inline bool xOneLinerManager::SystemIsDisabled()
{
    bool bDialogPlaying = xSndMgrIsDialogPlaying();
    bool bControlOff = globals.player.ControlOff != 0.0f;
    if (bDialogPlaying || bControlOff) {
        return true;
    }
    return false;
}

void xOneLinerManager::Inform(S32 event, void* pEventSpecific, F32 fOverrideDuration)
{
    if (event < m_NumEventTypes && m_NumOLPlayers != 0 && !SystemIsDisabled()) {
        S32 lowerIndex = 0;
        S32 upperIndex = m_NumOLPlayers;
        while (upperIndex > lowerIndex) {
            S32 thisIndex = (upperIndex + lowerIndex) / 2;
            if (event == m_aOneLinerPlayers[thisIndex].m_eventType) {
                if (thisIndex > 0) {
                    while (event == m_aOneLinerPlayers[thisIndex - 1].m_eventType) {
                        thisIndex--;
                    }
                }
                for (S32 i = thisIndex; event == m_aOneLinerPlayers[i].m_eventType; i++) {
                    F32 fDuration = fOverrideDuration;
                    if (fDuration < 0.0f) {
                        fDuration = m_aOneLinerPlayers[i].m_fDefaultDuration;
                    }
                    m_aOneLinerPlayers[i].Inform(fDuration, pEventSpecific);
                    if (fDuration <= 0.0f) {
                        F32 currentTime = iTimeGetGame();
                        bool bPlay = m_aOneLinerPlayers[i].UpdatePlaySound(currentTime, 0.0f);
                        if (bPlay) {
                            PlaySound(i, currentTime);
                        }
                    }
                }
                return;
            }
            if (event > m_aOneLinerPlayers[thisIndex].m_eventType) {
                lowerIndex = thisIndex + 1;
            } else {
                upperIndex = thisIndex;
            }
        }
    }
}

void xOneLinerManager::PlaySound(S32 index, F32 currentTime)
{
    if (m_queuedSoundTimer <= 0.0f &&
        (m_currentSoundID == ISND_INVALID_SND_GROUP_HANDLE || !xSndMgrIsPlaying(m_currentSoundID)) &&
        currentTime - m_lastPlayedTime >= m_pauseBetweenSounds) {
        if (m_iNextSound == -1 ||
            xSndMgrGetPriority(m_aOneLinerPlayers[index].m_soundGroupHandle, false, false) >
                xSndMgrGetPriority(m_aOneLinerPlayers[m_iNextSound].m_soundGroupHandle, false, false)) {
            m_iNextSound = index;
        }
    }
}

void xOneLinerManager::play()
{
    xSndMgrPlay(m_currentSoundID, 0, NULL, NULL, NULL, NULL, NULL);
    m_iNextSound = -1;
}

void xOneLinerManager::playAsMusic()
{
    F32 fFadeVolTime = m_aOneLinerPlayers[m_iNextSound].m_fSoundStartDelay;
    F32 fNewVolTime = m_aOneLinerPlayers[m_iNextSound].m_fSoundStartDelay;
    F32 fPlayTime = m_aOneLinerPlayers[m_iNextSound].m_fTimeSpan;
    F32 fNewVolume = 1.0f;
    F32 toParam[4] = { fFadeVolTime, fNewVolTime, fPlayTime, fNewVolume };
    U8 soundIndex = xrand_RandomChoice(xSndMgrGetSoundCount(m_aOneLinerPlayers[m_iNextSound].m_soundGroupHandle));
    U32 soundID = xSndMgrGetSoundId(m_aOneLinerPlayers[m_iNextSound].m_soundGroupHandle, soundIndex);
    zMusicTempSongEvent(toParam, soundID);
    m_iNextSound = -1;
}

void xOneLinerManager::PlayAudio()
{
    F32 currentTime = iTimeGetGame();
    F32 fTimeSinceLastUpdate = currentTime - m_lastUpdateTime;
    if (fTimeSinceLastUpdate >= 0.1f) {
        for (S32 i = 0; i < m_NumOLPlayers; i++) {
            if (m_aOneLinerPlayers[i].Age(fTimeSinceLastUpdate)) {
                bool bPlay = m_aOneLinerPlayers[i].UpdatePlaySound(currentTime, 10.0f);
                if (bPlay) {
                    PlaySound(i, currentTime);
                }
            }
        }
        m_lastUpdateTime = currentTime;
    }

    if (m_queuedSoundTimer > 0.0f) {
        m_queuedSoundTimer -= xglobals->update_dt;
        if (m_queuedSoundTimer <= 0.0f) {
            play();
        }
    } else if (!xSndMgrIsPlaying(m_currentSoundID) && m_iNextSound != -1) {
        m_currentSoundID = m_aOneLinerPlayers[m_iNextSound].m_soundGroupHandle;
        m_aOneLinerPlayers[m_iNextSound].m_uNumPlays++;
        F32 currentTime = iTimeGetGame();
        m_aOneLinerPlayers[m_iNextSound].m_fTimeLastPlayed = currentTime;
        if (m_aOneLinerPlayers[m_iNextSound].m_bPlaysInMusicChannel == TRUE) {
            playAsMusic();
        } else if (m_aOneLinerPlayers[m_iNextSound].m_fSoundStartDelay > 0.0f) {
            m_queuedSoundTimer = m_aOneLinerPlayers[m_iNextSound].m_fSoundStartDelay;
        } else {
            play();
        }
    }
}

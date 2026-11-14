#ifndef XONELINERMANAGER_H
#define XONELINERMANAGER_H

#include "xSndMgr.h"

class xOneLiner;

enum
{
    eOL_DIED,
    eOL_DIED_FALLING,
    eOL_ZIPLINE,
    eOL_NEAR_LEDGE,
    eOL_TURRET_MOUNT,
    eOL_EVENT_5,
    eOL_ENEMY_IN_RANGE,
    eOL_ENEMY_OUT_OF_RANGE,
    eOL_LIFT_LIGHT,
    eOL_LIFT_MEDIUM,
    eOL_LIFT_HEAVY,
    eOL_THROW_LIGHT,
    eOL_THROW_MEDIUM,
    eOL_THROW_HEAVY,
    eOL_IDLE,
    eOL_RUNNING,
    eOL_WALKING,
    eOL_FALLING,
    eOL_CARRY_SMALL,
    eOL_CARRY_LARGE,
    eOL_CARRY_MEDIUM,
    eOL_ON_TURRET,
    eOL_ZAPPED,
    eOL_BOOMED,
    eOL_BURNT,
    eOL_HIT,
    eOL_STRUCK_ROBOTS,
    eOL_STRUCK_HUMANS,
    eOL_HIT_BY_ENEMIES,
    eOL_HAND_HURT,
    eOL_STRIKE_BREAKS,
    eOL_PICKUP,
    eOL_TURNING,
    eOL_LEDGE_GRAB,
    eOL_LEDGE_JUMP_UP,
    eOL_LEDGE_JUMP_DOWN,
    eOL_LEDGE_FLIP,
    eOL_LEDGE_MOVE,
    eOL_CHECKPOINT,
    eOL_JUMP,
    eOL_TRIPPED,
    eOL_DETECTED,
    eOL_GO_VISIBLE,
    eOL_INVISIBLE_LOW,
    eOL_BALL_METER,
    eOL_BALL_SPEED,
    eOL_INVISIBLE,
    eOL_DASH_SPEED,
    eOL_DASH_FRICTION,
    eOL_STOP_RUNNING,
    eOL_EVENT_SIZE
};

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

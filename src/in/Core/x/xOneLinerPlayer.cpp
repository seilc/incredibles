#include "xCommon.h"
#include "xOneLinerPlayer.h"

#include "xOneLinerManager.h"
#include "xMemMgr.h"
#include "xMath.h"
#include "xRand.h"
#include "iTime.h"

#include <string.h>

#define MAX_TRACKED 10

class CActiveEvent : public BasicInform
{
public:
    void Inform(testerPlayer&, F32 fDuration, void*)
    {
        m_fDuration = fDuration;
    }

    bool Age(F32 fHowLong)
    {
        m_fDuration -= fHowLong;
        m_fDuration = MAX(m_fDuration, 0.0f);
        return m_fDuration > 0.0f;
    }

    bool Test(testerPlayer&)
    {
        return true;
    }

    F32 m_fDuration;
};

class CTrueForYLong : public BasicInform
{
public:
    void Inform(testerPlayer&, F32, void* uniqueness)
    {
        m_Uniqueness = uniqueness;
        if (uniqueness) {
            if (m_fTruthStartTime <= 0.0f) {
                m_fTruthStartTime = iTimeGetGame();
            }
        } else {
            m_fTruthStartTime = 0.0f;
        }
    }

    bool Age(F32)
    {
        if (m_Uniqueness) {
            return true;
        }
        return false;
    }

    bool Test(testerPlayer& data)
    {
        F32 fTrueForSeconds = 0.0f;
        if (m_Uniqueness && m_fTruthStartTime > 0.0f) {
            fTrueForSeconds = iTimeGetGame() - m_fTruthStartTime;
        }
        if (fTrueForSeconds > data.secondParam) {
            return true;
        }
        return false;
    }

    F32 m_fTruthStartTime;
    void* m_Uniqueness;
};

class CUniqueAmountLessThanX : public BasicInform
{
public:
    void Inform(testerPlayer&, F32 fDuration, void* uniqueness)
    {
        m_fDuration = fDuration;
        m_nUniqueAmount = (S32)uniqueness;
    }

    bool Age(F32 fHowLong)
    {
        m_fDuration -= fHowLong;
        m_fDuration = MAX(m_fDuration, 0.0f);
        return m_fDuration > 0.0f;
    }

    bool Test(testerPlayer& data)
    {
        return m_nUniqueAmount < data.firstParam;
    }

    F32 m_fDuration;
    S32 m_nUniqueAmount;
};

class CXNumUniqueWithinY : public BasicInform
{
public:
    CXNumUniqueWithinY()
    {
        m_nUniqueCount = 0;
    }

    void Inform(testerPlayer& data, F32, void* uniqueness)
    {
        S32 replaceIndex = -1;
        F32 leastTimeLeft = 999.0f;
        for (S32 i = 0; i < MAX_TRACKED; i++) {
            if (m_Uniqueness[i] == uniqueness) {
                m_fHitTimeCountdown[i] = data.secondParam;
                return;
            }
            if (m_Uniqueness[i] == NULL) {
                leastTimeLeft = 0.0f;
                replaceIndex = i;
                i = MAX_TRACKED;
            } else if (m_fHitTimeCountdown[i] < leastTimeLeft) {
                leastTimeLeft = m_fHitTimeCountdown[i];
                replaceIndex = i;
            }
        }
        if (m_Uniqueness[replaceIndex] == NULL) {
            m_nUniqueCount++;
        }
        m_fHitTimeCountdown[replaceIndex] = data.secondParam;
        m_Uniqueness[replaceIndex] = uniqueness;
    }

    bool Age(F32 fHowLong)
    {
        for (S32 i = 0; i < MAX_TRACKED; i++) {
            if (m_fHitTimeCountdown[i] > 0.0f) {
                m_fHitTimeCountdown[i] -= fHowLong;
                if (m_fHitTimeCountdown[i] <= 0.0f) {
                    m_fHitTimeCountdown[i] = 0.0f;
                    m_Uniqueness[i] = NULL;
                    m_nUniqueCount--;
                }
            }
        }
        return m_nUniqueCount > 0;
    }

    bool Test(testerPlayer& data)
    {
        return m_nUniqueCount > data.firstParam;
    }

    void* m_Uniqueness[MAX_TRACKED];
    F32 m_fHitTimeCountdown[MAX_TRACKED];
    S32 m_nUniqueCount;
};

class CHappensXTimesWithinY : public BasicInform
{
public:
    CHappensXTimesWithinY()
    {
        m_nHappenedCount = 0;
        memset(m_fHappenTimeCountdown, 0, sizeof(m_fHappenTimeCountdown));
    }

    void Inform(testerPlayer& data, F32, void*)
    {
        S32 replaceIndex = -1;
        F32 leastTimeLeft = 999.0f;
        for (S32 i = 0; i < MAX_TRACKED; i++) {
            if (m_fHappenTimeCountdown[i] < leastTimeLeft) {
                leastTimeLeft = m_fHappenTimeCountdown[i];
                replaceIndex = i;
            }
        }
        m_fHappenTimeCountdown[replaceIndex] = data.secondParam;
        m_nHappenedCount++;
        if (m_nHappenedCount > MAX_TRACKED) {
            m_nHappenedCount = MAX_TRACKED;
        }
    }

    bool Age(F32 fHowLong)
    {
        for (S32 i = 0; i < MAX_TRACKED; i++) {
            if (m_fHappenTimeCountdown[i] > 0.0f) {
                m_fHappenTimeCountdown[i] -= fHowLong;
                if (m_fHappenTimeCountdown[i] <= 0.0f) {
                    m_fHappenTimeCountdown[i] = 0.0f;
                    m_nHappenedCount--;
                }
            }
        }
        return m_nHappenedCount > 0;
    }

    bool Test(testerPlayer& data)
    {
        return m_nHappenedCount >= data.firstParam;
    }

    F32 m_fHappenTimeCountdown[MAX_TRACKED];
    S32 m_nHappenedCount;
};

class CUniqueAmountGreaterThanX : public BasicInform
{
public:
    void Inform(testerPlayer&, F32 fDuration, void* uniqueness)
    {
        m_fDuration = fDuration;
        m_nUniqueAmount = (S32)uniqueness;
    }

    bool Age(F32 fHowLong)
    {
        m_fDuration -= fHowLong;
        m_fDuration = MAX(m_fDuration, 0.0f);
        return m_fDuration > 0.0f;
    }

    bool Test(testerPlayer& data)
    {
        return m_nUniqueAmount > data.firstParam;
    }

    F32 m_fDuration;
    S32 m_nUniqueAmount;
};

bool xOneLiner::Init()
{
    switch (m_eventType) {
    case eOL_DIED:
    case eOL_DIED_FALLING:
    case eOL_ZIPLINE:
    case eOL_NEAR_LEDGE:
    case eOL_TURRET_MOUNT:
    case eOL_EVENT_5:
    case eOL_ENEMY_IN_RANGE:
    case eOL_ENEMY_OUT_OF_RANGE:
    case eOL_LIFT_LIGHT:
    case eOL_LIFT_MEDIUM:
    case eOL_LIFT_HEAVY:
    case eOL_THROW_LIGHT:
    case eOL_THROW_MEDIUM:
    case eOL_THROW_HEAVY:
    case eOL_PICKUP:
    case eOL_LEDGE_GRAB:
    case eOL_LEDGE_JUMP_UP:
    case eOL_LEDGE_JUMP_DOWN:
    case eOL_LEDGE_FLIP:
    case eOL_CHECKPOINT:
    case eOL_JUMP:
    case eOL_TRIPPED:
    case eOL_DETECTED:
    case eOL_GO_VISIBLE:
    default:
        m_pData = new (eMemStaticType_0, 0) CActiveEvent;
        return true;
    case eOL_IDLE:
    case eOL_RUNNING:
    case eOL_WALKING:
    case eOL_FALLING:
    case eOL_CARRY_SMALL:
    case eOL_CARRY_LARGE:
    case eOL_CARRY_MEDIUM:
    case eOL_ON_TURRET:
    case eOL_TURNING:
    case eOL_LEDGE_MOVE:
    case eOL_INVISIBLE:
        m_pData = new (eMemStaticType_0, 0) CTrueForYLong;
        return true;
    case eOL_ZAPPED:
    case eOL_BOOMED:
    case eOL_BURNT:
    case eOL_HIT:
    case eOL_INVISIBLE_LOW:
    case eOL_BALL_METER:
        m_pData = new (eMemStaticType_0, 0) CUniqueAmountLessThanX;
        return true;
    case eOL_STRUCK_ROBOTS:
    case eOL_STRUCK_HUMANS:
        m_pData = new (eMemStaticType_0, 0) CXNumUniqueWithinY;
        return true;
    case eOL_HIT_BY_ENEMIES:
    case eOL_HAND_HURT:
    case eOL_STRIKE_BREAKS:
        m_pData = new (eMemStaticType_0, 0) CHappensXTimesWithinY;
        return true;
    case eOL_BALL_SPEED:
    case eOL_DASH_SPEED:
    case eOL_DASH_FRICTION:
    case eOL_STOP_RUNNING:
        m_pData = new (eMemStaticType_0, 0) CUniqueAmountGreaterThanX;
        return true;
    }
}

bool xOneLiner::UpdatePlaySound(F32 currentTime, F32 fRollsPerSecond)
{
    if (m_uMaxPlays != 0 && m_uNumPlays >= m_uMaxPlays) {
        return false;
    }

    if (currentTime - m_fTimeLastPlayed < m_fDelayBetweenPlays) {
        return false;
    }

    bool play = true;
    if (m_playerType == eTESTER) {
        play = m_pData->Test(m_testerData);
    }

    bool roll;
    if (fRollsPerSecond > 0.0f && m_fLastDuration > 0.0f) {
        roll = xrand_RandomChance(m_fProbability / (m_fLastDuration * fRollsPerSecond));
    } else {
        roll = xrand_RandomChance(m_fProbability);
    }

    if (play && roll) {
        return true;
    }
    return false;
}

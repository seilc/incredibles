#ifndef RTSLERP_H
#define RTSLERP_H

#include "rtquat.h"

typedef struct RtQuatSlerpCache RtQuatSlerpCache;

struct RtQuatSlerpCache
{
    RtQuat raFrom;
    RtQuat raTo;
    RwReal omega;
    RwBool nearlyZeroOm;
};

#define RtQuatSlerpMacro(qpResult, qpFrom, qpTo, rT, sCache)                                         \
    MACRO_START                                                                                      \
    {                                                                                                \
        if ((rT) <= ((RwReal) 0))                                                                    \
        {                                                                                            \
            *(qpResult) = *(qpFrom);                                                                 \
        }                                                                                            \
        else if (((RwReal) 1) <= (rT))                                                               \
        {                                                                                            \
            *(qpResult) = *(qpTo);                                                                   \
        }                                                                                            \
        else                                                                                         \
        {                                                                                            \
            RwReal rSclFrom = ((RwReal) 1) - (rT);                                                   \
            RwReal rSclTo = (rT);                                                                    \
            if (!((sCache)->nearlyZeroOm))                                                           \
            {                                                                                        \
                rSclFrom *= (sCache)->omega;                                                         \
                RwSinMinusPiToPiMacro(rSclFrom, rSclFrom);                                           \
                rSclTo *= (sCache)->omega;                                                           \
                RwSinMinusPiToPiMacro(rSclTo, rSclTo);                                               \
            }                                                                                        \
            RwV3dScaleMacro(&(qpResult)->imag, &(sCache)->raFrom.imag, rSclFrom);                    \
            RwV3dIncrementScaledMacro(&(qpResult)->imag, &(sCache)->raTo.imag, rSclTo);              \
            (qpResult)->real = ((sCache)->raFrom.real * rSclFrom) + ((sCache)->raTo.real * rSclTo);  \
        }                                                                                            \
    }                                                                                                \
    MACRO_STOP

#ifdef __cplusplus
extern "C" {
#endif

void RtQuatSetupSlerpCache(RtQuat* qpFrom, RtQuat* qpTo, RtQuatSlerpCache* sCache);

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
void RtQuatSlerp(RtQuat* qpResult, RtQuat* qpFrom, RtQuat* qpTo, RwReal rT, RtQuatSlerpCache* sCache);
#endif

#ifdef __cplusplus
}
#endif

#if !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
#define RtQuatSlerp(qpResult, qpFrom, qpTo, rT, sCache) RtQuatSlerpMacro(qpResult, qpFrom, qpTo, rT, sCache)
#endif

#endif

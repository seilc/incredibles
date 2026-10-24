#include "xRand.h"

#include "xMath.h"
#include "xVec3.h"
#include "xMemMgr.h"
#include "iMath.h"

#include <cmath>

#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL
#define UMASK 0x80000000UL
#define LMASK 0x7fffffffUL
#define MIXBITS(u,v) ( ((u) & UMASK) | ((v) & LMASK) )
#define TWIST(u,v) ((MIXBITS(u,v) >> 1) ^ ((v)&1UL ? MATRIX_A : 0UL))

static U32 state[N];
static S32 left = 1;
static S32 initf = 0;
static U32* next;

void init_genrand(U32 s)
{
    S32 j;
    state[0] = s & 0xffffffffUL;
    for (j = 1; j < N; j++) {
        state[j] = (1812433253UL * (state[j-1] ^ (state[j-1] >> 30)) + j);
        state[j] &= 0xffffffffUL;
    }
    left = 1; initf = 1;
}

void init_by_array(U32* init_key, U32 key_length)
{
    S32 i, k;
    U32 j;
    init_genrand(19650218UL);
    i = 1; j = 0;
    k = (N > key_length ? N : key_length);
    for (; k; k--) {
        state[i] = (state[i] ^ ((state[i-1] ^ (state[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j;
        state[i] &= 0xffffffffUL;
        i++; j++;
        if (i >= N) { state[0] = state[N-1]; i = 1; }
        if (j >= key_length) j = 0;
    }
    for (k = N - 1; k; k--) {
        state[i] = (state[i] ^ ((state[i-1] ^ (state[i-1] >> 30)) * 1566083941UL))
          - i;
        state[i] &= 0xffffffffUL;
        i++;
        if (i >= N) { state[0] = state[N-1]; i = 1; }
    }

    state[0] = 0x80000000UL;
    left = 1; initf = 1;
}

void xrand_RandomInit()
{
    U32 init[4] = { 0x123, 0x234, 0x345, 0x456 }, length = 4;
    init_by_array(init, length);
}

inline void next_state()
{
    U32* p = state;
    S32 j;

    left = N; next = state;

    for (j = N - M + 1; --j; p++)
        *p = p[M] ^ TWIST(p[0], p[1]);

    for (j = M; --j; p++)
        *p = p[M-N] ^ TWIST(p[0], p[1]);

    *p = p[M-N] ^ TWIST(p[0], state[0]);
}

U32 xrand_GenRandInt32()
{
    U32 y;

    if (--left == 0) next_state();
    y = *next++;

    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

F32 xrand_GenRandFloat()
{
    U32 y;
    F32 fRandFloat;

    if (--left == 0) next_state();
    y = *next++;

    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    fRandFloat = (1.0f / 4294967296.0f) * y;
    xASSERT(178, fRandFloat >= 0.0f);
    xASSERT(179, fRandFloat <= 1.0f);

    return fRandFloat;
}

U32 xrand_RandomChoice(U32& chosenMask, U8& setBits, U32 maxCount, xRandomChoiceRule rule)
{
    U32 choice = 0;

    switch (rule) {
    case xRandomChoiceRule_ANY_EXCEPT_LAST:
        do {
            choice = xrand_RandomChoice(maxCount);
        } while (maxCount > 1 && ((1 << choice) & chosenMask));
        chosenMask = 1 << choice;
        break;
    case xRandomChoiceRule_ANY:
        choice = xrand_RandomChoice(maxCount);
        break;
    case xRandomChoiceRule_ANY_NOT_CHOSEN:
        U32 ithUnplayed = xrand_RandomChoice(maxCount - setBits) + 1;
        U32 testMask = 1;
        for (;;) {
            if (!(testMask & chosenMask)) {
                ithUnplayed--;
                if (ithUnplayed == 0) {
                    break;
                }
            }
            testMask <<= 1;
            choice++;
        }
        chosenMask |= 1 << choice;
        setBits++;
        if (setBits == maxCount) {
            if (maxCount > 1) {
                setBits = 1;
                chosenMask = 1 << choice;
            } else {
                setBits = 0;
                chosenMask = 0;
            }
        }
        break;
    case xRandomChoiceRule_IN_ORDER:
        choice = setBits;
        chosenMask |= 1 << choice;
        setBits++;
        if (setBits == maxCount) {
            setBits = 0;
            chosenMask = 0;
        }
        break;
    case xRandomChoiceRule_IN_ORDER_RANDOM:
        if (chosenMask == 0) {
            setBits = xrand_RandomChoice(maxCount);
        }
        choice = setBits;
        setBits++;
        if (setBits == maxCount) {
            setBits = 0;
        }
        break;
    }

    return choice;
}

void xRandUniformSpherePoints(xVec3* loc, S32 amount, F32 zmin, F32 zmax)
{
    xASSERT(361, (amount > 0) && (amount <= 0xffff));

    F32 z_frac = 0.5f * (zmax - zmin);
    S32 z_sectors = std::ceilf(z_frac * xsqrt(amount));
    if (z_sectors <= 0) {
        z_sectors = 1;
    }
    S32 yaw_sectors = (amount + z_sectors - 1) / z_sectors;
    S32 sectors = yaw_sectors * z_sectors;
    xASSERT(368, sectors >= amount);

    F32 dyaw = 2.0f * PI / yaw_sectors;
    F32 dz = (zmax - zmin) / z_sectors;

    struct sector_type
    {
        F32 yaw;
        F32 z;
    };

    sector_type* sector_buffer = (sector_type*)xMEMPUSHTEMP(sectors * sizeof(sector_type), 0, 378);

    sector_type* sector = sector_buffer;
    F32 sector_yaw = (2.0f * PI / 4294967296.0f) * xrand_RandomInt32();
    F32 sector_z;
    for (S32 lat_index = 0; lat_index < yaw_sectors; lat_index++) {
        sector_z = zmin;
        for (S32 long_index = 0; long_index < z_sectors; long_index++, sector++) {
            sector->yaw = sector_yaw;
            sector->z = sector_z;
            sector_z += dz;
        }
        sector_yaw += dyaw;
    }

    F32 yaw_range = (1.0f / 4294967296.0f) * dyaw;
    F32 z_range = (1.0f / 4294967296.0f) * dz;
    for (S32 i = 0; i < amount; i++, ++loc) {
        S32 rand_index = i + xrand_RandomInt32() % (sectors - i);

        sector_yaw = sector_buffer[rand_index].yaw;
        sector_z = sector_buffer[rand_index].z;
        sector_buffer[rand_index] = sector_buffer[i];

        F32 yaw = xrmod(sector_yaw + yaw_range * xrand_RandomInt32());
        F32 z = sector_z + z_range * xrand_RandomInt32();
        F32 r = xsqrt(1.0f - z * z);
        F32 x = r * icos(yaw), y = r * isin(yaw);
        loc->assign(x, y, z);
    }

    xMEMPOPTEMP(sector_buffer);
}

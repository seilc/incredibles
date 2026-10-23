#ifndef _DOLPHIN_CMATH_H_
#define _DOLPHIN_CMATH_H_

#ifdef __cplusplus
extern "C" {
#endif

float powf(float x, float y);
float tanf(float);

extern float sinf(float);
extern float cosf(float);
extern float acosf(float);
extern float atan2f(float, float);

#ifdef __cplusplus
}
#endif

#endif // _DOLPHIN_CMATH_H_

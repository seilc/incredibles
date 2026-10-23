#ifndef _MSL_MATH_H
#define _MSL_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

int __float_nan[];

#define NAN       (*(float*)      __float_nan)

int __fpclassifyf(float);
int __fpclassifyd(double);

#define FP_INFINITE 2

#define fpclassify(x) ((sizeof(x) == sizeof(float)) ? __fpclassifyf((float)(x)) : __fpclassifyd((double)(x)))
#define isinf(x) ((fpclassify(x) == FP_INFINITE))

extern double pow(double, double);
extern double cos(double);
extern double sin(double);
extern double sqrt(double);
extern double ceil(double);

extern float cosf(float);
extern float sinf(float);
extern float fabsf(float);
extern float powf(float, float);
extern float sqrtf(float);

inline float cosf(float x) { return (float)cos((double)x); }
inline float sinf(float x) { return (float)sin((double)x); }
inline float powf(float x, float y) { return (float)pow((double)x, (double)y); }

inline float sqrtf(float x)
{
	static const double _half=.5f;
	static const double _three=3.0f;

	if(x > 0.0f)
	{
		double xd = (double)x;
		double guess = __frsqrte(xd);		  		 	/* returns an approximation to	*/
		guess = _half*guess*(_three - guess*guess*xd);	/* now have 12 sig bits 		*/
		guess = _half*guess*(_three - guess*guess*xd);	/* now have 24 sig bits			*/
		return (float)(xd * guess);
	}
	else if (x < 0.0)
		return NAN;
	else
		return x;
}     

extern inline double sqrt(double x)
{

	if(x > 0.0)
	{
		double guess = __frsqrte(x);   				/* returns an approximation to	*/
		guess = .5*guess*(3.0 - guess*guess*x);  	/* now have 8 sig bits			*/
		guess = .5*guess*(3.0 - guess*guess*x);  	/* now have 16 sig bits			*/
		guess = .5*guess*(3.0 - guess*guess*x);  	/* now have 32 sig bits			*/
		guess = .5*guess*(3.0 - guess*guess*x);  	/* now have > 53 sig bits		*/
		return x*guess ;
	}
	else if (x < 0.0)
		return NAN;
	else
		return x;
}

#ifdef __cplusplus
}
#endif

#endif

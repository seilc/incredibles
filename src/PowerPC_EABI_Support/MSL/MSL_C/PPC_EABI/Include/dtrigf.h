#ifndef __DTRIGF_H__
#define __DTRIGF_H__

inline float sinf_a(float x);
inline float sinf_f(float x);
inline float cosf_a(float x);
inline float cosf_f(float x);
inline __vec2x32float__ sincosf_a(__vec2x32float__ x);
inline __vec2x32float__ sincosf_f(__vec2x32float__ x);

#pragma fp_contract on

#define __PIO2__ 1.57079632679489661923132169163975f
#define __PI 3.1415926535897932384626433832795f
#define __3PIO2__ 3.0f * 1.57079632679489661923132169163975f

#define FTYPE float

#define sine_core_series_fast(x)\
    asm\
    {\
        fmuls square_x, x, x;\
        fmsubs result, result, square_x, const1;\
        fmadds result, result, square_x, const2;\
        fmuls result, result, x;\
    }

#define sine_core_series(x)\
    asm\
    {\
        fmuls square_x, x, x;\
        fmadds result, result, square_x, const1;\
        fmsubs result, result, square_x, const2;\
        fmadds result, result, square_x, const3;\
        fmsubs result, result, square_x, const4;\
        fmadds result, result, square_x, const5;\
        fmuls result, result, x;\
    }

#define cosine_core_series_fast(x)\
    asm\
    {\
        fmuls square_x, x, x;\
        fmsubs result, result, square_x, const1;\
        fmadds result, result, square_x, const2;\
    }

#define cosine_core_series(x)\
    asm\
    {\
        fmuls square_x, x, x;\
        fmadds result, result, square_x, const1;\
        fmsubs result, result, square_x, const2;\
        fmadds result, result, square_x, const3;\
        fmsubs result, result, square_x, const4;\
        fmadds result, result, square_x, const5;\
    }

#if __PPCGEKKO__

#define vector_core_series_fast(x)\
    ps_mul square_x, x, x;\
    ps_msub result, result, square_x, const1;\
    ps_madd tempf, result, square_x, const2;\
    ps_mul result, tempf, x;\
    ps_merge01 result, result, tempf

#define vector_core_series(x)\
    ps_mul square_x, x, x;\
    ps_madd result, result, square_x, const1;\
    ps_msub result, result, square_x, const2;\
    ps_madd result, result, square_x, const3;\
    ps_msub result, result, square_x, const4;\
    ps_madd tempf, result, square_x, const5;\
    ps_mul result, tempf, x;\
    ps_merge01 result, result, tempf

#endif

#define MAP_TO_FIRST_QUAD(core, I, II, III, IV)\
    if (x <= __PI) {\
        if (x <= __PIO2__) {\
            core;\
            return I##result;\
        } else {\
            x = __PIO2__ - (x - __PIO2__);\
            core;\
            return II##result;\
        }\
    } else {\
        if (x < 3.0f * __PIO2__) {\
            x = x - __PI;\
            core;\
            return III##result;\
        } else {\
            x = (__PIO2__) - (x - 3.0f * __PIO2__);\
            core;\
            return IV##result;\
        }\
    }

float sinf_a(register float x)
{
    register FTYPE square_x;
    register FTYPE result = -2.39e-08f;
    register FTYPE const1 = 2.7526e-06f;
    register FTYPE const2 = 1.98409e-04f;
    register FTYPE const3 = 8.3333315e-03f;
    register FTYPE const4 = 1.666666664e-01f;
    register FTYPE const5 = 1.0f;

    MAP_TO_FIRST_QUAD(sine_core_series(x), +, +, -, -);
}

float sinf_f(register float x)
{
    register FTYPE square_x;
    register FTYPE result = 7.61E-03f;
    register FTYPE const1 = 1.6605E-01f;
    register FTYPE const2 = 1.0f;

    MAP_TO_FIRST_QUAD(sine_core_series_fast(x), +, +, -, -);
}

float cosf_a(register float x)
{
    register FTYPE square_x;
    register FTYPE result = -2.605e-07f;
    register FTYPE const1 = 2.47609e-05f;
    register FTYPE const2 = 1.3888397e-03f;
    register FTYPE const3 = 4.16666418e-02f;
    register FTYPE const4 = 4.999999963e-01f;
    register FTYPE const5 = 1.0f;

    MAP_TO_FIRST_QUAD(cosine_core_series(x), +, -, -, +);
}

float cosf_f(register float x)
{
    register FTYPE square_x;
    register FTYPE result = 3.705E-02f;
    register FTYPE const1 = 4.967E-01f;
    register FTYPE const2 = 1.0f;

    MAP_TO_FIRST_QUAD(cosine_core_series_fast(x), +, -, -, +);
}

#if __PPCGEKKO__

__vec2x32float__ sincosf_f(register __vec2x32float__ x)
{
    const long long one_negone = 0x3f800000bf800000;
    const long long negone_one = 0xbf8000003f800000;
    const long long pi_po2 = 0x40490fdb3fc90fdb;
    const long long sincos_result = 0x3bf95d4f3d17c1be;
    const long long sincos_const1 = 0x3e2a09033efe4f76;

    register const long long* const pone_negone = &one_negone;
    register const long long* const pnegone_one = &negone_one;
    register const long long* const ppi_po2 = &pi_po2;
    register const long long* const psincos_result = &sincos_result;
    register const long long* const psincos_const1 = &sincos_const1;

    register __vec2x32float__ square_x;
    register __vec2x32float__ result;
    register __vec2x32float__ const1;
    register __vec2x32float__ const2;
    register __vec2x32float__ tempf;
    register __vec2x32float__ tempf2;

    const FTYPE one = 1.0f;
    const FTYPE neg_one = -1.0f;
    const FTYPE __local_PI = __PI;
    const FTYPE __local_PIO2 = __PIO2__;
    const FTYPE __local_3PIO2 = __3PIO2__;

    asm
    {
        psq_l result, 0(psincos_result), 0, 0
        psq_l const1, 0(psincos_const1), 0, 0
        lfs const2, one
        psq_l tempf, 0(ppi_po2), 0, 0
        ps_cmpu0 cr0, x, tempf
        bgt cr0, GT_PI
        ps_cmpu1 cr1, x, tempf
        bgt cr1, QUAD_II
        vector_core_series_fast(x);
        b finish
QUAD_II:
        lfs tempf, __local_PIO2
        ps_sub tempf2, x, tempf
        ps_sub x, tempf, tempf2
        vector_core_series_fast(x);
        la r7, one_negone
        psq_l const1, 0(r7), 0, 0
        ps_mul result, result, const1
        b finish
GT_PI:
        lfs tempf, __local_3PIO2
        ps_cmpu0 cr0, x, tempf
        bgt QUAD_IV
        lfs tempf, __local_PI
        ps_sub x, x, tempf
        vector_core_series_fast(x);
        lfs const1, neg_one
        ps_mul result, result, const1
        b finish
QUAD_IV:
        lfs tempf, __local_3PIO2
        ps_sub tempf2, x, tempf
        lfs tempf, __local_PIO2
        ps_sub x, tempf, tempf2
        vector_core_series_fast(x);
        la r7, negone_one
        psq_l const1, 0(r7), 0, 0
        ps_mul result, result, const1
finish:
    }

    return result;
}

__vec2x32float__ sincosf_a(register __vec2x32float__ x)
{
    const long long one_negone = 0x3f800000bf800000;
    const long long negone_one = 0xbf8000003f800000;
    const long long pi_po2 = 0x40490fdb3fc90fdb;
    const long long sincos_result = 0xb2cd4ca8b48bdad9;
    const long long sincos_const1 = 0x3638b94f37cfb5a1;
    const long long sincos_const2 = 0x39500c033ab609ba;
    const long long sincos_const3 = 0x3c0888873d2aaaa4;
    const long long sincos_const4 = 0x3e2aaaab3f000000;

    register const long long* const pone_negone = &one_negone;
    register const long long* const pnegone_one = &negone_one;
    register const long long* const ppi_po2 = &pi_po2;
    register const long long* const psincos_result = &sincos_result;
    register const long long* const psincos_const1 = &sincos_const1;
    register const long long* const psincos_const2 = &sincos_const2;
    register const long long* const psincos_const3 = &sincos_const3;
    register const long long* const psincos_const4 = &sincos_const4;

    register __vec2x32float__ square_x;
    register __vec2x32float__ result;
    register __vec2x32float__ const1;
    register __vec2x32float__ const2;
    register __vec2x32float__ const3;
    register __vec2x32float__ const4;
    register __vec2x32float__ const5;
    register __vec2x32float__ tempf;
    register __vec2x32float__ tempf2;

    const FTYPE one = 1.0f;
    const FTYPE neg_one = -1.0f;
    const FTYPE __local_PI = __PI;
    const FTYPE __local_PIO2 = __PIO2__;
    const FTYPE __local_3PIO2 = __3PIO2__;

    asm
    {
        psq_l result, 0(psincos_result), 0, 0
        psq_l const1, 0(psincos_const1), 0, 0
        psq_l const2, 0(psincos_const2), 0, 0
        psq_l const3, 0(psincos_const3), 0, 0
        psq_l const4, 0(psincos_const4), 0, 0
        lfs const5, one
        psq_l tempf, 0(ppi_po2), 0, 0
        ps_cmpu0 cr0, x, tempf
        bgt cr0, GT_PI
        ps_cmpu1 cr1, x, tempf
        bgt cr1, QUAD_II
        vector_core_series(x);
        b finish
QUAD_II:
        lfs tempf, __local_PIO2
        ps_sub tempf2, x, tempf
        ps_sub x, tempf, tempf2
        vector_core_series(x);
        la r7, one_negone
        psq_l const1, 0(r7), 0, 0
        ps_mul result, result, const1
        b finish
GT_PI:
        lfs tempf, __local_3PIO2
        ps_cmpu0 cr0, x, tempf
        bgt QUAD_IV
        lfs tempf, __local_PI
        ps_sub x, x, tempf
        vector_core_series(x);
        lfs const1, neg_one
        ps_mul result, result, const1
        b finish
QUAD_IV:
        lfs tempf, __local_3PIO2
        ps_sub tempf2, x, tempf
        lfs tempf, __local_PIO2
        ps_sub x, tempf, tempf2
        vector_core_series(x);
        la r7, negone_one
        psq_l const1, 0(r7), 0, 0
        ps_mul result, result, const1
finish:
    }

    return result;
}

#endif

#pragma fp_contract reset

#endif

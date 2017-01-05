/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    This work is based on "SLEEF" library and converted to use MANGO SIMD abstraction
    Author : Naoki Shibata
*/
#define _USE_MATH_DEFINES /* TODO: required for M_PI, MPI_2, etc. rewrite to not require this */
#include <mango/simd/simd.hpp>

namespace mango {
namespace simd {

#ifdef MANGO_SIMD_FLOAT

    // ------------------------------------------------------------------------
    // Sleef: simd4f
    // ------------------------------------------------------------------------

    #define PI4_Af 0.78515625f
    #define PI4_Bf 0.00024187564849853515625f
    #define PI4_Cf 3.7747668102383613586e-08f
    #define PI4_Df 1.2816720341285448015e-12f
    #define L2Uf 0.693145751953125f
    #define L2Lf 1.428606765330187045e-06f
    #define R_LN2f 1.442695040888963407359924681001892137426645954152985934135449406931f
    #define R_INFf float(std::numeric_limits<float>::infinity())
    #define R_1_PIf 0.31830988618379067154f

    static inline simd4f signbit(__simd4f f)
    {
        return simd4f_and(f, simd4f_set1(-0.0f));
    }

    static inline simd4f mulsign(__simd4f x, __simd4f y)
    {
        return simd4f_xor(x, signbit(y));
    }

    static inline simd4f sign(__simd4f f)
    {
        return simd4f_or(simd4f_set1(1.0f), simd4f_and(simd4f_set1(-0.0f), f));
    }

    static inline simd4f is_nan(__simd4f d)
    {
        return simd4f_compare_neq(d, d);
    }

    static inline simd4f is_inf(__simd4f d)
    {
        return simd4f_compare_eq(simd4f_abs(d), R_INFf);
    }

    static inline simd4f is_inf2(__simd4f d, __simd4f m)
    {
        return simd4f_and(is_inf(d), simd4f_or(signbit(d), m));
    }

    static inline simd4f is_negative_inf(__simd4f d)
    {
        return simd4f_compare_eq(d, -R_INFf);
    }

    static inline simd4i sel(__simd4f f0, __simd4f f1, __simd4i x, __simd4i y)
    {
        simd4f m = simd4f_compare_lt(f0, f1);
        return simd4i_select(simd4i_cast(m), x, y);
    }

    static inline simd4f atan2kf(__simd4f b, __simd4f a)
    {
        simd4i q = sel(a, simd4f_set1(0.0f), simd4i_set1(-2), simd4i_set1(0));
        simd4f x = simd4f_abs(a);
        simd4f y = b;

        q = sel(x, y, simd4i_add(q, simd4i_set1(1)), q);
        simd4f p = simd4f_compare_lt(x, y);
        simd4f s = simd4f_select(p, simd4f_neg(x), y);
        simd4f t = simd4f_max(x, y);

        s = simd4f_div(s, t);
        t = simd4f_mul(s, s);

        simd4f u = simd4f_set1(0.00282363896258175373077393f);
        u = simd4f_madd(-0.0159569028764963150024414f, u, t);
        u = simd4f_madd( 0.0425049886107444763183594f, u, t);
        u = simd4f_madd(-0.0748900920152664184570312f, u, t);
        u = simd4f_madd( 0.106347933411598205566406f, u, t);
        u = simd4f_madd(-0.142027363181114196777344f, u, t);
        u = simd4f_madd( 0.199926957488059997558594f, u, t);
        u = simd4f_madd(-0.333331018686294555664062f, u, t);

        t = simd4f_madd(s, s, simd4f_mul(t, u));
        t = simd4f_madd(t, simd4f_convert(q), simd4f_set1(float(M_PI/2)));

        return t;
    }

    static inline simd4f ldexp(__simd4f x, __simd4i q)
    {
        simd4f u;
        simd4i m = simd4i_sra(q, 31);
        m = simd4i_sll(simd4i_sub(simd4i_sra(simd4i_add(m, q), 6), m), 4);
        const simd4i p = simd4i_sub(q, simd4i_sll(m, 2));
        m = simd4i_add(m, 0x7f);
        m = simd4i_and(simd4i_compare_gt(m, simd4i_zero()), m);
        const simd4i n = simd4i_compare_gt(m, 0xff);
        m = simd4i_or(simd4i_nand(n, m), simd4i_and(n, 0xff));
        u = simd4f_cast(simd4i_sll(m, 23));
        const simd4f y = simd4f_mul(simd4f_mul(simd4f_mul(simd4f_mul(x, u), u), u), u);
        u = simd4f_cast(simd4i_sll(simd4i_add(p, 0x7f), 23));
        return simd4f_mul(y, u);
    }

    static inline simd4f sincos_post(__simd4f s)
    {
        simd4f u;
        u = simd4f_set1(2.6083159809786593541503e-06f);
        u = simd4f_madd(-0.0001981069071916863322258f, u, s);
        u = simd4f_madd(0.00833307858556509017944336f, u, s);
        u = simd4f_madd(-0.166666597127914428710938f, u, s);
        return u;
    }

    simd4f simd4f_sin(__simd4f v)
    {
        simd4f d = v;
        simd4i q = simd4i_convert(simd4f_mul(v, R_1_PIf));
        simd4f u = simd4f_convert(q);

        d = simd4f_madd(d, u, -4.0f * PI4_Af);
        d = simd4f_madd(d, u, -4.0f * PI4_Bf);
        d = simd4f_madd(d, u, -4.0f * PI4_Cf);
        d = simd4f_madd(d, u, -4.0f * PI4_Df);

        const simd4i one = simd4i_set1(1);
        const simd4i q1 = simd4i_and(q, one);
        const simd4f mask = simd4f_cast(simd4i_compare_eq(q1, one));
        d = simd4f_xor(simd4f_and(mask, simd4f_set1(-0.0f)), d);

        const simd4f s = simd4f_mul(d, d);
        u = sincos_post(s);
        u = simd4f_madd(d, s, simd4f_mul(u, d));
        u = simd4f_or(is_inf(d), u);
        return u;
    }

    simd4f simd4f_cos(__simd4f v)
    {
        simd4f d = v;
        simd4i q;
        q = simd4i_convert(simd4f_madd(-0.5f, d, simd4f_set1(R_1_PIf)));
        q = simd4i_add(simd4i_add(q, q), 1);
        simd4f u = simd4f_convert(q);

        d = simd4f_madd(d, u, -2.0f * PI4_Af);
        d = simd4f_madd(d, u, -2.0f * PI4_Bf);
        d = simd4f_madd(d, u, -2.0f * PI4_Cf);
        d = simd4f_madd(d, u, -2.0f * PI4_Df);

        const simd4i q2 = simd4i_and(q, 2);
        const simd4f mask = simd4f_cast(simd4i_compare_eq(q2, simd4i_zero()));
        d = simd4f_xor(simd4f_and(mask, simd4f_set1(-0.0f)), d);

        const simd4f s = simd4f_mul(d, d);
        u = sincos_post(s);
        u = simd4f_madd(d, s, simd4f_mul(u, d));
        u = simd4f_or(is_inf(d), u);
        return u;
    }

    simd4f simd4f_tan(__simd4f d)
    {
        const simd4i q = simd4i_convert(simd4f_mul(d, simd4f_set1(2 * R_1_PIf)));
        simd4f u = simd4f_convert(q);

        simd4f x = d;
        x = simd4f_madd(x, u, -2.0f * PI4_Af);
        x = simd4f_madd(x, u, -2.0f * PI4_Bf);
        x = simd4f_madd(x, u, -2.0f * PI4_Cf);
        x = simd4f_madd(x, u, -2.0f * PI4_Df);

        const simd4f s = simd4f_mul(x, x);
        const simd4i m = simd4i_compare_eq(simd4i_and(q, simd4i_set1(1)), simd4i_set1(1));
        const simd4f mask = simd4f_cast(m);
        x = simd4f_xor(simd4f_and(mask, simd4f_set1(-0.0)), x);

        u = simd4f_set1(0.00927245803177356719970703f);
        u = simd4f_madd(0.00331984995864331722259521f, u, s);
        u = simd4f_madd(0.0242998078465461730957031f, u, s);
        u = simd4f_madd(0.0534495301544666290283203f, u, s);
        u = simd4f_madd(0.133383005857467651367188f, u, s);
        u = simd4f_madd(0.333331853151321411132812f, u, s);

        u = simd4f_madd(x, s, simd4f_mul(u, x));
        u = simd4f_select(mask, simd4f_div(simd4f_set1(1.0f), u), u);
        u = simd4f_or(is_inf(d), u);

        return u;
    }

    simd4f simd4f_exp(__simd4f v)
    {
        const simd4i q = simd4i_convert(simd4f_mul(v, R_LN2f));
        const simd4f p = simd4f_convert(q);

        simd4f s;
        s = simd4f_madd(v, p, -L2Uf);
        s = simd4f_madd(s, p, -L2Lf);

        simd4f u = simd4f_set1(0.00136324646882712841033936f);
        u = simd4f_madd(0.00836596917361021041870117f, u, s);
        u = simd4f_madd(0.0416710823774337768554688f, u, s);
        u = simd4f_madd(0.166665524244308471679688f, u, s);
        u = simd4f_madd(0.499999850988388061523438f, u, s);

        u = simd4f_add(1.0f, simd4f_madd(s, simd4f_mul(s, s), u));
        u = ldexp(u, q);
        u = simd4f_nand(is_negative_inf(v), u);

        return u;
    }

    simd4f simd4f_log2(__simd4f v)
    {
        const simd4i exponent = simd4i_sub(simd4i_srl(simd4i_and(simd4i_cast(v), 0x7fffffff), 23), 127);
        const simd4f x = simd4f_sub(simd4f_cast(simd4i_sub(simd4i_cast(v), simd4i_sll(exponent, 23))), 1.0f);
        const simd4f x2 = simd4f_mul(x, x);
        const simd4f x4 = simd4f_mul(x2, x2);
        simd4f hi = simd4f_set1(-0.00931049621349f);
        simd4f lo = simd4f_set1( 0.47868480909345f);
        hi = simd4f_madd( 0.05206469089414f, x, hi);
        lo = simd4f_madd(-0.72116591947498f, x, lo);
        hi = simd4f_madd(-0.13753123777116f, x, hi);
        hi = simd4f_madd( 0.24187369696082f, x, hi);
        hi = simd4f_madd(-0.34730547155299f, x, hi);
        lo = simd4f_madd(1.442689881667200f, x, lo);
        const simd4f u = simd4f_madd(simd4f_convert(exponent), x, lo);
        return simd4f_madd(u, x4, hi);
    }

    simd4f simd4f_log(__simd4f v)
    {
        return simd4f_mul(simd4f_log2(v), 0.69314718055995f);
    }

    simd4f simd4f_exp2(__simd4f v)
    {
        const simd4i ix = simd4i_truncate(simd4f_add(v, simd4f_cast(simd4i_nand(simd4i_sra(simd4i_convert(v), 31), 0x3f7fffff))));
        simd4f f = simd4f_mul(simd4f_sub(simd4f_convert(ix), v), 0.69314718055994530942f);
        simd4f hi = simd4f_madd(0.0013298820f, f, simd4f_set1(-0.0001413161f));
        simd4f lo = simd4f_madd(0.4999999206f, f, simd4f_set1(-0.1666653019f));
        hi = simd4f_madd(-0.0083013598f, f, hi);
        hi = simd4f_madd(0.0416573475f, f, hi);
        lo = simd4f_madd(-0.9999999995f, f, lo);
        lo = simd4f_madd(1.0f, f, lo);
        simd4f f2 = simd4f_mul(f, f);
        simd4f a = simd4f_add(simd4f_mul(simd4f_mul(f2, f2), hi), lo);
        simd4f b = simd4f_cast(simd4i_and(simd4i_sll((simd4i_add(ix, 127)), 23), simd4i_compare_gt(ix, -128)));
        return simd4f_or(simd4f_mul(a, b), simd4f_cast(simd4i_srl(simd4i_compare_gt(ix, 128), 1)));
    }

    simd4f simd4f_pow(__simd4f a, __simd4f b)
    {
        return simd4f_exp2(simd4f_mul(simd4f_log2(simd4f_abs(a)), b));
    }

    simd4f simd4f_asin(__simd4f d)
    {
        const simd4f one = simd4f_set1(1.0f);
        simd4f x, y;
        x = simd4f_add(one, d);
        y = simd4f_sub(one, d);
        x = simd4f_mul(x, y);
        x = simd4f_sqrt(x);

        x = simd4f_or(is_nan(x), atan2kf(simd4f_abs(d), x));
        return mulsign(x, d);
    }

    simd4f simd4f_acos(__simd4f d)
    {
        const simd4f one = simd4f_set1(1.0f);
        simd4f x, y;
        x = simd4f_add(one, d);
        y = simd4f_sub(one, d);
        x = simd4f_mul(x, y);
        x = simd4f_sqrt(x);

#if defined(MANGO_COMPILER_CLANG) && defined(MANGO_CPU_ARM)
        // internal compiler error w/ clang 3.6 build for arm neon
        simd4f mask = simd4f_set1(0x7fffffff);
        simd4f absd = simd4f_and(d, mask);
#else
        simd4f absd = simd4f_abs(d);
#endif

        x = mulsign(atan2kf(x, absd), d);
        y = simd4f_and(simd4f_compare_lt(d, simd4f_zero()), simd4f_set1(float(M_PI)));
        x = simd4f_add(x, y);
        return x;
    }

    simd4f simd4f_atan(__simd4f d)
    {
        const simd4f one = simd4f_set1(1.0f);

        simd4i q = sel(d, simd4f_set1(0.0f), simd4i_set1(2), simd4i_set1(0));
        simd4f s = simd4f_abs(d);

        q = sel(simd4f_set1(1.0f), s, simd4i_add(q, simd4i_set1(1)), q);
        s = simd4f_select(simd4f_compare_lt(one, s), simd4f_reciprocal(s), s);

        simd4f t = simd4f_mul(s, s);

        simd4f u = simd4f_set1(0.00282363896258175373077393f);
        u = simd4f_madd(-0.0159569028764963150024414f, u, t);
        u = simd4f_madd( 0.0425049886107444763183594f, u, t);
        u = simd4f_madd(-0.0748900920152664184570312f, u, t);
        u = simd4f_madd( 0.106347933411598205566406f, u, t);
        u = simd4f_madd(-0.142027363181114196777344f, u, t);
        u = simd4f_madd( 0.199926957488059997558594f, u, t);
        u = simd4f_madd(-0.333331018686294555664062f, u, t);

        t = simd4f_madd(s, s, simd4f_mul(t, u));

        simd4i m;
        m = simd4i_compare_eq(simd4i_and(q, simd4i_set1(1)), simd4i_set1(1));
        t = simd4f_select(simd4f_cast(m), simd4f_sub(simd4f_set1(float(M_PI/2)), t), t);

        m = simd4i_compare_eq(simd4i_and(q, simd4i_set1(2)), simd4i_set1(2));
        t = simd4f_cast(simd4i_xor(simd4i_and(m, simd4i_cast(simd4f_set1(-0.0f))), simd4i_cast(t)));

        return t;
    }

    simd4f simd4f_atan2(__simd4f y, __simd4f x)
    {
        static const simd4f pi = simd4f_set1(float(M_PI));
        static const simd4f pi_2 = simd4f_set1(float(M_PI_2));
        static const simd4f pi_4 = simd4f_set1(float(M_PI_4));

        simd4f r = atan2kf(simd4f_abs(y), x);
        r = mulsign(r, x);

        r = simd4f_select(simd4f_or(is_inf(x), simd4f_compare_eq(x, simd4f_zero())),
                          simd4f_sub(pi_2, is_inf2(x, mulsign(pi_2, x))), r);
        r = simd4f_select(is_inf(y), 
                          simd4f_sub(pi_2, is_inf2(x, mulsign(pi_4, x))), r);
        r = simd4f_select(simd4f_compare_eq(y, simd4f_zero()),
                          simd4f_and(simd4f_compare_eq(sign(x), simd4f_set1(-1.0f)), pi), r);
        r = simd4f_or(simd4f_or(is_nan(x), is_nan(y)), mulsign(r, y));
        return r;
    }

#else

    // ------------------------------------------------------------------
    // FPU Emulation: simd4f
    // ------------------------------------------------------------------

    simd4f simd4f_sin(__simd4f a)
    {
        simd4f v;
        v.x = std::sin(a.x);
        v.y = std::sin(a.y);
        v.z = std::sin(a.z);
        v.w = std::sin(a.w);
        return v;
    }

    simd4f simd4f_cos(__simd4f a)
    {
        simd4f v;
        v.x = std::cos(a.x);
        v.y = std::cos(a.y);
        v.z = std::cos(a.z);
        v.w = std::cos(a.w);
        return v;
    }

    simd4f simd4f_tan(__simd4f a)
    {
        simd4f v;
        v.x = std::tan(a.x);
        v.y = std::tan(a.y);
        v.z = std::tan(a.z);
        v.w = std::tan(a.w);
        return v;
    }

    simd4f simd4f_exp(__simd4f a)
    {
        simd4f v;
        v.x = std::exp(a.x);
        v.y = std::exp(a.y);
        v.z = std::exp(a.z);
        v.w = std::exp(a.w);
        return v;
    }

    simd4f simd4f_log(__simd4f a)
    {
        simd4f v;
        v.x = std::log(a.x);
        v.y = std::log(a.y);
        v.z = std::log(a.z);
        v.w = std::log(a.w);
        return v;
    }

    simd4f simd4f_log2(__simd4f a)
    {
        simd4f v;
        v.x = std::log2(a.x);
        v.y = std::log2(a.y);
        v.z = std::log2(a.z);
        v.w = std::log2(a.w);
        return v;
    }

    simd4f simd4f_exp2(__simd4f a)
    {
        simd4f v;
        v.x = std::exp2(a.x);
        v.y = std::exp2(a.y);
        v.z = std::exp2(a.z);
        v.w = std::exp2(a.w);
        return v;
    }

    simd4f simd4f_pow(__simd4f a, __simd4f b)
    {
        simd4f v;
        v.x = std::pow(a.x, b.x);
        v.y = std::pow(a.y, b.y);
        v.z = std::pow(a.z, b.z);
        v.w = std::pow(a.w, b.w);
        return v;
    }

    simd4f simd4f_asin(__simd4f a)
    {
        simd4f v;
        v.x = std::asin(a.x);
        v.y = std::asin(a.y);
        v.z = std::asin(a.z);
        v.w = std::asin(a.w);
        return v;
    }

    simd4f simd4f_acos(__simd4f a)
    {
        simd4f v;
        v.x = std::acos(a.x);
        v.y = std::acos(a.y);
        v.z = std::acos(a.z);
        v.w = std::acos(a.w);
        return v;
    }

    simd4f simd4f_atan(__simd4f a)
    {
        simd4f v;
        v.x = std::atan(a.x);
        v.y = std::atan(a.y);
        v.z = std::atan(a.z);
        v.w = std::atan(a.w);
        return v;
    }

    simd4f simd4f_atan2(__simd4f a, __simd4f b)
    {
        simd4f v;
        v.x = std::atan2(a.x, b.x);
        v.y = std::atan2(a.y, b.y);
        v.z = std::atan2(a.z, b.z);
        v.w = std::atan2(a.w, b.w);
        return v;
    }

#endif

#ifdef MANGO_SIMD_DOUBLE

    // ------------------------------------------------------------------------
    // Sleef: simd4d
    // ------------------------------------------------------------------------

    #define PI4_A 0.78539816290140151978
    #define PI4_B 4.9604678871439933374e-10
    #define PI4_C 1.1258708853173288931e-18
    #define PI4_D 1.7607799325916000908e-27

    #define L2U .69314718055966295651160180568695068359375
    #define L2L .28235290563031577122588448175013436025525412068e-12
    #define R_LN2 1.442695040888963407359924681001892137426645954152985934135449406931
    #define R_INF double(std::numeric_limits<double>::infinity())

    static inline simd4d signbit(__simd4d v)
    {
        return simd4d_and(v, simd4d_set1(-0.0));
    }

    static inline simd4d mulsign(__simd4d x, __simd4d y)
    {
        return simd4d_xor(x, signbit(y));
    }

    static inline simd4d sign(__simd4d v)
    {
        return simd4d_or(simd4d_set1(1.0), simd4d_and(simd4d_set1(-0.0), v));
    }

    static inline simd4d is_nan(__simd4d d)
    {
        return simd4d_compare_neq(d, d);
    }

    static inline simd4d is_inf(__simd4d d)
    {
        return simd4d_compare_eq(simd4d_abs(d), R_INF);
    }

    static inline simd4d is_inf2(__simd4d d, __simd4d m)
    {
        return simd4d_and(is_inf(d), simd4d_or(signbit(d), m));
    }

#if 0
    static inline simd4d is_negative_inf(__simd4d d)
    {
        return simd4d_compare_eq(d, -R_INF);
    }
#endif

    static inline simd4i sel(__simd4d f0, __simd4d f1, __simd4i x, __simd4i y)
    {
        const simd4d mask = simd4d_compare_lt(f0, f1);
        const simd4d xd = simd4d_convert(x);
        const simd4d yd = simd4d_convert(y);
        const simd4d s = simd4d_select(mask, xd, yd);
        return simd4i_convert(s);
    }

    static inline simd4d atan2k(__simd4d b, __simd4d a)
    {
        simd4i q = sel(a, simd4d_zero(), simd4i_set1(-2), simd4i_zero());
        simd4d x = simd4d_abs(a);
        simd4d y = b;

        q = sel(x, y, simd4i_add(q, simd4i_set1(1)), q);
        simd4d p = simd4d_compare_lt(x, y);
        simd4d s = simd4d_select(p, simd4d_neg(x), y);
        simd4d t = simd4d_max(x, y);

        s = simd4d_div(s, t);
        t = simd4d_mul(s, s);

        simd4d u = simd4d_set1(-1.88796008463073496563746e-05);
        u = simd4d_madd(0.000209850076645816976906797, u, t);
        u = simd4d_madd(-0.00110611831486672482563471, u, t);
        u = simd4d_madd(0.00370026744188713119232403, u, t);
        u = simd4d_madd(-0.00889896195887655491740809, u, t);
        u = simd4d_madd(0.016599329773529201970117, u, t);
        u = simd4d_madd(-0.0254517624932312641616861, u, t);
        u = simd4d_madd(0.0337852580001353069993897, u, t);
        u = simd4d_madd(-0.0407629191276836500001934, u, t);
        u = simd4d_madd(0.0466667150077840625632675, u, t);
        u = simd4d_madd(-0.0523674852303482457616113, u, t);
        u = simd4d_madd(0.0587666392926673580854313, u, t);
        u = simd4d_madd(-0.0666573579361080525984562, u, t);
        u = simd4d_madd(0.0769219538311769618355029, u, t);
        u = simd4d_madd(-0.090908995008245008229153, u, t);
        u = simd4d_madd(0.111111105648261418443745, u, t);
        u = simd4d_madd(-0.14285714266771329383765, u, t);
        u = simd4d_madd(0.199999999996591265594148, u, t);
        u = simd4d_madd(-0.333333333333311110369124, u, t);

        t = simd4d_madd(s, s, simd4d_mul(t, u));
        t = simd4d_madd(t, simd4d_convert(q), simd4d_set1(M_PI/2));

        return t;
    }

#if 0
    static inline simd4d ldexp(__simd4d x, simd4i q)
    {
        simd4i m = simd4i_sra(q, 31);
        m = simd4i_sll(simd4i_sub(simd4i_sra(simd4i_add(m ,q), 9), m), 7);
        q = simd4i_sub(q, simd4i_sll(m, 2));
        m = simd4i_add(m, simd4i_set4(0x3fff, 0x3ff, 0x0, 0x0));
        m = simd4i_nand(m, simd4i_compare_gt(simd4i_zero(), m));
        simd4i n = simd4i_compare_gt(m, simd4i_set4(0x7ff, 0x7ff, 0x0, 0x0));
        m = simd4i_or(simd4i_nand(n, m), simd4i_and(n, simd4i_set4(0x7ff, 0x7ff, 0x0, 0x0)));
        //m = (__m128i)_mm_shuffle_ps((__m128)m, (__m128)m, _MM_SHUFFLE(1,3,0,3));
        //vdouble y = (__m128d)_mm_slli_epi32(m, 20);
        //return vmul_vd_vd_vd(vmul_vd_vd_vd(vmul_vd_vd_vd(vmul_vd_vd_vd(vmul_vd_vd_vd(x, y), y), y), y), vpow2i_vd_vi(q));
        return x; // TODO
    }
#endif

    static inline simd4d sincos_post(__simd4d s)
    {
        simd4d u;
        u = simd4d_set1(-7.97255955009037868891952e-18);
        u = simd4d_madd(2.81009972710863200091251e-15, u, s);
        u = simd4d_madd(-7.64712219118158833288484e-13, u, s);
        u = simd4d_madd(1.60590430605664501629054e-10, u, s);
        u = simd4d_madd(-2.50521083763502045810755e-08, u, s);
        u = simd4d_madd(2.75573192239198747630416e-06, u, s);
        u = simd4d_madd(-0.000198412698412696162806809, u, s);
        u = simd4d_madd(0.00833333333333332974823815, u, s);
        u = simd4d_madd(-0.166666666666666657414808, u, s);
        return u;
    }

    simd4d simd4d_sin(__simd4d v)
    {
        simd4d d = v;

        simd4i q = simd4i_convert(simd4d_mul(d, M_1_PI));
        simd4d u = simd4d_convert(q);

        d = simd4d_madd(d, u, -4.0 * PI4_A);
        d = simd4d_madd(d, u, -4.0 * PI4_B);
        d = simd4d_madd(d, u, -4.0 * PI4_C);
        d = simd4d_madd(d, u, -4.0 * PI4_D);

        const simd4d q1 = simd4d_convert(simd4i_and(q, 1));
        const simd4d mask = simd4d_compare_eq(q1, 1.0);
        d = mulsign(d, mask);

        const simd4d s = simd4d_mul(d, d);
        u = sincos_post(s);
        u = simd4d_madd(d, s, simd4d_mul(u, d));
        return u;
    }

    simd4d simd4d_cos(__simd4d v)
    {
        simd4d d = v;

        simd4i q;
        q = simd4i_convert(simd4d_madd(-0.5, d, simd4d_set1(M_1_PI)));
        q = simd4i_add(simd4i_add(q, q), 1);
        simd4d u = simd4d_convert(q);

        d = simd4d_madd(d, u, -2.0 * PI4_A);
        d = simd4d_madd(d, u, -2.0 * PI4_B);
        d = simd4d_madd(d, u, -2.0 * PI4_C);
        d = simd4d_madd(d, u, -2.0 * PI4_D);

        const simd4d q2 = simd4d_convert(simd4i_and(q, 2));
        const simd4d mask = simd4d_compare_eq(q2, simd4d_zero());
        d = simd4d_xor(simd4d_and(mask, simd4d_set1(-0.0)), d);

        const simd4d s = simd4d_mul(d, d);
        u = sincos_post(s);
        u = simd4d_madd(d, s, simd4d_mul(u, d));
        return u;
    }

    simd4d simd4d_tan(__simd4d d)
    {
        const simd4i q = simd4i_convert(simd4d_mul(d, M_2_PI));
        simd4d u = simd4d_convert(q);

        simd4d x = d;
        x = simd4d_madd(x, u, -2.0 * PI4_A);
        x = simd4d_madd(x, u, -2.0 * PI4_B);
        x = simd4d_madd(x, u, -2.0 * PI4_C);
        x = simd4d_madd(x, u, -2.0 * PI4_D);

        const simd4d s = simd4d_mul(x, x);

        const simd4d mask = simd4d_compare_eq(simd4d_convert(simd4i_and(q, 1)), 1.0);
        x = simd4d_xor(simd4d_and(mask, simd4d_set1(-0.0)), x);

        u = simd4d_set1(1.01419718511083373224408e-05);
        u = simd4d_madd(-2.59519791585924697698614e-05, u, s);
        u = simd4d_madd(5.23388081915899855325186e-05, u, s);
        u = simd4d_madd(-3.05033014433946488225616e-05, u, s);
        u = simd4d_madd(7.14707504084242744267497e-05, u, s);
        u = simd4d_madd(8.09674518280159187045078e-05, u, s);
        u = simd4d_madd(0.000244884931879331847054404, u, s);
        u = simd4d_madd(0.000588505168743587154904506, u, s);
        u = simd4d_madd(0.00145612788922812427978848, u, s);
        u = simd4d_madd(0.00359208743836906619142924, u, s);
        u = simd4d_madd(0.00886323944362401618113356, u, s);
        u = simd4d_madd(0.0218694882853846389592078, u, s);
        u = simd4d_madd(0.0539682539781298417636002, u, s);
        u = simd4d_madd(0.133333333333125941821962, u, s);
        u = simd4d_madd(0.333333333333334980164153, u, s);

        u = simd4d_madd(x, s, simd4d_mul(u, x));
        u = simd4d_select(mask, simd4d_div(1.0, u), u);
        u = simd4d_or(is_inf(d), u);

        return u;
    }

#if 0
    simd4d simd4d_exp(__simd4d v)
    {
        const simd4i q = simd4i_convert(simd4d_mul(v, R_LN2));
        const simd4d p = simd4d_convert(q);

        simd4d s;
        s = simd4d_madd(v, p, -L2Uf);
        s = simd4d_madd(s, p, -L2Lf);

        simd4d u = simd4d_set1(2.08860621107283687536341e-09);
        u = simd4d_madd(2.51112930892876518610661e-08, u, s);
        u = simd4d_madd(2.75573911234900471893338e-07, u, s);
        u = simd4d_madd(2.75572362911928827629423e-06, u, s);
        u = simd4d_madd(2.4801587159235472998791e-05, u, s);
        u = simd4d_madd(0.000198412698960509205564975, u, s);
        u = simd4d_madd(0.00138888888889774492207962, u, s);
        u = simd4d_madd(0.00833333333331652721664984, u, s);
        u = simd4d_madd(0.0416666666666665047591422, u, s);
        u = simd4d_madd(0.166666666666666851703837, u, s);
        u = simd4d_madd(0.5, u, s);

        u = simd4d_add(1.0, simd4d_madd(s, simd4d_mul(s, s), u));
        u = ldexp(u, q);
        u = simd4d_nand(is_negative_inf(v), u);

        return u;
    }

    simd4d simd4d_log2(__simd4d v)
    {
        return v; // TODO
    }

    simd4d simd4d_log(__simd4d v)
    {
        return v; // TODO
        //return simd4f_mul(simd4f_log2(v), 0.69314718055995f);
    }

    simd4d simd4d_exp2(__simd4d v)
    {
        return v; // TODO
    }

    simd4d simd4d_pow(__simd4d a, __simd4d b)
    {
        return simd4d_exp2(simd4d_mul(simd4d_log2(simd4d_abs(a)), b));
    }
#endif

    simd4d simd4d_asin(__simd4d d)
    {
        const simd4d one = simd4d_set1(1.0);
        simd4d x, y;
        x = simd4d_add(one, d);
        y = simd4d_sub(one, d);
        x = simd4d_mul(x, y);
        x = simd4d_sqrt(x);

        x = simd4d_or(is_nan(x), atan2k(simd4d_abs(d), x));
        return mulsign(x, d);
    }

    simd4d simd4d_acos(__simd4d d)
    {
        const simd4d one = simd4d_set1(1.0);
        simd4d x, y;
        x = simd4d_add(one, d);
        y = simd4d_sub(one, d);
        x = simd4d_mul(x, y);
        x = simd4d_sqrt(x);

        x = mulsign(atan2k(x, simd4d_abs(d)), d);
        y = simd4d_and(simd4d_compare_lt(d, simd4d_zero()), simd4d_set1(M_PI));
        x = simd4d_add(x, y);
        return x;
    }

    simd4d simd4d_atan(__simd4d d)
    {
        const simd4d one = simd4d_set1(1.0);

        simd4i q = sel(d, simd4d_set1(0.0), simd4i_set1(2), simd4i_set1(0));
        simd4d s = simd4d_abs(d);

        q = sel(one, s, simd4i_add(q, simd4i_set1(1)), q);
        s = simd4d_select(simd4d_compare_lt(one, s), simd4d_reciprocal(s), s);

        simd4d t = simd4d_mul(s, s);
        
        simd4d u = simd4d_set1(-1.88796008463073496563746e-05);
        u = simd4d_madd(0.000209850076645816976906797, u, t);
        u = simd4d_madd(-0.00110611831486672482563471, u, t);
        u = simd4d_madd(0.00370026744188713119232403, u, t);
        u = simd4d_madd(-0.00889896195887655491740809, u, t);
        u = simd4d_madd(0.016599329773529201970117, u, t);
        u = simd4d_madd(-0.0254517624932312641616861, u, t);
        u = simd4d_madd(0.0337852580001353069993897, u, t);
        u = simd4d_madd(-0.0407629191276836500001934, u, t);
        u = simd4d_madd(0.0466667150077840625632675, u, t);
        u = simd4d_madd(-0.0523674852303482457616113, u, t);
        u = simd4d_madd(0.0587666392926673580854313, u, t);
        u = simd4d_madd(-0.0666573579361080525984562, u, t);
        u = simd4d_madd(0.0769219538311769618355029, u, t);
        u = simd4d_madd(-0.090908995008245008229153, u, t);
        u = simd4d_madd(0.111111105648261418443745, u, t);
        u = simd4d_madd(-0.14285714266771329383765, u, t);
        u = simd4d_madd(0.199999999996591265594148, u, t);
        u = simd4d_madd(-0.333333333333311110369124, u, t);

        t = simd4d_madd(s, s, simd4d_mul(t, u));

        simd4d m;
        const simd4d q1 = simd4d_convert(simd4i_and(q, simd4i_set1(1)));
        m = simd4d_compare_eq(q1, one);
        t = simd4d_select(m, simd4d_sub(M_PI / 2, t), t);

        const simd4d q2 = simd4d_convert(simd4i_and(q, simd4i_set1(2)));
        m = simd4d_compare_eq(q2, simd4d_set1(2.0));
        t = mulsign(t, m);

        return t;
    }

    simd4d simd4d_atan2(__simd4d y, __simd4d x)
    {
        const simd4d pi = simd4d_set1(M_PI);
        const simd4d pi_2 = simd4d_set1(M_PI_2);
        const simd4d pi_4 = simd4d_set1(M_PI_4);

        simd4d r = atan2k(simd4d_abs(y), x);
        r = mulsign(r, x);

	r = simd4d_select(simd4d_or(is_inf(x), simd4d_compare_eq(x, simd4d_zero())),
                          simd4d_sub(pi_2, is_inf2(x, mulsign(pi_2, x))), r);
        r = simd4d_select(is_inf(y), 
                          simd4d_sub(pi_2, is_inf2(x, mulsign(pi_4, x))), r);
        r = simd4d_select(simd4d_compare_eq(y, simd4d_zero()),
                          simd4d_and(simd4d_compare_eq(sign(x), simd4d_set1(-1.0)), pi), r);
        r = simd4d_or(simd4d_or(is_nan(x), is_nan(y)), mulsign(r, y));
        return r;
    }

#else

    // ------------------------------------------------------------------
    // FPU Emulation: simd4d
    // ------------------------------------------------------------------

    simd4d simd4d_sin(__simd4d a)
    {
        simd4d v;
        v.x = std::sin(a.x);
        v.y = std::sin(a.y);
        v.z = std::sin(a.z);
        v.w = std::sin(a.w);
        return v;
    }

    simd4d simd4d_cos(__simd4d a)
    {
        simd4d v;
        v.x = std::cos(a.x);
        v.y = std::cos(a.y);
        v.z = std::cos(a.z);
        v.w = std::cos(a.w);
        return v;
    }

    simd4d simd4d_tan(__simd4d a)
    {
        simd4d v;
        v.x = std::tan(a.x);
        v.y = std::tan(a.y);
        v.z = std::tan(a.z);
        v.w = std::tan(a.w);
        return v;
    }

#if 0
    simd4d simd4d_exp(__simd4d a)
    {
        simd4d v;
        v.x = std::exp(a.x);
        v.y = std::exp(a.y);
        v.z = std::exp(a.z);
        v.w = std::exp(a.w);
        return v;
    }

    simd4d simd4d_log(__simd4d a)
    {
        simd4d v;
        v.x = std::log(a.x);
        v.y = std::log(a.y);
        v.z = std::log(a.z);
        v.w = std::log(a.w);
        return v;
    }

    simd4d simd4d_log2(__simd4d a)
    {
        simd4d v;
        v.x = std::log2(a.x);
        v.y = std::log2(a.y);
        v.z = std::log2(a.z);
        v.w = std::log2(a.w);
        return v;
    }

    simd4d simd4d_exp2(__simd4d a)
    {
        simd4d v;
        v.x = std::exp2(a.x);
        v.y = std::exp2(a.y);
        v.z = std::exp2(a.z);
        v.w = std::exp2(a.w);
        return v;
    }

    simd4d simd4d_pow(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = std::pow(a.x, b.x);
        v.y = std::pow(a.y, b.y);
        v.z = std::pow(a.z, b.z);
        v.w = std::pow(a.w, b.w);
        return v;
    }
#endif

    simd4d simd4d_asin(__simd4d a)
    {
        simd4d v;
        v.x = std::asin(a.x);
        v.y = std::asin(a.y);
        v.z = std::asin(a.z);
        v.w = std::asin(a.w);
        return v;
    }

    simd4d simd4d_acos(__simd4d a)
    {
        simd4d v;
        v.x = std::acos(a.x);
        v.y = std::acos(a.y);
        v.z = std::acos(a.z);
        v.w = std::acos(a.w);
        return v;
    }

    simd4d simd4d_atan(__simd4d a)
    {
        simd4d v;
        v.x = std::atan(a.x);
        v.y = std::atan(a.y);
        v.z = std::atan(a.z);
        v.w = std::atan(a.w);
        return v;
    }

    simd4d simd4d_atan2(__simd4d a, __simd4d b)
    {
        simd4d v;
        v.x = std::atan2(a.x, b.x);
        v.y = std::atan2(a.y, b.y);
        v.z = std::atan2(a.z, b.z);
        v.w = std::atan2(a.w, b.w);
        return v;
    }

#endif

} // namespace simd
} // namespace mango

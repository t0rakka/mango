/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    This work is based on "SLEEF" library and converted to use MANGO SIMD abstraction
    Author : Naoki Shibata
*/
#include <mango/simd/simd.hpp>

namespace mango {
namespace simd {

    // ------------------------------------------------------------------------
    // Sleef: float32x4
    // ------------------------------------------------------------------------

    constexpr float float_pi   = 3.14159265358979323846f;
    constexpr float float_pi_2 = 1.57079632679489661923f;
    constexpr float float_pi_4 = 0.785398163397448309616f;
    constexpr float float_1_pi = 0.318309886183790671538f;
    constexpr float float_2_pi = 0.636619772367581343076f;

    constexpr float PI4_Af = 0.78515625f;
    constexpr float PI4_Bf = 0.00024187564849853515625f;
    constexpr float PI4_Cf = 3.7747668102383613586e-08f;
    constexpr float PI4_Df = 1.2816720341285448015e-12f;
    constexpr float L2Uf = 0.693145751953125f;
    constexpr float L2Lf = 1.428606765330187045e-06f;
    constexpr float R_LN2f = 1.442695040888963407359924681001892137426645954152985934135449406931f;
    constexpr float R_INFf = float(std::numeric_limits<float>::infinity());

    static inline float32x4 signbit(float32x4 f)
    {
        return bitwise_and(f, float32x4_set1(-0.0f));
    }

    static inline float32x4 mulsign(float32x4 x, float32x4 y)
    {
        return bitwise_xor(x, signbit(y));
    }

    static inline float32x4 signed_one(float32x4 f)
    {
        // difference to sign() is that +0.0 -> 1.0, -0.0 -> -1.0
        return bitwise_or(float32x4_set1(1.0f), bitwise_and(float32x4_set1(-0.0f), f));
    }

    static inline float32x4 is_nan(float32x4 d)
    {
        return select(compare_neq(d, d), reinterpret<float32x4>(int32x4_set1(0xffffffff)), float32x4_zero());
    }

    static inline float32x4 is_inf(float32x4 d)
    {
        return select(compare_eq(abs(d), R_INFf), reinterpret<float32x4>(int32x4_set1(0xffffffff)), float32x4_zero());
    }

    static inline float32x4 is_inf2(float32x4 d, float32x4 m)
    {
        return bitwise_and(is_inf(d), bitwise_or(signbit(d), m));
    }

    static inline float32x4 is_negative_inf(float32x4 d)
    {
        return select(compare_eq(d, -R_INFf), reinterpret<float32x4>(int32x4_set1(0xffffffff)), float32x4_zero());
    }

    static inline int32x4 sel(float32x4 f0, float32x4 f1, int32x4 x, int32x4 y)
    {
        return select(compare_lt(f0, f1), x, y);
    }

    static inline float32x4 atan2kf(float32x4 b, float32x4 a)
    {
        int32x4 q = sel(a, float32x4_zero(), int32x4_set1(-2), int32x4_set1(0));
        float32x4 x = abs(a);
        float32x4 y = b;

        q = sel(x, y, add(q, int32x4_set1(1)), q);
        float32x4 s = select(compare_lt(x, y), neg(x), y);
        float32x4 t = max(x, y);

        s = div(s, t);
        t = mul(s, s);

        float32x4 u = float32x4_set1(0.00282363896258175373077393f);
        u = madd(-0.0159569028764963150024414f, u, t);
        u = madd( 0.0425049886107444763183594f, u, t);
        u = madd(-0.0748900920152664184570312f, u, t);
        u = madd( 0.106347933411598205566406f, u, t);
        u = madd(-0.142027363181114196777344f, u, t);
        u = madd( 0.199926957488059997558594f, u, t);
        u = madd(-0.333331018686294555664062f, u, t);

        t = madd(s, s, mul(t, u));
        t = madd(t, convert<float32x4>(q), float32x4_set1(float_pi_2));

        return t;
    }

    static inline float32x4 ldexp(float32x4 x, int32x4 q)
    {
        int32x4 m = srai(q, 31);
        m = slli(sub(srai(add(m, q), 6), m), 4);
        q = sub(q, slli(m, 2));
        m = add(m, 0x7f);

        m = max(m, int32x4_set1(0));
        m = min(m, int32x4_set1(0xff));

        float32x4 u;
        u = reinterpret<float32x4>(slli(m, 23));
        x = mul(mul(mul(mul(x, u), u), u), u);
        u = reinterpret<float32x4>(slli(add(q, 0x7f), 23));

        return mul(x, u);
    }

    float32x4 sin(float32x4 v)
    {
        float32x4 d = v;
        int32x4 q = convert<int32x4>(mul(v, float_1_pi));
        float32x4 u = convert<float32x4>(q);

        d = madd(d, u, -4.0f * PI4_Af);
        d = madd(d, u, -4.0f * PI4_Bf);
        d = madd(d, u, -4.0f * PI4_Cf);
        d = madd(d, u, -4.0f * PI4_Df);

        int32x4 one = int32x4_set1(1);
        mask32x4 mask = compare_eq(bitwise_and(q, 1), one);
        d = select(mask, bitwise_xor(d, float32x4_set1(-0.0f)), d);

        const float32x4 s = mul(d, d);
        u = float32x4_set1(2.6083159809786593541503e-06f);
        u = madd(-0.0001981069071916863322258f, u, s);
        u = madd(0.00833307858556509017944336f, u, s);
        u = madd(-0.166666597127914428710938f, u, s);
        u = madd(d, s, mul(u, d));
        u = bitwise_or(is_inf(d), u);
        return u;
    }

    float32x4 cos(float32x4 v)
    {
        float32x4 d = v;
        int32x4 q;
        q = convert<int32x4>(madd(-0.5f, d, float32x4_set1(float_1_pi)));
        q = add(add(q, q), 1);
        float32x4 u = convert<float32x4>(q);

        d = madd(d, u, -2.0f * PI4_Af);
        d = madd(d, u, -2.0f * PI4_Bf);
        d = madd(d, u, -2.0f * PI4_Cf);
        d = madd(d, u, -2.0f * PI4_Df);

        int32x4 zero = int32x4_zero();
        mask32x4 mask = compare_eq(bitwise_and(q, 2), zero);
        d = select(mask, bitwise_xor(d, float32x4_set1(-0.0f)), d);

        const float32x4 s = mul(d, d);
        u = float32x4_set1(2.6083159809786593541503e-06f);
        u = madd(-0.0001981069071916863322258f, u, s);
        u = madd(0.00833307858556509017944336f, u, s);
        u = madd(-0.166666597127914428710938f, u, s);
        u = madd(d, s, mul(u, d));
        u = bitwise_or(is_inf(d), u);
        return u;
    }

    float32x4 tan(float32x4 d)
    {
        const int32x4 q = convert<int32x4>(mul(d, float32x4_set1(float_2_pi)));

        float32x4 x = d;
        float32x4 u = convert<float32x4>(q);
        x = madd(x, u, -2.0f * PI4_Af);
        x = madd(x, u, -2.0f * PI4_Bf);
        x = madd(x, u, -2.0f * PI4_Cf);
        x = madd(x, u, -2.0f * PI4_Df);

        const float32x4 s = mul(x, x);

        const mask32x4 mask = compare_eq(bitwise_and(q, int32x4_set1(1)), int32x4_set1(1));
        x = select(mask, bitwise_xor(x, float32x4_set1(-0.0f)), x);

        u = float32x4_set1(0.00927245803177356719970703f);
        u = madd(0.00331984995864331722259521f, u, s);
        u = madd(0.0242998078465461730957031f, u, s);
        u = madd(0.0534495301544666290283203f, u, s);
        u = madd(0.133383005857467651367188f, u, s);
        u = madd(0.333331853151321411132812f, u, s);

        u = madd(x, s, mul(u, x));
        u = select(mask, rcp(u), u);
        u = bitwise_or(is_inf(d), u);

        return u;
    }

    float32x4 exp(float32x4 v)
    {
        const int32x4 q = convert<int32x4>(mul(v, R_LN2f));
        const float32x4 p = convert<float32x4>(q);

        float32x4 s;
        s = madd(v, p, -L2Uf);
        s = madd(s, p, -L2Lf);

        float32x4 u = float32x4_set1(0.00136324646882712841033936f);
        u = madd(0.00836596917361021041870117f, u, s);
        u = madd(0.0416710823774337768554688f, u, s);
        u = madd(0.166665524244308471679688f, u, s);
        u = madd(0.499999850988388061523438f, u, s);

        u = add(1.0f, madd(s, mul(s, s), u));
        u = ldexp(u, q);
        u = bitwise_nand(is_negative_inf(v), u);

        return u;
    }

    float32x4 log2(float32x4 v)
    {
        const int32x4 exponent = sub(srli(bitwise_and(reinterpret<int32x4>(v), 0x7fffffff), 23), 127);
        const float32x4 x = sub(reinterpret<float32x4>(sub(reinterpret<int32x4>(v), slli(exponent, 23))), 1.0f);
        const float32x4 x2 = mul(x, x);
        const float32x4 x4 = mul(x2, x2);
        float32x4 hi = float32x4_set1(-0.00931049621349f);
        float32x4 lo = float32x4_set1( 0.47868480909345f);
        hi = madd( 0.05206469089414f, x, hi);
        lo = madd(-0.72116591947498f, x, lo);
        hi = madd(-0.13753123777116f, x, hi);
        hi = madd( 0.24187369696082f, x, hi);
        hi = madd(-0.34730547155299f, x, hi);
        lo = madd(1.442689881667200f, x, lo);
        const float32x4 u = madd(convert<float32x4>(exponent), x, lo);
        return madd(u, x4, hi);
    }

    float32x4 log(float32x4 v)
    {
        return mul(log2(v), 0.69314718055995f);
    }

    float32x4 exp2(float32x4 v)
    {
        const int32x4 ix = truncate<int32x4>(add(v, reinterpret<float32x4>(bitwise_nand(srai(convert<int32x4>(v), 31), 0x3f7fffff))));
        float32x4 f = mul(sub(convert<float32x4>(ix), v), 0.69314718055994530942f);
        float32x4 hi = madd(0.0013298820f, f, float32x4_set1(-0.0001413161f));
        float32x4 lo = madd(0.4999999206f, f, float32x4_set1(-0.1666653019f));
        hi = madd(-0.0083013598f, f, hi);
        hi = madd(0.0416573475f, f, hi);
        lo = madd(-0.9999999995f, f, lo);
        lo = madd(1.0f, f, lo);
        float32x4 f2 = mul(f, f);
        float32x4 a = add(mul(mul(f2, f2), hi), lo);
        int32x4 xxx = select(compare_gt(ix, int32x4_set1(-128)), int32x4_set1(0xffffffff), int32x4_zero());
        float32x4 b = reinterpret<float32x4>(bitwise_and(slli((add(ix, 127)), 23), xxx));
        int32x4 mask = select(compare_gt(ix, 128), int32x4_set1(0x7fffffff), int32x4_set1(0));
        return bitwise_or(mul(a, b), reinterpret<float32x4>(mask));
    }

    float32x4 pow(float32x4 a, float32x4 b)
    {
        return exp2(mul(log2(abs(a)), b));
    }

    float32x4 asin(float32x4 d)
    {
        const float32x4 one = float32x4_set1(1.0f);
        float32x4 x, y;
        x = add(one, d);
        y = sub(one, d);
        x = mul(x, y);
        x = sqrt(x);

        x = bitwise_or(is_nan(x), atan2kf(abs(d), x));
        return mulsign(x, d);
    }

    float32x4 acos(float32x4 d)
    {
        const float32x4 one = float32x4_set1(1.0f);
        float32x4 x, y;
        x = add(one, d);
        y = sub(one, d);
        x = mul(x, y);
        x = sqrt(x);

#if defined(MANGO_COMPILER_CLANG) && defined(MANGO_CPU_ARM)
        // internal compiler error w/ clang 3.6 build for arm neon
        float32x4 mask = float32x4_set1(0x7fffffff);
        float32x4 absd = bitwise_and(d, mask);
#else
        float32x4 absd = abs(d);
#endif

        x = mulsign(atan2kf(x, absd), d);
        y = select(compare_lt(d, float32x4_zero()), float32x4_set1(float_pi), float32x4_zero());
        x = add(x, y);
        return x;
    }

    float32x4 atan(float32x4 d)
    {
        const float32x4 one = float32x4_set1(1.0f);

        int32x4 q = sel(d, float32x4_set1(0.0f), int32x4_set1(2), int32x4_set1(0));
        float32x4 s = abs(d);

        q = sel(float32x4_set1(1.0f), s, add(q, int32x4_set1(1)), q);
        s = select(compare_lt(one, s), rcp(s), s);

        float32x4 t = mul(s, s);

        float32x4 u = float32x4_set1(0.00282363896258175373077393f);
        u = madd(-0.0159569028764963150024414f, u, t);
        u = madd( 0.0425049886107444763183594f, u, t);
        u = madd(-0.0748900920152664184570312f, u, t);
        u = madd( 0.106347933411598205566406f, u, t);
        u = madd(-0.142027363181114196777344f, u, t);
        u = madd( 0.199926957488059997558594f, u, t);
        u = madd(-0.333331018686294555664062f, u, t);

        t = madd(s, s, mul(t, u));

        const int32x4 i1 = int32x4_set1(1);
        const int32x4 i2 = add(i1, i1);
        t = select(compare_eq(bitwise_and(q, i1), i1), sub(float32x4_set1(float_pi_2), t), t);
        float32x4 mask = select(compare_eq(bitwise_and(q, i2), i2), convert<float32x4>(int32x4_set1(0xffffffff)), float32x4_set1(0.0f));
        t = bitwise_xor(bitwise_and(mask, float32x4_set1(-0.0f)), t);

        return t;
    }

    float32x4 atan2(float32x4 y, float32x4 x)
    {
        static const float32x4 pi = float32x4_set1(float_pi);
        static const float32x4 pi_2 = float32x4_set1(float_pi_2);
        static const float32x4 pi_4 = float32x4_set1(float_pi_4);

        float32x4 r = atan2kf(abs(y), x);
        r = mulsign(r, x);

        r = select(compare_eq(abs(x), R_INFf), sub(pi_2, is_inf2(x, mulsign(pi_2, x))), r);
        r = select(compare_eq(abs(y), R_INFf), sub(pi_2, is_inf2(x, mulsign(pi_4, x))), r);
        r = select(compare_eq(y, float32x4_zero()), select(compare_eq(signed_one(x), float32x4_set1(-1.0f)), pi, float32x4_zero()), r);
        r = bitwise_or(bitwise_or(is_nan(x), is_nan(y)), mulsign(r, y));
        return r;
    }

} // namespace simd
} // namespace mango

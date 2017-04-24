/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        return compare_neq(d, d);
    }

    static inline float32x4 is_inf(float32x4 d)
    {
        return compare_eq(abs(d), R_INFf);
    }

    static inline float32x4 is_inf2(float32x4 d, float32x4 m)
    {
        return bitwise_and(is_inf(d), bitwise_or(signbit(d), m));
    }

    static inline float32x4 is_negative_inf(float32x4 d)
    {
        return compare_eq(d, -R_INFf);
    }

    static inline int32x4 sel(float32x4 f0, float32x4 f1, int32x4 x, int32x4 y)
    {
        float32x4 m = compare_lt(f0, f1);
        return select(int32x4_reinterpret(m), x, y);
    }

    static inline float32x4 atan2kf(float32x4 b, float32x4 a)
    {
        int32x4 q = sel(a, float32x4_set1(0.0f), int32x4_set1(-2), int32x4_set1(0));
        float32x4 x = abs(a);
        float32x4 y = b;

        q = sel(x, y, add(q, int32x4_set1(1)), q);
        float32x4 p = compare_lt(x, y);
        float32x4 s = select(p, neg(x), y);
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
        t = madd(t, float32x4_convert(q), float32x4_set1(float_pi_2));

        return t;
    }

    static inline float32x4 ldexp(float32x4 x, int32x4 q)
    {
        float32x4 u;
        int32x4 m = srai(q, 31);
        m = slli(sub(srai(add(m, q), 6), m), 4);
        const int32x4 p = sub(q, slli(m, 2));
        m = add(m, 0x7f);
        m = bitwise_and(compare_gt(m, int32x4_zero()), m);
        const int32x4 n = compare_gt(m, 0xff);
        m = bitwise_or(bitwise_nand(n, m), bitwise_and(n, 0xff));
        u = float32x4_reinterpret(slli(m, 23));
        const float32x4 y = mul(mul(mul(mul(x, u), u), u), u);
        u = float32x4_reinterpret(slli(add(p, 0x7f), 23));
        return mul(y, u);
    }

    static inline float32x4 sincos_post(float32x4 s)
    {
        float32x4 u;
        u = float32x4_set1(2.6083159809786593541503e-06f);
        u = madd(-0.0001981069071916863322258f, u, s);
        u = madd(0.00833307858556509017944336f, u, s);
        u = madd(-0.166666597127914428710938f, u, s);
        return u;
    }

    float32x4 sin(float32x4 v)
    {
        float32x4 d = v;
        int32x4 q = int32x4_convert(mul(v, float_1_pi));
        float32x4 u = float32x4_convert(q);

        d = madd(d, u, -4.0f * PI4_Af);
        d = madd(d, u, -4.0f * PI4_Bf);
        d = madd(d, u, -4.0f * PI4_Cf);
        d = madd(d, u, -4.0f * PI4_Df);

        const int32x4 one = int32x4_set1(1);
        const int32x4 q1 = bitwise_and(q, one);
        const float32x4 mask = float32x4_reinterpret(compare_eq(q1, one));
        d = bitwise_xor(bitwise_and(mask, float32x4_set1(-0.0f)), d);

        const float32x4 s = mul(d, d);
        u = sincos_post(s);
        u = madd(d, s, mul(u, d));
        u = bitwise_or(is_inf(d), u);
        return u;
    }

    float32x4 cos(float32x4 v)
    {
        float32x4 d = v;
        int32x4 q;
        q = int32x4_convert(madd(-0.5f, d, float32x4_set1(float_1_pi)));
        q = add(add(q, q), 1);
        float32x4 u = float32x4_convert(q);

        d = madd(d, u, -2.0f * PI4_Af);
        d = madd(d, u, -2.0f * PI4_Bf);
        d = madd(d, u, -2.0f * PI4_Cf);
        d = madd(d, u, -2.0f * PI4_Df);

        const int32x4 q2 = bitwise_and(q, 2);
        const float32x4 mask = float32x4_reinterpret(compare_eq(q2, int32x4_zero()));
        d = bitwise_xor(bitwise_and(mask, float32x4_set1(-0.0f)), d);

        const float32x4 s = mul(d, d);
        u = sincos_post(s);
        u = madd(d, s, mul(u, d));
        u = bitwise_or(is_inf(d), u);
        return u;
    }

    float32x4 tan(float32x4 d)
    {
        const int32x4 q = int32x4_convert(mul(d, float32x4_set1(float_2_pi)));
        float32x4 u = float32x4_convert(q);

        float32x4 x = d;
        x = madd(x, u, -2.0f * PI4_Af);
        x = madd(x, u, -2.0f * PI4_Bf);
        x = madd(x, u, -2.0f * PI4_Cf);
        x = madd(x, u, -2.0f * PI4_Df);

        const float32x4 s = mul(x, x);
        const int32x4 m = compare_eq(bitwise_and(q, int32x4_set1(1)), int32x4_set1(1));
        const float32x4 mask = float32x4_reinterpret(m);
        x = bitwise_xor(bitwise_and(mask, float32x4_set1(-0.0)), x);

        u = float32x4_set1(0.00927245803177356719970703f);
        u = madd(0.00331984995864331722259521f, u, s);
        u = madd(0.0242998078465461730957031f, u, s);
        u = madd(0.0534495301544666290283203f, u, s);
        u = madd(0.133383005857467651367188f, u, s);
        u = madd(0.333331853151321411132812f, u, s);

        u = madd(x, s, mul(u, x));
        u = select(mask, div(float32x4_set1(1.0f), u), u);
        u = bitwise_or(is_inf(d), u);

        return u;
    }

    float32x4 exp(float32x4 v)
    {
        const int32x4 q = int32x4_convert(mul(v, R_LN2f));
        const float32x4 p = float32x4_convert(q);

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
        const int32x4 exponent = sub(srli(bitwise_and(int32x4_reinterpret(v), 0x7fffffff), 23), 127);
        const float32x4 x = sub(float32x4_reinterpret(sub(int32x4_reinterpret(v), slli(exponent, 23))), 1.0f);
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
        const float32x4 u = madd(float32x4_convert(exponent), x, lo);
        return madd(u, x4, hi);
    }

    float32x4 log(float32x4 v)
    {
        return mul(log2(v), 0.69314718055995f);
    }

    float32x4 exp2(float32x4 v)
    {
        const int32x4 ix = int32x4_truncate(add(v, float32x4_reinterpret(bitwise_nand(srai(int32x4_convert(v), 31), 0x3f7fffff))));
        float32x4 f = mul(sub(float32x4_convert(ix), v), 0.69314718055994530942f);
        float32x4 hi = madd(0.0013298820f, f, float32x4_set1(-0.0001413161f));
        float32x4 lo = madd(0.4999999206f, f, float32x4_set1(-0.1666653019f));
        hi = madd(-0.0083013598f, f, hi);
        hi = madd(0.0416573475f, f, hi);
        lo = madd(-0.9999999995f, f, lo);
        lo = madd(1.0f, f, lo);
        float32x4 f2 = mul(f, f);
        float32x4 a = add(mul(mul(f2, f2), hi), lo);
        float32x4 b = float32x4_reinterpret(bitwise_and(slli((add(ix, 127)), 23), compare_gt(ix, -128)));
        return bitwise_or(mul(a, b), float32x4_reinterpret(srli(compare_gt(ix, 128), 1)));
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
        y = bitwise_and(compare_lt(d, float32x4_zero()), float32x4_set1(float_pi));
        x = add(x, y);
        return x;
    }

    float32x4 atan(float32x4 d)
    {
        const float32x4 one = float32x4_set1(1.0f);

        int32x4 q = sel(d, float32x4_set1(0.0f), int32x4_set1(2), int32x4_set1(0));
        float32x4 s = abs(d);

        q = sel(float32x4_set1(1.0f), s, add(q, int32x4_set1(1)), q);
        s = select(compare_lt(one, s), reciprocal(s), s);

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

        int32x4 m;
        m = compare_eq(bitwise_and(q, int32x4_set1(1)), int32x4_set1(1));
        t = select(float32x4_reinterpret(m), sub(float32x4_set1(float_pi_2), t), t);

        m = compare_eq(bitwise_and(q, int32x4_set1(2)), int32x4_set1(2));
        t = float32x4_reinterpret(bitwise_xor(bitwise_and(m, int32x4_reinterpret(float32x4_set1(-0.0f))), int32x4_reinterpret(t)));

        return t;
    }

    float32x4 atan2(float32x4 y, float32x4 x)
    {
        static const float32x4 pi = float32x4_set1(float_pi);
        static const float32x4 pi_2 = float32x4_set1(float_pi_2);
        static const float32x4 pi_4 = float32x4_set1(float_pi_4);

        float32x4 r = atan2kf(abs(y), x);
        r = mulsign(r, x);

        r = select(bitwise_or(is_inf(x), compare_eq(x, float32x4_zero())),
                          sub(pi_2, is_inf2(x, mulsign(pi_2, x))), r);
        r = select(is_inf(y), 
                          sub(pi_2, is_inf2(x, mulsign(pi_4, x))), r);
        r = select(compare_eq(y, float32x4_zero()),
                          bitwise_and(compare_eq(signed_one(x), float32x4_set1(-1.0f)), pi), r);
        r = bitwise_or(bitwise_or(is_nan(x), is_nan(y)), mulsign(r, y));
        return r;
    }

    // ------------------------------------------------------------------------
    // Sleef: float64x4
    // ------------------------------------------------------------------------

    constexpr double double_pi   = 3.14159265358979323846;
    constexpr double double_pi_2 = 1.57079632679489661923;
    constexpr double double_pi_4 = 0.785398163397448309616;
    constexpr double double_1_pi = 0.318309886183790671538;
    constexpr double double_2_pi = 0.636619772367581343076;

    constexpr double PI4_A = 0.78539816290140151978;
    constexpr double PI4_B = 4.9604678871439933374e-10;
    constexpr double PI4_C = 1.1258708853173288931e-18;
    constexpr double PI4_D = 1.7607799325916000908e-27;
    //constexpr double L2U = 0.69314718055966295651160180568695068359375;
    //constexpr double L2L = 0.28235290563031577122588448175013436025525412068e-12;
    constexpr double R_LN2 = 1.442695040888963407359924681001892137426645954152985934135449406931;
    constexpr double R_INF = double(std::numeric_limits<double>::infinity());

    static inline float64x4 signbit(float64x4 v)
    {
        return bitwise_and(v, float64x4_set1(-0.0));
    }

    static inline float64x4 mulsign(float64x4 x, float64x4 y)
    {
        return bitwise_xor(x, signbit(y));
    }

    static inline float64x4 signed_one(float64x4 v)
    {
        // difference to sign() is that +0.0 -> 1.0, -0.0 -> -1.0
        return bitwise_or(float64x4_set1(1.0), bitwise_and(float64x4_set1(-0.0), v));
    }

    static inline float64x4 is_nan(float64x4 d)
    {
        return compare_neq(d, d);
    }

    static inline float64x4 is_inf(float64x4 d)
    {
        return compare_eq(abs(d), R_INF);
    }

    static inline float64x4 is_inf2(float64x4 d, float64x4 m)
    {
        return bitwise_and(is_inf(d), bitwise_or(signbit(d), m));
    }

    static inline float64x4 is_negative_inf(float64x4 d)
    {
        return compare_eq(d, -R_INF);
    }

    static inline int32x4 sel(float64x4 f0, float64x4 f1, int32x4 x, int32x4 y)
    {
        const float64x4 mask = compare_lt(f0, f1);
        const float64x4 xd = float64x4_convert(x);
        const float64x4 yd = float64x4_convert(y);
        const float64x4 s = select(mask, xd, yd);
        return int32x4_convert(s);
    }

    static inline float64x4 atan2k(float64x4 b, float64x4 a)
    {
        int32x4 q = sel(a, float64x4_zero(), int32x4_set1(-2), int32x4_zero());
        float64x4 x = abs(a);
        float64x4 y = b;

        q = sel(x, y, add(q, int32x4_set1(1)), q);
        float64x4 p = compare_lt(x, y);
        float64x4 s = select(p, neg(x), y);
        float64x4 t = max(x, y);

        s = div(s, t);
        t = mul(s, s);

        float64x4 u = float64x4_set1(-1.88796008463073496563746e-05);
        u = madd(0.000209850076645816976906797, u, t);
        u = madd(-0.00110611831486672482563471, u, t);
        u = madd(0.00370026744188713119232403, u, t);
        u = madd(-0.00889896195887655491740809, u, t);
        u = madd(0.016599329773529201970117, u, t);
        u = madd(-0.0254517624932312641616861, u, t);
        u = madd(0.0337852580001353069993897, u, t);
        u = madd(-0.0407629191276836500001934, u, t);
        u = madd(0.0466667150077840625632675, u, t);
        u = madd(-0.0523674852303482457616113, u, t);
        u = madd(0.0587666392926673580854313, u, t);
        u = madd(-0.0666573579361080525984562, u, t);
        u = madd(0.0769219538311769618355029, u, t);
        u = madd(-0.090908995008245008229153, u, t);
        u = madd(0.111111105648261418443745, u, t);
        u = madd(-0.14285714266771329383765, u, t);
        u = madd(0.199999999996591265594148, u, t);
        u = madd(-0.333333333333311110369124, u, t);

        t = madd(s, s, mul(t, u));
        t = madd(t, float64x4_convert(q), float64x4_set1(double_pi_2));

        return t;
    }

    static inline float64x4 ldexp(float64x4 x, int32x4 q)
    {
        // TODO: implement ldexp() with 64 bit precision
        return float64x4_convert(ldexp(float32x4_convert(x), q));
    }

    static inline float64x4 sincos_post(float64x4 s)
    {
        float64x4 u;
        u = float64x4_set1(-7.97255955009037868891952e-18);
        u = madd(2.81009972710863200091251e-15, u, s);
        u = madd(-7.64712219118158833288484e-13, u, s);
        u = madd(1.60590430605664501629054e-10, u, s);
        u = madd(-2.50521083763502045810755e-08, u, s);
        u = madd(2.75573192239198747630416e-06, u, s);
        u = madd(-0.000198412698412696162806809, u, s);
        u = madd(0.00833333333333332974823815, u, s);
        u = madd(-0.166666666666666657414808, u, s);
        return u;
    }

    float64x4 sin(float64x4 v)
    {
        float64x4 d = v;

        int32x4 q = int32x4_convert(mul(d, double_1_pi));
        float64x4 u = float64x4_convert(q);

        d = madd(d, u, -4.0 * PI4_A);
        d = madd(d, u, -4.0 * PI4_B);
        d = madd(d, u, -4.0 * PI4_C);
        d = madd(d, u, -4.0 * PI4_D);

        const float64x4 q1 = float64x4_convert(bitwise_and(q, 1));
        const float64x4 mask = compare_eq(q1, 1.0);
        d = mulsign(d, mask);

        const float64x4 s = mul(d, d);
        u = sincos_post(s);
        u = madd(d, s, mul(u, d));
        return u;
    }

    float64x4 cos(float64x4 v)
    {
        float64x4 d = v;

        int32x4 q;
        q = int32x4_convert(madd(-0.5, d, float64x4_set1(double_1_pi)));
        q = add(add(q, q), 1);
        float64x4 u = float64x4_convert(q);

        d = madd(d, u, -2.0 * PI4_A);
        d = madd(d, u, -2.0 * PI4_B);
        d = madd(d, u, -2.0 * PI4_C);
        d = madd(d, u, -2.0 * PI4_D);

        const float64x4 q2 = float64x4_convert(bitwise_and(q, 2));
        const float64x4 mask = compare_eq(q2, float64x4_zero());
        d = bitwise_xor(bitwise_and(mask, float64x4_set1(-0.0)), d);

        const float64x4 s = mul(d, d);
        u = sincos_post(s);
        u = madd(d, s, mul(u, d));
        return u;
    }

    float64x4 tan(float64x4 d)
    {
        const int32x4 q = int32x4_convert(mul(d, double_2_pi));
        float64x4 u = float64x4_convert(q);

        float64x4 x = d;
        x = madd(x, u, -2.0 * PI4_A);
        x = madd(x, u, -2.0 * PI4_B);
        x = madd(x, u, -2.0 * PI4_C);
        x = madd(x, u, -2.0 * PI4_D);

        const float64x4 s = mul(x, x);

        const float64x4 mask = compare_eq(float64x4_convert(bitwise_and(q, 1)), 1.0);
        x = bitwise_xor(bitwise_and(mask, float64x4_set1(-0.0)), x);

        u = float64x4_set1(1.01419718511083373224408e-05);
        u = madd(-2.59519791585924697698614e-05, u, s);
        u = madd(5.23388081915899855325186e-05, u, s);
        u = madd(-3.05033014433946488225616e-05, u, s);
        u = madd(7.14707504084242744267497e-05, u, s);
        u = madd(8.09674518280159187045078e-05, u, s);
        u = madd(0.000244884931879331847054404, u, s);
        u = madd(0.000588505168743587154904506, u, s);
        u = madd(0.00145612788922812427978848, u, s);
        u = madd(0.00359208743836906619142924, u, s);
        u = madd(0.00886323944362401618113356, u, s);
        u = madd(0.0218694882853846389592078, u, s);
        u = madd(0.0539682539781298417636002, u, s);
        u = madd(0.133333333333125941821962, u, s);
        u = madd(0.333333333333334980164153, u, s);

        u = madd(x, s, mul(u, x));
        u = select(mask, div(1.0, u), u);
        u = bitwise_or(is_inf(d), u);

        return u;
    }

    float64x4 exp(float64x4 v)
    {
        const int32x4 q = int32x4_convert(mul(v, R_LN2));
        const float64x4 p = float64x4_convert(q);

        float64x4 s;
        s = madd(v, p, -L2Uf);
        s = madd(s, p, -L2Lf);

        float64x4 u = float64x4_set1(2.08860621107283687536341e-09);
        u = madd(2.51112930892876518610661e-08, u, s);
        u = madd(2.75573911234900471893338e-07, u, s);
        u = madd(2.75572362911928827629423e-06, u, s);
        u = madd(2.4801587159235472998791e-05, u, s);
        u = madd(0.000198412698960509205564975, u, s);
        u = madd(0.00138888888889774492207962, u, s);
        u = madd(0.00833333333331652721664984, u, s);
        u = madd(0.0416666666666665047591422, u, s);
        u = madd(0.166666666666666851703837, u, s);
        u = madd(0.5, u, s);

        u = add(1.0, madd(s, mul(s, s), u));
        u = ldexp(u, q);
        u = bitwise_nand(is_negative_inf(v), u);

        return u;
    }

    float64x4 log2(float64x4 v)
    {
#if 0
        // TODO: implement float64x4_log2() with 64 bit precision
        return float64x4_convert(float32x4_log2(float32x4_convert(v)));
#else
        // Choose precision over performance (see above)
        double x = std::log2(get_x(v));
        double y = std::log2(get_y(v));
        double z = std::log2(get_z(v));
        double w = std::log2(get_w(v));
        return float64x4_set4(x, y, z, w);
#endif
    }

    float64x4 log(float64x4 v)
    {
        return mul(log2(v), 0.69314718055994530941723212145818);
    }

    float64x4 exp2(float64x4 v)
    {
#if 0
        // TODO: implement float64x4_exp2() with 64 bit precision
        return float64x4_convert(float32x4_exp2(float32x4_convert(v)));
#else
        // Choose precision over performance (see above)
        double x = std::exp2(get_x(v));
        double y = std::exp2(get_y(v));
        double z = std::exp2(get_z(v));
        double w = std::exp2(get_w(v));
        return float64x4_set4(x, y, z, w);
#endif
    }

    float64x4 pow(float64x4 a, float64x4 b)
    {
        return exp2(mul(log2(abs(a)), b));
    }

    float64x4 asin(float64x4 d)
    {
        const float64x4 one = float64x4_set1(1.0);
        float64x4 x, y;
        x = add(one, d);
        y = sub(one, d);
        x = mul(x, y);
        x = sqrt(x);

        x = bitwise_or(is_nan(x), atan2k(abs(d), x));
        return mulsign(x, d);
    }

    float64x4 acos(float64x4 d)
    {
        const float64x4 one = float64x4_set1(1.0);
        float64x4 x, y;
        x = add(one, d);
        y = sub(one, d);
        x = mul(x, y);
        x = sqrt(x);

        x = mulsign(atan2k(x, abs(d)), d);
        y = bitwise_and(compare_lt(d, float64x4_zero()), float64x4_set1(double_pi));
        x = add(x, y);
        return x;
    }

    float64x4 atan(float64x4 d)
    {
        const float64x4 one = float64x4_set1(1.0);

        int32x4 q = sel(d, float64x4_set1(0.0), int32x4_set1(2), int32x4_set1(0));
        float64x4 s = abs(d);

        q = sel(one, s, add(q, int32x4_set1(1)), q);
        s = select(compare_lt(one, s), reciprocal(s), s);

        float64x4 t = mul(s, s);

        float64x4 u = float64x4_set1(-1.88796008463073496563746e-05);
        u = madd(0.000209850076645816976906797, u, t);
        u = madd(-0.00110611831486672482563471, u, t);
        u = madd(0.00370026744188713119232403, u, t);
        u = madd(-0.00889896195887655491740809, u, t);
        u = madd(0.016599329773529201970117, u, t);
        u = madd(-0.0254517624932312641616861, u, t);
        u = madd(0.0337852580001353069993897, u, t);
        u = madd(-0.0407629191276836500001934, u, t);
        u = madd(0.0466667150077840625632675, u, t);
        u = madd(-0.0523674852303482457616113, u, t);
        u = madd(0.0587666392926673580854313, u, t);
        u = madd(-0.0666573579361080525984562, u, t);
        u = madd(0.0769219538311769618355029, u, t);
        u = madd(-0.090908995008245008229153, u, t);
        u = madd(0.111111105648261418443745, u, t);
        u = madd(-0.14285714266771329383765, u, t);
        u = madd(0.199999999996591265594148, u, t);
        u = madd(-0.333333333333311110369124, u, t);

        t = madd(s, s, mul(t, u));

        float64x4 m;
        const float64x4 q1 = float64x4_convert(bitwise_and(q, int32x4_set1(1)));
        m = compare_eq(q1, one);
        t = select(m, sub(double_pi_2, t), t);

        const float64x4 q2 = float64x4_convert(bitwise_and(q, int32x4_set1(2)));
        m = compare_eq(q2, float64x4_set1(2.0));
        t = mulsign(t, m);

        return t;
    }

    float64x4 atan2(float64x4 y, float64x4 x)
    {
        const float64x4 pi = float64x4_set1(double_pi);
        const float64x4 pi_2 = float64x4_set1(double_pi_2);
        const float64x4 pi_4 = float64x4_set1(double_pi_4);

        float64x4 r = atan2k(abs(y), x);
        r = mulsign(r, x);

        r = select(bitwise_or(is_inf(x), compare_eq(x, float64x4_zero())), sub(pi_2, is_inf2(x, mulsign(pi_2, x))), r);
        r = select(is_inf(y), sub(pi_2, is_inf2(x, mulsign(pi_4, x))), r);
        r = select(compare_eq(y, float64x4_zero()), bitwise_and(compare_eq(signed_one(x), float64x4_set1(-1.0)), pi), r);
        r = bitwise_or(bitwise_or(is_nan(x), is_nan(y)), mulsign(r, y));
        return r;
    }

} // namespace simd
} // namespace mango

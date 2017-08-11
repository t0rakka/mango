/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    This work is based on "SLEEF" library and converted to use MANGO SIMD abstraction
    Author : Naoki Shibata
*/
#include <mango/math/vector.hpp>

namespace mango {

// ------------------------------------------------------------------------
// float32x4
// ------------------------------------------------------------------------

namespace
{
    constexpr float float_pi    = 3.14159265358979323846f;
    constexpr float float_pi_2  = 1.57079632679489661923f;
    constexpr float float_pi_4  = 0.785398163397448309616f;
    constexpr float float_1_pi  = 0.318309886183790671538f;
    constexpr float float_2_pi  = 0.636619772367581343076f;
    constexpr float float_pi4_a = 0.78515625f;
    constexpr float float_pi4_b = 0.00024187564849853515625f;
    constexpr float float_pi4_c = 3.7747668102383613586e-08f;
    constexpr float float_pi4_d = 1.2816720341285448015e-12f;
    constexpr float float_r_ln2 = 1.442695040888963407359924681001892137426645954152985934135449406931f;
    constexpr float float_r_inf = float(std::numeric_limits<float>::infinity());
    constexpr float float_l2u   = 0.693145751953125f;
    constexpr float float_l2l   = 1.428606765330187045e-06f;

    static inline float32x4 signbit(float32x4 f)
    {
        return f & float32x4(-0.0f);
    }

    static inline float32x4 mulsign(float32x4 x, float32x4 y)
    {
        return x ^ signbit(y);
    }

    static inline float32x4 mask_to_vector(mask32x4 mask)
    {
        return select(mask, reinterpret<float32x4>(int32x4(0xffffffff)), float32x4(0.0f));

    }

    static inline float32x4 is_nan(float32x4 d)
    {
        const mask32x4 mask = d != d;
        return mask_to_vector(mask);
    }

    static inline float32x4 is_inf(float32x4 d)
    {
        const mask32x4 mask = abs(d) == float32x4(float_r_inf);
        return mask_to_vector(mask);
    }

    static inline float32x4 is_inf2(float32x4 d, float32x4 m)
    {
        return is_inf(d) & (signbit(d) | m);
    }

    static inline float32x4 is_negative_inf(float32x4 d)
    {
        const mask32x4 mask = d == float32x4(-float_r_inf);
        return mask_to_vector(mask);
    }

    static inline float32x4 signed_one(float32x4 f)
    {
        // difference to sign() is that +0.0 -> 1.0, -0.0 -> -1.0
        return float32x4(1.0f) | (float32x4(-0.0f) & f);
    }

    static inline float32x4 ldexp(float32x4 x, int32x4 q)
    {
        int32x4 m = simd::srai(q, 31);
        m = simd::slli(simd::srai(m + q, 6) - m, 4);
        q = q - simd::slli(m, 2);
        m = m + 0x7f;

        m = max(m, int32x4(0));
        m = min(m, int32x4(0xff));

        float32x4 u;
        u = reinterpret<float32x4>(simd::slli(m, 23));
        x = x * u * u * u * u;
        u = reinterpret<float32x4>(simd::slli(q + 0x7f, 23));
        return x * u;
    }

    static inline int32x4 sel(float32x4 f0, float32x4 f1, int32x4 x, int32x4 y)
    {
        return select(f0 < f1, x, y);
    }

    static inline float32x4 atan2kf(float32x4 b, float32x4 a)
    {
        int32x4 q = sel(a, float32x4(0.0f), int32x4(-2), int32x4(0));
        float32x4 x = abs(a);
        float32x4 y = b;

        q = sel(x, y, q + int32x4(1), q);
        float32x4 s = select(x < y, -x, y);
        float32x4 t = max(x, y);

        s = s / t;
        t = s * s;

        float32x4 u(0.00282363896258175373077393f);
        u = madd(-0.0159569028764963150024414f, u, t);
        u = madd( 0.0425049886107444763183594f, u, t);
        u = madd(-0.0748900920152664184570312f, u, t);
        u = madd( 0.106347933411598205566406f, u, t);
        u = madd(-0.142027363181114196777344f, u, t);
        u = madd( 0.199926957488059997558594f, u, t);
        u = madd(-0.333331018686294555664062f, u, t);

        t = madd(s, s, t * u);
        t = madd(t, convert<float32x4>(q), float32x4(float_pi_2));

        return t;
    }

} // namespace

float32x4 sin(float32x4 d)
{
    int32x4 q = convert<int32x4>(d * float_1_pi);
    float32x4 u = convert<float32x4>(q);

    d = madd(d, u, -4.0f * float_pi4_a);
    d = madd(d, u, -4.0f * float_pi4_b);
    d = madd(d, u, -4.0f * float_pi4_c);
    d = madd(d, u, -4.0f * float_pi4_d);

    d = select((q & 1) == 1, -d, d);

    const float32x4 s = d * d;
    u = float32x4(2.6083159809786593541503e-06f);
    u = madd(-0.0001981069071916863322258f, u, s);
    u = madd(0.00833307858556509017944336f, u, s);
    u = madd(-0.166666597127914428710938f, u, s);
    u = madd(d, s, u * d);
    u = is_inf(d) | u;
    return u;
}

float32x4 cos(float32x4 d)
{
    int32x4 q = convert<int32x4>(madd(float32x4(-0.5f), d, float_1_pi));
    q = q + q + 1;
    float32x4 u = convert<float32x4>(q);

    d = madd(d, u, -2.0f * float_pi4_a);
    d = madd(d, u, -2.0f * float_pi4_b);
    d = madd(d, u, -2.0f * float_pi4_c);
    d = madd(d, u, -2.0f * float_pi4_d);

    d = select((q & 2) == 0, -d, d);

    const float32x4 s = d * d;
    u = float32x4(2.6083159809786593541503e-06f);
    u = madd(-0.0001981069071916863322258f, u, s);
    u = madd(0.00833307858556509017944336f, u, s);
    u = madd(-0.166666597127914428710938f, u, s);
    u = madd(d, s, u * d);
    u = is_inf(d) | u;
    return u;
}

float32x4 tan(float32x4 d)
{
    const int32x4 q = convert<int32x4>(d * float_2_pi);

    float32x4 x = d;
    float32x4 u = convert<float32x4>(q);
    x = madd(x, u, -2.0f * float_pi4_a);
    x = madd(x, u, -2.0f * float_pi4_b);
    x = madd(x, u, -2.0f * float_pi4_c);
    x = madd(x, u, -2.0f * float_pi4_d);

    const float32x4 s = x * x;
    const mask32x4 mask = (q & 1) == 1;
    x = select(mask, -x, x);

    u = float32x4(0.00927245803177356719970703f);
    u = madd(0.00331984995864331722259521f, u, s);
    u = madd(0.0242998078465461730957031f, u, s);
    u = madd(0.0534495301544666290283203f, u, s);
    u = madd(0.133383005857467651367188f, u, s);
    u = madd(0.333331853151321411132812f, u, s);
    u = madd(x, s, u * x);
    u = select(mask, 1.0f / u, u);
    u = is_inf(d) | u;
    return u;
}

float32x4 exp(float32x4 v)
{
    const int32x4 q = convert<int32x4>(v * float_r_ln2);
    const float32x4 p = convert<float32x4>(q);

    float32x4 s;
    s = madd(v, p, -float_l2u);
    s = madd(s, p, -float_l2l);

    float32x4 u(0.00136324646882712841033936f);
    u = madd(0.00836596917361021041870117f, u, s);
    u = madd(0.0416710823774337768554688f, u, s);
    u = madd(0.166665524244308471679688f, u, s);
    u = madd(0.499999850988388061523438f, u, s);

    u = madd(s, s * s, u) + 1.0f;
    u = ldexp(u, q);
    u = nand(is_negative_inf(v), u);

    return u;
}

float32x4 exp2(float32x4 v)
{
    const float32x4 fx = v + reinterpret<float32x4>(nand(simd::srai(convert<int32x4>(v), 31), 0x3f7fffff));
    const int32x4 ix = truncate<int32x4>(fx);
    float32x4 f = (convert<float32x4>(ix) - v) * 0.69314718055994530942f;
    float32x4 hi = madd(0.0013298820f, f, float32x4(-0.0001413161f));
    float32x4 lo = madd(0.4999999206f, f, float32x4(-0.1666653019f));
    hi = madd(-0.0083013598f, f, hi);
    hi = madd(0.0416573475f, f, hi);
    lo = madd(-0.9999999995f, f, lo);
    lo = madd(1.0f, f, lo);
    float32x4 f2 = f * f;
    float32x4 a = f2 * f2 * hi + lo;
    int32x4 xxx = select(ix > -128, int32x4(0xffffffff), int32x4(0));
    float32x4 b = reinterpret<float32x4>(simd::slli((ix + 127), 23) & xxx);
    int32x4 mask = select(ix > 128, int32x4(0x7fffffff), int32x4(0));
    return (a * b) | reinterpret<float32x4>(mask);
}

float32x4 log(float32x4 v)
{
    return log2(v) * 0.69314718055995f;
}

float32x4 log2(float32x4 v)
{
    const int32x4 exponent = simd::srli(reinterpret<int32x4>(v) & 0x7fffffff, 23) - 127;
    const float32x4 x = reinterpret<float32x4>(reinterpret<int32x4>(v) - simd::slli(exponent, 23)) - 1.0f;
    const float32x4 x2 = x * x;
    const float32x4 x4 = x2 * x2;
    float32x4 hi(-0.00931049621349f);
    float32x4 lo( 0.47868480909345f);
    hi = madd( 0.05206469089414f, x, hi);
    lo = madd(-0.72116591947498f, x, lo);
    hi = madd(-0.13753123777116f, x, hi);
    hi = madd( 0.24187369696082f, x, hi);
    hi = madd(-0.34730547155299f, x, hi);
    lo = madd(1.442689881667200f, x, lo);
    const float32x4 u = madd(convert<float32x4>(exponent), x, lo);
    return madd(u, x4, hi);
}

float32x4 asin(float32x4 d)
{
    const float32x4 one(1.0f);
    float32x4 x, y;
    x = one + d;
    y = one - d;
    x = x * y;
    x = sqrt(x);
    x = is_nan(x) | atan2kf(abs(d), x);
    return mulsign(x, d);
}

float32x4 acos(float32x4 d)
{
    const float32x4 zero(0.0f);
    const float32x4 one(1.0f);

    float32x4 x, y;
    x = one + d;
    y = one - d;
    x = x * y;
    x = sqrt(x);

    float32x4 absd = abs(d);
    x = mulsign(atan2kf(x, absd), d);
    y = select(d < zero, float32x4(float_pi), zero);
    return x + y;
}

float32x4 atan(float32x4 d)
{
    const float32x4 zero(0.0f);
    const float32x4 one(1.0f);

    const int32x4 i1 = 1;
    const int32x4 i2 = i1 + i1;

    int32x4 q = sel(d, zero, i2, int32x4(0));
    float32x4 s = abs(d);

    q = sel(one, s, q + i1, q);
    s = select(one < s, 1.0f / s, s);

    float32x4 t = s * s;
    float32x4 u = float32x4(0.00282363896258175373077393f);
    u = madd(-0.0159569028764963150024414f, u, t);
    u = madd( 0.0425049886107444763183594f, u, t);
    u = madd(-0.0748900920152664184570312f, u, t);
    u = madd( 0.106347933411598205566406f, u, t);
    u = madd(-0.142027363181114196777344f, u, t);
    u = madd( 0.199926957488059997558594f, u, t);
    u = madd(-0.333331018686294555664062f, u, t);
    t = madd(s, s, t * u);

    t = select((q & i1) == i1, float32x4(float_pi_2) - t, t);
    t = select((q & i2) == i2, -t, t);
    return t;
}

float32x4 atan2(float32x4 y, float32x4 x)
{
    static const float32x4 pi(float_pi);
    static const float32x4 pi_2(float_pi_2);
    static const float32x4 pi_4(float_pi_4);

    float32x4 r = atan2kf(abs(y), x);
    r = mulsign(r, x);
    r = select(abs(x) == float_r_inf, pi_2 - is_inf2(x, mulsign(pi_2, x)), r);
    r = select(abs(y) == float_r_inf, pi_2 - is_inf2(x, mulsign(pi_4, x)), r);
    r = select(y == 0.0f, select(signed_one(x) == -1.0f, pi, float32x4(0.0f)), r);
    r = is_nan(x) | is_nan(y) | mulsign(r, y);
    return r;
}

float32x4 pow(float32x4 a, float32x4 b)
{
    return exp2(log2(abs(a)) * b);
}

} // namespace mango

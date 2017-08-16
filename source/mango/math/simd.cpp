/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    This work is based on "SLEEF" library and converted to use MANGO SIMD abstraction
    Author : Naoki Shibata
*/
#include <mango/math/vector.hpp>

namespace
{
    using namespace mango;

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

    template <typename FloatVector>
    inline FloatVector signbit(FloatVector f)
    {
        return f & FloatVector(-0.0f);
    }

    template <typename FloatVector>
    inline FloatVector mulsign(FloatVector x, FloatVector y)
    {
        return x ^ signbit(y);
    }

    template <typename FloatVector, typename IntVector>
    inline FloatVector is_nan(FloatVector d)
    {
        const auto mask = d != d;
        return select(mask, reinterpret<FloatVector>(IntVector(0xffffffff)), FloatVector(0.0f));
    }

    template <typename FloatVector, typename IntVector>
    inline FloatVector is_inf(FloatVector d)
    {
        const auto mask = abs(d) == FloatVector(float_r_inf);
        return select(mask, reinterpret<FloatVector>(IntVector(0xffffffff)), FloatVector(0.0f));
    }

    template <typename FloatVector, typename IntVector>
    inline FloatVector is_inf2(FloatVector d, FloatVector m)
    {
        return is_inf<FloatVector, IntVector>(d) & (signbit(d) | m);
    }

    template <typename FloatVector, typename IntVector>
    inline FloatVector is_negative_inf(FloatVector d)
    {
        const auto mask = d == FloatVector(-float_r_inf);
        return select(mask, reinterpret<FloatVector>(IntVector(0xffffffff)), FloatVector(0.0f));
    }

    template <typename FloatVector>
    inline FloatVector signed_one(FloatVector f)
    {
        return FloatVector(1.0f) | signbit(f);
    }

    template <typename FloatVector, typename IntVector>
    inline FloatVector ldexp(FloatVector x, IntVector q)
    {
        IntVector m = simd::srai(q, 31);
        m = simd::slli(simd::srai(m + q, 6) - m, 4);
        q = q - simd::slli(m, 2);
        m = m + 0x7f;

        m = max(m, IntVector(0));
        m = min(m, IntVector(0xff));

        FloatVector u;
        u = reinterpret<FloatVector>(simd::slli(m, 23));
        x = x * u * u * u * u;
        u = reinterpret<FloatVector>(simd::slli(q + 0x7f, 23));
        return x * u;
    }

    template <typename FloatVector, typename IntVector>
    inline FloatVector atan2kf(FloatVector b, FloatVector a)
    {
        IntVector q = select(a < 0.0f, IntVector(-2), IntVector(0));
        FloatVector x = abs(a);
        FloatVector y = b;

        q = select(x < y, q + 1, q);
        FloatVector s = select(x < y, -x, y);
        FloatVector t = max(x, y);

        s = s / t;
        t = s * s;

        FloatVector u(0.00282363896258175373077393f);
        u = madd(-0.0159569028764963150024414f, u, t);
        u = madd( 0.0425049886107444763183594f, u, t);
        u = madd(-0.0748900920152664184570312f, u, t);
        u = madd( 0.106347933411598205566406f, u, t);
        u = madd(-0.142027363181114196777344f, u, t);
        u = madd( 0.199926957488059997558594f, u, t);
        u = madd(-0.333331018686294555664062f, u, t);

        t = madd(s, s, t * u);
        t = madd(t, convert<FloatVector>(q), FloatVector(float_pi_2));

        return t;
    }

    template <typename FloatVector, typename IntVector>
    FloatVector sin_template(FloatVector d)
    {
        IntVector q = convert<IntVector>(d * float_1_pi);
        FloatVector u = convert<FloatVector>(q);

        d = madd(d, u, -4.0f * float_pi4_a);
        d = madd(d, u, -4.0f * float_pi4_b);
        d = madd(d, u, -4.0f * float_pi4_c);
        d = madd(d, u, -4.0f * float_pi4_d);

        d = select((q & 1) == 1, -d, d);

        const FloatVector s = d * d;
        u = FloatVector(2.6083159809786593541503e-06f);
        u = madd(-0.0001981069071916863322258f, u, s);
        u = madd(0.00833307858556509017944336f, u, s);
        u = madd(-0.166666597127914428710938f, u, s);
        u = madd(d, s, u * d);
        u = is_inf<FloatVector, IntVector>(d) | u;
        return u;
    }

    template <typename FloatVector, typename IntVector>
    FloatVector cos_template(FloatVector d)
    {
        IntVector q = convert<IntVector>(madd(FloatVector(-0.5f), d, float_1_pi));
        q = q + q + 1;
        FloatVector u = convert<FloatVector>(q);

        d = madd(d, u, -2.0f * float_pi4_a);
        d = madd(d, u, -2.0f * float_pi4_b);
        d = madd(d, u, -2.0f * float_pi4_c);
        d = madd(d, u, -2.0f * float_pi4_d);

        d = select((q & 2) == 0, -d, d);

        const FloatVector s = d * d;
        u = FloatVector(2.6083159809786593541503e-06f);
        u = madd(-0.0001981069071916863322258f, u, s);
        u = madd(0.00833307858556509017944336f, u, s);
        u = madd(-0.166666597127914428710938f, u, s);
        u = madd(d, s, u * d);
        u = is_inf<FloatVector, IntVector>(d) | u;
        return u;
    }

    template <typename FloatVector, typename IntVector, typename Mask>
    FloatVector tan_template(FloatVector d)
    {
        const IntVector q = convert<IntVector>(d * float_2_pi);

        FloatVector x = d;
        FloatVector u = convert<FloatVector>(q);
        x = madd(x, u, -2.0f * float_pi4_a);
        x = madd(x, u, -2.0f * float_pi4_b);
        x = madd(x, u, -2.0f * float_pi4_c);
        x = madd(x, u, -2.0f * float_pi4_d);

        const FloatVector s = x * x;
        const Mask mask = (q & 1) == 1;
        x = select(mask, -x, x);

        u = FloatVector(0.00927245803177356719970703f);
        u = madd(0.00331984995864331722259521f, u, s);
        u = madd(0.0242998078465461730957031f, u, s);
        u = madd(0.0534495301544666290283203f, u, s);
        u = madd(0.133383005857467651367188f, u, s);
        u = madd(0.333331853151321411132812f, u, s);
        u = madd(x, s, u * x);
        u = select(mask, 1.0f / u, u);
        u = is_inf<FloatVector, IntVector>(d) | u;
        return u;
    }

    template <typename FloatVector, typename IntVector>
    FloatVector exp_template(FloatVector v)
    {
        const IntVector q = convert<IntVector>(v * float_r_ln2);
        const FloatVector p = convert<FloatVector>(q);

        FloatVector s;
        s = madd(v, p, -float_l2u);
        s = madd(s, p, -float_l2l);

        FloatVector u(0.00136324646882712841033936f);
        u = madd(0.00836596917361021041870117f, u, s);
        u = madd(0.0416710823774337768554688f, u, s);
        u = madd(0.166665524244308471679688f, u, s);
        u = madd(0.499999850988388061523438f, u, s);

        u = madd(s, s * s, u) + 1.0f;
        u = ldexp(u, q);
        u = nand(is_negative_inf<FloatVector, IntVector>(v), u);

        return u;
    }

    template <typename FloatVector, typename IntVector>
    FloatVector exp2_template(FloatVector v)
    {
        const FloatVector fx = v + reinterpret<FloatVector>(nand(simd::srai(convert<IntVector>(v), 31), 0x3f7fffff));
        const IntVector ix = truncate<IntVector>(fx);
        FloatVector f = (convert<FloatVector>(ix) - v) * 0.69314718055994530942f;
        FloatVector hi = madd(0.0013298820f, f, FloatVector(-0.0001413161f));
        FloatVector lo = madd(0.4999999206f, f, FloatVector(-0.1666653019f));
        hi = madd(-0.0083013598f, f, hi);
        hi = madd(0.0416573475f, f, hi);
        lo = madd(-0.9999999995f, f, lo);
        lo = madd(1.0f, f, lo);
        FloatVector f2 = f * f;
        FloatVector a = f2 * f2 * hi + lo;
        IntVector xxx = select(ix > -128, IntVector(0xffffffff), IntVector(0));
        FloatVector b = reinterpret<FloatVector>(simd::slli((ix + 127), 23) & xxx);
        IntVector mask = select(ix > 128, IntVector(0x7fffffff), IntVector(0));
        return (a * b) | reinterpret<FloatVector>(mask);
    }

    template <typename FloatVector, typename IntVector>
    FloatVector log2_template(FloatVector v)
    {
        const IntVector exponent = simd::srli(reinterpret<IntVector>(v) & 0x7fffffff, 23) - 127;
        const FloatVector x = reinterpret<FloatVector>(reinterpret<IntVector>(v) - simd::slli(exponent, 23)) - 1.0f;
        const FloatVector x2 = x * x;
        const FloatVector x4 = x2 * x2;
        FloatVector hi(-0.00931049621349f);
        FloatVector lo( 0.47868480909345f);
        hi = madd( 0.05206469089414f, x, hi);
        lo = madd(-0.72116591947498f, x, lo);
        hi = madd(-0.13753123777116f, x, hi);
        hi = madd( 0.24187369696082f, x, hi);
        hi = madd(-0.34730547155299f, x, hi);
        lo = madd(1.442689881667200f, x, lo);
        const FloatVector u = madd(convert<FloatVector>(exponent), x, lo);
        return madd(u, x4, hi);
    }

    template <typename FloatVector, typename IntVector>
    FloatVector asin_template(FloatVector d)
    {
        const FloatVector one(1.0f);
        FloatVector x, y;
        x = one + d;
        y = one - d;
        x = x * y;
        x = sqrt(x);
        x = is_nan<FloatVector, IntVector>(x) | atan2kf<FloatVector, IntVector>(abs(d), x);
        return mulsign(x, d);
    }

    template <typename FloatVector, typename IntVector>
    FloatVector acos_template(FloatVector d)
    {
        const FloatVector zero(0.0f);
        const FloatVector one(1.0f);

        FloatVector x, y;
        x = one + d;
        y = one - d;
        x = x * y;
        x = sqrt(x);

        FloatVector absd = abs(d);
        x = mulsign<FloatVector>(atan2kf<FloatVector, IntVector>(x, absd), d);
        y = select(d < zero, FloatVector(float_pi), zero);
        return x + y;
    }

    template <typename FloatVector, typename IntVector>
    FloatVector atan_template(FloatVector d)
    {
        const FloatVector zero(0.0f);
        const FloatVector one(1.0f);

        const IntVector i1 = 1;
        const IntVector i2 = i1 + i1;

        IntVector q = select(d < zero, i2, 0);
        FloatVector s = abs(d);

        q = select(one < s, q + i1, q);
        s = select(one < s, 1.0f / s, s);

        FloatVector t = s * s;
        FloatVector u = FloatVector(0.00282363896258175373077393f);
        u = madd(-0.0159569028764963150024414f, u, t);
        u = madd( 0.0425049886107444763183594f, u, t);
        u = madd(-0.0748900920152664184570312f, u, t);
        u = madd( 0.106347933411598205566406f, u, t);
        u = madd(-0.142027363181114196777344f, u, t);
        u = madd( 0.199926957488059997558594f, u, t);
        u = madd(-0.333331018686294555664062f, u, t);
        t = madd(s, s, t * u);

        t = select((q & i1) == i1, FloatVector(float_pi_2) - t, t);
        t = select((q & i2) == i2, -t, t);
        return t;
    }

    template <typename FloatVector, typename IntVector>
    FloatVector atan2_template(FloatVector y, FloatVector x)
    {
        static const FloatVector pi(float_pi);
        static const FloatVector pi_2(float_pi_2);
        static const FloatVector pi_4(float_pi_4);

        FloatVector r = atan2kf<FloatVector, IntVector>(abs(y), x);
        r = mulsign<FloatVector>(r, x);
        r = select(abs(x) == float_r_inf, pi_2 - is_inf2<FloatVector, IntVector>(x, mulsign<FloatVector>(pi_2, x)), r);
        r = select(abs(y) == float_r_inf, pi_2 - is_inf2<FloatVector, IntVector>(x, mulsign<FloatVector>(pi_4, x)), r);
        r = select(y == 0.0f, select(signed_one(x) == -1.0f, pi, FloatVector(0.0f)), r);
        r = is_nan<FloatVector, IntVector>(x) | is_nan<FloatVector, IntVector>(y) | mulsign(r, y);
        return r;
    }

    template <typename FloatVector, typename IntVector>
    FloatVector pow_template(FloatVector a, FloatVector b)
    {
        FloatVector temp = log2_template<FloatVector, IntVector>(abs(a)) * b;
        return exp2_template<FloatVector, IntVector>(temp);
    }

} // namespace

namespace mango {

    // ------------------------------------------------------------------------
    // float32x4
    // ------------------------------------------------------------------------

    float32x4 sin(float32x4 v)
    {
        return sin_template<float32x4, int32x4>(v);
    }

    float32x4 cos(float32x4 v)
    {
        return cos_template<float32x4, int32x4>(v);
    }

    float32x4 tan(float32x4 v)
    {
        return tan_template<float32x4, int32x4, mask32x4>(v);
    }

    float32x4 exp(float32x4 v)
    {
        return exp_template<float32x4, int32x4>(v);
    }

    float32x4 exp2(float32x4 v)
    {
        return exp2_template<float32x4, int32x4>(v);
    }

    float32x4 log(float32x4 v)
    {
        return log2_template<float32x4, int32x4>(v) * 0.69314718055995f;
    }

    float32x4 log2(float32x4 v)
    {
        return log2_template<float32x4, int32x4>(v);
    }

    float32x4 asin(float32x4 v)
    {
        return asin_template<float32x4, int32x4>(v);
    }

    float32x4 acos(float32x4 v)
    {
        return acos_template<float32x4, int32x4>(v);
    }

    float32x4 atan(float32x4 v)
    {
        return atan_template<float32x4, int32x4>(v);
    }

    float32x4 atan2(float32x4 y, float32x4 x)
    {
        return atan2_template<float32x4, int32x4>(y, x);
    }

    float32x4 pow(float32x4 a, float32x4 b)
    {
        return pow_template<float32x4, int32x4>(a, b);
    }

    // ------------------------------------------------------------------------
    // float32x8
    // ------------------------------------------------------------------------

    float32x8 sin(float32x8 v)
    {
        return sin_template<float32x8, int32x8>(v);
    }

    float32x8 cos(float32x8 v)
    {
        return cos_template<float32x8, int32x8>(v);
    }

    float32x8 tan(float32x8 v)
    {
        return tan_template<float32x8, int32x8, mask32x8>(v);
    }

    float32x8 exp(float32x8 v)
    {
        return exp_template<float32x8, int32x8>(v);
    }

    float32x8 exp2(float32x8 v)
    {
        return exp2_template<float32x8, int32x8>(v);
    }

    float32x8 log(float32x8 v)
    {
        return log2_template<float32x8, int32x8>(v) * 0.69314718055995f;
    }

    float32x8 log2(float32x8 v)
    {
        return log2_template<float32x8, int32x8>(v);
    }

    float32x8 asin(float32x8 v)
    {
        return asin_template<float32x8, int32x8>(v);
    }

    float32x8 acos(float32x8 v)
    {
        return acos_template<float32x8, int32x8>(v);
    }

    float32x8 atan(float32x8 v)
    {
        return atan_template<float32x8, int32x8>(v);
    }

    float32x8 atan2(float32x8 y, float32x8 x)
    {
        return atan2_template<float32x8, int32x8>(y, x);
    }

    float32x8 pow(float32x8 a, float32x8 b)
    {
        return pow_template<float32x8, int32x8>(a, b);
    }

    // ------------------------------------------------------------------------
    // float32x16
    // ------------------------------------------------------------------------

    float32x16 sin(float32x16 v)
    {
        return sin_template<float32x16, int32x16>(v);
    }

    float32x16 cos(float32x16 v)
    {
        return cos_template<float32x16, int32x16>(v);
    }

    float32x16 tan(float32x16 v)
    {
        return tan_template<float32x16, int32x16, mask32x16>(v);
    }

    float32x16 exp(float32x16 v)
    {
        return exp_template<float32x16, int32x16>(v);
    }

    float32x16 exp2(float32x16 v)
    {
        return exp2_template<float32x16, int32x16>(v);
    }

    float32x16 log(float32x16 v)
    {
        return log2_template<float32x16, int32x16>(v) * 0.69314718055995f;
    }

    float32x16 log2(float32x16 v)
    {
        return log2_template<float32x16, int32x16>(v);
    }

    float32x16 asin(float32x16 v)
    {
        return asin_template<float32x16, int32x16>(v);
    }

    float32x16 acos(float32x16 v)
    {
        return acos_template<float32x16, int32x16>(v);
    }

    float32x16 atan(float32x16 v)
    {
        return atan_template<float32x16, int32x16>(v);
    }

    float32x16 atan2(float32x16 y, float32x16 x)
    {
        return atan2_template<float32x16, int32x16>(y, x);
    }

    float32x16 pow(float32x16 a, float32x16 b)
    {
        return pow_template<float32x16, int32x16>(a, b);
    }

} // namespace mango

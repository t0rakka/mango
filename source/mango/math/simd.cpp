/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    This work is based on "SLEEF" library and converted to use MANGO SIMD abstraction
    Author : Naoki Shibata
*/
#include <mango/math/vector.hpp>

namespace fp32
{
    using namespace mango;
    using namespace mango::math;

    // constants

    static constexpr float float_pi    = 3.14159265358979323846f;
    static constexpr float float_pi_2  = 1.57079632679489661923f;
    static constexpr float float_pi_4  = 0.785398163397448309616f;
    static constexpr float float_1_pi  = 0.318309886183790671538f;
    static constexpr float float_2_pi  = 0.636619772367581343076f;
    static constexpr float float_pi4_a = 0.78515625f;
    static constexpr float float_pi4_b = 0.00024187564849853515625f;
    static constexpr float float_pi4_c = 3.7747668102383613586e-08f;
    static constexpr float float_pi4_d = 1.2816720341285448015e-12f;
    static constexpr float float_r_ln2 = 1.442695040888963407359924681001892137426645954152985934135449406931f;
    static constexpr float float_r_inf = float(std::numeric_limits<float>::infinity());
    static constexpr float float_l2u   = 0.693145751953125f;
    static constexpr float float_l2l   = 1.428606765330187045e-06f;

    // utility functions

    template <typename F>
    inline F signbit(F f)
    {
        return f & F(-0.0f);
    }

    template <typename F>
    inline F mulsign(F x, F y)
    {
        return x ^ signbit(y);
    }

    template <typename F, typename I>
    inline F is_nan(F d)
    {
        const auto mask = d != d;
        return select(mask, reinterpret<F>(I(0xffffffff)), F(0.0f));
    }

    template <typename F, typename I>
    inline F is_inf(F d)
    {
        const auto mask = abs(d) == F(float_r_inf);
        return select(mask, reinterpret<F>(I(0xffffffff)), F(0.0f));
    }

    template <typename F, typename I>
    inline F is_inf2(F d, F m)
    {
        return is_inf<F, I>(d) & (signbit(d) | m);
    }

    template <typename F, typename I>
    inline F is_negative_inf(F d)
    {
        const auto mask = d == F(-float_r_inf);
        return select(mask, reinterpret<F>(I(0xffffffff)), F(0.0f));
    }

    template <typename F>
    inline F signed_one(F f)
    {
        return F(1.0f) | signbit(f);
    }

    template <typename F, typename I>
    inline F ldexp(F x, I q)
    {
        I m = simd::srai(q, 31);
        m = simd::slli(simd::srai(m + q, 6) - m, 4);
        q = q - simd::slli(m, 2);
        m = m + 0x7f;

        m = max(m, I(0));
        m = min(m, I(0xff));

        F u;
        u = reinterpret<F>(simd::slli(m, 23));
        x = x * u * u * u * u;
        u = reinterpret<F>(simd::slli(q + 0x7f, 23));
        return x * u;
    }

    template <typename F, typename I>
    inline F atan2kf(F b, F a)
    {
        I q = select(a < 0.0f, I(-2), I(0));
        F x = abs(a);
        F y = b;

        q = select(x < y, q + 1, q);
        F s = select(x < y, -x, y);
        F t = max(x, y);

        s = s / t;
        t = s * s;

        F u(0.00282363896258175373077393f);
        u = madd(-0.0159569028764963150024414f, u, t);
        u = madd( 0.0425049886107444763183594f, u, t);
        u = madd(-0.0748900920152664184570312f, u, t);
        u = madd( 0.106347933411598205566406f, u, t);
        u = madd(-0.142027363181114196777344f, u, t);
        u = madd( 0.199926957488059997558594f, u, t);
        u = madd(-0.333331018686294555664062f, u, t);

        t = madd(s, s, t * u);
        t = madd(t, convert<F>(q), F(float_pi_2));

        return t;
    }

    // API

    template <typename F, typename I>
    F sin(F d)
    {
        I q = convert<I>(d * float_1_pi);
        F u = convert<F>(q);

        d = madd(d, u, -4.0f * float_pi4_a);
        d = madd(d, u, -4.0f * float_pi4_b);
        d = madd(d, u, -4.0f * float_pi4_c);
        d = madd(d, u, -4.0f * float_pi4_d);

        d = select((q & 1) == 1, -d, d);

        const F s = d * d;
        u = F(2.6083159809786593541503e-06f);
        u = madd(-0.0001981069071916863322258f, u, s);
        u = madd(0.00833307858556509017944336f, u, s);
        u = madd(-0.166666597127914428710938f, u, s);
        u = madd(d, s, u * d);
        u = is_inf<F, I>(d) | u;
        return u;
    }

    template <typename F, typename I>
    F cos(F d)
    {
        I q = convert<I>(madd(F(-0.5f), d, float_1_pi));
        q = q + q + 1;
        F u = convert<F>(q);

        d = madd(d, u, -2.0f * float_pi4_a);
        d = madd(d, u, -2.0f * float_pi4_b);
        d = madd(d, u, -2.0f * float_pi4_c);
        d = madd(d, u, -2.0f * float_pi4_d);

        d = select((q & 2) == 0, -d, d);

        const F s = d * d;
        u = F(2.6083159809786593541503e-06f);
        u = madd(-0.0001981069071916863322258f, u, s);
        u = madd(0.00833307858556509017944336f, u, s);
        u = madd(-0.166666597127914428710938f, u, s);
        u = madd(d, s, u * d);
        u = is_inf<F, I>(d) | u;
        return u;
    }

    template <typename F, typename I>
    F tan(F d)
    {
        const I q = convert<I>(d * float_2_pi);

        F x = d;
        F u = convert<F>(q);
        x = madd(x, u, -2.0f * float_pi4_a);
        x = madd(x, u, -2.0f * float_pi4_b);
        x = madd(x, u, -2.0f * float_pi4_c);
        x = madd(x, u, -2.0f * float_pi4_d);

        const F s = x * x;
        const auto mask = (q & 1) == 1;
        x = select(mask, -x, x);

        u = F(0.00927245803177356719970703f);
        u = madd(0.00331984995864331722259521f, u, s);
        u = madd(0.0242998078465461730957031f, u, s);
        u = madd(0.0534495301544666290283203f, u, s);
        u = madd(0.133383005857467651367188f, u, s);
        u = madd(0.333331853151321411132812f, u, s);
        u = madd(x, s, u * x);
        u = select(mask, 1.0f / u, u);
        u = is_inf<F, I>(d) | u;
        return u;
    }

    template <typename F, typename I>
    F exp(F v)
    {
        const I q = convert<I>(v * float_r_ln2);
        const F p = convert<F>(q);

        F s;
        s = madd(v, p, -float_l2u);
        s = madd(s, p, -float_l2l);

        F u(0.00136324646882712841033936f);
        u = madd(0.00836596917361021041870117f, u, s);
        u = madd(0.0416710823774337768554688f, u, s);
        u = madd(0.166665524244308471679688f, u, s);
        u = madd(0.499999850988388061523438f, u, s);

        u = madd(s, s * s, u) + 1.0f;
        u = ldexp(u, q);
        u = nand(is_negative_inf<F, I>(v), u);

        return u;
    }

    template <typename F, typename I>
    F exp2(F v)
    {
        const F fx = v + reinterpret<F>(nand(simd::srai(convert<I>(v), 31), 0x3f7fffff));
        const I ix = truncate<I>(fx);
        F f = (convert<F>(ix) - v) * 0.69314718055994530942f;
        F hi = madd(0.0013298820f, f, F(-0.0001413161f));
        F lo = madd(0.4999999206f, f, F(-0.1666653019f));
        hi = madd(-0.0083013598f, f, hi);
        hi = madd(0.0416573475f, f, hi);
        lo = madd(-0.9999999995f, f, lo);
        lo = madd(1.0f, f, lo);
        F f2 = f * f;
        F a = f2 * f2 * hi + lo;
        I xxx = select(ix > -128, I(0xffffffff), I(0));
        F b = reinterpret<F>(simd::slli((ix + 127), 23) & xxx);
        I mask = select(ix > 128, I(0x7fffffff), I(0));
        return (a * b) | reinterpret<F>(mask);
    }

    template <typename F, typename I>
    F log2(F v)
    {
        const I exponent = simd::srli(reinterpret<I>(v) & 0x7fffffff, 23) - 127;
        const F x = reinterpret<F>(reinterpret<I>(v) - simd::slli(exponent, 23)) - 1.0f;
        const F x2 = x * x;
        const F x4 = x2 * x2;
        F hi(-0.00931049621349f);
        F lo( 0.47868480909345f);
        hi = madd( 0.05206469089414f, x, hi);
        lo = madd(-0.72116591947498f, x, lo);
        hi = madd(-0.13753123777116f, x, hi);
        hi = madd( 0.24187369696082f, x, hi);
        hi = madd(-0.34730547155299f, x, hi);
        lo = madd(1.442689881667200f, x, lo);
        const F u = madd(convert<F>(exponent), x, lo);
        return madd(u, x4, hi);
    }

    template <typename F, typename I>
    F asin(F d)
    {
        const F one(1.0f);
        F x = one + d;
        F y = one - d;
        x = x * y;
        x = sqrt(x);
        x = is_nan<F, I>(x) | atan2kf<F, I>(abs(d), x);
        return mulsign(x, d);
    }

    template <typename F, typename I>
    F acos(F d)
    {
        const F zero(0.0f);
        const F one(1.0f);

        F x = one + d;
        F y = one - d;
        x = x * y;
        x = sqrt(x);

        F absd = abs(d);
        x = mulsign<F>(atan2kf<F, I>(x, absd), d);
        y = select(d < zero, F(float_pi), zero);
        return x + y;
    }

    template <typename F, typename I>
    F atan(F d)
    {
        const F zero(0.0f);
        const F one(1.0f);

        const I i1 = 1;
        const I i2 = i1 + i1;

        I q = select(d < zero, i2, 0);
        F s = abs(d);

        q = select(one < s, q + i1, q);
        s = select(one < s, 1.0f / s, s);

        F t = s * s;
        F u = F(0.00282363896258175373077393f);
        u = madd(-0.0159569028764963150024414f, u, t);
        u = madd( 0.0425049886107444763183594f, u, t);
        u = madd(-0.0748900920152664184570312f, u, t);
        u = madd( 0.106347933411598205566406f, u, t);
        u = madd(-0.142027363181114196777344f, u, t);
        u = madd( 0.199926957488059997558594f, u, t);
        u = madd(-0.333331018686294555664062f, u, t);
        t = madd(s, s, t * u);

        t = select((q & i1) == i1, F(float_pi_2) - t, t);
        t = select((q & i2) == i2, -t, t);
        return t;
    }

    template <typename F, typename I>
    F atan2(F y, F x)
    {
        const F pi_1(float_pi);
        const F pi_2(float_pi_2);
        const F pi_4(float_pi_4);

        F r = atan2kf<F, I>(abs(y), x);
        r = mulsign<F>(r, x);
        r = select(abs(x) == float_r_inf, pi_2 - is_inf2<F, I>(x, mulsign<F>(pi_2, x)), r);
        r = select(abs(y) == float_r_inf, pi_2 - is_inf2<F, I>(x, mulsign<F>(pi_4, x)), r);
        r = select(y == 0.0f, select(signed_one(x) == -1.0f, pi_1, F(0.0f)), r);
        r = is_nan<F, I>(x) | is_nan<F, I>(y) | mulsign(r, y);
        return r;
    }

    template <typename F, typename I>
    F pow(F a, F b)
    {
        F temp = log2<F, I>(abs(a)) * b;
        return exp2<F, I>(temp);
    }

} // namespace fp32

#if 0

    // DO NOT ENABLE THIS CODE IT IS WORK-IN-PROGRESS

    // The bit-field masks are for 32 bit values

namespace fp64
{
    using namespace mango;
    using namespace mango::math;

    // constants

    static constexpr double double_pi    = 3.14159265358979323846;
    static constexpr double double_pi_2  = 1.57079632679489661923;
    static constexpr double double_pi_4  = 0.785398163397448309616;
    static constexpr double double_1_pi  = 0.318309886183790671538;
    static constexpr double double_2_pi  = 0.636619772367581343076;
    static constexpr double double_pi4_a = 0.78539816290140151978;
    static constexpr double double_pi4_b = 4.9604678871439933374e-10;
    static constexpr double double_pi4_c = 1.1258708853173288931e-18;
    static constexpr double double_pi4_d = 1.7607799325916000908e-27;
    static constexpr double double_r_ln2 = 1.442695040888963407359924681001892137426645954152985934135449406931;
    static constexpr double double_r_inf = double(std::numeric_limits<double>::infinity());
    static constexpr double double_l2u   = 0.693145751953125;
    static constexpr double double_l2l   = 1.428606765330187045e-06;

    // utility functions

    static inline
    simd::s64x2 srai64(simd::s64x2 a, int count)
    {
        s64* data = reinterpret_cast<s64*>(&a);
        data[0] >>= count;
        data[1] >>= count;
        return a;
    }

    static inline
    simd::s64x4 srai64(simd::s64x4 a, int count)
    {
        s64* data = reinterpret_cast<s64*>(&a);
        data[0] >>= count;
        data[1] >>= count;
        data[2] >>= count;
        data[3] >>= count;
        return a;
    }

    static inline
    simd::s64x8 srai64(simd::s64x8 a, int count)
    {
        s64* data = reinterpret_cast<s64*>(&a);
        data[0] >>= count;
        data[1] >>= count;
        data[2] >>= count;
        data[3] >>= count;
        data[4] >>= count;
        data[5] >>= count;
        data[6] >>= count;
        data[7] >>= count;
        return a;
    }

    template <typename F>
    inline F signbit(F f)
    {
        return f & F(-0.0);
    }

    template <typename F>
    inline F mulsign(F x, F y)
    {
        return x ^ signbit(y);
    }

    template <typename F, typename I>
    inline F is_nan(F d)
    {
        const auto mask = d != d;
        return select(mask, reinterpret<F>(I(0xffffffffffffffff)), F(0.0));
    }

    template <typename F, typename I>
    inline F is_inf(F d)
    {
        const auto mask = abs(d) == F(double_r_inf);
        return select(mask, reinterpret<F>(I(0xffffffffffffffff)), F(0.0));
    }

    template <typename F, typename I>
    inline F is_inf2(F d, F m)
    {
        return is_inf<F, I>(d) & (signbit(d) | m);
    }

    template <typename F, typename I>
    inline F is_negative_inf(F d)
    {
        const auto mask = d == F(-double_r_inf);
        return select(mask, reinterpret<F>(I(0xffffffffffffffff)), F(0.0));
    }

    template <typename F>
    inline F signed_one(F f)
    {
        return F(1.0) | signbit(f);
    }

    template <typename F, typename I>
    inline F ldexp(F x, I q)
    {
        I m = srai64(q, 31);
        m = simd::slli(srai64(m + q, 9) - m, 7);
        q = q - simd::slli(m, 2);
        m = m + 0x3ff;

        m = max(m, I(0));
        m = min(m, I(0x7ff));

        F u;
        u = reinterpret<F>(simd::slli(m, 20));
        x = x * u * u * u * u;
        u = reinterpret<F>(simd::slli(q + 0x3ff, 20));
        return x * u;
    }

    template <typename F, typename I>
    inline F atan2kf(F b, F a)
    {
        I q = select(a < 0.0, I(-2), I(0));
        F x = abs(a);
        F y = b;

        q = select(x < y, q + 1, q);
        F s = select(x < y, -x, y);
        F t = max(x, y);

        s = s / t;
        t = s * s;

        F u(-1.88796008463073496563746e-05);
        u = madd( 0.000209850076645816976906797, u, t);
        u = madd(-0.00110611831486672482563471, u, t);
        u = madd( 0.00370026744188713119232403, u, t);
        u = madd(-0.00889896195887655491740809, u, t);
        u = madd( 0.016599329773529201970117, u, t);
        u = madd(-0.0254517624932312641616861, u, t);
        u = madd( 0.0337852580001353069993897, u, t);
        u = madd(-0.0407629191276836500001934, u, t);
        u = madd( 0.0466667150077840625632675, u, t);
        u = madd(-0.0523674852303482457616113, u, t);
        u = madd( 0.0587666392926673580854313, u, t);
        u = madd(-0.0666573579361080525984562, u, t);
        u = madd( 0.0769219538311769618355029, u, t);
        u = madd(-0.090908995008245008229153, u, t);
        u = madd( 0.111111105648261418443745, u, t);
        u = madd(-0.14285714266771329383765, u, t);
        u = madd( 0.199999999996591265594148, u, t);
        u = madd(-0.333333333333311110369124, u, t);

        t = madd(s, s, t * u);
        t = madd(t, convert<F>(q), F(double_pi_2));

        return t;
    }

    // API

    template <typename F, typename I>
    F sin(F d)
    {
        F u = round(d * double_1_pi);

        d = madd(d, u, -4.0 * double_pi4_a);
        d = madd(d, u, -4.0 * double_pi4_b);
        d = madd(d, u, -4.0 * double_pi4_c);
        d = madd(d, u, -4.0 * double_pi4_d);

        d = select((u & 1ll) != 0, -d, d);

        const F s = d * d;

        u = F(-7.97255955009037868891952e-18);
        u = madd( 2.81009972710863200091251e-15, u, s);
        u = madd(-7.64712219118158833288484e-13, u, s);
        u = madd( 1.60590430605664501629054e-10, u, s);
        u = madd(-2.50521083763502045810755e-08, u, s);
        u = madd( 2.75573192239198747630416e-06, u, s);
        u = madd(-0.000198412698412696162806809, u, s);
        u = madd( 0.00833333333333332974823815, u, s);
        u = madd(-0.166666666666666657414808, u, s);

        u = madd(d, s, u * d);
        u = is_inf<F, I>(d) | u;
        return u;
    }

    template <typename F, typename I>
    F cos(F d)
    {
        F u = round(madd(F(-0.5), d, double_1_pi)) + 1.0;

        d = madd(d, u, -2.0 * double_pi4_a);
        d = madd(d, u, -2.0 * double_pi4_b);
        d = madd(d, u, -2.0 * double_pi4_c);
        d = madd(d, u, -2.0 * double_pi4_d);

        d = select((u & 2ll) != 0, d, -d);

        const F s = d * d;

        u = F(-7.97255955009037868891952e-18);
        u = madd( 2.81009972710863200091251e-15, u, s);
        u = madd(-7.64712219118158833288484e-13, u, s);
        u = madd( 1.60590430605664501629054e-10, u, s);
        u = madd(-2.50521083763502045810755e-08, u, s);
        u = madd( 2.75573192239198747630416e-06, u, s);
        u = madd(-0.000198412698412696162806809, u, s);
        u = madd( 0.00833333333333332974823815, u, s);
        u = madd(-0.166666666666666657414808, u, s);

        u = madd(d, s, u * d);
        u = is_inf<F, I>(d) | u;
        return u;
    }

    template <typename F, typename I>
    F tan(F d)
    {
        const I q = convert<I>(d * double_2_pi);

        F x = d;
        F u = convert<F>(q);
        x = madd(x, u, -2.0 * double_pi4_a);
        x = madd(x, u, -2.0 * double_pi4_b);
        x = madd(x, u, -2.0 * double_pi4_c);
        x = madd(x, u, -2.0 * double_pi4_d);

        const F s = x * x;
        const auto mask = (q & 1) == 1;
        x = select(mask, -x, x);

        u = F(0.00927245803177356719970703);
        u = madd(0.00331984995864331722259521, u, s);
        u = madd(0.0242998078465461730957031, u, s);
        u = madd(0.0534495301544666290283203, u, s);
        u = madd(0.133383005857467651367188, u, s);
        u = madd(0.333331853151321411132812, u, s);
        u = madd(x, s, u * x);
        u = select(mask, 1.0 / u, u);
        u = is_inf<F, I>(d) | u;
        return u;
    }

    template <typename F, typename I>
    F exp(F v)
    {
        const I q = convert<I>(v * double_r_ln2);
        const F p = convert<F>(q);

        F s;
        s = madd(v, p, -double_l2u);
        s = madd(s, p, -double_l2l);

        F u(0.00136324646882712841033936);
        u = madd(0.00836596917361021041870117, u, s);
        u = madd(0.0416710823774337768554688, u, s);
        u = madd(0.166665524244308471679688, u, s);
        u = madd(0.499999850988388061523438, u, s);

        u = madd(s, s * s, u) + 1.0;
        u = ldexp(u, q);
        u = nand(is_negative_inf<F, I>(v), u);

        return u;
    }

    template <typename F, typename I>
    F exp2(F v)
    {
        const F fx = v + reinterpret<F>(nand(srai64(convert<I>(v), 31), 0x3f7fffff));
        const I ix = truncate<I>(fx);
        F f = (convert<F>(ix) - v) * 0.69314718055994530942;
        F hi = madd(0.0013298820, f, F(-0.0001413161));
        F lo = madd(0.4999999206, f, F(-0.1666653019));
        hi = madd(-0.0083013598, f, hi);
        hi = madd(0.0416573475, f, hi);
        lo = madd(-0.9999999995, f, lo);
        lo = madd(1.0f, f, lo);
        F f2 = f * f;
        F a = f2 * f2 * hi + lo;
        I xxx = select(ix > -128, I(0xffffffff), I(0));
        F b = reinterpret<F>(simd::slli((ix + 127), 23) & xxx);
        I mask = select(ix > 128, I(0x7fffffff), I(0));
        return (a * b) | reinterpret<F>(mask);
    }

    template <typename F, typename I>
    F log2(F v)
    {
        const I exponent = simd::srli(reinterpret<I>(v) & 0x7fffffff, 23) - 127;
        const F x = reinterpret<F>(reinterpret<I>(v) - simd::slli(exponent, 23)) - 1.0f;
        const F x2 = x * x;
        const F x4 = x2 * x2;
        F hi(-0.00931049621349);
        F lo( 0.47868480909345);
        hi = madd( 0.05206469089414, x, hi);
        lo = madd(-0.72116591947498, x, lo);
        hi = madd(-0.13753123777116, x, hi);
        hi = madd( 0.24187369696082, x, hi);
        hi = madd(-0.34730547155299, x, hi);
        lo = madd(1.442689881667200, x, lo);
        const F u = madd(convert<F>(exponent), x, lo);
        return madd(u, x4, hi);
    }

    template <typename F, typename I>
    F asin(F d)
    {
        const F one(1.0);
        F x = one + d;
        F y = one - d;
        x = x * y;
        x = sqrt(x);
        x = is_nan<F, I>(x) | atan2kf<F, I>(abs(d), x);
        return mulsign(x, d);
    }

    template <typename F, typename I>
    F acos(F d)
    {
        const F zero(0.0);
        const F one(1.0);

        F x = one + d;
        F y = one - d;
        x = x * y;
        x = sqrt(x);

        F absd = abs(d);
        x = mulsign<F>(atan2kf<F, I>(x, absd), d);
        y = select(d < zero, F(double_pi), zero);
        return x + y;
    }

    template <typename F, typename I>
    F atan(F s)
    {
        const F zero(0.0);
        const F one(1.0);

        I q = select(s < zero, I(2), I(0));
        s = abs(s);

        q = select(s > 1, q + 1, q);
        s = select(s > 1, 1.0 / s, s);
        F t = s * s;

        F u = F(-1.88796008463073496563746e-05);
        u = madd( 0.000209850076645816976906797, u, t);
        u = madd(-0.00110611831486672482563471, u, t);
        u = madd( 0.00370026744188713119232403, u, t);
        u = madd(-0.00889896195887655491740809, u, t);
        u = madd( 0.016599329773529201970117, u, t);
        u = madd(-0.0254517624932312641616861, u, t);
        u = madd( 0.0337852580001353069993897, u, t);
        u = madd(-0.0407629191276836500001934, u, t);
        u = madd( 0.0466667150077840625632675, u, t);
        u = madd(-0.0523674852303482457616113, u, t);
        u = madd( 0.0587666392926673580854313, u, t);
        u = madd(-0.0666573579361080525984562, u, t);
        u = madd( 0.0769219538311769618355029, u, t);
        u = madd(-0.090908995008245008229153, u, t);
        u = madd( 0.111111105648261418443745, u, t);
        u = madd(-0.14285714266771329383765, u, t);
        u = madd( 0.199999999996591265594148, u, t);
        u = madd(-0.333333333333311110369124, u, t);

        t = madd(s, s, t * u);

        t = select((q & 1) == 1, F(double_pi_2) - t, t);
        t = select((q & 2) == 2, -t, t);

        return t;
    }

    template <typename F, typename I>
    F atan2(F y, F x)
    {
        const F pi_1(double_pi);
        const F pi_2(double_pi_2);
        const F pi_4(double_pi_4);

        F r = atan2kf<F, I>(abs(y), x);
        r = mulsign<F>(r, x);
        r = select(abs(x) == double_r_inf, pi_2 - is_inf2<F, I>(x, mulsign<F>(pi_2, x)), r);
        r = select(abs(y) == double_r_inf, pi_2 - is_inf2<F, I>(x, mulsign<F>(pi_4, x)), r);
        r = select(y == 0.0f, select(signed_one(x) == -1.0, pi_1, F(0.0)), r);
        r = is_nan<F, I>(x) | is_nan<F, I>(y) | mulsign(r, y);
        return r;
    }

    template <typename F, typename I>
    F pow(F a, F b)
    {
        F temp = log2<F, I>(abs(a)) * b;
        return exp2<F, I>(temp);
    }

} // namespace fp64

#endif // 0

namespace mango::math
{

    // ------------------------------------------------------------------------
    // float32x4
    // ------------------------------------------------------------------------

    float32x4 sin(float32x4 v)
    {
        return fp32::sin<float32x4, int32x4>(v);
    }

    float32x4 cos(float32x4 v)
    {
        return fp32::cos<float32x4, int32x4>(v);
    }

    float32x4 tan(float32x4 v)
    {
        return fp32::tan<float32x4, int32x4>(v);
    }

    float32x4 exp(float32x4 v)
    {
        return fp32::exp<float32x4, int32x4>(v);
    }

    float32x4 exp2(float32x4 v)
    {
        return fp32::exp2<float32x4, int32x4>(v);
    }

    float32x4 log(float32x4 v)
    {
        return fp32::log2<float32x4, int32x4>(v) * 0.69314718055995f;
    }

    float32x4 log2(float32x4 v)
    {
        return fp32::log2<float32x4, int32x4>(v);
    }

    float32x4 asin(float32x4 v)
    {
        return fp32::asin<float32x4, int32x4>(v);
    }

    float32x4 acos(float32x4 v)
    {
        return fp32::acos<float32x4, int32x4>(v);
    }

    float32x4 atan(float32x4 v)
    {
        return fp32::atan<float32x4, int32x4>(v);
    }

    float32x4 atan2(float32x4 y, float32x4 x)
    {
        return fp32::atan2<float32x4, int32x4>(y, x);
    }

    float32x4 pow(float32x4 a, float32x4 b)
    {
        return fp32::pow<float32x4, int32x4>(a, b);
    }

    // ------------------------------------------------------------------------
    // float32x8
    // ------------------------------------------------------------------------

    float32x8 sin(float32x8 v)
    {
        return fp32::sin<float32x8, int32x8>(v);
    }

    float32x8 cos(float32x8 v)
    {
        return fp32::cos<float32x8, int32x8>(v);
    }

    float32x8 tan(float32x8 v)
    {
        return fp32::tan<float32x8, int32x8>(v);
    }

    float32x8 exp(float32x8 v)
    {
        return fp32::exp<float32x8, int32x8>(v);
    }

    float32x8 exp2(float32x8 v)
    {
        return fp32::exp2<float32x8, int32x8>(v);
    }

    float32x8 log(float32x8 v)
    {
        return fp32::log2<float32x8, int32x8>(v) * 0.69314718055995f;
    }

    float32x8 log2(float32x8 v)
    {
        return fp32::log2<float32x8, int32x8>(v);
    }

    float32x8 asin(float32x8 v)
    {
        return fp32::asin<float32x8, int32x8>(v);
    }

    float32x8 acos(float32x8 v)
    {
        return fp32::acos<float32x8, int32x8>(v);
    }

    float32x8 atan(float32x8 v)
    {
        return fp32::atan<float32x8, int32x8>(v);
    }

    float32x8 atan2(float32x8 y, float32x8 x)
    {
        return fp32::atan2<float32x8, int32x8>(y, x);
    }

    float32x8 pow(float32x8 a, float32x8 b)
    {
        return fp32::pow<float32x8, int32x8>(a, b);
    }

    // ------------------------------------------------------------------------
    // float32x16
    // ------------------------------------------------------------------------

    float32x16 sin(float32x16 v)
    {
        return fp32::sin<float32x16, int32x16>(v);
    }

    float32x16 cos(float32x16 v)
    {
        return fp32::cos<float32x16, int32x16>(v);
    }

    float32x16 tan(float32x16 v)
    {
        return fp32::tan<float32x16, int32x16>(v);
    }

    float32x16 exp(float32x16 v)
    {
        return fp32::exp<float32x16, int32x16>(v);
    }

    float32x16 exp2(float32x16 v)
    {
        return fp32::exp2<float32x16, int32x16>(v);
    }

    float32x16 log(float32x16 v)
    {
        return fp32::log2<float32x16, int32x16>(v) * 0.69314718055995f;
    }

    float32x16 log2(float32x16 v)
    {
        return fp32::log2<float32x16, int32x16>(v);
    }

    float32x16 asin(float32x16 v)
    {
        return fp32::asin<float32x16, int32x16>(v);
    }

    float32x16 acos(float32x16 v)
    {
        return fp32::acos<float32x16, int32x16>(v);
    }

    float32x16 atan(float32x16 v)
    {
        return fp32::atan<float32x16, int32x16>(v);
    }

    float32x16 atan2(float32x16 y, float32x16 x)
    {
        return fp32::atan2<float32x16, int32x16>(y, x);
    }

    float32x16 pow(float32x16 a, float32x16 b)
    {
        return fp32::pow<float32x16, int32x16>(a, b);
    }

#if 0

    // ------------------------------------------------------------------------
    // float64x2
    // ------------------------------------------------------------------------

    float64x2 sin(float64x2 v)
    {
        return fp64::sin<float64x2, int64x2>(v);
    }

    float64x2 cos(float64x2 v)
    {
        return fp64::cos<float64x2, int64x2>(v);
    }

    float64x2 tan(float64x2 v)
    {
        return fp64::tan<float64x2, int64x2>(v);
    }

    float64x2 exp(float64x2 v)
    {
        return fp64::exp<float64x2, int64x2>(v);
    }

    float64x2 exp2(float64x2 v)
    {
        return fp64::exp2<float64x2, int64x2>(v);
    }

    float64x2 log(float64x2 v)
    {
        return fp64::log2<float64x2, int64x2>(v) * 0.69314718055995;
    }

    float64x2 log2(float64x2 v)
    {
        return fp64::log2<float64x2, int64x2>(v);
    }

    float64x2 asin(float64x2 v)
    {
        return fp64::asin<float64x2, int64x2>(v);
    }

    float64x2 acos(float64x2 v)
    {
        return fp64::acos<float64x2, int64x2>(v);
    }

    float64x2 atan(float64x2 v)
    {
        return fp64::atan<float64x2, int64x2>(v);
    }

    float64x2 atan2(float64x2 y, float64x2 x)
    {
        return fp64::atan2<float64x2, int64x2>(y, x);
    }

    float64x2 pow(float64x2 a, float64x2 b)
    {
        return fp64::pow<float64x2, int64x2>(a, b);
    }

    // ------------------------------------------------------------------------
    // float64x4
    // ------------------------------------------------------------------------

    float64x4 sin(float64x4 v)
    {
        return fp64::sin<float64x4, int64x4>(v);
    }

    float64x4 cos(float64x4 v)
    {
        return fp64::cos<float64x4, int64x4>(v);
    }

    float64x4 tan(float64x4 v)
    {
        return fp64::tan<float64x4, int64x4>(v);
    }

    float64x4 exp(float64x4 v)
    {
        return fp64::exp<float64x4, int64x4>(v);
    }

    float64x4 exp2(float64x4 v)
    {
        return fp64::exp2<float64x4, int64x4>(v);
    }

    float64x4 log(float64x4 v)
    {
        return fp64::log2<float64x4, int64x4>(v) * 0.69314718055995;
    }

    float64x4 log2(float64x4 v)
    {
        return fp64::log2<float64x4, int64x4>(v);
    }

    float64x4 asin(float64x4 v)
    {
        return fp64::asin<float64x4, int64x4>(v);
    }

    float64x4 acos(float64x4 v)
    {
        return fp64::acos<float64x4, int64x4>(v);
    }

    float64x4 atan(float64x4 v)
    {
        return fp64::atan<float64x4, int64x4>(v);
    }

    float64x4 atan2(float64x4 y, float64x4 x)
    {
        return fp64::atan2<float64x4, int64x4>(y, x);
    }

    float64x4 pow(float64x4 a, float64x4 b)
    {
        return fp64::pow<float64x4, int64x4>(a, b);
    }

    // ------------------------------------------------------------------------
    // float64x8
    // ------------------------------------------------------------------------

    float64x8 sin(float64x8 v)
    {
        return fp64::sin<float64x8, int64x8>(v);
    }

    float64x8 cos(float64x8 v)
    {
        return fp64::cos<float64x8, int64x8>(v);
    }

    float64x8 tan(float64x8 v)
    {
        return fp64::tan<float64x8, int64x8>(v);
    }

    float64x8 exp(float64x8 v)
    {
        return fp64::exp<float64x8, int64x8>(v);
    }

    float64x8 exp2(float64x8 v)
    {
        return fp64::exp2<float64x8, int64x8>(v);
    }

    float64x8 log(float64x8 v)
    {
        return fp64::log2<float64x8, int64x8>(v) * 0.69314718055995;
    }

    float64x8 log2(float64x8 v)
    {
        return fp64::log2<float64x8, int64x8>(v);
    }

    float64x8 asin(float64x8 v)
    {
        return fp64::asin<float64x8, int64x8>(v);
    }

    float64x8 acos(float64x8 v)
    {
        return fp64::acos<float64x8, int64x8>(v);
    }

    float64x8 atan(float64x8 v)
    {
        return fp64::atan<float64x8, int64x8>(v);
    }

    float64x8 atan2(float64x8 y, float64x8 x)
    {
        return fp64::atan2<float64x8, int64x8>(y, x);
    }

    float64x8 pow(float64x8 a, float64x8 b)
    {
        return fp64::pow<float64x8, int64x8>(a, b);
    }

#endif // 0

} // namespace mango::math

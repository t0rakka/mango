/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

namespace mango::math
{

    // ------------------------------------------------------------------
    // simd operator / function wrappers
    // ------------------------------------------------------------------

#define MATH_SIMD_FLOAT_OPERATORS(T, N, SIMD) \
    \
    static inline Vector<T, N> operator + (Vector<T, N> a) \
    { \
        return a; \
    } \
    \
    static inline Vector<T, N> operator - (Vector<T, N> a) \
    { \
        return simd::neg(a); \
    } \
    \
    static inline Vector<T, N>& operator += (Vector<T, N>& a, Vector<T, N> b) \
    { \
        a = simd::add(a, b); \
        return a; \
    } \
    \
    static inline Vector<T, N>& operator += (Vector<T, N>& a, T b) \
    { \
        a = simd::add(a, simd::SIMD##_set(b)); \
        return a; \
    } \
    \
    static inline Vector<T, N>& operator -= (Vector<T, N>& a, Vector<T, N> b) \
    { \
        a = simd::sub(a, b); \
        return a; \
    } \
    \
    static inline Vector<T, N>& operator -= (Vector<T, N>& a, T b) \
    { \
        a = simd::sub(a, simd::SIMD##_set(b)); \
        return a; \
    } \
    \
    static inline Vector<T, N>& operator *= (Vector<T, N>& a, Vector<T, N> b) \
    { \
        a = simd::mul(a, b); \
        return a; \
    } \
    \
    static inline Vector<T, N>& operator *= (Vector<T, N>& a, T b) \
    { \
        a = simd::mul(a, simd::SIMD##_set(b)); \
        return a; \
    } \
    \
    static inline Vector<T, N>& operator /= (Vector<T, N>& a, Vector<T, N> b) \
    { \
        a = simd::div(a, b); \
        return a; \
    } \
    \
    static inline Vector<T, N>& operator /= (Vector<T, N>& a, T b) \
    { \
        a = simd::div(a, b); \
        return a; \
    } \
    \
    static inline Vector<T, N> operator + (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::add(a, b); \
    } \
    \
    static inline Vector<T, N> operator + (Vector<T, N> a, T b) \
    { \
        return simd::add(a, simd::SIMD##_set(b)); \
    } \
    \
    static inline Vector<T, N> operator + (T a, Vector<T, N> b) \
    { \
        return simd::add(simd::SIMD##_set(a), b); \
    } \
    \
    static inline Vector<T, N> operator - (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::sub(a, b); \
    } \
    \
    static inline Vector<T, N> operator - (Vector<T, N> a, T b) \
    { \
        return simd::sub(a, simd::SIMD##_set(b)); \
    } \
    \
    static inline Vector<T, N> operator - (T a, Vector<T, N> b) \
    { \
        return simd::sub(simd::SIMD##_set(a), b); \
    } \
    \
    static inline Vector<T, N> operator * (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::mul(a, b); \
    } \
    \
    static inline Vector<T, N> operator / (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::div(a, b); \
    } \
    \
    static inline Vector<T, N> operator / (Vector<T, N> a, T b) \
    { \
        return simd::div(a, b); \
    }

#define MATH_SIMD_FLOAT_FUNCTIONS(T, N, SIMD, MASK) \
    \
    static inline Vector<T, N> add(Vector<T, N> a, Vector<T, N> b, MASK mask) \
    { \
        return simd::add(a, b, mask); \
    } \
    \
    static inline Vector<T, N> add(Vector<T, N> a, Vector<T, N> b, MASK mask, Vector<T, N> value) \
    { \
        return simd::add(a, b, mask, value); \
    } \
    \
    static inline Vector<T, N> sub(Vector<T, N> a, Vector<T, N> b, MASK mask) \
    { \
        return simd::sub(a, b, mask); \
    } \
    \
    static inline Vector<T, N> sub(Vector<T, N> a, Vector<T, N> b, MASK mask, Vector<T, N> value) \
    { \
        return simd::sub(a, b, mask, value); \
    } \
    \
    static inline Vector<T, N> mul(Vector<T, N> a, Vector<T, N> b, MASK mask) \
    { \
        return simd::mul(a, b, mask); \
    } \
    \
    static inline Vector<T, N> mul(Vector<T, N> a, Vector<T, N> b, MASK mask, Vector<T, N> value) \
    { \
        return simd::mul(a, b, mask, value); \
    } \
    \
    static inline Vector<T, N> div(Vector<T, N> a, Vector<T, N> b, MASK mask) \
    { \
        return simd::div(a, b, mask); \
    } \
    \
    static inline Vector<T, N> div(Vector<T, N> a, Vector<T, N> b, MASK mask, Vector<T, N> value) \
    { \
        return simd::div(a, b, mask, value); \
    } \
    \
    static inline Vector<T, N> abs(Vector<T, N> a) \
    { \
        return simd::abs(a); \
    } \
    \
    static inline Vector<T, N> round(Vector<T, N> a) \
    { \
        return simd::round(a); \
    } \
    \
    static inline Vector<T, N> floor(Vector<T, N> a) \
    { \
        return simd::floor(a); \
    } \
    \
    static inline Vector<T, N> ceil(Vector<T, N> a) \
    { \
        return simd::ceil(a); \
    } \
    \
    static inline Vector<T, N> trunc(Vector<T, N> a) \
    { \
        return simd::trunc(a); \
    } \
    \
    static inline Vector<T, N> fract(Vector<T, N> a) \
    { \
        return simd::fract(a); \
    } \
    \
    static inline Vector<T, N> mod(Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::sub(a, simd::mul(b, simd::floor(simd::div(a, b)))); \
    } \
    \
    static inline Vector<T, N> sign(Vector<T, N> a) \
    { \
        return simd::sign(a); \
    } \
    \
    static inline Vector<T, N> radians(Vector<T, N> a) \
    { \
        return simd::mul(a, simd::SIMD##_set(T(0.01745329251))); \
    } \
    \
    static inline Vector<T, N> degrees(Vector<T, N> a) \
    { \
        return simd::mul(a, simd::SIMD##_set(T(57.2957795131))); \
    } \
    \
    static inline Vector<T, N> sqrt(Vector<T, N> a) \
    { \
        return simd::sqrt(a); \
    } \
    \
    static inline Vector<T, N> rsqrt(Vector<T, N> a) \
    { \
        return simd::rsqrt(a); \
    } \
    \
    static inline Vector<T, N> rcp(Vector<T, N> a) \
    { \
        return simd::rcp(a); \
    } \
    \
    static inline Vector<T, N> unpacklo(Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::unpacklo(a, b); \
    } \
    \
    static inline Vector<T, N> unpackhi(Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::unpackhi(a, b); \
    } \
    \
    static inline Vector<T, N> min(Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::min(a, b); \
    } \
    \
    static inline Vector<T, N> min(Vector<T, N> a, Vector<T, N> b, MASK mask) \
    { \
        return simd::min(a, b, mask); \
    } \
    \
    static inline Vector<T, N> min(Vector<T, N> a, Vector<T, N> b, MASK mask, Vector<T, N> value) \
    { \
        return simd::min(a, b, mask, value); \
    } \
    \
    static inline Vector<T, N> max(Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::max(a, b); \
    } \
    \
    static inline Vector<T, N> max(Vector<T, N> a, Vector<T, N> b, MASK mask) \
    { \
        return simd::max(a, b, mask); \
    } \
    \
    static inline Vector<T, N> max(Vector<T, N> a, Vector<T, N> b, MASK mask, Vector<T, N> value) \
    { \
        return simd::max(a, b, mask, value); \
    } \
    \
    static inline Vector<T, N> madd(Vector<T, N> a, Vector<T, N> b, Vector<T, N> c) \
    { \
        return simd::madd(a, b, c); \
    } \
    \
    static inline Vector<T, N> msub(Vector<T, N> a, Vector<T, N> b, Vector<T, N> c) \
    { \
        return simd::msub(a, b, c); \
    } \
    \
    static inline Vector<T, N> nmadd(Vector<T, N> a, Vector<T, N> b, Vector<T, N> c) \
    { \
        return simd::nmadd(a, b, c); \
    } \
    \
    static inline Vector<T, N> nmsub(Vector<T, N> a, Vector<T, N> b, Vector<T, N> c) \
    { \
        return simd::nmsub(a, b, c); \
    } \
    \
    static inline Vector<T, N> clamp(Vector<T, N> a, Vector<T, N> low, Vector<T, N> high) \
    { \
        return simd::min(high, simd::max(low, a)); \
    } \
    \
    static inline Vector<T, N> lerp(Vector<T, N> a, Vector<T, N> b, float factor) \
    { \
        Vector<T, N> s(factor); \
        return simd::lerp(a, b, s); \
    } \
    \
    static inline Vector<T, N> lerp(Vector<T, N> a, Vector<T, N> b, Vector<T, N> factor) \
    { \
        return simd::lerp(a, b, factor); \
    }

#define MATH_SIMD_SIGNED_INTEGER_OPERATORS(T, N) \
    \
    static inline Vector<T, N> operator - (Vector<T, N> a) \
    { \
        return simd::neg(a); \
    }

#define MATH_SIMD_UNSIGNED_INTEGER_OPERATORS(T, N) \
    \
    static inline Vector<T, N> operator + (Vector<T, N> a) \
    { \
        return a; \
    } \
    \
    static inline Vector<T, N>& operator += (Vector<T, N>& a, Vector<T, N> b) \
    { \
        a = simd::add(a, b); \
        return a; \
    } \
    \
    static inline Vector<T, N>& operator += (Vector<T, N>& a, T b) \
    { \
        a = simd::add(a, simd::T##x##N##_set(b)); \
        return a; \
    } \
    \
    static inline Vector<T, N>& operator -= (Vector<T, N>& a, Vector<T, N> b) \
    { \
        a = simd::sub(a, b); \
        return a; \
    } \
    \
    static inline Vector<T, N>& operator -= (Vector<T, N>& a, T b) \
    { \
        a = simd::sub(a, simd::T##x##N##_set(b)); \
        return a; \
    } \
    \
    static inline Vector<T, N> operator + (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::add(a, b); \
    } \
    \
    static inline Vector<T, N> operator + (Vector<T, N> a, T b) \
    { \
        return simd::add(a, simd::T##x##N##_set(b)); \
    } \
    \
    static inline Vector<T, N> operator + (T a, Vector<T, N> b) \
    { \
        return simd::add(simd::T##x##N##_set(a), b); \
    } \
    \
    static inline Vector<T, N> operator - (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::sub(a, b); \
    } \
    \
    static inline Vector<T, N> operator - (Vector<T, N> a, T b) \
    { \
        return simd::sub(a, simd::T##x##N##_set(b)); \
    } \
    \
    static inline Vector<T, N> operator - (T a, Vector<T, N> b) \
    { \
        return simd::sub(simd::T##x##N##_set(a), b); \
    }

#define MATH_SIMD_INTEGER_FUNCTIONS(T, N, MASK) \
    \
    static inline Vector<T, N> unpacklo(Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::unpacklo(a, b); \
    } \
    \
    static inline Vector<T, N> unpackhi(Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::unpackhi(a, b); \
    } \
    \
    static inline Vector<T, N> add(Vector<T, N> a, Vector<T, N> b, MASK mask) \
    { \
        return simd::add(a, b, mask); \
    } \
    \
    static inline Vector<T, N> add(Vector<T, N> a, Vector<T, N> b, MASK mask, Vector<T, N> value) \
    { \
        return simd::add(a, b, mask, value); \
    } \
    \
    static inline Vector<T, N> sub(Vector<T, N> a, Vector<T, N> b, MASK mask) \
    { \
        return simd::sub(a, b, mask); \
    } \
    \
    static inline Vector<T, N> sub(Vector<T, N> a, Vector<T, N> b, MASK mask, Vector<T, N> value) \
    { \
        return simd::sub(a, b, mask, value); \
    } \
    \
    static inline Vector<T, N> min(Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::min(a, b); \
    } \
    \
    static inline Vector<T, N> min(Vector<T, N> a, Vector<T, N> b, MASK mask) \
    { \
        return simd::min(a, b, mask); \
    } \
    \
    static inline Vector<T, N> min(Vector<T, N> a, Vector<T, N> b, MASK mask, Vector<T, N> value) \
    { \
        return simd::min(a, b, mask, value); \
    } \
    \
    static inline Vector<T, N> max(Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::max(a, b); \
    } \
    \
    static inline Vector<T, N> max(Vector<T, N> a, Vector<T, N> b, MASK mask) \
    { \
        return simd::max(a, b, mask); \
    } \
    \
    static inline Vector<T, N> max(Vector<T, N> a, Vector<T, N> b, MASK mask, Vector<T, N> value) \
    { \
        return simd::max(a, b, mask, value); \
    } \
    \
    static inline Vector<T, N> clamp(Vector<T, N> a, Vector<T, N> low, Vector<T, N> high) \
    { \
        return simd::min(high, simd::max(low, a)); \
    }

#define MATH_SIMD_SATURATING_INTEGER_FUNCTIONS(T, N, MASK) \
    \
    static inline Vector<T, N> adds(Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::adds(a, b); \
    } \
    \
    static inline Vector<T, N> adds(Vector<T, N> a, Vector<T, N> b, MASK mask) \
    { \
        return simd::adds(a, b, mask); \
    } \
    \
    static inline Vector<T, N> adds(Vector<T, N> a, Vector<T, N> b, MASK mask, Vector<T, N> value) \
    { \
        return simd::adds(a, b, mask, value); \
    } \
    \
    static inline Vector<T, N> subs(Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::subs(a, b); \
    } \
    \
    static inline Vector<T, N> subs(Vector<T, N> a, Vector<T, N> b, MASK mask) \
    { \
        return simd::subs(a, b, mask); \
    } \
    \
    static inline Vector<T, N> subs(Vector<T, N> a, Vector<T, N> b, MASK mask, Vector<T, N> value) \
    { \
        return simd::subs(a, b, mask, value); \
    }

#define MATH_SIMD_ABS_INTEGER_FUNCTIONS(T, N, MASK) \
    \
    static inline Vector<T, N> abs(Vector<T, N> a) \
    { \
        return simd::abs(a); \
    } \
    \
    static inline Vector<T, N> abs(Vector<T, N> a, MASK mask) \
    { \
        return simd::abs(a, mask); \
    } \
    \
    static inline Vector<T, N> abs(Vector<T, N> a, MASK mask, Vector<T, N> value) \
    { \
        return simd::abs(a, mask, value); \
    }

#define MATH_SIMD_BITWISE_FUNCTIONS(T, N) \
    \
    static inline Vector<T, N> nand(Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::bitwise_nand(a, b); \
    } \
    \
    static inline Vector<T, N> operator & (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::bitwise_and(a, b); \
    } \
    \
    static inline Vector<T, N> operator | (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::bitwise_or(a, b); \
    } \
    \
    static inline Vector<T, N> operator ^ (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::bitwise_xor(a, b); \
    } \
    \
    static inline Vector<T, N> operator ~ (Vector<T, N> a) \
    { \
        return simd::bitwise_not(a); \
    }

#define MATH_SIMD_COMPARE_FUNCTIONS(T, N, MASK) \
    \
    static inline MASK operator > (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::compare_gt(a, b); \
    } \
    \
    static inline MASK operator >= (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::compare_ge(a, b); \
    } \
    \
    static inline MASK operator < (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::compare_lt(a, b); \
    } \
    \
    static inline MASK operator <= (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::compare_le(a, b); \
    } \
    \
    static inline MASK operator == (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::compare_eq(a, b); \
    } \
    \
    static inline MASK operator != (Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::compare_neq(a, b); \
    } \
    \
    static inline Vector<T, N> select(MASK mask, Vector<T, N> a, Vector<T, N> b) \
    { \
        return simd::select(mask, a, b); \
    }

} // namespace mango::math

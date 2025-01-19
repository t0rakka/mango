/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

namespace mango::math
{

    // ------------------------------------------------------------------
    // reinterpret / convert
    // ------------------------------------------------------------------

    // The reinterpret and conversion casts forward the work to the simd abstraction.
    // This is enforced by requiring "VectorType" declaration in the Vector specialization.

    template <typename D, typename S>
    static inline D reinterpret(S s)
    {
        typename S::VectorType temp = s;
        return simd::reinterpret<typename D::VectorType>(temp);
    }

    template <typename D, typename S>
    static inline D convert(S s)
    {
        typename S::VectorType temp = s;
        return simd::convert<typename D::VectorType>(temp);
    }

    template <typename D, typename S>
    static inline D truncate(S s)
    {
        typename S::VectorType temp = s;
        return simd::truncate<typename D::VectorType>(temp);
    }

    // ------------------------------------------------------------------
    // specializations
    // ------------------------------------------------------------------

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize> load_low(const ScalarType *source)
    {
        MANGO_UNREFERENCED(source);

        // load_low() is not available by default
        Vector<ScalarType, VectorSize>::undefined_operation();
    }

    // ------------------------------------------------------------------
    // ScalarAccessor
    // ------------------------------------------------------------------

    template <typename ScalarType, typename VectorType, int Index>
    struct ScalarAccessor
    {
        VectorType m;

        ScalarAccessor(const ScalarAccessor& accessor) = default;

        operator ScalarType () const
        {
            return simd::get_component<Index>(m);
        }

        template <int VectorSize>
        operator Vector<ScalarType, VectorSize> () const
        {
            return Vector<ScalarType, VectorSize>(simd::get_component<Index>(m));
        }

        ScalarAccessor& operator = (const ScalarAccessor& accessor)
        {
            m = simd::set_component<Index>(m, accessor);
            return *this;
        }

        ScalarAccessor& operator = (ScalarType s)
        {
            m = simd::set_component<Index>(m, s);
            return *this;
        }

        ScalarAccessor& operator += (ScalarType s)
        {
            *this = ScalarType(*this) + s;
            return *this;
        }

        ScalarAccessor& operator -= (ScalarType s)
        {
            *this = ScalarType(*this) - s;
            return *this;
        }

        ScalarAccessor& operator *= (ScalarType s)
        {
            *this = ScalarType(*this) * s;
            return *this;
        }

        ScalarAccessor& operator /= (ScalarType s)
        {
            *this = ScalarType(*this) / s;
            return *this;
        }
    };

    // operator +

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator + (const ScalarAccessor<ScalarType, VectorType, Index>& a)
    {
        // +a
        return ScalarType(a);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    ScalarType operator + (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        // a + a
        return ScalarType(a) + ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator + (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b)
    {
        // a + s
        return ScalarType(a) + b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator + (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b)
    {
        // s + a
        return a + ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    Vector<ScalarType, N> operator + (const ScalarAccessor<ScalarType, VectorType, Index>& a, Vector<ScalarType, N> b)
    {
        // a + v
        return ScalarType(a) + b;
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    Vector<ScalarType, N> operator + (Vector<ScalarType, N> a, const ScalarAccessor<ScalarType, VectorType, Index>& b)
    {
        // v + a
        return a + ScalarType(b);
    }

    // operator -

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator - (const ScalarAccessor<ScalarType, VectorType, Index>& a)
    {
        // -a
        return -ScalarType(a);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    ScalarType operator - (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        // a - a
        return ScalarType(a) - ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator - (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b)
    {
        // a - s
        return ScalarType(a) - b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator - (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b)
    {
        // s - a
        return a - ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    Vector<ScalarType, N> operator - (const ScalarAccessor<ScalarType, VectorType, Index>& a, Vector<ScalarType, N> b)
    {
        // a - v
        return ScalarType(a) - b;
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    Vector<ScalarType, N> operator - (Vector<ScalarType, N> a, const ScalarAccessor<ScalarType, VectorType, Index>& b)
    {
        // v - a
        return a - ScalarType(b);
    }

    // operator *

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    ScalarType operator * (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        // a * a
        return ScalarType(a) * ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator * (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b)
    {
        // a * s
        return ScalarType(a) * b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator * (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b)
    {
        // s * a
        return a * ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    Vector<ScalarType, N> operator * (const ScalarAccessor<ScalarType, VectorType, Index>& a, Vector<ScalarType, N> b)
    {
        // a * v
        return ScalarType(a) * b;
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    Vector<ScalarType, N> operator * (Vector<ScalarType, N> a, const ScalarAccessor<ScalarType, VectorType, Index>& b)
    {
        // v * a
        return a * ScalarType(b);
    }

    // operator /

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    ScalarType operator / (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        // a / a
        return ScalarType(a) / ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator / (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b)
    {
        // a / s
        return ScalarType(a) / b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator / (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b)
    {
        // s / a
        return a / ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    Vector<ScalarType, N> operator / (const ScalarAccessor<ScalarType, VectorType, Index>& a, Vector<ScalarType, N> b)
    {
        // a / v
        return ScalarType(a) / b;
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    Vector<ScalarType, N> operator / (Vector<ScalarType, N> a, const ScalarAccessor<ScalarType, VectorType, Index>& b)
    {
        // v / a
        return a / ScalarType(b);
    }

    // operator *=

    template <typename ScalarType, typename VectorType, int Index, int N>
    static inline
    Vector<ScalarType, N>& operator *= (Vector<ScalarType, N>& a, ScalarAccessor<ScalarType, VectorType, Index> b)
    {
        a = a * ScalarType(b);
        return a;
    }

    // operator /=

    template <typename ScalarType, typename VectorType, int Index, int N>
    static inline
    Vector<ScalarType, N>& operator /= (Vector<ScalarType, N>& a, ScalarAccessor<ScalarType, VectorType, Index> b)
    {
        a = a / ScalarType(b);
        return a;
    }

    // compare

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    bool operator < (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                     const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        return ScalarType(a) < ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    bool operator > (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                     const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        return ScalarType(a) > ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    bool operator <= (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                      const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        return ScalarType(a) <= ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    bool operator >= (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                      const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        return ScalarType(a) >= ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    bool operator == (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                      const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        return ScalarType(a) == ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    bool operator != (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                      const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        return ScalarType(a) != ScalarType(b);
    }

    // ------------------------------------------------------------------
    // LowAccessor
    // ------------------------------------------------------------------

    template <typename LowType, typename VectorType>
    struct LowAccessor
    {
        VectorType m;

        operator LowType () const
        {
            return simd::get_low(m);
        }

        void operator = (LowType low)
        {
            m = simd::set_low(m, low);
        }
    };

    // ------------------------------------------------------------------
    // HighAccessor
    // ------------------------------------------------------------------

    template <typename HighType, typename VectorType>
    struct HighAccessor
    {
        VectorType m;

        operator HighType () const
        {
            return simd::get_high(m);
        }

        void operator = (HighType high)
        {
            m = simd::set_high(m, high);
        }
    };

    // ------------------------------------------------------------------
    // ShuffleAccessor2
    // ------------------------------------------------------------------

    template <typename ScalarType, typename VectorType, int X, int Y>
    struct ShuffleAccessor2
    {
        VectorType m;

        operator Vector<ScalarType, 2> () const
        {
            return simd::shuffle<X, Y>(m);
        }
    };

    // ------------------------------------------------------------------
    // ShuffleAccessor4x2
    // ------------------------------------------------------------------

    template <typename ScalarType, typename VectorType, int X, int Y>
    struct ShuffleAccessor4x2
    {
        VectorType m;

        operator Vector<ScalarType, 2> () const
        {
            const ScalarType x = simd::get_component<X>(m);
            const ScalarType y = simd::get_component<Y>(m);
            return Vector<ScalarType, 2>(x, y);
        }
    };

    // ------------------------------------------------------------------
    // ShuffleAccessor4x3
    // ------------------------------------------------------------------

    template <typename ScalarType, typename VectorType, int X, int Y, int Z>
    struct ShuffleAccessor4x3
    {
        VectorType m;

        operator Vector<ScalarType, 3> () const
        {
            const ScalarType x = simd::get_component<X>(m);
            const ScalarType y = simd::get_component<Y>(m);
            const ScalarType z = simd::get_component<Z>(m);
            return Vector<ScalarType, 3>(x, y, z);
        }
    };

    // ------------------------------------------------------------------
    // ShuffleAccessor4
    // ------------------------------------------------------------------

    template <typename ScalarType, typename VectorType, int X, int Y, int Z, int W>
    struct ShuffleAccessor4
    {
        VectorType m;

        operator Vector<ScalarType, 4> () const
        {
            return simd::shuffle<X, Y, Z, W>(m);
        }

#if 0
        VectorType operator = (VectorType v)
        {
            m = simd::shuffle<X, Y, Z, W>(v);
            return m;
        }

        template <int A, int B, int C, int D>
        ShuffleAccessor4& operator = (const ShuffleAccessor4<ScalarType, VectorType, A, B, C, D>& v)
        {
            constexpr u32 mask = (D << 6) | (C << 4) | (B << 2) | A;
            constexpr u32 s0 = (mask >> (X * 2)) & 3;
            constexpr u32 s1 = (mask >> (Y * 2)) & 3;
            constexpr u32 s2 = (mask >> (Z * 2)) & 3;
            constexpr u32 s3 = (mask >> (W * 2)) & 3;
            m = simd::shuffle<s0, s1, s2, s3>(v.m);
            return *this;
        }
#endif
    };

    // ------------------------------------------------------------------
    // unaligned load / store
    // ------------------------------------------------------------------

#define MATH_LOAD_STORE_ALIAS(T) \
    static constexpr auto T##_uload = simd::T##_uload; \
    static constexpr auto T##_ustore = simd::T##_ustore

    MATH_LOAD_STORE_ALIAS(s32x2);
    MATH_LOAD_STORE_ALIAS(u32x2);

    MATH_LOAD_STORE_ALIAS(s8x16);
    MATH_LOAD_STORE_ALIAS(s16x8);
    MATH_LOAD_STORE_ALIAS(s32x4);
    MATH_LOAD_STORE_ALIAS(s64x2);
    MATH_LOAD_STORE_ALIAS(u8x16);
    MATH_LOAD_STORE_ALIAS(u16x8);
    MATH_LOAD_STORE_ALIAS(u32x4);
    MATH_LOAD_STORE_ALIAS(u64x2);

    MATH_LOAD_STORE_ALIAS(s8x32);
    MATH_LOAD_STORE_ALIAS(s16x16);
    MATH_LOAD_STORE_ALIAS(s32x8);
    MATH_LOAD_STORE_ALIAS(s64x4);
    MATH_LOAD_STORE_ALIAS(u8x32);
    MATH_LOAD_STORE_ALIAS(u16x16);
    MATH_LOAD_STORE_ALIAS(u32x8);
    MATH_LOAD_STORE_ALIAS(u64x4);

    MATH_LOAD_STORE_ALIAS(s8x64);
    MATH_LOAD_STORE_ALIAS(s16x32);
    MATH_LOAD_STORE_ALIAS(s32x16);
    MATH_LOAD_STORE_ALIAS(s64x8);
    MATH_LOAD_STORE_ALIAS(u8x64);
    MATH_LOAD_STORE_ALIAS(u16x32);
    MATH_LOAD_STORE_ALIAS(u32x16);
    MATH_LOAD_STORE_ALIAS(u64x8);

    MATH_LOAD_STORE_ALIAS(f32x2);
    MATH_LOAD_STORE_ALIAS(f32x4);
    MATH_LOAD_STORE_ALIAS(f32x8);
    MATH_LOAD_STORE_ALIAS(f32x16);

    MATH_LOAD_STORE_ALIAS(f64x2);
    MATH_LOAD_STORE_ALIAS(f64x4);
    MATH_LOAD_STORE_ALIAS(f64x8);

#undef MATH_LOAD_STORE_ALIAS

    // ------------------------------------------------------------------
    // maskToInt()
    // ------------------------------------------------------------------

    static inline u32 maskToInt(mask8x16 mask)
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask16x8 mask)
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask32x4 mask)
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask64x2 mask)
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask8x32 mask)
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask16x16 mask)
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask32x8 mask)
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask64x4 mask)
    {
        return simd::get_mask(mask);
    }

    static inline u64 maskToInt(mask8x64 mask)
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask16x32 mask)
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask32x16 mask)
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask64x8 mask)
    {
        return simd::get_mask(mask);
    }

    // ------------------------------------------------------------------
    // mask reduction
    // ------------------------------------------------------------------

    template <typename T>
    static inline bool none_of(T mask)
    {
        return simd::none_of(mask);
    }

    template <typename T>
    static inline bool any_of(T mask)
    {
        return simd::any_of(mask);
    }

    template <typename T>
    static inline bool all_of(T mask)
    {
        return simd::all_of(mask);
    }

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

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cassert>
#include <algorithm>
#include <mango/core/configure.hpp>
#include <mango/simd/simd.hpp>

/*
    Abstract:

    The Short Vector Math code presented in this header was originally a straightforward
    template N-dimensional vector implementation. Eventually, over time this code merged
    with a low level SIMD abstraction library which is invoked for compatible vector types.

    The "correct" way to utilize SIMD is to work with as wide vectors as possible and not
    wrap vec3, vec4, etc. This code does wrap these types anyway but gives a convenient
    building block to go extra-wide. Because the front-end is all templates, we can write this:

    using vfloat3 = Vector<float32x8, 3>;

    Now the vfloat3 is a three-component vector of 256 bit SIMD vectors.

    vfloat3 a, b;
    auto s = dot(a, b);

    The object s is a vector of scalars; we executed eight dot products in parallel when
    the hardware has support for 256 bit wide vector registers! It is a rather neat trick
    we pulled with this layered approach to writing vector code.

    It is also possible to write "scalar" code using the vector register as scalar type with
    above arrangement:

    auto s = a.x * b.x + a.y * b.y + a.z * b.z;

    This is equivalent to calling the dot product function above; we just wrote the code
    manually. All vector code can be written as-if it were scalar code. Of course, the
    data layout is different so always keep that in mind. Have fun!

*/

namespace mango::math
{

    // ------------------------------------------------------------------
    // scalar functions
    // ------------------------------------------------------------------

    template <typename T>
    static inline T abs(T a)
    {
        return std::abs(a);
    }

    template <typename T>
    static inline T sqrt(T a)
    {
        return std::sqrt(a);
    }

    template <typename T>
    static inline T sin(T a)
    {
        return std::sin(a);
    }

    template <typename T>
    static inline T cos(T a)
    {
        return std::cos(a);
    }

    template <typename T>
    static inline T tan(T a)
    {
        return std::tan(a);
    }

    template <typename T>
    static inline T asin(T a)
    {
        return std::asin(a);
    }

    template <typename T>
    static inline T acos(T a)
    {
        return std::acos(a);
    }

    template <typename T>
    static inline T atan(T a)
    {
        return std::atan(a);
    }

    template <typename T>
    static inline T exp(T a)
    {
        return std::exp(a);
    }

    template <typename T>
    static inline T log(T a)
    {
        return std::log(a);
    }

    template <typename T>
    static inline T exp2(T a)
    {
        return std::exp2(a);
    }

    template <typename T>
    static inline T log2(T a)
    {
        return std::log2(a);
    }

    template <typename T>
    static inline T pow(T a, T b)
    {
        return std::pow(a, b);
    }

    template <typename T>
    static inline T atan2(T a, T b)
    {
        return std::atan2(a, b);
    }

    template <typename T>
    static inline T round(T a)
    {
        return std::round(a);
    }

    template <typename T>
    static inline T floor(T a)
    {
        return std::floor(a);
    }

    template <typename T>
    static inline T ceil(T a)
    {
        return std::ceil(a);
    }

    template <typename T>
    static inline T clamp(T value, T low, T high)
    {
        return std::max(low, std::min(high, value));
    }

    template <typename T>
    static inline T lerp(T a, T b, T t)
    {
        return a + (b - a) * t;
    }

    template <typename T>
    static inline T smoothstep(T a, T b, T t)
    {
        t = clamp((t - a) / (b - a), T(0.0), T(1.0));
        return t * t * (T(3.0) - T(2.0) * t);
    }

    template <typename T>
    static inline T sign(T a)
    {
        if (a < 0) a = -T(1.0);
        else if (a > 0) a = T(1.0);
        return a;
    }

    template <typename T>
    static inline T radians(T a)
    {
        return a * T(0.01745329251);
    }

    template <typename T>
    static inline T degrees(T a)
    {
        return a * T(57.2957795131);
    }

    // ------------------------------------------------------------------
    // Vector
    // ------------------------------------------------------------------

    template <typename ScalarType__, int VectorSize__>
    struct Vector
    {
        using VectorType = void;
        using ScalarType = ScalarType__;
        enum { VectorSize = VectorSize__ };

        ScalarType component[VectorSize];

        ScalarType& operator [] (unsigned int index)
        {
            assert(index < VectorSize);
            return component[index];
        }

        ScalarType operator [] (unsigned int index) const
        {
            assert(index < VectorSize);
            return component[index];
        }

        const ScalarType* data() const
        {
            return component;
        }

        explicit Vector()
        {
        }

        explicit Vector(ScalarType s)
        {
            for (int i = 0; i < VectorSize; ++i)
            {
                component[i] = s;
            }
        }

        Vector(const Vector& v)
        {
            for (int i = 0; i < VectorSize; ++i)
            {
                component[i] = v[i];
            }
        }

        ~Vector()
        {
        }

        Vector& operator = (ScalarType s)
        {
            for (int i = 0; i < VectorSize; ++i)
            {
                component[i] = s;
            }
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
            for (int i = 0; i < VectorSize; ++i)
            {
                component[i] = v[i];
            }
            return *this;
        }

        static Vector ascend()
        {
            Vector v;
            for (int i = 0; i < VectorSize; ++i)
            {
                v.component[i] = ScalarType(i);
            }
            return v;
        }
    };

    template <typename ScalarType__>
    struct Vector<ScalarType__, 2>
    {
        using VectorType = void;
        using ScalarType = ScalarType__;
        enum { VectorSize = 2 };
        union
        {
            ScalarType component[VectorSize];
            struct
            {
                ScalarType x, y;
            };
        };

        ScalarType& operator [] (unsigned int index)
        {
            assert(index < VectorSize);
            return component[index];
        }

        ScalarType operator [] (unsigned int index) const
        {
            assert(index < VectorSize);
            return component[index];
        }

        const ScalarType* data() const
        {
            return component;
        }

        explicit Vector()
        {
        }

        explicit Vector(ScalarType s)
            : x(s), y(s)
        {
        }

        explicit Vector(ScalarType x, ScalarType y)
            : x(x)
            , y(y)
        {
        }

        Vector(const Vector& v)
            : x(v.x)
            , y(v.y)
        {
        }

        ~Vector()
        {
        }

        Vector& operator = (ScalarType s)
        {
            x = s;
            y = s;
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
            x = v.x;
            y = v.y;
            return *this;
        }

        static Vector ascend()
        {
            return Vector(
                ScalarType(0),
                ScalarType(1)
            );
        }
    };

    template <typename ScalarType__>
    struct Vector<ScalarType__, 3>
    {
        using VectorType = void;
        using ScalarType = ScalarType__;
        enum { VectorSize = 3 };
        union
        {
            ScalarType component[VectorSize];
            struct
            {
                ScalarType x, y, z;
            };
        };

        ScalarType& operator [] (unsigned int index)
        {
            assert(index < VectorSize);
            return component[index];
        }

        ScalarType operator [] (unsigned int index) const
        {
            assert(index < VectorSize);
            return component[index];
        }

        const ScalarType* data() const
        {
            return component;
        }

        explicit Vector()
        {
        }

        explicit Vector(ScalarType s)
            : x(s)
            , y(s)
            , z(s)
        {
        }

        explicit Vector(ScalarType x, ScalarType y, ScalarType z)
            : x(x)
            , y(y)
            , z(z)
        {
        }

        explicit Vector(const Vector<ScalarType, 2>& v, ScalarType s)
            : x(v.x)
            , y(v.y)
            , z(s)
        {
        }

        Vector(const Vector& v)
            : x(v.x)
            , y(v.y)
            , z(v.z)
        {
        }

        ~Vector()
        {
        }

        Vector& operator = (ScalarType s)
        {
            x = s;
            y = s;
            z = s;
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
            x = v.x;
            y = v.y;
            z = v.z;
            return *this;
        }

        static Vector ascend()
        {
            return Vector(
                ScalarType(0),
                ScalarType(1),
                ScalarType(2)
            );
        }
    };

    template <typename ScalarType__>
    struct Vector<ScalarType__, 4>
    {
        using VectorType = void;
        using ScalarType = ScalarType__;
        enum { VectorSize = 4 };
        union
        {
            ScalarType component[VectorSize];
            struct
            {
                ScalarType x, y, z, w;
            };
        };

        ScalarType& operator [] (unsigned int index)
        {
            assert(index < VectorSize);
            return component[index];
        }

        ScalarType operator [] (unsigned int index) const
        {
            assert(index < VectorSize);
            return component[index];
        }

        const ScalarType* data() const
        {
            return component;
        }

        explicit Vector()
        {
        }

        explicit Vector(ScalarType s)
            : x(s)
            , y(s)
            , z(s)
            , w(s)
        {
        }

        explicit Vector(ScalarType x, ScalarType y, ScalarType z, ScalarType w)
            : x(x)
            , y(y)
            , z(z)
            , w(w)
        {
        }

        explicit Vector(const Vector<ScalarType, 2>& v0, const Vector<ScalarType, 2>& v1)
            : x(v0.x)
            , y(v0.y)
            , z(v1.x)
            , w(v1.y)
        {
        }

        explicit Vector(const Vector<ScalarType, 3>& v, ScalarType s)
            : x(v.x)
            , y(v.y)
            , z(v.z)
            , w(s)
        {
        }

        Vector(const Vector& v)
            : x(v.x)
            , y(v.y)
            , z(v.z)
            , w(v.w)
        {
        }

        ~Vector()
        {
        }

        Vector& operator = (ScalarType s)
        {
            x = s;
            y = s;
            z = s;
            w = s;
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
            x = v.x;
            y = v.y;
            z = v.z;
            w = v.w;
            return *this;
        }

        static Vector ascend()
        {
            return Vector(
                ScalarType(0),
                ScalarType(1),
                ScalarType(2),
                ScalarType(3)
            );
        }
    };

    // ------------------------------------------------------------------
    // named vector types
    // ------------------------------------------------------------------

    // integer vectors
    using int32x2  = Vector<s32, 2>;
    using int32x3  = Vector<s32, 3>;
    using uint32x2 = Vector<u32, 2>;
    using uint32x3 = Vector<u32, 3>;

    // 128 bit integer vectors
    using int8x16  = Vector<s8, 16>;
    using int16x8  = Vector<s16, 8>;
    using int32x4  = Vector<s32, 4>;
    using int64x2  = Vector<s64, 2>;
    using uint8x16 = Vector<u8, 16>;
    using uint16x8 = Vector<u16, 8>;
    using uint32x4 = Vector<u32, 4>;
    using uint64x2 = Vector<u64, 2>;

    // 256 bit integer vectors
    using int8x32   = Vector<s8, 32>;
    using int16x16  = Vector<s16, 16>;
    using int32x8   = Vector<s32, 8>;
    using int64x4   = Vector<s64, 4>;
    using uint8x32  = Vector<u8, 32>;
    using uint16x16 = Vector<u16, 16>;
    using uint32x8  = Vector<u32, 8>;
    using uint64x4  = Vector<u64, 4>;

    // 512 bit integer vectors
    using int8x64   = Vector<s8, 64>;
    using int16x32  = Vector<s16, 32>;
    using int32x16  = Vector<s32, 16>;
    using int64x8   = Vector<s64, 8>;
    using uint8x64  = Vector<u8, 64>;
    using uint16x32 = Vector<u16, 32>;
    using uint32x16 = Vector<u32, 16>;
    using uint64x8  = Vector<u64, 8>;

    // half vectors
    using float16x4  = Vector<float16, 4>;

    // float vectors
    using float32x2  = Vector<float32, 2>;
    using float32x3  = Vector<float32, 3>;
    using float32x4  = Vector<float32, 4>;
    using float32x8  = Vector<float32, 8>;
    using float32x16 = Vector<float32, 16>;

    // double vectors
    using float64x2  = Vector<float64, 2>;
    using float64x3  = Vector<float64, 3>;
    using float64x4  = Vector<float64, 4>;
    using float64x8  = Vector<float64, 8>;

    // 128 bit vector masks
    using mask8x16   = simd::mask8x16;
    using mask16x8   = simd::mask16x8;
    using mask32x4   = simd::mask32x4;
    using mask64x2   = simd::mask64x2;

    // 256 bit vector masks
    using mask8x32   = simd::mask8x32;
    using mask16x16  = simd::mask16x16;
    using mask32x8   = simd::mask32x8;
    using mask64x4   = simd::mask64x4;

    // 512 bit vector masks
    using mask8x64   = simd::mask8x64;
    using mask16x32  = simd::mask16x32;
    using mask32x16  = simd::mask32x16;
    using mask64x8   = simd::mask64x8;

} // namespace mango::math

#include <mango/math/vector_simd.hpp>

#include <mango/math/vector128_uint8x16.hpp>
#include <mango/math/vector128_uint16x8.hpp>
#include <mango/math/vector128_uint32x4.hpp>
#include <mango/math/vector128_uint64x2.hpp>
#include <mango/math/vector128_int8x16.hpp>
#include <mango/math/vector128_int16x8.hpp>
#include <mango/math/vector128_int32x4.hpp>
#include <mango/math/vector128_int64x2.hpp>

#include <mango/math/vector256_uint8x32.hpp>
#include <mango/math/vector256_uint16x16.hpp>
#include <mango/math/vector256_uint32x8.hpp>
#include <mango/math/vector256_uint64x4.hpp>
#include <mango/math/vector256_int8x32.hpp>
#include <mango/math/vector256_int16x16.hpp>
#include <mango/math/vector256_int32x8.hpp>
#include <mango/math/vector256_int64x4.hpp>

#include <mango/math/vector512_uint8x64.hpp>
#include <mango/math/vector512_uint16x32.hpp>
#include <mango/math/vector512_uint32x16.hpp>
#include <mango/math/vector512_uint64x8.hpp>
#include <mango/math/vector512_int8x64.hpp>
#include <mango/math/vector512_int16x32.hpp>
#include <mango/math/vector512_int32x16.hpp>
#include <mango/math/vector512_int64x8.hpp>

#include <mango/math/vector_float16x4.hpp>
#include <mango/math/vector_float32x2.hpp>
#include <mango/math/vector_float32x3.hpp>
#include <mango/math/vector_float32x4.hpp>
#include <mango/math/vector_float32x8.hpp>
#include <mango/math/vector_float32x16.hpp>
#include <mango/math/vector_float64x2.hpp>
#include <mango/math/vector_float64x3.hpp>
#include <mango/math/vector_float64x4.hpp>
#include <mango/math/vector_float64x8.hpp>

#include <mango/math/vector_gather.hpp>

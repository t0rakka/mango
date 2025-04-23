/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cassert>
#include <algorithm>
#include <array>
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
    concept is_scalar = std::is_scalar_v<T>;

    template <typename T>
        requires is_scalar<T>
    static inline T abs(const T& a)
    {
        return std::abs(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T sqrt(const T& a)
    {
        return std::sqrt(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T sin(const T& a)
    {
        return std::sin(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T cos(const T& a)
    {
        return std::cos(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T tan(const T& a)
    {
        return std::tan(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T asin(const T& a)
    {
        return std::asin(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T acos(const T& a)
    {
        return std::acos(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T atan(const T& a)
    {
        return std::atan(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T exp(const T& a)
    {
        return std::exp(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T log(const T& a)
    {
        return std::log(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T exp2(const T& a)
    {
        return std::exp2(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T log2(const T& a)
    {
        return std::log2(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T pow(const T& a, const T& b)
    {
        return std::pow(a, b);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T atan2(const T& a, const T& b)
    {
        return std::atan2(a, b);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T round(const T& a)
    {
        return std::round(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T floor(const T& a)
    {
        return std::floor(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T ceil(const T& a)
    {
        return std::ceil(a);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T min(const T& a, const T& b)
    {
        return std::min(a, b);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T max(const T& a, const T& b)
    {
        return std::max(a, b);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T clamp(const T& value, const T& low, const T& high)
    {
        return std::max(low, std::min(high, value));
    }

    template <typename T>
        requires is_scalar<T>
    static inline T lerp(const T& a, const T& b, const T& t)
    {
        return a + (b - a) * t;
    }

    template <typename T>
        requires is_scalar<T>
    static inline T smoothstep(const T& a, const T& b, const T& t)
    {
        t = clamp((t - a) / (b - a), T(0.0), T(1.0));
        return t * t * (T(3.0) - T(2.0) * t);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T sign(const T& a)
    {
        if (a < 0) a = -T(1.0);
        else if (a > 0) a = T(1.0);
        return a;
    }

    template <typename T>
        requires is_scalar<T>
    static inline T radians(const T& a)
    {
        return a * T(0.01745329251);
    }

    template <typename T>
        requires is_scalar<T>
    static inline T degrees(const T& a)
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

        std::array<ScalarType, VectorSize> component;

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

        ScalarType* data()
        {
            return component.data();
        }

        const ScalarType* data() const
        {
            return component.data();
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

    // Vector<ScalarType, 2>

    template <typename ScalarType__>
    struct Vector<ScalarType__, 2>
    {
        using VectorType = void;
        using ScalarType = ScalarType__;
        enum { VectorSize = 2 };
        union
        {
            std::array<ScalarType, VectorSize> component;
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

        ScalarType* data()
        {
            return component.data();
        }

        const ScalarType* data() const
        {
            return component.data();
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

    // Vector<ScalarType, 3>

    template <typename ScalarType__>
    struct Vector<ScalarType__, 3>
    {
        using VectorType = void;
        using ScalarType = ScalarType__;
        enum { VectorSize = 3 };
        union
        {
            std::array<ScalarType, VectorSize> component;
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

        ScalarType* data()
        {
            return component.data() ;
        }

        const ScalarType* data() const
        {
            return component.data();
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

    // Vector<ScalarType, 4>

    template <typename ScalarType__>
    struct Vector<ScalarType__, 4>
    {
        using VectorType = void;
        using ScalarType = ScalarType__;
        enum { VectorSize = 4 };
        union
        {
            std::array<ScalarType, VectorSize> component;
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

        ScalarType* data()
        {
            return component.data();
        }

        const ScalarType* data() const
        {
            return component.data();
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

    // ------------------------------------------------------------------
    // Forward declarations
    // ------------------------------------------------------------------

    template <typename ScalarType, typename VectorType, int Index>
    struct ScalarAccessor;

    template <typename VectorType, typename StorageType, int... Indices>
    struct ShuffleAccessor;

    template <typename LowType, typename VectorType>
    struct LowAccessor;

    template <typename HighType, typename VectorType>
    struct HighAccessor;

    // ------------------------------------------------------------------
    // Core concepts and type traits
    // ------------------------------------------------------------------

    template <typename T>
    struct is_scalar_accessor : std::false_type {};

    template <typename ScalarType, typename VectorType, int Index>
    struct is_scalar_accessor<ScalarAccessor<ScalarType, VectorType, Index>> : std::true_type {};

    template <typename T>
    struct is_shuffle_accessor : std::false_type {};

    template <typename VectorType, typename StorageType, int... Indices>
    struct is_shuffle_accessor<ShuffleAccessor<VectorType, StorageType, Indices...>> : std::true_type {};

    template <typename T>
    concept is_simd_vector = requires(T v)
    {
        typename T::VectorType;
        { T::VectorSize };
        requires !std::same_as<typename T::VectorType, void>;
    };

    template <typename T>
    concept is_vector = requires(T v)
    {
        { T::VectorSize };
        typename T::ScalarType;
        { v[0] } -> std::convertible_to<typename T::ScalarType>;
    };

    template <typename T>
    concept resolves_to_scalar = 
        is_scalar<T> || 
        is_scalar_accessor<std::remove_cvref_t<T>>::value;

    template <typename T>
    concept resolves_to_vector = 
        is_vector<T> || 
        is_simd_vector<T> ||
        is_shuffle_accessor<std::remove_cvref_t<T>>::value;

    template <typename T>
    concept is_vector_or_scalar = resolves_to_vector<T> || resolves_to_scalar<T>;

    template <typename T>
    concept is_simd_vector_or_scalar = is_simd_vector<T> || resolves_to_scalar<T>;

    template <typename T>
    concept is_signed_vector = is_vector<T> && std::is_signed_v<typename T::ScalarType>;

    template <typename T>
    concept is_float_vector = std::is_floating_point_v<typename T::ScalarType>;

    template <typename T>
    struct get_vector_type
    {
        using type = T;
    };

    template <typename VectorType, typename StorageType, int... Indices>
    struct get_vector_type<ShuffleAccessor<VectorType, StorageType, Indices...>>
    {
        using type = VectorType;
    };

    // has_vector

    template <typename... Args>
    concept has_vector = (is_vector<Args> || ...) &&
        ((is_vector_or_scalar<Args> || 
        is_shuffle_accessor<std::remove_cvref_t<Args>>::value) && ...);

    template <typename... Args>
    struct first_vector_type;

    template <typename T, typename U>
    struct first_vector_type<T, U>
    {
        using type = std::conditional_t<
            is_vector<T> || is_shuffle_accessor<std::remove_cvref_t<T>>::value,
            typename get_vector_type<T>::type,
            typename get_vector_type<U>::type>;
    };

    template <typename T>
    struct first_vector_type<T>
    {
        using type = T;
    };

    template <typename... Args>
    using first_vector_t = typename first_vector_type<Args...>::type;

    // has_simd_vector

    template <typename... Args>
    concept has_simd_vector = (is_simd_vector<Args> || ...) &&
                              (is_simd_vector_or_scalar<Args> && ...);

    template <typename... Args>
    struct first_simd_vector_type;

    template <typename T, typename... Args>
    struct first_simd_vector_type<T, Args...>
    {
        using type = std::conditional_t<is_simd_vector<T>, T,
            typename first_simd_vector_type<Args...>::type>;
    };

    template <typename T>
    struct first_simd_vector_type<T>
    {
        using type = T;
    };

    template <typename... Args>
    using first_simd_vector_t = typename first_simd_vector_type<Args...>::type;

    // resolve type

    template <typename A, typename B>
    struct resolve_type
    {
        using type = std::conditional_t<resolves_to_vector<A> || resolves_to_vector<B>,
            typename first_vector_type<A, B>::type,  // vector case
            std::remove_cvref_t<A>>;                 // scalar case (includes both raw scalars and scalar accessors)
    };

    template <typename A, typename B>
    using resolve_t = typename resolve_type<A, B>::type;

    // ------------------------------------------------------------------
    // ScalarAccessor
    // ------------------------------------------------------------------

    template <typename ScalarType, typename VectorType, int Index>
    struct ScalarAccessor
    {
        VectorType m;

        ScalarAccessor(const ScalarAccessor& accessor) = default;

        constexpr operator ScalarType () const noexcept
        {
            return simd::get_component<Index>(m);
        }

        template <int VectorSize>
        constexpr operator Vector<ScalarType, VectorSize> () const noexcept
        {
            return Vector<ScalarType, VectorSize>(simd::get_component<Index>(m));
        }

        constexpr ScalarAccessor& operator = (const ScalarAccessor& accessor) noexcept
        {
            m = simd::set_component<Index>(m, ScalarType(accessor));
            return *this;
        }

        constexpr ScalarAccessor& operator = (ScalarType s) noexcept
        {
            m = simd::set_component<Index>(m, s);
            return *this;
        }

        constexpr ScalarAccessor& operator += (ScalarType s) noexcept
        {
            *this = ScalarType(*this) + s;
            return *this;
        }

        constexpr ScalarAccessor& operator -= (ScalarType s) noexcept
        {
            *this = ScalarType(*this) - s;
            return *this;
        }

        constexpr ScalarAccessor& operator *= (ScalarType s) noexcept
        {
            *this = ScalarType(*this) * s;
            return *this;
        }

        constexpr ScalarAccessor& operator /= (ScalarType s) noexcept
        {
            *this = ScalarType(*this) / s;
            return *this;
        }
    };

    // ------------------------------------------------------------------
    // ScalarAccessor operators
    // ------------------------------------------------------------------

    // NOTE: These operators are not strictly necessary, since the type system below will
    //       generate code but it will expand these to full vector types and generate less
    //       efficient code. By providing scalar specialization we get scalar code directly.

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator + (const ScalarAccessor<ScalarType, VectorType, Index>& a) noexcept
    {
        return ScalarType(a);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    ScalarType operator + (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) + ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator + (const ScalarAccessor<ScalarType, VectorType, Index>& a,
                           ScalarType b) noexcept
    {
        return ScalarType(a) + b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator + (ScalarType a,
                           const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        return a + ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator - (const ScalarAccessor<ScalarType, VectorType, Index>& a) noexcept
    {
        return -ScalarType(a);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    ScalarType operator - (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) - ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator - (const ScalarAccessor<ScalarType, VectorType, Index>& a,
                           ScalarType b) noexcept
    {
        return ScalarType(a) - b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator - (ScalarType a,
                           const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        return a - ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    ScalarType operator * (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) * ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator * (const ScalarAccessor<ScalarType, VectorType, Index>& a,
                           ScalarType b) noexcept
    {
        return ScalarType(a) * b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator * (ScalarType a,
                           const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        return a * ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    ScalarType operator / (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) / ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator / (const ScalarAccessor<ScalarType, VectorType, Index>& a,
                           ScalarType b) noexcept
    {
        return ScalarType(a) / b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator / (ScalarType a,
                          const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        return a / ScalarType(b);
    }

    // ------------------------------------------------------------------
    // ScalarAccessor comparison operators
    // ------------------------------------------------------------------

    // operator <

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    bool operator < (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                     const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) < ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    bool operator < (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b) noexcept
    {
        return ScalarType(a) < b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    bool operator < (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        return a < ScalarType(b);
    }

    // operator >

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    bool operator > (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                     const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) > ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    bool operator > (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b) noexcept
    {
        return ScalarType(a) > b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    bool operator > (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        return a > ScalarType(b);
    }

    // operator <=

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    bool operator <= (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                      const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) <= ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    bool operator <= (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b) noexcept
    {
        return ScalarType(a) <= b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    bool operator <= (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        return a <= ScalarType(b);
    }

    // operator >=

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    bool operator >= (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                      const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) >= ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    bool operator >= (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b) noexcept
    {
        return ScalarType(a) >= b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    bool operator >= (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        return a >= ScalarType(b);
    }

    // operator ==

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    bool operator == (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                      const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) == ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    bool operator == (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b) noexcept
    {
        return ScalarType(a) == b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    bool operator == (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        return a == ScalarType(b);
    }

    // operator !=

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    bool operator != (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                      const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) != ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    bool operator != (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b) noexcept
    {
        return ScalarType(a) != b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    bool operator != (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        return a != ScalarType(b);
    }

    // ------------------------------------------------------------------
    // ShuffleAccessor
    // ------------------------------------------------------------------

    template <typename VectorType, typename StorageType, int... Indices>
    struct ShuffleAccessor
    {
        using ScalarType = typename VectorType::ScalarType;
        static constexpr int VectorSize = VectorType::VectorSize;

        StorageType m;

        // implemented in specializations
        constexpr operator VectorType () const noexcept;
    };

    // vec2 -> vec2 shuffle (simd)
    template <typename VectorType, typename StorageType, int X, int Y>
        requires is_simd_vector<VectorType> && (VectorType::VectorSize == 2)
    struct ShuffleAccessor<VectorType, StorageType, X, Y>
    {
        using ScalarType = typename VectorType::ScalarType;
        static constexpr int VectorSize = 2;

        StorageType m;

        constexpr operator VectorType () const noexcept
        {
            return simd::shuffle<X, Y>(m);
        }
    };

    // vec2 -> vec2 shuffle (scalar)
    template <typename VectorType, typename StorageType, int X, int Y>
        requires (!is_simd_vector<VectorType>) && (VectorType::VectorSize == 2)
    struct ShuffleAccessor<VectorType, StorageType, X, Y>
    {
        using ScalarType = typename VectorType::ScalarType;
        static constexpr int VectorSize = 2;

        StorageType m;

        constexpr operator VectorType () const noexcept
        {
            const ScalarType x = m[X];
            const ScalarType y = m[Y];
            return Vector<ScalarType, 2>(x, y);
        }
    };

    // vec3 -> vec2 shuffle
    template <typename VectorType, typename StorageType, int X, int Y>
        requires (VectorType::VectorSize == 3)
    struct ShuffleAccessor<VectorType, StorageType, X, Y>
    {
        using ScalarType = typename VectorType::ScalarType;
        static constexpr int VectorSize = 2;

        StorageType m;

        constexpr operator Vector<ScalarType, 2> () const noexcept
        {
            const ScalarType x = m[X];
            const ScalarType y = m[Y];
            return Vector<ScalarType, 2>(x, y);
        }
    };

    // vec3 -> vec3 shuffle
    template <typename VectorType, typename StorageType, int X, int Y, int Z>
        requires (VectorType::VectorSize == 3)
    struct ShuffleAccessor<VectorType, StorageType, X, Y, Z>
    {
        using ScalarType = typename VectorType::ScalarType;
        static constexpr int VectorSize = 3;

        StorageType m;

        constexpr operator VectorType () const noexcept
        {
            const ScalarType x = m[X];
            const ScalarType y = m[Y];
            const ScalarType z = m[Z];
            return VectorType(x, y, z);
        }
    };

    // vec4 -> vec2 shuffle
    template <typename VectorType, typename StorageType, int X, int Y>
        requires (VectorType::VectorSize == 4)
    struct ShuffleAccessor<VectorType, StorageType, X, Y>
    {
        using ScalarType = typename VectorType::ScalarType;
        static constexpr int VectorSize = 2;

        StorageType m;

        constexpr operator Vector<ScalarType, 2> () const noexcept
        {
            const ScalarType x = simd::get_component<X>(m);
            const ScalarType y = simd::get_component<Y>(m);
            return Vector<ScalarType, 2>(x, y);
        }
    };

    // vec4 -> vec3 shuffle
    template <typename VectorType, typename StorageType, int X, int Y, int Z>
        requires (VectorType::VectorSize == 4)
    struct ShuffleAccessor<VectorType, StorageType, X, Y, Z>
    {
        using ScalarType = typename VectorType::ScalarType;
        static constexpr int VectorSize = 3;

        StorageType m;

        constexpr operator Vector<ScalarType, 3> () const noexcept
        {
            const ScalarType x = simd::get_component<X>(m);
            const ScalarType y = simd::get_component<Y>(m);
            const ScalarType z = simd::get_component<Z>(m);
            return Vector<ScalarType, 3>(x, y, z);
        }
    };

    // vec4 -> vec4 shuffle
    template <typename VectorType, typename StorageType, int X, int Y, int Z, int W>
        requires (VectorType::VectorSize == 4)
    struct ShuffleAccessor<VectorType, StorageType, X, Y, Z, W>
    {
        using ScalarType = typename VectorType::ScalarType;
        static constexpr int VectorSize = 4;

        StorageType m;

        constexpr operator VectorType () const noexcept
        {
            return simd::shuffle<X, Y, Z, W>(m);
        }
    };

    // ------------------------------------------------------------------
    // ShuffleAccessor operators
    // ------------------------------------------------------------------

    template <typename VectorType, typename StorageType, int... AIndices, int... BIndices>
    static constexpr
    auto operator * (const ShuffleAccessor<VectorType, StorageType, AIndices...>& a,
                     const ShuffleAccessor<VectorType, StorageType, BIndices...>& b) noexcept
    {
        return VectorType(a) * VectorType(b);
    }

    template <typename VectorType, typename StorageType, int... Indices>
    static constexpr
    auto operator * (const ShuffleAccessor<VectorType, StorageType, Indices...>& a,
                     typename VectorType::ScalarType s) noexcept
    {
        return VectorType(a) * s;
    }

    template <typename VectorType, typename StorageType, int... Indices>
    static constexpr
    auto operator * (typename VectorType::ScalarType s,
                     const ShuffleAccessor<VectorType, StorageType, Indices...>& a) noexcept
    {
        return s * VectorType(a);
    }

    template <typename VectorType, typename StorageType, int... Indices>
    static constexpr
    auto operator / (const ShuffleAccessor<VectorType, StorageType, Indices...>& a,
                     typename VectorType::ScalarType s) noexcept
    {
        return VectorType(a) / s;
    }

    template <typename VectorType, typename StorageType, int... Indices>
    static constexpr
    auto operator + (const ShuffleAccessor<VectorType, StorageType, Indices...>& a,
                     typename VectorType::ScalarType s) noexcept
    {
        return VectorType(a) + s;
    }

    template <typename VectorType, typename StorageType, int... Indices>
    static constexpr
    auto operator + (typename VectorType::ScalarType s,
                     const ShuffleAccessor<VectorType, StorageType, Indices...>& a) noexcept
    {
        return s + VectorType(a);
    }

    template <typename VectorType, typename StorageType, int... Indices>
    static constexpr
    auto operator - (const ShuffleAccessor<VectorType, StorageType, Indices...>& a,
                     typename VectorType::ScalarType s) noexcept
    {
        return VectorType(a) - s;
    }

    template <typename VectorType, typename StorageType, int... Indices>
    static constexpr
    auto operator - (typename VectorType::ScalarType s,
                     const ShuffleAccessor<VectorType, StorageType, Indices...>& a) noexcept
    {
        return s - VectorType(a);
    }

    // ------------------------------------------------------------------
    // LowAccessor
    // ------------------------------------------------------------------

    template <typename LowType, typename VectorType>
    struct LowAccessor
    {
        VectorType m;

        constexpr operator LowType () const noexcept
        {
            return simd::get_low(m);
        }

        constexpr LowAccessor& operator = (LowType low) noexcept
        {
            m = simd::set_low(m, low);
            return *this;
        }
    };

    // ------------------------------------------------------------------
    // HighAccessor
    // ------------------------------------------------------------------

    template <typename HighType, typename VectorType>
    struct HighAccessor
    {
        VectorType m;

        constexpr operator HighType () const noexcept
        {
            return simd::get_high(m);
        }

        constexpr HighAccessor& operator = (HighType high) noexcept
        {
            m = simd::set_high(m, high);
            return *this;
        }
    };

    // ------------------------------------------------------------------
    // vector_ops
    // ------------------------------------------------------------------

    // vector_ops

    template <typename T>
    struct vector_ops
    {
        using ScalarType = typename T::ScalarType;

        static T abs(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = std::abs(a[i]);
            }
            return temp;
        }

        static T neg(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = -a[i];
            }
            return temp;
        }

        static T sub(const T& a, const T& b)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = a[i] - b[i];
            }
            return temp;
        }

        static T add(const T& a, const T& b)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = a[i] + b[i];
            }
            return temp;
        }

        static T mul(const T& a, const T& b)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = a[i] * b[i];
            }
            return temp;
        }

        static T div(const T& a, const T& b)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = a[i] / b[i];
            }
            return temp;
        }

        static T madd(const T& a, const T& b, const T& c)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = a[i] + b[i] * c[i];
            }
            return temp;
        }

        static T msub(const T& a, const T& b, const T& c)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = b[i] * c[i] - a[i];
            }
            return temp;
        }

        static T nmadd(const T& a, const T& b, const T& c)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = a[i] - b[i] * c[i];
            }
            return temp;
        }

        static T nmsub(const T& a, const T& b, const T& c)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = -(a[i] + b[i] * c[i]);
            }
            return temp;
        }

        static T min(const T& a, const T& b)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = std::min(a[i], b[i]);
            }
            return temp;
        }

        static T max(const T& a, const T& b)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = std::max(a[i], b[i]);
            }
            return temp;
        }

        static T sign(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = sign(a[i]);
            }
            return temp;
        }

        static T clamp(const T& a, const T& low, const T& high)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = std::max(low[i], std::min(high[i], a[i]));
            }
            return temp;
        }

        static T lerp(const T& a, const T& b, ScalarType factor)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = a[i] + (b[i] - a[i]) * factor;
            }
            return temp;
        }

        static T radians(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = radians(a[i]);
            }
            return temp;
        }

        static T degrees(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = degrees(a[i]);
            }
            return temp;
        }

        static T rcp(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = ScalarType(1.0) / a[i];
            }
            return temp;
        }

        static T sqrt(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = std::sqrt(a[i]);
            }
            return temp;
        }

        static T rsqrt(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = ScalarType(1.0) / std::sqrt(a[i]);
            }
            return temp;
        }

        static T round(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = std::round(a[i]);
            }
            return temp;
        }

        static T floor(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = std::floor(a[i]);
            }
            return temp;
        }

        static T ceil(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = std::ceil(a[i]);
            }
            return temp;
        }

        static T trunc(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = std::trunc(a[i]);
            }
            return temp;
        }

        static T fract(const T& a)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = a[i] - std::floor(a[i]);
            }
            return temp;
        }

        static T mod(const T& a, const T& b)
        {
            T temp;
            for (int i = 0; i < T::VectorSize; ++i)
            {
                temp[i] = a[i] - b[i] * std::floor(a[i] / b[i]);
            }
            return temp;
        }
    };

    // simd vector_ops

    template <typename T>
        requires is_simd_vector<T>
    struct vector_ops<T>
    {
        using ScalarType = typename T::ScalarType;

        static T abs(const T& a)
        {
            return simd::abs(a);
        }

        static T neg(const T& a)
        {
            return simd::neg(a);
        }

        static T sub(const T& a, const T& b)
        {
            return simd::sub(a, b);
        }

        static T add(const T& a, const T& b)
        {
            return simd::add(a, b);
        }

        static T mul(const T& a, const T& b)
        {
            return simd::mul(a, b);
        }

        static T div(const T& a, const T& b)
        {
            return simd::div(a, b);
        }

        static T madd(const T& a, const T& b, const T& c)
        {
            return simd::madd(a, b, c);
        }

        static T msub(const T& a, const T& b, const T& c)
        {
            return simd::msub(a, b, c);
        }

        static T nmadd(const T& a, const T& b, const T& c)
        {
            return simd::nmadd(a, b, c);
        }

        static T nmsub(const T& a, const T& b, const T& c)
        {
            return simd::nmsub(a, b, c);
        }

        static T min(const T& a, const T& b)
        {
            return simd::min(a, b);
        }

        static T max(const T& a, const T& b)
        {
            return simd::max(a, b);
        }

        static T sign(const T& a)
        {
            return simd::sign(a);
        }

        static T clamp(const T& a, const T& low, const T& high)
        {
            return simd::max(low, simd::min(high, a));
        }

        static T lerp(const T& a, const T& b, ScalarType factor)
        {
            return simd::lerp(a, b, T(factor));
        }

        static T radians(const T& a)
        {
            return simd::mul(a, T(0.01745329251));
        }

        static T degrees(const T& a)
        {
            return simd::mul(a, T(57.2957795131));
        }

        static T rcp(const T& a)
        {
            return simd::rcp(a);
        }

        static T sqrt(const T& a)
        {
            return simd::sqrt(a);
        }

        static T rsqrt(const T& a)
        {
            return simd::rsqrt(a);
        }

        static T round(const T& a)
        {
            return simd::round(a);
        }

        static T floor(const T& a)
        {
            return simd::floor(a);
        }

        static T ceil(const T& a)
        {
            return simd::ceil(a);
        }

        static T trunc(const T& a)
        {
            return simd::trunc(a);
        }

        static T fract(const T& a)
        {
            return simd::fract(a);
        }

        static T mod(const T& a, const T& b)
        {
            return simd::sub(a, simd::mul(b, simd::floor(simd::div(a, b))));
        }
    };

    // unary operators

    template <typename T>
        requires is_vector<T>
    static inline T operator + (const T& a)
    {
        return a;
    }

    template <typename T>
        requires is_signed_vector<T>
    static inline T operator - (const T& a)
    {
        return vector_ops<T>::neg(a);
    }

    // binary operators

    template <typename A, typename B>
        requires (resolves_to_vector<A> || resolves_to_vector<B>)
    static inline auto operator + (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return vector_ops<T>::add(T(a), T(b));
    }

    template <typename A, typename B>
        requires (resolves_to_vector<A> || resolves_to_vector<B>)
    static inline auto operator - (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return vector_ops<T>::sub(T(a), T(b));
    }

    template <typename A, typename B>
        requires has_vector<A, B> && is_float_vector<first_vector_t<A, B>>
    static inline auto operator * (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return vector_ops<T>::mul(T(a), T(b));
    }

    template <typename A, typename B>
        requires (resolves_to_vector<A> || resolves_to_vector<B>) &&  is_float_vector<first_vector_t<A, B>>
    static inline auto operator / (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return vector_ops<T>::div(T(a), T(b));
    }

    // compound assignment operators

    template <typename A, typename B>
        requires is_vector<A> && is_vector_or_scalar<B>
    static inline A& operator += (A& a, const B& b)
    {
        a = vector_ops<A>::add(a, A(b));
        return a;
    }

    template <typename A, typename B>
        requires is_vector<A> && is_vector_or_scalar<B>
    static inline A& operator -= (A& a, const B& b)
    {
        a = vector_ops<A>::sub(a, A(b));
        return a;
    }

    template <typename A, typename B>
        requires is_vector<A> && is_float_vector<A> && is_vector_or_scalar<B>
    static inline A& operator *= (A& a, const B& b)
    {
        a = vector_ops<A>::mul(a, A(b));
        return a;
    }

    template <typename A, typename B>
        requires is_vector<A> && is_float_vector<A> && is_vector_or_scalar<B>
    static inline A& operator /= (A& a, const B& b)
    {
        a = vector_ops<A>::div(a, A(b));
        return a;
    }

    // functions

    template <typename A>
        requires resolves_to_vector<A>
    static inline auto abs(const A& a)
    {
        using T = resolve_t<A, A>;
        return vector_ops<T>::abs(T(a));
    }

    template <typename A, typename B, typename C>
        requires resolves_to_vector<A> || resolves_to_vector<B> || resolves_to_vector<C>
    static inline auto madd(const A& a, const B& b, const C& c)
    {
        using T = resolve_t<A, resolve_t<B, C>>;
        return vector_ops<T>::madd(T(a), T(b), T(c));
    }

    template <typename A, typename B, typename C>
        requires resolves_to_vector<A> || resolves_to_vector<B> || resolves_to_vector<C>
    static inline auto msub(const A& a, const B& b, const C& c)
    {
        using T = resolve_t<A, resolve_t<B, C>>;
        return vector_ops<T>::msub(T(a), T(b), T(c));
    }

    template <typename A, typename B, typename C>
        requires resolves_to_vector<A> || resolves_to_vector<B> || resolves_to_vector<C>
    static inline auto nmadd(const A& a, const B& b, const C& c)
    {
        using T = resolve_t<A, resolve_t<B, C>>;
        return vector_ops<T>::nmadd(T(a), T(b), T(c));
    }

    template <typename A, typename B, typename C>
        requires resolves_to_vector<A> || resolves_to_vector<B> || resolves_to_vector<C>
    static inline auto nmsub(const A& a, const B& b, const C& c)
    {
        using T = resolve_t<A, resolve_t<B, C>>;
        return vector_ops<T>::nmsub(T(a), T(b), T(c));
    }

    template <typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto min(const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return vector_ops<T>::min(T(a), T(b));
    }

    template <typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto max(const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return vector_ops<T>::max(T(a), T(b));
    }

    template <typename A>
        requires resolves_to_vector<A> && (A::VectorSize >= 2 || A::VectorSize <= 4)
    static inline auto sign(const A& a)
    {
        using T = resolve_t<A, A>;
        return vector_ops<T>::sign(T(a));
    }

    template <typename A, typename B, typename C>
        requires resolves_to_vector<A> || resolves_to_vector<B> || resolves_to_vector<C>
    static inline auto clamp(const A& a, const B& b, const C& c)
    {
        using T = resolve_t<A, resolve_t<B, C>>;
        return vector_ops<T>::clamp(T(a), T(b), T(c));
    }

    template <typename A, typename B, typename C>
        requires (resolves_to_vector<A> || resolves_to_vector<B>) && resolves_to_scalar<C>
    static inline auto lerp(const A& a, const B& b, C s)
    {
        using T = resolve_t<A, B>;
        return vector_ops<T>::lerp(T(a), T(b), typename T::ScalarType(s));
    }

    template <typename A>
        requires resolves_to_vector<A>
    static inline auto radians(const A& a)
    {
        using T = resolve_t<A, A>;
        return vector_ops<T>::radians(T(a));
    }

    template <typename A>
        requires resolves_to_vector<A>
    static inline auto degrees(const A& a)
    {
        using T = resolve_t<A, A>;
        return vector_ops<T>::degrees(T(a));
    }

    template <typename A>
        requires resolves_to_vector<A>
    static inline auto rcp(const A& a)
    {
        using T = resolve_t<A, A>;
        return vector_ops<T>::rcp(T(a));
    }

    template <typename A>
        requires resolves_to_vector<A>
    static inline auto sqrt(const A& a)
    {
        using T = resolve_t<A, A>;
        return vector_ops<T>::sqrt(T(a));
    }

    template <typename A>
        requires resolves_to_vector<A>
    static inline auto rsqrt(const A& a)
    {
        using T = resolve_t<A, A>;
        return vector_ops<T>::rsqrt(T(a));
    }

    template <typename A>
        requires resolves_to_vector<A>
    static inline auto round(const A& a)
    {
        using T = resolve_t<A, A>;
        return vector_ops<T>::round(T(a));
    }

    template <typename A>
        requires resolves_to_vector<A>
    static inline auto floor(const A& a)
    {
        using T = resolve_t<A, A>;
        return vector_ops<T>::floor(T(a));
    }

    template <typename A>
        requires resolves_to_vector<A>
    static inline auto ceil(const A& a)
    {
        using T = resolve_t<A, A>;
        return vector_ops<T>::ceil(T(a));
    }

    template <typename A>
        requires resolves_to_vector<A>
    static inline auto trunc(const A& a)
    {
        using T = resolve_t<A, A>;
        return vector_ops<T>::trunc(T(a));
    }

    template <typename A>
        requires resolves_to_vector<A>
    static inline auto fract(const A& a)
    {
        using T = resolve_t<A, A>;
        return vector_ops<T>::fract(T(a));
    }

    template <typename A, typename B>
        requires (resolves_to_vector<A> || resolves_to_vector<B>)
    static inline auto mod(const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return vector_ops<T>::mod(T(a), T(b));
    }

    // ------------------------------------------------------------------
    // vector functions
    // ------------------------------------------------------------------

    template <typename T>
        requires is_vector<T> && (T::VectorSize >= 2 && T::VectorSize <= 4)
    static inline auto dot(const T& a, const T& b)
    {
        typename T::ScalarType temp(0);
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp += a[i] * b[i];
        }
        return temp;
    }

    template <typename T>
        requires is_vector<T> && (T::VectorSize >= 2 && T::VectorSize <= 4)
    static inline auto square(const T& a)
    {
        return dot(a, a);
    }

    template <typename T>
        requires is_vector<T> && (T::VectorSize >= 2 && T::VectorSize <= 4)
    static inline auto length(const T& a)
    {
        return std::sqrt(square(a));
    }

    template <typename T>
        requires is_vector<T> && (T::VectorSize >= 2 && T::VectorSize <= 4)
    static inline auto distance(const T& a, const T& b)
    {
        return length(a - b);
    }

    template <typename T>
        requires is_vector<T> && (T::VectorSize >= 2 && T::VectorSize <= 4)
    static inline auto normalize(const T& a)
    {
        using ScalarType = typename T::ScalarType;
        return a * (ScalarType(1.0) / length(a));
    }

    template <typename T>
        requires is_vector<T> && (T::VectorSize == 2 || T::VectorSize == 3)
    static inline auto project(const T& v, const T& normal)
    {
        return v - normal * (dot(v, normal) / dot(normal, normal));
    }

    template <typename T>
        requires is_vector<T> && (T::VectorSize == 2 || T::VectorSize == 3)
    static inline auto reflect(const T& v, const T& normal)
    {
        using ScalarType = typename T::ScalarType;
        return v - normal * (ScalarType(2.0) * dot(v, normal));
    }

    template <typename T, typename S>
        requires is_vector<T> && (T::VectorSize == 2 || T::VectorSize == 3) &&
                 is_scalar<S>
    static inline auto refract(const T& v, const T& normal, S factor)
    {
        using ScalarType = typename T::ScalarType;
        ScalarType f = ScalarType(factor);
        ScalarType vdotn = dot(v, normal);
        ScalarType p = ScalarType(1.0) - f * f * (ScalarType(1.0) - vdotn * vdotn);
        if (p < 0)
        {
            return T(ScalarType(0.0));
        }
        return v * f - normal * (std::sqrt(p) + f * vdotn);
    }

    template <typename T>
        requires is_vector<T> && (T::VectorSize == 3)
    static inline auto cross(const T& a, const T& b)
    {
        auto x = a.y * b.z - a.z * b.y;
        auto y = a.z * b.x - a.x * b.z;
        auto z = a.x * b.y - a.y * b.x;
        return T(x, y, z);
    }

    template <typename T>
        requires is_vector<T> && (T::VectorSize == 2)
    static inline auto hmin(const T& a)
    {
        auto s = std::min(a.x, a.y);
        return T(s);
    }

    template <typename T>
        requires is_vector<T> && (T::VectorSize == 2)
    static inline auto hmax(const T& a)
    {
        auto s = std::max(a.x, a.y);
        return T(s);
    }

    template <typename T>
        requires is_vector<T> && (T::VectorSize == 3)
    static inline auto hmin(const T& a)
    {
        auto s = std::ranges::min({a.x, a.y, a.z});
        return T(s);
    }

    template <typename T>
        requires is_vector<T> && (T::VectorSize == 3)
    static inline auto hmax(const T& a)
    {
        auto s = std::ranges::max({a.x, a.y, a.z});
        return T(s);
    }

    // ------------------------------------------------------------------
    // trigonometric functions
    // ------------------------------------------------------------------

    template <typename T>
        requires is_vector<T>
    static inline auto sin(const T& a)
    {
        T temp;
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp[i] = std::sin(a[i]);
        }
        return temp;
    }

    template <typename T>
        requires is_vector<T>
    static inline auto cos(const T& a)
    {
        T temp;
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp[i] = std::cos(a[i]);
        }
        return temp;
    }

    template <typename T>
        requires is_vector<T>
    static inline auto tan(const T& a)
    {
        T temp;
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp[i] = std::tan(a[i]);
        }
        return temp;
    }

    template <typename T>
        requires is_vector<T>
    static inline auto asin(const T& a)
    {
        T temp;
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp[i] = std::asin(a[i]);
        }
        return temp;
    }

    template <typename T>
        requires is_vector<T>
    static inline auto acos(const T& a)
    {
        T temp;
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp[i] = std::acos(a[i]);
        }
        return temp;
    }

    template <typename T>
        requires is_vector<T>
    static inline auto atan(const T& a)
    {
        T temp;
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp[i] = std::atan(a[i]);
        }
        return temp;
    }

    template <typename T>
        requires is_vector<T>
    static inline auto exp(const T& a)
    {
        T temp;
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp[i] = std::exp(a[i]);
        }
        return temp;
    }

    template <typename T>
        requires is_vector<T>
    static inline auto log(const T& a)
    {
        T temp;
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp[i] = std::log(a[i]);
        }
        return temp;
    }

    template <typename T>
        requires is_vector<T>
    static inline auto exp2(const T& a)
    {
        T temp;
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp[i] = std::exp2(a[i]);
        }
        return temp;
    }

    template <typename T>
        requires is_vector<T>
    static inline auto log2(const T& a)
    {
        T temp;
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp[i] = std::log2(a[i]);
        }
        return temp;
    }

    template <typename T>
        requires is_vector<T>
    static inline auto pow(const T& a, const T& b)
    {
        T temp;
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp[i] = std::pow(a[i], b[i]);
        }
        return temp;
    }

    template <typename T>
        requires is_vector<T>
    static inline auto atan2(const T& a, const T& b)
    {
        T temp;
        for (int i = 0; i < T::VectorSize; ++i)
        {
            temp[i] = std::atan2(a[i], b[i]);
        }
        return temp;
    }

    // ------------------------------------------------------------------
    // simd vector functions
    // ------------------------------------------------------------------

    template <typename T>
        requires is_simd_vector<T>
    static inline auto unpacklo(const T& a, const T& b) -> T
    {
        return simd::unpacklo(a, b);
    }

    template <typename T>
        requires is_simd_vector<T>
    static inline auto unpackhi(const T& a, const T& b) -> T
    {
        return simd::unpackhi(a, b);
    }

    // ------------------------------------------------------------------
    // simd integer functions
    // ------------------------------------------------------------------

    template <typename T>
    concept is_saturating_integer_vector = is_simd_vector<T> &&
        std::is_integral_v<typename T::scalar> &&
        (sizeof(typename T::ScalarType) == 1 ||
         sizeof(typename T::ScalarType) == 2 ||
         sizeof(typename T::ScalarType) == 4);

    template <typename T>
        requires is_saturating_integer_vector<T>
    static inline auto adds(const T& a, const T& b) -> T
    {
        return simd::adds(a, b);
    }

    template <typename T>
        requires is_saturating_integer_vector<T>
    static inline auto subs(const T& a, const T& b) -> T
    {
        return simd::subs(a, b);
    }

    template <typename A, typename B>
    concept is_multiplying_integer_vector = has_simd_vector<A, B> &&
        std::is_integral_v<typename first_simd_vector_t<A, B>::ScalarType> &&
        (sizeof(typename first_simd_vector_t<A, B>::ScalarType) == 2 ||
         sizeof(typename first_simd_vector_t<A, B>::ScalarType) == 4);

    template <typename A, typename B>
        requires is_multiplying_integer_vector<A, B>
    static inline auto operator * (const A& a, const B& b)
    {
        using T = first_simd_vector_t<A, B>;
        return T(simd::mullo(T(a), T(b)));
    }

    // ------------------------------------------------------------------
    // masked simd functions
    // ------------------------------------------------------------------

    template <typename T, typename M>
        requires is_simd_vector<T> && simd::is_mask<M>
    static inline auto add(const T& a, const T& b, M mask) -> T
    {
        return simd::add(a, b, mask);
    }

    template <typename T, typename M>
        requires is_simd_vector<T> && simd::is_mask<M>
    static inline auto add(const T& a, const T& b, M mask, const T& value) -> T
    {
        return simd::add(a, b, mask, value);
    }

    template <typename T, typename M>
        requires is_simd_vector<T> && simd::is_mask<M>
    static inline auto sub(const T& a, const T& b, M mask) -> T
    {
        return simd::sub(a, b, mask);
    }

    template <typename T, typename M>
        requires is_simd_vector<T> && simd::is_mask<M>
    static inline auto sub(const T& a, const T& b, M mask, const T& value) -> T
    {
        return simd::sub(a, b, mask, value);
    }

    template <typename T, typename M>
        requires is_simd_vector<T> && simd::is_mask<M>
    static inline auto min(const T& a, const T& b, M mask) -> T
    {
        return simd::min(a, b, mask);
    }

    template <typename T, typename M>
        requires is_simd_vector<T> && simd::is_mask<M>
    static inline auto min(const T& a, const T& b, M mask, const T& value) -> T
    {
        return simd::min(a, b, mask, value);
    }

    template <typename T, typename M>
        requires is_simd_vector<T> && simd::is_mask<M>
    static inline auto max(const T& a, const T& b, M mask) -> T
    {
        return simd::max(a, b, mask);
    }

    template <typename T, typename M>
        requires is_simd_vector<T> && simd::is_mask<M>
    static inline auto max(const T& a, const T& b, M mask, const T& value) -> T
    {
        return simd::max(a, b, mask, value);
    }

    template <typename T, typename M>
        requires is_simd_vector<T> && is_float_vector<T> && simd::is_mask<M>
    static inline auto mul(const T& a, const T& b, M mask) -> T
    {
        return simd::mul(a, b, mask);
    }

    template <typename T, typename M>
        requires is_simd_vector<T> && is_float_vector<T> && simd::is_mask<M>
    static inline auto mul(const T& a, const T& b, M mask, const T& value) -> T
    {
        return simd::mul(a, b, mask, value);
    }

    template <typename T, typename M>
        requires is_simd_vector<T> && is_float_vector<T> && simd::is_mask<M>
    static inline auto div(const T& a, const T& b, M mask) -> T
    {
        return simd::div(a, b, mask);
    }

    template <typename T, typename M>
        requires is_simd_vector<T> && is_float_vector<T> && simd::is_mask<M>
    static inline auto div(const T& a, const T& b, M mask, const T& value) -> T
    {
        return simd::div(a, b, mask, value);
    }

    // ------------------------------------------------------------------
    // bitwise operators
    // ------------------------------------------------------------------

    template <typename A>
        requires resolves_to_vector<A>
    static inline auto operator ~ (const A& a)
    {
        using T = resolve_t<A, A>;
        return T(simd::bitwise_not(T(a)));
    }

    template <typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto operator & (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return T(simd::bitwise_and(T(a), T(b)));
    }

    template <typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto operator | (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return T(simd::bitwise_or(T(a), T(b)));
    }

    template <typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto operator ^ (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return T(simd::bitwise_xor(T(a), T(b)));
    }

    template <typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto nand(const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return T(simd::bitwise_nand(T(a), T(b)));
    }

    // ------------------------------------------------------------------
    // compare operators
    // ------------------------------------------------------------------

    template <typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto operator > (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return simd::compare_gt(T(a), T(b));
    }

    template <typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto operator >= (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return simd::compare_ge(T(a), T(b));
    }

    template <typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto operator < (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return simd::compare_lt(T(a), T(b));
    }

    template <typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto operator <= (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return simd::compare_le(T(a), T(b));
    }

    template <typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto operator == (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return simd::compare_eq(T(a), T(b));
    }

    template <typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto operator != (const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return simd::compare_neq(T(a), T(b));
    }

    template <typename M, typename A, typename B>
        requires resolves_to_vector<A> || resolves_to_vector<B>
    static inline auto select(M mask, const A& a, const B& b)
    {
        using T = resolve_t<A, B>;
        return T(simd::select(mask, T(a), T(b)));
    }

    // ------------------------------------------------------------------
    // maskToInt()
    // ------------------------------------------------------------------

    static inline u32 maskToInt(mask8x16 mask) noexcept
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask16x8 mask) noexcept
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask32x4 mask) noexcept
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask64x2 mask) noexcept
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask8x32 mask) noexcept
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask16x16 mask) noexcept
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask32x8 mask) noexcept
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask64x4 mask) noexcept
    {
        return simd::get_mask(mask);
    }

    static inline u64 maskToInt(mask8x64 mask) noexcept
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask16x32 mask) noexcept
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask32x16 mask) noexcept
    {
        return simd::get_mask(mask);
    }

    static inline u32 maskToInt(mask64x8 mask) noexcept
    {
        return simd::get_mask(mask);
    }

    // ------------------------------------------------------------------
    // mask reduction
    // ------------------------------------------------------------------

    template <typename T>
    static inline bool none_of(T mask) noexcept
    {
        return simd::none_of(mask);
    }

    template <typename T>
    static inline bool any_of(T mask) noexcept
    {
        return simd::any_of(mask);
    }

    template <typename T>
    static inline bool all_of(T mask) noexcept
    {
        return simd::all_of(mask);
    }

    // ------------------------------------------------------------------
    // reinterpret / convert
    // ------------------------------------------------------------------

    // The reinterpret and conversion casts forward the work to the simd abstraction.
    // This is enforced by requiring "VectorType" declaration in the Vector specialization.

    template <typename D, typename S>
    static constexpr D reinterpret(S s) noexcept
    {
        typename S::VectorType temp = s;
        return simd::reinterpret<typename D::VectorType>(temp);
    }

    template <typename D, typename S>
    static constexpr D convert(S s) noexcept
    {
        typename S::VectorType temp = s;
        return simd::convert<typename D::VectorType>(temp);
    }

    template <typename D, typename S>
    static constexpr D truncate(S s) noexcept
    {
        typename S::VectorType temp = s;
        return simd::truncate<typename D::VectorType>(temp);
    }

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
    // specializations:
    //     load_low()
    // ------------------------------------------------------------------

    template <typename ScalarType, int VectorSize>
    static constexpr
    Vector<ScalarType, VectorSize> load_low(const ScalarType *source) noexcept
    {
        MANGO_UNREFERENCED(source);

        // load_low() is not available by default
        Vector<ScalarType, VectorSize>::undefined_operation();
    }

    // ------------------------------------------------------------------
    // macros
    // ------------------------------------------------------------------

    // Hide the worst hacks here in the end. We desperately want to use
    // API that allows to the "constant integer" parameter to be passed as-if
    // the shift was a normal function. CLANG implementation for example does not
    // accept anything else so we do this immoral macro sleight-of-hand to get
    // what we want. The count still has to be a compile-time constant, of course.


    #define slli(Value, Count) decltype(Value)(simd::slli<Count>(Value))
    #define srli(Value, Count) decltype(Value)(simd::srli<Count>(Value))
    #define srai(Value, Count) decltype(Value)(simd::srai<Count>(Value))

} // namespace mango::math

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

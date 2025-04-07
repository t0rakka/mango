/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

namespace mango::math
{

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
    concept is_scalar = std::is_scalar_v<T>;

    template <typename T>
    struct is_shuffle_accessor : std::false_type {};

    template <typename VectorType, typename StorageType, int... Indices>
    struct is_shuffle_accessor<ShuffleAccessor<VectorType, StorageType, Indices...>> : std::true_type {};

    template <typename T>
    concept is_vector = requires(T v)
    {
        { T::VectorSize };
        typename T::ScalarType;
        { v[0] } -> std::convertible_to<typename T::ScalarType>;
    };

    template <typename T>
    concept is_vector_or_scalar = is_vector<T> || is_scalar<T>;

    template <typename T>
    concept is_simd_vector = requires(T v)
    {
        typename T::VectorType;
        { T::VectorSize };
        requires !std::same_as<typename T::VectorType, void>;
    };

    template <typename T>
    concept is_simd_vector_or_scalar = is_simd_vector<T> || is_scalar<T>;

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

    template <typename T, typename... Args>
    struct first_vector_type<T, Args...>
    {
        using type = std::conditional_t<
            is_vector<T> || is_shuffle_accessor<std::remove_cvref_t<T>>::value,
            typename get_vector_type<T>::type,
            typename first_vector_type<Args...>::type>;
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
            m = simd::set_component<Index>(m, accessor);
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

    // operator +

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator + (const ScalarAccessor<ScalarType, VectorType, Index>& a) noexcept
    {
        // +a
        return ScalarType(a);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    ScalarType operator + (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        // a + b
        return ScalarType(a) + ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator + (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b) noexcept
    {
        // a + s
        return ScalarType(a) + b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator + (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        // s + a
        return a + ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    static constexpr
    Vector<ScalarType, N> operator + (const ScalarAccessor<ScalarType, VectorType, Index>& a, Vector<ScalarType, N> b) noexcept
    {
        // a + v
        return ScalarType(a) + b;
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    static constexpr
    Vector<ScalarType, N> operator + (Vector<ScalarType, N> a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        // v + a
        return a + ScalarType(b);
    }

    // operator -

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator - (const ScalarAccessor<ScalarType, VectorType, Index>& a) noexcept
    {
        // -a
        return -ScalarType(a);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    ScalarType operator - (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        // a - b
        return ScalarType(a) - ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator - (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b) noexcept
    {
        // a - s
        return ScalarType(a) - b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator - (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        // s - a
        return a - ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    static constexpr
    Vector<ScalarType, N> operator - (const ScalarAccessor<ScalarType, VectorType, Index>& a, Vector<ScalarType, N> b) noexcept
    {
        // a - v
        return ScalarType(a) - b;
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    static constexpr
    Vector<ScalarType, N> operator - (Vector<ScalarType, N> a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        // v - a
        return a - ScalarType(b);
    }

    // operator *

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    ScalarType operator * (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        // a * b
        return ScalarType(a) * ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator * (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b) noexcept
    {
        // a * s
        return ScalarType(a) * b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator * (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        // s * a
        return a * ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    static constexpr
    Vector<ScalarType, N> operator * (const ScalarAccessor<ScalarType, VectorType, Index>& a, Vector<ScalarType, N> b) noexcept
    {
        // a * v
        return ScalarType(a) * b;
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    static constexpr
    Vector<ScalarType, N> operator * (Vector<ScalarType, N> a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        // v * a
        return a * ScalarType(b);
    }

    // operator /

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    ScalarType operator / (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        // a / b
        return ScalarType(a) / ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator / (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b) noexcept
    {
        // a / s
        return ScalarType(a) / b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    static constexpr
    ScalarType operator / (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        // s / a
        return a / ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    static constexpr
    Vector<ScalarType, N> operator / (const ScalarAccessor<ScalarType, VectorType, Index>& a, Vector<ScalarType, N> b) noexcept
    {
        // a / v
        return ScalarType(a) / b;
    }

    template <typename ScalarType, typename VectorType, int Index, int N>
    static constexpr
    Vector<ScalarType, N> operator / (Vector<ScalarType, N> a, const ScalarAccessor<ScalarType, VectorType, Index>& b) noexcept
    {
        // v / a
        return a / ScalarType(b);
    }

    // operator *=

    template <typename ScalarType, typename VectorType, int Index, int N>
    static constexpr
    Vector<ScalarType, N>& operator *= (Vector<ScalarType, N>& a, ScalarAccessor<ScalarType, VectorType, Index> b) noexcept
    {
        a = a * ScalarType(b);
        return a;
    }

    // operator /=

    template <typename ScalarType, typename VectorType, int Index, int N>
    static constexpr
    Vector<ScalarType, N>& operator /= (Vector<ScalarType, N>& a, ScalarAccessor<ScalarType, VectorType, Index> b) noexcept
    {
        a = a / ScalarType(b);
        return a;
    }

    // compare

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    bool operator < (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                     const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) < ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    bool operator > (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                     const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) > ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    bool operator <= (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                      const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) <= ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    bool operator >= (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                      const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) >= ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    bool operator == (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                      const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) == ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    static constexpr
    bool operator != (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                      const ScalarAccessor<ScalarType, VectorType, Index1>& b) noexcept
    {
        return ScalarType(a) != ScalarType(b);
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

    // vec2 -> vec2 shuffle
    template <typename VectorType, typename StorageType, int X, int Y>
        requires (VectorType::VectorSize == 2)
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
            const ScalarType x = simd::get_component<X>(m);
            const ScalarType y = simd::get_component<Y>(m);
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
            const ScalarType x = simd::get_component<X>(m);
            const ScalarType y = simd::get_component<Y>(m);
            const ScalarType z = simd::get_component<Z>(m);
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

    // Scalar operators for ShuffleAccessor

    template <typename VectorType, typename StorageType, int... Indices>
    constexpr auto operator * (const ShuffleAccessor<VectorType, StorageType, Indices...>& a, typename VectorType::ScalarType s) noexcept
    {
        return VectorType(a) * s;
    }

    template <typename VectorType, typename StorageType, int... Indices>
    constexpr auto operator * (typename VectorType::ScalarType s, const ShuffleAccessor<VectorType, StorageType, Indices...>& a) noexcept
    {
        return s * VectorType(a);
    }

    template <typename VectorType, typename StorageType, int... Indices>
    constexpr auto operator / (const ShuffleAccessor<VectorType, StorageType, Indices...>& a, typename VectorType::ScalarType s) noexcept
    {
        return VectorType(a) / s;
    }

    template <typename VectorType, typename StorageType, int... Indices>
    constexpr auto operator + (const ShuffleAccessor<VectorType, StorageType, Indices...>& a, typename VectorType::ScalarType s) noexcept
    {
        return VectorType(a) + s;
    }

    template <typename VectorType, typename StorageType, int... Indices>
    constexpr auto operator + (typename VectorType::ScalarType s, const ShuffleAccessor<VectorType, StorageType, Indices...>& a) noexcept
    {
        return s + VectorType(a);
    }

    template <typename VectorType, typename StorageType, int... Indices>
    constexpr auto operator - (const ShuffleAccessor<VectorType, StorageType, Indices...>& a, typename VectorType::ScalarType s) noexcept
    {
        return VectorType(a) - s;
    }

    template <typename VectorType, typename StorageType, int... Indices>
    constexpr auto operator - (typename VectorType::ScalarType s, const ShuffleAccessor<VectorType, StorageType, Indices...>& a) noexcept
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

    // operator -

    template <typename T>
        requires is_signed_vector<T>
    static inline T operator - (const T& a)
    {
        return vector_ops<T>::neg(a);
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto operator - (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return vector_ops<T>::sub(T(a), T(b));
    }

    template <typename A, typename B>
        requires is_vector<A> && is_vector_or_scalar<B>
    static inline A& operator -= (A& a, const B& b)
    {
        a = vector_ops<A>::sub(a, A(b));
        return a;
    }

    // operator +

    template <typename T>
        requires is_vector<T>
    static inline T operator + (const T& a)
    {
        return a;
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto operator + (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return vector_ops<T>::add(T(a), T(b));
    }

    template <typename A, typename B>
        requires is_vector<A> && is_vector_or_scalar<B>
    static inline A& operator += (A& a, const B& b)
    {
        a = vector_ops<A>::add(a, A(b));
        return a;
    }

    // operator *

    template <typename A, typename B>
        requires has_vector<A, B> && is_float_vector<first_vector_t<A, B>>
    static inline auto operator * (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return vector_ops<T>::mul(T(a), T(b));
    }

    template <typename A, typename B>
        requires is_vector<A> && is_float_vector<A> && is_vector_or_scalar<B>
    static inline A& operator *= (A& a, const B& b)
    {
        a = vector_ops<A>::mul(a, A(b));
        return a;
    }

    // operator /

    template <typename A, typename B>
        requires has_vector<A, B> && is_float_vector<first_vector_t<A, B>>
    static inline auto operator / (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return vector_ops<T>::div(T(a), T(b));
    }

    template <typename A, typename B>
        requires is_vector<A> && is_float_vector<A> && is_vector_or_scalar<B>
    static inline A& operator /= (A& a, const B& b)
    {
        a = vector_ops<A>::div(a, A(b));
        return a;
    }

    // functions

    template <typename T>
        requires is_vector<T>
    static inline T abs(const T& a)
    {
        return vector_ops<T>::abs(a);
    }

    template <typename A, typename B, typename C>
        requires has_vector<A, B, C>
    static inline auto madd(const A& a, const B& b, const C& c)
    {
        using T = first_vector_t<A, B, C>;
        return vector_ops<T>::madd(T(a), T(b), T(c));
    }

    template <typename A, typename B, typename C>
        requires has_vector<A, B, C>
    static inline auto msub(const A& a, const B& b, const C& c)
    {
        using T = first_vector_t<A, B, C>;
        return vector_ops<T>::msub(T(a), T(b), T(c));
    }

    template <typename A, typename B, typename C>
        requires has_vector<A, B, C>
    static inline auto nmadd(const A& a, const B& b, const C& c)
    {
        using T = first_vector_t<A, B, C>;
        return vector_ops<T>::nmadd(T(a), T(b), T(c));
    }

    template <typename A, typename B, typename C>
        requires has_vector<A, B, C>
    static inline auto nmsub(const A& a, const B& b, const C& c)
    {
        using T = first_vector_t<A, B, C>;
        return vector_ops<T>::nmsub(T(a), T(b), T(c));
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto min(const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return vector_ops<T>::min(T(a), T(b));
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto max(const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return vector_ops<T>::max(T(a), T(b));
    }

    template <typename T>
        requires is_vector<T> && (T::VectorSize >= 2 || T::VectorSize <= 4)
    static inline auto sign(const T& a)
    {
        return vector_ops<T>::sign(a);
    }

    template <typename A, typename B, typename C>
        requires has_vector<A, B, C>
    static inline auto clamp(const A& a, const B& b, const C& c)
    {
        using T = first_vector_t<A, B, C>;
        return vector_ops<T>::clamp(T(a), T(b), T(c));
    }

    template <typename T, typename S>
        requires is_vector<T> && is_scalar<S>
    static inline auto lerp(const T& a, const T& b, S s)
    {
        return vector_ops<T>::lerp(a, b, typename T::ScalarType(s));
    }

    template <typename T>
        requires is_vector<T>
    static inline auto radians(const T& a)
    {
        return vector_ops<T>::radians(a);
    }

    template <typename T>
        requires is_vector<T>
    static inline auto degrees(const T& a)
    {
        return vector_ops<T>::degrees(a);
    }

    template <typename T>
        requires is_vector<T>
    static inline auto rcp(const T& a)
    {
        return vector_ops<T>::rcp(a);
    }

    template <typename T>
        requires is_vector<T>
    static inline auto sqrt(const T& a)
    {
        return vector_ops<T>::sqrt(a);
    }

    template <typename T>
        requires is_vector<T>
    static inline auto rsqrt(const T& a)
    {
        return vector_ops<T>::rsqrt(a);
    }

    template <typename T>
        requires is_vector<T>
    static inline auto round(const T& a)
    {
        return vector_ops<T>::round(a);
    }

    template <typename T>
        requires is_vector<T>
    static inline auto floor(const T& a)
    {
        return vector_ops<T>::floor(a);
    }

    template <typename T>
        requires is_vector<T>
    static inline auto ceil(const T& a)
    {
        return vector_ops<T>::ceil(a);
    }

    template <typename T>
        requires is_vector<T>
    static inline auto trunc(const T& a)
    {
        return vector_ops<T>::trunc(a);
    }

    template <typename T>
        requires is_vector<T>
    static inline auto fract(const T& a)
    {
        return vector_ops<T>::fract(a);
    }

    template <typename T>
        requires is_vector<T>
    static inline auto mod(const T& a, const T& b)
    {
        return vector_ops<T>::mod(a, b);
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

    template <typename T>
        requires is_vector<T>
    static inline auto operator ~ (const T& a) -> T
    {
        return simd::bitwise_not(a);
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto operator & (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return T(simd::bitwise_and(T(a), T(b)));
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto operator | (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return T(simd::bitwise_or(T(a), T(b)));
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto operator ^ (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return T(simd::bitwise_xor(T(a), T(b)));
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto nand(const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return T(simd::bitwise_nand(T(a), T(b)));
    }

    // ------------------------------------------------------------------
    // compare operators
    // ------------------------------------------------------------------

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto operator > (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return simd::compare_gt(T(a), T(b));
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto operator >= (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return simd::compare_ge(T(a), T(b));
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto operator < (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return simd::compare_lt(T(a), T(b));
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto operator <= (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return simd::compare_le(T(a), T(b));
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto operator == (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return simd::compare_eq(T(a), T(b));
    }

    template <typename A, typename B>
        requires has_vector<A, B>
    static inline auto operator != (const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
        return simd::compare_neq(T(a), T(b));
    }

    template <typename M, typename A, typename B>
        requires has_vector<A, B>
    static inline auto select(M mask, const A& a, const B& b)
    {
        using T = first_vector_t<A, B>;
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

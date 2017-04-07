/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cassert>
#include <algorithm>
#include "../simd/simd.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // VectorBase
    // ------------------------------------------------------------------

    template <typename Type, int Size>
    struct VectorBase
    {
        operator Type* ()
        {
            return reinterpret_cast<Type*>(this);
        }

        operator const Type* () const
        {
            return reinterpret_cast<const Type*>(this);
        }

        Type& operator [] (int index)
        {
            assert(index >= 0 && index < Size);
            return reinterpret_cast<Type*>(this)[index];
        }

        const Type& operator [] (int index) const
        {
            assert(index >= 0 && index < Size);
            return reinterpret_cast<const Type*>(this)[index];
        }
    };

    // ------------------------------------------------------------------
    // Vector
    // ------------------------------------------------------------------

    template <typename Type, int Size>
    struct Vector : VectorBase<Type, Size>
    {
        Type m[Size];

        explicit Vector()
        {
        }

        explicit Vector(Type s)
        {
            for (int i = 0; i < Size; ++i)
            {
                m[i] = s;
            }
        }

        Vector(const Vector& v)
        {
            for (int i = 0; i < Size; ++i)
            {
                m[i] = v[i];
            }
        }

        ~Vector()
        {
        }

        Vector& operator = (Type s)
        {
            for (int i = 0; i < Size; ++i)
            {
                m[i] = s;
            }
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
            for (int i = 0; i < Size; ++i)
            {
                m[i] = v[i];
            }
            return *this;
        }
    };

    template <typename Type>
    struct Vector<Type, 2> : VectorBase<Type, 2>
    {
        enum { Size = 2 };
        union
        {
            Type m[Size];
            struct
            {
                Type x, y;
            };
        };

        explicit Vector()
        {
        }

        explicit Vector(Type s)
        : x(s), y(s)
        {
        }

        explicit Vector(Type s0, Type s1)
        : x(s0), y(s1)
        {
        }

        Vector(const Vector& v)
        : x(v.x), y(v.y)
        {
        }

        ~Vector()
        {
        }

        Vector& operator = (Type s)
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
    };

    template <typename Type>
    struct Vector<Type, 3> : VectorBase<Type, 3>
    {
        enum { Size = 3 };
        union
        {
            Type m[Size];
            struct
            {
                Type x, y, z;
            };
        };

        explicit Vector()
        {
        }

        explicit Vector(Type s)
        : x(s), y(s), z(s)
        {
        }

        explicit Vector(Type s0, Type s1, Type s2)
        : x(s0), y(s1), z(s2)
        {
        }

        explicit Vector(const Vector<Type, 2>& v, Type s)
        : x(v.x), y(v.y), z(s)
        {
        }

        Vector(const Vector& v)
        : x(v.x), y(v.y), z(v.z)
        {
        }

        ~Vector()
        {
        }

        Vector& operator = (Type s)
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
    };

    template <typename Type>
    struct Vector<Type, 4> : VectorBase<Type, 4>
    {
        enum { Size = 4 };
        union
        {
            Type m[Size];
            struct
            {
                Type x, y, z, w;
            };
        };

        explicit Vector()
        {
        }

        explicit Vector(Type s)
        : x(s), y(s), z(s), w(s)
        {
        }

        explicit Vector(Type s0, Type s1, Type s2, Type s3)
        : x(s0), y(s1), z(s2), w(s3)
        {
        }

        explicit Vector(const Vector<Type, 2>& v0, const Vector<Type, 2>& v1)
        : x(v0.x), y(v0.y), z(v1.x), w(v1.y)
        {
        }

        explicit Vector(const Vector<Type, 3>& v, Type s)
        : x(v.x), y(v.y), z(v.z), w(s)
        {
        }

        Vector(const Vector& v)
        : x(v.x), y(v.y), z(v.z), w(v.w)
        {
        }

        ~Vector()
        {
        }

        Vector& operator = (Type s)
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
    };

    // ------------------------------------------------------------------
    // Vector operators
    // ------------------------------------------------------------------

    template <typename Type, int Size>
    static inline const Vector<Type, Size>& operator + (const Vector<Type, Size>& v)
    {
        return v;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size> operator - (const Vector<Type, Size>& v)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = -v[i];
        }
        return temp;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size>& operator += (Vector<Type, Size>& a, const Vector<Type, Size>& b)
    {
        for (int i = 0; i < Size; ++i)
        {
            a[i] += b[i];
        }
        return a;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size>& operator -= (Vector<Type, Size>& a, const Vector<Type, Size>& b)
    {
        for (int i = 0; i < Size; ++i)
        {
            a[i] -= b[i];
        }
        return a;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size>& operator *= (Vector<Type, Size>& a, const Vector<Type, Size>& b)
    {
        for (int i = 0; i < Size; ++i)
        {
            a[i] *= b[i];
        }
        return a;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size>& operator *= (Vector<Type, Size>& a, Type b)
    {
        for (int i = 0; i < Size; ++i)
        {
            a[i] *= b;
        }
        return a;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size>& operator /= (Vector<Type, Size>& a, const Vector<Type, Size>& b)
    {
        for (int i = 0; i < Size; ++i)
        {
            a[i] /= b[i];
        }
        return a;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size>& operator /= (Vector<Type, Size>& a, Type b)
    {
        for (int i = 0; i < Size; ++i)
        {
            a[i] /= b;
        }
        return a;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size> operator + (const Vector<Type, Size>& a, const Vector<Type, Size>& b)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = a[i] + b[i];
        }
        return temp;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size> operator - (const Vector<Type, Size>& a, const Vector<Type, Size>& b)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = a[i] - b[i];
        }
        return temp;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size> operator * (const Vector<Type, Size>& a, const Vector<Type, Size>& b)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = a[i] * b[i];
        }
        return temp;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size> operator * (const Vector<Type, Size>& a, Type b)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = a[i] * b;
        }
        return temp;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size> operator * (Type a, const Vector<Type, Size>& b)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = a * b[i];
        }
        return temp;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size> operator / (const Vector<Type, Size>& a, const Vector<Type, Size>& b)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = a[i] / b[i];
        }
        return temp;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size> operator / (const Vector<Type, Size>& a, Type b)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = a[i] / b;
        }
        return temp;
    }

    // ------------------------------------------------------------------
    // Vector functions
    // ------------------------------------------------------------------

    template <typename Type, int Size>
    static inline const Vector<Type, Size> abs(const Vector<Type, Size>& a)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = std::abs(a[i]);
        }
        return temp;
    }

    template <typename Type, int Size>
    static inline const Vector<Type, Size> min(const Vector<Type, Size>& a, const Vector<Type, Size>& b)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = std::min(a[i], b[i]);
        }
        return temp;
    }

    template <typename Type, int Size>
    static inline const Vector<Type, Size> max(const Vector<Type, Size>& a, const Vector<Type, Size>& b)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = std::max(a[i], b[i]);
        }
        return temp;
    }

    template <typename Type, int Size>
    static inline Type dot(const Vector<Type, Size>& a, const Vector<Type, Size>& b)
    {
        Type s = 0;
        for (int i = 0; i < Size; ++i)
        {
            s += a[i] * b[i];
        }
        return s;
    }

    template <typename Type, int Size>
    static inline Type square(const Vector<Type, Size>& a)
    {
        Type s = 0;
        for (int i = 0; i < Size; ++i)
        {
            s += a[i] * a[i];
        }
        return s;
    }

    template <typename Type, int Size>
    static inline Type length(const Vector<Type, Size>& a)
    {
        Type s = 0;
        for (int i = 0; i < Size; ++i)
        {
            s += a[i] * a[i];
        }
        return static_cast<Type>(std::sqrt(s));
    }

    template <typename Type, int Size>
    static inline const Vector<Type, Size> normalize(const Vector<Type, Size>& a)
    {
        return a * Type(1.0 / length(a));
    }

    template <typename Type, int Size>
    static inline const Vector<Type, Size> clamp(const Vector<Type, Size>& a, const Vector<Type, Size>& amin, const Vector<Type, Size>& amax)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = std::min(amax[i], std::max(amin[i], a[i]));
        }
        return temp;
    }

    template <typename Type, int Size>
    static inline Vector<Type, Size> madd(const Vector<Type, Size>& a, const Vector<Type, Size>& b, const Vector<Type, Size>& c)
    {
        Vector<Type, Size> temp;
        for (int i = 0; i < Size; ++i)
        {
            temp[i] = a[i] + b[i] * c[i];
        }
        return temp;
    }

    template <typename Type, int Size>
    static inline const Vector<Type, Size> lerp(const Vector<Type, Size>& a, const Vector<Type, Size>& b, Type factor)
    {
        return a + (b - a) * factor;
    }

    // ------------------------------------------------------------------
    // Vector float functions
    // ------------------------------------------------------------------

#define MAKE_VECTOR_FUNCTION1(Type, Name, Expression) \
	template <int Size> \
	static inline const Vector<Type, Size> Name(const Vector<Type, Size>& a) { \
        Vector<Type, Size> temp; \
        for (int i = 0; i < Size; ++i) { \
            temp[i] = Expression; \
        } \
        return temp; \
	}

#define MAKE_VECTOR_FUNCTION2(Type, Name, Expression) \
	template <int Size> \
	static inline const Vector<Type, Size> Name(const Vector<Type, Size>& a, const Vector<Type, Size>& b) { \
        Vector<Type, Size> temp; \
        for (int i = 0; i < Size; ++i) { \
            temp[i] = Expression; \
        } \
        return temp; \
	}

	static inline float sign(float a)
	{
		if (a < 0) a = -1.0f;
		else if (a > 0) a = 1.0f;
		return a;
	}

	constexpr static inline float radians(float a)
	{
        return a * 0.01745329251f;
	}

	constexpr static inline float degrees(float a)
	{
        return a * 57.2957795131f;
	}

	MAKE_VECTOR_FUNCTION1(float, sign, sign(a[i]))
	MAKE_VECTOR_FUNCTION1(float, radians, radians(a[i]))
	MAKE_VECTOR_FUNCTION1(float, degrees, degrees(a[i]))
	MAKE_VECTOR_FUNCTION1(float, sin, std::sin(a[i]))
	MAKE_VECTOR_FUNCTION1(float, cos, std::cos(a[i]))
	MAKE_VECTOR_FUNCTION1(float, tan, std::tan(a[i]))
	MAKE_VECTOR_FUNCTION1(float, asin, std::asin(a[i]))
	MAKE_VECTOR_FUNCTION1(float, acos, std::acos(a[i]))
	MAKE_VECTOR_FUNCTION1(float, atan, std::atan(a[i]))
	MAKE_VECTOR_FUNCTION1(float, exp, std::exp(a[i]))
	MAKE_VECTOR_FUNCTION1(float, log, std::log(a[i]))
	MAKE_VECTOR_FUNCTION1(float, exp2, std::exp2(a[i]))
	MAKE_VECTOR_FUNCTION1(float, log2, std::log2(a[i]))
	MAKE_VECTOR_FUNCTION1(float, sqrt, float(std::sqrt(a[i])))
	MAKE_VECTOR_FUNCTION1(float, rsqrt, 1.0f / std::sqrt(a[i]))
    MAKE_VECTOR_FUNCTION1(float, round, std::round(a[i]))
	MAKE_VECTOR_FUNCTION1(float, floor, std::floor(a[i]))
	MAKE_VECTOR_FUNCTION1(float, ceil, std::ceil(a[i]))
	MAKE_VECTOR_FUNCTION1(float, fract, a[i] - std::floor(a[i]))
	MAKE_VECTOR_FUNCTION2(float, pow, std::pow(a[i], b[i]))
	MAKE_VECTOR_FUNCTION2(float, mod, a[i] - b[i] * std::floor(a[i] / b[i]))
    MAKE_VECTOR_FUNCTION2(float, atan2, std::atan2(a[i], b[i]))

#undef MAKE_VECTOR_FUNCTION1
#undef MAKE_VECTOR_FUNCTION2

    // ------------------------------------------------------------------
    // Vector2 functions
    // ------------------------------------------------------------------

    template <typename Type>
    static inline Type length(const Vector<Type, 2>& v)
    {
        Type s = square(v);
        return static_cast<Type>(std::sqrt(s));
    }

    template <typename Type>
    static inline Type distance(const Vector<Type, 2>& a, const Vector<Type, 2>& b)
    {
        return length(a - b);
    }

    template <typename Type>
    static inline Vector<Type, 2> normalize(const Vector<Type, 2>& v)
    {
        Type s = length(v);
        if (s) s = Type(1.0) / s;
        return v * s;
    }

    template <typename Type>
    static inline Vector<Type, 2> project(const Vector<Type, 2>& v, const Vector<Type, 2>& normal)
    {
        return v - normal * (dot(v, normal) / dot(normal, normal));
    }

    template <typename Type>
    static inline Vector<Type, 2> reflect(const Vector<Type, 2>& v, const Vector<Type, 2>& normal)
    {
        return v - normal * (Type(2.0) * dot(v, normal));
    }

    template <typename Type>
    static inline Vector<Type, 2> refract(const Vector<Type, 2>& v, const Vector<Type, 2>& normal, Type factor)
    {
        Type vdotn = dot(v, normal);
        Type p = Type(1.0) - factor * factor * (Type(1.0) - vdotn * vdotn);
        if (p < 0)
        {
            p = 0;
            factor = 0;
        }
        else
        {
            p = Type(std::sqrt(p));
        }
        return v * factor - normal * (p + factor * vdotn);
    }

    template <typename Type>
    static inline Vector<Type, 2> hmin(const Vector<Type, 2>& v)
    {
        const Type s = std::min(v.x, v.y);
        return Vector<Type, 2>(s);
    }

    template <typename Type>
    static inline Vector<Type, 2> hmax(const Vector<Type, 2>& v)
    {
        const Type s = std::max(v.x, v.y);
        return Vector<Type, 2>(s);
    }

    // ------------------------------------------------------------------
    // Vector3 functions
    // ------------------------------------------------------------------

    template <typename Type>
    static inline Vector<Type, 3> cross(const Vector<Type, 3>& a, const Vector<Type, 3>& b)
    {
        Type x = a.y * b.z - a.z * b.y;
        Type y = a.z * b.x - a.x * b.z;
        Type z = a.x * b.y - a.y * b.x;
        return Vector<Type, 3>(x, y, z);
    }

    template <typename Type>
    static inline Type length(const Vector<Type, 3>& v)
    {
        Type s = square(v);
        return static_cast<Type>(std::sqrt(s));
    }

    template <typename Type>
    static inline Type distance(const Vector<Type, 3>& a, const Vector<Type, 3>& b)
    {
        return length(a - b);
    }

    template <typename Type>
    static inline Vector<Type, 3> normalize(const Vector<Type, 3>& v)
    {
        Type s = length(v);
        if (s) s = Type(1.0) / s;
        return v * s;
    }

    template <typename Type>
    static inline Vector<Type, 3> project(const Vector<Type, 3>& v, const Vector<Type, 3>& normal)
    {
        return v - normal * (dot(v, normal) / dot(normal, normal));
    }

    template <typename Type>
    static inline Vector<Type, 3> reflect(const Vector<Type, 3>& v, const Vector<Type, 3>& normal)
    {
        return v - normal * (Type(2.0) * dot(v, normal));
    }

    template <typename Type>
    static inline Vector<Type, 3> refract(const Vector<Type, 3>& v, const Vector<Type, 3>& normal, Type factor)
    {
        Type vdotn = dot(v, normal);
        Type p = Type(1.0) - factor * factor * (Type(1.0) - vdotn * vdotn);
        if (p < 0)
        {
            p = 0;
            factor = 0;
        }
        else
        {
            p = Type(std::sqrt(p));
        }
        return v * factor - normal * (p + factor * vdotn);
    }

    template <typename Type>
    static inline Vector<Type, 3> hmin(const Vector<Type, 3>& v)
    {
        const Type s = std::min(std::min(v.x, v.y), v.z);
        return Vector<Type, 3>(s);
    }

    template <typename Type>
    static inline Vector<Type, 3> hmax(const Vector<Type, 3>& v)
    {
        const Type s = std::max(std::max(v.x, v.y), v.z);
        return Vector<Type, 3>(s);
    }

    // ------------------------------------------------------------------
    // ScalarAccessor
    // ------------------------------------------------------------------

    template <typename ScalarType, typename VectorType, int Index>
    struct ScalarAccessor
    {
        VectorType m;

        operator ScalarType () const
        {
            return simd::get_component<Index>(m);
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

    // ------------------------------------------------------------------
    // Permute2
    // ------------------------------------------------------------------

    template <typename ScalarType, typename VectorType, int X, int Y>
    struct Permute2
    {
        VectorType m;

		operator Vector<ScalarType, 2> () const
		{
            return simd::shuffle<X, Y>(m);
		}
    };

    // ------------------------------------------------------------------
    // Permute4
    // ------------------------------------------------------------------

    template <typename ScalarType, typename VectorType, int X, int Y, int Z, int W>
    struct Permute4
    {
        VectorType m;

        operator Vector<ScalarType, 4> () const
        {
            return simd::shuffle<X, Y, Z, W>(m);
        }
    };

    // ------------------------------------------------------------------
    // Permute4x2
    // ------------------------------------------------------------------

    template <typename ScalarType, typename VectorType, int X, int Y>
    struct Permute4x2
    {
        VectorType m;

        operator Vector<ScalarType, 2> () const
        {
            const VectorType temp = simd::shuffle<X, Y, X, Y>(m);
            return simd::get_low(temp);
        }
    };

    template <typename ScalarType, typename VectorType>
    struct Permute4x2<ScalarType, VectorType, 0, 1>
    {
        VectorType m;

        operator Vector<ScalarType, 2> () const
        {
            return simd::get_low(m);
        }
    };

    template <typename ScalarType, typename VectorType>
    struct Permute4x2<ScalarType, VectorType, 2, 3>
    {
        VectorType m;

        operator Vector<ScalarType, 2> () const
        {
            return simd::get_high(m);
        }
    };

    // ------------------------------------------------------------------
    // Permute4x3
    // ------------------------------------------------------------------

    template <typename ScalarType, typename VectorType, int X, int Y, int Z>
    struct Permute4x3
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
    // typedefs
    // ------------------------------------------------------------------

    typedef Vector<int, 2>      int2;
    typedef Vector<int, 3>      int3;
    typedef Vector<int, 4>      int4;

    typedef Vector<half, 4>     half4;

    typedef Vector<float, 2>    float2;
    typedef Vector<float, 3>    float3;
    typedef Vector<float, 4>    float4;
    typedef Vector<float, 8>    float8;

    typedef Vector<double, 2>   double2;
    typedef Vector<double, 3>   double3;
    typedef Vector<double, 4>   double4;

} // namespace mango

#include "vector_float16x4.hpp"
#include "vector_float32x2.hpp"
#include "vector_float32x3.hpp"
#include "vector_float32x4.hpp"
#include "vector_float32x8.hpp"
#include "vector_float64x2.hpp"
#include "vector_float64x4.hpp"

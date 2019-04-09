/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"
#include "vector_float32x2.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // Vector<float, 3>
    // ------------------------------------------------------------------

    template <>
    struct Vector<float, 3>
    {
        using VectorType = void;
        using ScalarType = float;
        enum { VectorSize = 3 };

        template <int X, int Y>
        struct ShuffleAccessor2
        {
			float v[3];

			operator Vector<float, 2> () const
			{
				return Vector<float, 2>(v[X], v[Y]);
			}
        };

        template <int X, int Y, int Z>
        struct ShuffleAccessor3
        {
			float v[3];

			operator Vector<float, 3> () const
			{
				return Vector<float, 3>(v[X], v[Y], v[Z]);
			}
        };

        union
        {
            struct { float x, y, z; };

			ShuffleAccessor2<0, 0> xx;
			ShuffleAccessor2<1, 0> yx;
			ShuffleAccessor2<2, 0> zx;
			ShuffleAccessor2<0, 1> xy;
			ShuffleAccessor2<1, 1> yy;
			ShuffleAccessor2<2, 1> zy;
			ShuffleAccessor2<0, 2> xz;
			ShuffleAccessor2<1, 2> yz;
			ShuffleAccessor2<2, 2> zz;

            ShuffleAccessor3<0, 0, 0> xxx;
            ShuffleAccessor3<1, 0, 0> yxx;
            ShuffleAccessor3<2, 0, 0> zxx;
            ShuffleAccessor3<0, 1, 0> xyx;
            ShuffleAccessor3<1, 1, 0> yyx;
            ShuffleAccessor3<2, 1, 0> zyx;
            ShuffleAccessor3<0, 2, 0> xzx;
            ShuffleAccessor3<1, 2, 0> yzx;
            ShuffleAccessor3<2, 2, 0> zzx;
            ShuffleAccessor3<0, 0, 1> xxy;
            ShuffleAccessor3<1, 0, 1> yxy;
            ShuffleAccessor3<2, 0, 1> zxy;
            ShuffleAccessor3<0, 1, 1> xyy;
            ShuffleAccessor3<1, 1, 1> yyy;
            ShuffleAccessor3<2, 1, 1> zyy;
            ShuffleAccessor3<0, 2, 1> xzy;
            ShuffleAccessor3<1, 2, 1> yzy;
            ShuffleAccessor3<2, 2, 1> zzy;
            ShuffleAccessor3<0, 0, 2> xxz;
            ShuffleAccessor3<1, 0, 2> yxz;
            ShuffleAccessor3<2, 0, 2> zxz;
            ShuffleAccessor3<0, 1, 2> xyz;
            ShuffleAccessor3<1, 1, 2> yyz;
            ShuffleAccessor3<2, 1, 2> zyz;
            ShuffleAccessor3<0, 2, 2> xzz;
            ShuffleAccessor3<1, 2, 2> yzz;
            ShuffleAccessor3<2, 2, 2> zzz;
        };

        ScalarType& operator [] (size_t index)
        {
            assert(index < VectorSize);
            return data()[index];
        }

        ScalarType operator [] (size_t index) const
        {
            assert(index < VectorSize);
            return data()[index];
        }

        ScalarType* data()
        {
            return reinterpret_cast<ScalarType *>(this);
        }

        const ScalarType* data() const
        {
            return reinterpret_cast<const ScalarType *>(this);
        }

        explicit Vector() {}

        Vector(float s)
        {
			x = s;
			y = s;
			z = s;
        }

        explicit Vector(float s0, float s1, float s2)
        {
			x = s0;
			y = s1;
			z = s2;
        }

		explicit Vector(const Vector<float, 2>& v, float s)
		{
			x = v.x;
			y = v.y;
			z = s;
		}

		explicit Vector(float s, const Vector<float, 2>& v)
		{
			x = s;
			y = v.x;
			z = v.y;
		}

        Vector(const Vector& v)
        {
			x = v.x;
			y = v.y;
			z = v.z;
        }

        template <int X, int Y, int Z>
        Vector(const ShuffleAccessor3<X, Y, Z>& p)
        {
			const float* v = p.v;
			x = v[X];
			y = v[Y];
			z = v[Z];
        }

        template <int X, int Y, int Z>
        Vector& operator = (const ShuffleAccessor3<X, Y, Z>& p)
        {
			const float* v = p.v;
			x = v[X];
			y = v[Y];
			z = v[Z];
            return *this;
        }

        Vector& operator = (float s)
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

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline const Vector<float, 3>& operator + (const Vector<float, 3>& v)
    {
        return v;
    }

    static inline Vector<float, 3> operator - (const Vector<float, 3>& v)
    {
        return Vector<float, 3>(-v.x, -v.y, -v.z);
    }

    static inline Vector<float, 3>& operator += (Vector<float, 3>& a, const Vector<float, 3>& b)
    {
        a.x += b.x;
        a.y += b.y;
        a.z += b.z;
        return a;
    }

    static inline Vector<float, 3>& operator -= (Vector<float, 3>& a, const Vector<float, 3>& b)
    {
        a.x -= b.x;
        a.y -= b.y;
        a.z -= b.z;
        return a;
    }

    static inline Vector<float, 3>& operator *= (Vector<float, 3>& a, const Vector<float, 3>& b)
    {
        a.x *= b.x;
        a.y *= b.y;
        a.z *= b.z;
        return a;
    }

    static inline Vector<float, 3>& operator *= (Vector<float, 3>& a, float b)
    {
        a.x *= b;
        a.y *= b;
        a.z *= b;
        return a;
    }

    template <typename VT, int I>
    static inline Vector<float, 3>& operator /= (Vector<float, 3>& a, ScalarAccessor<float, VT, I> b)
    {
        float s = b;
        a.x /= s;
        a.y /= s;
        a.z /= s;
        return a;
    }

    static inline Vector<float, 3>& operator /= (Vector<float, 3>& a, const Vector<float, 3>& b)
    {
        a.x /= b.x;
        a.y /= b.y;
        a.z /= b.z;
        return a;
    }

    static inline Vector<float, 3>& operator /= (Vector<float, 3>& a, float b)
    {
        b = 1.0f / b;
        a.x *= b;
        a.y *= b;
        a.z *= b;
        return a;
    }

    static inline Vector<float, 3> operator + (Vector<float, 3> a, Vector<float, 3> b)
    {
        float x = a.x + b.x;
        float y = a.y + b.y;
        float z = a.z + b.z;
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> operator - (Vector<float, 3> a, Vector<float, 3> b)
    {
        float x = a.x - b.x;
        float y = a.y - b.y;
        float z = a.z - b.z;
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> operator * (Vector<float, 3> a, Vector<float, 3> b)
    {
        float x = a.x * b.x;
        float y = a.y * b.y;
        float z = a.z * b.z;
        return Vector<float, 3>(x, y, z);
    }

    template <typename VT, int I>
    static inline Vector<float, 3> operator / (Vector<float, 3> a, ScalarAccessor<float, VT, I> b)
    {
        float s = b;
        float x = a.x / s;
        float y = a.y / s;
        float z = a.z / s;
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> operator / (Vector<float, 3> a, Vector<float, 3> b)
    {
        float x = a.x / b.x;
        float y = a.y / b.y;
        float z = a.z / b.z;
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> operator / (Vector<float, 3> a, float b)
    {
        float s = 1.0f / b;
        float x = a.x * s;
        float y = a.y * s;
        float z = a.z * s;
        return Vector<float, 3>(x, y, z);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<float, 3> abs(Vector<float, 3> a)
    {
        float x = std::abs(a.x);
        float y = std::abs(a.y);
        float z = std::abs(a.z);
        return Vector<float, 3>(x, y, z);
    }

    static inline float square(Vector<float, 3> a)
    {
        return a.x * a.x + a.y * a.y + a.z * a.z;
    }

    static inline float length(Vector<float, 3> a)
    {
        return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    }

    static inline Vector<float, 3> normalize(Vector<float, 3> a)
    {
        float s = 1.0f / length(a);
        return a * s;
    }

    static inline Vector<float, 3> min(Vector<float, 3> a, Vector<float, 3> b)
    {
        float x = std::min(a.x, b.x);
        float y = std::min(a.y, b.y);
        float z = std::min(a.z, b.z);
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> max(Vector<float, 3> a, Vector<float, 3> b)
    {
        float x = std::max(a.x, b.x);
        float y = std::max(a.y, b.y);
        float z = std::max(a.z, b.z);
        return Vector<float, 3>(x, y, z);
    }

    static inline float dot(Vector<float, 3> a, Vector<float, 3> b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static inline Vector<float, 3> cross(Vector<float, 3> a, Vector<float, 3> b)
    {
        float x = a.y * b.z - a.z * b.y;
        float y = a.z * b.x - a.x * b.z;
        float z = a.x * b.y - a.y * b.x;
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> clamp(const Vector<float, 3>& a, const Vector<float, 3>& a_min, const Vector<float, 3>& a_max)
    {
        const float x = std::max(a_min.x, std::min(a_max.x, a.x));
        const float y = std::max(a_min.y, std::min(a_max.y, a.y));
        const float z = std::max(a_min.z, std::min(a_max.z, a.z));
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> lerp(const Vector<float, 3>& a, const Vector<float, 3>& b, float factor)
    {
        const float x = a.x + (b.x - a.x) * factor;
        const float y = a.y + (b.y - a.y) * factor;
        const float z = a.z + (b.z - a.z) * factor;
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> lerp(const Vector<float, 3>& a, const Vector<float, 3>& b, const Vector<float, 3>& factor)
    {
        const float x = a.x + (b.x - a.x) * factor.x;
        const float y = a.y + (b.y - a.y) * factor.y;
        const float z = a.z + (b.z - a.z) * factor.z;
        return Vector<float, 3>(x, y, z);
    }

    static inline Vector<float, 3> hmin(const Vector<float, 3>& v)
    {
        const float s = std::min(std::min(v.x, v.y), v.z);
        return Vector<float, 3>(s);
    }

    static inline Vector<float, 3> hmax(const Vector<float, 3>& v)
    {
        const float s = std::max(std::max(v.x, v.y), v.z);
        return Vector<float, 3>(s);
    }

} // namespace mango

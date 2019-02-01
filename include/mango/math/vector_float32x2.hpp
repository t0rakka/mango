/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // Vector<float, 2>
    // ------------------------------------------------------------------

    template <>
    struct Vector<float, 2>
    {
        using VectorType = void;
        using ScalarType = float;
        enum { VectorSize = 2 };

        template <int X, int Y>
        struct ShuffleAccessor2
        {
			float v[2];

			operator Vector<float, 2> () const
			{
				return Vector<float, 2>(v[X], v[Y]);
			}
        };

        union
        {
            struct { float x, y; };

			ShuffleAccessor2<0, 0> xx;
			ShuffleAccessor2<1, 0> yx;
			ShuffleAccessor2<0, 1> xy;
			ShuffleAccessor2<1, 1> yy;

            DeAggregate<ScalarType> component[VectorSize];
        };

        ScalarType& operator [] (size_t index)
        {
            assert(index < VectorSize);
            return component[index].data;
        }

        ScalarType operator [] (size_t index) const
        {
            assert(index < VectorSize);
            return component[index].data;
        }

        const ScalarType* data() const
        {
            return reinterpret_cast<const ScalarType *>(component);
        }

        explicit Vector() {}
        ~Vector() {}

        Vector(float s)
        {
			x = s;
			y = s;
        }

        explicit Vector(float s0, float s1)
        {
			x = s0;
			y = s1;
        }

        Vector(const Vector& v)
        {
			x = v.x;
			y = v.y;
        }

        template <int X, int Y>
        Vector(const ShuffleAccessor2<X, Y>& p)
        {
			const float* v = p.v;
			x = v[X];
			y = v[Y];
        }

        template <int X, int Y>
        Vector& operator = (const ShuffleAccessor2<X, Y>& p)
        {
			const float* v = p.v;
			x = v[X];
			y = v[Y];
            return *this;
        }

        Vector& operator = (float s)
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

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline const float32x2& operator + (const float32x2& v)
    {
        return v;
    }

    static inline float32x2 operator - (const float32x2& v)
    {
        return float32x2(-v.x, -v.y);
    }

    static inline float32x2& operator += (float32x2& a, const float32x2& b)
    {
        a.x += b.x;
        a.y += b.y;
        return a;
    }

    static inline float32x2& operator -= (float32x2& a, const float32x2& b)
    {
        a.x -= b.x;
        a.y -= b.y;
        return a;
    }

    static inline float32x2& operator *= (float32x2& a, const float32x2& b)
    {
        a.x *= b.x;
        a.y *= b.y;
        return a;
    }

    static inline float32x2& operator *= (float32x2& a, float b)
    {
        a.x *= b;
        a.y *= b;
        return a;
    }

    static inline float32x2& operator /= (float32x2& a, const float32x2& b)
    {
        a.x /= b.x;
        a.y /= b.y;
        return a;
    }

    static inline float32x2& operator /= (float32x2& a, float b)
    {
        b = 1.0f / b;
        a.x *= b;
        a.y *= b;
        return a;
    }

    static inline float32x2 operator + (float32x2 a, float32x2 b)
    {
        float x = a.x + b.x;
        float y = a.y + b.y;
        return float32x2(x, y);
    }

    static inline float32x2 operator - (float32x2 a, float32x2 b)
    {
        float x = a.x - b.x;
        float y = a.y - b.y;
        return float32x2(x, y);
    }

    static inline float32x2 operator * (float32x2 a, float32x2 b)
    {
        float x = a.x * b.x;
        float y = a.y * b.y;
        return float32x2(x, y);
    }

    static inline float32x2 operator / (float32x2 a, float32x2 b)
    {
        float x = a.x / b.x;
        float y = a.y / b.y;
        return float32x2(x, y);
    }

    static inline float32x2 operator / (float32x2 a, float b)
    {
        float s = 1.0f / b;
        float x = a.x * s;
        float y = a.y * s;
        return float32x2(x, y);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline float32x2 abs(float32x2 a)
    {
        float x = std::abs(a.x);
        float y = std::abs(a.y);
        return float32x2(x, y);
    }

    static inline float square(float32x2 a)
    {
        return a.x * a.x + a.y * a.y;
    }

    static inline float length(float32x2 a)
    {
        return std::sqrt(a.x * a.x + a.y * a.y);
    }

    static inline float32x2 normalize(float32x2 a)
    {
        float s = 1.0f / length(a);
        return a * s;
    }

    static inline float32x2 min(float32x2 a, float32x2 b)
    {
        float x = std::min(a.x, b.x);
        float y = std::min(a.y, b.y);
        return float32x2(x, y);
    }

    static inline float32x2 max(float32x2 a, float32x2 b)
    {
        float x = std::max(a.x, b.x);
        float y = std::max(a.y, b.y);
        return float32x2(x, y);
    }

    static inline float dot(float32x2 a, float32x2 b)
    {
        return a.x * b.x + a.y * b.y;
    }

    static inline float32x2 clamp(const float32x2& a, const float32x2& amin, const float32x2& amax)
    {
        const float x = std::max(amin.x, std::min(amax.x, a.x));
        const float y = std::max(amin.y, std::min(amax.y, a.y));
        return float32x2(x, y);
    }

    static inline float32x2 lerp(const float32x2& a, const float32x2& b, float factor)
    {
        const float x = a.x + (b.x - a.x) * factor;
        const float y = a.y + (b.y - a.y) * factor;
        return float32x2(x, y);
    }

    static inline float32x2 lerp(const float32x2& a, const float32x2& b, const float32x2& factor)
    {
        const float x = a.x + (b.x - a.x) * factor.x;
        const float y = a.y + (b.y - a.y) * factor.y;
        return float32x2(x, y);
    }

    static inline float32x2 hmin(const float32x2& v)
    {
        const float s = std::min(v.x, v.y);
        return float32x2(s);
    }

    static inline float32x2 hmax(const float32x2& v)
    {
        const float s = std::max(v.x, v.y);
        return float32x2(s);
    }

} // namespace mango

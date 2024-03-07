/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/math/vector.hpp>
#include <mango/math/vector_float64x2.hpp>

namespace mango::math
{

    // ------------------------------------------------------------------
    // Vector<double, 3>
    // ------------------------------------------------------------------

    template <>
    struct Vector<double, 3>
    {
        using VectorType = void;
        using ScalarType = double;
        enum { VectorSize = 3 };

        template <int X, int Y>
        struct ShuffleAccessor2
        {
            double v[3];

            operator Vector<double, 2> () const
            {
                return Vector<double, 2>(v[X], v[Y]);
            }
        };

        template <int X, int Y, int Z>
        struct ShuffleAccessor3
        {
            double v[3];

            operator Vector<double, 3> () const
            {
                return Vector<double, 3>(v[X], v[Y], v[Z]);
            }
        };

        union
        {
            struct { double x, y, z; };

            // generate 2 component accessors
#define VECTOR3_SHUFFLE_ACCESSOR2(A, B, NAME) \
            ShuffleAccessor2<A, B> NAME
#include <mango/math/accessor.hpp>
#undef VECTOR3_SHUFFLE_ACCESSOR2

            // generate 3 component accessors
#define VECTOR3_SHUFFLE_ACCESSOR3(A, B, C, NAME) \
            ShuffleAccessor3<A, B, C> NAME
#include <mango/math/accessor.hpp>
#undef VECTOR3_SHUFFLE_ACCESSOR3
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

        Vector(double s)
        {
            x = s;
            y = s;
            z = s;
        }

        explicit Vector(double s0, double s1, double s2)
        {
            x = s0;
            y = s1;
            z = s2;
        }

        explicit Vector(const Vector<double, 2>& v, double s)
        {
            x = v.x;
            y = v.y;
            z = s;
        }

        explicit Vector(double s, const Vector<double, 2>& v)
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

#if 0
        template <int X, int Y, int Z>
        Vector(const ShuffleAccessor3<X, Y, Z>& p)
        {
            const double* v = p.v;
            x = v[X];
            y = v[Y];
            z = v[Z];
        }

        template <int X, int Y, int Z>
        Vector& operator = (const ShuffleAccessor3<X, Y, Z>& p)
        {
            const double* v = p.v;
            x = v[X];
            y = v[Y];
            z = v[Z];
            return *this;
        }
#endif

        Vector& operator = (double s)
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

        static Vector uload(const void* source)
        {
            Vector v;
            std::memcpy(v.data(), source, 12);
            return v;
        }

        static void ustore(void* dest, Vector v)
        {
            std::memcpy(dest, v.data(), 12);
        }

        static Vector ascend()
        {
            return Vector(0.0f, 1.0f, 2.0f);
        }
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline const Vector<double, 3>& operator + (const Vector<double, 3>& a)
    {
        return a;
    }

    static inline Vector<double, 3> operator - (const Vector<double, 3>& a)
    {
        return Vector<double, 3>(-a.x, -a.y, -a.z);
    }

    static inline Vector<double, 3>& operator += (Vector<double, 3>& a, const Vector<double, 3>& b)
    {
        a.x += b.x;
        a.y += b.y;
        a.z += b.z;
        return a;
    }

    static inline Vector<double, 3>& operator += (Vector<double, 3>& a, double b)
    {
        a.x += b;
        a.y += b;
        a.z += b;
        return a;
    }

    static inline Vector<double, 3>& operator -= (Vector<double, 3>& a, const Vector<double, 3>& b)
    {
        a.x -= b.x;
        a.y -= b.y;
        a.z -= b.z;
        return a;
    }

    static inline Vector<double, 3>& operator -= (Vector<double, 3>& a, double b)
    {
        a.x -= b;
        a.y -= b;
        a.z -= b;
        return a;
    }

    static inline Vector<double, 3>& operator *= (Vector<double, 3>& a, const Vector<double, 3>& b)
    {
        a.x *= b.x;
        a.y *= b.y;
        a.z *= b.z;
        return a;
    }

    static inline Vector<double, 3>& operator *= (Vector<double, 3>& a, double b)
    {
        a.x *= b;
        a.y *= b;
        a.z *= b;
        return a;
    }

    template <typename VT, int I>
    static inline Vector<double, 3>& operator /= (Vector<double, 3>& a, ScalarAccessor<double, VT, I> b)
    {
        double s = b;
        a.x /= s;
        a.y /= s;
        a.z /= s;
        return a;
    }

    static inline Vector<double, 3>& operator /= (Vector<double, 3>& a, const Vector<double, 3>& b)
    {
        a.x /= b.x;
        a.y /= b.y;
        a.z /= b.z;
        return a;
    }

    static inline Vector<double, 3>& operator /= (Vector<double, 3>& a, double b)
    {
        b = 1.0f / b;
        a.x *= b;
        a.y *= b;
        a.z *= b;
        return a;
    }

    static inline Vector<double, 3> operator + (Vector<double, 3> a, Vector<double, 3> b)
    {
        double x = a.x + b.x;
        double y = a.y + b.y;
        double z = a.z + b.z;
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> operator + (Vector<double, 3> a, double b)
    {
        double x = a.x + b;
        double y = a.y + b;
        double z = a.z + b;
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> operator + (double a, Vector<double, 3> b)
    {
        double x = a + b.x;
        double y = a + b.y;
        double z = a + b.z;
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> operator - (Vector<double, 3> a, Vector<double, 3> b)
    {
        double x = a.x - b.x;
        double y = a.y - b.y;
        double z = a.z - b.z;
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> operator - (Vector<double, 3> a, double b)
    {
        double x = a.x - b;
        double y = a.y - b;
        double z = a.z - b;
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> operator - (double a, Vector<double, 3> b)
    {
        double x = a - b.x;
        double y = a - b.y;
        double z = a - b.z;
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> operator * (Vector<double, 3> a, Vector<double, 3> b)
    {
        double x = a.x * b.x;
        double y = a.y * b.y;
        double z = a.z * b.z;
        return Vector<double, 3>(x, y, z);
    }

    template <typename VT, int I>
    static inline Vector<double, 3> operator / (Vector<double, 3> a, ScalarAccessor<double, VT, I> b)
    {
        double s = b;
        double x = a.x / s;
        double y = a.y / s;
        double z = a.z / s;
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> operator / (Vector<double, 3> a, Vector<double, 3> b)
    {
        double x = a.x / b.x;
        double y = a.y / b.y;
        double z = a.z / b.z;
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> operator / (Vector<double, 3> a, double b)
    {
        double s = 1.0f / b;
        double x = a.x * s;
        double y = a.y * s;
        double z = a.z * s;
        return Vector<double, 3>(x, y, z);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<double, 3> abs(Vector<double, 3> a)
    {
        double x = std::abs(a.x);
        double y = std::abs(a.y);
        double z = std::abs(a.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline double square(Vector<double, 3> a)
    {
        return a.x * a.x + a.y * a.y + a.z * a.z;
    }

    static inline double length(Vector<double, 3> a)
    {
        return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    }

    static inline Vector<double, 3> normalize(Vector<double, 3> a)
    {
        double s = 1.0f / length(a);
        return a * s;
    }

    static inline Vector<double, 3> min(Vector<double, 3> a, Vector<double, 3> b)
    {
        double x = std::min(a.x, b.x);
        double y = std::min(a.y, b.y);
        double z = std::min(a.z, b.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> max(Vector<double, 3> a, Vector<double, 3> b)
    {
        double x = std::max(a.x, b.x);
        double y = std::max(a.y, b.y);
        double z = std::max(a.z, b.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline double dot(Vector<double, 3> a, Vector<double, 3> b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static inline Vector<double, 3> cross(Vector<double, 3> a, Vector<double, 3> b)
    {
        double x = a.y * b.z - a.z * b.y;
        double y = a.z * b.x - a.x * b.z;
        double z = a.x * b.y - a.y * b.x;
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> clamp(const Vector<double, 3>& a, const Vector<double, 3>& a_min, const Vector<double, 3>& a_max)
    {
        const double x = std::max(a_min.x, std::min(a_max.x, a.x));
        const double y = std::max(a_min.y, std::min(a_max.y, a.y));
        const double z = std::max(a_min.z, std::min(a_max.z, a.z));
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> lerp(const Vector<double, 3>& a, const Vector<double, 3>& b, double factor)
    {
        const double x = a.x + (b.x - a.x) * factor;
        const double y = a.y + (b.y - a.y) * factor;
        const double z = a.z + (b.z - a.z) * factor;
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> lerp(const Vector<double, 3>& a, const Vector<double, 3>& b, const Vector<double, 3>& factor)
    {
        const double x = a.x + (b.x - a.x) * factor.x;
        const double y = a.y + (b.y - a.y) * factor.y;
        const double z = a.z + (b.z - a.z) * factor.z;
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> hmin(const Vector<double, 3>& v)
    {
        const double s = std::min(std::min(v.x, v.y), v.z);
        return Vector<double, 3>(s);
    }

    static inline Vector<double, 3> hmax(const Vector<double, 3>& v)
    {
        const double s = std::max(std::max(v.x, v.y), v.z);
        return Vector<double, 3>(s);
    }

    // ------------------------------------------------------------------
    // trigonometric functions
    // ------------------------------------------------------------------

    static inline Vector<double, 3> sin(Vector<double, 3> v)
    {
        double x = std::sin(v.x);
        double y = std::sin(v.y);
        double z = std::sin(v.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> cos(Vector<double, 3> v)
    {
        double x = std::cos(v.x);
        double y = std::cos(v.y);
        double z = std::cos(v.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> tan(Vector<double, 3> v)
    {
        double x = std::tan(v.x);
        double y = std::tan(v.y);
        double z = std::tan(v.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> exp(Vector<double, 3> v)
    {
        double x = std::exp(v.x);
        double y = std::exp(v.y);
        double z = std::exp(v.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> exp2(Vector<double, 3> v)
    {
        double x = std::exp2(v.x);
        double y = std::exp2(v.y);
        double z = std::exp2(v.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> log(Vector<double, 3> v)
    {
        double x = std::log(v.x);
        double y = std::log(v.y);
        double z = std::log(v.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> log2(Vector<double, 3> v)
    {
        double x = std::log2(v.x);
        double y = std::log2(v.y);
        double z = std::log2(v.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> asin(Vector<double, 3> v)
    {
        double x = std::asin(v.x);
        double y = std::asin(v.y);
        double z = std::asin(v.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> acos(Vector<double, 3> v)
    {
        double x = std::acos(v.x);
        double y = std::acos(v.y);
        double z = std::acos(v.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> atan(Vector<double, 3> v)
    {
        double x = std::atan(v.x);
        double y = std::atan(v.y);
        double z = std::atan(v.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> atan2(Vector<double, 3> a, Vector<double, 3> b)
    {
        double x = std::atan2(a.x, b.x);
        double y = std::atan2(a.y, b.y);
        double z = std::atan2(a.z, b.z);
        return Vector<double, 3>(x, y, z);
    }

    static inline Vector<double, 3> pow(Vector<double, 3> a, Vector<double, 3> b)
    {
        double x = std::pow(a.x, b.x);
        double y = std::pow(a.y, b.y);
        double z = std::pow(a.z, b.z);
        return Vector<double, 3>(x, y, z);
    }

} // namespace mango::math

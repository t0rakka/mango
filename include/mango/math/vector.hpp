/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cassert>
#include <algorithm>
#include "../simd/simd.hpp"

/*
    Abstract:

    The Short Vector Math code presented in this header was originally a straightforward
    template N-dimensional vector implementation. Eventually, over time this code merged
    with a low level SIMD abstraction library which is invoked for compatible vector types.

    The "correct" way to utilize SIMD is to work with as wide vectors as possible and not
    wrap vec3, vec4, etc. This code does wrap these types anyway but gives a convenient
    building block to go extra-wide. Because the front-end is all templates, we can write this:

    using float3 = Vector<float32x8, 3>;

    Now the float3 is a three-component vector of 256 bit SIMD vectors.

    float3 a, b;
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

namespace mango
{

    // ------------------------------------------------------------------
    // scalar variants
    // ------------------------------------------------------------------

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

    // ------------------------------------------------------------------
    // Vector
    // ------------------------------------------------------------------

    template <typename ScalarType, int VectorSize>
    struct Vector
    {
        using VectorType = void;

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
    };

    template <typename ScalarType>
    struct Vector<ScalarType, 2>
    {
        using VectorType = void;
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
    };

    template <typename ScalarType>
    struct Vector<ScalarType, 3>
    {
        using VectorType = void;
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
    };

    template <typename ScalarType>
    struct Vector<ScalarType, 4>
    {
        using VectorType = void;
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
    };

    // ------------------------------------------------------------------
    // Vector operators
    // ------------------------------------------------------------------

    template <typename ScalarType, int VectorSize>
    static inline const Vector<ScalarType, VectorSize>& operator + (const Vector<ScalarType, VectorSize>& v)
    {
        return v;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize> operator - (const Vector<ScalarType, VectorSize>& v)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = -v[i];
        }
        return temp;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize>& operator += (Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        for (int i = 0; i < VectorSize; ++i)
        {
            a[i] += b[i];
        }
        return a;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize>& operator -= (Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        for (int i = 0; i < VectorSize; ++i)
        {
            a[i] -= b[i];
        }
        return a;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize>& operator *= (Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        for (int i = 0; i < VectorSize; ++i)
        {
            a[i] *= b[i];
        }
        return a;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize>& operator *= (Vector<ScalarType, VectorSize>& a, ScalarType b)
    {
        for (int i = 0; i < VectorSize; ++i)
        {
            a[i] *= b;
        }
        return a;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize>& operator /= (Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        for (int i = 0; i < VectorSize; ++i)
        {
            a[i] /= b[i];
        }
        return a;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize>& operator /= (Vector<ScalarType, VectorSize>& a, ScalarType b)
    {
        for (int i = 0; i < VectorSize; ++i)
        {
            a[i] /= b;
        }
        return a;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize> operator + (const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = a[i] + b[i];
        }
        return temp;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize> operator - (const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = a[i] - b[i];
        }
        return temp;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize> operator * (const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = a[i] * b[i];
        }
        return temp;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize> operator * (const Vector<ScalarType, VectorSize>& a, ScalarType b)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = a[i] * b;
        }
        return temp;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize> operator * (ScalarType a, const Vector<ScalarType, VectorSize>& b)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = a * b[i];
        }
        return temp;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize> operator / (const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = a[i] / b[i];
        }
        return temp;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize> operator / (const Vector<ScalarType, VectorSize>& a, ScalarType b)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = a[i] / b;
        }
        return temp;
    }

    // ------------------------------------------------------------------
    // Vector functions
    // ------------------------------------------------------------------

    template <typename ScalarType, int VectorSize>
    static inline const Vector<ScalarType, VectorSize> abs(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::abs(a[i]);
        }
        return temp;
    }

    template <typename ScalarType, int VectorSize>
    static inline const Vector<ScalarType, VectorSize> min(const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::min(a[i], b[i]);
        }
        return temp;
    }

    template <typename ScalarType, int VectorSize>
    static inline const Vector<ScalarType, VectorSize> max(const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::max(a[i], b[i]);
        }
        return temp;
    }

    template <typename ScalarType, int VectorSize>
    static inline ScalarType dot(const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        ScalarType s(0);
        for (int i = 0; i < VectorSize; ++i)
        {
            s += a[i] * b[i];
        }
        return s;
    }

    template <typename ScalarType, int VectorSize>
    static inline ScalarType square(const Vector<ScalarType, VectorSize>& a)
    {
        ScalarType s(0);
        for (int i = 0; i < VectorSize; ++i)
        {
            s += a[i] * a[i];
        }
        return s;
    }

    template <typename ScalarType, int VectorSize>
    static inline ScalarType length(const Vector<ScalarType, VectorSize>& a)
    {
        ScalarType s(0);
        for (int i = 0; i < VectorSize; ++i)
        {
            s += a[i] * a[i];
        }
        return static_cast<ScalarType>(std::sqrt(s));
    }

    template <typename ScalarType, int VectorSize>
    static inline const Vector<ScalarType, VectorSize> normalize(const Vector<ScalarType, VectorSize>& a)
    {
        return a * ScalarType(1.0 / length(a));
    }

    template <typename ScalarType, int VectorSize>
    static inline const Vector<ScalarType, VectorSize> clamp(const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& amin, const Vector<ScalarType, VectorSize>& amax)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::min(amax[i], std::max(amin[i], a[i]));
        }
        return temp;
    }

    template <typename ScalarType, int VectorSize>
    static inline Vector<ScalarType, VectorSize> madd(const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b, const Vector<ScalarType, VectorSize>& c)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = a[i] + b[i] * c[i];
        }
        return temp;
    }

    template <typename ScalarType, int VectorSize>
    static inline const Vector<ScalarType, VectorSize> lerp(const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b, ScalarType factor)
    {
        return a + (b - a) * factor;
    }

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> sign(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = sign(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> radians(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = radians(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> degrees(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = degrees(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> sin(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::sin(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> cos(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::cos(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> tan(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::tan(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> asin(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::asin(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> acos(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::acos(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> atan(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::atan(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> exp(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::exp(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> log(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::log(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> exp2(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::exp2(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> log2(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::log2(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> sqrt(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::sqrt(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> rsqrt(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = 1.0f / std::sqrt(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> round(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::round(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> floor(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::floor(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> ceil(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::ceil(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> fract(const Vector<ScalarType, VectorSize>& a)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = a[i] - std::floor(a[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> pow(const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::pow(a[i], b[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> mod(const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = a[i] - b[i] * std::floor(a[i] / b[i]);
        }
        return temp;
	}

	template <typename ScalarType, int VectorSize>
	static inline const Vector<ScalarType, VectorSize> atan2(const Vector<ScalarType, VectorSize>& a, const Vector<ScalarType, VectorSize>& b)
    {
        Vector<ScalarType, VectorSize> temp;
        for (int i = 0; i < VectorSize; ++i)
        {
            temp[i] = std::atan2(a[i], b[i]);
        }
        return temp;
	}

    // ------------------------------------------------------------------
    // Vector2 functions
    // ------------------------------------------------------------------

    template <typename ScalarType>
    static inline ScalarType length(const Vector<ScalarType, 2>& v)
    {
        ScalarType s = square(v);
        return static_cast<ScalarType>(std::sqrt(s));
    }

    template <typename ScalarType>
    static inline ScalarType distance(const Vector<ScalarType, 2>& a, const Vector<ScalarType, 2>& b)
    {
        return length(a - b);
    }

    template <typename ScalarType>
    static inline Vector<ScalarType, 2> normalize(const Vector<ScalarType, 2>& v)
    {
        ScalarType s = length(v);
        if (s) s = ScalarType(1.0) / s;
        return v * s;
    }

    template <typename ScalarType>
    static inline Vector<ScalarType, 2> project(const Vector<ScalarType, 2>& v, const Vector<ScalarType, 2>& normal)
    {
        return v - normal * (dot(v, normal) / dot(normal, normal));
    }

    template <typename ScalarType>
    static inline Vector<ScalarType, 2> reflect(const Vector<ScalarType, 2>& v, const Vector<ScalarType, 2>& normal)
    {
        return v - normal * (ScalarType(2.0) * dot(v, normal));
    }

    template <typename ScalarType>
    static inline Vector<ScalarType, 2> refract(const Vector<ScalarType, 2>& v, const Vector<ScalarType, 2>& normal, ScalarType factor)
    {
        ScalarType vdotn = dot(v, normal);
        ScalarType p = ScalarType(1.0) - factor * factor * (ScalarType(1.0) - vdotn * vdotn);
        if (p < 0)
        {
            p = 0;
            factor = 0;
        }
        else
        {
            p = ScalarType(std::sqrt(p));
        }
        return v * factor - normal * (p + factor * vdotn);
    }

    template <typename ScalarType>
    static inline Vector<ScalarType, 2> hmin(const Vector<ScalarType, 2>& v)
    {
        const ScalarType s = std::min(v.x, v.y);
        return Vector<ScalarType, 2>(s);
    }

    template <typename ScalarType>
    static inline Vector<ScalarType, 2> hmax(const Vector<ScalarType, 2>& v)
    {
        const ScalarType s = std::max(v.x, v.y);
        return Vector<ScalarType, 2>(s);
    }

    // ------------------------------------------------------------------
    // Vector3 functions
    // ------------------------------------------------------------------

    template <typename ScalarType>
    static inline Vector<ScalarType, 3> cross(const Vector<ScalarType, 3>& a, const Vector<ScalarType, 3>& b)
    {
        ScalarType x = a.y * b.z - a.z * b.y;
        ScalarType y = a.z * b.x - a.x * b.z;
        ScalarType z = a.x * b.y - a.y * b.x;
        return Vector<ScalarType, 3>(x, y, z);
    }

    template <typename ScalarType>
    static inline ScalarType length(const Vector<ScalarType, 3>& v)
    {
        ScalarType s = square(v);
        return static_cast<ScalarType>(std::sqrt(s));
    }

    template <typename ScalarType>
    static inline ScalarType distance(const Vector<ScalarType, 3>& a, const Vector<ScalarType, 3>& b)
    {
        return length(a - b);
    }

    template <typename ScalarType>
    static inline Vector<ScalarType, 3> normalize(const Vector<ScalarType, 3>& v)
    {
        ScalarType s = length(v);
        if (s) s = ScalarType(1.0) / s;
        return v * s;
    }

    template <typename ScalarType>
    static inline Vector<ScalarType, 3> project(const Vector<ScalarType, 3>& v, const Vector<ScalarType, 3>& normal)
    {
        return v - normal * (dot(v, normal) / dot(normal, normal));
    }

    template <typename ScalarType>
    static inline Vector<ScalarType, 3> reflect(const Vector<ScalarType, 3>& v, const Vector<ScalarType, 3>& normal)
    {
        return v - normal * (ScalarType(2.0) * dot(v, normal));
    }

    template <typename ScalarType>
    static inline Vector<ScalarType, 3> refract(const Vector<ScalarType, 3>& v, const Vector<ScalarType, 3>& normal, ScalarType factor)
    {
        ScalarType vdotn = dot(v, normal);
        ScalarType p = ScalarType(1.0) - factor * factor * (ScalarType(1.0) - vdotn * vdotn);
        if (p < 0)
        {
            p = 0;
            factor = 0;
        }
        else
        {
            p = ScalarType(std::sqrt(p));
        }
        return v * factor - normal * (p + factor * vdotn);
    }

    template <typename ScalarType>
    static inline Vector<ScalarType, 3> hmin(const Vector<ScalarType, 3>& v)
    {
        const ScalarType s = std::min(std::min(v.x, v.y), v.z);
        return Vector<ScalarType, 3>(s);
    }

    template <typename ScalarType>
    static inline Vector<ScalarType, 3> hmax(const Vector<ScalarType, 3>& v)
    {
        const ScalarType s = std::max(std::max(v.x, v.y), v.z);
        return Vector<ScalarType, 3>(s);
    }

    // ------------------------------------------------------------------
    // reinterpret / convert
    // ------------------------------------------------------------------

    // The reinterpret and conversion casts forward the work to the simd abstraction.
    // This is enforced by requiring "VectorType" declaration in the Vector specialization.

    template <typename D, typename S>
    static inline  D reinterpret(S s)
    {
        typename S::VectorType temp = s;
        return simd::reinterpret<typename D::VectorType>(temp);
    }

    template <typename D, typename S>
    static inline  D convert(S s)
    {
        typename S::VectorType temp = s;
        return simd::convert<typename D::VectorType>(temp);
    }

    template <typename D, typename S>
    static inline  D truncate(S s)
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

        template <int VectorSize>
        operator Vector<ScalarType, VectorSize> () const
        {
            ScalarType s = simd::get_component<Index>(m);
            return Vector<ScalarType, VectorSize>(s);
        }

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

    // operators

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    ScalarType operator + (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        return ScalarType(a) + ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator + (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b)
    {
        return ScalarType(a) + b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator + (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b)
    {
        return a + ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    ScalarType operator - (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        return ScalarType(a) - ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator - (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b)
    {
        return ScalarType(a) - b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator - (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b)
    {
        return a - ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    ScalarType operator * (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        return ScalarType(a) * ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator * (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b)
    {
        return ScalarType(a) * b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator * (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b)
    {
        return a * ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index0, int Index1>
    ScalarType operator / (const ScalarAccessor<ScalarType, VectorType, Index0>& a,
                           const ScalarAccessor<ScalarType, VectorType, Index1>& b)
    {
        return ScalarType(a) / ScalarType(b);
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator / (const ScalarAccessor<ScalarType, VectorType, Index>& a, ScalarType b)
    {
        return ScalarType(a) / b;
    }

    template <typename ScalarType, typename VectorType, int Index>
    ScalarType operator / (ScalarType a, const ScalarAccessor<ScalarType, VectorType, Index>& b)
    {
        return a / ScalarType(b);
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
            constexpr uint32 mask = (D << 6) | (C << 4) | (B << 2) | A;
            constexpr uint32 s0 = (mask >> (X * 2)) & 3;
            constexpr uint32 s1 = (mask >> (Y * 2)) & 3;
            constexpr uint32 s2 = (mask >> (Z * 2)) & 3;
            constexpr uint32 s3 = (mask >> (W * 2)) & 3;
            m = simd::shuffle<s0, s1, s2, s3>(v.m);
            return *this;
        }
#endif
    };

    // ------------------------------------------------------------------
    // DeAggregate
    // ------------------------------------------------------------------

    // HACK / workaround for non-stable GCC ABI for aggregate types with
    // stricter alignment than the architecture default.

    template <typename T>
    struct DeAggregate
    {
        T data;

        DeAggregate() {} // <-- this constructor is what makes the magic happen
        ~DeAggregate() {}

        DeAggregate& operator = (T t) { data = t; return *this; }
        operator T () { return data; }
    };

    // ------------------------------------------------------------------
    // named vector types
    // ------------------------------------------------------------------

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

    // float vectors
    using float16x4  = Vector<float16, 4>;
    using float32x2  = Vector<float, 2>;
    using float32x3  = Vector<float, 3>;
    using float32x4  = Vector<float, 4>;
    using float32x8  = Vector<float, 8>;
    using float32x16 = Vector<float, 16>;
    using float64x2  = Vector<double, 2>;
    using float64x3  = Vector<double, 3>;
    using float64x4  = Vector<double, 4>;
    using float64x8  = Vector<double, 8>;

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

    // OpenCL vectors
    using int2    = Vector<s32, 2>;
    using int3    = Vector<s32, 3>;
    using int4    = Vector<s32, 4>;
    using uint2   = Vector<u32, 2>;
    using uint3   = Vector<u32, 3>;
    using uint4   = Vector<u32, 4>;
    using half4   = Vector<float16, 4>;
    using float2  = Vector<float, 2>;
    using float3  = Vector<float, 3>;
    using float4  = Vector<float, 4>;
    using float8  = Vector<float, 8>;
    using double2 = Vector<double, 2>;
    using double3 = Vector<double, 3>;
    using double4 = Vector<double, 4>;

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

} // namespace mango

#include "vector128_uint8x16.hpp"
#include "vector128_uint16x8.hpp"
#include "vector128_uint32x4.hpp"
#include "vector128_uint64x2.hpp"
#include "vector128_int8x16.hpp"
#include "vector128_int16x8.hpp"
#include "vector128_int32x4.hpp"
#include "vector128_int64x2.hpp"

#include "vector256_uint8x32.hpp"
#include "vector256_uint16x16.hpp"
#include "vector256_uint32x8.hpp"
#include "vector256_uint64x4.hpp"
#include "vector256_int8x32.hpp"
#include "vector256_int16x16.hpp"
#include "vector256_int32x8.hpp"
#include "vector256_int64x4.hpp"

#include "vector512_uint8x64.hpp"
#include "vector512_uint16x32.hpp"
#include "vector512_uint32x16.hpp"
#include "vector512_uint64x8.hpp"
#include "vector512_int8x64.hpp"
#include "vector512_int16x32.hpp"
#include "vector512_int32x16.hpp"
#include "vector512_int64x8.hpp"

#include "vector_float16x4.hpp"
#include "vector_float32x2.hpp"
#include "vector_float32x3.hpp"
#include "vector_float32x4.hpp"
#include "vector_float32x8.hpp"
#include "vector_float32x16.hpp"
#include "vector_float64x2.hpp"
#include "vector_float64x4.hpp"
#include "vector_float64x8.hpp"
#include "vector_gather.hpp"

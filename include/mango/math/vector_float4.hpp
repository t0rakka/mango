/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"
#include "vector_float2.hpp"
#include "vector_float3.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // Vector<float, 4>
    // ------------------------------------------------------------------

    template <>
    struct Vector<float, 4> : VectorBase<float, 4>
    {
        template <int Index>
        struct ScalarAccessor
        {
            simd4f m;

            operator float () const
            {
                return simd4f_get_component<Index>(m);
            }

            ScalarAccessor& operator = (float s)
            {
                m = simd4f_set_component<Index>(m, s);
                return *this;
            }

            ScalarAccessor& operator += (float s)
            {
                *this = float(*this) + s;
                return *this;
            }

            ScalarAccessor& operator -= (float s)
            {
                *this = float(*this) - s;
                return *this;
            }

            ScalarAccessor& operator *= (float s)
            {
                *this = float(*this) * s;
                return *this;
            }

            ScalarAccessor& operator /= (float s)
            {
                *this = float(*this) / s;
                return *this;
            }
        };

        template <int X, int Y>
        struct Permute2
        {
            simd4f m;

            operator Vector<float, 2> () const
            {
                const float x = simd4f_get_component<X>(m);
                const float y = simd4f_get_component<Y>(m);
                return Vector<float, 2>(x, y);
            }
        };

        template <int X, int Y, int Z>
        struct Permute3
        {
            simd4f m;

            operator Vector<float, 3> () const
            {
                const float x = simd4f_get_component<X>(m);
                const float y = simd4f_get_component<Y>(m);
                const float z = simd4f_get_component<Z>(m);
                return Vector<float, 3>(x, y, z);
            }
        };

        template <int X, int Y, int Z, int W>
        struct Permute4
        {
            simd4f m;

            operator simd4f () const
            {
                return simd4f_shuffle<X, Y, Z, W>(m);
            }
        };

        union
        {
            simd4f m;

            ScalarAccessor<0> x;
            ScalarAccessor<1> y;
            ScalarAccessor<2> z;
            ScalarAccessor<3> w;

            Permute2<0, 0> xx;
            Permute2<1, 0> yx;
            Permute2<2, 0> zx;
            Permute2<3, 0> wx;
            Permute2<0, 1> xy;
            Permute2<1, 1> yy;
            Permute2<2, 1> zy;
            Permute2<3, 1> wy;
            Permute2<0, 2> xz;
            Permute2<1, 2> yz;
            Permute2<2, 2> zz;
            Permute2<3, 2> wz;
            Permute2<0, 3> xw;
            Permute2<1, 3> yw;
            Permute2<2, 3> zw;
            Permute2<3, 3> ww;

            Permute3<0, 0, 0> xxx;
            Permute3<1, 0, 0> yxx;
            Permute3<2, 0, 0> zxx;
            Permute3<3, 0, 0> wxx;
            Permute3<0, 1, 0> xyx;
            Permute3<1, 1, 0> yyx;
            Permute3<2, 1, 0> zyx;
            Permute3<3, 1, 0> wyx;
            Permute3<0, 2, 0> xzx;
            Permute3<1, 2, 0> yzx;
            Permute3<2, 2, 0> zzx;
            Permute3<3, 2, 0> wzx;
            Permute3<0, 3, 0> xwx;
            Permute3<1, 3, 0> ywx;
            Permute3<2, 3, 0> zwx;
            Permute3<3, 3, 0> wwx;
            Permute3<0, 0, 1> xxy;
            Permute3<1, 0, 1> yxy;
            Permute3<2, 0, 1> zxy;
            Permute3<3, 0, 1> wxy;
            Permute3<0, 1, 1> xyy;
            Permute3<1, 1, 1> yyy;
            Permute3<2, 1, 1> zyy;
            Permute3<3, 1, 1> wyy;
            Permute3<0, 2, 1> xzy;
            Permute3<1, 2, 1> yzy;
            Permute3<2, 2, 1> zzy;
            Permute3<3, 2, 1> wzy;
            Permute3<0, 3, 1> xwy;
            Permute3<1, 3, 1> ywy;
            Permute3<2, 3, 1> zwy;
            Permute3<3, 3, 1> wwy;
            Permute3<0, 0, 2> xxz;
            Permute3<1, 0, 2> yxz;
            Permute3<2, 0, 2> zxz;
            Permute3<3, 0, 2> wxz;
            Permute3<0, 1, 2> xyz;
            Permute3<1, 1, 2> yyz;
            Permute3<2, 1, 2> zyz;
            Permute3<3, 1, 2> wyz;
            Permute3<0, 2, 2> xzz;
            Permute3<1, 2, 2> yzz;
            Permute3<2, 2, 2> zzz;
            Permute3<3, 2, 2> wzz;
            Permute3<0, 3, 2> xwz;
            Permute3<1, 3, 2> ywz;
            Permute3<2, 3, 2> zwz;
            Permute3<3, 3, 2> wwz;
            Permute3<0, 0, 3> xxw;
            Permute3<1, 0, 3> yxw;
            Permute3<2, 0, 3> zxw;
            Permute3<3, 0, 3> wxw;
            Permute3<0, 1, 3> xyw;
            Permute3<1, 1, 3> yyw;
            Permute3<2, 1, 3> zyw;
            Permute3<3, 1, 3> wyw;
            Permute3<0, 2, 3> xzw;
            Permute3<1, 2, 3> yzw;
            Permute3<2, 2, 3> zzw;
            Permute3<3, 2, 3> wzw;
            Permute3<0, 3, 3> xww;
            Permute3<1, 3, 3> yww;
            Permute3<2, 3, 3> zww;
            Permute3<3, 3, 3> www;

            Permute4<0, 0, 0, 0> xxxx;
            Permute4<1, 0, 0, 0> yxxx;
            Permute4<2, 0, 0, 0> zxxx;
            Permute4<3, 0, 0, 0> wxxx;
            Permute4<0, 1, 0, 0> xyxx;
            Permute4<1, 1, 0, 0> yyxx;
            Permute4<2, 1, 0, 0> zyxx;
            Permute4<3, 1, 0, 0> wyxx;
            Permute4<0, 2, 0, 0> xzxx;
            Permute4<1, 2, 0, 0> yzxx;
            Permute4<2, 2, 0, 0> zzxx;
            Permute4<3, 2, 0, 0> wzxx;
            Permute4<0, 3, 0, 0> xwxx;
            Permute4<1, 3, 0, 0> ywxx;
            Permute4<2, 3, 0, 0> zwxx;
            Permute4<3, 3, 0, 0> wwxx;
            Permute4<0, 0, 1, 0> xxyx;
            Permute4<1, 0, 1, 0> yxyx;
            Permute4<2, 0, 1, 0> zxyx;
            Permute4<3, 0, 1, 0> wxyx;
            Permute4<0, 1, 1, 0> xyyx;
            Permute4<1, 1, 1, 0> yyyx;
            Permute4<2, 1, 1, 0> zyyx;
            Permute4<3, 1, 1, 0> wyyx;
            Permute4<0, 2, 1, 0> xzyx;
            Permute4<1, 2, 1, 0> yzyx;
            Permute4<2, 2, 1, 0> zzyx;
            Permute4<3, 2, 1, 0> wzyx;
            Permute4<0, 3, 1, 0> xwyx;
            Permute4<1, 3, 1, 0> ywyx;
            Permute4<2, 3, 1, 0> zwyx;
            Permute4<3, 3, 1, 0> wwyx;
            Permute4<0, 0, 2, 0> xxzx;
            Permute4<1, 0, 2, 0> yxzx;
            Permute4<2, 0, 2, 0> zxzx;
            Permute4<3, 0, 2, 0> wxzx;
            Permute4<0, 1, 2, 0> xyzx;
            Permute4<1, 1, 2, 0> yyzx;
            Permute4<2, 1, 2, 0> zyzx;
            Permute4<3, 1, 2, 0> wyzx;
            Permute4<0, 2, 2, 0> xzzx;
            Permute4<1, 2, 2, 0> yzzx;
            Permute4<2, 2, 2, 0> zzzx;
            Permute4<3, 2, 2, 0> wzzx;
            Permute4<0, 3, 2, 0> xwzx;
            Permute4<1, 3, 2, 0> ywzx;
            Permute4<2, 3, 2, 0> zwzx;
            Permute4<3, 3, 2, 0> wwzx;
            Permute4<0, 0, 3, 0> xxwx;
            Permute4<1, 0, 3, 0> yxwx;
            Permute4<2, 0, 3, 0> zxwx;
            Permute4<3, 0, 3, 0> wxwx;
            Permute4<0, 1, 3, 0> xywx;
            Permute4<1, 1, 3, 0> yywx;
            Permute4<2, 1, 3, 0> zywx;
            Permute4<3, 1, 3, 0> wywx;
            Permute4<0, 2, 3, 0> xzwx;
            Permute4<1, 2, 3, 0> yzwx;
            Permute4<2, 2, 3, 0> zzwx;
            Permute4<3, 2, 3, 0> wzwx;
            Permute4<0, 3, 3, 0> xwwx;
            Permute4<1, 3, 3, 0> ywwx;
            Permute4<2, 3, 3, 0> zwwx;
            Permute4<3, 3, 3, 0> wwwx;
            Permute4<0, 0, 0, 1> xxxy;
            Permute4<1, 0, 0, 1> yxxy;
            Permute4<2, 0, 0, 1> zxxy;
            Permute4<3, 0, 0, 1> wxxy;
            Permute4<0, 1, 0, 1> xyxy;
            Permute4<1, 1, 0, 1> yyxy;
            Permute4<2, 1, 0, 1> zyxy;
            Permute4<3, 1, 0, 1> wyxy;
            Permute4<0, 2, 0, 1> xzxy;
            Permute4<1, 2, 0, 1> yzxy;
            Permute4<2, 2, 0, 1> zzxy;
            Permute4<3, 2, 0, 1> wzxy;
            Permute4<0, 3, 0, 1> xwxy;
            Permute4<1, 3, 0, 1> ywxy;
            Permute4<2, 3, 0, 1> zwxy;
            Permute4<3, 3, 0, 1> wwxy;
            Permute4<0, 0, 1, 1> xxyy;
            Permute4<1, 0, 1, 1> yxyy;
            Permute4<2, 0, 1, 1> zxyy;
            Permute4<3, 0, 1, 1> wxyy;
            Permute4<0, 1, 1, 1> xyyy;
            Permute4<1, 1, 1, 1> yyyy;
            Permute4<2, 1, 1, 1> zyyy;
            Permute4<3, 1, 1, 1> wyyy;
            Permute4<0, 2, 1, 1> xzyy;
            Permute4<1, 2, 1, 1> yzyy;
            Permute4<2, 2, 1, 1> zzyy;
            Permute4<3, 2, 1, 1> wzyy;
            Permute4<0, 3, 1, 1> xwyy;
            Permute4<1, 3, 1, 1> ywyy;
            Permute4<2, 3, 1, 1> zwyy;
            Permute4<3, 3, 1, 1> wwyy;
            Permute4<0, 0, 2, 1> xxzy;
            Permute4<1, 0, 2, 1> yxzy;
            Permute4<2, 0, 2, 1> zxzy;
            Permute4<3, 0, 2, 1> wxzy;
            Permute4<0, 1, 2, 1> xyzy;
            Permute4<1, 1, 2, 1> yyzy;
            Permute4<2, 1, 2, 1> zyzy;
            Permute4<3, 1, 2, 1> wyzy;
            Permute4<0, 2, 2, 1> xzzy;
            Permute4<1, 2, 2, 1> yzzy;
            Permute4<2, 2, 2, 1> zzzy;
            Permute4<3, 2, 2, 1> wzzy;
            Permute4<0, 3, 2, 1> xwzy;
            Permute4<1, 3, 2, 1> ywzy;
            Permute4<2, 3, 2, 1> zwzy;
            Permute4<3, 3, 2, 1> wwzy;
            Permute4<0, 0, 3, 1> xxwy;
            Permute4<1, 0, 3, 1> yxwy;
            Permute4<2, 0, 3, 1> zxwy;
            Permute4<3, 0, 3, 1> wxwy;
            Permute4<0, 1, 3, 1> xywy;
            Permute4<1, 1, 3, 1> yywy;
            Permute4<2, 1, 3, 1> zywy;
            Permute4<3, 1, 3, 1> wywy;
            Permute4<0, 2, 3, 1> xzwy;
            Permute4<1, 2, 3, 1> yzwy;
            Permute4<2, 2, 3, 1> zzwy;
            Permute4<3, 2, 3, 1> wzwy;
            Permute4<0, 3, 3, 1> xwwy;
            Permute4<1, 3, 3, 1> ywwy;
            Permute4<2, 3, 3, 1> zwwy;
            Permute4<3, 3, 3, 1> wwwy;
            Permute4<0, 0, 0, 2> xxxz;
            Permute4<1, 0, 0, 2> yxxz;
            Permute4<2, 0, 0, 2> zxxz;
            Permute4<3, 0, 0, 2> wxxz;
            Permute4<0, 1, 0, 2> xyxz;
            Permute4<1, 1, 0, 2> yyxz;
            Permute4<2, 1, 0, 2> zyxz;
            Permute4<3, 1, 0, 2> wyxz;
            Permute4<0, 2, 0, 2> xzxz;
            Permute4<1, 2, 0, 2> yzxz;
            Permute4<2, 2, 0, 2> zzxz;
            Permute4<3, 2, 0, 2> wzxz;
            Permute4<0, 3, 0, 2> xwxz;
            Permute4<1, 3, 0, 2> ywxz;
            Permute4<2, 3, 0, 2> zwxz;
            Permute4<3, 3, 0, 2> wwxz;
            Permute4<0, 0, 1, 2> xxyz;
            Permute4<1, 0, 1, 2> yxyz;
            Permute4<2, 0, 1, 2> zxyz;
            Permute4<3, 0, 1, 2> wxyz;
            Permute4<0, 1, 1, 2> xyyz;
            Permute4<1, 1, 1, 2> yyyz;
            Permute4<2, 1, 1, 2> zyyz;
            Permute4<3, 1, 1, 2> wyyz;
            Permute4<0, 2, 1, 2> xzyz;
            Permute4<1, 2, 1, 2> yzyz;
            Permute4<2, 2, 1, 2> zzyz;
            Permute4<3, 2, 1, 2> wzyz;
            Permute4<0, 3, 1, 2> xwyz;
            Permute4<1, 3, 1, 2> ywyz;
            Permute4<2, 3, 1, 2> zwyz;
            Permute4<3, 3, 1, 2> wwyz;
            Permute4<0, 0, 2, 2> xxzz;
            Permute4<1, 0, 2, 2> yxzz;
            Permute4<2, 0, 2, 2> zxzz;
            Permute4<3, 0, 2, 2> wxzz;
            Permute4<0, 1, 2, 2> xyzz;
            Permute4<1, 1, 2, 2> yyzz;
            Permute4<2, 1, 2, 2> zyzz;
            Permute4<3, 1, 2, 2> wyzz;
            Permute4<0, 2, 2, 2> xzzz;
            Permute4<1, 2, 2, 2> yzzz;
            Permute4<2, 2, 2, 2> zzzz;
            Permute4<3, 2, 2, 2> wzzz;
            Permute4<0, 3, 2, 2> xwzz;
            Permute4<1, 3, 2, 2> ywzz;
            Permute4<2, 3, 2, 2> zwzz;
            Permute4<3, 3, 2, 2> wwzz;
            Permute4<0, 0, 3, 2> xxwz;
            Permute4<1, 0, 3, 2> yxwz;
            Permute4<2, 0, 3, 2> zxwz;
            Permute4<3, 0, 3, 2> wxwz;
            Permute4<0, 1, 3, 2> xywz;
            Permute4<1, 1, 3, 2> yywz;
            Permute4<2, 1, 3, 2> zywz;
            Permute4<3, 1, 3, 2> wywz;
            Permute4<0, 2, 3, 2> xzwz;
            Permute4<1, 2, 3, 2> yzwz;
            Permute4<2, 2, 3, 2> zzwz;
            Permute4<3, 2, 3, 2> wzwz;
            Permute4<0, 3, 3, 2> xwwz;
            Permute4<1, 3, 3, 2> ywwz;
            Permute4<2, 3, 3, 2> zwwz;
            Permute4<3, 3, 3, 2> wwwz;
            Permute4<0, 0, 0, 3> xxxw;
            Permute4<1, 0, 0, 3> yxxw;
            Permute4<2, 0, 0, 3> zxxw;
            Permute4<3, 0, 0, 3> wxxw;
            Permute4<0, 1, 0, 3> xyxw;
            Permute4<1, 1, 0, 3> yyxw;
            Permute4<2, 1, 0, 3> zyxw;
            Permute4<3, 1, 0, 3> wyxw;
            Permute4<0, 2, 0, 3> xzxw;
            Permute4<1, 2, 0, 3> yzxw;
            Permute4<2, 2, 0, 3> zzxw;
            Permute4<3, 2, 0, 3> wzxw;
            Permute4<0, 3, 0, 3> xwxw;
            Permute4<1, 3, 0, 3> ywxw;
            Permute4<2, 3, 0, 3> zwxw;
            Permute4<3, 3, 0, 3> wwxw;
            Permute4<0, 0, 1, 3> xxyw;
            Permute4<1, 0, 1, 3> yxyw;
            Permute4<2, 0, 1, 3> zxyw;
            Permute4<3, 0, 1, 3> wxyw;
            Permute4<0, 1, 1, 3> xyyw;
            Permute4<1, 1, 1, 3> yyyw;
            Permute4<2, 1, 1, 3> zyyw;
            Permute4<3, 1, 1, 3> wyyw;
            Permute4<0, 2, 1, 3> xzyw;
            Permute4<1, 2, 1, 3> yzyw;
            Permute4<2, 2, 1, 3> zzyw;
            Permute4<3, 2, 1, 3> wzyw;
            Permute4<0, 3, 1, 3> xwyw;
            Permute4<1, 3, 1, 3> ywyw;
            Permute4<2, 3, 1, 3> zwyw;
            Permute4<3, 3, 1, 3> wwyw;
            Permute4<0, 0, 2, 3> xxzw;
            Permute4<1, 0, 2, 3> yxzw;
            Permute4<2, 0, 2, 3> zxzw;
            Permute4<3, 0, 2, 3> wxzw;
            Permute4<0, 1, 2, 3> xyzw;
            Permute4<1, 1, 2, 3> yyzw;
            Permute4<2, 1, 2, 3> zyzw;
            Permute4<3, 1, 2, 3> wyzw;
            Permute4<0, 2, 2, 3> xzzw;
            Permute4<1, 2, 2, 3> yzzw;
            Permute4<2, 2, 2, 3> zzzw;
            Permute4<3, 2, 2, 3> wzzw;
            Permute4<0, 3, 2, 3> xwzw;
            Permute4<1, 3, 2, 3> ywzw;
            Permute4<2, 3, 2, 3> zwzw;
            Permute4<3, 3, 2, 3> wwzw;
            Permute4<0, 0, 3, 3> xxww;
            Permute4<1, 0, 3, 3> yxww;
            Permute4<2, 0, 3, 3> zxww;
            Permute4<3, 0, 3, 3> wxww;
            Permute4<0, 1, 3, 3> xyww;
            Permute4<1, 1, 3, 3> yyww;
            Permute4<2, 1, 3, 3> zyww;
            Permute4<3, 1, 3, 3> wyww;
            Permute4<0, 2, 3, 3> xzww;
            Permute4<1, 2, 3, 3> yzww;
            Permute4<2, 2, 3, 3> zzww;
            Permute4<3, 2, 3, 3> wzww;
            Permute4<0, 3, 3, 3> xwww;
            Permute4<1, 3, 3, 3> ywww;
            Permute4<2, 3, 3, 3> zwww;
            Permute4<3, 3, 3, 3> wwww;
        };

        explicit Vector() = default;

        explicit Vector(float s)
        : m(simd4f_set1(s))
        {
        }

        explicit Vector(int s)
        : m(simd4f_set1(float(s)))
        {
        }

        explicit Vector(float x, float y, float z, float w)
        : m(simd4f_set4(x, y, z, w))
        {
        }

        explicit Vector(const Vector<float, 2>& v, float z, float w)
        : m(simd4f_set4(v.x, v.y, z, w))
        {
        }

        explicit Vector(float x, float y, const Vector<float, 2>& v)
        : m(simd4f_set4(x, y, v.x, v.y))
        {
        }

        explicit Vector(float x, const Vector<float, 2>& v, float w)
        : m(simd4f_set4(x, v.x, v.y, w))
        {
        }

        explicit Vector(const Vector<float, 2>& xy, const Vector<float, 2>& zw)
        : m(simd4f_set4(xy.x, xy.y, zw.x, zw.y))
        {
        }

        explicit Vector(const Vector<float, 3>& v, float w)
        : m(simd4f_set4(v.x, v.y, v.z, w))
        {
        }

        explicit Vector(float x, const Vector<float, 3>& v)
        : m(simd4f_set4(x, v.x, v.y, v.z))
        {
        }

        Vector(__simd4f v)
        : m(v)
        {
        }

        template <int X, int Y, int Z, int W>
        Vector(const Permute4<X, Y, Z, W>& p)
        {
            m = p;
        }

        template <int X, int Y, int Z, int W>
        Vector& operator = (const Permute4<X, Y, Z, W>& p)
        {
            m = p;
            return *this;
        }

        Vector& operator = (__simd4f v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (float s)
        {
            m = simd4f_set1(s);
            return *this;
        }

        operator __simd4f () const
        {
            return m;
        }

        operator __simd4f ()
        {
            return m;
        }

        uint32 pack() const
        {
            const simd4i i = simd4i_convert(m);
            return simd4i_pack(i);
        }

        void unpack(uint32 a)
        {
            const simd4i i = simd4i_unpack(a);
            m = simd4f_convert(i);
        }
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline const float4& operator + (const float4& v)
    {
        return v;
    }

    static inline float4 operator - (const float4& v)
    {
        return simd4f_neg(v);
    }

    static inline float4& operator += (float4& a, const float4& b)
    {
        a = simd4f_add(a, b);
        return a;
    }

    static inline float4& operator -= (float4& a, const float4& b)
    {
        a = simd4f_sub(a, b);
        return a;
    }

    static inline float4& operator *= (float4& a, const float4& b)
    {
        a = simd4f_mul(a, b);
        return a;
    }

    static inline float4& operator *= (float4& a, float b)
    {
        a = simd4f_mul(a, b);
        return a;
    }

    static inline float4& operator /= (float4& a, const float4& b)
    {
        a = simd4f_div(a, b);
        return a;
    }

    static inline float4& operator /= (float4& a, float b)
    {
        a = simd4f_div(a, b);
        return a;
    }

    static inline float4 operator + (const float4& a, const float4& b)
    {
        return simd4f_add(a, b);
    }

    template <int X, int Y, int Z, int W, int A, int B, int C, int D>
    static inline float4 operator + (const float4::Permute4<X,Y,Z,W>& a, const float4::Permute4<A,B,C,D>& b)
    {
        return simd4f_add(a, b);
    }

    static inline float4 operator - (const float4& a, const float4& b)
    {
        return simd4f_sub(a, b);
    }

    template <int X, int Y, int Z, int W, int A, int B, int C, int D>
    static inline float4 operator - (const float4::Permute4<X,Y,Z,W>& a, const float4::Permute4<A,B,C,D>& b)
    {
        return simd4f_sub(a, b);
    }

    static inline float4 operator * (const float4& a, const float4& b)
    {
        return simd4f_mul(a, b);
    }

    static inline float4 operator * (const float4& a, float b)
    {
        return simd4f_mul(a, b);
    }

    static inline float4 operator * (float a, const float4& b)
    {
        return simd4f_mul(a, b);
    }

    template <int X, int Y, int Z, int W, int A, int B, int C, int D>
    static inline float4 operator * (const float4::Permute4<X,Y,Z,W>& a, const float4::Permute4<A,B,C,D>& b)
    {
        return simd4f_mul(a, b);
    }

    static inline float4 operator / (const float4& a, const float4& b)
    {
        return simd4f_div(a, b);
    }

    static inline float4 operator / (const float4& a, float b)
    {
        return simd4f_div(a, b);
    }

    template <int X, int Y, int Z, int W, int A, int B, int C, int D>
    static inline float4 operator / (const float4::Permute4<X,Y,Z,W>& a, const float4::Permute4<A,B,C,D>& b)
    {
        return simd4f_div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

#define MAKE_VECTOR_FUNCTION1(Name, SimdName) \
    static inline float4 Name(const float4& a) { \
        return SimdName(a); \
    }

#define MAKE_VECTOR_FUNCTION2(Name, SimdName) \
    static inline float4 Name(const float4& a, const float4& b) { \
        return SimdName(a, b); \
    }
    
    MAKE_VECTOR_FUNCTION1(abs, simd4f_abs)
    MAKE_VECTOR_FUNCTION1(square, simd4f_square)
    MAKE_VECTOR_FUNCTION1(length, simd4f_length)
    MAKE_VECTOR_FUNCTION1(normalize, simd4f_normalize)
    MAKE_VECTOR_FUNCTION1(round, simd4f_round)
    MAKE_VECTOR_FUNCTION1(floor, simd4f_floor)
    MAKE_VECTOR_FUNCTION1(ceil, simd4f_ceil)
    MAKE_VECTOR_FUNCTION1(fract, simd4f_fract)
    MAKE_VECTOR_FUNCTION1(sin, simd4f_sin)
    MAKE_VECTOR_FUNCTION1(cos, simd4f_cos)
    MAKE_VECTOR_FUNCTION1(tan, simd4f_tan)
    MAKE_VECTOR_FUNCTION1(asin, simd4f_asin)
    MAKE_VECTOR_FUNCTION1(acos, simd4f_acos)
    MAKE_VECTOR_FUNCTION1(atan, simd4f_atan)
    MAKE_VECTOR_FUNCTION1(exp, simd4f_exp)
    MAKE_VECTOR_FUNCTION1(log, simd4f_log)
    MAKE_VECTOR_FUNCTION1(exp2, simd4f_exp2)
    MAKE_VECTOR_FUNCTION1(log2, simd4f_log2)
    MAKE_VECTOR_FUNCTION1(sign, simd4f_sign)
    MAKE_VECTOR_FUNCTION1(radians, simd4f_radians)
    MAKE_VECTOR_FUNCTION1(degrees, simd4f_degrees)
    MAKE_VECTOR_FUNCTION1(sqrt, simd4f_sqrt)
    MAKE_VECTOR_FUNCTION1(rsqrt, simd4f_rsqrt)

    MAKE_VECTOR_FUNCTION2(min, simd4f_min)
    MAKE_VECTOR_FUNCTION2(max, simd4f_max)
    MAKE_VECTOR_FUNCTION2(dot, simd4f_dot4)
    MAKE_VECTOR_FUNCTION2(cross, simd4f_cross3)
    MAKE_VECTOR_FUNCTION2(mod, simd4f_mod)
    MAKE_VECTOR_FUNCTION2(pow, simd4f_pow)
    MAKE_VECTOR_FUNCTION2(atan2, simd4f_atan2)

#undef MAKE_VECTOR_FUNCTION1
#undef MAKE_VECTOR_FUNCTION2

    static inline float4 clamp(const float4& a, const float4& amin, const float4& amax)
    {
        return simd4f_clamp(a, amin, amax);
    }

    static inline float4 madd(const float4& a, const float4& b, const float4& c)
    {
        return simd4f_madd(a, b, c);
    }

    static inline float4 msub(const float4& a, const float4& b, const float4& c)
    {
        return simd4f_msub(a, b, c);
    }

    static inline float4 lerp(const float4& a, const float4& b, float factor)
    {
        return a + (b - a) * factor;
    }

    static inline float4 lerp(const float4& a, const float4& b, const float4& factor)
    {
        return a + (b - a) * factor;
    }

    static inline float4 hmin(const float4& v)
    {
        const float4 temp = min(v, v.zwxy);
        return min(temp, temp.yxwz);
    }

    static inline float4 hmax(const float4& v)
    {
        const float4 temp = max(v, v.zwxy);
        return max(temp, temp.yxwz);
    }

    // ------------------------------------------------------------------
    // compare / select
    // ------------------------------------------------------------------

    static inline simd4f operator > (const float4& a, const float4& b)
    {
        return simd4f_compare_gt(a, b);
    }

    static inline simd4f operator >= (const float4& a, const float4& b)
    {
        return simd4f_compare_ge(a, b);
    }

    static inline simd4f operator < (const float4& a, const float4& b)
    {
        return simd4f_compare_lt(a, b);
    }

    static inline simd4f operator <= (const float4& a, const float4& b)
    {
        return simd4f_compare_le(a, b);
    }

    static inline simd4f operator == (const float4& a, const float4& b)
    {
        return simd4f_compare_eq(a, b);
    }

    static inline simd4f operator != (const float4& a, const float4& b)
    {
        return simd4f_compare_neq(a, b);
    }

    static inline simd4f select(__simd4f mask, const float4& a, const float4& b)
    {
        return simd4f_select(mask, a, b);
    }

} // namespace mango

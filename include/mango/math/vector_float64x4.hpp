/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"
#include "vector_float64x2.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // Vector<double, 4>
    // ------------------------------------------------------------------

    template <>
    struct Vector<double, 4> : VectorBase<double, 4>
    {
        using VectorType = simd::float64x4;

        union
        {
            simd::float64x4 xyzw;

            LowAccessor<Vector<double, 2>, simd::float64x4> low;
            HighAccessor<Vector<double, 2>, simd::float64x4> high;

            ScalarAccessor<double, simd::float64x4, 0> x;
            ScalarAccessor<double, simd::float64x4, 1> y;
            ScalarAccessor<double, simd::float64x4, 2> z;
            ScalarAccessor<double, simd::float64x4, 3> w;

            Permute4x2<double, simd::float64x4, 0, 0> xx;
            Permute4x2<double, simd::float64x4, 1, 0> yx;
            Permute4x2<double, simd::float64x4, 2, 0> zx;
            Permute4x2<double, simd::float64x4, 3, 0> wx;
            Permute4x2<double, simd::float64x4, 0, 1> xy;
            Permute4x2<double, simd::float64x4, 1, 1> yy;
            Permute4x2<double, simd::float64x4, 2, 1> zy;
            Permute4x2<double, simd::float64x4, 3, 1> wy;
            Permute4x2<double, simd::float64x4, 0, 2> xz;
            Permute4x2<double, simd::float64x4, 1, 2> yz;
            Permute4x2<double, simd::float64x4, 2, 2> zz;
            Permute4x2<double, simd::float64x4, 3, 2> wz;
            Permute4x2<double, simd::float64x4, 0, 3> xw;
            Permute4x2<double, simd::float64x4, 1, 3> yw;
            Permute4x2<double, simd::float64x4, 2, 3> zw;
            Permute4x2<double, simd::float64x4, 3, 3> ww;

            Permute4x3<double, simd::float64x4, 0, 0, 0> xxx;
            Permute4x3<double, simd::float64x4, 1, 0, 0> yxx;
            Permute4x3<double, simd::float64x4, 2, 0, 0> zxx;
            Permute4x3<double, simd::float64x4, 3, 0, 0> wxx;
            Permute4x3<double, simd::float64x4, 0, 1, 0> xyx;
            Permute4x3<double, simd::float64x4, 1, 1, 0> yyx;
            Permute4x3<double, simd::float64x4, 2, 1, 0> zyx;
            Permute4x3<double, simd::float64x4, 3, 1, 0> wyx;
            Permute4x3<double, simd::float64x4, 0, 2, 0> xzx;
            Permute4x3<double, simd::float64x4, 1, 2, 0> yzx;
            Permute4x3<double, simd::float64x4, 2, 2, 0> zzx;
            Permute4x3<double, simd::float64x4, 3, 2, 0> wzx;
            Permute4x3<double, simd::float64x4, 0, 3, 0> xwx;
            Permute4x3<double, simd::float64x4, 1, 3, 0> ywx;
            Permute4x3<double, simd::float64x4, 2, 3, 0> zwx;
            Permute4x3<double, simd::float64x4, 3, 3, 0> wwx;
            Permute4x3<double, simd::float64x4, 0, 0, 1> xxy;
            Permute4x3<double, simd::float64x4, 1, 0, 1> yxy;
            Permute4x3<double, simd::float64x4, 2, 0, 1> zxy;
            Permute4x3<double, simd::float64x4, 3, 0, 1> wxy;
            Permute4x3<double, simd::float64x4, 0, 1, 1> xyy;
            Permute4x3<double, simd::float64x4, 1, 1, 1> yyy;
            Permute4x3<double, simd::float64x4, 2, 1, 1> zyy;
            Permute4x3<double, simd::float64x4, 3, 1, 1> wyy;
            Permute4x3<double, simd::float64x4, 0, 2, 1> xzy;
            Permute4x3<double, simd::float64x4, 1, 2, 1> yzy;
            Permute4x3<double, simd::float64x4, 2, 2, 1> zzy;
            Permute4x3<double, simd::float64x4, 3, 2, 1> wzy;
            Permute4x3<double, simd::float64x4, 0, 3, 1> xwy;
            Permute4x3<double, simd::float64x4, 1, 3, 1> ywy;
            Permute4x3<double, simd::float64x4, 2, 3, 1> zwy;
            Permute4x3<double, simd::float64x4, 3, 3, 1> wwy;
            Permute4x3<double, simd::float64x4, 0, 0, 2> xxz;
            Permute4x3<double, simd::float64x4, 1, 0, 2> yxz;
            Permute4x3<double, simd::float64x4, 2, 0, 2> zxz;
            Permute4x3<double, simd::float64x4, 3, 0, 2> wxz;
            Permute4x3<double, simd::float64x4, 0, 1, 2> xyz;
            Permute4x3<double, simd::float64x4, 1, 1, 2> yyz;
            Permute4x3<double, simd::float64x4, 2, 1, 2> zyz;
            Permute4x3<double, simd::float64x4, 3, 1, 2> wyz;
            Permute4x3<double, simd::float64x4, 0, 2, 2> xzz;
            Permute4x3<double, simd::float64x4, 1, 2, 2> yzz;
            Permute4x3<double, simd::float64x4, 2, 2, 2> zzz;
            Permute4x3<double, simd::float64x4, 3, 2, 2> wzz;
            Permute4x3<double, simd::float64x4, 0, 3, 2> xwz;
            Permute4x3<double, simd::float64x4, 1, 3, 2> ywz;
            Permute4x3<double, simd::float64x4, 2, 3, 2> zwz;
            Permute4x3<double, simd::float64x4, 3, 3, 2> wwz;
            Permute4x3<double, simd::float64x4, 0, 0, 3> xxw;
            Permute4x3<double, simd::float64x4, 1, 0, 3> yxw;
            Permute4x3<double, simd::float64x4, 2, 0, 3> zxw;
            Permute4x3<double, simd::float64x4, 3, 0, 3> wxw;
            Permute4x3<double, simd::float64x4, 0, 1, 3> xyw;
            Permute4x3<double, simd::float64x4, 1, 1, 3> yyw;
            Permute4x3<double, simd::float64x4, 2, 1, 3> zyw;
            Permute4x3<double, simd::float64x4, 3, 1, 3> wyw;
            Permute4x3<double, simd::float64x4, 0, 2, 3> xzw;
            Permute4x3<double, simd::float64x4, 1, 2, 3> yzw;
            Permute4x3<double, simd::float64x4, 2, 2, 3> zzw;
            Permute4x3<double, simd::float64x4, 3, 2, 3> wzw;
            Permute4x3<double, simd::float64x4, 0, 3, 3> xww;
            Permute4x3<double, simd::float64x4, 1, 3, 3> yww;
            Permute4x3<double, simd::float64x4, 2, 3, 3> zww;
            Permute4x3<double, simd::float64x4, 3, 3, 3> www;

            Permute4<double, simd::float64x4, 0, 0, 0, 0> xxxx;
            Permute4<double, simd::float64x4, 1, 0, 0, 0> yxxx;
            Permute4<double, simd::float64x4, 2, 0, 0, 0> zxxx;
            Permute4<double, simd::float64x4, 3, 0, 0, 0> wxxx;
            Permute4<double, simd::float64x4, 0, 1, 0, 0> xyxx;
            Permute4<double, simd::float64x4, 1, 1, 0, 0> yyxx;
            Permute4<double, simd::float64x4, 2, 1, 0, 0> zyxx;
            Permute4<double, simd::float64x4, 3, 1, 0, 0> wyxx;
            Permute4<double, simd::float64x4, 0, 2, 0, 0> xzxx;
            Permute4<double, simd::float64x4, 1, 2, 0, 0> yzxx;
            Permute4<double, simd::float64x4, 2, 2, 0, 0> zzxx;
            Permute4<double, simd::float64x4, 3, 2, 0, 0> wzxx;
            Permute4<double, simd::float64x4, 0, 3, 0, 0> xwxx;
            Permute4<double, simd::float64x4, 1, 3, 0, 0> ywxx;
            Permute4<double, simd::float64x4, 2, 3, 0, 0> zwxx;
            Permute4<double, simd::float64x4, 3, 3, 0, 0> wwxx;
            Permute4<double, simd::float64x4, 0, 0, 1, 0> xxyx;
            Permute4<double, simd::float64x4, 1, 0, 1, 0> yxyx;
            Permute4<double, simd::float64x4, 2, 0, 1, 0> zxyx;
            Permute4<double, simd::float64x4, 3, 0, 1, 0> wxyx;
            Permute4<double, simd::float64x4, 0, 1, 1, 0> xyyx;
            Permute4<double, simd::float64x4, 1, 1, 1, 0> yyyx;
            Permute4<double, simd::float64x4, 2, 1, 1, 0> zyyx;
            Permute4<double, simd::float64x4, 3, 1, 1, 0> wyyx;
            Permute4<double, simd::float64x4, 0, 2, 1, 0> xzyx;
            Permute4<double, simd::float64x4, 1, 2, 1, 0> yzyx;
            Permute4<double, simd::float64x4, 2, 2, 1, 0> zzyx;
            Permute4<double, simd::float64x4, 3, 2, 1, 0> wzyx;
            Permute4<double, simd::float64x4, 0, 3, 1, 0> xwyx;
            Permute4<double, simd::float64x4, 1, 3, 1, 0> ywyx;
            Permute4<double, simd::float64x4, 2, 3, 1, 0> zwyx;
            Permute4<double, simd::float64x4, 3, 3, 1, 0> wwyx;
            Permute4<double, simd::float64x4, 0, 0, 2, 0> xxzx;
            Permute4<double, simd::float64x4, 1, 0, 2, 0> yxzx;
            Permute4<double, simd::float64x4, 2, 0, 2, 0> zxzx;
            Permute4<double, simd::float64x4, 3, 0, 2, 0> wxzx;
            Permute4<double, simd::float64x4, 0, 1, 2, 0> xyzx;
            Permute4<double, simd::float64x4, 1, 1, 2, 0> yyzx;
            Permute4<double, simd::float64x4, 2, 1, 2, 0> zyzx;
            Permute4<double, simd::float64x4, 3, 1, 2, 0> wyzx;
            Permute4<double, simd::float64x4, 0, 2, 2, 0> xzzx;
            Permute4<double, simd::float64x4, 1, 2, 2, 0> yzzx;
            Permute4<double, simd::float64x4, 2, 2, 2, 0> zzzx;
            Permute4<double, simd::float64x4, 3, 2, 2, 0> wzzx;
            Permute4<double, simd::float64x4, 0, 3, 2, 0> xwzx;
            Permute4<double, simd::float64x4, 1, 3, 2, 0> ywzx;
            Permute4<double, simd::float64x4, 2, 3, 2, 0> zwzx;
            Permute4<double, simd::float64x4, 3, 3, 2, 0> wwzx;
            Permute4<double, simd::float64x4, 0, 0, 3, 0> xxwx;
            Permute4<double, simd::float64x4, 1, 0, 3, 0> yxwx;
            Permute4<double, simd::float64x4, 2, 0, 3, 0> zxwx;
            Permute4<double, simd::float64x4, 3, 0, 3, 0> wxwx;
            Permute4<double, simd::float64x4, 0, 1, 3, 0> xywx;
            Permute4<double, simd::float64x4, 1, 1, 3, 0> yywx;
            Permute4<double, simd::float64x4, 2, 1, 3, 0> zywx;
            Permute4<double, simd::float64x4, 3, 1, 3, 0> wywx;
            Permute4<double, simd::float64x4, 0, 2, 3, 0> xzwx;
            Permute4<double, simd::float64x4, 1, 2, 3, 0> yzwx;
            Permute4<double, simd::float64x4, 2, 2, 3, 0> zzwx;
            Permute4<double, simd::float64x4, 3, 2, 3, 0> wzwx;
            Permute4<double, simd::float64x4, 0, 3, 3, 0> xwwx;
            Permute4<double, simd::float64x4, 1, 3, 3, 0> ywwx;
            Permute4<double, simd::float64x4, 2, 3, 3, 0> zwwx;
            Permute4<double, simd::float64x4, 3, 3, 3, 0> wwwx;
            Permute4<double, simd::float64x4, 0, 0, 0, 1> xxxy;
            Permute4<double, simd::float64x4, 1, 0, 0, 1> yxxy;
            Permute4<double, simd::float64x4, 2, 0, 0, 1> zxxy;
            Permute4<double, simd::float64x4, 3, 0, 0, 1> wxxy;
            Permute4<double, simd::float64x4, 0, 1, 0, 1> xyxy;
            Permute4<double, simd::float64x4, 1, 1, 0, 1> yyxy;
            Permute4<double, simd::float64x4, 2, 1, 0, 1> zyxy;
            Permute4<double, simd::float64x4, 3, 1, 0, 1> wyxy;
            Permute4<double, simd::float64x4, 0, 2, 0, 1> xzxy;
            Permute4<double, simd::float64x4, 1, 2, 0, 1> yzxy;
            Permute4<double, simd::float64x4, 2, 2, 0, 1> zzxy;
            Permute4<double, simd::float64x4, 3, 2, 0, 1> wzxy;
            Permute4<double, simd::float64x4, 0, 3, 0, 1> xwxy;
            Permute4<double, simd::float64x4, 1, 3, 0, 1> ywxy;
            Permute4<double, simd::float64x4, 2, 3, 0, 1> zwxy;
            Permute4<double, simd::float64x4, 3, 3, 0, 1> wwxy;
            Permute4<double, simd::float64x4, 0, 0, 1, 1> xxyy;
            Permute4<double, simd::float64x4, 1, 0, 1, 1> yxyy;
            Permute4<double, simd::float64x4, 2, 0, 1, 1> zxyy;
            Permute4<double, simd::float64x4, 3, 0, 1, 1> wxyy;
            Permute4<double, simd::float64x4, 0, 1, 1, 1> xyyy;
            Permute4<double, simd::float64x4, 1, 1, 1, 1> yyyy;
            Permute4<double, simd::float64x4, 2, 1, 1, 1> zyyy;
            Permute4<double, simd::float64x4, 3, 1, 1, 1> wyyy;
            Permute4<double, simd::float64x4, 0, 2, 1, 1> xzyy;
            Permute4<double, simd::float64x4, 1, 2, 1, 1> yzyy;
            Permute4<double, simd::float64x4, 2, 2, 1, 1> zzyy;
            Permute4<double, simd::float64x4, 3, 2, 1, 1> wzyy;
            Permute4<double, simd::float64x4, 0, 3, 1, 1> xwyy;
            Permute4<double, simd::float64x4, 1, 3, 1, 1> ywyy;
            Permute4<double, simd::float64x4, 2, 3, 1, 1> zwyy;
            Permute4<double, simd::float64x4, 3, 3, 1, 1> wwyy;
            Permute4<double, simd::float64x4, 0, 0, 2, 1> xxzy;
            Permute4<double, simd::float64x4, 1, 0, 2, 1> yxzy;
            Permute4<double, simd::float64x4, 2, 0, 2, 1> zxzy;
            Permute4<double, simd::float64x4, 3, 0, 2, 1> wxzy;
            Permute4<double, simd::float64x4, 0, 1, 2, 1> xyzy;
            Permute4<double, simd::float64x4, 1, 1, 2, 1> yyzy;
            Permute4<double, simd::float64x4, 2, 1, 2, 1> zyzy;
            Permute4<double, simd::float64x4, 3, 1, 2, 1> wyzy;
            Permute4<double, simd::float64x4, 0, 2, 2, 1> xzzy;
            Permute4<double, simd::float64x4, 1, 2, 2, 1> yzzy;
            Permute4<double, simd::float64x4, 2, 2, 2, 1> zzzy;
            Permute4<double, simd::float64x4, 3, 2, 2, 1> wzzy;
            Permute4<double, simd::float64x4, 0, 3, 2, 1> xwzy;
            Permute4<double, simd::float64x4, 1, 3, 2, 1> ywzy;
            Permute4<double, simd::float64x4, 2, 3, 2, 1> zwzy;
            Permute4<double, simd::float64x4, 3, 3, 2, 1> wwzy;
            Permute4<double, simd::float64x4, 0, 0, 3, 1> xxwy;
            Permute4<double, simd::float64x4, 1, 0, 3, 1> yxwy;
            Permute4<double, simd::float64x4, 2, 0, 3, 1> zxwy;
            Permute4<double, simd::float64x4, 3, 0, 3, 1> wxwy;
            Permute4<double, simd::float64x4, 0, 1, 3, 1> xywy;
            Permute4<double, simd::float64x4, 1, 1, 3, 1> yywy;
            Permute4<double, simd::float64x4, 2, 1, 3, 1> zywy;
            Permute4<double, simd::float64x4, 3, 1, 3, 1> wywy;
            Permute4<double, simd::float64x4, 0, 2, 3, 1> xzwy;
            Permute4<double, simd::float64x4, 1, 2, 3, 1> yzwy;
            Permute4<double, simd::float64x4, 2, 2, 3, 1> zzwy;
            Permute4<double, simd::float64x4, 3, 2, 3, 1> wzwy;
            Permute4<double, simd::float64x4, 0, 3, 3, 1> xwwy;
            Permute4<double, simd::float64x4, 1, 3, 3, 1> ywwy;
            Permute4<double, simd::float64x4, 2, 3, 3, 1> zwwy;
            Permute4<double, simd::float64x4, 3, 3, 3, 1> wwwy;
            Permute4<double, simd::float64x4, 0, 0, 0, 2> xxxz;
            Permute4<double, simd::float64x4, 1, 0, 0, 2> yxxz;
            Permute4<double, simd::float64x4, 2, 0, 0, 2> zxxz;
            Permute4<double, simd::float64x4, 3, 0, 0, 2> wxxz;
            Permute4<double, simd::float64x4, 0, 1, 0, 2> xyxz;
            Permute4<double, simd::float64x4, 1, 1, 0, 2> yyxz;
            Permute4<double, simd::float64x4, 2, 1, 0, 2> zyxz;
            Permute4<double, simd::float64x4, 3, 1, 0, 2> wyxz;
            Permute4<double, simd::float64x4, 0, 2, 0, 2> xzxz;
            Permute4<double, simd::float64x4, 1, 2, 0, 2> yzxz;
            Permute4<double, simd::float64x4, 2, 2, 0, 2> zzxz;
            Permute4<double, simd::float64x4, 3, 2, 0, 2> wzxz;
            Permute4<double, simd::float64x4, 0, 3, 0, 2> xwxz;
            Permute4<double, simd::float64x4, 1, 3, 0, 2> ywxz;
            Permute4<double, simd::float64x4, 2, 3, 0, 2> zwxz;
            Permute4<double, simd::float64x4, 3, 3, 0, 2> wwxz;
            Permute4<double, simd::float64x4, 0, 0, 1, 2> xxyz;
            Permute4<double, simd::float64x4, 1, 0, 1, 2> yxyz;
            Permute4<double, simd::float64x4, 2, 0, 1, 2> zxyz;
            Permute4<double, simd::float64x4, 3, 0, 1, 2> wxyz;
            Permute4<double, simd::float64x4, 0, 1, 1, 2> xyyz;
            Permute4<double, simd::float64x4, 1, 1, 1, 2> yyyz;
            Permute4<double, simd::float64x4, 2, 1, 1, 2> zyyz;
            Permute4<double, simd::float64x4, 3, 1, 1, 2> wyyz;
            Permute4<double, simd::float64x4, 0, 2, 1, 2> xzyz;
            Permute4<double, simd::float64x4, 1, 2, 1, 2> yzyz;
            Permute4<double, simd::float64x4, 2, 2, 1, 2> zzyz;
            Permute4<double, simd::float64x4, 3, 2, 1, 2> wzyz;
            Permute4<double, simd::float64x4, 0, 3, 1, 2> xwyz;
            Permute4<double, simd::float64x4, 1, 3, 1, 2> ywyz;
            Permute4<double, simd::float64x4, 2, 3, 1, 2> zwyz;
            Permute4<double, simd::float64x4, 3, 3, 1, 2> wwyz;
            Permute4<double, simd::float64x4, 0, 0, 2, 2> xxzz;
            Permute4<double, simd::float64x4, 1, 0, 2, 2> yxzz;
            Permute4<double, simd::float64x4, 2, 0, 2, 2> zxzz;
            Permute4<double, simd::float64x4, 3, 0, 2, 2> wxzz;
            Permute4<double, simd::float64x4, 0, 1, 2, 2> xyzz;
            Permute4<double, simd::float64x4, 1, 1, 2, 2> yyzz;
            Permute4<double, simd::float64x4, 2, 1, 2, 2> zyzz;
            Permute4<double, simd::float64x4, 3, 1, 2, 2> wyzz;
            Permute4<double, simd::float64x4, 0, 2, 2, 2> xzzz;
            Permute4<double, simd::float64x4, 1, 2, 2, 2> yzzz;
            Permute4<double, simd::float64x4, 2, 2, 2, 2> zzzz;
            Permute4<double, simd::float64x4, 3, 2, 2, 2> wzzz;
            Permute4<double, simd::float64x4, 0, 3, 2, 2> xwzz;
            Permute4<double, simd::float64x4, 1, 3, 2, 2> ywzz;
            Permute4<double, simd::float64x4, 2, 3, 2, 2> zwzz;
            Permute4<double, simd::float64x4, 3, 3, 2, 2> wwzz;
            Permute4<double, simd::float64x4, 0, 0, 3, 2> xxwz;
            Permute4<double, simd::float64x4, 1, 0, 3, 2> yxwz;
            Permute4<double, simd::float64x4, 2, 0, 3, 2> zxwz;
            Permute4<double, simd::float64x4, 3, 0, 3, 2> wxwz;
            Permute4<double, simd::float64x4, 0, 1, 3, 2> xywz;
            Permute4<double, simd::float64x4, 1, 1, 3, 2> yywz;
            Permute4<double, simd::float64x4, 2, 1, 3, 2> zywz;
            Permute4<double, simd::float64x4, 3, 1, 3, 2> wywz;
            Permute4<double, simd::float64x4, 0, 2, 3, 2> xzwz;
            Permute4<double, simd::float64x4, 1, 2, 3, 2> yzwz;
            Permute4<double, simd::float64x4, 2, 2, 3, 2> zzwz;
            Permute4<double, simd::float64x4, 3, 2, 3, 2> wzwz;
            Permute4<double, simd::float64x4, 0, 3, 3, 2> xwwz;
            Permute4<double, simd::float64x4, 1, 3, 3, 2> ywwz;
            Permute4<double, simd::float64x4, 2, 3, 3, 2> zwwz;
            Permute4<double, simd::float64x4, 3, 3, 3, 2> wwwz;
            Permute4<double, simd::float64x4, 0, 0, 0, 3> xxxw;
            Permute4<double, simd::float64x4, 1, 0, 0, 3> yxxw;
            Permute4<double, simd::float64x4, 2, 0, 0, 3> zxxw;
            Permute4<double, simd::float64x4, 3, 0, 0, 3> wxxw;
            Permute4<double, simd::float64x4, 0, 1, 0, 3> xyxw;
            Permute4<double, simd::float64x4, 1, 1, 0, 3> yyxw;
            Permute4<double, simd::float64x4, 2, 1, 0, 3> zyxw;
            Permute4<double, simd::float64x4, 3, 1, 0, 3> wyxw;
            Permute4<double, simd::float64x4, 0, 2, 0, 3> xzxw;
            Permute4<double, simd::float64x4, 1, 2, 0, 3> yzxw;
            Permute4<double, simd::float64x4, 2, 2, 0, 3> zzxw;
            Permute4<double, simd::float64x4, 3, 2, 0, 3> wzxw;
            Permute4<double, simd::float64x4, 0, 3, 0, 3> xwxw;
            Permute4<double, simd::float64x4, 1, 3, 0, 3> ywxw;
            Permute4<double, simd::float64x4, 2, 3, 0, 3> zwxw;
            Permute4<double, simd::float64x4, 3, 3, 0, 3> wwxw;
            Permute4<double, simd::float64x4, 0, 0, 1, 3> xxyw;
            Permute4<double, simd::float64x4, 1, 0, 1, 3> yxyw;
            Permute4<double, simd::float64x4, 2, 0, 1, 3> zxyw;
            Permute4<double, simd::float64x4, 3, 0, 1, 3> wxyw;
            Permute4<double, simd::float64x4, 0, 1, 1, 3> xyyw;
            Permute4<double, simd::float64x4, 1, 1, 1, 3> yyyw;
            Permute4<double, simd::float64x4, 2, 1, 1, 3> zyyw;
            Permute4<double, simd::float64x4, 3, 1, 1, 3> wyyw;
            Permute4<double, simd::float64x4, 0, 2, 1, 3> xzyw;
            Permute4<double, simd::float64x4, 1, 2, 1, 3> yzyw;
            Permute4<double, simd::float64x4, 2, 2, 1, 3> zzyw;
            Permute4<double, simd::float64x4, 3, 2, 1, 3> wzyw;
            Permute4<double, simd::float64x4, 0, 3, 1, 3> xwyw;
            Permute4<double, simd::float64x4, 1, 3, 1, 3> ywyw;
            Permute4<double, simd::float64x4, 2, 3, 1, 3> zwyw;
            Permute4<double, simd::float64x4, 3, 3, 1, 3> wwyw;
            Permute4<double, simd::float64x4, 0, 0, 2, 3> xxzw;
            Permute4<double, simd::float64x4, 1, 0, 2, 3> yxzw;
            Permute4<double, simd::float64x4, 2, 0, 2, 3> zxzw;
            Permute4<double, simd::float64x4, 3, 0, 2, 3> wxzw;
            Permute4<double, simd::float64x4, 1, 1, 2, 3> yyzw;
            Permute4<double, simd::float64x4, 2, 1, 2, 3> zyzw;
            Permute4<double, simd::float64x4, 3, 1, 2, 3> wyzw;
            Permute4<double, simd::float64x4, 0, 2, 2, 3> xzzw;
            Permute4<double, simd::float64x4, 1, 2, 2, 3> yzzw;
            Permute4<double, simd::float64x4, 2, 2, 2, 3> zzzw;
            Permute4<double, simd::float64x4, 3, 2, 2, 3> wzzw;
            Permute4<double, simd::float64x4, 0, 3, 2, 3> xwzw;
            Permute4<double, simd::float64x4, 1, 3, 2, 3> ywzw;
            Permute4<double, simd::float64x4, 2, 3, 2, 3> zwzw;
            Permute4<double, simd::float64x4, 3, 3, 2, 3> wwzw;
            Permute4<double, simd::float64x4, 0, 0, 3, 3> xxww;
            Permute4<double, simd::float64x4, 1, 0, 3, 3> yxww;
            Permute4<double, simd::float64x4, 2, 0, 3, 3> zxww;
            Permute4<double, simd::float64x4, 3, 0, 3, 3> wxww;
            Permute4<double, simd::float64x4, 0, 1, 3, 3> xyww;
            Permute4<double, simd::float64x4, 1, 1, 3, 3> yyww;
            Permute4<double, simd::float64x4, 2, 1, 3, 3> zyww;
            Permute4<double, simd::float64x4, 3, 1, 3, 3> wyww;
            Permute4<double, simd::float64x4, 0, 2, 3, 3> xzww;
            Permute4<double, simd::float64x4, 1, 2, 3, 3> yzww;
            Permute4<double, simd::float64x4, 2, 2, 3, 3> zzww;
            Permute4<double, simd::float64x4, 3, 2, 3, 3> wzww;
            Permute4<double, simd::float64x4, 0, 3, 3, 3> xwww;
            Permute4<double, simd::float64x4, 1, 3, 3, 3> ywww;
            Permute4<double, simd::float64x4, 2, 3, 3, 3> zwww;
            Permute4<double, simd::float64x4, 3, 3, 3, 3> wwww;
        };

        explicit Vector()
        {
        }

        explicit Vector(double s)
        : xyzw(simd::float64x4_set1(s))
        {
        }

        explicit Vector(int s)
        : xyzw(simd::float64x4_set1(double(s)))
        {
        }

        explicit Vector(double s0, double s1, double s2, double s3)
        : xyzw(simd::float64x4_set4(s0, s1, s2, s3))
        {
        }

        explicit Vector(const Vector<double, 2>& v, double s0, double s1)
        : xyzw(simd::float64x4_set4(v.x, v.y, s0, s1))
        {
        }

        explicit Vector(double s0, double s1, const Vector<double, 2>& v)
        : xyzw(simd::float64x4_set4(s0, s1, v.x, v.y))
        {
        }

        explicit Vector(double s0, const Vector<double, 2>& v, double s1)
        : xyzw(simd::float64x4_set4(s0, v.x, v.y, s1))
        {
        }

        explicit Vector(const Vector<double, 2>& v0, const Vector<double, 2>& v1)
        : xyzw(simd::combine(v0, v1))
        {
        }

        explicit Vector(const Vector<double, 3>& v, double s)
        : xyzw(simd::float64x4_set4(v.x, v.y, v.z, s))
        {
        }

        explicit Vector(double s, const Vector<double, 3>& v)
        : xyzw(simd::float64x4_set4(s, v.x, v.y, v.z))
        {
        }

        Vector(simd::float64x4 v)
        : xyzw(v)
        {
        }

        template <int X, int Y, int Z, int W>
        Vector(const Permute4<double, simd::float64x4, X, Y, Z, W>& p)
        {
            xyzw = p;
        }

        template <int X, int Y, int Z, int W>
        Vector& operator = (const Permute4<double, simd::float64x4, X, Y, Z, W>& p)
        {
            xyzw = p;
            return *this;
        }

        Vector& operator = (simd::float64x4 v)
        {
            xyzw = v;
            return *this;
        }

        Vector& operator = (double s)
        {
            xyzw = simd::float64x4_set1(s);
            return *this;
        }

        operator simd::float64x4 () const
        {
            return xyzw;
        }

        operator simd::float64x4 ()
        {
            return xyzw;
        }
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline double4 operator + (double4 v)
    {
        return v;
    }

    static inline double4 operator - (double4 v)
    {
        return simd::neg(v);
    }

    static inline double4& operator += (double4& a, double4 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline double4& operator += (double4& a, double b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline double4& operator -= (double4& a, double4 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline double4& operator -= (double4& a, double b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline double4& operator *= (double4& a, double4 b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline double4& operator *= (double4& a, double b)
    {
        a = simd::mul(a, b);
        return a;
    }

    static inline double4& operator /= (double4& a, double4 b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline double4& operator /= (double4& a, double b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline double4 operator + (double4 a, double4 b)
    {
        return simd::add(a, b);
    }

    static inline double4 operator + (double4 a, double b)
    {
        return simd::add(a, b);
    }

    static inline double4 operator + (double a, double4 b)
    {
        return simd::add(a, b);
    }

    static inline double4 operator - (double4 a, double4 b)
    {
        return simd::sub(a, b);
    }

    static inline double4 operator - (double4 a, double b)
    {
        return simd::sub(a, b);
    }

    static inline double4 operator - (double a, double4 b)
    {
        return simd::sub(a, b);
    }

    static inline double4 operator * (double4 a, double4 b)
    {
        return simd::mul(a, b);
    }

    static inline double4 operator * (double4 a, double b)
    {
        return simd::mul(a, b);
    }

    static inline double4 operator * (double a, double4 b)
    {
        return simd::mul(a, b);
    }

    static inline double4 operator / (double4 a, double4 b)
    {
        return simd::div(a, b);
    }

    static inline double4 operator / (double4 a, double b)
    {
        return simd::div(a, b);
    }

    static inline double4 operator / (double a, double4 b)
    {
        return simd::div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

#define MAKE_VECTOR_FUNCTION1(Name, SimdName) \
    static inline double4 Name(double4 a) { \
        return SimdName(a); \
    }

#define MAKE_VECTOR_FUNCTION2(Name, SimdName) \
    static inline double4 Name(double4 a, double4 b) { \
        return SimdName(a, b); \
    }

    MAKE_VECTOR_FUNCTION1(abs, simd::abs)
    MAKE_VECTOR_FUNCTION1(square, simd::square)
    MAKE_VECTOR_FUNCTION1(length, simd::length)
    MAKE_VECTOR_FUNCTION1(normalize, simd::normalize)
    MAKE_VECTOR_FUNCTION1(round, simd::round)
    MAKE_VECTOR_FUNCTION1(floor, simd::floor)
    MAKE_VECTOR_FUNCTION1(ceil, simd::ceil)
    MAKE_VECTOR_FUNCTION1(trunc, simd::trunc)
    MAKE_VECTOR_FUNCTION1(fract, simd::fract)
    MAKE_VECTOR_FUNCTION1(sign, simd::sign)
    MAKE_VECTOR_FUNCTION1(radians, simd::radians)
    MAKE_VECTOR_FUNCTION1(degrees, simd::degrees)
    MAKE_VECTOR_FUNCTION1(sqrt, simd::sqrt)
    MAKE_VECTOR_FUNCTION1(rsqrt, simd::rsqrt)

    MAKE_VECTOR_FUNCTION2(min, simd::min)
    MAKE_VECTOR_FUNCTION2(max, simd::max)
    MAKE_VECTOR_FUNCTION2(dot, simd::dot4)
    //MAKE_VECTOR_FUNCTION2(cross, simd::cross3)
    MAKE_VECTOR_FUNCTION2(mod, simd::mod)

#undef MAKE_VECTOR_FUNCTION1
#undef MAKE_VECTOR_FUNCTION2

    static inline double4 clamp(double4 a, double4 amin, double4 amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline double4 madd(double4 a, double4 b, double4 c)
    {
        return simd::madd(a, b, c);
    }

    static inline double4 msub(double4 a, double4 b, double4 c)
    {
        return simd::msub(a, b, c);
    }

    static inline double4 lerp(double4 a, double4 b, double factor)
    {
        return a + (b - a) * factor;
    }

    static inline double4 lerp(double4 a, double4 b, double4 factor)
    {
        return a + (b - a) * factor;
    }

    static inline double4 hmin(double4 v)
    {
        return simd::hmin(v);
    }

    static inline double4 hmax(double4 v)
    {
        return simd::hmax(v);
    }

    // ------------------------------------------------------------------
	// bitwise operators
    // ------------------------------------------------------------------

    static inline double4 nand(double4 a, double4 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline double4 operator & (double4 a, double4 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline double4 operator | (double4 a, double4 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline double4 operator ^ (double4 a, double4 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline double4 operator ~ (double4 a)
    {
        return simd::bitwise_not(a);
    }

    // ------------------------------------------------------------------
	// compare / select
    // ------------------------------------------------------------------

    static inline mask64x4 operator > (double4 a, double4 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask64x4 operator >= (double4 a, double4 b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask64x4 operator < (double4 a, double4 b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask64x4 operator <= (double4 a, double4 b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask64x4 operator == (double4 a, double4 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask64x4 operator != (double4 a, double4 b)
    {
        return simd::compare_neq(a, b);
    }

    static inline double4 select(mask64x4 mask, double4 a, double4 b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango

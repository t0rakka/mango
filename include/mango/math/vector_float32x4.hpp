/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"
#include "vector_float32x2.hpp"
#include "vector_float32x3.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // Vector<float, 4>
    // ------------------------------------------------------------------

    template <>
    struct Vector<float, 4>
    {
        using VectorType = simd::f32x4;
        using ScalarType = float;
        enum { VectorSize = 4 };

        union
        {
            simd::f32x4 m;

            LowAccessor<Vector<float, 2>, simd::f32x4> low;
            HighAccessor<Vector<float, 2>, simd::f32x4> high;

            ScalarAccessor<float, simd::f32x4, 0> x;
            ScalarAccessor<float, simd::f32x4, 1> y;
            ScalarAccessor<float, simd::f32x4, 2> z;
            ScalarAccessor<float, simd::f32x4, 3> w;

            ShuffleAccessor4x2<float, simd::f32x4, 0, 0> xx;
            ShuffleAccessor4x2<float, simd::f32x4, 1, 0> yx;
            ShuffleAccessor4x2<float, simd::f32x4, 2, 0> zx;
            ShuffleAccessor4x2<float, simd::f32x4, 3, 0> wx;
            ShuffleAccessor4x2<float, simd::f32x4, 0, 1> xy;
            ShuffleAccessor4x2<float, simd::f32x4, 1, 1> yy;
            ShuffleAccessor4x2<float, simd::f32x4, 2, 1> zy;
            ShuffleAccessor4x2<float, simd::f32x4, 3, 1> wy;
            ShuffleAccessor4x2<float, simd::f32x4, 0, 2> xz;
            ShuffleAccessor4x2<float, simd::f32x4, 1, 2> yz;
            ShuffleAccessor4x2<float, simd::f32x4, 2, 2> zz;
            ShuffleAccessor4x2<float, simd::f32x4, 3, 2> wz;
            ShuffleAccessor4x2<float, simd::f32x4, 0, 3> xw;
            ShuffleAccessor4x2<float, simd::f32x4, 1, 3> yw;
            ShuffleAccessor4x2<float, simd::f32x4, 2, 3> zw;
            ShuffleAccessor4x2<float, simd::f32x4, 3, 3> ww;

            ShuffleAccessor4x3<float, simd::f32x4, 0, 0, 0> xxx;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 0, 0> yxx;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 0, 0> zxx;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 0, 0> wxx;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 1, 0> xyx;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 1, 0> yyx;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 1, 0> zyx;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 1, 0> wyx;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 2, 0> xzx;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 2, 0> yzx;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 2, 0> zzx;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 2, 0> wzx;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 3, 0> xwx;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 3, 0> ywx;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 3, 0> zwx;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 3, 0> wwx;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 0, 1> xxy;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 0, 1> yxy;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 0, 1> zxy;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 0, 1> wxy;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 1, 1> xyy;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 1, 1> yyy;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 1, 1> zyy;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 1, 1> wyy;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 2, 1> xzy;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 2, 1> yzy;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 2, 1> zzy;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 2, 1> wzy;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 3, 1> xwy;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 3, 1> ywy;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 3, 1> zwy;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 3, 1> wwy;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 0, 2> xxz;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 0, 2> yxz;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 0, 2> zxz;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 0, 2> wxz;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 1, 2> xyz;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 1, 2> yyz;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 1, 2> zyz;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 1, 2> wyz;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 2, 2> xzz;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 2, 2> yzz;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 2, 2> zzz;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 2, 2> wzz;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 3, 2> xwz;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 3, 2> ywz;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 3, 2> zwz;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 3, 2> wwz;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 0, 3> xxw;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 0, 3> yxw;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 0, 3> zxw;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 0, 3> wxw;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 1, 3> xyw;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 1, 3> yyw;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 1, 3> zyw;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 1, 3> wyw;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 2, 3> xzw;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 2, 3> yzw;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 2, 3> zzw;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 2, 3> wzw;
            ShuffleAccessor4x3<float, simd::f32x4, 0, 3, 3> xww;
            ShuffleAccessor4x3<float, simd::f32x4, 1, 3, 3> yww;
            ShuffleAccessor4x3<float, simd::f32x4, 2, 3, 3> zww;
            ShuffleAccessor4x3<float, simd::f32x4, 3, 3, 3> www;

            ShuffleAccessor4<float, simd::f32x4, 0, 0, 0, 0> xxxx;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 0, 0> yxxx;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 0, 0> zxxx;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 0, 0> wxxx;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 0, 0> xyxx;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 0, 0> yyxx;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 0, 0> zyxx;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 0, 0> wyxx;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 0, 0> xzxx;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 0, 0> yzxx;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 0, 0> zzxx;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 0, 0> wzxx;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 0, 0> xwxx;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 0, 0> ywxx;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 0, 0> zwxx;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 0, 0> wwxx;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 1, 0> xxyx;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 1, 0> yxyx;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 1, 0> zxyx;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 1, 0> wxyx;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 1, 0> xyyx;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 1, 0> yyyx;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 1, 0> zyyx;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 1, 0> wyyx;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 1, 0> xzyx;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 1, 0> yzyx;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 1, 0> zzyx;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 1, 0> wzyx;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 1, 0> xwyx;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 1, 0> ywyx;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 1, 0> zwyx;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 1, 0> wwyx;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 2, 0> xxzx;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 2, 0> yxzx;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 2, 0> zxzx;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 2, 0> wxzx;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 2, 0> xyzx;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 2, 0> yyzx;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 2, 0> zyzx;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 2, 0> wyzx;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 2, 0> xzzx;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 2, 0> yzzx;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 2, 0> zzzx;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 2, 0> wzzx;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 2, 0> xwzx;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 2, 0> ywzx;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 2, 0> zwzx;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 2, 0> wwzx;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 3, 0> xxwx;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 3, 0> yxwx;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 3, 0> zxwx;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 3, 0> wxwx;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 3, 0> xywx;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 3, 0> yywx;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 3, 0> zywx;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 3, 0> wywx;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 3, 0> xzwx;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 3, 0> yzwx;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 3, 0> zzwx;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 3, 0> wzwx;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 3, 0> xwwx;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 3, 0> ywwx;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 3, 0> zwwx;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 3, 0> wwwx;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 0, 1> xxxy;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 0, 1> yxxy;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 0, 1> zxxy;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 0, 1> wxxy;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 0, 1> xyxy;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 0, 1> yyxy;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 0, 1> zyxy;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 0, 1> wyxy;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 0, 1> xzxy;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 0, 1> yzxy;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 0, 1> zzxy;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 0, 1> wzxy;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 0, 1> xwxy;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 0, 1> ywxy;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 0, 1> zwxy;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 0, 1> wwxy;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 1, 1> xxyy;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 1, 1> yxyy;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 1, 1> zxyy;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 1, 1> wxyy;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 1, 1> xyyy;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 1, 1> yyyy;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 1, 1> zyyy;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 1, 1> wyyy;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 1, 1> xzyy;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 1, 1> yzyy;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 1, 1> zzyy;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 1, 1> wzyy;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 1, 1> xwyy;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 1, 1> ywyy;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 1, 1> zwyy;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 1, 1> wwyy;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 2, 1> xxzy;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 2, 1> yxzy;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 2, 1> zxzy;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 2, 1> wxzy;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 2, 1> xyzy;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 2, 1> yyzy;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 2, 1> zyzy;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 2, 1> wyzy;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 2, 1> xzzy;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 2, 1> yzzy;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 2, 1> zzzy;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 2, 1> wzzy;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 2, 1> xwzy;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 2, 1> ywzy;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 2, 1> zwzy;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 2, 1> wwzy;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 3, 1> xxwy;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 3, 1> yxwy;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 3, 1> zxwy;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 3, 1> wxwy;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 3, 1> xywy;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 3, 1> yywy;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 3, 1> zywy;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 3, 1> wywy;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 3, 1> xzwy;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 3, 1> yzwy;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 3, 1> zzwy;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 3, 1> wzwy;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 3, 1> xwwy;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 3, 1> ywwy;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 3, 1> zwwy;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 3, 1> wwwy;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 0, 2> xxxz;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 0, 2> yxxz;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 0, 2> zxxz;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 0, 2> wxxz;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 0, 2> xyxz;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 0, 2> yyxz;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 0, 2> zyxz;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 0, 2> wyxz;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 0, 2> xzxz;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 0, 2> yzxz;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 0, 2> zzxz;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 0, 2> wzxz;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 0, 2> xwxz;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 0, 2> ywxz;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 0, 2> zwxz;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 0, 2> wwxz;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 1, 2> xxyz;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 1, 2> yxyz;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 1, 2> zxyz;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 1, 2> wxyz;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 1, 2> xyyz;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 1, 2> yyyz;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 1, 2> zyyz;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 1, 2> wyyz;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 1, 2> xzyz;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 1, 2> yzyz;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 1, 2> zzyz;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 1, 2> wzyz;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 1, 2> xwyz;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 1, 2> ywyz;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 1, 2> zwyz;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 1, 2> wwyz;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 2, 2> xxzz;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 2, 2> yxzz;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 2, 2> zxzz;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 2, 2> wxzz;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 2, 2> xyzz;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 2, 2> yyzz;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 2, 2> zyzz;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 2, 2> wyzz;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 2, 2> xzzz;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 2, 2> yzzz;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 2, 2> zzzz;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 2, 2> wzzz;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 2, 2> xwzz;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 2, 2> ywzz;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 2, 2> zwzz;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 2, 2> wwzz;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 3, 2> xxwz;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 3, 2> yxwz;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 3, 2> zxwz;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 3, 2> wxwz;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 3, 2> xywz;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 3, 2> yywz;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 3, 2> zywz;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 3, 2> wywz;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 3, 2> xzwz;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 3, 2> yzwz;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 3, 2> zzwz;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 3, 2> wzwz;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 3, 2> xwwz;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 3, 2> ywwz;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 3, 2> zwwz;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 3, 2> wwwz;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 0, 3> xxxw;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 0, 3> yxxw;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 0, 3> zxxw;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 0, 3> wxxw;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 0, 3> xyxw;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 0, 3> yyxw;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 0, 3> zyxw;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 0, 3> wyxw;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 0, 3> xzxw;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 0, 3> yzxw;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 0, 3> zzxw;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 0, 3> wzxw;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 0, 3> xwxw;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 0, 3> ywxw;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 0, 3> zwxw;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 0, 3> wwxw;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 1, 3> xxyw;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 1, 3> yxyw;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 1, 3> zxyw;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 1, 3> wxyw;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 1, 3> xyyw;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 1, 3> yyyw;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 1, 3> zyyw;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 1, 3> wyyw;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 1, 3> xzyw;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 1, 3> yzyw;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 1, 3> zzyw;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 1, 3> wzyw;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 1, 3> xwyw;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 1, 3> ywyw;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 1, 3> zwyw;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 1, 3> wwyw;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 2, 3> xxzw;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 2, 3> yxzw;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 2, 3> zxzw;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 2, 3> wxzw;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 2, 3> xyzw;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 2, 3> yyzw;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 2, 3> zyzw;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 2, 3> wyzw;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 2, 3> xzzw;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 2, 3> yzzw;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 2, 3> zzzw;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 2, 3> wzzw;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 2, 3> xwzw;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 2, 3> ywzw;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 2, 3> zwzw;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 2, 3> wwzw;
            ShuffleAccessor4<float, simd::f32x4, 0, 0, 3, 3> xxww;
            ShuffleAccessor4<float, simd::f32x4, 1, 0, 3, 3> yxww;
            ShuffleAccessor4<float, simd::f32x4, 2, 0, 3, 3> zxww;
            ShuffleAccessor4<float, simd::f32x4, 3, 0, 3, 3> wxww;
            ShuffleAccessor4<float, simd::f32x4, 0, 1, 3, 3> xyww;
            ShuffleAccessor4<float, simd::f32x4, 1, 1, 3, 3> yyww;
            ShuffleAccessor4<float, simd::f32x4, 2, 1, 3, 3> zyww;
            ShuffleAccessor4<float, simd::f32x4, 3, 1, 3, 3> wyww;
            ShuffleAccessor4<float, simd::f32x4, 0, 2, 3, 3> xzww;
            ShuffleAccessor4<float, simd::f32x4, 1, 2, 3, 3> yzww;
            ShuffleAccessor4<float, simd::f32x4, 2, 2, 3, 3> zzww;
            ShuffleAccessor4<float, simd::f32x4, 3, 2, 3, 3> wzww;
            ShuffleAccessor4<float, simd::f32x4, 0, 3, 3, 3> xwww;
            ShuffleAccessor4<float, simd::f32x4, 1, 3, 3, 3> ywww;
            ShuffleAccessor4<float, simd::f32x4, 2, 3, 3, 3> zwww;
            ShuffleAccessor4<float, simd::f32x4, 3, 3, 3, 3> wwww;
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
            : m(simd::f32x4_set1(s))
        {
        }

        explicit Vector(float x, float y, float z, float w)
            : m(simd::f32x4_set4(x, y, z, w))
        {
        }

        explicit Vector(const Vector<float, 2>& v, float z, float w)
            : m(simd::f32x4_set4(v.x, v.y, z, w))
        {
        }

        explicit Vector(float x, float y, const Vector<float, 2>& v)
            : m(simd::f32x4_set4(x, y, v.x, v.y))
        {
        }

        explicit Vector(float x, const Vector<float, 2>& v, float w)
            :m(simd::f32x4_set4(x, v.x, v.y, w))
        {
        }

        explicit Vector(const Vector<float, 2>& xy, const Vector<float, 2>& zw)
            : m(simd::f32x4_set4(xy.x, xy.y, zw.x, zw.y))
        {
        }

        explicit Vector(const Vector<float, 3>& v, float w)
            : m(simd::f32x4_set4(v.x, v.y, v.z, w))
        {
        }

        explicit Vector(float x, const Vector<float, 3>& v)
            : m(simd::f32x4_set4(x, v.x, v.y, v.z))
        {
        }

        Vector(simd::f32x4 v)
            : m(v)
        {
        }

        template <typename T, int I>
        Vector& operator = (const ScalarAccessor<ScalarType, T, I>& accessor)
        {
            *this = ScalarType(accessor);
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
            m = v.m;
            return *this;
        }

        Vector& operator = (simd::f32x4 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (float s)
        {
            m = simd::f32x4_set1(s);
            return *this;
        }

        operator simd::f32x4 () const
        {
            return m;
        }

#ifdef float128_is_hardware_vector
        operator simd::f32x4::vector () const
        {
            return m.data;
        }
#endif

        u32 pack() const
        {
            const simd::s32x4 temp = simd::convert<simd::s32x4>(m);
            return simd::pack(temp);
        }

        void unpack(u32 a)
        {
            const simd::s32x4 temp = simd::unpack(a);
            m = simd::convert<simd::f32x4>(temp);
        }
    };

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline Vector<float, 4> operator + (Vector<float, 4> v)
    {
        return v;
    }

    static inline Vector<float, 4> operator - (Vector<float, 4> v)
    {
        return simd::neg(v);
    }

    static inline Vector<float, 4>& operator += (Vector<float, 4>& a, Vector<float, 4> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<float, 4>& operator -= (Vector<float, 4>& a, Vector<float, 4> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<float, 4>& operator *= (Vector<float, 4>& a, Vector<float, 4> b)
    {
        a = simd::mul(a, b);
        return a;
    }

    template <typename VT, int I>
    static inline Vector<float, 4>& operator /= (Vector<float, 4>& a, ScalarAccessor<float, VT, I> b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<float, 4>& operator /= (Vector<float, 4>& a, Vector<float, 4> b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<float, 4>& operator /= (Vector<float, 4>& a, float b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<float, 4> operator + (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<float, 4> operator - (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<float, 4> operator * (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::mul(a, b);
    }

    template <typename VT, int I>
    static inline Vector<float, 4> operator / (Vector<float, 4> a, ScalarAccessor<float, VT, I> b)
    {
        return simd::div(a, b);
    }

    static inline Vector<float, 4> operator / (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::div(a, b);
    }

    static inline Vector<float, 4> operator / (Vector<float, 4> a, float b)
    {
        return simd::div(a, b);
    }

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<float, 4> abs(Vector<float, 4> a)
    {
        return simd::abs(a);
    }

    static inline float square(Vector<float, 4> a)
    {
        return simd::square(a);
    }

    static inline float length(Vector<float, 4> a)
    {
        return simd::length(a);
    }

    static inline Vector<float, 4> normalize(Vector<float, 4> a)
    {
        return simd::normalize(a);
    }

    static inline Vector<float, 4> round(Vector<float, 4> a)
    {
        return simd::round(a);
    }

    static inline Vector<float, 4> floor(Vector<float, 4> a)
    {
        return simd::floor(a);
    }

    static inline Vector<float, 4> ceil(Vector<float, 4> a)
    {
        return simd::ceil(a);
    }

    static inline Vector<float, 4> trunc(Vector<float, 4> a)
    {
        return simd::trunc(a);
    }

    static inline Vector<float, 4> fract(Vector<float, 4> a)
    {
        return simd::fract(a);
    }

    static inline Vector<float, 4> sign(Vector<float, 4> a)
    {
        return simd::sign(a);
    }

    static inline Vector<float, 4> radians(Vector<float, 4> a)
    {
        return simd::radians(a);
    }

    static inline Vector<float, 4> degrees(Vector<float, 4> a)
    {
        return simd::degrees(a);
    }

    static inline Vector<float, 4> sqrt(Vector<float, 4> a)
    {
        return simd::sqrt(a);
    }

    static inline Vector<float, 4> rsqrt(Vector<float, 4> a)
    {
        return simd::rsqrt(a);
    }

    static inline Vector<float, 4> rcp(Vector<float, 4> a)
    {
        return simd::rcp(a);
    }

    static inline Vector<float, 4> min(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<float, 4> max(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::max(a, b);
    }

    static inline float dot(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::dot4(a, b);
    }

    static inline Vector<float, 4> cross(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::cross3(a, b);
    }

    static inline Vector<float, 4> mod(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::mod(a, b);
    }

    static inline Vector<float, 4> clamp(Vector<float, 4> a, Vector<float, 4> amin, Vector<float, 4> amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline Vector<float, 4> madd(Vector<float, 4> a, Vector<float, 4> b, Vector<float, 4> c)
    {
        return simd::madd(a, b, c);
    }

    static inline Vector<float, 4> msub(Vector<float, 4> a, Vector<float, 4> b, Vector<float, 4> c)
    {
        return simd::msub(a, b, c);
    }

    static inline Vector<float, 4> lerp(Vector<float, 4> a, Vector<float, 4> b, Vector<float, 4> factor)
    {
        return a + (b - a) * factor;
    }

    static inline Vector<float, 4> hmin(Vector<float, 4> v)
    {
        return simd::hmin(v);
    }

    static inline Vector<float, 4> hmax(Vector<float, 4> v)
    {
        return simd::hmax(v);
    }

    template <int x, int y, int z, int w>
    static inline Vector<float, 4> shuffle(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::shuffle<x, y, z, w>(a, b);
    }

    static inline Vector<float, 4> movelh(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::movelh(a, b);
    }
    
    static inline Vector<float, 4> movehl(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::movehl(a, b);
    }

    // ------------------------------------------------------------------
    // trigonometric functions
    // ------------------------------------------------------------------

    Vector<float, 4> sin(Vector<float, 4> a);
    Vector<float, 4> cos(Vector<float, 4> a);
    Vector<float, 4> tan(Vector<float, 4> a);
    Vector<float, 4> exp(Vector<float, 4> a);
    Vector<float, 4> exp2(Vector<float, 4> a);
    Vector<float, 4> log(Vector<float, 4> a);
    Vector<float, 4> log2(Vector<float, 4> a);
    Vector<float, 4> asin(Vector<float, 4> a);
    Vector<float, 4> acos(Vector<float, 4> a);
    Vector<float, 4> atan(Vector<float, 4> a);
    Vector<float, 4> atan2(Vector<float, 4> a, Vector<float, 4> b);
    Vector<float, 4> pow(Vector<float, 4> a, Vector<float, 4> b);

    // ------------------------------------------------------------------
	// bitwise operators
    // ------------------------------------------------------------------

    static inline Vector<float, 4> nand(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<float, 4> operator & (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<float, 4> operator | (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<float, 4> operator ^ (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<float, 4> operator ~ (Vector<float, 4> a)
    {
        return simd::bitwise_not(a);
    }

    // ------------------------------------------------------------------
    // compare / select
    // ------------------------------------------------------------------

    static inline mask32x4 operator > (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask32x4 operator >= (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask32x4 operator < (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask32x4 operator <= (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask32x4 operator == (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask32x4 operator != (Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<float, 4> select(mask32x4 mask, Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango

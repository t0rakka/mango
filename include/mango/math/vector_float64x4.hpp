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
    struct Vector<double, 4> : simd::VectorBase<double, 4>
    {
        union
        {
            simd::float64x4 xyzw;

            simd::LowAccessor<Vector<double, 2>, simd::float64x4> low;
            simd::HighAccessor<Vector<double, 2>, simd::float64x4> high;

            simd::ScalarAccessor<double, simd::float64x4, 0> x;
            simd::ScalarAccessor<double, simd::float64x4, 1> y;
            simd::ScalarAccessor<double, simd::float64x4, 2> z;
            simd::ScalarAccessor<double, simd::float64x4, 3> w;

            simd::Permute4x2<double, simd::float64x4, 0, 0> xx;
            simd::Permute4x2<double, simd::float64x4, 1, 0> yx;
            simd::Permute4x2<double, simd::float64x4, 2, 0> zx;
            simd::Permute4x2<double, simd::float64x4, 3, 0> wx;
            simd::Permute4x2<double, simd::float64x4, 0, 1> xy;
            simd::Permute4x2<double, simd::float64x4, 1, 1> yy;
            simd::Permute4x2<double, simd::float64x4, 2, 1> zy;
            simd::Permute4x2<double, simd::float64x4, 3, 1> wy;
            simd::Permute4x2<double, simd::float64x4, 0, 2> xz;
            simd::Permute4x2<double, simd::float64x4, 1, 2> yz;
            simd::Permute4x2<double, simd::float64x4, 2, 2> zz;
            simd::Permute4x2<double, simd::float64x4, 3, 2> wz;
            simd::Permute4x2<double, simd::float64x4, 0, 3> xw;
            simd::Permute4x2<double, simd::float64x4, 1, 3> yw;
            simd::Permute4x2<double, simd::float64x4, 2, 3> zw;
            simd::Permute4x2<double, simd::float64x4, 3, 3> ww;

            simd::Permute4x3<double, simd::float64x4, 0, 0, 0> xxx;
            simd::Permute4x3<double, simd::float64x4, 1, 0, 0> yxx;
            simd::Permute4x3<double, simd::float64x4, 2, 0, 0> zxx;
            simd::Permute4x3<double, simd::float64x4, 3, 0, 0> wxx;
            simd::Permute4x3<double, simd::float64x4, 0, 1, 0> xyx;
            simd::Permute4x3<double, simd::float64x4, 1, 1, 0> yyx;
            simd::Permute4x3<double, simd::float64x4, 2, 1, 0> zyx;
            simd::Permute4x3<double, simd::float64x4, 3, 1, 0> wyx;
            simd::Permute4x3<double, simd::float64x4, 0, 2, 0> xzx;
            simd::Permute4x3<double, simd::float64x4, 1, 2, 0> yzx;
            simd::Permute4x3<double, simd::float64x4, 2, 2, 0> zzx;
            simd::Permute4x3<double, simd::float64x4, 3, 2, 0> wzx;
            simd::Permute4x3<double, simd::float64x4, 0, 3, 0> xwx;
            simd::Permute4x3<double, simd::float64x4, 1, 3, 0> ywx;
            simd::Permute4x3<double, simd::float64x4, 2, 3, 0> zwx;
            simd::Permute4x3<double, simd::float64x4, 3, 3, 0> wwx;
            simd::Permute4x3<double, simd::float64x4, 0, 0, 1> xxy;
            simd::Permute4x3<double, simd::float64x4, 1, 0, 1> yxy;
            simd::Permute4x3<double, simd::float64x4, 2, 0, 1> zxy;
            simd::Permute4x3<double, simd::float64x4, 3, 0, 1> wxy;
            simd::Permute4x3<double, simd::float64x4, 0, 1, 1> xyy;
            simd::Permute4x3<double, simd::float64x4, 1, 1, 1> yyy;
            simd::Permute4x3<double, simd::float64x4, 2, 1, 1> zyy;
            simd::Permute4x3<double, simd::float64x4, 3, 1, 1> wyy;
            simd::Permute4x3<double, simd::float64x4, 0, 2, 1> xzy;
            simd::Permute4x3<double, simd::float64x4, 1, 2, 1> yzy;
            simd::Permute4x3<double, simd::float64x4, 2, 2, 1> zzy;
            simd::Permute4x3<double, simd::float64x4, 3, 2, 1> wzy;
            simd::Permute4x3<double, simd::float64x4, 0, 3, 1> xwy;
            simd::Permute4x3<double, simd::float64x4, 1, 3, 1> ywy;
            simd::Permute4x3<double, simd::float64x4, 2, 3, 1> zwy;
            simd::Permute4x3<double, simd::float64x4, 3, 3, 1> wwy;
            simd::Permute4x3<double, simd::float64x4, 0, 0, 2> xxz;
            simd::Permute4x3<double, simd::float64x4, 1, 0, 2> yxz;
            simd::Permute4x3<double, simd::float64x4, 2, 0, 2> zxz;
            simd::Permute4x3<double, simd::float64x4, 3, 0, 2> wxz;
            simd::Permute4x3<double, simd::float64x4, 0, 1, 2> xyz;
            simd::Permute4x3<double, simd::float64x4, 1, 1, 2> yyz;
            simd::Permute4x3<double, simd::float64x4, 2, 1, 2> zyz;
            simd::Permute4x3<double, simd::float64x4, 3, 1, 2> wyz;
            simd::Permute4x3<double, simd::float64x4, 0, 2, 2> xzz;
            simd::Permute4x3<double, simd::float64x4, 1, 2, 2> yzz;
            simd::Permute4x3<double, simd::float64x4, 2, 2, 2> zzz;
            simd::Permute4x3<double, simd::float64x4, 3, 2, 2> wzz;
            simd::Permute4x3<double, simd::float64x4, 0, 3, 2> xwz;
            simd::Permute4x3<double, simd::float64x4, 1, 3, 2> ywz;
            simd::Permute4x3<double, simd::float64x4, 2, 3, 2> zwz;
            simd::Permute4x3<double, simd::float64x4, 3, 3, 2> wwz;
            simd::Permute4x3<double, simd::float64x4, 0, 0, 3> xxw;
            simd::Permute4x3<double, simd::float64x4, 1, 0, 3> yxw;
            simd::Permute4x3<double, simd::float64x4, 2, 0, 3> zxw;
            simd::Permute4x3<double, simd::float64x4, 3, 0, 3> wxw;
            simd::Permute4x3<double, simd::float64x4, 0, 1, 3> xyw;
            simd::Permute4x3<double, simd::float64x4, 1, 1, 3> yyw;
            simd::Permute4x3<double, simd::float64x4, 2, 1, 3> zyw;
            simd::Permute4x3<double, simd::float64x4, 3, 1, 3> wyw;
            simd::Permute4x3<double, simd::float64x4, 0, 2, 3> xzw;
            simd::Permute4x3<double, simd::float64x4, 1, 2, 3> yzw;
            simd::Permute4x3<double, simd::float64x4, 2, 2, 3> zzw;
            simd::Permute4x3<double, simd::float64x4, 3, 2, 3> wzw;
            simd::Permute4x3<double, simd::float64x4, 0, 3, 3> xww;
            simd::Permute4x3<double, simd::float64x4, 1, 3, 3> yww;
            simd::Permute4x3<double, simd::float64x4, 2, 3, 3> zww;
            simd::Permute4x3<double, simd::float64x4, 3, 3, 3> www;

            simd::Permute4<double, simd::float64x4, 0, 0, 0, 0> xxxx;
            simd::Permute4<double, simd::float64x4, 1, 0, 0, 0> yxxx;
            simd::Permute4<double, simd::float64x4, 2, 0, 0, 0> zxxx;
            simd::Permute4<double, simd::float64x4, 3, 0, 0, 0> wxxx;
            simd::Permute4<double, simd::float64x4, 0, 1, 0, 0> xyxx;
            simd::Permute4<double, simd::float64x4, 1, 1, 0, 0> yyxx;
            simd::Permute4<double, simd::float64x4, 2, 1, 0, 0> zyxx;
            simd::Permute4<double, simd::float64x4, 3, 1, 0, 0> wyxx;
            simd::Permute4<double, simd::float64x4, 0, 2, 0, 0> xzxx;
            simd::Permute4<double, simd::float64x4, 1, 2, 0, 0> yzxx;
            simd::Permute4<double, simd::float64x4, 2, 2, 0, 0> zzxx;
            simd::Permute4<double, simd::float64x4, 3, 2, 0, 0> wzxx;
            simd::Permute4<double, simd::float64x4, 0, 3, 0, 0> xwxx;
            simd::Permute4<double, simd::float64x4, 1, 3, 0, 0> ywxx;
            simd::Permute4<double, simd::float64x4, 2, 3, 0, 0> zwxx;
            simd::Permute4<double, simd::float64x4, 3, 3, 0, 0> wwxx;
            simd::Permute4<double, simd::float64x4, 0, 0, 1, 0> xxyx;
            simd::Permute4<double, simd::float64x4, 1, 0, 1, 0> yxyx;
            simd::Permute4<double, simd::float64x4, 2, 0, 1, 0> zxyx;
            simd::Permute4<double, simd::float64x4, 3, 0, 1, 0> wxyx;
            simd::Permute4<double, simd::float64x4, 0, 1, 1, 0> xyyx;
            simd::Permute4<double, simd::float64x4, 1, 1, 1, 0> yyyx;
            simd::Permute4<double, simd::float64x4, 2, 1, 1, 0> zyyx;
            simd::Permute4<double, simd::float64x4, 3, 1, 1, 0> wyyx;
            simd::Permute4<double, simd::float64x4, 0, 2, 1, 0> xzyx;
            simd::Permute4<double, simd::float64x4, 1, 2, 1, 0> yzyx;
            simd::Permute4<double, simd::float64x4, 2, 2, 1, 0> zzyx;
            simd::Permute4<double, simd::float64x4, 3, 2, 1, 0> wzyx;
            simd::Permute4<double, simd::float64x4, 0, 3, 1, 0> xwyx;
            simd::Permute4<double, simd::float64x4, 1, 3, 1, 0> ywyx;
            simd::Permute4<double, simd::float64x4, 2, 3, 1, 0> zwyx;
            simd::Permute4<double, simd::float64x4, 3, 3, 1, 0> wwyx;
            simd::Permute4<double, simd::float64x4, 0, 0, 2, 0> xxzx;
            simd::Permute4<double, simd::float64x4, 1, 0, 2, 0> yxzx;
            simd::Permute4<double, simd::float64x4, 2, 0, 2, 0> zxzx;
            simd::Permute4<double, simd::float64x4, 3, 0, 2, 0> wxzx;
            simd::Permute4<double, simd::float64x4, 0, 1, 2, 0> xyzx;
            simd::Permute4<double, simd::float64x4, 1, 1, 2, 0> yyzx;
            simd::Permute4<double, simd::float64x4, 2, 1, 2, 0> zyzx;
            simd::Permute4<double, simd::float64x4, 3, 1, 2, 0> wyzx;
            simd::Permute4<double, simd::float64x4, 0, 2, 2, 0> xzzx;
            simd::Permute4<double, simd::float64x4, 1, 2, 2, 0> yzzx;
            simd::Permute4<double, simd::float64x4, 2, 2, 2, 0> zzzx;
            simd::Permute4<double, simd::float64x4, 3, 2, 2, 0> wzzx;
            simd::Permute4<double, simd::float64x4, 0, 3, 2, 0> xwzx;
            simd::Permute4<double, simd::float64x4, 1, 3, 2, 0> ywzx;
            simd::Permute4<double, simd::float64x4, 2, 3, 2, 0> zwzx;
            simd::Permute4<double, simd::float64x4, 3, 3, 2, 0> wwzx;
            simd::Permute4<double, simd::float64x4, 0, 0, 3, 0> xxwx;
            simd::Permute4<double, simd::float64x4, 1, 0, 3, 0> yxwx;
            simd::Permute4<double, simd::float64x4, 2, 0, 3, 0> zxwx;
            simd::Permute4<double, simd::float64x4, 3, 0, 3, 0> wxwx;
            simd::Permute4<double, simd::float64x4, 0, 1, 3, 0> xywx;
            simd::Permute4<double, simd::float64x4, 1, 1, 3, 0> yywx;
            simd::Permute4<double, simd::float64x4, 2, 1, 3, 0> zywx;
            simd::Permute4<double, simd::float64x4, 3, 1, 3, 0> wywx;
            simd::Permute4<double, simd::float64x4, 0, 2, 3, 0> xzwx;
            simd::Permute4<double, simd::float64x4, 1, 2, 3, 0> yzwx;
            simd::Permute4<double, simd::float64x4, 2, 2, 3, 0> zzwx;
            simd::Permute4<double, simd::float64x4, 3, 2, 3, 0> wzwx;
            simd::Permute4<double, simd::float64x4, 0, 3, 3, 0> xwwx;
            simd::Permute4<double, simd::float64x4, 1, 3, 3, 0> ywwx;
            simd::Permute4<double, simd::float64x4, 2, 3, 3, 0> zwwx;
            simd::Permute4<double, simd::float64x4, 3, 3, 3, 0> wwwx;
            simd::Permute4<double, simd::float64x4, 0, 0, 0, 1> xxxy;
            simd::Permute4<double, simd::float64x4, 1, 0, 0, 1> yxxy;
            simd::Permute4<double, simd::float64x4, 2, 0, 0, 1> zxxy;
            simd::Permute4<double, simd::float64x4, 3, 0, 0, 1> wxxy;
            simd::Permute4<double, simd::float64x4, 0, 1, 0, 1> xyxy;
            simd::Permute4<double, simd::float64x4, 1, 1, 0, 1> yyxy;
            simd::Permute4<double, simd::float64x4, 2, 1, 0, 1> zyxy;
            simd::Permute4<double, simd::float64x4, 3, 1, 0, 1> wyxy;
            simd::Permute4<double, simd::float64x4, 0, 2, 0, 1> xzxy;
            simd::Permute4<double, simd::float64x4, 1, 2, 0, 1> yzxy;
            simd::Permute4<double, simd::float64x4, 2, 2, 0, 1> zzxy;
            simd::Permute4<double, simd::float64x4, 3, 2, 0, 1> wzxy;
            simd::Permute4<double, simd::float64x4, 0, 3, 0, 1> xwxy;
            simd::Permute4<double, simd::float64x4, 1, 3, 0, 1> ywxy;
            simd::Permute4<double, simd::float64x4, 2, 3, 0, 1> zwxy;
            simd::Permute4<double, simd::float64x4, 3, 3, 0, 1> wwxy;
            simd::Permute4<double, simd::float64x4, 0, 0, 1, 1> xxyy;
            simd::Permute4<double, simd::float64x4, 1, 0, 1, 1> yxyy;
            simd::Permute4<double, simd::float64x4, 2, 0, 1, 1> zxyy;
            simd::Permute4<double, simd::float64x4, 3, 0, 1, 1> wxyy;
            simd::Permute4<double, simd::float64x4, 0, 1, 1, 1> xyyy;
            simd::Permute4<double, simd::float64x4, 1, 1, 1, 1> yyyy;
            simd::Permute4<double, simd::float64x4, 2, 1, 1, 1> zyyy;
            simd::Permute4<double, simd::float64x4, 3, 1, 1, 1> wyyy;
            simd::Permute4<double, simd::float64x4, 0, 2, 1, 1> xzyy;
            simd::Permute4<double, simd::float64x4, 1, 2, 1, 1> yzyy;
            simd::Permute4<double, simd::float64x4, 2, 2, 1, 1> zzyy;
            simd::Permute4<double, simd::float64x4, 3, 2, 1, 1> wzyy;
            simd::Permute4<double, simd::float64x4, 0, 3, 1, 1> xwyy;
            simd::Permute4<double, simd::float64x4, 1, 3, 1, 1> ywyy;
            simd::Permute4<double, simd::float64x4, 2, 3, 1, 1> zwyy;
            simd::Permute4<double, simd::float64x4, 3, 3, 1, 1> wwyy;
            simd::Permute4<double, simd::float64x4, 0, 0, 2, 1> xxzy;
            simd::Permute4<double, simd::float64x4, 1, 0, 2, 1> yxzy;
            simd::Permute4<double, simd::float64x4, 2, 0, 2, 1> zxzy;
            simd::Permute4<double, simd::float64x4, 3, 0, 2, 1> wxzy;
            simd::Permute4<double, simd::float64x4, 0, 1, 2, 1> xyzy;
            simd::Permute4<double, simd::float64x4, 1, 1, 2, 1> yyzy;
            simd::Permute4<double, simd::float64x4, 2, 1, 2, 1> zyzy;
            simd::Permute4<double, simd::float64x4, 3, 1, 2, 1> wyzy;
            simd::Permute4<double, simd::float64x4, 0, 2, 2, 1> xzzy;
            simd::Permute4<double, simd::float64x4, 1, 2, 2, 1> yzzy;
            simd::Permute4<double, simd::float64x4, 2, 2, 2, 1> zzzy;
            simd::Permute4<double, simd::float64x4, 3, 2, 2, 1> wzzy;
            simd::Permute4<double, simd::float64x4, 0, 3, 2, 1> xwzy;
            simd::Permute4<double, simd::float64x4, 1, 3, 2, 1> ywzy;
            simd::Permute4<double, simd::float64x4, 2, 3, 2, 1> zwzy;
            simd::Permute4<double, simd::float64x4, 3, 3, 2, 1> wwzy;
            simd::Permute4<double, simd::float64x4, 0, 0, 3, 1> xxwy;
            simd::Permute4<double, simd::float64x4, 1, 0, 3, 1> yxwy;
            simd::Permute4<double, simd::float64x4, 2, 0, 3, 1> zxwy;
            simd::Permute4<double, simd::float64x4, 3, 0, 3, 1> wxwy;
            simd::Permute4<double, simd::float64x4, 0, 1, 3, 1> xywy;
            simd::Permute4<double, simd::float64x4, 1, 1, 3, 1> yywy;
            simd::Permute4<double, simd::float64x4, 2, 1, 3, 1> zywy;
            simd::Permute4<double, simd::float64x4, 3, 1, 3, 1> wywy;
            simd::Permute4<double, simd::float64x4, 0, 2, 3, 1> xzwy;
            simd::Permute4<double, simd::float64x4, 1, 2, 3, 1> yzwy;
            simd::Permute4<double, simd::float64x4, 2, 2, 3, 1> zzwy;
            simd::Permute4<double, simd::float64x4, 3, 2, 3, 1> wzwy;
            simd::Permute4<double, simd::float64x4, 0, 3, 3, 1> xwwy;
            simd::Permute4<double, simd::float64x4, 1, 3, 3, 1> ywwy;
            simd::Permute4<double, simd::float64x4, 2, 3, 3, 1> zwwy;
            simd::Permute4<double, simd::float64x4, 3, 3, 3, 1> wwwy;
            simd::Permute4<double, simd::float64x4, 0, 0, 0, 2> xxxz;
            simd::Permute4<double, simd::float64x4, 1, 0, 0, 2> yxxz;
            simd::Permute4<double, simd::float64x4, 2, 0, 0, 2> zxxz;
            simd::Permute4<double, simd::float64x4, 3, 0, 0, 2> wxxz;
            simd::Permute4<double, simd::float64x4, 0, 1, 0, 2> xyxz;
            simd::Permute4<double, simd::float64x4, 1, 1, 0, 2> yyxz;
            simd::Permute4<double, simd::float64x4, 2, 1, 0, 2> zyxz;
            simd::Permute4<double, simd::float64x4, 3, 1, 0, 2> wyxz;
            simd::Permute4<double, simd::float64x4, 0, 2, 0, 2> xzxz;
            simd::Permute4<double, simd::float64x4, 1, 2, 0, 2> yzxz;
            simd::Permute4<double, simd::float64x4, 2, 2, 0, 2> zzxz;
            simd::Permute4<double, simd::float64x4, 3, 2, 0, 2> wzxz;
            simd::Permute4<double, simd::float64x4, 0, 3, 0, 2> xwxz;
            simd::Permute4<double, simd::float64x4, 1, 3, 0, 2> ywxz;
            simd::Permute4<double, simd::float64x4, 2, 3, 0, 2> zwxz;
            simd::Permute4<double, simd::float64x4, 3, 3, 0, 2> wwxz;
            simd::Permute4<double, simd::float64x4, 0, 0, 1, 2> xxyz;
            simd::Permute4<double, simd::float64x4, 1, 0, 1, 2> yxyz;
            simd::Permute4<double, simd::float64x4, 2, 0, 1, 2> zxyz;
            simd::Permute4<double, simd::float64x4, 3, 0, 1, 2> wxyz;
            simd::Permute4<double, simd::float64x4, 0, 1, 1, 2> xyyz;
            simd::Permute4<double, simd::float64x4, 1, 1, 1, 2> yyyz;
            simd::Permute4<double, simd::float64x4, 2, 1, 1, 2> zyyz;
            simd::Permute4<double, simd::float64x4, 3, 1, 1, 2> wyyz;
            simd::Permute4<double, simd::float64x4, 0, 2, 1, 2> xzyz;
            simd::Permute4<double, simd::float64x4, 1, 2, 1, 2> yzyz;
            simd::Permute4<double, simd::float64x4, 2, 2, 1, 2> zzyz;
            simd::Permute4<double, simd::float64x4, 3, 2, 1, 2> wzyz;
            simd::Permute4<double, simd::float64x4, 0, 3, 1, 2> xwyz;
            simd::Permute4<double, simd::float64x4, 1, 3, 1, 2> ywyz;
            simd::Permute4<double, simd::float64x4, 2, 3, 1, 2> zwyz;
            simd::Permute4<double, simd::float64x4, 3, 3, 1, 2> wwyz;
            simd::Permute4<double, simd::float64x4, 0, 0, 2, 2> xxzz;
            simd::Permute4<double, simd::float64x4, 1, 0, 2, 2> yxzz;
            simd::Permute4<double, simd::float64x4, 2, 0, 2, 2> zxzz;
            simd::Permute4<double, simd::float64x4, 3, 0, 2, 2> wxzz;
            simd::Permute4<double, simd::float64x4, 0, 1, 2, 2> xyzz;
            simd::Permute4<double, simd::float64x4, 1, 1, 2, 2> yyzz;
            simd::Permute4<double, simd::float64x4, 2, 1, 2, 2> zyzz;
            simd::Permute4<double, simd::float64x4, 3, 1, 2, 2> wyzz;
            simd::Permute4<double, simd::float64x4, 0, 2, 2, 2> xzzz;
            simd::Permute4<double, simd::float64x4, 1, 2, 2, 2> yzzz;
            simd::Permute4<double, simd::float64x4, 2, 2, 2, 2> zzzz;
            simd::Permute4<double, simd::float64x4, 3, 2, 2, 2> wzzz;
            simd::Permute4<double, simd::float64x4, 0, 3, 2, 2> xwzz;
            simd::Permute4<double, simd::float64x4, 1, 3, 2, 2> ywzz;
            simd::Permute4<double, simd::float64x4, 2, 3, 2, 2> zwzz;
            simd::Permute4<double, simd::float64x4, 3, 3, 2, 2> wwzz;
            simd::Permute4<double, simd::float64x4, 0, 0, 3, 2> xxwz;
            simd::Permute4<double, simd::float64x4, 1, 0, 3, 2> yxwz;
            simd::Permute4<double, simd::float64x4, 2, 0, 3, 2> zxwz;
            simd::Permute4<double, simd::float64x4, 3, 0, 3, 2> wxwz;
            simd::Permute4<double, simd::float64x4, 0, 1, 3, 2> xywz;
            simd::Permute4<double, simd::float64x4, 1, 1, 3, 2> yywz;
            simd::Permute4<double, simd::float64x4, 2, 1, 3, 2> zywz;
            simd::Permute4<double, simd::float64x4, 3, 1, 3, 2> wywz;
            simd::Permute4<double, simd::float64x4, 0, 2, 3, 2> xzwz;
            simd::Permute4<double, simd::float64x4, 1, 2, 3, 2> yzwz;
            simd::Permute4<double, simd::float64x4, 2, 2, 3, 2> zzwz;
            simd::Permute4<double, simd::float64x4, 3, 2, 3, 2> wzwz;
            simd::Permute4<double, simd::float64x4, 0, 3, 3, 2> xwwz;
            simd::Permute4<double, simd::float64x4, 1, 3, 3, 2> ywwz;
            simd::Permute4<double, simd::float64x4, 2, 3, 3, 2> zwwz;
            simd::Permute4<double, simd::float64x4, 3, 3, 3, 2> wwwz;
            simd::Permute4<double, simd::float64x4, 0, 0, 0, 3> xxxw;
            simd::Permute4<double, simd::float64x4, 1, 0, 0, 3> yxxw;
            simd::Permute4<double, simd::float64x4, 2, 0, 0, 3> zxxw;
            simd::Permute4<double, simd::float64x4, 3, 0, 0, 3> wxxw;
            simd::Permute4<double, simd::float64x4, 0, 1, 0, 3> xyxw;
            simd::Permute4<double, simd::float64x4, 1, 1, 0, 3> yyxw;
            simd::Permute4<double, simd::float64x4, 2, 1, 0, 3> zyxw;
            simd::Permute4<double, simd::float64x4, 3, 1, 0, 3> wyxw;
            simd::Permute4<double, simd::float64x4, 0, 2, 0, 3> xzxw;
            simd::Permute4<double, simd::float64x4, 1, 2, 0, 3> yzxw;
            simd::Permute4<double, simd::float64x4, 2, 2, 0, 3> zzxw;
            simd::Permute4<double, simd::float64x4, 3, 2, 0, 3> wzxw;
            simd::Permute4<double, simd::float64x4, 0, 3, 0, 3> xwxw;
            simd::Permute4<double, simd::float64x4, 1, 3, 0, 3> ywxw;
            simd::Permute4<double, simd::float64x4, 2, 3, 0, 3> zwxw;
            simd::Permute4<double, simd::float64x4, 3, 3, 0, 3> wwxw;
            simd::Permute4<double, simd::float64x4, 0, 0, 1, 3> xxyw;
            simd::Permute4<double, simd::float64x4, 1, 0, 1, 3> yxyw;
            simd::Permute4<double, simd::float64x4, 2, 0, 1, 3> zxyw;
            simd::Permute4<double, simd::float64x4, 3, 0, 1, 3> wxyw;
            simd::Permute4<double, simd::float64x4, 0, 1, 1, 3> xyyw;
            simd::Permute4<double, simd::float64x4, 1, 1, 1, 3> yyyw;
            simd::Permute4<double, simd::float64x4, 2, 1, 1, 3> zyyw;
            simd::Permute4<double, simd::float64x4, 3, 1, 1, 3> wyyw;
            simd::Permute4<double, simd::float64x4, 0, 2, 1, 3> xzyw;
            simd::Permute4<double, simd::float64x4, 1, 2, 1, 3> yzyw;
            simd::Permute4<double, simd::float64x4, 2, 2, 1, 3> zzyw;
            simd::Permute4<double, simd::float64x4, 3, 2, 1, 3> wzyw;
            simd::Permute4<double, simd::float64x4, 0, 3, 1, 3> xwyw;
            simd::Permute4<double, simd::float64x4, 1, 3, 1, 3> ywyw;
            simd::Permute4<double, simd::float64x4, 2, 3, 1, 3> zwyw;
            simd::Permute4<double, simd::float64x4, 3, 3, 1, 3> wwyw;
            simd::Permute4<double, simd::float64x4, 0, 0, 2, 3> xxzw;
            simd::Permute4<double, simd::float64x4, 1, 0, 2, 3> yxzw;
            simd::Permute4<double, simd::float64x4, 2, 0, 2, 3> zxzw;
            simd::Permute4<double, simd::float64x4, 3, 0, 2, 3> wxzw;
            simd::Permute4<double, simd::float64x4, 1, 1, 2, 3> yyzw;
            simd::Permute4<double, simd::float64x4, 2, 1, 2, 3> zyzw;
            simd::Permute4<double, simd::float64x4, 3, 1, 2, 3> wyzw;
            simd::Permute4<double, simd::float64x4, 0, 2, 2, 3> xzzw;
            simd::Permute4<double, simd::float64x4, 1, 2, 2, 3> yzzw;
            simd::Permute4<double, simd::float64x4, 2, 2, 2, 3> zzzw;
            simd::Permute4<double, simd::float64x4, 3, 2, 2, 3> wzzw;
            simd::Permute4<double, simd::float64x4, 0, 3, 2, 3> xwzw;
            simd::Permute4<double, simd::float64x4, 1, 3, 2, 3> ywzw;
            simd::Permute4<double, simd::float64x4, 2, 3, 2, 3> zwzw;
            simd::Permute4<double, simd::float64x4, 3, 3, 2, 3> wwzw;
            simd::Permute4<double, simd::float64x4, 0, 0, 3, 3> xxww;
            simd::Permute4<double, simd::float64x4, 1, 0, 3, 3> yxww;
            simd::Permute4<double, simd::float64x4, 2, 0, 3, 3> zxww;
            simd::Permute4<double, simd::float64x4, 3, 0, 3, 3> wxww;
            simd::Permute4<double, simd::float64x4, 0, 1, 3, 3> xyww;
            simd::Permute4<double, simd::float64x4, 1, 1, 3, 3> yyww;
            simd::Permute4<double, simd::float64x4, 2, 1, 3, 3> zyww;
            simd::Permute4<double, simd::float64x4, 3, 1, 3, 3> wyww;
            simd::Permute4<double, simd::float64x4, 0, 2, 3, 3> xzww;
            simd::Permute4<double, simd::float64x4, 1, 2, 3, 3> yzww;
            simd::Permute4<double, simd::float64x4, 2, 2, 3, 3> zzww;
            simd::Permute4<double, simd::float64x4, 3, 2, 3, 3> wzww;
            simd::Permute4<double, simd::float64x4, 0, 3, 3, 3> xwww;
            simd::Permute4<double, simd::float64x4, 1, 3, 3, 3> ywww;
            simd::Permute4<double, simd::float64x4, 2, 3, 3, 3> zwww;
            simd::Permute4<double, simd::float64x4, 3, 3, 3, 3> wwww;
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
        Vector(const simd::Permute4<double, simd::float64x4, X, Y, Z, W>& p)
        {
            xyzw = p;
        }

        template <int X, int Y, int Z, int W>
        Vector& operator = (const simd::Permute4<double, simd::float64x4, X, Y, Z, W>& p)
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
    MAKE_VECTOR_FUNCTION1(fract, simd::fract)
    MAKE_VECTOR_FUNCTION1(sin, simd::sin)
    MAKE_VECTOR_FUNCTION1(cos, simd::cos)
    MAKE_VECTOR_FUNCTION1(tan, simd::tan)
    MAKE_VECTOR_FUNCTION1(asin, simd::asin)
    MAKE_VECTOR_FUNCTION1(acos, simd::acos)
    MAKE_VECTOR_FUNCTION1(atan, simd::atan)
    MAKE_VECTOR_FUNCTION1(exp, simd::exp)
    MAKE_VECTOR_FUNCTION1(log, simd::log)
    MAKE_VECTOR_FUNCTION1(exp2, simd::exp2)
    MAKE_VECTOR_FUNCTION1(log2, simd::log2)
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
    MAKE_VECTOR_FUNCTION2(pow, simd::pow)
    MAKE_VECTOR_FUNCTION2(atan2, simd::atan2)

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

    static inline double4 operator > (double4 a, double4 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline double4 operator >= (double4 a, double4 b)
    {
        return simd::compare_ge(a, b);
    }

    static inline double4 operator < (double4 a, double4 b)
    {
        return simd::compare_lt(a, b);
    }

    static inline double4 operator <= (double4 a, double4 b)
    {
        return simd::compare_le(a, b);
    }

    static inline double4 operator == (double4 a, double4 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline double4 operator != (double4 a, double4 b)
    {
        return simd::compare_neq(a, b);
    }

    static inline double4 select(double4 mask, double4 a, double4 b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango

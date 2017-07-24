/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int32, 4> : VectorBase<int32, 4>
    {
        using VectorType = simd::int32x4;
        using Mask = simd::int32x4::mask;

        union
        {
            simd::int32x4 xyzw;

            ScalarAccessor<int32, simd::int32x4, 0> x;
            ScalarAccessor<int32, simd::int32x4, 1> y;
            ScalarAccessor<int32, simd::int32x4, 2> z;
            ScalarAccessor<int32, simd::int32x4, 3> w;

            Permute4<int32, simd::int32x4, 0, 0, 0, 0> xxxx;
            Permute4<int32, simd::int32x4, 1, 0, 0, 0> yxxx;
            Permute4<int32, simd::int32x4, 2, 0, 0, 0> zxxx;
            Permute4<int32, simd::int32x4, 3, 0, 0, 0> wxxx;
            Permute4<int32, simd::int32x4, 0, 1, 0, 0> xyxx;
            Permute4<int32, simd::int32x4, 1, 1, 0, 0> yyxx;
            Permute4<int32, simd::int32x4, 2, 1, 0, 0> zyxx;
            Permute4<int32, simd::int32x4, 3, 1, 0, 0> wyxx;
            Permute4<int32, simd::int32x4, 0, 2, 0, 0> xzxx;
            Permute4<int32, simd::int32x4, 1, 2, 0, 0> yzxx;
            Permute4<int32, simd::int32x4, 2, 2, 0, 0> zzxx;
            Permute4<int32, simd::int32x4, 3, 2, 0, 0> wzxx;
            Permute4<int32, simd::int32x4, 0, 3, 0, 0> xwxx;
            Permute4<int32, simd::int32x4, 1, 3, 0, 0> ywxx;
            Permute4<int32, simd::int32x4, 2, 3, 0, 0> zwxx;
            Permute4<int32, simd::int32x4, 3, 3, 0, 0> wwxx;
            Permute4<int32, simd::int32x4, 0, 0, 1, 0> xxyx;
            Permute4<int32, simd::int32x4, 1, 0, 1, 0> yxyx;
            Permute4<int32, simd::int32x4, 2, 0, 1, 0> zxyx;
            Permute4<int32, simd::int32x4, 3, 0, 1, 0> wxyx;
            Permute4<int32, simd::int32x4, 0, 1, 1, 0> xyyx;
            Permute4<int32, simd::int32x4, 1, 1, 1, 0> yyyx;
            Permute4<int32, simd::int32x4, 2, 1, 1, 0> zyyx;
            Permute4<int32, simd::int32x4, 3, 1, 1, 0> wyyx;
            Permute4<int32, simd::int32x4, 0, 2, 1, 0> xzyx;
            Permute4<int32, simd::int32x4, 1, 2, 1, 0> yzyx;
            Permute4<int32, simd::int32x4, 2, 2, 1, 0> zzyx;
            Permute4<int32, simd::int32x4, 3, 2, 1, 0> wzyx;
            Permute4<int32, simd::int32x4, 0, 3, 1, 0> xwyx;
            Permute4<int32, simd::int32x4, 1, 3, 1, 0> ywyx;
            Permute4<int32, simd::int32x4, 2, 3, 1, 0> zwyx;
            Permute4<int32, simd::int32x4, 3, 3, 1, 0> wwyx;
            Permute4<int32, simd::int32x4, 0, 0, 2, 0> xxzx;
            Permute4<int32, simd::int32x4, 1, 0, 2, 0> yxzx;
            Permute4<int32, simd::int32x4, 2, 0, 2, 0> zxzx;
            Permute4<int32, simd::int32x4, 3, 0, 2, 0> wxzx;
            Permute4<int32, simd::int32x4, 0, 1, 2, 0> xyzx;
            Permute4<int32, simd::int32x4, 1, 1, 2, 0> yyzx;
            Permute4<int32, simd::int32x4, 2, 1, 2, 0> zyzx;
            Permute4<int32, simd::int32x4, 3, 1, 2, 0> wyzx;
            Permute4<int32, simd::int32x4, 0, 2, 2, 0> xzzx;
            Permute4<int32, simd::int32x4, 1, 2, 2, 0> yzzx;
            Permute4<int32, simd::int32x4, 2, 2, 2, 0> zzzx;
            Permute4<int32, simd::int32x4, 3, 2, 2, 0> wzzx;
            Permute4<int32, simd::int32x4, 0, 3, 2, 0> xwzx;
            Permute4<int32, simd::int32x4, 1, 3, 2, 0> ywzx;
            Permute4<int32, simd::int32x4, 2, 3, 2, 0> zwzx;
            Permute4<int32, simd::int32x4, 3, 3, 2, 0> wwzx;
            Permute4<int32, simd::int32x4, 0, 0, 3, 0> xxwx;
            Permute4<int32, simd::int32x4, 1, 0, 3, 0> yxwx;
            Permute4<int32, simd::int32x4, 2, 0, 3, 0> zxwx;
            Permute4<int32, simd::int32x4, 3, 0, 3, 0> wxwx;
            Permute4<int32, simd::int32x4, 0, 1, 3, 0> xywx;
            Permute4<int32, simd::int32x4, 1, 1, 3, 0> yywx;
            Permute4<int32, simd::int32x4, 2, 1, 3, 0> zywx;
            Permute4<int32, simd::int32x4, 3, 1, 3, 0> wywx;
            Permute4<int32, simd::int32x4, 0, 2, 3, 0> xzwx;
            Permute4<int32, simd::int32x4, 1, 2, 3, 0> yzwx;
            Permute4<int32, simd::int32x4, 2, 2, 3, 0> zzwx;
            Permute4<int32, simd::int32x4, 3, 2, 3, 0> wzwx;
            Permute4<int32, simd::int32x4, 0, 3, 3, 0> xwwx;
            Permute4<int32, simd::int32x4, 1, 3, 3, 0> ywwx;
            Permute4<int32, simd::int32x4, 2, 3, 3, 0> zwwx;
            Permute4<int32, simd::int32x4, 3, 3, 3, 0> wwwx;
            Permute4<int32, simd::int32x4, 0, 0, 0, 1> xxxy;
            Permute4<int32, simd::int32x4, 1, 0, 0, 1> yxxy;
            Permute4<int32, simd::int32x4, 2, 0, 0, 1> zxxy;
            Permute4<int32, simd::int32x4, 3, 0, 0, 1> wxxy;
            Permute4<int32, simd::int32x4, 0, 1, 0, 1> xyxy;
            Permute4<int32, simd::int32x4, 1, 1, 0, 1> yyxy;
            Permute4<int32, simd::int32x4, 2, 1, 0, 1> zyxy;
            Permute4<int32, simd::int32x4, 3, 1, 0, 1> wyxy;
            Permute4<int32, simd::int32x4, 0, 2, 0, 1> xzxy;
            Permute4<int32, simd::int32x4, 1, 2, 0, 1> yzxy;
            Permute4<int32, simd::int32x4, 2, 2, 0, 1> zzxy;
            Permute4<int32, simd::int32x4, 3, 2, 0, 1> wzxy;
            Permute4<int32, simd::int32x4, 0, 3, 0, 1> xwxy;
            Permute4<int32, simd::int32x4, 1, 3, 0, 1> ywxy;
            Permute4<int32, simd::int32x4, 2, 3, 0, 1> zwxy;
            Permute4<int32, simd::int32x4, 3, 3, 0, 1> wwxy;
            Permute4<int32, simd::int32x4, 0, 0, 1, 1> xxyy;
            Permute4<int32, simd::int32x4, 1, 0, 1, 1> yxyy;
            Permute4<int32, simd::int32x4, 2, 0, 1, 1> zxyy;
            Permute4<int32, simd::int32x4, 3, 0, 1, 1> wxyy;
            Permute4<int32, simd::int32x4, 0, 1, 1, 1> xyyy;
            Permute4<int32, simd::int32x4, 1, 1, 1, 1> yyyy;
            Permute4<int32, simd::int32x4, 2, 1, 1, 1> zyyy;
            Permute4<int32, simd::int32x4, 3, 1, 1, 1> wyyy;
            Permute4<int32, simd::int32x4, 0, 2, 1, 1> xzyy;
            Permute4<int32, simd::int32x4, 1, 2, 1, 1> yzyy;
            Permute4<int32, simd::int32x4, 2, 2, 1, 1> zzyy;
            Permute4<int32, simd::int32x4, 3, 2, 1, 1> wzyy;
            Permute4<int32, simd::int32x4, 0, 3, 1, 1> xwyy;
            Permute4<int32, simd::int32x4, 1, 3, 1, 1> ywyy;
            Permute4<int32, simd::int32x4, 2, 3, 1, 1> zwyy;
            Permute4<int32, simd::int32x4, 3, 3, 1, 1> wwyy;
            Permute4<int32, simd::int32x4, 0, 0, 2, 1> xxzy;
            Permute4<int32, simd::int32x4, 1, 0, 2, 1> yxzy;
            Permute4<int32, simd::int32x4, 2, 0, 2, 1> zxzy;
            Permute4<int32, simd::int32x4, 3, 0, 2, 1> wxzy;
            Permute4<int32, simd::int32x4, 0, 1, 2, 1> xyzy;
            Permute4<int32, simd::int32x4, 1, 1, 2, 1> yyzy;
            Permute4<int32, simd::int32x4, 2, 1, 2, 1> zyzy;
            Permute4<int32, simd::int32x4, 3, 1, 2, 1> wyzy;
            Permute4<int32, simd::int32x4, 0, 2, 2, 1> xzzy;
            Permute4<int32, simd::int32x4, 1, 2, 2, 1> yzzy;
            Permute4<int32, simd::int32x4, 2, 2, 2, 1> zzzy;
            Permute4<int32, simd::int32x4, 3, 2, 2, 1> wzzy;
            Permute4<int32, simd::int32x4, 0, 3, 2, 1> xwzy;
            Permute4<int32, simd::int32x4, 1, 3, 2, 1> ywzy;
            Permute4<int32, simd::int32x4, 2, 3, 2, 1> zwzy;
            Permute4<int32, simd::int32x4, 3, 3, 2, 1> wwzy;
            Permute4<int32, simd::int32x4, 0, 0, 3, 1> xxwy;
            Permute4<int32, simd::int32x4, 1, 0, 3, 1> yxwy;
            Permute4<int32, simd::int32x4, 2, 0, 3, 1> zxwy;
            Permute4<int32, simd::int32x4, 3, 0, 3, 1> wxwy;
            Permute4<int32, simd::int32x4, 0, 1, 3, 1> xywy;
            Permute4<int32, simd::int32x4, 1, 1, 3, 1> yywy;
            Permute4<int32, simd::int32x4, 2, 1, 3, 1> zywy;
            Permute4<int32, simd::int32x4, 3, 1, 3, 1> wywy;
            Permute4<int32, simd::int32x4, 0, 2, 3, 1> xzwy;
            Permute4<int32, simd::int32x4, 1, 2, 3, 1> yzwy;
            Permute4<int32, simd::int32x4, 2, 2, 3, 1> zzwy;
            Permute4<int32, simd::int32x4, 3, 2, 3, 1> wzwy;
            Permute4<int32, simd::int32x4, 0, 3, 3, 1> xwwy;
            Permute4<int32, simd::int32x4, 1, 3, 3, 1> ywwy;
            Permute4<int32, simd::int32x4, 2, 3, 3, 1> zwwy;
            Permute4<int32, simd::int32x4, 3, 3, 3, 1> wwwy;
            Permute4<int32, simd::int32x4, 0, 0, 0, 2> xxxz;
            Permute4<int32, simd::int32x4, 1, 0, 0, 2> yxxz;
            Permute4<int32, simd::int32x4, 2, 0, 0, 2> zxxz;
            Permute4<int32, simd::int32x4, 3, 0, 0, 2> wxxz;
            Permute4<int32, simd::int32x4, 0, 1, 0, 2> xyxz;
            Permute4<int32, simd::int32x4, 1, 1, 0, 2> yyxz;
            Permute4<int32, simd::int32x4, 2, 1, 0, 2> zyxz;
            Permute4<int32, simd::int32x4, 3, 1, 0, 2> wyxz;
            Permute4<int32, simd::int32x4, 0, 2, 0, 2> xzxz;
            Permute4<int32, simd::int32x4, 1, 2, 0, 2> yzxz;
            Permute4<int32, simd::int32x4, 2, 2, 0, 2> zzxz;
            Permute4<int32, simd::int32x4, 3, 2, 0, 2> wzxz;
            Permute4<int32, simd::int32x4, 0, 3, 0, 2> xwxz;
            Permute4<int32, simd::int32x4, 1, 3, 0, 2> ywxz;
            Permute4<int32, simd::int32x4, 2, 3, 0, 2> zwxz;
            Permute4<int32, simd::int32x4, 3, 3, 0, 2> wwxz;
            Permute4<int32, simd::int32x4, 0, 0, 1, 2> xxyz;
            Permute4<int32, simd::int32x4, 1, 0, 1, 2> yxyz;
            Permute4<int32, simd::int32x4, 2, 0, 1, 2> zxyz;
            Permute4<int32, simd::int32x4, 3, 0, 1, 2> wxyz;
            Permute4<int32, simd::int32x4, 0, 1, 1, 2> xyyz;
            Permute4<int32, simd::int32x4, 1, 1, 1, 2> yyyz;
            Permute4<int32, simd::int32x4, 2, 1, 1, 2> zyyz;
            Permute4<int32, simd::int32x4, 3, 1, 1, 2> wyyz;
            Permute4<int32, simd::int32x4, 0, 2, 1, 2> xzyz;
            Permute4<int32, simd::int32x4, 1, 2, 1, 2> yzyz;
            Permute4<int32, simd::int32x4, 2, 2, 1, 2> zzyz;
            Permute4<int32, simd::int32x4, 3, 2, 1, 2> wzyz;
            Permute4<int32, simd::int32x4, 0, 3, 1, 2> xwyz;
            Permute4<int32, simd::int32x4, 1, 3, 1, 2> ywyz;
            Permute4<int32, simd::int32x4, 2, 3, 1, 2> zwyz;
            Permute4<int32, simd::int32x4, 3, 3, 1, 2> wwyz;
            Permute4<int32, simd::int32x4, 0, 0, 2, 2> xxzz;
            Permute4<int32, simd::int32x4, 1, 0, 2, 2> yxzz;
            Permute4<int32, simd::int32x4, 2, 0, 2, 2> zxzz;
            Permute4<int32, simd::int32x4, 3, 0, 2, 2> wxzz;
            Permute4<int32, simd::int32x4, 0, 1, 2, 2> xyzz;
            Permute4<int32, simd::int32x4, 1, 1, 2, 2> yyzz;
            Permute4<int32, simd::int32x4, 2, 1, 2, 2> zyzz;
            Permute4<int32, simd::int32x4, 3, 1, 2, 2> wyzz;
            Permute4<int32, simd::int32x4, 0, 2, 2, 2> xzzz;
            Permute4<int32, simd::int32x4, 1, 2, 2, 2> yzzz;
            Permute4<int32, simd::int32x4, 2, 2, 2, 2> zzzz;
            Permute4<int32, simd::int32x4, 3, 2, 2, 2> wzzz;
            Permute4<int32, simd::int32x4, 0, 3, 2, 2> xwzz;
            Permute4<int32, simd::int32x4, 1, 3, 2, 2> ywzz;
            Permute4<int32, simd::int32x4, 2, 3, 2, 2> zwzz;
            Permute4<int32, simd::int32x4, 3, 3, 2, 2> wwzz;
            Permute4<int32, simd::int32x4, 0, 0, 3, 2> xxwz;
            Permute4<int32, simd::int32x4, 1, 0, 3, 2> yxwz;
            Permute4<int32, simd::int32x4, 2, 0, 3, 2> zxwz;
            Permute4<int32, simd::int32x4, 3, 0, 3, 2> wxwz;
            Permute4<int32, simd::int32x4, 0, 1, 3, 2> xywz;
            Permute4<int32, simd::int32x4, 1, 1, 3, 2> yywz;
            Permute4<int32, simd::int32x4, 2, 1, 3, 2> zywz;
            Permute4<int32, simd::int32x4, 3, 1, 3, 2> wywz;
            Permute4<int32, simd::int32x4, 0, 2, 3, 2> xzwz;
            Permute4<int32, simd::int32x4, 1, 2, 3, 2> yzwz;
            Permute4<int32, simd::int32x4, 2, 2, 3, 2> zzwz;
            Permute4<int32, simd::int32x4, 3, 2, 3, 2> wzwz;
            Permute4<int32, simd::int32x4, 0, 3, 3, 2> xwwz;
            Permute4<int32, simd::int32x4, 1, 3, 3, 2> ywwz;
            Permute4<int32, simd::int32x4, 2, 3, 3, 2> zwwz;
            Permute4<int32, simd::int32x4, 3, 3, 3, 2> wwwz;
            Permute4<int32, simd::int32x4, 0, 0, 0, 3> xxxw;
            Permute4<int32, simd::int32x4, 1, 0, 0, 3> yxxw;
            Permute4<int32, simd::int32x4, 2, 0, 0, 3> zxxw;
            Permute4<int32, simd::int32x4, 3, 0, 0, 3> wxxw;
            Permute4<int32, simd::int32x4, 0, 1, 0, 3> xyxw;
            Permute4<int32, simd::int32x4, 1, 1, 0, 3> yyxw;
            Permute4<int32, simd::int32x4, 2, 1, 0, 3> zyxw;
            Permute4<int32, simd::int32x4, 3, 1, 0, 3> wyxw;
            Permute4<int32, simd::int32x4, 0, 2, 0, 3> xzxw;
            Permute4<int32, simd::int32x4, 1, 2, 0, 3> yzxw;
            Permute4<int32, simd::int32x4, 2, 2, 0, 3> zzxw;
            Permute4<int32, simd::int32x4, 3, 2, 0, 3> wzxw;
            Permute4<int32, simd::int32x4, 0, 3, 0, 3> xwxw;
            Permute4<int32, simd::int32x4, 1, 3, 0, 3> ywxw;
            Permute4<int32, simd::int32x4, 2, 3, 0, 3> zwxw;
            Permute4<int32, simd::int32x4, 3, 3, 0, 3> wwxw;
            Permute4<int32, simd::int32x4, 0, 0, 1, 3> xxyw;
            Permute4<int32, simd::int32x4, 1, 0, 1, 3> yxyw;
            Permute4<int32, simd::int32x4, 2, 0, 1, 3> zxyw;
            Permute4<int32, simd::int32x4, 3, 0, 1, 3> wxyw;
            Permute4<int32, simd::int32x4, 0, 1, 1, 3> xyyw;
            Permute4<int32, simd::int32x4, 1, 1, 1, 3> yyyw;
            Permute4<int32, simd::int32x4, 2, 1, 1, 3> zyyw;
            Permute4<int32, simd::int32x4, 3, 1, 1, 3> wyyw;
            Permute4<int32, simd::int32x4, 0, 2, 1, 3> xzyw;
            Permute4<int32, simd::int32x4, 1, 2, 1, 3> yzyw;
            Permute4<int32, simd::int32x4, 2, 2, 1, 3> zzyw;
            Permute4<int32, simd::int32x4, 3, 2, 1, 3> wzyw;
            Permute4<int32, simd::int32x4, 0, 3, 1, 3> xwyw;
            Permute4<int32, simd::int32x4, 1, 3, 1, 3> ywyw;
            Permute4<int32, simd::int32x4, 2, 3, 1, 3> zwyw;
            Permute4<int32, simd::int32x4, 3, 3, 1, 3> wwyw;
            Permute4<int32, simd::int32x4, 0, 0, 2, 3> xxzw;
            Permute4<int32, simd::int32x4, 1, 0, 2, 3> yxzw;
            Permute4<int32, simd::int32x4, 2, 0, 2, 3> zxzw;
            Permute4<int32, simd::int32x4, 3, 0, 2, 3> wxzw;
            Permute4<int32, simd::int32x4, 1, 1, 2, 3> yyzw;
            Permute4<int32, simd::int32x4, 2, 1, 2, 3> zyzw;
            Permute4<int32, simd::int32x4, 3, 1, 2, 3> wyzw;
            Permute4<int32, simd::int32x4, 0, 2, 2, 3> xzzw;
            Permute4<int32, simd::int32x4, 1, 2, 2, 3> yzzw;
            Permute4<int32, simd::int32x4, 2, 2, 2, 3> zzzw;
            Permute4<int32, simd::int32x4, 3, 2, 2, 3> wzzw;
            Permute4<int32, simd::int32x4, 0, 3, 2, 3> xwzw;
            Permute4<int32, simd::int32x4, 1, 3, 2, 3> ywzw;
            Permute4<int32, simd::int32x4, 2, 3, 2, 3> zwzw;
            Permute4<int32, simd::int32x4, 3, 3, 2, 3> wwzw;
            Permute4<int32, simd::int32x4, 0, 0, 3, 3> xxww;
            Permute4<int32, simd::int32x4, 1, 0, 3, 3> yxww;
            Permute4<int32, simd::int32x4, 2, 0, 3, 3> zxww;
            Permute4<int32, simd::int32x4, 3, 0, 3, 3> wxww;
            Permute4<int32, simd::int32x4, 0, 1, 3, 3> xyww;
            Permute4<int32, simd::int32x4, 1, 1, 3, 3> yyww;
            Permute4<int32, simd::int32x4, 2, 1, 3, 3> zyww;
            Permute4<int32, simd::int32x4, 3, 1, 3, 3> wyww;
            Permute4<int32, simd::int32x4, 0, 2, 3, 3> xzww;
            Permute4<int32, simd::int32x4, 1, 2, 3, 3> yzww;
            Permute4<int32, simd::int32x4, 2, 2, 3, 3> zzww;
            Permute4<int32, simd::int32x4, 3, 2, 3, 3> wzww;
            Permute4<int32, simd::int32x4, 0, 3, 3, 3> xwww;
            Permute4<int32, simd::int32x4, 1, 3, 3, 3> ywww;
            Permute4<int32, simd::int32x4, 2, 3, 3, 3> zwww;
            Permute4<int32, simd::int32x4, 3, 3, 3, 3> wwww;
        };

        explicit Vector() = default;

        explicit Vector(int32 s)
        : xyzw(simd::int32x4_set1(s))
        {
        }

        Vector(int32 x, int32 y, int32 z, int32 w)
        : xyzw(simd::int32x4_set4(x, y, z, w))
        {
        }

        Vector(simd::int32x4 v)
        : xyzw(v)
        {
        }

        Vector& operator = (simd::int32x4 v)
        {
            xyzw = v;
            return *this;
        }

        Vector& operator = (int32 s)
        {
            xyzw = simd::int32x4_set1(s);
            return *this;
        }

        operator simd::int32x4 () const
        {
            return xyzw;
        }

        operator simd::int32x4 ()
        {
            return xyzw;
        }

        uint32 mask() const
        {
            return simd::get_mask(xyzw);
        }
    };

    static inline const Vector<int32, 4> operator + (Vector<int32, 4> v)
    {
        return v;
    }

    static inline Vector<int32, 4> operator - (Vector<int32, 4> v)
    {
        return simd::sub(simd::int32x4_zero(), v);
    }

    static inline Vector<int32, 4>& operator += (Vector<int32, 4>& a, Vector<int32, 4> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<int32, 4>& operator += (Vector<int32, 4>& a, int32 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<int32, 4>& operator -= (Vector<int32, 4>& a, Vector<int32, 4> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<int32, 4>& operator -= (Vector<int32, 4>& a, int32 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<int32, 4> operator + (Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int32, 4> operator + (Vector<int32, 4> a, int32 b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int32, 4> operator + (int32 a, Vector<int32, 4> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<int32, 4> operator - (Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int32, 4> operator - (Vector<int32, 4> a, int32 b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int32, 4> operator - (int32 a, Vector<int32, 4> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<int32, 4> nand(Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<int32, 4> operator & (Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<int32, 4> operator | (Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<int32, 4> operator ^ (Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<int32, 4> operator ~ (Vector<int32, 4> a)
    {
        return simd::bitwise_not(a);
    }

    static inline Vector<int32, 4> adds(Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<int32, 4> subs(Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<int32, 4> min(Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<int32, 4> max(Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<int32, 4> clamp(Vector<int32, 4> a, Vector<int32, 4> amin, Vector<int32, 4> amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline Vector<int32, 4>::Mask operator > (Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline Vector<int32, 4>::Mask operator >= (Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline Vector<int32, 4>::Mask operator < (Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline Vector<int32, 4>::Mask operator <= (Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::compare_le(a, b);
    }

    static inline Vector<int32, 4>::Mask operator == (Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<int32, 4>::Mask operator != (Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<int32, 4> select(Vector<int32, 4>::Mask mask, Vector<int32, 4> a, Vector<int32, 4> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<int32, 4> operator << (Vector<int32, 4> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<int32, 4> operator >> (Vector<int32, 4> a, int b)
    {
        return simd::sra(a, b);
    }

} // namespace mango

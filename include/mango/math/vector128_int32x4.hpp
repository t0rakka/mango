/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<int32, 4>
    {
        using VectorType = simd::int32x4;
        using ScalarType = int32;
        enum { VectorSize = 4 };

        union
        {
            simd::int32x4 m;

            ScalarAccessor<int32, simd::int32x4, 0> x;
            ScalarAccessor<int32, simd::int32x4, 1> y;
            ScalarAccessor<int32, simd::int32x4, 2> z;
            ScalarAccessor<int32, simd::int32x4, 3> w;

            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 0, 0> xxxx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 0, 0> yxxx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 0, 0> zxxx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 0, 0> wxxx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 0, 0> xyxx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 0, 0> yyxx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 0, 0> zyxx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 0, 0> wyxx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 0, 0> xzxx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 0, 0> yzxx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 0, 0> zzxx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 0, 0> wzxx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 0, 0> xwxx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 0, 0> ywxx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 0, 0> zwxx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 0, 0> wwxx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 1, 0> xxyx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 1, 0> yxyx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 1, 0> zxyx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 1, 0> wxyx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 1, 0> xyyx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 1, 0> yyyx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 1, 0> zyyx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 1, 0> wyyx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 1, 0> xzyx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 1, 0> yzyx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 1, 0> zzyx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 1, 0> wzyx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 1, 0> xwyx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 1, 0> ywyx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 1, 0> zwyx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 1, 0> wwyx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 2, 0> xxzx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 2, 0> yxzx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 2, 0> zxzx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 2, 0> wxzx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 2, 0> xyzx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 2, 0> yyzx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 2, 0> zyzx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 2, 0> wyzx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 2, 0> xzzx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 2, 0> yzzx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 2, 0> zzzx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 2, 0> wzzx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 2, 0> xwzx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 2, 0> ywzx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 2, 0> zwzx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 2, 0> wwzx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 3, 0> xxwx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 3, 0> yxwx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 3, 0> zxwx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 3, 0> wxwx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 3, 0> xywx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 3, 0> yywx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 3, 0> zywx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 3, 0> wywx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 3, 0> xzwx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 3, 0> yzwx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 3, 0> zzwx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 3, 0> wzwx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 3, 0> xwwx;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 3, 0> ywwx;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 3, 0> zwwx;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 3, 0> wwwx;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 0, 1> xxxy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 0, 1> yxxy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 0, 1> zxxy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 0, 1> wxxy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 0, 1> xyxy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 0, 1> yyxy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 0, 1> zyxy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 0, 1> wyxy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 0, 1> xzxy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 0, 1> yzxy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 0, 1> zzxy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 0, 1> wzxy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 0, 1> xwxy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 0, 1> ywxy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 0, 1> zwxy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 0, 1> wwxy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 1, 1> xxyy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 1, 1> yxyy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 1, 1> zxyy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 1, 1> wxyy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 1, 1> xyyy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 1, 1> yyyy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 1, 1> zyyy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 1, 1> wyyy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 1, 1> xzyy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 1, 1> yzyy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 1, 1> zzyy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 1, 1> wzyy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 1, 1> xwyy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 1, 1> ywyy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 1, 1> zwyy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 1, 1> wwyy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 2, 1> xxzy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 2, 1> yxzy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 2, 1> zxzy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 2, 1> wxzy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 2, 1> xyzy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 2, 1> yyzy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 2, 1> zyzy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 2, 1> wyzy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 2, 1> xzzy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 2, 1> yzzy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 2, 1> zzzy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 2, 1> wzzy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 2, 1> xwzy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 2, 1> ywzy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 2, 1> zwzy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 2, 1> wwzy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 3, 1> xxwy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 3, 1> yxwy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 3, 1> zxwy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 3, 1> wxwy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 3, 1> xywy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 3, 1> yywy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 3, 1> zywy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 3, 1> wywy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 3, 1> xzwy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 3, 1> yzwy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 3, 1> zzwy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 3, 1> wzwy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 3, 1> xwwy;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 3, 1> ywwy;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 3, 1> zwwy;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 3, 1> wwwy;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 0, 2> xxxz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 0, 2> yxxz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 0, 2> zxxz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 0, 2> wxxz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 0, 2> xyxz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 0, 2> yyxz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 0, 2> zyxz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 0, 2> wyxz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 0, 2> xzxz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 0, 2> yzxz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 0, 2> zzxz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 0, 2> wzxz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 0, 2> xwxz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 0, 2> ywxz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 0, 2> zwxz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 0, 2> wwxz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 1, 2> xxyz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 1, 2> yxyz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 1, 2> zxyz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 1, 2> wxyz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 1, 2> xyyz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 1, 2> yyyz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 1, 2> zyyz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 1, 2> wyyz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 1, 2> xzyz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 1, 2> yzyz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 1, 2> zzyz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 1, 2> wzyz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 1, 2> xwyz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 1, 2> ywyz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 1, 2> zwyz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 1, 2> wwyz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 2, 2> xxzz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 2, 2> yxzz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 2, 2> zxzz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 2, 2> wxzz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 2, 2> xyzz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 2, 2> yyzz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 2, 2> zyzz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 2, 2> wyzz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 2, 2> xzzz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 2, 2> yzzz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 2, 2> zzzz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 2, 2> wzzz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 2, 2> xwzz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 2, 2> ywzz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 2, 2> zwzz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 2, 2> wwzz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 3, 2> xxwz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 3, 2> yxwz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 3, 2> zxwz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 3, 2> wxwz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 3, 2> xywz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 3, 2> yywz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 3, 2> zywz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 3, 2> wywz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 3, 2> xzwz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 3, 2> yzwz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 3, 2> zzwz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 3, 2> wzwz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 3, 2> xwwz;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 3, 2> ywwz;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 3, 2> zwwz;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 3, 2> wwwz;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 0, 3> xxxw;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 0, 3> yxxw;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 0, 3> zxxw;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 0, 3> wxxw;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 0, 3> xyxw;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 0, 3> yyxw;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 0, 3> zyxw;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 0, 3> wyxw;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 0, 3> xzxw;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 0, 3> yzxw;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 0, 3> zzxw;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 0, 3> wzxw;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 0, 3> xwxw;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 0, 3> ywxw;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 0, 3> zwxw;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 0, 3> wwxw;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 1, 3> xxyw;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 1, 3> yxyw;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 1, 3> zxyw;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 1, 3> wxyw;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 1, 3> xyyw;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 1, 3> yyyw;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 1, 3> zyyw;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 1, 3> wyyw;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 1, 3> xzyw;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 1, 3> yzyw;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 1, 3> zzyw;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 1, 3> wzyw;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 1, 3> xwyw;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 1, 3> ywyw;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 1, 3> zwyw;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 1, 3> wwyw;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 2, 3> xxzw;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 2, 3> yxzw;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 2, 3> zxzw;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 2, 3> wxzw;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 2, 3> xyzw;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 2, 3> yyzw;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 2, 3> zyzw;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 2, 3> wyzw;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 2, 3> xzzw;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 2, 3> yzzw;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 2, 3> zzzw;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 2, 3> wzzw;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 2, 3> xwzw;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 2, 3> ywzw;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 2, 3> zwzw;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 2, 3> wwzw;
            ShuffleAccessor4<int32, simd::int32x4, 0, 0, 3, 3> xxww;
            ShuffleAccessor4<int32, simd::int32x4, 1, 0, 3, 3> yxww;
            ShuffleAccessor4<int32, simd::int32x4, 2, 0, 3, 3> zxww;
            ShuffleAccessor4<int32, simd::int32x4, 3, 0, 3, 3> wxww;
            ShuffleAccessor4<int32, simd::int32x4, 0, 1, 3, 3> xyww;
            ShuffleAccessor4<int32, simd::int32x4, 1, 1, 3, 3> yyww;
            ShuffleAccessor4<int32, simd::int32x4, 2, 1, 3, 3> zyww;
            ShuffleAccessor4<int32, simd::int32x4, 3, 1, 3, 3> wyww;
            ShuffleAccessor4<int32, simd::int32x4, 0, 2, 3, 3> xzww;
            ShuffleAccessor4<int32, simd::int32x4, 1, 2, 3, 3> yzww;
            ShuffleAccessor4<int32, simd::int32x4, 2, 2, 3, 3> zzww;
            ShuffleAccessor4<int32, simd::int32x4, 3, 2, 3, 3> wzww;
            ShuffleAccessor4<int32, simd::int32x4, 0, 3, 3, 3> xwww;
            ShuffleAccessor4<int32, simd::int32x4, 1, 3, 3, 3> ywww;
            ShuffleAccessor4<int32, simd::int32x4, 2, 3, 3, 3> zwww;
            ShuffleAccessor4<int32, simd::int32x4, 3, 3, 3, 3> wwww;

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

        Vector(int32 s)
            : m(simd::int32x4_set1(s))
        {
        }

        explicit Vector(int32 x, int32 y, int32 z, int32 w)
            : m(simd::int32x4_set4(x, y, z, w))
        {
        }

        Vector(simd::int32x4 v)
            : m(v)
        {
        }

        Vector& operator = (simd::int32x4 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (int32 s)
        {
            m = simd::int32x4_set1(s);
            return *this;
        }

        operator simd::int32x4 () const
        {
            return m;
        }

#ifdef int128_is_hardware_vector
        operator simd::int32x4::vector () const
        {
            return m.data;
        }
#endif

        uint32 pack() const
        {
            return simd::pack(m);
        }

        void unpack(uint32 a)
        {
            m = simd::unpack(a);
        }
    };

    template <>
    inline Vector<int32, 4> load_low<int32, 4>(const int32 *source)
    {
        return simd::int32x4_load_low(source);
    }

    static inline void store_low(int32 *dest, Vector<int32, 4> v)
    {
        simd::int32x4_store_low(dest, v);
    }

    static inline const int32x4 operator + (int32x4 v)
    {
        return v;
    }

    static inline int32x4 operator - (int32x4 v)
    {
        return simd::sub(simd::int32x4_zero(), v);
    }

    static inline int32x4& operator += (int32x4& a, int32x4 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline int32x4& operator -= (int32x4& a, int32x4 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline int32x4 operator + (int32x4 a, int32x4 b)
    {
        return simd::add(a, b);
    }

    static inline int32x4 operator - (int32x4 a, int32x4 b)
    {
        return simd::sub(a, b);
    }

    static inline int32x4 nand(int32x4 a, int32x4 b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline int32x4 operator & (int32x4 a, int32x4 b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline int32x4 operator | (int32x4 a, int32x4 b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline int32x4 operator ^ (int32x4 a, int32x4 b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline int32x4 operator ~ (int32x4 a)
    {
        return simd::bitwise_not(a);
    }

    static inline int32x4 adds(int32x4 a, int32x4 b)
    {
        return simd::adds(a, b);
    }

    static inline int32x4 subs(int32x4 a, int32x4 b)
    {
        return simd::subs(a, b);
    }

    static inline int32x4 min(int32x4 a, int32x4 b)
    {
        return simd::min(a, b);
    }

    static inline int32x4 max(int32x4 a, int32x4 b)
    {
        return simd::max(a, b);
    }

    static inline int32x4 clamp(int32x4 a, int32x4 amin, int32x4 amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline mask32x4 operator > (int32x4 a, int32x4 b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask32x4 operator >= (int32x4 a, int32x4 b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask32x4 operator < (int32x4 a, int32x4 b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask32x4 operator <= (int32x4 a, int32x4 b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask32x4 operator == (int32x4 a, int32x4 b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask32x4 operator != (int32x4 a, int32x4 b)
    {
        return simd::compare_neq(a, b);
    }

    static inline int32x4 select(mask32x4 mask, int32x4 a, int32x4 b)
    {
        return simd::select(mask, a, b);
    }

    static inline int32x4 operator << (int32x4 a, int b)
    {
        return simd::sll(a, b);
    }

    static inline int32x4 operator >> (int32x4 a, int b)
    {
        return simd::sra(a, b);
    }

    static inline int32x4 operator << (int32x4 a, uint32x4 b)
    {
        return simd::sll(a, b);
    }

    static inline int32x4 operator >> (int32x4 a, uint32x4 b)
    {
        return simd::sra(a, b);
    }

} // namespace mango

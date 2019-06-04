/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<s32, 4>
    {
        using VectorType = simd::s32x4;
        using ScalarType = s32;
        enum { VectorSize = 4 };

        union
        {
            simd::s32x4 m;

            ScalarAccessor<s32, simd::s32x4, 0> x;
            ScalarAccessor<s32, simd::s32x4, 1> y;
            ScalarAccessor<s32, simd::s32x4, 2> z;
            ScalarAccessor<s32, simd::s32x4, 3> w;

            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 0, 0> xxxx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 0, 0> yxxx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 0, 0> zxxx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 0, 0> wxxx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 0, 0> xyxx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 0, 0> yyxx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 0, 0> zyxx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 0, 0> wyxx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 0, 0> xzxx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 0, 0> yzxx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 0, 0> zzxx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 0, 0> wzxx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 0, 0> xwxx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 0, 0> ywxx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 0, 0> zwxx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 0, 0> wwxx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 1, 0> xxyx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 1, 0> yxyx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 1, 0> zxyx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 1, 0> wxyx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 1, 0> xyyx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 1, 0> yyyx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 1, 0> zyyx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 1, 0> wyyx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 1, 0> xzyx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 1, 0> yzyx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 1, 0> zzyx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 1, 0> wzyx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 1, 0> xwyx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 1, 0> ywyx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 1, 0> zwyx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 1, 0> wwyx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 2, 0> xxzx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 2, 0> yxzx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 2, 0> zxzx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 2, 0> wxzx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 2, 0> xyzx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 2, 0> yyzx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 2, 0> zyzx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 2, 0> wyzx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 2, 0> xzzx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 2, 0> yzzx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 2, 0> zzzx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 2, 0> wzzx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 2, 0> xwzx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 2, 0> ywzx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 2, 0> zwzx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 2, 0> wwzx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 3, 0> xxwx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 3, 0> yxwx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 3, 0> zxwx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 3, 0> wxwx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 3, 0> xywx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 3, 0> yywx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 3, 0> zywx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 3, 0> wywx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 3, 0> xzwx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 3, 0> yzwx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 3, 0> zzwx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 3, 0> wzwx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 3, 0> xwwx;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 3, 0> ywwx;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 3, 0> zwwx;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 3, 0> wwwx;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 0, 1> xxxy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 0, 1> yxxy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 0, 1> zxxy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 0, 1> wxxy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 0, 1> xyxy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 0, 1> yyxy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 0, 1> zyxy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 0, 1> wyxy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 0, 1> xzxy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 0, 1> yzxy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 0, 1> zzxy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 0, 1> wzxy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 0, 1> xwxy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 0, 1> ywxy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 0, 1> zwxy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 0, 1> wwxy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 1, 1> xxyy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 1, 1> yxyy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 1, 1> zxyy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 1, 1> wxyy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 1, 1> xyyy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 1, 1> yyyy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 1, 1> zyyy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 1, 1> wyyy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 1, 1> xzyy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 1, 1> yzyy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 1, 1> zzyy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 1, 1> wzyy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 1, 1> xwyy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 1, 1> ywyy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 1, 1> zwyy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 1, 1> wwyy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 2, 1> xxzy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 2, 1> yxzy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 2, 1> zxzy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 2, 1> wxzy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 2, 1> xyzy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 2, 1> yyzy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 2, 1> zyzy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 2, 1> wyzy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 2, 1> xzzy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 2, 1> yzzy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 2, 1> zzzy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 2, 1> wzzy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 2, 1> xwzy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 2, 1> ywzy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 2, 1> zwzy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 2, 1> wwzy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 3, 1> xxwy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 3, 1> yxwy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 3, 1> zxwy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 3, 1> wxwy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 3, 1> xywy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 3, 1> yywy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 3, 1> zywy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 3, 1> wywy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 3, 1> xzwy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 3, 1> yzwy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 3, 1> zzwy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 3, 1> wzwy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 3, 1> xwwy;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 3, 1> ywwy;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 3, 1> zwwy;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 3, 1> wwwy;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 0, 2> xxxz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 0, 2> yxxz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 0, 2> zxxz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 0, 2> wxxz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 0, 2> xyxz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 0, 2> yyxz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 0, 2> zyxz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 0, 2> wyxz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 0, 2> xzxz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 0, 2> yzxz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 0, 2> zzxz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 0, 2> wzxz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 0, 2> xwxz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 0, 2> ywxz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 0, 2> zwxz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 0, 2> wwxz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 1, 2> xxyz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 1, 2> yxyz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 1, 2> zxyz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 1, 2> wxyz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 1, 2> xyyz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 1, 2> yyyz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 1, 2> zyyz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 1, 2> wyyz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 1, 2> xzyz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 1, 2> yzyz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 1, 2> zzyz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 1, 2> wzyz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 1, 2> xwyz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 1, 2> ywyz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 1, 2> zwyz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 1, 2> wwyz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 2, 2> xxzz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 2, 2> yxzz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 2, 2> zxzz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 2, 2> wxzz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 2, 2> xyzz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 2, 2> yyzz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 2, 2> zyzz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 2, 2> wyzz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 2, 2> xzzz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 2, 2> yzzz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 2, 2> zzzz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 2, 2> wzzz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 2, 2> xwzz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 2, 2> ywzz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 2, 2> zwzz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 2, 2> wwzz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 3, 2> xxwz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 3, 2> yxwz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 3, 2> zxwz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 3, 2> wxwz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 3, 2> xywz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 3, 2> yywz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 3, 2> zywz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 3, 2> wywz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 3, 2> xzwz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 3, 2> yzwz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 3, 2> zzwz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 3, 2> wzwz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 3, 2> xwwz;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 3, 2> ywwz;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 3, 2> zwwz;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 3, 2> wwwz;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 0, 3> xxxw;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 0, 3> yxxw;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 0, 3> zxxw;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 0, 3> wxxw;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 0, 3> xyxw;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 0, 3> yyxw;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 0, 3> zyxw;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 0, 3> wyxw;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 0, 3> xzxw;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 0, 3> yzxw;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 0, 3> zzxw;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 0, 3> wzxw;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 0, 3> xwxw;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 0, 3> ywxw;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 0, 3> zwxw;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 0, 3> wwxw;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 1, 3> xxyw;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 1, 3> yxyw;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 1, 3> zxyw;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 1, 3> wxyw;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 1, 3> xyyw;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 1, 3> yyyw;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 1, 3> zyyw;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 1, 3> wyyw;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 1, 3> xzyw;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 1, 3> yzyw;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 1, 3> zzyw;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 1, 3> wzyw;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 1, 3> xwyw;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 1, 3> ywyw;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 1, 3> zwyw;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 1, 3> wwyw;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 2, 3> xxzw;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 2, 3> yxzw;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 2, 3> zxzw;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 2, 3> wxzw;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 2, 3> xyzw;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 2, 3> yyzw;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 2, 3> zyzw;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 2, 3> wyzw;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 2, 3> xzzw;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 2, 3> yzzw;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 2, 3> zzzw;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 2, 3> wzzw;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 2, 3> xwzw;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 2, 3> ywzw;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 2, 3> zwzw;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 2, 3> wwzw;
            ShuffleAccessor4<s32, simd::s32x4, 0, 0, 3, 3> xxww;
            ShuffleAccessor4<s32, simd::s32x4, 1, 0, 3, 3> yxww;
            ShuffleAccessor4<s32, simd::s32x4, 2, 0, 3, 3> zxww;
            ShuffleAccessor4<s32, simd::s32x4, 3, 0, 3, 3> wxww;
            ShuffleAccessor4<s32, simd::s32x4, 0, 1, 3, 3> xyww;
            ShuffleAccessor4<s32, simd::s32x4, 1, 1, 3, 3> yyww;
            ShuffleAccessor4<s32, simd::s32x4, 2, 1, 3, 3> zyww;
            ShuffleAccessor4<s32, simd::s32x4, 3, 1, 3, 3> wyww;
            ShuffleAccessor4<s32, simd::s32x4, 0, 2, 3, 3> xzww;
            ShuffleAccessor4<s32, simd::s32x4, 1, 2, 3, 3> yzww;
            ShuffleAccessor4<s32, simd::s32x4, 2, 2, 3, 3> zzww;
            ShuffleAccessor4<s32, simd::s32x4, 3, 2, 3, 3> wzww;
            ShuffleAccessor4<s32, simd::s32x4, 0, 3, 3, 3> xwww;
            ShuffleAccessor4<s32, simd::s32x4, 1, 3, 3, 3> ywww;
            ShuffleAccessor4<s32, simd::s32x4, 2, 3, 3, 3> zwww;
            ShuffleAccessor4<s32, simd::s32x4, 3, 3, 3, 3> wwww;
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

        Vector(s32 s)
            : m(simd::s32x4_set1(s))
        {
        }

        explicit Vector(s32 x, s32 y, s32 z, s32 w)
            : m(simd::s32x4_set4(x, y, z, w))
        {
        }

        Vector(simd::s32x4 v)
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

        Vector& operator = (simd::s32x4 v)
        {
            m = v;
            return *this;
        }

        Vector& operator = (s32 s)
        {
            m = simd::s32x4_set1(s);
            return *this;
        }

        operator simd::s32x4 () const
        {
            return m;
        }

#ifdef int128_is_hardware_vector
        operator simd::s32x4::vector () const
        {
            return m.data;
        }
#endif

        u32 pack() const
        {
            return simd::pack(m);
        }

        void unpack(u32 a)
        {
            m = simd::unpack(a);
        }
    };

    template <int x, int y, int z, int w>
    static inline Vector<s32, 4> shuffle(Vector<s32, 4> v)
    {
        return simd::shuffle<x, y, z, w>(v);
    }

    template <>
    inline Vector<s32, 4> load_low<s32, 4>(const s32 *source)
    {
        return simd::s32x4_load_low(source);
    }

    static inline void store_low(s32 *dest, Vector<s32, 4> v)
    {
        simd::s32x4_store_low(dest, v);
    }

    static inline const Vector<s32, 4> operator + (Vector<s32, 4> v)
    {
        return v;
    }

    static inline Vector<s32, 4> operator - (Vector<s32, 4> v)
    {
        return simd::sub(simd::s32x4_zero(), v);
    }

    static inline Vector<s32, 4>& operator += (Vector<s32, 4>& a, Vector<s32, 4> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<s32, 4>& operator -= (Vector<s32, 4>& a, Vector<s32, 4> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<s32, 4> operator + (Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<s32, 4> operator - (Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<s32, 4> nand(Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<s32, 4> operator & (Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<s32, 4> operator | (Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<s32, 4> operator ^ (Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<s32, 4> operator ~ (Vector<s32, 4> a)
    {
        return simd::bitwise_not(a);
    }

    static inline Vector<s32, 4> unpacklo(Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::unpacklo(a, b);
    }

    static inline Vector<s32, 4> unpackhi(Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::unpackhi(a, b);
    }

    static inline Vector<s32, 4> adds(Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<s32, 4> subs(Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<s32, 4> min(Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<s32, 4> max(Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<s32, 4> clamp(Vector<s32, 4> a, Vector<s32, 4> low, Vector<s32, 4> high)
    {
        return simd::clamp(a, low, high);
    }

    static inline mask32x4 operator > (Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline mask32x4 operator >= (Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline mask32x4 operator < (Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline mask32x4 operator <= (Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::compare_le(a, b);
    }

    static inline mask32x4 operator == (Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline mask32x4 operator != (Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<s32, 4> select(mask32x4 mask, Vector<s32, 4> a, Vector<s32, 4> b)
    {
        return simd::select(mask, a, b);
    }

    static inline Vector<s32, 4> operator << (Vector<s32, 4> a, int b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<s32, 4> operator >> (Vector<s32, 4> a, int b)
    {
        return simd::sra(a, b);
    }

    static inline Vector<s32, 4> operator << (Vector<s32, 4> a, Vector<u32, 4> b)
    {
        return simd::sll(a, b);
    }

    static inline Vector<s32, 4> operator >> (Vector<s32, 4> a, Vector<u32, 4> b)
    {
        return simd::sra(a, b);
    }

} // namespace mango

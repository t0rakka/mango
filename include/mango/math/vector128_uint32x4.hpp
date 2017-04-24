/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"

namespace mango
{

    template <>
    struct Vector<uint32, 4> : simd::VectorBase<uint32, 4>
    {
        union
        {
            simd::uint32x4 xyzw;

            simd::ScalarAccessor<uint32, simd::uint32x4, 0> x;
            simd::ScalarAccessor<uint32, simd::uint32x4, 1> y;
            simd::ScalarAccessor<uint32, simd::uint32x4, 2> z;
            simd::ScalarAccessor<uint32, simd::uint32x4, 3> w;

            simd::Permute4<uint32, simd::uint32x4, 0, 0, 0, 0> xxxx;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 0, 0> yxxx;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 0, 0> zxxx;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 0, 0> wxxx;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 0, 0> xyxx;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 0, 0> yyxx;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 0, 0> zyxx;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 0, 0> wyxx;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 0, 0> xzxx;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 0, 0> yzxx;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 0, 0> zzxx;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 0, 0> wzxx;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 0, 0> xwxx;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 0, 0> ywxx;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 0, 0> zwxx;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 0, 0> wwxx;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 1, 0> xxyx;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 1, 0> yxyx;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 1, 0> zxyx;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 1, 0> wxyx;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 1, 0> xyyx;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 1, 0> yyyx;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 1, 0> zyyx;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 1, 0> wyyx;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 1, 0> xzyx;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 1, 0> yzyx;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 1, 0> zzyx;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 1, 0> wzyx;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 1, 0> xwyx;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 1, 0> ywyx;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 1, 0> zwyx;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 1, 0> wwyx;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 2, 0> xxzx;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 2, 0> yxzx;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 2, 0> zxzx;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 2, 0> wxzx;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 2, 0> xyzx;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 2, 0> yyzx;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 2, 0> zyzx;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 2, 0> wyzx;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 2, 0> xzzx;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 2, 0> yzzx;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 2, 0> zzzx;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 2, 0> wzzx;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 2, 0> xwzx;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 2, 0> ywzx;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 2, 0> zwzx;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 2, 0> wwzx;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 3, 0> xxwx;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 3, 0> yxwx;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 3, 0> zxwx;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 3, 0> wxwx;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 3, 0> xywx;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 3, 0> yywx;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 3, 0> zywx;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 3, 0> wywx;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 3, 0> xzwx;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 3, 0> yzwx;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 3, 0> zzwx;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 3, 0> wzwx;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 3, 0> xwwx;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 3, 0> ywwx;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 3, 0> zwwx;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 3, 0> wwwx;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 0, 1> xxxy;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 0, 1> yxxy;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 0, 1> zxxy;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 0, 1> wxxy;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 0, 1> xyxy;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 0, 1> yyxy;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 0, 1> zyxy;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 0, 1> wyxy;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 0, 1> xzxy;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 0, 1> yzxy;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 0, 1> zzxy;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 0, 1> wzxy;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 0, 1> xwxy;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 0, 1> ywxy;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 0, 1> zwxy;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 0, 1> wwxy;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 1, 1> xxyy;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 1, 1> yxyy;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 1, 1> zxyy;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 1, 1> wxyy;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 1, 1> xyyy;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 1, 1> yyyy;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 1, 1> zyyy;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 1, 1> wyyy;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 1, 1> xzyy;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 1, 1> yzyy;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 1, 1> zzyy;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 1, 1> wzyy;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 1, 1> xwyy;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 1, 1> ywyy;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 1, 1> zwyy;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 1, 1> wwyy;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 2, 1> xxzy;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 2, 1> yxzy;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 2, 1> zxzy;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 2, 1> wxzy;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 2, 1> xyzy;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 2, 1> yyzy;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 2, 1> zyzy;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 2, 1> wyzy;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 2, 1> xzzy;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 2, 1> yzzy;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 2, 1> zzzy;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 2, 1> wzzy;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 2, 1> xwzy;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 2, 1> ywzy;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 2, 1> zwzy;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 2, 1> wwzy;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 3, 1> xxwy;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 3, 1> yxwy;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 3, 1> zxwy;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 3, 1> wxwy;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 3, 1> xywy;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 3, 1> yywy;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 3, 1> zywy;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 3, 1> wywy;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 3, 1> xzwy;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 3, 1> yzwy;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 3, 1> zzwy;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 3, 1> wzwy;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 3, 1> xwwy;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 3, 1> ywwy;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 3, 1> zwwy;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 3, 1> wwwy;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 0, 2> xxxz;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 0, 2> yxxz;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 0, 2> zxxz;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 0, 2> wxxz;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 0, 2> xyxz;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 0, 2> yyxz;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 0, 2> zyxz;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 0, 2> wyxz;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 0, 2> xzxz;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 0, 2> yzxz;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 0, 2> zzxz;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 0, 2> wzxz;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 0, 2> xwxz;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 0, 2> ywxz;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 0, 2> zwxz;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 0, 2> wwxz;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 1, 2> xxyz;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 1, 2> yxyz;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 1, 2> zxyz;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 1, 2> wxyz;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 1, 2> xyyz;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 1, 2> yyyz;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 1, 2> zyyz;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 1, 2> wyyz;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 1, 2> xzyz;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 1, 2> yzyz;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 1, 2> zzyz;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 1, 2> wzyz;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 1, 2> xwyz;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 1, 2> ywyz;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 1, 2> zwyz;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 1, 2> wwyz;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 2, 2> xxzz;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 2, 2> yxzz;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 2, 2> zxzz;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 2, 2> wxzz;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 2, 2> xyzz;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 2, 2> yyzz;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 2, 2> zyzz;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 2, 2> wyzz;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 2, 2> xzzz;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 2, 2> yzzz;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 2, 2> zzzz;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 2, 2> wzzz;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 2, 2> xwzz;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 2, 2> ywzz;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 2, 2> zwzz;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 2, 2> wwzz;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 3, 2> xxwz;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 3, 2> yxwz;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 3, 2> zxwz;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 3, 2> wxwz;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 3, 2> xywz;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 3, 2> yywz;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 3, 2> zywz;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 3, 2> wywz;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 3, 2> xzwz;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 3, 2> yzwz;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 3, 2> zzwz;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 3, 2> wzwz;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 3, 2> xwwz;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 3, 2> ywwz;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 3, 2> zwwz;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 3, 2> wwwz;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 0, 3> xxxw;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 0, 3> yxxw;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 0, 3> zxxw;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 0, 3> wxxw;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 0, 3> xyxw;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 0, 3> yyxw;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 0, 3> zyxw;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 0, 3> wyxw;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 0, 3> xzxw;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 0, 3> yzxw;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 0, 3> zzxw;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 0, 3> wzxw;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 0, 3> xwxw;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 0, 3> ywxw;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 0, 3> zwxw;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 0, 3> wwxw;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 1, 3> xxyw;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 1, 3> yxyw;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 1, 3> zxyw;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 1, 3> wxyw;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 1, 3> xyyw;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 1, 3> yyyw;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 1, 3> zyyw;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 1, 3> wyyw;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 1, 3> xzyw;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 1, 3> yzyw;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 1, 3> zzyw;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 1, 3> wzyw;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 1, 3> xwyw;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 1, 3> ywyw;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 1, 3> zwyw;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 1, 3> wwyw;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 2, 3> xxzw;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 2, 3> yxzw;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 2, 3> zxzw;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 2, 3> wxzw;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 2, 3> yyzw;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 2, 3> zyzw;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 2, 3> wyzw;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 2, 3> xzzw;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 2, 3> yzzw;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 2, 3> zzzw;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 2, 3> wzzw;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 2, 3> xwzw;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 2, 3> ywzw;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 2, 3> zwzw;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 2, 3> wwzw;
            simd::Permute4<uint32, simd::uint32x4, 0, 0, 3, 3> xxww;
            simd::Permute4<uint32, simd::uint32x4, 1, 0, 3, 3> yxww;
            simd::Permute4<uint32, simd::uint32x4, 2, 0, 3, 3> zxww;
            simd::Permute4<uint32, simd::uint32x4, 3, 0, 3, 3> wxww;
            simd::Permute4<uint32, simd::uint32x4, 0, 1, 3, 3> xyww;
            simd::Permute4<uint32, simd::uint32x4, 1, 1, 3, 3> yyww;
            simd::Permute4<uint32, simd::uint32x4, 2, 1, 3, 3> zyww;
            simd::Permute4<uint32, simd::uint32x4, 3, 1, 3, 3> wyww;
            simd::Permute4<uint32, simd::uint32x4, 0, 2, 3, 3> xzww;
            simd::Permute4<uint32, simd::uint32x4, 1, 2, 3, 3> yzww;
            simd::Permute4<uint32, simd::uint32x4, 2, 2, 3, 3> zzww;
            simd::Permute4<uint32, simd::uint32x4, 3, 2, 3, 3> wzww;
            simd::Permute4<uint32, simd::uint32x4, 0, 3, 3, 3> xwww;
            simd::Permute4<uint32, simd::uint32x4, 1, 3, 3, 3> ywww;
            simd::Permute4<uint32, simd::uint32x4, 2, 3, 3, 3> zwww;
            simd::Permute4<uint32, simd::uint32x4, 3, 3, 3, 3> wwww;
        };

        explicit Vector() = default;

        explicit Vector(uint32 s)
            : xyzw(simd::uint32x4_set1(s))
        {
        }

        Vector(uint32 x, uint32 y, uint32 z, uint32 w)
            : xyzw(simd::uint32x4_set4(x, y, z, w))
        {
        }

        Vector(simd::uint32x4 v)
            : xyzw(v)
        {
        }

        template <int X, int Y, int Z, int W>
        Vector(const simd::Permute4<uint32, simd::uint32x4, X, Y, Z, W>& p)
        {
            *this = p;
        }

        template <int X, int Y, int Z, int W>
        Vector& operator = (const simd::Permute4<uint32, simd::uint32x4, X, Y, Z, W>& p)
        {
            *this = p;
            return *this;
        }

        Vector& operator = (simd::uint32x4 v)
        {
            xyzw = v;
            return *this;
        }

        Vector& operator = (uint32 s)
        {
            xyzw = simd::uint32x4_set1(s);
            return *this;
        }

        operator simd::uint32x4 () const
        {
            return xyzw;
        }

        operator simd::uint32x4 ()
        {
            return xyzw;
        }
    };

    static inline const Vector<uint32, 4> operator + (Vector<uint32, 4> v)
    {
        return v;
    }

    static inline Vector<uint32, 4> operator - (Vector<uint32, 4> v)
    {
        return simd::sub(simd::uint32x4_zero(), v);
    }

    static inline Vector<uint32, 4>& operator += (Vector<uint32, 4>& a, Vector<uint32, 4> b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<uint32, 4>& operator += (Vector<uint32, 4>& a, uint32 b)
    {
        a = simd::add(a, b);
        return a;
    }

    static inline Vector<uint32, 4>& operator -= (Vector<uint32, 4>& a, Vector<uint32, 4> b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint32, 4>& operator -= (Vector<uint32, 4>& a, uint32 b)
    {
        a = simd::sub(a, b);
        return a;
    }

    static inline Vector<uint32, 4> operator + (Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint32, 4> operator + (Vector<uint32, 4> a, uint32 b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint32, 4> operator + (uint32 a, Vector<uint32, 4> b)
    {
        return simd::add(a, b);
    }

    static inline Vector<uint32, 4> operator - (Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint32, 4> operator - (Vector<uint32, 4> a, uint32 b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint32, 4> operator - (uint32 a, Vector<uint32, 4> b)
    {
        return simd::sub(a, b);
    }

    static inline Vector<uint32, 4> nand(Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::bitwise_nand(a, b);
    }

    static inline Vector<uint32, 4> operator & (Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::bitwise_and(a, b);
    }

    static inline Vector<uint32, 4> operator | (Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::bitwise_or(a, b);
    }

    static inline Vector<uint32, 4> operator ^ (Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::bitwise_xor(a, b);
    }

    static inline Vector<uint32, 4> operator ~ (Vector<uint32, 4> a)
    {
        return simd::bitwise_not(a);
    }

    static inline Vector<uint32, 4> adds(Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::adds(a, b);
    }

    static inline Vector<uint32, 4> subs(Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::subs(a, b);
    }

    static inline Vector<uint32, 4> min(Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::min(a, b);
    }

    static inline Vector<uint32, 4> max(Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::max(a, b);
    }

    static inline Vector<uint32, 4> clamp(Vector<uint32, 4> a, Vector<uint32, 4> amin, Vector<uint32, 4> amax)
    {
        return simd::clamp(a, amin, amax);
    }

    static inline Vector<uint32, 4> operator > (Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::compare_gt(a, b);
    }

    static inline Vector<uint32, 4> operator >= (Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::compare_ge(a, b);
    }

    static inline Vector<uint32, 4> operator < (Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::compare_lt(a, b);
    }

    static inline Vector<uint32, 4> operator <= (Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::compare_le(a, b);
    }

    static inline Vector<uint32, 4> operator == (Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::compare_eq(a, b);
    }

    static inline Vector<uint32, 4> operator != (Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::compare_neq(a, b);
    }

    static inline Vector<uint32, 4> select(Vector<uint32, 4> mask, Vector<uint32, 4> a, Vector<uint32, 4> b)
    {
        return simd::select(mask, a, b);
    }

} // namespace mango

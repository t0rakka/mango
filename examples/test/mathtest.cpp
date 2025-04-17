/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/math/math.hpp>
#include <mango/core/system.hpp>

using namespace mango;
using namespace mango::math;

// ----------------------------------------------------------------------
// permutations
// ----------------------------------------------------------------------

/*

    vector * vector
    vector * shuffle_accessor (vector)
    vector * scalar_accesor (scalar)
    vector * scalar

    shuffle_accessor * vector
    shuffle_accessor * shuffle_accessor (vector)
    shuffle_accessor * scalar_accesor (scalar)
    shuffle_accessor * scalar

    scalar_accesor * vector
    scalar_accesor * shuffle_accessor (vector)
    scalar_accesor * scalar_accesor (scalar)
    scalar_accesor * scalar

    scalar * vector
    scalar * shuffle_accessor (vector)
    scalar * scalar_accesor (scalar)
    scalar * scalar   <-- use default scalar operators

*/

bool eq(float a, float b)
{
    // "good enough"
    return std::abs(a - b) < 0.00001f;
}

void print(const char* text, float32x4 v, float x, float y, float z, float w)
{
    bool identical = eq(v.x, x) && eq(v.y, y) && eq(v.z, z) && eq(v.w, w);
    const char* status = identical ? "OK" : "FAILED";
    printf("%s %f %f %f %f %s\n", text, float(v.x), float(v.y), float(v.z), float(v.w), status);
}

template <typename ScalarType>
void print(const Vector<ScalarType, 4>& v)
{
    float x = v.x;
    float y = v.y;
    float z = v.z;
    float w = v.w;
    printLine("{} {} {} {}", x, y, z, w);
}

void test1(float32x4 a, float32x4 b)
{
    a = b.x;
    print("[test1] a:", a, 0.2f, 0.2f, 0.2f, 0.2f);
}

void test2(float32x4 a, float32x4 b)
{
    a.x = b.x;
    print("[test2] a:", a, 0.2f, 2.0f, 3.0f, 4.0f);
}

void test3(float32x4 a, float32x2 b)
{
    a.x = b.x;
    print("[test3] a:", a, 7.0f, 2.0f, 3.0f, 4.0f);
}

void test4(float32x4 a, float32x3 b)
{
    a.y = b.z;
    print("[test4] a:", a, 1.0f, 5.0f, 3.0f, 4.0f);
}

void test5(float32x4 a, float32x4 b)
{
    a = a * b.x;
    print("[test5] a:", a, 0.2f, 0.4f, 0.6f, 0.8f);
}

void test6(float32x4 a, float32x4 b)
{
    a = a.xxyy * (b.w + 1.0f) + b.x * a.yywx / (b.xxzz + 2.0f);
    print("[test6] a:", a, 1.981818f, 1.981818f, 3.907692f, 3.676923f);
}

void test7(float32x4 a, float32x4 b)
{
    a.x = b.x;
    a.y += b.y;
    a.z *= b.z;
    a.w /= b.w;
    print("[test7] a:", a, 0.2f, 2.4f, 1.8f, 5.0f);
}

void test8(float32x4 a, float32x4 b)
{
    float32x4 c;
    c = a.xxzz + b * 2.0f * a.x;
    c.x = b.x;
    c = a.xxyy * (b.w + 1.0f) + b.x * a.yywx / (b.xxzz + 2.0f);
    c = (a.x * b.y - 2.0f) * b.xxxx + a * min(a.xzzw, b.yywx) * std::clamp<float>(a.z, 1.0f, 2.0f);
}

void test()
{
    float32x4 a(1.00f, 2.00f, 3.00f, 4.00f);
    float32x4 b(0.20f, 0.40f, 0.60f, 0.80f);
    float32x2 c(7.0f);
    float32x3 d(5.0f);

    test1(a, b);
    test2(a, b);
    test3(a, c);
    test4(a, d);
    test5(a, b);
    test6(a, b);
    test7(a, b);
    test8(a, b);
}

// ----------------------------------------------------------------------
// vec2, vec4
// ----------------------------------------------------------------------

void test_vec_size()
{
    /* These must be exact sizes, no padding allowed as they will be used as
       storage for rendering APIs like D3D12 and Vulkan.
    */

    // half vectors
    static_assert(sizeof(float16x4) == 8, "float16x4 size == 8");

    // float vectors
    static_assert(sizeof(float32x2)  ==  8, "float32x2 size == 8");
    static_assert(sizeof(float32x3)  == 12, "float32x3 size == 12");
    static_assert(sizeof(float32x4)  == 16, "float32x4 size == 16");
    static_assert(sizeof(float32x8)  == 32, "float32x8 size == 32");
    static_assert(sizeof(float32x16) == 64, "float32x16 size == 64");

    // double vectors
    static_assert(sizeof(float64x2) == 16, "float64x2 size == 16");
    static_assert(sizeof(float64x3) == 24, "float64x3 size == 24");
    static_assert(sizeof(float64x4) == 32, "float64x4 size == 32");
    static_assert(sizeof(float64x8) == 64, "float64x8 size == 64");

    // integer vectors
    static_assert(sizeof(int32x2)  ==  8, "int32x2 size == 8");
    static_assert(sizeof(int32x3)  == 12, "int32x3 size == 12");
    static_assert(sizeof(uint32x2) ==  8, "uint32x2 size == 8");
    static_assert(sizeof(uint32x3) == 12, "uint32x3 size == 12");

    // 128 bit integer vectors
    static_assert(sizeof(int8x16)  == 16, "int8x16 size == 16");
    static_assert(sizeof(int16x8)  == 16, "int16x8 size == 16");
    static_assert(sizeof(int32x4)  == 16, "int32x4 size == 16");
    static_assert(sizeof(int64x2)  == 16, "int64x2 size == 16");
    static_assert(sizeof(uint8x16) == 16, "uint8x16 size == 16");
    static_assert(sizeof(uint16x8) == 16, "uint16x8 size == 16");
    static_assert(sizeof(uint32x4) == 16, "uint32x4 size == 16");
    static_assert(sizeof(uint64x2) == 16, "uint64x2 size == 16");

    // 256 bit integer vectors
    static_assert(sizeof(int8x32)   == 32, "int8x32 size == 32");
    static_assert(sizeof(int16x16)  == 32, "int16x16 size == 32");
    static_assert(sizeof(int32x8)   == 32, "int32x8 size == 32");
    static_assert(sizeof(int64x4)   == 32, "int64x4 size == 32");
    static_assert(sizeof(uint8x32)  == 32, "uint8x32 size == 32");
    static_assert(sizeof(uint16x16) == 32, "uint16x16 size == 32"); 
    static_assert(sizeof(uint32x8)  == 32, "uint32x8 size == 32");
    static_assert(sizeof(uint64x4)  == 32, "uint64x4 size == 32");

    // 512 bit integer vectors
    static_assert(sizeof(int8x64)   == 64, "int8x64 size == 64");
    static_assert(sizeof(int16x32)  == 64, "int16x32 size == 64");
    static_assert(sizeof(int32x16)  == 64, "int32x16 size == 64");
    static_assert(sizeof(int64x8)   == 64, "int64x8 size == 64"); 
    static_assert(sizeof(uint8x64)  == 64, "uint8x64 size == 64");
    static_assert(sizeof(uint16x32) == 64, "uint16x32 size == 64");
    static_assert(sizeof(uint32x16) == 64, "uint32x16 size == 64");
    static_assert(sizeof(uint64x8)  == 64, "uint64x8 size == 64"); 
}

void test_vec2()
{
    float32x2 a(1.0f, 2.0f);
    float32x2 r0 = a.xx;
    MANGO_UNREFERENCED(r0);
}

template <typename ScalarType>
void test_vec4()
{
    using VectorType = Vector<ScalarType, 4>;

    VectorType a(ScalarType(1), ScalarType(2), ScalarType(3), ScalarType(4));
    VectorType b(ScalarType(5), ScalarType(6), ScalarType(7), ScalarType(8));

    VectorType v0 = a + b;
    VectorType v1 = a + b.wzxy;
    VectorType v2 = a + b.y;
    VectorType v3 = a + ScalarType(2);

    MANGO_UNREFERENCED(v0);
    MANGO_UNREFERENCED(v1);
    MANGO_UNREFERENCED(v2);
    MANGO_UNREFERENCED(v3);
}

// ----------------------------------------------------------------------
// float32x3, float32x4
// ----------------------------------------------------------------------

void test_float32x3()
{
    float32x3 a(1.0f, 2.0f, 3.0f);
    auto r0 = a.xxz;
    auto r1 = a.xz;
    MANGO_UNREFERENCED(r0);
    MANGO_UNREFERENCED(r1);
}

void test_float32x4()
{
    float32x4 a(1.0f, 2.0f, 3.0f, 4.0f);
    float32x4 b(5.0f, 6.0f, 7.0f, 8.0f);

    float32x4 v0 = a * b;
    float32x4 v1 = a * b.xyzw;
    float32x4 v2 = a * b.x;
    float32x4 v3 = a * 1.5f;

    float32x4 v4 = b.xyyw * a;
    float32x4 v5 = a.xyww * b.xxzw + a * b.x;
    float32x4 v6 = b.xyyw * b.y;
    float32x4 v7 = b.xyxw * 1.5f;

    float32x4 s0 = a.y * b;
    float32x4 s1 = a.y * b.xyww;
    float s2 = a.y * b.z;
    float s3 = a.y * 1.5f;

    float32x4 s4 = 1.5f * b;
    float32x4 s5 = 1.5f * b.xyzx;
    float s6 = 1.5f * b.z;
    float s7 = 1.5f * 1.5f;

    v7 = float32x4(b.xywz).xxyy * 1.2f * a.wxzz / b.z;

    v0 += 1.0f;
    v0 += b.x;
    v0 += b.xxyy;
    v0 -= 1.0f;
    v0 -= b.x;
    v0 -= b.xxyy;
    v0 *= 2.0f;
    v0 *= b.y;
    v0 *= b.yzwx;
    v0 /= 2.0f;
    v0 /= b.z;
    v0 /= b.yzwx;

    v0.x = v1.y;
    v0.x = -v1.y;
    v0.x = +v1.y;
    v0.x += v1.y;
    v0.x -= v1.y;
    v0.x *= v1.y;
    v0.x /= v1.y;

    int sum = 0;
    sum += v0.x < v1.y;
    sum += 1.0f < v1.y;
    sum += v0.x < 1.0f;                                                                                                                                                                                           

    v0 = abs(v0);
    v0 = abs(v0 + v1);
    v0 = abs(v1.xzwx);
    v0 = abs(v0.xxyy + v1.xzwx);

    v0 = sign(v0);
    v0 = sign(v0 + v1);
    v0 = sign(v1.xzwx);
    v0 = sign(v0.xxyy + v1.xzwx);

    v0 = fract(v0);
    v0 = fract(v0 + v1);
    v0 = fract(v1.xzwx);
    v0 = fract(v0.xxyy + v1.xzwx);

    v0 = min(v0, v1);
    v0 = min(v0, v1 + v2);
    v0 = min(v0, v1.xzwx);
    v0 = min(v0, v1.xxyy + v2.xzwx);
    v0 = min(v0.xxyy + v1.xywz, v1.xywz + v2.xywz);
    //v0 = min(v0.xyzz, 1.0f);

    v0 = max(v0, v1);
    v0 = max(v0, v1 + v2);
    v0 = max(v0, v1.xzwx);
    v0 = max(v0, v1.xxyy + v2.xzwx);
    v0 = max(v0.xxyy + v1.xywz, v1.xywz + v2.xywz);

    v0 = lerp(v0, v1, 0.5f);
    v0 = lerp(v0, v1, v2.y);
    v0 = lerp(v0.xxzz, v1.wyxz + v2, (v1.xxzz + v2.xywz).w);

    v0 = unpacklo(v1, v2);
    v0 = unpackhi(v1, v2);

    v0 = ~v1;
    v0 = ~v1.xwyz;
    v0 = ~(v1 + v2);

    v0 = v1 & v2;
    v0 = v1 & v2.x;
    v0 = v1.x & v2.xwwy;
    v0 = v1.xwyz & v2.xwwy;

    v0 = v1 | v2;
    v0 = v1 | v2.x;
    v0 = v1.x | v2.xwwy;
    v0 = v1.xwyz | v2.xwwy;

    v0 = v1 ^ v2;
    v0 = v1 ^ v2.x;
    v0 = v1.x ^ v2.xwwy;
    v0 = v1.xwyz ^ v2.xwwy;

    v0 = nand(v1, v2);
    v0 = nand(v1, v2.x);
    v0 = nand(v1.x, v2.xwwy);
    v0 = nand(v1.xwyz, v2.xwwy);

    auto m0 = v0 > v1;
    m0 = v0 > v1 + v2;
    m0 = v0 > v1.xzwx;
    m0 = v0 > v1.xxyy + v2.xzwx;
    m0 = v0 > v0.xxyy + v1.xywz;
    m0 = v0 > v1.xywz + v2.xywz;
    m0 = 1.0f > v0;
    m0 = 1.0f > (v0 + v1);
    m0 = 1.0f > v0.xzwx;
    m0 = v0 > 1.0f;
    m0 = (v0 + v1) > 1.0f;
    m0 = v0.xxyy > 1.0f;
    m0 = v0.x > (v0 + v1).xxzw;

    auto m1 = v0 >= v1;
    m1 = v0 >= v1 + v2;
    m1 = v0 >= v1.xzwx;
    m1 = v0 >= v1.xxyy + v2.xzwx;
    m1 = v0 >= v0.xxyy + v1.xywz;
    m1 = v0 >= v1.xywz + v2.xywz;
    m1 = 1.0f >= v0;
    m1 = 1.0f >= (v0 + v1);
    m1 = 1.0f >= v0.xzwx;
    m1 = v0 >= 1.0f;
    m1 = (v0 + v1) >= 1.0f;
    m1 = v0.xxyy >= 1.0f;
    m1 = v0.x >= (v0 + v1).xxzw;

    auto m2 = v0 < v1;
    m2 = v0 < v1 + v2;
    m2 = v0 < v1.xzwx;
    m2 = v0 < v1.xxyy + v2.xzwx;
    m2 = v0 < v0.xxyy + v1.xywz;
    m2 = v0 < v1.xywz + v2.xywz;
    m2 = 1.0f < v0;
    m2 = 1.0f < (v0 + v1);
    m2 = 1.0f < v0.xzwx;
    m2 = v0 < 1.0f;
    m2 = (v0 + v1) < 1.0f;
    m2 = v0.xxyy < 1.0f;
    m2 = v0.x < (v0 + v1).xxzw;

    auto m3 = v0 <= v1;
    m3 = v0 <= v1 + v2;
    m3 = v0 <= v1.xzwx;
    m3 = v0 <= v1.xxyy + v2.xzwx;
    m3 = v0 <= v0.xxyy + v1.xywz;
    m3 = v0 <= v1.xywz + v2.xywz;
    m3 = 1.0f <= v0;
    m3 = 1.0f <= (v0 + v1);
    m3 = 1.0f <= v0.xzwx;
    m3 = v0 <= 1.0f;
    m3 = (v0 + v1) <= 1.0f;
    m3 = v0.xxyy <= 1.0f;
    m3 = v0.x <= (v0 + v1).xxzw;

    auto m4 = v0 == v1;
    m4 = v0 == v1 + v2;
    m4 = v0 == v1.xzwx;
    m4 = v0 == v1.xxyy + v2.xzwx;
    m4 = v0 == v0.xxyy + v1.xywz;
    m4 = v0 == v1.xywz + v2.xywz;
    m4 = 1.0f == v0;
    m4 = 1.0f == (v0 + v1);
    m4 = 1.0f == v0.xzwx;
    m4 = v0 == 1.0f;
    m4 = (v0 + v1) == 1.0f;
    m4 = v0.xxyy == 1.0f;
    m4 = v0.x == (v0 + v1).xxzw;

    auto m5 = v0 != v1;
    m5 = v0 != v1 + v2;
    m5 = v0 != v1.xzwx;
    m5 = v0 != v1.xxyy + v2.xzwx;
    m5 = v0 != v0.xxyy + v1.xywz;
    m5 = v0 != v1.xywz + v2.xywz;
    m5 = 1.0f != v0;
    m5 = 1.0f != (v0 + v1);
    m5 = 1.0f != v0.xzwx;
    m5 = v0 != 1.0f;
    m5 = (v0 + v1) != 1.0f;
    m5 = v0.xxyy != 1.0f;
    m5 = v0.x != (v0 + v1).xxzw;

    MANGO_UNREFERENCED(v0);
    MANGO_UNREFERENCED(v1);
    MANGO_UNREFERENCED(v2);
    MANGO_UNREFERENCED(v3);
    MANGO_UNREFERENCED(v4);
    MANGO_UNREFERENCED(v5);
    MANGO_UNREFERENCED(v6);
    MANGO_UNREFERENCED(v7);
    MANGO_UNREFERENCED(s0);
    MANGO_UNREFERENCED(s1);
    MANGO_UNREFERENCED(s2);
    MANGO_UNREFERENCED(s3);
    MANGO_UNREFERENCED(s4);
    MANGO_UNREFERENCED(s5);
    MANGO_UNREFERENCED(s6);
    MANGO_UNREFERENCED(s7);
    MANGO_UNREFERENCED(sum);
    MANGO_UNREFERENCED(m0);
    MANGO_UNREFERENCED(m1);
    MANGO_UNREFERENCED(m2);
    MANGO_UNREFERENCED(m3);
    MANGO_UNREFERENCED(m4);
    MANGO_UNREFERENCED(m5);
}

/*
void test_float32x4()
{
    float32x4 a(1.0f, 2.0f, 3.0f, 4.0f);
    float32x4 b(5.0f, 6.0f, 7.0f, 8.0f);

    float32x4 c = a + b + 1.0f;
    c = 2.0f * a;
    c = 4 * a;

    MANGO_UNREFERENCED(c);


    print(a * b);
    print(a * b.xyzw);
    print(a * 5.0f);
    print(a * b.x);
    print(b.x * a.xyzw);

    a = a + b;
    a /= b.x;
    print(a);

    a = a / b.x;
    print(a);

    {
        float32x4 v4(1.0f, 2.0f, 3.0f, 4.0f);
        float32x3 v3(1.0f, 2.0f, 3.0f);
        printLine("v4: {}", length(v4));
        printLine("v3: {}", length(v3));
    }
}
*/

int main()
{
    test();
    test_vec_size();
    test_vec2();
    test_vec4<float>();
    test_vec4<double>();
    test_vec4<s32>();
    test_vec4<u32>();
    test_vec4<s64>();
    test_vec4<u64>();
    test_float32x3();
    test_float32x4();
}

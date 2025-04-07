/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/math/math.hpp>
#include <mango/core/system.hpp>

using namespace mango;
using namespace mango::math;

// ----------------------------------------------------------------------
// float32x4
// ----------------------------------------------------------------------

void print(float32x4 v)
{
    float x = v.x;
    float y = v.y;
    float z = v.z;
    float w = v.w;
    printLine("{} {} {} {}", x, y, z, w);
}

void test_multiply()
{
    float32x4 a(1.0f, 2.0f, 3.0f, 4.0f);
    float32x4 b(5.0f, 6.0f, 7.0f, 8.0f);

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

    float32x4 v0 = a * b;
    float32x4 v1 = a * b.xyzw;
    float32x4 v2 = a * b.x;
    float32x4 v3 = a * 1.5f;

    float32x4 v4 = b.xyzw * a;
    float32x4 v5 = b.xyzw * b.xyzw;
    float32x4 v6 = b.xyzw * b.y;
    float32x4 v7 = b.xyzw * 1.5f;

    float32x4 s0 = a.y * b;
    float32x4 s1 = a.y * b.xyzw;
    float s2 = a.y * b.z;
    float s3 = a.y * 1.5f;

    float32x4 s4 = 1.5f * b;
    float32x4 s5 = 1.5f * b.xyzw;
    float s6 = 1.5f * b.z;
    float s7 = 1.5f * 1.5f;

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
    v0.x += v1.y;
    v0.x -= v1.y;
    v0.x *= v1.y;
    v0.x /= v1.y;

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
}

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

int main()
{
    test_float32x4();
    test_multiply();
}

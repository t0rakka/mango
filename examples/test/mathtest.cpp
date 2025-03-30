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
    printf("%f %f %f %f\n", x, y, z, w);
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
}

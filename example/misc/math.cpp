/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <vector>
#include <mango/math/math.hpp>

using namespace mango;
using namespace mango::math;

void example1()
{
    float32x4 a(1.0f, 2.0f, 3.0f, 4.0f);
    float32x4 b = sin(a);
    float32x4 c = cross(a, b) * 1.5f - a * 2.0f * dot(a, b * 3.0f);

    MANGO_UNREFERENCED(c);
}

float32x4 example2(const float32x4& a, const float32x4& b)
{
    float32x4 ab = a.xxyy * 2.0f - b * b.wwww;
    return ab / ab.x;
}

float32x4 example3(const float32x4& a, const float32x4& b)
{
    // select() is a bit more exotic operation; it uses a mask to select between
    // two values. The mask is generated in the comparison operator. This is useful
    // for avoiding branching; it is sometimes more efficient to compute both
    // results and choose one depending on some predicate (in this case a > b).

    float32x4 result = select(a > b, sin(a), cos(b));
    return result;
}

float32x4 example4(const float32x4& a, const float32x4& b)
{
    float32x4 result;

    // Same as example3 but using scalars instead of select()
    for (int i = 0; i < 4; ++i)
    {
        result[i] = a[i] > b[i] ? sin(a[i]) : cos(b[i]);
    }

    return result;
}

float32x3 example5(float32x3 a, float32x3 b, float32x3 c)
{
    // compute triangle normal given three vertices (a, b, c)
    float32x3 normal = cross(a - b, a - c);
    return normalize(normal);
}

float32x4 example6(float32x4 a)
{
    // a slower way to do a.zwxy to demonstrate decomposing vectors
    float32x2 low = a.xy;
    float32x2 high = a.zw;
    return float32x4(high, low);
}

void example7(float32x3 normal, float dist)
{
    Plane plane(normal, dist);
    float32x3 p(20.0f, 0.0f, 0.0f);
    float distanceToPlane = plane.distance(p);
    if (distanceToPlane < 0)
    {
        // point p is behind the plane
    }
}

void example8(const Plane& plane, float32x3 point0, float32x3 point1)
{
    Ray ray(point0, point1);

    Intersect is;
    if (is.intersect(ray, plane))
    {
        // compute point of intersection
        float32x3 p = ray.origin + ray.direction * is.t0;
        MANGO_UNREFERENCED(p);
    }
}

void example9(const Sphere& sphere, const Ray& ray)
{
    IntersectRange is;
    if (is.intersect(ray, sphere))
    {
        // compute points where ray enters and leaves the sphere
        float32x3 enter = ray.origin + ray.direction * is.t0;
        float32x3 leave = ray.origin + ray.direction * is.t1;
        MANGO_UNREFERENCED(enter);
        MANGO_UNREFERENCED(leave);
    }
}

void example10(const std::vector<Box>& boxes, const Ray& ray)
{
    // FastRay has precomputed data to accelerate intersection computations
    // However, it has overhead for doing this computation so it is best used
    // when the same ray is intersected to a lot of different geometry
    FastRay fast(ray);

    for (auto& box : boxes)
    {
        IntersectRange is;
        if (is.intersect(fast, box))
        {
            float32x3 enter = ray.origin + ray.direction * is.t0;
            float32x3 leave = ray.origin + ray.direction * is.t1;
            MANGO_UNREFERENCED(enter);
            MANGO_UNREFERENCED(leave);
        }
    }
}

void example11()
{
    float32x4 linear(1.0f, 0.5f, 0.5f, 1.0f);
    float32x4 nonlinear = srgbEncode(linear);
    linear = srgbDecode(nonlinear);
}

/*
    Portable low level SIMD abstraction. Uses a simple functional API for
    getting the job done. Supports multiple architectures using a common interface.
    Does only expose functionalty that is efficient and common for all architectures.

    The higher-level short vector math library is written on top of the low level
    code to be more user-friendly. This abstracts all of the platform specific
    minute details into it's own neat compartment for easier maintenance. This also
    allows to add more platforms easier; we already have quite a few targets:
    - Intel (SSE, SSE2, SSE3, SSE 4.1, AVX, AVX2)
    - ARM neon
    - PPC Altivec / SPU

*/

void example12()
{
    simd::f32x4 a = simd::f32x4_set(1.0f, 2.0f, 2.0f, 1.0f);
    simd::f32x4 b = simd::f32x4_set(0.0f, 1.0f, 0.5f, 0.5f);
    simd::f32x4 c = simd::add(a, b);
    simd::f32x4 d = simd::mul(c, b);
    simd::mask32x4 mask = simd::compare_gt(a, b);
    simd::f32x4 e = simd::select(mask, d, c);
    MANGO_UNREFERENCED(e);
}

// previous example using higher-level "Short Vector Math" abstraction:
void example13()
{
    float32x4 a(1.0f, 2.0f, 2.0f, 1.0f);
    float32x4 b(0.0f, 1.0f, 0.5f, 0.5f);
    float32x4 c = a + b;
    float32x4 d = c * b;
    float32x4 e = select(a > b, d, c);
    MANGO_UNREFERENCED(e);
}

int main()
{
    // NOTE: This code is compiled for validation purposes
}

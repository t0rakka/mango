/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/math/math.hpp>

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

    print(a * b);
    print(a * b.xyzw);
    print(a * 5.0f);
    print(a * b.x);
    print(b.x * a.xyzw);
}

/*

    TODO: test everything

    // ------------------------------------------------------------------
    // operators
    // ------------------------------------------------------------------

    static inline Vector<float, 4> operator + (Vector<float, 4> v)
    static inline Vector<float, 4> operator - (Vector<float, 4> v)
    static inline Vector<float, 4>& operator += (Vector<float, 4>& a, Vector<float, 4> b)
    static inline Vector<float, 4>& operator -= (Vector<float, 4>& a, Vector<float, 4> b)
    static inline Vector<float, 4>& operator *= (Vector<float, 4>& a, Vector<float, 4> b)

    template <typename VT, int I>
    static inline Vector<float, 4>& operator /= (Vector<float, 4>& a, ScalarAccessor<float, VT, I> b)
    {
        a = simd::div(a, b);
        return a;
    }

    static inline Vector<float, 4>& operator /= (Vector<float, 4>& a, Vector<float, 4> b)
    static inline Vector<float, 4>& operator /= (Vector<float, 4>& a, float b)
    static inline Vector<float, 4> operator + (Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> operator - (Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> operator * (Vector<float, 4> a, Vector<float, 4> b)

    template <typename VT, int I>
    static inline Vector<float, 4> operator / (Vector<float, 4> a, ScalarAccessor<float, VT, I> b)
    {
        return simd::div(a, b);
    }

    static inline Vector<float, 4> operator / (Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> operator / (Vector<float, 4> a, float b)

    // ------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------

    static inline Vector<float, 4> add(Vector<float, 4> a, Vector<float, 4> b, mask32x4 mask)
    static inline Vector<float, 4> add(Vector<float, 4> a, Vector<float, 4> b, mask32x4 mask, Vector<float, 4> value)
    static inline Vector<float, 4> sub(Vector<float, 4> a, Vector<float, 4> b, mask32x4 mask)
    static inline Vector<float, 4> sub(Vector<float, 4> a, Vector<float, 4> b, mask32x4 mask, Vector<float, 4> value)
    static inline Vector<float, 4> mul(Vector<float, 4> a, Vector<float, 4> b, mask32x4 mask)
    static inline Vector<float, 4> mul(Vector<float, 4> a, Vector<float, 4> b, mask32x4 mask, Vector<float, 4> value)
    static inline Vector<float, 4> div(Vector<float, 4> a, Vector<float, 4> b, mask32x4 mask)
    static inline Vector<float, 4> div(Vector<float, 4> a, Vector<float, 4> b, mask32x4 mask, Vector<float, 4> value)
    static inline Vector<float, 4> abs(Vector<float, 4> a)
    static inline float square(Vector<float, 4> a)
    static inline float length(Vector<float, 4> a)
    static inline Vector<float, 4> normalize(Vector<float, 4> a)
    static inline Vector<float, 4> round(Vector<float, 4> a)
    static inline Vector<float, 4> floor(Vector<float, 4> a)
    static inline Vector<float, 4> ceil(Vector<float, 4> a)
    static inline Vector<float, 4> trunc(Vector<float, 4> a)
    static inline Vector<float, 4> fract(Vector<float, 4> a)
    static inline Vector<float, 4> sign(Vector<float, 4> a)
    static inline Vector<float, 4> radians(Vector<float, 4> a)
    static inline Vector<float, 4> degrees(Vector<float, 4> a)
    static inline Vector<float, 4> sqrt(Vector<float, 4> a)
    static inline Vector<float, 4> rsqrt(Vector<float, 4> a)
    static inline Vector<float, 4> rcp(Vector<float, 4> a)
    static inline Vector<float, 4> unpacklo(Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> unpackhi(Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> min(Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> min(Vector<float, 4> a, Vector<float, 4> b, mask32x4 mask)
    static inline Vector<float, 4> min(Vector<float, 4> a, Vector<float, 4> b, mask32x4 mask, Vector<float, 4> value)
    static inline Vector<float, 4> max(Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> max(Vector<float, 4> a, Vector<float, 4> b, mask32x4 mask)
    static inline Vector<float, 4> max(Vector<float, 4> a, Vector<float, 4> b, mask32x4 mask, Vector<float, 4> value)
    static inline float dot(Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> cross(Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> mod(Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> clamp(Vector<float, 4> a, Vector<float, 4> low, Vector<float, 4> high)
    static inline Vector<float, 4> madd(Vector<float, 4> a, Vector<float, 4> b, Vector<float, 4> c)
    static inline Vector<float, 4> msub(Vector<float, 4> a, Vector<float, 4> b, Vector<float, 4> c)
    static inline Vector<float, 4> nmadd(Vector<float, 4> a, Vector<float, 4> b, Vector<float, 4> c)
    static inline Vector<float, 4> nmsub(Vector<float, 4> a, Vector<float, 4> b, Vector<float, 4> c)
    static inline Vector<float, 4> lerp(Vector<float, 4> a, Vector<float, 4> b, float factor)
    static inline Vector<float, 4> lerp(Vector<float, 4> a, Vector<float, 4> b, Vector<float, 4> factor)
    static inline Vector<float, 4> hmin(Vector<float, 4> v)
    static inline Vector<float, 4> hmax(Vector<float, 4> v)

    template <int x, int y, int z, int w>
    static inline Vector<float, 4> shuffle(Vector<float, 4> a, Vector<float, 4> b)
    {
        return simd::shuffle<x, y, z, w>(a, b);
    }

    static inline Vector<float, 4> movelh(Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> movehl(Vector<float, 4> a, Vector<float, 4> b)

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
    static inline Vector<float, 4> operator & (Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> operator | (Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> operator ^ (Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> operator ~ (Vector<float, 4> a)

    // ------------------------------------------------------------------
    // compare / select
    // ------------------------------------------------------------------

    static inline mask32x4 operator > (Vector<float, 4> a, Vector<float, 4> b)
    static inline mask32x4 operator >= (Vector<float, 4> a, Vector<float, 4> b)
    static inline mask32x4 operator < (Vector<float, 4> a, Vector<float, 4> b)
    static inline mask32x4 operator <= (Vector<float, 4> a, Vector<float, 4> b)
    static inline mask32x4 operator == (Vector<float, 4> a, Vector<float, 4> b)
    static inline mask32x4 operator != (Vector<float, 4> a, Vector<float, 4> b)
    static inline Vector<float, 4> select(mask32x4 mask, Vector<float, 4> a, Vector<float, 4> b)



    struct Vector<float, 4>
    {
        union
        {
            simd::f32x4 m;

            LowAccessor<Vector<float, 2>, simd::f32x4> low;
            HighAccessor<Vector<float, 2>, simd::f32x4> high;

            ScalarAccessor<float, simd::f32x4, 0> x;
            ScalarAccessor<float, simd::f32x4, 1> y;
            ScalarAccessor<float, simd::f32x4, 2> z;
            ScalarAccessor<float, simd::f32x4, 3> w;

            // generate 2 component accessors
            ShuffleAccessor4x2<float, simd::f32x4, A, B> NAME

            // generate 3 component accessors
            ShuffleAccessor4x3<float, simd::f32x4, A, B, C> NAME

            // generate 4 component accessors
            ShuffleAccessor4<float, simd::f32x4, A, B, C, D> NAME
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

        explicit Vector() {}
        Vector(float s)
        explicit Vector(float x, float y, float z, float w)
        explicit Vector(const Vector<float, 2>& v, float z, float w)
        explicit Vector(float x, float y, const Vector<float, 2>& v)
        explicit Vector(float x, const Vector<float, 2>& v, float w)
        explicit Vector(const Vector<float, 2>& xy, const Vector<float, 2>& zw)
        explicit Vector(const Vector<float, 3>& v, float w)
        explicit Vector(float x, const Vector<float, 3>& v)
        Vector(simd::f32x4 v)

        template <typename T, int I>
        Vector& operator = (const ScalarAccessor<ScalarType, T, I>& accessor)

        Vector(const Vector& v) = default;
        Vector& operator = (const Vector& v)
        Vector& operator = (simd::f32x4 v)
        Vector& operator = (float s)
        operator simd::f32x4 () const

#ifdef simd_float128_is_hardware_vector
        operator simd::f32x4::vector () const
#endif

        u32 pack() const
        void unpack(u32 a)
        static Vector ascend()
    };

*/

int main()
{
    test_float32x4();
}

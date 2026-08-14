/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "core_test.hpp"

#include <cmath>
#include <cstring>
#include <limits>

using namespace mango;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    bool eq_bits(u16 expected, u16 actual)
    {
        return expected == actual;
    }

    bool near_float(float a, float b, float epsilon = 1e-3f)
    {
        if (std::isnan(a) && std::isnan(b))
        {
            return true;
        }

        if (std::isinf(a) && std::isinf(b))
        {
            return std::signbit(a) == std::signbit(b);
        }

        return std::fabs(a - b) <= epsilon;
    }

    bool test_known_bit_patterns()
    {
        struct Vector
        {
            float value;
            u16 bits;
        };

        const Vector vectors [] =
        {
            {  0.0f, 0x0000 },
            { -0.0f, 0x8000 },
            {  1.0f, 0x3c00 },
            { -1.0f, 0xbc00 },
            {  0.5f, 0x3800 },
            { -0.5f, 0xb800 },
            {  2.0f, 0x4000 },
            { -2.0f, 0xc000 },
            {  65504.0f, 0x7bff },
            { -65504.0f, 0xfbff },
        };

        for (const Vector& vector : vectors)
        {
            Half from_float(vector.value);
            CHECK(eq_bits(vector.bits, from_float.u));

            Half from_bits(vector.bits);
            CHECK(near_float(vector.value, float(from_bits)));
            CHECK(eq_bits(vector.bits, from_bits.u));
        }

        return true;
    }

    bool test_infinity_and_nan()
    {
        Half pos_inf(std::numeric_limits<float>::infinity());
        Half neg_inf(-std::numeric_limits<float>::infinity());
        Half nan(std::numeric_limits<float>::quiet_NaN());

        CHECK(pos_inf.exponent == 0x1f);
        CHECK(pos_inf.mantissa == 0);
        CHECK(pos_inf.sign == 0);

        CHECK(neg_inf.exponent == 0x1f);
        CHECK(neg_inf.mantissa == 0);
        CHECK(neg_inf.sign == 1);

        CHECK(std::isinf(float(pos_inf)));
        CHECK(std::isinf(float(neg_inf)));
        CHECK(std::signbit(float(neg_inf)));
        CHECK(std::isnan(float(nan)));

        return true;
    }

    bool test_float_roundtrip()
    {
        const float values [] =
        {
            0.0f, -0.0f, 1.0f, -1.0f, 0.25f, -0.25f, 3.14159f, -3.14159f,
            0.00006103515625f,  // 2^-14, smallest positive normal
            65504.0f, -65504.0f,
            1.0e-4f, -1.0e-4f,
            100.0f, -100.0f,
        };

        for (float value : values)
        {
            Half half(value);
            float restored = half;
            CHECK(near_float(value, restored));
        }

        return true;
    }

    bool test_double_conversion()
    {
        const double values [] =
        {
            0.0, -0.0, 1.0, -1.0, 0.5, -0.5, 42.0, -42.0,
        };

        for (double value : values)
        {
            Half half(value);
            double restored = half;
            CHECK(near_float(float(value), float(restored)));
        }

        return true;
    }

    bool test_assignment_and_aliases()
    {
        float16 a = 2.0f;
        CHECK(a.u == 0x4000);

        a = u16(0x3c00);
        CHECK(float(a) == 1.0f);

        Half b;
        b = a;
        CHECK(b.u == 0x3c00);

        return true;
    }

    bool test_endian_load_store()
    {
        alignas(2) u8 storage[2] = { 0x00, 0x3c };

        float16 loaded = uload16f(storage);
        CHECK(loaded.u == 0x3c00);
        CHECK(float(loaded) == 1.0f);

        ustore16f(storage, float16(2.0f));
        CHECK(storage[0] == 0x00);
        CHECK(storage[1] == 0x40);

        float16 swapped = byteswap(float16(u16(0x3c00)));
        CHECK(swapped.u == 0x003c);

        return true;
    }

    bool test_denormals()
    {
        // Smallest positive subnormal: 0x0001 -> 2^-24 * 2^-14
        Half subnormal(u16(0x0001));
        float value = subnormal;
        CHECK(value > 0.0f);
        CHECK(value < 6.0e-8f);

        Half roundtrip(value);
        CHECK(near_float(value, float(roundtrip), 1.0e-9f));

        return true;
    }

    const Case g_cases [] =
    {
        { "known bit patterns", test_known_bit_patterns },
        { "infinity and nan", test_infinity_and_nan },
        { "float roundtrip", test_float_roundtrip },
        { "double conversion", test_double_conversion },
        { "assignment and aliases", test_assignment_and_aliases },
        { "endian load store", test_endian_load_store },
        { "denormals", test_denormals },
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("core_half", g_cases, std::size(g_cases), argc, argv);
}

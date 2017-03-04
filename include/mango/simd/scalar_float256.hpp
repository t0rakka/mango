/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifdef MANGO_INCLUDE_SIMD

    // -----------------------------------------------------------------
    // float32x8
    // -----------------------------------------------------------------

    static inline float32x8 float32x8_zero()
    {
        return {{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }};
    }

    static inline float32x8 float32x8_set1(float s)
    {
        return {{ s, s, s, s, s, s, s, s }};
    }

    static inline float32x8 float32x8_set8(float s0, float s1, float s2, float s3, float s4, float s5, float s6, float s7)
    {
        return {{ s0, s1, s2, s3, s4, s5, s6, s7 }};
    }

    static inline float32x8 float32x8_uload(const float* s)
    {
        return {{ s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7] }};
    }

    static inline void float32x8_ustore(float* dest, float32x8 a)
    {
        dest[0] = a[0];
        dest[1] = a[1];
        dest[2] = a[2];
        dest[3] = a[3];
        dest[4] = a[4];
        dest[5] = a[5];
        dest[6] = a[6];
        dest[7] = a[7];
    }

    static inline float32x8 unpackhi(float32x8 a, float32x8 b)
    {
        return float32x8_set8(a[4], b[4], a[5], b[5], a[6], b[6], a[7], b[7] );
    }

    static inline float32x8 unpacklo(float32x8 a, float32x8 b)
    {
        return float32x8_set8(a[0], b[0], a[1], b[1], a[2], b[2], a[3], b[3] );
    }

    // bitwise

    static inline float32x8 float32x8_and(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = Float(Float(a[i]).u & Float(b[i]).u);
        }
        return v;
    }

    static inline float32x8 float32x8_nand(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = Float(~Float(a[i]).u & Float(b[i]).u);
        }
        return v;
    }

    static inline float32x8 float32x8_or(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = Float(Float(a[i]).u | Float(b[i]).u);
        }
        return v;
    }

    static inline float32x8 float32x8_xor(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = Float(Float(a[i]).u ^ Float(b[i]).u);
        }
        return v;
    }

    static inline float32x8 min(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = std::min(a[i], b[i]);
        }
        return v;
    }

    static inline float32x8 max(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = std::max(a[i], b[i]);
        }
        return v;
    }

    static inline float32x8 abs(float32x8 a)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = std::abs(a[i]);
        }
        return v;
    }

    static inline float32x8 neg(float32x8 a)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = -a[i];
        }
        return v;
    }

    static inline float32x8 add(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = a[i] + b[i];
        }
        return v;
    }

    static inline float32x8 sub(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = a[i] - b[i];
        }
        return v;
    }

    static inline float32x8 mul(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = a[i] * b[i];
        }
        return v;
    }

    static inline float32x8 div(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = a[i] / b[i];
        }
        return v;
    }

    static inline float32x8 div(float32x8 a, float b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = a[i] / b;
        }
        return v;
    }

    static inline float32x8 fast_reciprocal(float32x8 a)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = 1.0f / a[i];
        }
        return v;
    }

    static inline float32x8 fast_rsqrt(float32x8 a)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = 1.0f / float(std::sqrt(a[i]));
        }
        return v;
    }

    static inline float32x8 fast_sqrt(float32x8 a)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = float(std::sqrt(a[i]));
        }
        return v;
    }

    static inline float32x8 reciprocal(float32x8 a)
    {
        return fast_reciprocal(a);
    }

    static inline float32x8 rsqrt(float32x8 a)
    {
        return fast_rsqrt(a);
    }

    static inline float32x8 sqrt(float32x8 a)
    {
        return fast_sqrt(a);
    }

    // compare

    static inline float32x8 compare_neq(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = Float(-uint32(a[i] != b[i]));
        }
        return v;
    }

    static inline float32x8 compare_eq(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = Float(-uint32(a[i] == b[i]));
        }
        return v;
    }

    static inline float32x8 compare_lt(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = Float(-uint32(a[i] < b[i]));
        }
        return v;
    }

    static inline float32x8 compare_le(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = Float(-uint32(a[i] <= b[i]));
        }
        return v;
    }

    static inline float32x8 compare_gt(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = Float(-uint32(a[i] > b[i]));
        }
        return v;
    }

    static inline float32x8 compare_ge(float32x8 a, float32x8 b)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = Float(-uint32(a[i] >= b[i]));
        }
        return v;
    }

    static inline float32x8 select(float32x8 mask, float32x8 a, float32x8 b)
    {
        return float32x8_or(float32x8_and(mask, a), float32x8_nand(mask, b));
    }

    // rounding

    static inline float32x8 round(float32x8 s)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = std::round(s[i]);
        }
        return v;
    }

    static inline float32x8 trunc(float32x8 s)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = std::trunc(s[i]);
        }
        return v;
    }

    static inline float32x8 floor(float32x8 s)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = std::floor(s[i]);
        }
        return v;
    }

    static inline float32x8 ceil(float32x8 s)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = std::ceil(s[i]);
        }
        return v;
    }

    static inline float32x8 fract(float32x8 s)
    {
        float32x8 v;
        for (int i = 0; i < 8; ++i)
        {
            v[i] = s[i] - std::floor(s[i]);
        }
        return v;
    }

#endif // MANGO_INCLUDE_SIMD

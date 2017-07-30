/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // helpers
    // -----------------------------------------------------------------

    template <typename ScalarType, int Size>
    static inline scalar_vector<ScalarType, Size>
    scalar_set(ScalarType value)
    {
        scalar_vector<ScalarType, Size> v;
        for (int i = 0; i < Size; ++i) {
            v[i] = value;
        }
        return v;
    }

    // unary

    template <typename ScalarType, int Size>
    static inline scalar_vector<ScalarType, Size>
    scalar_unroll(ScalarType (*func)(ScalarType), scalar_vector<ScalarType, Size> a)
    {
        scalar_vector<ScalarType, Size> v;
        for (int i = 0; i < Size; ++i) {
            v[i] = func(a[i]);
        }
        return v;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_abs(ScalarType a)
    {
        return std::abs(a);
    }

    template <typename ScalarType>
    static inline ScalarType scalar_neg(ScalarType a)
    {
        return -a;
    }

    // binary

    template <typename ScalarType, int Size>
    static inline scalar_vector<ScalarType, Size>
    scalar_unroll(ScalarType (*func)(ScalarType, ScalarType), scalar_vector<ScalarType, Size> a, scalar_vector<ScalarType, Size> b)
    {
        scalar_vector<ScalarType, Size> v;
        for (int i = 0; i < Size; ++i) {
            v[i] = func(a[i], b[i]);
        }
        return v;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_add(ScalarType a, ScalarType b)
    {
        return a + b;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_sub(ScalarType a, ScalarType b)
    {
        return a - b;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_mullo(ScalarType a, ScalarType b)
    {
        return ScalarType(a * b);
    }

    template <typename ScalarType>
    static inline ScalarType scalar_unsigned_adds(ScalarType a, ScalarType b)
    {
	    ScalarType v = a + b;
	    v |= -(v < a);
	    return v;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_unsigned_subs(ScalarType a, ScalarType b)
    {
    	ScalarType v = a - b;
	    v &= -(v <= a);
	    return v;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_signed_adds(ScalarType a, ScalarType b)
    {
      	typedef typename std::make_unsigned<ScalarType>::type UnsignedScalarType;
        UnsignedScalarType x = a;
		UnsignedScalarType y = b;
		UnsignedScalarType v = x + y;

  	    // overflow
	    x = (x >> (sizeof(ScalarType) * 8 - 1)) + std::numeric_limits<ScalarType>::max();
	    if (ScalarType((x ^ y) | ~(y ^ v)) >= 0)
	    {
		    v = x;
	    }

	    return v;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_signed_subs(ScalarType a, ScalarType b)
    {
      	typedef typename std::make_unsigned<ScalarType>::type UnsignedScalarType;
        UnsignedScalarType x = a;
        UnsignedScalarType y = b;
        UnsignedScalarType v = x - y;

  	    // overflow
	    x = (x >> (sizeof(ScalarType) * 8 - 1)) + std::numeric_limits<ScalarType>::max();
	    if (ScalarType((x ^ y) & (x ^ v)) < 0)
	    {
		    v = x;
	    }

	    return v;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_nand(ScalarType a, ScalarType b)
    {
        return ~a & b;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_and(ScalarType a, ScalarType b)
    {
        return a & b;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_or(ScalarType a, ScalarType b)
    {
        return a | b;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_xor(ScalarType a, ScalarType b)
    {
        return a ^ b;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_not(ScalarType a)
    {
        return ~a;
    }

    template <typename VectorType, typename ScalarType, int Count>
    static inline VectorType scalar_shift_left(VectorType a)
    {
        VectorType v;
        for (int i = 0; i < VectorType::size; ++i)
        {
            v[i] = Count < sizeof(ScalarType) * 8 ? ScalarType(a[i]) << Count : 0;
        }
        return v;
    }

    template <typename VectorType, typename ScalarType, int Count>
    static inline VectorType scalar_shift_right(VectorType a)
    {
        VectorType v;
        for (int i = 0; i < VectorType::size; ++i)
        {
            v[i] = Count < sizeof(ScalarType) * 8 ? ScalarType(a[i]) >> Count : 0;
        }
        return v;
    }

    template <typename VectorType, typename ScalarType>
    static inline VectorType scalar_shift_left(VectorType a, int count)
    {
        VectorType v;
        for (int i = 0; i < VectorType::size; ++i)
        {
            v[i] = count < sizeof(ScalarType) * 8 ? ScalarType(a[i]) << count : 0;
        }
        return v;
    }

    template <typename VectorType, typename ScalarType>
    static inline VectorType scalar_shift_right(VectorType a, int count)
    {
        VectorType v;
        for (int i = 0; i < VectorType::size; ++i)
        {
            v[i] = count < sizeof(ScalarType) * 8 ? ScalarType(a[i]) >> count : 0;
        }
        return v;
    }

    template <typename ScalarType>
    static inline ScalarType scalar_min(ScalarType a, ScalarType b)
    {
        return std::min(a, b);
    }

    template <typename ScalarType>
    static inline ScalarType scalar_max(ScalarType a, ScalarType b)
    {
        return std::max(a, b);
    }

    // cmp

    template <typename VectorType>
    static inline uint32 scalar_compare_eq(VectorType a, VectorType b)
    {
        uint32 mask = 0;
        for (int i = 0; i < VectorType::size; ++i)
        {
            mask |= uint32(a[i] == b[i]) << i;
        }
        return mask;
    }

    template <typename VectorType>
    static inline uint32 scalar_compare_gt(VectorType a, VectorType b)
    {
        uint32 mask = 0;
        for (int i = 0; i < VectorType::size; ++i)
        {
            mask |= uint32(a[i] > b[i]) << i;
        }
        return mask;
    }

    template <typename VectorType>
    static inline uint32 scalar_compare_neq(VectorType a, VectorType b)
    {
        uint32 mask = 0;
        for (int i = 0; i < VectorType::size; ++i)
        {
            mask |= uint32(a[i] != b[i]) << i;
        }
        return mask;
    }

    template <typename VectorType>
    static inline uint32 scalar_compare_lt(VectorType a, VectorType b)
    {
        uint32 mask = 0;
        for (int i = 0; i < VectorType::size; ++i)
        {
            mask |= uint32(a[i] < b[i]) << i;
        }
        return mask;
    }

    template <typename VectorType>
    static inline uint32 scalar_compare_le(VectorType a, VectorType b)
    {
        uint32 mask = 0;
        for (int i = 0; i < VectorType::size; ++i)
        {
            mask |= uint32(a[i] <= b[i]) << i;
        }
        return mask;
    }

    template <typename VectorType>
    static inline uint32 scalar_compare_ge(VectorType a, VectorType b)
    {
        uint32 mask = 0;
        for (int i = 0; i < VectorType::size; ++i)
        {
            mask |= uint32(a[i] >= b[i]) << i;
        }
        return mask;
    }

    // misc

    template <typename ScalarType, int Size>
    static inline scalar_vector<ScalarType, Size>
    scalar_select(uint32 mask, scalar_vector<ScalarType, Size> a, scalar_vector<ScalarType, Size> b)
    {
        scalar_vector<ScalarType, Size> v;
        for (int i = 0; i < Size; ++i) {
            v[i] = mask & (1 << i) ? a[i] : b[i];
        }
        return v;
    }

    template <typename VectorType>
    static inline VectorType scalar_unpacklo(VectorType a, VectorType b)
    {
        VectorType v;
        for (int i = 0; i < VectorType::size / 2; ++i)
        {
            v[i * 2 + 0] = a[i];
            v[i * 2 + 1] = b[i];
        }
        return v;
    }

    template <typename VectorType>
    static inline VectorType scalar_unpackhi(VectorType a, VectorType b)
    {
        VectorType v;
        constexpr int half = VectorType::size / 2;
        for (int i = 0; i < half; ++i)
        {
            v[i * 2 + 0] = a[i + half];
            v[i * 2 + 1] = b[i + half];
        }
        return v;
    }

    // -----------------------------------------------------------------
    // uint8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint8x16 set_component(uint8x16 a, uint8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline uint8 get_component(uint8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return a[Index];
    }

    static inline uint8x16 uint8x16_zero()
    {
        return scalar_set<uint8, 16>(0);
    }

    static inline uint8x16 uint8x16_set1(uint8 s)
    {
        return scalar_set<uint8, 16>(s);
    }

    static inline uint8x16 unpacklo(uint8x16 a, uint8x16 b)
    {
        return scalar_unpacklo(a, b);
    }

    static inline uint8x16 unpackhi(uint8x16 a, uint8x16 b)
    {
        return scalar_unpackhi(a, b);
    }

    static inline uint8x16 add(uint8x16 a, uint8x16 b)
    {
        return scalar_unroll(scalar_add, a, b);
    }

    static inline uint8x16 sub(uint8x16 a, uint8x16 b)
    {
        return scalar_unroll(scalar_sub, a, b);
    }

    static inline uint8x16 mullo(uint8x16 a, uint8x16 b)
    {
        return scalar_unroll(scalar_mullo, a, b);
    }

    // saturated

    static inline uint8x16 adds(uint8x16 a, uint8x16 b)
    {
        return scalar_unroll(scalar_unsigned_adds, a, b);
    }

    static inline uint8x16 subs(uint8x16 a, uint8x16 b)
    {
        return scalar_unroll(scalar_unsigned_subs, a, b);
    }

    // bitwise

    static inline uint8x16 bitwise_nand(uint8x16 a, uint8x16 b)
    {
        return scalar_unroll(scalar_nand, a, b);
    }

    static inline uint8x16 bitwise_and(uint8x16 a, uint8x16 b)
    {
        return scalar_unroll(scalar_and, a, b);
    }

    static inline uint8x16 bitwise_or(uint8x16 a, uint8x16 b)
    {
        return scalar_unroll(scalar_or, a, b);
    }

    static inline uint8x16 bitwise_xor(uint8x16 a, uint8x16 b)
    {
        return scalar_unroll(scalar_xor, a, b);
    }

    static inline uint8x16 bitwise_not(uint8x16 a)
    {
        return scalar_unroll(scalar_not, a);
    }

    // compare

    static inline mask8x16 compare_eq(uint8x16 a, uint8x16 b)
    {
        return scalar_compare_eq(a, b);
    }

    static inline mask8x16 compare_gt(uint8x16 a, uint8x16 b)
    {
        return scalar_compare_gt(a, b);
    }

    static inline mask8x16 compare_neq(uint8x16 a, uint8x16 b)
    {
        return scalar_compare_neq(a, b);
    }

    static inline mask8x16 compare_lt(uint8x16 a, uint8x16 b)
    {
        return scalar_compare_lt(a, b);
    }

    static inline mask8x16 compare_le(uint8x16 a, uint8x16 b)
    {
        return scalar_compare_le(a, b);
    }

    static inline mask8x16 compare_ge(uint8x16 a, uint8x16 b)
    {
        return scalar_compare_ge(a, b);
    }

    static inline uint8x16 select(mask8x16 mask, uint8x16 a, uint8x16 b)
    {
        return scalar_select(mask, a, b);
    }

    static inline uint8x16 min(uint8x16 a, uint8x16 b)
    {
        return scalar_unroll(scalar_min, a, b);
    }

    static inline uint8x16 max(uint8x16 a, uint8x16 b)
    {
        return scalar_unroll(scalar_max, a, b);
    }

    // -----------------------------------------------------------------
    // uint16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint16x8 set_component(uint16x8 a, uint16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline uint16 get_component(uint16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return a[Index];
    }

    static inline uint16x8 uint16x8_zero()
    {
        return scalar_set<uint16, 8>(0);
    }

    static inline uint16x8 uint16x8_set1(uint16 s)
    {
        return scalar_set<uint16, 8>(s);
    }

    static inline uint16x8 unpacklo(uint16x8 a, uint16x8 b)
    {
        return scalar_unpacklo(a, b);
    }

    static inline uint16x8 unpackhi(uint16x8 a, uint16x8 b)
    {
        return scalar_unpackhi(a, b);
    }

    static inline uint16x8 add(uint16x8 a, uint16x8 b)
    {
        return scalar_unroll(scalar_add, a, b);
    }

    static inline uint16x8 sub(uint16x8 a, uint16x8 b)
    {
        return scalar_unroll(scalar_sub, a, b);
    }

    static inline uint16x8 mullo(uint16x8 a, uint16x8 b)
    {
        return scalar_unroll(scalar_mullo, a, b);
    }

    // saturated

    static inline uint16x8 adds(uint16x8 a, uint16x8 b)
    {
        return scalar_unroll(scalar_unsigned_adds, a, b);
    }

    static inline uint16x8 subs(uint16x8 a, uint16x8 b)
    {
        return scalar_unroll(scalar_unsigned_subs, a, b);
    }

    // bitwise

    static inline uint16x8 bitwise_nand(uint16x8 a, uint16x8 b)
    {
        return scalar_unroll(scalar_nand, a, b);
    }

    static inline uint16x8 bitwise_and(uint16x8 a, uint16x8 b)
    {
        return scalar_unroll(scalar_and, a, b);
    }

    static inline uint16x8 bitwise_or(uint16x8 a, uint16x8 b)
    {
        return scalar_unroll(scalar_or, a, b);
    }

    static inline uint16x8 bitwise_xor(uint16x8 a, uint16x8 b)
    {
        return scalar_unroll(scalar_xor, a, b);
    }

    static inline uint16x8 bitwise_not(uint16x8 a)
    {
        return scalar_unroll(scalar_not, a);
    }

    // compare

    static inline mask16x8 compare_eq(uint16x8 a, uint16x8 b)
    {
        return scalar_compare_eq(a, b);
    }

    static inline mask16x8 compare_gt(uint16x8 a, uint16x8 b)
    {
        return scalar_compare_gt(a, b);
    }

    static inline mask16x8 compare_neq(uint16x8 a, uint16x8 b)
    {
        return scalar_compare_neq(a, b);
    }

    static inline mask16x8 compare_lt(uint16x8 a, uint16x8 b)
    {
        return scalar_compare_lt(a, b);
    }

    static inline mask16x8 compare_le(uint16x8 a, uint16x8 b)
    {
        return scalar_compare_le(a, b);
    }

    static inline mask16x8 compare_ge(uint16x8 a, uint16x8 b)
    {
        return scalar_compare_ge(a, b);
    }

    static inline uint16x8 select(mask16x8 mask, uint16x8 a, uint16x8 b)
    {
        return scalar_select(mask, a, b);
    }

    // shift

    template <int Count>
    static inline uint16x8 slli(uint16x8 a)
    {
        return scalar_shift_left<uint16x8, uint16, Count>(a);
    }

    template <int Count>
    static inline uint16x8 srli(uint16x8 a)
    {
        return scalar_shift_right<uint16x8, uint16, Count>(a);
    }

    template <int Count>
    static inline uint16x8 srai(uint16x8 a)
    {
        return scalar_shift_right<uint16x8, int16, Count>(a);
    }

    static inline uint16x8 sll(uint16x8 a, int count)
    {
        return scalar_shift_left<uint16x8, uint16>(a, count);
    }

    static inline uint16x8 srl(uint16x8 a, int count)
    {
        return scalar_shift_right<uint16x8, uint16>(a, count);
    }

    static inline uint16x8 sra(uint16x8 a, int count)
    {
        return scalar_shift_right<uint16x8, int16>(a, count);
    }

    static inline uint16x8 min(uint16x8 a, uint16x8 b)
    {
        return scalar_unroll(scalar_min, a, b);
    }

    static inline uint16x8 max(uint16x8 a, uint16x8 b)
    {
        return scalar_unroll(scalar_max, a, b);
    }
    
    // -----------------------------------------------------------------
    // uint32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline uint32x4 shuffle(uint32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return {{ v[x], v[y], v[z], v[w] }};
    }

    template <>
    inline uint32x4 shuffle<0, 1, 2, 3>(uint32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline uint32x4 set_component(uint32x4 a, uint32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline uint32 get_component(uint32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return a[Index];
    }

    static inline uint32x4 uint32x4_zero()
    {
        return {{ 0, 0, 0, 0 }};
    }

    static inline uint32x4 uint32x4_set1(uint32 s)
    {
        return {{ s, s, s, s }};
    }

    static inline uint32x4 uint32x4_set4(uint32 x, uint32 y, uint32 z, uint32 w)
    {
        return {{ x, y, z, w }};
    }

    static inline uint32x4 uint32x4_uload(const uint32* source)
    {
        return uint32x4_set4(source[0], source[1], source[2], source[3]);
    }

    static inline void uint32x4_ustore(uint32* dest, uint32x4 a)
    {
        dest[0] = a[0];
        dest[1] = a[1];
        dest[2] = a[2];
        dest[3] = a[3];
    }

    static inline uint32x4 unpacklo(uint32x4 a, uint32x4 b)
    {
        return scalar_unpacklo(a, b);
    }

    static inline uint32x4 unpackhi(uint32x4 a, uint32x4 b)
    {
        return scalar_unpackhi(a, b);
    }

    static inline uint32x4 add(uint32x4 a, uint32x4 b)
    {
        return scalar_unroll(scalar_add, a, b);
    }

    static inline uint32x4 sub(uint32x4 a, uint32x4 b)
    {
        return scalar_unroll(scalar_sub, a, b);
    }

    static inline uint32x4 mullo(uint32x4 a, uint32x4 b)
    {
        return scalar_unroll(scalar_mullo, a, b);
    }

    // saturated

    static inline uint32x4 adds(uint32x4 a, uint32x4 b)
    {
        return scalar_unroll(scalar_unsigned_adds, a, b);
    }

    static inline uint32x4 subs(uint32x4 a, uint32x4 b)
    {
        return scalar_unroll(scalar_unsigned_subs, a, b);
    }

    // bitwise

    static inline uint32x4 bitwise_nand(uint32x4 a, uint32x4 b)
    {
        return scalar_unroll(scalar_nand, a, b);
    }

    static inline uint32x4 bitwise_and(uint32x4 a, uint32x4 b)
    {
        return scalar_unroll(scalar_and, a, b);
    }

    static inline uint32x4 bitwise_or(uint32x4 a, uint32x4 b)
    {
        return scalar_unroll(scalar_or, a, b);
    }

    static inline uint32x4 bitwise_xor(uint32x4 a, uint32x4 b)
    {
        return scalar_unroll(scalar_xor, a, b);
    }

    static inline uint32x4 bitwise_not(uint32x4 a)
    {
        return scalar_unroll(scalar_not, a);
    }

    // compare

    static inline mask32x4 compare_eq(uint32x4 a, uint32x4 b)
    {
        return scalar_compare_eq(a, b);
    }

    static inline mask32x4 compare_gt(uint32x4 a, uint32x4 b)
    {
        return scalar_compare_gt(a, b);
    }

    static inline mask32x4 compare_neq(uint32x4 a, uint32x4 b)
    {
        return scalar_compare_neq(a, b);
    }

    static inline mask32x4 compare_lt(uint32x4 a, uint32x4 b)
    {
        return scalar_compare_lt(a, b);
    }

    static inline mask32x4 compare_le(uint32x4 a, uint32x4 b)
    {
        return scalar_compare_le(a, b);
    }

    static inline mask32x4 compare_ge(uint32x4 a, uint32x4 b)
    {
        return scalar_compare_ge(a, b);
    }

    static inline uint32x4 select(mask32x4 mask, uint32x4 a, uint32x4 b)
    {
        return scalar_select(mask, a, b);
    }

    // shift

    template <int Count>
    static inline uint32x4 slli(uint32x4 a)
    {
        return scalar_shift_left<uint32x4, uint32, Count>(a);
    }

    template <int Count>
    static inline uint32x4 srli(uint32x4 a)
    {
        return scalar_shift_right<uint32x4, uint32, Count>(a);
    }

    template <int Count>
    static inline uint32x4 srai(uint32x4 a)
    {
        return scalar_shift_right<uint32x4, int32, Count>(a);
    }

    static inline uint32x4 sll(uint32x4 a, int count)
    {
        return scalar_shift_left<uint32x4, uint32>(a, count);
    }

    static inline uint32x4 srl(uint32x4 a, int count)
    {
        return scalar_shift_right<uint32x4, uint32>(a, count);
    }

    static inline uint32x4 sra(uint32x4 a, int count)
    {
        return scalar_shift_right<uint32x4, int32>(a, count);
    }

    static inline uint32x4 min(uint32x4 a, uint32x4 b)
    {
        return scalar_unroll(scalar_min, a, b);
    }

    static inline uint32x4 max(uint32x4 a, uint32x4 b)
    {
        return scalar_unroll(scalar_max, a, b);
    }

    // -----------------------------------------------------------------
    // uint64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline uint64x2 set_component(uint64x2 a, uint64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline uint64 get_component(uint64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return a[Index];
    }

    static inline uint64x2 uint64x2_zero()
    {
        return scalar_set<uint64, 2>(0);
    }

    static inline uint64x2 uint64x2_set1(uint64 s)
    {
        return scalar_set<uint64, 2>(s);
    }

    static inline uint64x2 uint64x2_set2(uint64 x, uint64 y)
    {
        return {{ x, y }};
    }

    static inline uint64x2 unpacklo(uint64x2 a, uint64x2 b)
    {
        return scalar_unpacklo(a, b);
    }

    static inline uint64x2 unpackhi(uint64x2 a, uint64x2 b)
    {
        return scalar_unpackhi(a, b);
    }

    static inline uint64x2 add(uint64x2 a, uint64x2 b)
    {
        return scalar_unroll(scalar_add, a, b);
    }

    static inline uint64x2 sub(uint64x2 a, uint64x2 b)
    {
        return scalar_unroll(scalar_sub, a, b);
    }

    static inline uint64x2 bitwise_nand(uint64x2 a, uint64x2 b)
    {
        return scalar_unroll(scalar_nand, a, b);
    }

    static inline uint64x2 bitwise_and(uint64x2 a, uint64x2 b)
    {
        return scalar_unroll(scalar_and, a, b);
    }

    static inline uint64x2 bitwise_or(uint64x2 a, uint64x2 b)
    {
        return scalar_unroll(scalar_or, a, b);
    }

    static inline uint64x2 bitwise_xor(uint64x2 a, uint64x2 b)
    {
        return scalar_unroll(scalar_xor, a, b);
    }

    static inline uint64x2 bitwise_not(uint64x2 a)
    {
        return scalar_unroll(scalar_not, a);
    }

    static inline uint64x2 select(mask64x2 mask, uint64x2 a, uint64x2 b)
    {
        return scalar_select(mask, a, b);
    }

    // shift

    template <int Count>
    static inline uint64x2 slli(uint64x2 a)
    {
        return scalar_shift_left<uint64x2, uint64, Count>(a);
    }

    template <int Count>
    static inline uint64x2 srli(uint64x2 a)
    {
        return scalar_shift_right<uint64x2, uint64, Count>(a);
    }

    static inline uint64x2 sll(uint64x2 a, int count)
    {
        return scalar_shift_left<uint64x2, uint64>(a, count);
    }

    static inline uint64x2 srl(uint64x2 a, int count)
    {
        return scalar_shift_right<uint64x2, uint64>(a, count);
    }

    // -----------------------------------------------------------------
    // int8x16
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int8x16 set_component(int8x16 a, int8 s)
    {
        static_assert(Index < 16, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline int8 get_component(int8x16 a)
    {
        static_assert(Index < 16, "Index out of range.");
        return a[Index];
    }

    static inline int8x16 int8x16_zero()
    {
        return scalar_set<int8, 16>(0);
    }

    static inline int8x16 int8x16_set1(int8 s)
    {
        return scalar_set<int8, 16>(s);
    }

    static inline int8x16 unpacklo(int8x16 a, int8x16 b)
    {
        return scalar_unpacklo(a, b);
    }

    static inline int8x16 unpackhi(int8x16 a, int8x16 b)
    {
        return scalar_unpackhi(a, b);
    }

    static inline int8x16 add(int8x16 a, int8x16 b)
    {
        return scalar_unroll(scalar_add, a, b);
    }

    static inline int8x16 sub(int8x16 a, int8x16 b)
    {
        return scalar_unroll(scalar_sub, a, b);
    }

    static inline int8x16 mullo(int8x16 a, int8x16 b)
    {
        return scalar_unroll(scalar_mullo, a, b);
    }

    // saturated

    static inline int8x16 adds(int8x16 a, int8x16 b)
    {
        return scalar_unroll(scalar_signed_adds, a, b);
    }

    static inline int8x16 subs(int8x16 a, int8x16 b)
    {
        return scalar_unroll(scalar_signed_subs, a, b);
    }

    static inline int8x16 abs(int8x16 a)
    {
        return scalar_unroll(scalar_abs, a);
    }

    static inline int8x16 neg(int8x16 a)
    {
        return scalar_unroll(scalar_neg, a);
    }

    // bitwise

    static inline int8x16 bitwise_nand(int8x16 a, int8x16 b)
    {
        return scalar_unroll(scalar_nand, a, b);
    }

    static inline int8x16 bitwise_and(int8x16 a, int8x16 b)
    {
        return scalar_unroll(scalar_and, a, b);
    }

    static inline int8x16 bitwise_or(int8x16 a, int8x16 b)
    {
        return scalar_unroll(scalar_or, a, b);
    }

    static inline int8x16 bitwise_xor(int8x16 a, int8x16 b)
    {
        return scalar_unroll(scalar_xor, a, b);
    }

    static inline int8x16 bitwise_not(int8x16 a)
    {
        return scalar_unroll(scalar_not, a);
    }

    // compare

    static inline mask8x16 compare_eq(int8x16 a, int8x16 b)
    {
        return scalar_compare_eq(a, b);
    }

    static inline mask8x16 compare_gt(int8x16 a, int8x16 b)
    {
        return scalar_compare_gt(a, b);
    }

    static inline mask8x16 compare_neq(int8x16 a, int8x16 b)
    {
        return scalar_compare_neq(a, b);
    }

    static inline mask8x16 compare_lt(int8x16 a, int8x16 b)
    {
        return scalar_compare_lt(a, b);
    }

    static inline mask8x16 compare_le(int8x16 a, int8x16 b)
    {
        return scalar_compare_le(a, b);
    }

    static inline mask8x16 compare_ge(int8x16 a, int8x16 b)
    {
        return scalar_compare_ge(a, b);
    }

    static inline int8x16 select(mask8x16 mask, int8x16 a, int8x16 b)
    {
        return scalar_select(mask, a, b);
    }

    static inline int8x16 min(int8x16 a, int8x16 b)
    {
        return scalar_unroll(scalar_min, a, b);
    }

    static inline int8x16 max(int8x16 a, int8x16 b)
    {
        return scalar_unroll(scalar_max, a, b);
    }

    // -----------------------------------------------------------------
    // int16x8
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int16x8 set_component(int16x8 a, int16 s)
    {
        static_assert(Index < 8, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline int16 get_component(int16x8 a)
    {
        static_assert(Index < 8, "Index out of range.");
        return a[Index];
    }

    static inline int16x8 int16x8_zero()
    {
        return scalar_set<int16, 8>(0);
    }

    static inline int16x8 int16x8_set1(int16 s)
    {
        return scalar_set<int16, 8>(s);
    }

    static inline int16x8 unpacklo(int16x8 a, int16x8 b)
    {
        return scalar_unpacklo(a, b);
    }

    static inline int16x8 unpackhi(int16x8 a, int16x8 b)
    {
        return scalar_unpackhi(a, b);
    }

    static inline int16x8 add(int16x8 a, int16x8 b)
    {
        return scalar_unroll(scalar_add, a, b);
    }

    static inline int16x8 sub(int16x8 a, int16x8 b)
    {
        return scalar_unroll(scalar_sub, a, b);
    }

    static inline int16x8 mullo(int16x8 a, int16x8 b)
    {
        return scalar_unroll(scalar_mullo, a, b);
    }

    // saturated

    static inline int16x8 adds(int16x8 a, int16x8 b)
    {
        return scalar_unroll(scalar_signed_adds, a, b);
    }

    static inline int16x8 subs(int16x8 a, int16x8 b)
    {
        return scalar_unroll(scalar_signed_subs, a, b);
    }

    static inline int16x8 abs(int16x8 a)
    {
        return scalar_unroll(scalar_abs, a);
    }

    static inline int16x8 neg(int16x8 a)
    {
        return scalar_unroll(scalar_neg, a);
    }

    // bitwise

    static inline int16x8 bitwise_nand(int16x8 a, int16x8 b)
    {
        return scalar_unroll(scalar_nand, a, b);
    }

    static inline int16x8 bitwise_and(int16x8 a, int16x8 b)
    {
        return scalar_unroll(scalar_and, a, b);
    }

    static inline int16x8 bitwise_or(int16x8 a, int16x8 b)
    {
        return scalar_unroll(scalar_or, a, b);
    }

    static inline int16x8 bitwise_xor(int16x8 a, int16x8 b)
    {
        return scalar_unroll(scalar_xor, a, b);
    }

    static inline int16x8 bitwise_not(int16x8 a)
    {
        return scalar_unroll(scalar_not, a);
    }

    // compare

    static inline mask16x8 compare_eq(int16x8 a, int16x8 b)
    {
        return scalar_compare_eq(a, b);
    }

    static inline mask16x8 compare_gt(int16x8 a, int16x8 b)
    {
        return scalar_compare_gt(a, b);
    }

    static inline mask16x8 compare_neq(int16x8 a, int16x8 b)
    {
        return scalar_compare_neq(a, b);
    }

    static inline mask16x8 compare_lt(int16x8 a, int16x8 b)
    {
        return scalar_compare_lt(a, b);
    }

    static inline mask16x8 compare_le(int16x8 a, int16x8 b)
    {
        return scalar_compare_le(a, b);
    }

    static inline mask16x8 compare_ge(int16x8 a, int16x8 b)
    {
        return scalar_compare_ge(a, b);
    }

    static inline int16x8 select(mask16x8 mask, int16x8 a, int16x8 b)
    {
        return scalar_select(mask, a, b);
    }

    template <int Count>
    static inline int16x8 slli(int16x8 a)
    {
        return scalar_shift_left<int16x8, uint16, Count>(a);
    }

    template <int Count>
    static inline int16x8 srli(int16x8 a)
    {
        return scalar_shift_right<int16x8, uint16, Count>(a);
    }

    template <int Count>
    static inline int16x8 srai(int16x8 a)
    {
        return scalar_shift_right<int16x8, int16, Count>(a);
    }

    static inline int16x8 sll(int16x8 a, int count)
    {
        return scalar_shift_left<int16x8, uint16>(a, count);
    }

    static inline int16x8 srl(int16x8 a, int count)
    {
        return scalar_shift_right<int16x8, uint16>(a, count);
    }

    static inline int16x8 sra(int16x8 a, int count)
    {
        return scalar_shift_right<int16x8, int16>(a, count);
    }

    static inline int16x8 min(int16x8 a, int16x8 b)
    {
        return scalar_unroll(scalar_min, a, b);
    }

    static inline int16x8 max(int16x8 a, int16x8 b)
    {
        return scalar_unroll(scalar_max, a, b);
    }

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    // shuffle

    template <uint32 x, uint32 y, uint32 z, uint32 w>
    static inline int32x4 shuffle(int32x4 v)
    {
        static_assert(x < 4 && y < 4 && z < 4 && w < 4, "Index out of range.");
        return {{ v[x], v[y], v[z], v[w] }};
    }

    template <>
    inline int32x4 shuffle<0, 1, 2, 3>(int32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <unsigned int Index>
    static inline int32x4 set_component(int32x4 a, int32 s)
    {
        static_assert(Index < 4, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline int32 get_component(int32x4 a)
    {
        static_assert(Index < 4, "Index out of range.");
        return a[Index];
    }

    static inline int32x4 int32x4_zero()
    {
        return {{ 0, 0, 0, 0 }};
    }

    static inline int32x4 int32x4_set1(int s)
    {
        return {{ s, s, s, s }};
    }

    static inline int32x4 int32x4_set4(int x, int y, int z, int w)
    {
        return {{ x, y, z, w }};
    }

    static inline int32x4 int32x4_uload(const int* source)
    {
        return int32x4_set4(source[0], source[1], source[2], source[3]);
    }

    static inline void int32x4_ustore(int* dest, int32x4 a)
    {
        dest[0] = a[0];
        dest[1] = a[1];
        dest[2] = a[2];
        dest[3] = a[3];
    }

    static inline int32x4 unpacklo(int32x4 a, int32x4 b)
    {
        return scalar_unpacklo(a, b);
    }

    static inline int32x4 unpackhi(int32x4 a, int32x4 b)
    {
        return scalar_unpackhi(a, b);
    }

    static inline int32x4 abs(int32x4 a)
    {
        return scalar_unroll(scalar_abs, a);
    }

    static inline int32x4 neg(int32x4 a)
    {
        return scalar_unroll(scalar_neg, a);
    }

    static inline int32x4 add(int32x4 a, int32x4 b)
    {
        return scalar_unroll(scalar_add, a, b);
    }

    static inline int32x4 sub(int32x4 a, int32x4 b)
    {
        return scalar_unroll(scalar_sub, a, b);
    }

    static inline int32x4 mullo(int32x4 a, int32x4 b)
    {
        return scalar_unroll(scalar_mullo, a, b);
    }

    // saturated

    static inline int32x4 adds(int32x4 a, int32x4 b)
    {
        return scalar_unroll(scalar_signed_adds, a, b);
    }

    static inline int32x4 subs(int32x4 a, int32x4 b)
    {
        return scalar_unroll(scalar_signed_subs, a, b);
    }

    // bitwise

    static inline int32x4 bitwise_nand(int32x4 a, int32x4 b)
    {
        return scalar_unroll(scalar_nand, a, b);
    }

    static inline int32x4 bitwise_and(int32x4 a, int32x4 b)
    {
        return scalar_unroll(scalar_and, a, b);
    }

    static inline int32x4 bitwise_or(int32x4 a, int32x4 b)
    {
        return scalar_unroll(scalar_or, a, b);
    }

    static inline int32x4 bitwise_xor(int32x4 a, int32x4 b)
    {
        return scalar_unroll(scalar_xor, a, b);
    }

    static inline int32x4 bitwise_not(int32x4 a)
    {
        return scalar_unroll(scalar_not, a);
    }

    // compare

    static inline mask32x4 compare_eq(int32x4 a, int32x4 b)
    {
        return scalar_compare_eq(a, b);
    }

    static inline mask32x4 compare_gt(int32x4 a, int32x4 b)
    {
        return scalar_compare_gt(a, b);
    }

    static inline mask32x4 compare_neq(int32x4 a, int32x4 b)
    {
        return scalar_compare_neq(a, b);
    }

    static inline mask32x4 compare_lt(int32x4 a, int32x4 b)
    {
        return scalar_compare_lt(a, b);
    }

    static inline mask32x4 compare_le(int32x4 a, int32x4 b)
    {
        return scalar_compare_le(a, b);
    }

    static inline mask32x4 compare_ge(int32x4 a, int32x4 b)
    {
        return scalar_compare_ge(a, b);
    }

    static inline int32x4 select(mask32x4 mask, int32x4 a, int32x4 b)
    {
        return scalar_select(mask, a, b);
    }

    // shift

    template <int Count>
    static inline int32x4 slli(int32x4 a)
    {
        return scalar_shift_left<int32x4, uint32, Count>(a);
    }

    template <int Count>
    static inline int32x4 srli(int32x4 a)
    {
        return scalar_shift_right<int32x4, uint32, Count>(a);
    }

    template <int Count>
    static inline int32x4 srai(int32x4 a)
    {
        return scalar_shift_right<int32x4, int32, Count>(a);
    }

    static inline int32x4 sll(int32x4 a, int count)
    {
        return scalar_shift_left<int32x4, uint32>(a, count);
    }

    static inline int32x4 srl(int32x4 a, int count)
    {
        return scalar_shift_right<int32x4, uint32>(a, count);
    }

    static inline int32x4 sra(int32x4 a, int count)
    {
        return scalar_shift_right<int32x4, int32>(a, count);
    }

    static inline int32x4 min(int32x4 a, int32x4 b)
    {
        return scalar_unroll(scalar_min, a, b);
    }

    static inline int32x4 max(int32x4 a, int32x4 b)
    {
        return scalar_unroll(scalar_max, a, b);
    }

    static inline uint32 pack(int32x4 s)
    {
        const uint32 x = byteclamp(s[0]);
        const uint32 y = byteclamp(s[1]);
        const uint32 z = byteclamp(s[2]);
        const uint32 w = byteclamp(s[3]);
        return x | (y << 8) | (z << 16) | (w << 24);
    }

    static inline int32x4 unpack(uint32 s)
    {
        int32x4 v;
        v[0] = (s >> 0) & 0xff;
        v[1] = (s >> 8) & 0xff;
        v[2] = (s >> 16) & 0xff;
        v[3] = (s >> 24);
        return v;
    }

    // -----------------------------------------------------------------
    // int64x2
    // -----------------------------------------------------------------

    template <unsigned int Index>
    static inline int64x2 set_component(int64x2 a, int64 s)
    {
        static_assert(Index < 2, "Index out of range.");
        a[Index] = s;
        return a;
    }

    template <unsigned int Index>
    static inline int64 get_component(int64x2 a)
    {
        static_assert(Index < 2, "Index out of range.");
        return a[Index];
    }

    static inline int64x2 int64x2_zero()
    {
        return scalar_set<int64, 2>(0);
    }

    static inline int64x2 int64x2_set1(int64 s)
    {
        return scalar_set<int64, 2>(s);
    }

    static inline int64x2 int64x2_set2(int64 x, int64 y)
    {
        return {{ x, y }};
    }

    static inline int64x2 unpacklo(int64x2 a, int64x2 b)
    {
        return scalar_unpacklo(a, b);
    }

    static inline int64x2 unpackhi(int64x2 a, int64x2 b)
    {
        return scalar_unpackhi(a, b);
    }

    static inline int64x2 add(int64x2 a, int64x2 b)
    {
        return scalar_unroll(scalar_add, a, b);
    }

    static inline int64x2 sub(int64x2 a, int64x2 b)
    {
        return scalar_unroll(scalar_sub, a, b);
    }

    static inline int64x2 bitwise_nand(int64x2 a, int64x2 b)
    {
        return scalar_unroll(scalar_nand, a, b);
    }

    static inline int64x2 bitwise_and(int64x2 a, int64x2 b)
    {
        return scalar_unroll(scalar_and, a, b);
    }

    static inline int64x2 bitwise_or(int64x2 a, int64x2 b)
    {
        return scalar_unroll(scalar_or, a, b);
    }

    static inline int64x2 bitwise_xor(int64x2 a, int64x2 b)
    {
        return scalar_unroll(scalar_xor, a, b);
    }

    static inline int64x2 bitwise_not(int64x2 a)
    {
        return scalar_unroll(scalar_not, a);
    }

    static inline int64x2 select(mask64x2 mask, int64x2 a, int64x2 b)
    {
        return scalar_select(mask, a, b);
    }

    // shift

    template <int Count>
    static inline int64x2 slli(int64x2 a)
    {
        return scalar_shift_left<int64x2, uint64, Count>(a);
    }

    template <int Count>
    static inline int64x2 srli(int64x2 a)
    {
        return scalar_shift_right<int64x2, uint64, Count>(a);
    }

    static inline int64x2 sll(int64x2 a, int count)
    {
        return scalar_shift_left<int64x2, uint64>(a, count);
    }

    static inline int64x2 srl(int64x2 a, int count)
    {
        return scalar_shift_right<int64x2, uint64>(a, count);
    }

    // -----------------------------------------------------------------
    // mask8x16
    // -----------------------------------------------------------------

    static inline mask8x16 operator & (mask8x16 a, mask8x16 b)
    {
        return a.mask & b.mask;
    }

    static inline mask8x16 operator | (mask8x16 a, mask8x16 b)
    {
        return a.mask | b.mask;
    }

    static inline mask8x16 operator ^ (mask8x16 a, mask8x16 b)
    {
        return a.mask ^ b.mask;
    }

    // -----------------------------------------------------------------
    // mask16x8
    // -----------------------------------------------------------------

    static inline mask16x8 operator & (mask16x8 a, mask16x8 b)
    {
        return a.mask & b.mask;
    }

    static inline mask16x8 operator | (mask16x8 a, mask16x8 b)
    {
        return a.mask | b.mask;
    }

    static inline mask16x8 operator ^ (mask16x8 a, mask16x8 b)
    {
        return a.mask ^ b.mask;
    }

    // -----------------------------------------------------------------
    // mask32x4
    // -----------------------------------------------------------------

    static inline mask32x4 operator & (mask32x4 a, mask32x4 b)
    {
        return a.mask & b.mask;
    }

    static inline mask32x4 operator | (mask32x4 a, mask32x4 b)
    {
        return a.mask | b.mask;
    }

    static inline mask32x4 operator ^ (mask32x4 a, mask32x4 b)
    {
        return a.mask ^ b.mask;
    }

    static inline uint32 get_mask(mask32x4 a)
    {
        return a.mask;
    }

    // -----------------------------------------------------------------
    // mask64x2
    // -----------------------------------------------------------------

    static inline mask64x2 operator & (mask64x2 a, mask64x2 b)
    {
        return a.mask & b.mask;
    }

    static inline mask64x2 operator | (mask64x2 a, mask64x2 b)
    {
        return a.mask | b.mask;
    }

    static inline mask64x2 operator ^ (mask64x2 a, mask64x2 b)
    {
        return a.mask ^ b.mask;
    }

} // namespace simd
} // namespace mango

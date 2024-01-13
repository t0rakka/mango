/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cctype>
#include <algorithm>
#include <locale>
#include <cstring>
#include <mango/core/string.hpp>
#include <mango/core/bits.hpp>
#include "../../external/fast_float/fast_float.h"

namespace
{
    using mango::u8;
    using mango::u32;

    /*
        WARNING!
        StringBuilder eliminates small-string construction overhead when
        the string is constructed one element at a time. A small temporary
        buffer is filled and flushed so that string does not have to
        continuously grow it's capacity. The constructed string is not
        a member so that optimizing compilers can utilize NRVO.

        Guardband is utilized so that insertions do not have to be checked
        individually and the loops can be unrolled. This requires intimate
        knowledge of the decoder feeding the buffer. Incorrectly configured
        guardband WILL cause buffer overflow; handle this code with care.
    */
    template <typename T, int SIZE, int GUARDBAND>
    struct StringBuilder
    {
        std::basic_string<T>& s;
        T buffer[SIZE + GUARDBAND];
        T* ptr;
        T* end;

        StringBuilder(std::basic_string<T>& str)
            : s(str), ptr(buffer), end(buffer + SIZE)
        {
        }

        ~StringBuilder()
        {
        }

        void ensure()
        {
            if (ptr >= end)
            {
                s.append(buffer, ptr);
                ptr = buffer;
            }
        }

        void flush()
        {
            s.append(buffer, ptr);
        }
    };

    template <typename S, typename T>
    inline std::vector<S> splitTemplate(const S& s, T delimiter)
    {
        std::vector<S> result;

        std::size_t current = 0;
        std::size_t p = s.find_first_of(delimiter, 0);

        while (p != S::npos)
        {
            result.emplace_back(s.substr(current, p - current));
            current = p + 1;
            p = s.find_first_of(delimiter, current);
        }

        result.emplace_back(s.substr(current));

        return result;
    }

    // -----------------------------------------------------------------
    // Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
    // See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
    // -----------------------------------------------------------------

    enum {
        UTF8_ACCEPT = 0,
        UTF8_REJECT = 12
    };

    inline u32 utf8_decode(u32& state, u32& code, u32 byte)
    {
        static const u8 utf8d[] = {
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
            7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
            8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
            0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
            0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
            0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
            1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
            1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
            1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
        };

        const u32 type = utf8d[byte];

        code = (state != UTF8_ACCEPT) ? (byte & 0x3fu) | (code << 6) : (0xff >> type) & (byte);
        state = utf8d[256 + state * 16 + type];
        return state;
    }

    inline char* utf8_encode(char* ptr, u32 code)
    {
        const u32 mask = 0x3f;

        if (code < 0x80)
        {
            *ptr++ = code;
        }
        else if (code < 0x800)
        {
            ptr[1] = 0x80 | (code & mask); code >>= 6;
            ptr[0] = 0xc0 | code;
            ptr += 2;
        }
        else if (code < 0x10000)
        {
            ptr[2] = 0x80 | (code & mask); code >>= 6;
            ptr[1] = 0x80 | (code & mask); code >>= 6;
            ptr[0] = 0xe0 | code;
            ptr += 3;
        }
        else if (code < 0x200000)
        {
            ptr[3] = 0x80 | (code & mask); code >>= 6;
            ptr[2] = 0x80 | (code & mask); code >>= 6;
            ptr[1] = 0x80 | (code & mask); code >>= 6;
            ptr[0] = 0xf0 | code;
            ptr += 4;
        }

        return ptr;
    }

} // namespace

namespace mango
{

    // -----------------------------------------------------------------
    // unicode conversions
    // -----------------------------------------------------------------

    bool is_utf8(const std::string& source)
    {
        u32 code = 0;
        u32 state = 0;

        for (u8 c : source)
            utf8_decode(state, code, c);

        return state == UTF8_ACCEPT;
    }

    std::u32string utf32_from_utf8(const std::string& source)
    {
        std::u32string s;
        StringBuilder<char32_t, 256, 1> sb(s);

        u32 state = 0;
        u32 code = 0;

        for (u8 c : source)
        {
            if (!utf8_decode(state, code, c))
            {
                sb.ensure();
                *sb.ptr++ = code;
            }
        }

        sb.flush();
        return s;
    }

    std::string utf8_from_utf32(const std::u32string& source)
    {
        std::string s;
        StringBuilder<char, 128, 16> sb(s);

        const size_t length4 = source.length() & ~3;
        size_t i = 0;

        for (; i < length4; i += 4)
        {
            sb.ensure();
            sb.ptr = utf8_encode(sb.ptr, source[i + 0]);
            sb.ptr = utf8_encode(sb.ptr, source[i + 1]);
            sb.ptr = utf8_encode(sb.ptr, source[i + 2]);
            sb.ptr = utf8_encode(sb.ptr, source[i + 3]);
        }

        sb.ensure();
        for ( ; i < source.length(); ++i)
        {
            sb.ptr = utf8_encode(sb.ptr, source[i]);
        }

        sb.flush();
        return s;
    }

    std::u16string utf16_from_utf8(const std::string& source)
    {
        std::u16string s;
        StringBuilder<char16_t, 256, 2> sb(s);

        u32 state = 0;
        u32 code = 0;

        for (u8 c : source)
        {
            if (!utf8_decode(state, code, c))
            {
                sb.ensure();

                // encode into temporary buffer
                if (code <= 0xffff)
                {
                    *sb.ptr++ = code;
                }
                else
                {
                    // encode code points above U+FFFF as surrogate pair
                    sb.ptr[0] = 0xd7c0 + (code >> 10);
                    sb.ptr[1] = 0xdc00 + (code & 0x3ff);
                    sb.ptr += 2;
                }
            }
        }

        sb.flush();
        return s;
    }

    std::string utf8_from_utf16(const std::u16string& source)
    {
        std::string s;
        StringBuilder<char, 256, 4> sb(s);

        const size_t length = source.length();

        for (size_t i = 0; i < length; ++i)
        {
            u32 code = source[i];

            // decode surrogate pair
            if ((code - 0xd800) < 0x400)
            {
                const u32 low = source[++i];

                if ((low - 0xdc00) < 0x400)
                {
                    code = ((code - 0xd800) << 10) + (low - 0xdc00) + 0x10000;
                }
            }

            sb.ensure();
            sb.ptr = utf8_encode(sb.ptr, code);
        }

        sb.flush();
        return s;
    }

    std::string u16_toBytes(const std::wstring& source)
    {
        // MANGO TODO: validate against reference implementation
#if 0
        std::string s;
        StringBuilder<char, 256, 4> sb(s);

        const size_t length = source.length();

        for (size_t i = 0; i < length; ++i)
        {
            const u32 mask = 0x3f;
            const u32 code = source[i];

            sb.ensure();

            if (code < 0x80)
            {
                *sb.ptr++ = code;
            }
            else
            {
                sb.ptr[0] = 0xc0 | (code >> 6);
                sb.ptr[1] = 0x80 | (code & mask);
                sb.ptr += 2;
            }
        }

        sb.flush();
        return s;
#else
        const wchar_t* src = source.c_str();
        std::string s;

        while (*src)
        {
            u32 c = *src++;

            if (c < 0x80)
            {
                s.push_back(c);
            }
            else if (c < 0x800)
            {
                s.push_back(0xc0 | (c >> 6));
                s.push_back(0x80 | (c & 0x3f));
            }
            else if (c < 0x10000)
            {
                s.push_back(0xe0 | (c >> 12));
                s.push_back(0x80 | ((c >> 6) & 0x3f));
                s.push_back(0x80 | (c & 0x3f));
            }
            else if (c < 0x200000)
            {
                s.push_back(0xf0 | (c >> 18));
                s.push_back(0x80 | ((c >> 12) & 0x3f));
                s.push_back(0x80 | ((c >> 6) & 0x3f));
                s.push_back(0x80 | (c & 0x3f));
            }
        }

        return s;
#endif
    }

    std::wstring u16_fromBytes(const std::string& source)
    {
        // MANGO TODO: validate against reference implementation
#if 0
        std::wstring s;
        StringBuilder<wchar_t, 256, 1> sb(s);

        u32 state = 0;
        u32 code = 0;

        for (u8 c : source)
        {
            if (!utf8_decode(state, code, c))
            {
                if (code <= 0xffff)
                {
                    sb.ensure();
                    *sb.ptr++ = code;
                }
            }
        }

        sb.flush() ;
        return s;
#else
        const char* src = source.c_str();
        std::wstring s;

        while (*src)
        {
            u32 c = static_cast<u8>(*src++);
            u32 d;

            if (c < 0x80)
            {
                d = c;
            }
            else if ((c >> 5) == 6)
            {
                if ((*src & 0xc0) != 0x80)
                    break;
                d = ((c & 0x1f) << 6) | (*src & 0x3f);
                src++;
            }
            else if ((c >> 4) == 14)
            {
                if ((src[0] & 0xc0) != 0x80 || (src[1] & 0xc0) != 0x80)
                    break;
                d = ((c & 0xf) << 12) | ((src[0] & 0x3f) << 6) | (src[1] & 0x3f);
                src += 2;
            }
            else if ((c >> 3) == 30)
            {
                if ((src[0] & 0xc0) != 0x80 || (src[1] & 0xc0) != 0x80 || (src[2] & 0xc0) != 0x80)
                    break;
                d = ((c & 7) << 18) | ((src[0] & 0x3f) << 12) | ((src[1] & 0x3f) << 6) | (src[2] & 0x3f);
                src += 3;
            }
            else
            {
                // Ignore bad characters
                continue;
            }

            if (d > 0xffff)
            {
                if (d > 0x10ffff)
                    continue;
                s.push_back(((d - 0x10000) >> 10) + 0xd800);
                s.push_back((d & 0x3ff) + 0xdc00);
            }
            else
            {
                s.push_back(d);
            }
        }

        return s;
#endif
    }

    // -----------------------------------------------------------------
    // string utilities
    // -----------------------------------------------------------------

    std::string toLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    std::string toUpper(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    }

    std::string removePrefix(std::string_view s, std::string_view prefix)
    {
        size_t offset = s.find(prefix) != std::string_view::npos ? prefix.length() : 0;
        return std::string(s.begin() + offset, s.end());
    }

    bool isPrefix(std::string_view s, std::string_view prefix)
    {
        return s.length() > prefix.length() && !s.find(prefix, 0);
    }

    bool isMatch(std::string_view text, std::string_view pattern)
    {
        // Based on this article:
        // https://www.geeksforgeeks.org/wildcard-pattern-matching/
        // Summary: O(n) wildcard pattern matching

        const size_t textLength = text.length();
        const size_t patternLength = pattern.length();

        size_t textIndex = 0;
        size_t patternIndex = 0;
        size_t startIndex = std::string_view::npos;
        size_t matchIndex = 0;

        while (textIndex < textLength)
        {
            if (patternIndex < patternLength && (pattern[patternIndex] == '?' || pattern[patternIndex] == text[textIndex]))
            {
                // Characters match or '?' in pattern matches any character.
                ++textIndex;
                ++patternIndex;
            }
            else if (patternIndex < patternLength && pattern[patternIndex] == '*')
            {
                // Wildcard character '*', mark the current position in the pattern and the text as a proper match.
                startIndex = patternIndex;
                matchIndex = textIndex;
                ++patternIndex;
            }
            else if (startIndex != std::string_view::npos)
            {
                // No match, but a previous wildcard was found. Backtrack to the last '*' character position and try for a different match.
                patternIndex = startIndex + 1;
                textIndex = ++matchIndex;
            }
            else
            {
                // If none of the above cases comply, the pattern does not match.
                return false;
            }
        }

        // Consume any remaining '*' characters in the given pattern.
        while (patternIndex < patternLength && pattern[patternIndex] == '*')
        {
            ++patternIndex;
        }

        // If we have reached the end of both the pattern and the text, the pattern matches the text.
        return patternIndex == patternLength;
    }

    void replace(std::string& s, std::string_view from, std::string_view to)
    {
        if (from.empty())
            return;

        size_t start = 0;
        while ((start = s.find(from, start)) != std::string::npos)
        {
            s.replace(start, from.length(), to);
            start += to.length();
        }
    }

    std::vector<std::string> split(const std::string& s, char delimiter)
    {
        return splitTemplate(s, delimiter);
    }

    std::vector<std::string> split(const std::string& s, const char* delimiter)
    {
        return splitTemplate(s, delimiter);
    }

    std::vector<std::string> split(const std::string& s, const std::string& delimiter)
    {
        return splitTemplate(s, delimiter);
    }

    std::vector<std::string_view> split(std::string_view s, std::string_view delimiter)
    {
        return splitTemplate(s, delimiter);
    }

    // ----------------------------------------------------------------------------
    // memchr()
    // ----------------------------------------------------------------------------

#if defined(MANGO_ENABLE_SSE2)

    const u8* memchr(const u8* p, u8 value, size_t count)
    {
        __m128i ref = _mm_set1_epi8(value);
        while (count >= 16)
        {
            __m128i v = _mm_loadu_si128(reinterpret_cast<__m128i const *>(p));
            u32 mask = _mm_movemask_epi8(_mm_cmpeq_epi8(v, ref));
            if (mask)
            {
                int index = u32_tzcnt(mask);
                return p + index;
            }

            count -= 16;
            p += 16;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (p[i] == value)
                return p + i;
        }

        return nullptr;
    }

#else

    const u8* memchr(const u8* p, u8 value, size_t count)
    {
        p = reinterpret_cast<const u8 *>(std::memchr(p, value, count));
        return p;
    }

#endif

    float parseFloat(std::string_view s)
    {
        float value = 0.0f;
        fast_float::from_chars(s.data(), s.data() + s.size(), value);
        return value;
    }

} // namespace mango

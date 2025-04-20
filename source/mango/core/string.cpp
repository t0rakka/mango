/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cctype>
#include <algorithm>
#include <locale>
#include <cstring>
#include <charconv>
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
            : s(str)
            , ptr(buffer)
            , end(buffer + SIZE)
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

    enum
    {
        UTF8_ACCEPT = 0,
        UTF8_REJECT = 12
    };

    static inline
    u32 utf8_decode(u32& state, u32& code, u32 byte)
    {
        static const u8 utf8d [] =
        {
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

    static inline
    char* utf8_encode(char* ptr, u32 code)
    {
        const u32 mask = 0x3f;

        if (code < 0x80)
        {
            *ptr++ = char(code);
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
        {
            utf8_decode(state, code, c);
        }

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
                    *sb.ptr++ = char16_t(code);
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
        std::string s;
        StringBuilder<char, 256, 4> sb(s);

        const size_t length = source.length();

        for (size_t i = 0; i < length; ++i)
        {
            const u32 code = source[i];

            sb.ensure();

            if (code < 0x80)
            {
                sb.ptr[0] = char(code);
                sb.ptr += 1;
            }
            else if (code < 0x800)
            {
                sb.ptr[0] = 0xc0 | (code >> 6);
                sb.ptr[1] = 0x80 | (code & 0x3f);
                sb.ptr += 2;
            }
            else if (code < 0x10000)
            {
                sb.ptr[0] = 0xe0 | (code >> 12);
                sb.ptr[1] = 0x80 | ((code >> 6) & 0x3f);
                sb.ptr[2] = 0x80 | (code & 0x3f);
                sb.ptr += 3;
            }
            else if (code < 0x200000)
            {
                sb.ptr[0] = 0xf0 | (code >> 18);
                sb.ptr[1] = 0x80 | ((code >> 12) & 0x3f);
                sb.ptr[2] = 0x80 | ((code >> 6) & 0x3f);
                sb.ptr[3] = 0x80 | (code & 0x3f);
                sb.ptr += 4;
            }
        }

        sb.flush();
        return s;
    }

    std::wstring u16_fromBytes(const std::string& source)
    {
        std::wstring s;
        StringBuilder<wchar_t, 256, 2> sb(s);

        const char* src = source.c_str();
        while (*src)
        {
            u32 code = static_cast<u8>(*src++);

            if (code >= 0x80)
            {
                if ((code >> 5) == 6)
                {
                    if ((src[0] & 0xc0) != 0x80)
                        break;

                    code = ((code & 0x1f) << 6) | (*src & 0x3f);
                    src += 1;
                }
                else if ((code >> 4) == 14)
                {
                    if ((src[0] & 0xc0) != 0x80 || (src[1] & 0xc0) != 0x80)
                        break;

                    code = ((code & 0xf) << 12) | ((src[0] & 0x3f) << 6) | (src[1] & 0x3f);
                    src += 2;
                }
                else if ((code >> 3) == 30)
                {
                    if ((src[0] & 0xc0) != 0x80 || (src[1] & 0xc0) != 0x80 || (src[2] & 0xc0) != 0x80)
                        break;

                    code = ((code & 7) << 18) | ((src[0] & 0x3f) << 12) | ((src[1] & 0x3f) << 6) | (src[2] & 0x3f);
                    src += 3;
                }
                else
                {
                    // Ignore bad characters
                    continue;
                }
            }

            sb.ensure();

            if (code > 0xffff)
            {
                if (code > 0x10ffff)
                    continue;

                sb.ptr[0] = wchar_t(((code - 0x10000) >> 10) + 0xd800);
                sb.ptr[1] = wchar_t((code & 0x3ff) + 0xdc00);
                sb.ptr += 2;
            }
            else
            {
                sb.ptr[0] = wchar_t(code);
                sb.ptr += 1;
            }
        }

        sb.flush();
        return s;
    }

    // -----------------------------------------------------------------
    // string utilities
    // -----------------------------------------------------------------

    std::string toLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [] (unsigned char c)
        {
            return char(std::tolower(c));
        });

        return s;
    }

    std::string toUpper(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [] (unsigned char c)
        {
            return char(std::toupper(c));
        });

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

        for (size_t index = 0; index < count; ++index)
        {
            if (p[index] == value)
                return p + index;
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

    // ----------------------------------------------------------------------------
    // stringLength()
    // ----------------------------------------------------------------------------

    // strnlen, strnlen_s and others are not well supported by all compiler and is
    // generally a hassle to choose which one to use with different tool-chains.
    //
    // Let's just re-invent the wheel and forget this ever happened.

    size_t stringLength(const char* s, size_t maxlen)
    {
        if (!s)
        {
            return 0;
        }

        const u8* p0 = reinterpret_cast<const u8*>(s);
        const u8* p1 = memchr(p0, 0, maxlen);
        return p1 ? p1 - p0 : maxlen;
    }

    float parseFloat(std::string_view s)
    {
        float value = 0.0f;
        fast_float::from_chars(s.data(), s.data() + s.size(), value);
        return value;
    }

    int parseInt(std::string_view str)
    {
        int value = 0;
        std::from_chars(str.data(), str.data() + str.size(), value);
        return value;
    }

} // namespace mango

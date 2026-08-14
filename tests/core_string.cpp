/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "core_test.hpp"

#include <string>

using namespace mango;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    bool test_is_utf8_valid()
    {
        CHECK(is_utf8(std::string_view("")));
        CHECK(is_utf8(std::string_view("hello")));
        CHECK(is_utf8(std::string_view("caf\xc3\xa9")));                  // U+00E9, 2-byte
        CHECK(is_utf8(std::string_view("\xe2\x82\xac")));                // U+20AC, 3-byte euro
        CHECK(is_utf8(std::string_view("\xf0\x9f\x98\x80")));            // U+1F600, grinning face
        CHECK(is_utf8(std::string_view("\xf4\x8f\xbf\xbf")));            // U+10FFFF, last scalar

        return true;
    }

    bool test_is_utf8_invalid()
    {
        CHECK(!is_utf8(std::string_view("\x80")));
        CHECK(!is_utf8(std::string_view("\xc0\x80")));                   // overlong NUL
        CHECK(!is_utf8(std::string_view("\xed\xa0\x80")));               // UTF-8 encoded surrogate
        CHECK(!is_utf8(std::string_view("\xe2\x82")));                   // truncated 3-byte
        CHECK(!is_utf8(std::string_view("a\xe2\x82")));                  // truncated at end
        CHECK(!is_utf8(std::string_view("\xfe\xff")));                   // invalid lead byte

        return true;
    }

    bool test_utf32_roundtrip()
    {
        const std::string source = "ascii \xc3\xa4 \xe2\x82\xac \xf0\x9f\x98\x80";

        std::u32string codepoints = utf32_from_utf8(source);
        CHECK(codepoints.size() == 11);
        CHECK(codepoints[0] == U'a');
        CHECK(codepoints[6] == U'\u00E4');
        CHECK(codepoints[8] == U'\u20AC');
        CHECK(codepoints[10] == U'\U0001F600');

        std::string restored = utf8_from_utf32(codepoints);
        CHECK(restored == source);

        return true;
    }

    bool test_utf16_roundtrip()
    {
        const std::string source = "plane0 \xf0\x9f\x98\x80";

        std::u16string units = utf16_from_utf8(source);
        CHECK(units.size() == 9);
        CHECK(units[7] == 0xd83d);
        CHECK(units[8] == 0xde00);

        std::string restored = utf8_from_utf16(units);
        CHECK(restored == source);

        return true;
    }

    bool test_utf16_from_utf8_direct()
    {
        const std::u16string expected = { u'h', u'i' };
        std::u16string units = utf16_from_utf8(std::string_view("hi"));
        CHECK(units == expected);

        return true;
    }

    bool test_invalid_utf8_replacement()
    {
        const std::string broken = "ok\xc0\x80tail";

        std::u32string codepoints = utf32_from_utf8(broken);
        CHECK(codepoints.size() == 7);
        CHECK(codepoints[0] == U'o');
        CHECK(codepoints[1] == U'k');
        CHECK(codepoints[2] == U'\uFFFD');
        CHECK(codepoints[3] == U't');
        CHECK(codepoints[4] == U'a');
        CHECK(codepoints[5] == U'i');
        CHECK(codepoints[6] == U'l');

        std::u16string units = utf16_from_utf8(broken);
        CHECK(units.size() == 7);
        CHECK(units[2] == u'\uFFFD');
        CHECK(units[3] == u't');

        return true;
    }

    bool test_truncated_utf8_replacement()
    {
        const std::string truncated = "abc\xe2\x82";

        CHECK(!is_utf8(truncated));

        std::u32string codepoints = utf32_from_utf8(truncated);
        CHECK(codepoints.size() == 4);
        CHECK(codepoints[0] == U'a');
        CHECK(codepoints[1] == U'b');
        CHECK(codepoints[2] == U'c');
        CHECK(codepoints[3] == U'\uFFFD');

        return true;
    }

    bool test_utf8_from_utf32_scalars()
    {
        const std::u32string codepoints =
        {
            U'a',
            U'\u00E4',
            U'\u20AC',
            U'\U0001F600',
        };

        std::string utf8 = utf8_from_utf32(codepoints);
        CHECK(is_utf8(std::string_view(utf8)));
        CHECK(utf32_from_utf8(utf8) == codepoints);

        return true;
    }

    const Case g_cases [] =
    {
        { "is_utf8 valid",              test_is_utf8_valid },
        { "is_utf8 invalid",            test_is_utf8_invalid },
        { "utf32 roundtrip",            test_utf32_roundtrip },
        { "utf16 roundtrip",            test_utf16_roundtrip },
        { "utf16 from utf8 direct",     test_utf16_from_utf8_direct },
        { "invalid utf8 replacement",   test_invalid_utf8_replacement },
        { "truncated utf8 replacement", test_truncated_utf8_replacement },
        { "utf8 from utf32 scalars",    test_utf8_from_utf32_scalars },
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("core_string", g_cases, std::size(g_cases), argc, argv);
}

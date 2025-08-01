/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdarg>
#include <mango/core/configure.hpp>

#ifdef _MSC_VER
    #define FMT_UNICODE 0
    #pragma warning(disable : 5045)
#endif
#include <fmt/format.h>
#include <fmt/color.h>

namespace mango
{

    // unicode conversions
    bool is_utf8(const std::string& s);
    std::u16string utf16_from_utf8(const std::string& str);
    std::u32string utf32_from_utf8(const std::string& str);
    std::string utf8_from_utf16(const std::u16string& str);
    std::string utf8_from_utf32(const std::u32string& str);

    // microsoft specific unicode conversions
    std::string u16_toBytes(const std::wstring& source);
    std::wstring u16_fromBytes(const std::string& source);

    // string utilities
    std::string toLower(std::string_view s) noexcept;
    std::string toUpper(std::string_view s) noexcept;
    std::string removePrefix(std::string_view s, std::string_view prefix) noexcept;
    bool isPrefix(std::string_view s, std::string_view prefix) noexcept;
    bool isMatch(std::string_view s, std::string_view pattern) noexcept;
    void replace(std::string& s, std::string_view from, std::string_view to) noexcept;
    std::vector<std::string> split(const std::string& s, char delimiter) noexcept;
    std::vector<std::string> split(const std::string& s, const char* delimiter) noexcept;
    std::vector<std::string> split(const std::string& s, const std::string& delimiter) noexcept;
    std::vector<std::string_view> split(std::string_view s, std::string_view delimiter) noexcept;
    const u8* memchr(const u8* p, u8 value, size_t count) noexcept;
    size_t stringLength(const char* s, size_t maxlen) noexcept;
    float parseFloat(std::string_view str) noexcept;
    int parseInt(std::string_view str) noexcept;

} // namespace mango

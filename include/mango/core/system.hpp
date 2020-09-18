/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <cstdio>
#include <mango/core/configure.hpp>

namespace mango
{

    std::string getSystemInfo();

    // --------------------------------------------------------------
    // debugPrint()
    // --------------------------------------------------------------

#ifdef MANGO_ENABLE_DEBUG_PRINT
    #define debugPrint(...) \
        std::printf(__VA_ARGS__); \
        std::fflush(stdout)
#else
    #define debugPrint(...)
#endif

} // namespace mango

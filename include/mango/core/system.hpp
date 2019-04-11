/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <cstdio>
#include "configure.hpp"

namespace mango
{

    std::string getSystemInfo();

#ifdef MANGO_ENABLE_DEBUG_PRINT
    #define debugPrint(...) std::printf(__VA_ARGS__)
#else
    #define debugPrint(...)
#endif

} // namespace mango

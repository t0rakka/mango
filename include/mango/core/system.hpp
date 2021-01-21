/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <mango/core/configure.hpp>

namespace mango
{

    std::string getSystemInfo();

    bool debugPrintIsEnable();
    void debugPrintEnable(bool enable);
    void debugPrint(const char* format, ...);

} // namespace mango

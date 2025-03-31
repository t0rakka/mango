/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

namespace mango
{

    struct GLXConfiguration
    {
        GLXFBConfig selected;
        std::set<std::string_view> extensions;
    
        GLXConfiguration(Display* display, int screen, const OpenGLContext::Config* pConfig);
    };
    
} // namespace mango

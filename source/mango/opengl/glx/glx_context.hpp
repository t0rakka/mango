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
    
        GLXConfiguration(Display* display, int screen, const OpenGLWindow::Config* pConfig);

        GLXContext createContext(Display* display, GLXContext shared);
    };

    // Apply every available swap-interval extension. EXT is often advertised under
    // compositors but is a silent no-op; MESA/SGI may still enforce blocking.
    void glxSetSwapInterval(Display* display, GLXDrawable drawable, int interval);
    
} // namespace mango

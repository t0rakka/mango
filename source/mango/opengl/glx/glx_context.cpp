/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>

#if defined(MANGO_WINDOW_SYSTEM_XLIB) || defined(MANGO_WINDOW_SYSTEM_XCB)

#include "glx_context.hpp"

#include <GL/glx.h>
#include <X11/Xlib.h>

namespace mango
{
    using namespace math;

    GLXConfiguration::GLXConfiguration(Display* display, int screen, const OpenGLContext::Config* pConfig)
    {
        // resolve configuration
        OpenGLContext::Config config;
        if (pConfig)
        {
            // override defaults
            config = *pConfig;
        }

        // Get GLX extensions
        const char* exts = glXQueryExtensionsString(display, screen);
        if (exts)
        {
            // parse extensions
            for (const char* s = exts; *s; ++s)
            {
                if (*s == ' ')
                {
                    const std::ptrdiff_t length = s - exts;
                    if (length > 0)
                    {
                        extensions.emplace(exts, length);
                    }

                    exts = s + 1;
                }
            }
        }

        // Get GLX version
        int version_major;
        int version_minor;
        if (!glXQueryVersion(display, &version_major, &version_minor))
        {
            MANGO_EXCEPTION("[OpenGLContext] glXQueryVersion() failed.");
        }

        printLine(Print::Info, "GLX version: {}.{}", version_major, version_minor);

        if (version_major * 10 + version_minor < 13)
        {
            MANGO_EXCEPTION("[OpenGLContext] Invalid GLX version).");
        }

        // Configure attributes
        std::vector<int> visualAttribs;

        visualAttribs.push_back(GLX_X_RENDERABLE);
        visualAttribs.push_back(True);

        visualAttribs.push_back(GLX_DRAWABLE_TYPE);
        visualAttribs.push_back(GLX_WINDOW_BIT);

        visualAttribs.push_back(GLX_X_VISUAL_TYPE);
        visualAttribs.push_back(GLX_TRUE_COLOR);

        visualAttribs.push_back(GLX_DOUBLEBUFFER);
        visualAttribs.push_back(True);

        visualAttribs.push_back(GLX_DEPTH_SIZE);
        visualAttribs.push_back(config.depth);

        visualAttribs.push_back(GLX_STENCIL_SIZE);
        visualAttribs.push_back(config.stencil);

        // helper lambdas

        auto chooseConfig = [=, this] (std::vector<int> attribs) -> std::vector<GLXFBConfig>
        {
            attribs.push_back(None);

            int numFBConfigs = 0;
            GLXFBConfig* fbconfigs = glXChooseFBConfig(display, screen, attribs.data(), &numFBConfigs);
            std::vector<GLXFBConfig> configs(fbconfigs + 0, fbconfigs + numFBConfigs);
            XFree(fbconfigs);

            return configs;
        };

        // select first config that satisfies the basic requirements

        auto fbconfigs = chooseConfig(visualAttribs);
        if (fbconfigs.empty())
        {
            MANGO_EXCEPTION("[OpenGLContext] glXChooseFBConfig() failed.");
        }

        selected = fbconfigs[0];

        // try config with float capabilities

        bool float_buffer_selected = false;

        if (config.red > 8 || config.green > 8 || config.blue > 8)
        {
            auto tempAttribs = visualAttribs;

            tempAttribs.push_back(GLX_RED_SIZE);
            tempAttribs.push_back(config.red);

            tempAttribs.push_back(GLX_GREEN_SIZE);
            tempAttribs.push_back(config.green);

            tempAttribs.push_back(GLX_BLUE_SIZE);
            tempAttribs.push_back(config.blue);

            tempAttribs.push_back(GLX_ALPHA_SIZE);
            tempAttribs.push_back(config.alpha);

            tempAttribs.push_back(GLX_RENDER_TYPE);
            tempAttribs.push_back(GLX_RGBA_FLOAT_TYPE_ARB);

            auto temp = chooseConfig(tempAttribs);
            if (!temp.empty())
            {
                fbconfigs = temp;
                selected = fbconfigs[0];
                float_buffer_selected = true;
            }
            else
            {
                // float config not found; use default values
                config.red = 8;
                config.green = 8;
                config.blue = 8;
                config.alpha = 8;
            }
        }

        if (!float_buffer_selected)
        {
            visualAttribs.push_back(GLX_RED_SIZE);
            visualAttribs.push_back(config.red);

            visualAttribs.push_back(GLX_GREEN_SIZE);
            visualAttribs.push_back(config.green);

            visualAttribs.push_back(GLX_BLUE_SIZE);
            visualAttribs.push_back(config.blue);

            visualAttribs.push_back(GLX_ALPHA_SIZE);
            visualAttribs.push_back(config.alpha);

            auto temp = chooseConfig(visualAttribs);
            if (!temp.empty())
            {
                fbconfigs = temp;
                selected = fbconfigs[0];
            }
        }

        if (extensions.find("GLX_ARB_fbconfig_float") != extensions.end())
        {
            //printLine(Print::Info, "GLX_ARB_fbconfig_float : ENABLE");
        }

        if (extensions.find("GLX_ARB_framebuffer_sRGB") != extensions.end())
        {
            //printLine(Print::Info, "GLX_ARB_framebuffer_sRGB : ENABLE");
        }

        if (extensions.find("GLX_ARB_multisample") != extensions.end())
        {
            if (config.samples > 1)
            {
                auto tempAttribs = visualAttribs;

                tempAttribs.push_back(GLX_SAMPLE_BUFFERS_ARB);
                tempAttribs.push_back(1);

                tempAttribs.push_back(GLX_SAMPLES_ARB);
                tempAttribs.push_back(config.samples);

                auto temp = chooseConfig(tempAttribs);
                if (!temp.empty())
                {
                    printLine(Print::Info, "GLX_ARB_multisample : ENABLE");
                    fbconfigs = temp;
                    selected = fbconfigs[0];
                }
            }
        }
    }

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_XLIB) || defined(MANGO_WINDOW_SYSTEM_XCB)

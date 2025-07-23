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

#ifndef GLX_CONTEXT_SHARE_CONTEXT_ARB
#define GLX_CONTEXT_SHARE_CONTEXT_ARB        0x2090
#endif

namespace mango
{
    using namespace math;

    static
    int contextErrorHandler(Display* display, XErrorEvent* event)
    {
        MANGO_UNREFERENCED(display);
        MANGO_UNREFERENCED(event);
        return 0;
    }

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

        visualAttribs.push_back(None);

        // select first config that satisfies the basic requirements

        int numFBConfigs = 0;
        GLXFBConfig* fbconfigs = glXChooseFBConfig(display, screen, visualAttribs.data(), &numFBConfigs);

        if (!numFBConfigs || !fbconfigs)
        {
            MANGO_EXCEPTION("[OpenGLContext] glXChooseFBConfig() failed.");
        }

        selected = fbconfigs[0];

        XFree(fbconfigs);

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

            tempAttribs.push_back(None);

            int numFBConfigs = 0;
            GLXFBConfig* fbconfigs = glXChooseFBConfig(display, screen, tempAttribs.data(), &numFBConfigs);

            if (numFBConfigs && fbconfigs)
            {
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

            XFree(fbconfigs);
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

            visualAttribs.push_back(None);

            int numFBConfigs = 0;
            GLXFBConfig* fbconfigs = glXChooseFBConfig(display, screen, visualAttribs.data(), &numFBConfigs);

            if (numFBConfigs && fbconfigs)
            {
                selected = fbconfigs[0];
            }

            XFree(fbconfigs);
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

                tempAttribs.push_back(None);

                int numFBConfigs = 0;
                GLXFBConfig* fbconfigs = glXChooseFBConfig(display, screen, tempAttribs.data(), &numFBConfigs);

                if (numFBConfigs && fbconfigs)
                {
                    printLine(Print::Info, "GLX_ARB_multisample : ENABLE");
                    selected = fbconfigs[0];
                }

                XFree(fbconfigs);
            }
        }
    }

    GLXContext GLXConfiguration::createContext(Display* display, GLXContext shared)
    {
        // NOTE: It is not necessary to create or make current to a context before calling glXGetProcAddressARB
        PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
            (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

        // Detect extension.
        bool isGLX_ARB_create_context = extensions.find("GLX_ARB_create_context") != extensions.end();

        // Install an X error handler so the application won't exit if GL 3.0
        // context allocation fails.
        //
        // Note this error handler is global.  All display connections in all threads
        // of a process use the same error handler, so be sure to guard against other
        // threads issuing X commands while this code is running.
        int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&contextErrorHandler);

        GLXContext context = 0;

        // Check for the GLX_ARB_create_context extension string and the function.
        if (isGLX_ARB_create_context && glXCreateContextAttribsARB)
        {
            std::vector<int> contextAttribs;

            //contextAttribs.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);
            //contextAttribs.push_back(GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);

            contextAttribs.push_back(GLX_CONTEXT_FLAGS_ARB);
            contextAttribs.push_back(GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB);

            if (shared)
            {
                contextAttribs.push_back(GLX_CONTEXT_SHARE_CONTEXT_ARB);
                contextAttribs.push_back(intptr_t(shared));
            }

            contextAttribs.push_back(None);

            context = glXCreateContextAttribsARB(display, selected, shared, True, contextAttribs.data());

            // Sync to ensure any errors generated are processed.
            XSync(display, False);

            if (context)
            {
                //printLine(Print::Info, "Created GL 3.0 context");
            }
            else
            {
                //printLine(Print::Error, "Failed to create GL 3.0 context ... using old-style GLX context");
                context = glXCreateContextAttribsARB(display, selected, 0, True, NULL);
            }
        }
        else
        {
            //printLine(Print::Warning, "glXCreateContextAttribsARB() not found ... using old-style GLX context");
            context = glXCreateNewContext(display, selected, GLX_RGBA_TYPE, 0, True);
        }

        // Sync to ensure any errors generated are processed.
        XSync(display, False);

        // Restore the original error handler
        XSetErrorHandler(oldHandler);

        return context;
    }

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_XLIB) || defined(MANGO_WINDOW_SYSTEM_XCB)

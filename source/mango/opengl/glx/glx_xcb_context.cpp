/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>

#if defined(MANGO_WINDOW_SYSTEM_XCB)

#define explicit explicit_
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_icccm.h>
#include <GL/glx.h>
#include "../../window/xcb/xcb_window.hpp"
#include "glx_context.hpp"
#undef explicit

namespace mango
{
    using namespace math;

    // -----------------------------------------------------------------------
    // OpenGLContextGLX
    // -----------------------------------------------------------------------

    static
    int contextErrorHandler(Display* display, XErrorEvent* event)
    {
        MANGO_UNREFERENCED(display);
        MANGO_UNREFERENCED(event);
        return 0;
    }

    struct OpenGLContextGLX : OpenGLContextHandle
    {
        GLXContext context { 0 };
        Display* display { nullptr };
        WindowContext* window;

        OpenGLContextGLX(OpenGLContext* theContext, int width, int height, u32 flags, const OpenGLContext::Config* pConfig, OpenGLContext* shared)
            : window(*theContext)
        {
            display = XOpenDisplay(NULL);
            int screen = DefaultScreen(display);

            GLXConfiguration glxConfiguration(display, screen, pConfig);

            // print selected fbconfig

            auto selected = glxConfiguration.selected;
            const auto& glxExtensions = glxConfiguration.extensions;

            auto getAttrib = [=, this] (auto fbconfig, int attribute) -> int
            {
                int value;
                glXGetFBConfigAttrib(display, fbconfig, attribute, &value);
                return value;
            };

            bool is_sRGB = getAttrib(selected, GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT);
            bool is_float = getAttrib(selected, GLX_RENDER_TYPE) & GLX_RGBA_FLOAT_BIT_ARB;

            printLine(Print::Info, "GLX FBConfig: [{} {} {} {}] D:{} S:{} sRGB:{} Float:{} [XCB]",
                getAttrib(selected, GLX_RED_SIZE),
                getAttrib(selected, GLX_GREEN_SIZE),
                getAttrib(selected, GLX_BLUE_SIZE),
                getAttrib(selected, GLX_ALPHA_SIZE),
                getAttrib(selected, GLX_DEPTH_SIZE),
                getAttrib(selected, GLX_STENCIL_SIZE),
                is_sRGB, is_float);

            // ----------------------------------------------------------------------

            // Create the XCB window with the GLX visual
            if (!window->init(width, height, flags, "OpenGL"))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] Failed to create X window.");
            }

            GLXContext shared_context = 0;
            if (shared)
            {
                shared_context = reinterpret_cast<OpenGLContextGLX*>(shared)->context;
            }

            context = glxConfiguration.createContext(display, shared_context);
            if (!context)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] OpenGL Context creation failed.");
            }

            // Verifying that context is a direct context
            if (glXIsDirect(display, context))
            {
                //printLine(Print::Info, "  glXIsDirect: ENABLE");
            }

            // MANGO TODO: configuration selection API
            // MANGO TODO: initialize GLX extensions using GLEXT headers
            glXMakeCurrent(display, *window, context);

#if 0
            PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC)glXGetProcAddress((const GLubyte*)"glGetStringi");

            if (glGetStringi == NULL)
            {
                // Fall-back to the pre-3.0 method for querying extensions.
                std::string extensionString = (const char*)glGetString(GL_EXTENSIONS);
                m_extensions = tokenizeString(extensionString, " ");
            }
            else
            {
                // Use the post-3.0 method for querying extensions
                GLint numExtensions = 0;
                glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

                for (int i = 0; i < numExtensions; ++i)
                {
                    std::string ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
                    m_extensions.push_back(ext);
                }
            }
#endif
        }

        ~OpenGLContextGLX()
        {
            shutdown();
        }

        void shutdown()
        {
            if (display)
            {
                glXMakeCurrent(display, 0, 0);
                if (context)
                {
                    glXDestroyContext(display, context);
                    context = 0;
                }
                XCloseDisplay(display);
                display = nullptr;
            }
        }

        void makeCurrent() override
        {
            if (display)
            {
                ::Window xwindow = static_cast<::Window>(*window);
                if (xwindow)
                {
                    glXMakeCurrent(display, xwindow, context);
                }
            }
        }

        void swapBuffers() override
        {
            if (display)
            {
                ::Window xwindow = static_cast<::Window>(*window);
                if (xwindow)
                {
                    glXSwapBuffers(display, xwindow);
                }
            }
        }

        void swapInterval(int interval) override
        {
            if (display)
            {
                ::Window xwindow = static_cast<::Window>(*window);
                if (xwindow)
                {
                    glXSwapIntervalEXT(display, xwindow, interval);
                }
            }
        }

        void toggleFullscreen() override
        {
            // Disable rendering while switching fullscreen mode
            glXMakeCurrent(display, 0, 0);
            window->busy = true;

            window->toggleFullscreen();

            // Enable rendering now that all the tricks are done
            window->busy = false;
            glXMakeCurrent(display, *window, context);
        }

        bool isFullscreen() const override
        {
            return window->fullscreen;
        }

        int32x2 getWindowSize() const override
        {
            return window->getWindowSize();
        }
    };

    OpenGLContextHandle* createOpenGLContextGLX(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared)
    {
        auto* context = new OpenGLContextGLX(parent, width, height, flags, configPtr, shared);
        return context;
    }

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_XCB)

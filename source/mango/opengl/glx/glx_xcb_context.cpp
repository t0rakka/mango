/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>

#if defined(MANGO_HAS_XCB_WINDOW)

#define explicit explicit_
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_icccm.h>
#define GLX_GLXEXT_PROTOTYPES
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

    // Unnamed namespace: this type shares its name with the Xlib GLX context, so it
    // must have internal linkage. Otherwise both definitions collide (ODR) and the
    // linker may bind the XCB factory to the Xlib constructor.
    namespace
    {

    struct OpenGLContextGLX : OpenGLContext
    {
        GLXContext context { 0 };
        Display* display { nullptr };
        XcbBackend* window;

        OpenGLContextGLX(OpenGLWindow* theContext, int width, int height, u32 flags, const OpenGLWindow::Config* pConfig, OpenGLWindow* shared)
            : window(static_cast<XcbBackend*>(theContext->backend()))
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
                MANGO_EXCEPTION("[OpenGLWindow] Failed to create X window.");
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
                MANGO_EXCEPTION("[OpenGLWindow] OpenGL Context creation failed.");
            }

            // Verifying that context is a direct context
            if (glXIsDirect(display, context))
            {
                //printLine(Print::Info, "  glXIsDirect: ENABLE");
            }

            glXMakeCurrent(display, *window, context);
            window->xlib_display = display;
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
            return window->isFullscreen();
        }

        int32x2 getWindowSize() const override
        {
            return window->getWindowSize();
        }
    };

    } // unnamed namespace

    OpenGLContext* createOpenGLContextGLX_Xcb(OpenGLWindow* parent, int width, int height, u32 flags, const OpenGLWindow::Config* configPtr, OpenGLWindow* shared)
    {
        auto* context = new OpenGLContextGLX(parent, width, height, flags, configPtr, shared);
        return context;
    }

} // namespace mango

#endif // defined(MANGO_HAS_XCB_WINDOW)

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>

#if defined(MANGO_HAS_XLIB_WINDOW)

#include "../../window/xlib/xlib_window.hpp"
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include "glx_context.hpp"

namespace mango
{
    using namespace math;

    // -----------------------------------------------------------------------
    // helpers
    // -----------------------------------------------------------------------

    static
    int contextErrorHandler(Display* display, XErrorEvent* event)
    {
        MANGO_UNREFERENCED(display);
        MANGO_UNREFERENCED(event);
        return 0;
    }

    // -----------------------------------------------------------------------
    // OpenGLContextGLX
    // -----------------------------------------------------------------------

    // Unnamed namespace: this type shares its name with the XCB GLX context, so it
    // must have internal linkage. Otherwise both definitions collide (ODR) and the
    // linker may bind the Xlib factory to the XCB constructor.
    namespace
    {

    struct OpenGLContextGLX : OpenGLContext
    {
        GLXContext context { 0 };
        bool fullscreen { false };

        XlibBackend* window;

        OpenGLContextGLX(OpenGLWindow* theContext, int width, int height, u32 flags, const OpenGLWindow::Config* pConfig, OpenGLWindow* shared)
            : window(static_cast<XlibBackend*>(theContext->backend()))
        {
            Display* display = window->x11Display();
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

            printLine(Print::Info, "GLX FBConfig: [{} {} {} {}] D:{} S:{} sRGB:{} Float:{} [XLIB]",
                getAttrib(selected, GLX_RED_SIZE),
                getAttrib(selected, GLX_GREEN_SIZE),
                getAttrib(selected, GLX_BLUE_SIZE),
                getAttrib(selected, GLX_ALPHA_SIZE),
                getAttrib(selected, GLX_DEPTH_SIZE),
                getAttrib(selected, GLX_STENCIL_SIZE),
                is_sRGB, is_float);

            // ----------------------------------------------------------------------

            XVisualInfo* vi = glXGetVisualFromFBConfig(display, selected);

            // create window
            if (!window->init(vi->screen, vi->depth, vi->visual, width, height, flags, "OpenGL"))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLWindow] init failed.");
            }

            XFree(vi);

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

            glXMakeCurrent(display, window->x11Window(), context);
        }

        ~OpenGLContextGLX()
        {
            shutdown();
        }

        void shutdown()
        {
            Display* display = window->x11Display();
            if (display)
            {
                glXMakeCurrent(display, 0, 0);

                if (context)
                {
                    glXDestroyContext(display, context);
                }
            }
        }

        void makeCurrent() override
        {
            glXMakeCurrent(window->x11Display(), window->x11Window(), context);
        }

        void swapBuffers() override
        {
            glXSwapBuffers(window->x11Display(), window->x11Window());
        }

        void swapInterval(int interval) override
        {
            glXSwapIntervalEXT(window->x11Display(), window->x11Window(), interval);
        }

        void toggleFullscreen() override
        {
            Display* display = window->x11Display();

            // Disable rendering while switching fullscreen mode
            glXMakeCurrent(display, 0, 0);
            window->busy = true;

            window->toggleFullscreen();

            // Enable rendering now that all the tricks are done
            window->busy = false;
            glXMakeCurrent(display, window->x11Window(), context);
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

    OpenGLContext* createOpenGLContextGLX_Xlib(OpenGLWindow* parent, int width, int height, u32 flags, const OpenGLWindow::Config* configPtr, OpenGLWindow* shared)
    {
        auto* context = new OpenGLContextGLX(parent, width, height, flags, configPtr, shared);
        return context;
    }

} // namespace mango

#endif // defined(MANGO_HAS_XLIB_WINDOW)

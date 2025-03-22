/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>
//#include "../../window/xlib/xlib_window.hpp"

#if defined(MANGO_WINDOW_SYSTEM_XCB)

#define explicit explicit_
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_icccm.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include "../../window/xcb/xcb_window.hpp"
#undef explicit

namespace mango
{
    using namespace math;

    // -----------------------------------------------------------------------
    // OpenGLContextXCB
    // -----------------------------------------------------------------------

    struct OpenGLContextXCB : OpenGLContextHandle
    {
        GLXContext context { 0 };
        bool fullscreen { false };
        WindowHandle* window;
        Display* display { nullptr };

        OpenGLContextXCB(OpenGLContext* theContext, int width, int height, u32 flags, const OpenGLContext::Config* pConfig, OpenGLContext* shared)
            : window(*theContext)
        {
            // resolve configuration
            OpenGLContext::Config config;
            if (pConfig)
            {
                config = *pConfig;
            }

            // Open X display
            display = XOpenDisplay(nullptr);
            if (!display)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] Failed to open X display.");
            }

            int screen = DefaultScreen(display);

            // Get GLX version
            int version_major, version_minor;
            if (!glXQueryVersion(display, &version_major, &version_minor))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] Failed to query GLX version.");
            }

            // Configure visual attributes
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

            visualAttribs.push_back(GLX_RED_SIZE);
            visualAttribs.push_back(config.red);

            visualAttribs.push_back(GLX_GREEN_SIZE);
            visualAttribs.push_back(config.green);

            visualAttribs.push_back(GLX_BLUE_SIZE);
            visualAttribs.push_back(config.blue);

            visualAttribs.push_back(GLX_ALPHA_SIZE);
            visualAttribs.push_back(config.alpha);

            if (config.samples > 1)
            {
                visualAttribs.push_back(GLX_SAMPLE_BUFFERS);
                visualAttribs.push_back(1);

                visualAttribs.push_back(GLX_SAMPLES);
                visualAttribs.push_back(config.samples);
            }

            visualAttribs.push_back(None);

            // Choose FBConfig
            int num_fb_configs = 0;
            GLXFBConfig* fbconfigs = glXChooseFBConfig(display, screen, visualAttribs.data(), &num_fb_configs);
            if (!fbconfigs || num_fb_configs == 0)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] Failed to get FBConfigs.");
            }

            // Select the first FBConfig
            GLXFBConfig fbconfig = fbconfigs[0];

            // Get visual info
            XVisualInfo* vi = glXGetVisualFromFBConfig(display, fbconfig);
            if (!vi)
            {
                XFree(fbconfigs);
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] Failed to get visual info.");
            }

            // Create context
            GLXContext shared_context = 0;
            if (shared)
            {
                shared_context = reinterpret_cast<OpenGLContextXCB*>(shared)->context;
            }

            // Get the GLX_ARB_create_context extension
            PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
                (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");

            if (glXCreateContextAttribsARB)
            {
                std::vector<int> contextAttribs;
                contextAttribs.push_back(GLX_CONTEXT_MAJOR_VERSION_ARB);
                contextAttribs.push_back(4);
                contextAttribs.push_back(GLX_CONTEXT_MINOR_VERSION_ARB);
                contextAttribs.push_back(6);
                contextAttribs.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);
                contextAttribs.push_back(GLX_CONTEXT_CORE_PROFILE_BIT_ARB);
                contextAttribs.push_back(GLX_CONTEXT_FLAGS_ARB);
                contextAttribs.push_back(GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB);
                contextAttribs.push_back(0); // End of attributes

                context = glXCreateContextAttribsARB(display, fbconfig, shared_context, True, contextAttribs.data());

                // If 4.6 is not available, try 3.3
                if (!context)
                {
                    contextAttribs[1] = 3; // major version
                    contextAttribs[3] = 3; // minor version
                    context = glXCreateContextAttribsARB(display, fbconfig, shared_context, True, contextAttribs.data());
                }
            }
            else
            {
                context = glXCreateNewContext(display, fbconfig, GLX_RGBA_TYPE, shared_context, True);
            }

            XFree(fbconfigs);

            if (!context)
            {
                XFree(vi);
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] Failed to create OpenGL context.");
            }

            // Create the XCB window with the GLX visual
            if (!window->createXWindow(vi->screen, vi->depth, vi->visualid, width, height, "OpenGL"))
            {
                XFree(vi);
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] Failed to create X window.");
            }

            XFree(vi);

            // Make context current
            ::Window xwindow = static_cast<::Window>(window->native.window);
            if (!glXMakeCurrent(display, xwindow, context))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] Failed to make context current.");
            }
        }

        ~OpenGLContextXCB()
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
                ::Window xwindow = static_cast<::Window>(window->native.window);
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
                ::Window xwindow = static_cast<::Window>(window->native.window);
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
                ::Window xwindow = static_cast<::Window>(window->native.window);
                if (xwindow)
                {
                    glXSwapIntervalEXT(display, xwindow, interval);
                }
            }
        }

        void toggleFullscreen() override
        {
            if (!display) return;

            // Disable rendering while switching fullscreen mode
            glXMakeCurrent(display, 0, 0);
            window->busy = true;

            ::Window xwindow = static_cast<::Window>(window->native.window);
            if (!xwindow) return;

            // Get the root window
            ::Window root = DefaultRootWindow(display);

            // Create client message event
            XEvent event = { 0 };
            event.type = ClientMessage;
            event.xclient.window = xwindow;
            event.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", False);
            event.xclient.format = 32;
            event.xclient.data.l[0] = 2; // NET_WM_STATE_TOGGLE
            event.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
            event.xclient.data.l[2] = 0;
            event.xclient.data.l[3] = 1;
            event.xclient.data.l[4] = 0;

            // Send the event to the root window
            XSendEvent(display, root, False,
                SubstructureRedirectMask | SubstructureNotifyMask, &event);

            // Map window to ensure it's visible
            XMapWindow(display, xwindow);
            XFlush(display);

            // Enable rendering now that all the tricks are done
            window->busy = false;
            glXMakeCurrent(display, xwindow, context);

            // Get window dimensions
            XWindowAttributes attributes;
            XGetWindowAttributes(display, xwindow, &attributes);

            // Send Expose event (generates Window::onDraw callback)
            XEvent expose = { 0 };
            expose.type = Expose;
            expose.xexpose.window = xwindow;
            expose.xexpose.x = 0;
            expose.xexpose.y = 0;
            expose.xexpose.width = attributes.width;
            expose.xexpose.height = attributes.height;
            expose.xexpose.count = 0;

            XSendEvent(display, xwindow, False, NoEventMask, &expose);
            XFlush(display);

            fullscreen = !fullscreen;
        }

        bool isFullscreen() const override
        {
            return fullscreen;
        }

        int32x2 getWindowSize() const override
        {
            if (!display) return int32x2(0, 0);

            ::Window xwindow = static_cast<::Window>(window->native.window);
            if (!xwindow) return int32x2(0, 0);

            XWindowAttributes attributes;
            XGetWindowAttributes(display, xwindow, &attributes);

            return int32x2(attributes.width, attributes.height);
        }
    };

    OpenGLContextHandle* createOpenGLContextGLX(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared)
    {
        auto* context = new OpenGLContextXCB(parent, width, height, flags, configPtr, shared);
        return context;
    }

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_XCB)

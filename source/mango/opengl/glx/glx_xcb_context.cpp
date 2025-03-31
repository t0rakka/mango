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
#include <X11/Xlib.h>
#include "../../window/xcb/xcb_window.hpp"
#include "glx_context.hpp"
#undef explicit

#ifndef GLX_CONTEXT_SHARE_CONTEXT_ARB
#define GLX_CONTEXT_SHARE_CONTEXT_ARB        0x2090
#endif

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
        bool fullscreen { false };

        WindowHandle* window;
        Display* display { nullptr };

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

            XVisualInfo* vi = glXGetVisualFromFBConfig(display, selected);

            // Create the XCB window with the GLX visual
            if (!window->createXWindow(vi->screen, vi->depth, vi->visualid, width, height, "OpenGL"))
            {
                XFree(vi);
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] Failed to create X window.");
            }

            XFree(vi);

            // NOTE: It is not necessary to create or make current to a context before calling glXGetProcAddressARB
            PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
                (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

            // Detect extension.
            bool isGLX_ARB_create_context = glxExtensions.find("GLX_ARB_create_context") != glxExtensions.end();

            // Install an X error handler so the application won't exit if GL 3.0
            // context allocation fails.
            //
            // Note this error handler is global.  All display connections in all threads
            // of a process use the same error handler, so be sure to guard against other
            // threads issuing X commands while this code is running.
            int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&contextErrorHandler);

            // Check for the GLX_ARB_create_context extension string and the function.
            if (isGLX_ARB_create_context && glXCreateContextAttribsARB)
            {
                std::vector<int> contextAttribs;

                //contextAttribs.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);
                //contextAttribs.push_back(GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);

                contextAttribs.push_back(GLX_CONTEXT_FLAGS_ARB);
                contextAttribs.push_back(GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB);

                GLXContext shared_context = 0;
                if (shared)
                {
                    shared_context = reinterpret_cast<OpenGLContextGLX*>(shared)->context;

                    contextAttribs.push_back(GLX_CONTEXT_SHARE_CONTEXT_ARB);
                    contextAttribs.push_back(intptr_t(shared_context));
                }

                contextAttribs.push_back(None);

                context = glXCreateContextAttribsARB(display, selected, shared_context, True, contextAttribs.data());

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
            glXMakeCurrent(display, window->window, context);

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
                ::Window xwindow = static_cast<::Window>(window->window);
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
                ::Window xwindow = static_cast<::Window>(window->window);
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
                ::Window xwindow = static_cast<::Window>(window->window);
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

            ::Window xwindow = static_cast<::Window>(window->window);
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

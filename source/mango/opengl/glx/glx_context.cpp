/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>
#include "../../window/xlib/xlib_handle.hpp"

#ifndef GLX_CONTEXT_SHARE_CONTEXT_ARB
#define GLX_CONTEXT_SHARE_CONTEXT_ARB        0x2090
#endif

#if defined(MANGO_WINDOW_SYSTEM_XCB)

#define explicit explicit_
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_icccm.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include "../../window/xcb/xcb_handle.hpp"
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

#if defined(MANGO_WINDOW_SYSTEM_XLIB)

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

        OpenGLContextGLX(OpenGLContext* theContext, int width, int height, u32 flags, const OpenGLContext::Config* pConfig, OpenGLContext* shared)
            : window(*theContext)
        {
            // resolve configuration
            OpenGLContext::Config config;
            if (pConfig)
            {
                // override defaults
                config = *pConfig;
            }

            int screen = DefaultScreen(window->native.display);
            Display* display = window->native.display;

            // Create GLX extension set
            std::set<std::string_view> glxExtensions;

            // Get GLX extensions
            const char* extensions = glXQueryExtensionsString(display, screen);
            if (extensions)
            {
                // parse extensions
                for (const char* s = extensions; *s; ++s)
                {
                    if (*s == ' ')
                    {
                        const std::ptrdiff_t length = s - extensions;
                        if (length > 0)
                        {
                            glxExtensions.emplace(extensions, length);
                        }

                        extensions = s + 1;
                    }
                }
            }

            // Get GLX version
            int version_major;
            int version_minor;
            if (!glXQueryVersion(display, &version_major, &version_minor))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] glXQueryVersion() failed.");
            }

            printLine(Print::Info, "GLX version: {}.{}", version_major, version_minor);

            if (version_major * 10 + version_minor < 13)
            {
                shutdown();
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

            auto chooseConfig = [=] (std::vector<int> attribs) -> std::vector<GLXFBConfig>
            {
                attribs.push_back(None);

                int numFBConfigs = 0;
                GLXFBConfig* fbconfigs = glXChooseFBConfig(display, screen, attribs.data(), &numFBConfigs);
                std::vector<GLXFBConfig> configs(fbconfigs + 0, fbconfigs + numFBConfigs);
                XFree(fbconfigs);

                return configs;
            };

            auto getAttrib = [=] (auto fbconfig, int attribute) -> int
            {
                int value;
                glXGetFBConfigAttrib(display, fbconfig, attribute, &value);
                return value;
            };

            // select first config that satisfies the basic requirements

            auto fbconfigs = chooseConfig(visualAttribs);
            if (fbconfigs.empty())
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] glXChooseFBConfig() failed.");
            }

            auto selected = fbconfigs[0];

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

            if (glxExtensions.find("GLX_ARB_fbconfig_float") != glxExtensions.end())
            {
                //printLine(Print::Info, "GLX_ARB_fbconfig_float : ENABLE");
            }

            if (glxExtensions.find("GLX_ARB_framebuffer_sRGB") != glxExtensions.end())
            {
                //printLine(Print::Info, "GLX_ARB_framebuffer_sRGB : ENABLE");
            }

            if (glxExtensions.find("GLX_ARB_multisample") != glxExtensions.end())
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

            // print selected fbconfig

            bool is_sRGB = getAttrib(selected, GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT);
            bool is_float = getAttrib(selected, GLX_RENDER_TYPE) & GLX_RGBA_FLOAT_BIT_ARB;

            printLine(Print::Info, "GLX FBConfig: [{} {} {} {}] D:{} S:{} sRGB:{} Float:{}",
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
            if (!window->createXWindow(vi->screen, vi->depth, vi->visual, width, height, "OpenGL"))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContext] createXWindow() failed.");
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

                // If 4.6 is not available, try 3.3
                if (!context)
                {
                    contextAttribs[1] = 3; // major version
                    contextAttribs[3] = 3; // minor version
                    context = glXCreateContextAttribsARB(display, selected, shared_context, True, contextAttribs.data());
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
            glXMakeCurrent(display, window->native.window, context);

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
            Display* display = window->native.display;
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
            glXMakeCurrent(window->native.display, window->native.window, context);
        }

        void swapBuffers() override
        {
            glXSwapBuffers(window->native.display, window->native.window);
        }

        void swapInterval(int interval) override
        {
            glXSwapIntervalEXT(window->native.display, window->native.window, interval);
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
            XWindowAttributes attributes;
            XGetWindowAttributes(window->native.display, window->native.window, &attributes);
            return int32x2(attributes.width, attributes.height);
        }
    };

    OpenGLContextHandle* createOpenGLContextGLX(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared)
    {
        auto* context = new OpenGLContextGLX(parent, width, height, flags, configPtr, shared);
        return context;
    }

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_XLIB)

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>

#if defined(MANGO_OPENGL_CONTEXT_EGL)

#if defined(MANGO_WINDOW_SYSTEM_XLIB)
#include "../../window/xlib/xlib_window.hpp"
#endif

#if defined(MANGO_WINDOW_SYSTEM_XCB)
#include "../../window/xcb/xcb_window.hpp"
#endif

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)
#include "../../window/wayland/wayland_window.hpp"
#endif

#include <EGL/egl.h>

namespace mango
{
    using namespace math;

    // -----------------------------------------------------------------------
    // OpenGLContextEGL
    // -----------------------------------------------------------------------

    struct OpenGLContextEGL : OpenGLContextHandle
    {
        EGLDisplay egl_display = EGL_NO_DISPLAY;
        EGLContext egl_context = EGL_NO_CONTEXT;
        EGLSurface egl_surface = EGL_NO_SURFACE;

        bool fullscreen { false };

        WindowHandle* window;

        OpenGLContextEGL(OpenGLContext* theContext, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* theShared)
            : window(*theContext)
        {

            //egl_display = eglGetDisplay((EGLNativeDisplayType)window->native.display);
            egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            if (egl_display == EGL_NO_DISPLAY)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglGetDisplay() failed.");
            }

            if (!eglInitialize(egl_display, NULL, NULL))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglInitialize() failed.");
            }

            // override defaults
            OpenGLContext::Config config;
            if (configPtr)
            {
                // Override defaults
                config = *configPtr;
            }

            // Configure attributes

            std::vector<EGLint> configAttribs;

            //config.red
            //config.green
            //config.blue
            //config.alpha

            /*

            #ifndef EGL_EXT_pixel_format_float
                #define EGL_COLOR_COMPONENT_TYPE_EXT      0x3339
                #define EGL_COLOR_COMPONENT_TYPE_FIXED_EXT 0x333A
                #define EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT 0x333B
            #endif

            #ifndef EGL_KHR_gl_colorspace
                #define EGL_GL_COLORSPACE_KHR             0x309D
                #define EGL_GL_COLORSPACE_SRGB_KHR        0x3089
                #define EGL_GL_COLORSPACE_LINEAR_KHR      0x308A
            #endif

            #ifndef EGL_EXT_gl_colorspace_bt2020_linear
                #define EGL_GL_COLORSPACE_BT2020_LINEAR_EXT 0x333F
            #endif

            #ifndef EGL_EXT_gl_colorspace_bt2020_pq
                #define EGL_GL_COLORSPACE_BT2020_PQ_EXT   0x3340
            #endif

            #ifndef EGL_EXT_gl_colorspace_display_p3
                #define EGL_GL_COLORSPACE_DISPLAY_P3_EXT  0x3363
            #endif

            #ifndef EGL_EXT_gl_colorspace_display_p3_linear
                #define EGL_GL_COLORSPACE_DISPLAY_P3_LINEAR_EXT 0x3362
            #endif

            #ifndef EGL_EXT_gl_colorspace_display_p3_passthrough
                #define EGL_GL_COLORSPACE_DISPLAY_P3_PASSTHROUGH_EXT 0x3490
            #endif

            #ifndef EGL_EXT_gl_colorspace_scrgb
                #define EGL_GL_COLORSPACE_SCRGB_EXT       0x3351
            #endif

            #ifndef EGL_EXT_gl_colorspace_scrgb_linear
                #define EGL_GL_COLORSPACE_SCRGB_LINEAR_EXT 0x3350
            #endif

            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_RED_SIZE, 16,
            EGL_GREEN_SIZE, 16,
            EGL_BLUE_SIZE, 16,
            EGL_ALPHA_SIZE, 16,
            EGL_COLOR_COMPONENT_TYPE_EXT, EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT,
            EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_DISPLAY_P3_EXT,

            */

            configAttribs.push_back(EGL_BUFFER_SIZE);
            configAttribs.push_back(8);

            configAttribs.push_back(EGL_DEPTH_SIZE);
            configAttribs.push_back(config.depth);

            configAttribs.push_back(EGL_STENCIL_SIZE);
            configAttribs.push_back(config.stencil);

            configAttribs.push_back(EGL_SURFACE_TYPE);
            configAttribs.push_back(EGL_WINDOW_BIT);

            configAttribs.push_back(EGL_RENDERABLE_TYPE);
            configAttribs.push_back(EGL_OPENGL_BIT);
            //configAttribs.push_back(EGL_OPENGL_ES2_BIT);

            if (config.samples > 1)
            {
                // MANGO TODO
            }

            configAttribs.push_back(EGL_NONE);

            EGLint numConfig;
            EGLConfig eglConfig[1];

            if (!eglChooseConfig(egl_display, configAttribs.data(), eglConfig, 1, &numConfig))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglChooseConfig() failed.");
            }

            OpenGLContextEGL* shared = reinterpret_cast<OpenGLContextEGL*>(theShared);
            EGLContext shared_context = shared ? shared->egl_context : EGL_NO_CONTEXT;

            const EGLint contextAttribs[] =
            {
                //EGL_CONTEXT_MAJOR_VERSION, 4,
                //EGL_CONTEXT_MINOR_VERSION, 6,
                //EGL_CONTEXT_OPENGL_PROFILE_MASK,  EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                EGL_NONE
            };

            eglBindAPI(EGL_OPENGL_API);

            egl_context = eglCreateContext(egl_display, eglConfig[0], shared_context, contextAttribs);
            if (egl_context == EGL_NO_CONTEXT)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglCreateContext() failed.");
            }

            printLine("[EGL] eglCreateContext() : OK");

#if defined(MANGO_WINDOW_SYSTEM_XLIB)

            if (!window->createXWindow(0, 0, nullptr, width, height, "OpenGL"))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] createWindow() failed.");
            }

            egl_surface = eglCreateWindowSurface(egl_display, eglConfig[0], window->native.window, NULL);
            if (egl_surface == EGL_NO_SURFACE)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglCreateWindowSurface() failed.");
            }

#endif

#if defined(MANGO_WINDOW_SYSTEM_XCB)

            // Get the default screen and its depth
            const xcb_setup_t* setup = xcb_get_setup(window->connection);
            xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
            xcb_screen_t* screen = iter.data;
            if (!screen)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] Failed to get default screen.");
            }

            // Get visual info from EGL config
            EGLint visual_id;
            if (!eglGetConfigAttrib(egl_display, eglConfig[0], EGL_NATIVE_VISUAL_ID, &visual_id))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] Failed to get visual ID from EGL config.");
            }

            if (!window->createXWindow(screen->root_depth, screen->root_depth, visual_id, width, height, "OpenGL"))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] createWindow() failed.");
            }

            egl_surface = eglCreateWindowSurface(egl_display, eglConfig[0], window->window, NULL);
            if (egl_surface == EGL_NO_SURFACE)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglCreateWindowSurface() failed.");
            }

#endif

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

            // TODO

#endif

            if (!eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglMakeCurrent() failed.");
            }
        }

        ~OpenGLContextEGL()
        {
            shutdown();
        }

        void shutdown()
        {
            if (egl_display != EGL_NO_DISPLAY)
            {
                if (egl_surface != EGL_NO_SURFACE)
                {
                    eglDestroySurface(egl_display, egl_surface);
                }

                if (egl_context != EGL_NO_CONTEXT)
                {
                    eglDestroyContext(egl_display, egl_context);
                }

                eglTerminate(egl_display);
            }
        }

        void makeCurrent() override
        {
            eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
        }

        void swapBuffers() override
        {
            eglSwapBuffers(egl_display, egl_surface);
        }

        void swapInterval(int interval) override
        {
            eglSwapInterval(egl_display, interval);
        }

#if defined(MANGO_WINDOW_SYSTEM_XLIB)

        void toggleFullscreen() override
        {
            // Disable rendering while switching fullscreen mode
            eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            window->busy = true;

            XEvent xevent;
            std::memset(&xevent, 0, sizeof(xevent));

            xevent.type = ClientMessage;
            xevent.xclient.window = window->native.window;
            xevent.xclient.message_type = window->atom_state;
            xevent.xclient.format = 32;
            xevent.xclient.data.l[0] = 2; // NET_WM_STATE_TOGGLE
            xevent.xclient.data.l[1] = window->atom_fullscreen;
            xevent.xclient.data.l[2] = 0; // no second property to toggle
            xevent.xclient.data.l[3] = 1; // source indication: application
            xevent.xclient.data.l[4] = 0; // unused

            XMapWindow(window->native.display, window->native.window);

            // send the event to the root window
            XSendEvent(window->native.display, DefaultRootWindow(window->native.display), False,
                SubstructureRedirectMask | SubstructureNotifyMask, &xevent);

            XFlush(window->native.display);

            // Enable rendering now that all the tricks are done
            window->busy = false;
            eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

            fullscreen = !fullscreen;
        }

#endif

#if defined(MANGO_WINDOW_SYSTEM_XCB)

        void toggleFullscreen() override
        {
            xcb_connection_t* connection = window->connection;
            xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
            xcb_window_t root_window = screen->root;

            eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            window->busy = true;

            xcb_client_message_event_t xevent = {0};

            xevent.response_type = XCB_CLIENT_MESSAGE;
            xevent.window = window->window;
            xevent.type = window->atom_state;
            xevent.format = 32;
            xevent.data.data32[0] = 2;  // NET_WM_STATE_TOGGLE
            xevent.data.data32[1] = window->atom_fullscreen;
            xevent.data.data32[2] = 0;  // No second property to toggle
            xevent.data.data32[3] = 1;  // Source indication: application
            xevent.data.data32[4] = 0;  // Unused field

            // Send the event to the root window
            xcb_send_event(connection, 0, screen->root,
                           XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                           reinterpret_cast<const char*>(&xevent));
            xcb_flush(connection);

            window->busy = false;
            eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

            fullscreen = !fullscreen;
        }

#endif

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

    void toggleFullscreen() override
    {
        // TODO
        fullscreen = !fullscreen;
    }

#endif

        bool isFullscreen() const override
        {
            return fullscreen;
        }

        int32x2 getWindowSize() const override
        {
            return window->getWindowSize();
        }
    };

    OpenGLContextHandle* createOpenGLContextEGL(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared)
    {
        auto* context = new OpenGLContextEGL(parent, width, height, flags, configPtr, shared);
        return context;
    }

} // namespace mango

#endif // defined(MANGO_OPENGL_CONTEXT_EGL)

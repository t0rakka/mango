/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>

#if defined(MANGO_OPENGL_CONTEXT_EGL)

#include "../../window/xlib/xlib_handle.hpp"
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

            //egl_display = eglGetDisplay((EGLNativeDisplayType)window->x11_display);
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
                // TODO
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

            if (!window->createWindow(0, 0, nullptr, width, height, "OpenGL"))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] createWindow() failed.");
            }

            egl_surface = eglCreateWindowSurface(egl_display, eglConfig[0], window->x11_window, NULL);
            if (egl_surface == EGL_NO_SURFACE)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglCreateWindowSurface() failed.");
            }

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

        void toggleFullscreen() override
        {
            // Disable rendering while switching fullscreen mode
            window->busy = true;
            eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            XEvent xevent;
            std::memset(&xevent, 0, sizeof(xevent));

            xevent.type = ClientMessage;
            xevent.xclient.window = window->x11_window;
            xevent.xclient.message_type = window->atom_state;
            xevent.xclient.format = 32;
            xevent.xclient.data.l[0] = 2; // NET_WM_STATE_TOGGLE
            xevent.xclient.data.l[1] = window->atom_fullscreen;
            xevent.xclient.data.l[2] = 0; // no second property to toggle
            xevent.xclient.data.l[3] = 1; // source indication: application
            xevent.xclient.data.l[4] = 0; // unused

            XMapWindow(window->x11_display, window->x11_window);

            // send the event to the root window
            if (!XSendEvent(window->x11_display, DefaultRootWindow(window->x11_display), False,
                SubstructureRedirectMask | SubstructureNotifyMask, &xevent))
            {
                // TODO: failed
            }

            XFlush(window->x11_display);

            // Enable rendering now that all the tricks are done
            window->busy = false;
            eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

            fullscreen = !fullscreen;
        }

        bool isFullscreen() const override
        {
            return fullscreen;
        }

        int32x2 getWindowSize() const override
        {
            XWindowAttributes attributes;
            XGetWindowAttributes(window->x11_display, window->x11_window, &attributes);
            return int32x2(attributes.width, attributes.height);
        }
    };

    OpenGLContextHandle* createOpenGLContextEGL(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared)
    {
        auto* context = new OpenGLContextEGL(parent, width, height, flags, configPtr, shared);
        return context;
    }

} // namespace mango

#endif // defined(MANGO_OPENGL_CONTEXT_EGL)

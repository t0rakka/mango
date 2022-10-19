/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>
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

            //egl_display = eglGetDisplay((EGLNativeDisplayType)window->display);
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

            /*
            // Configure attributes

            std::vector<int> visualAttribs;

            visualAttribs.push_back(config.red);
            visualAttribs.push_back(config.green);
            visualAttribs.push_back(config.blue);
            visualAttribs.push_back(config.alpha);
            visualAttribs.push_back(config.depth);
            visualAttribs.push_back(config.stencil);

            if (config.samples > 1)
            {
            }
            */

            const EGLint configAttribs [] =
            {
                EGL_BUFFER_SIZE,     8,
                EGL_DEPTH_SIZE,      24,
                EGL_STENCIL_SIZE,    8,
                EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_NONE
            };

            EGLint numConfig;
            EGLConfig eglConfig[1];

            if (!eglChooseConfig(egl_display, configAttribs, eglConfig, 1, &numConfig))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglChooseConfig() failed.");
            }

            OpenGLContextEGL* shared = reinterpret_cast<OpenGLContextEGL*>(theShared);
            EGLContext shared_context = shared ? shared->egl_context : EGL_NO_CONTEXT;

            const EGLint contextAttribs[] =
            {
                EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL_NONE
            };

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

            egl_surface = eglCreateWindowSurface(egl_display, eglConfig[0], window->window, NULL);
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
            eglSwapInterval(interval);
        }

        void toggleFullscreen() override
        {
            // Disable rendering while switching fullscreen mode
            window->busy = true;
            eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            XEvent xevent;
            std::memset(&xevent, 0, sizeof(xevent));

            xevent.type = ClientMessage;
            xevent.xclient.window = window->window;
            xevent.xclient.message_type = window->atom_state;
            xevent.xclient.format = 32;
            xevent.xclient.data.l[0] = 2; // NET_WM_STATE_TOGGLE
            xevent.xclient.data.l[1] = window->atom_fullscreen;
            xevent.xclient.data.l[2] = 0; // no second property to toggle
            xevent.xclient.data.l[3] = 1; // source indication: application
            xevent.xclient.data.l[4] = 0; // unused

            XMapWindow(window->display, window->window);

            // send the event to the root window
            if (!XSendEvent(window->display, DefaultRootWindow(window->display), False,
                SubstructureRedirectMask | SubstructureNotifyMask, &xevent))
            {
                // TODO: failed
            }

            XFlush(window->display);

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
            XGetWindowAttributes(window->display, window->window, &attributes);
            return int32x2(attributes.width, attributes.height);
        }
    };

    // -----------------------------------------------------------------------
    // OpenGLContext
    // -----------------------------------------------------------------------

    void OpenGLContext::initContext(int width, int height, u32 flags, const Config* configPtr, OpenGLContext* shared)
    {
        m_context = new OpenGLContextEGL(this, width, height, flags, configPtr, shared);
    }

} // namespace mango

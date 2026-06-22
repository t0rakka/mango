/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>

#include <algorithm>
#include <cstring>
#include <vector>

#if defined(MANGO_OPENGL_CONTEXT_EGL)

#if defined(MANGO_WINDOW_SYSTEM_XLIB)
#include "../../window/xlib/xlib_window.hpp"
#endif

#if defined(MANGO_WINDOW_SYSTEM_XCB)
#include "../../window/xcb/xcb_window.hpp"
#endif

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)
#include "../../window/wayland/wayland_window.hpp"
#include <wayland-egl.h>
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace
{

    struct EGLConfigChoice
    {
        EGLConfig config = nullptr;
        bool srgb = false;
    };

    bool eglHasExtension(EGLDisplay display, const char* name)
    {
        const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
        return extensions && std::strstr(extensions, name) != nullptr;
    }

    void eglClearErrors()
    {
        while (eglGetError() != EGL_SUCCESS)
        {
        }
    }

    std::vector<EGLint> buildConfigAttribs(const mango::OpenGLContext::Config& config)
    {
        std::vector<EGLint> attribs;

        attribs.push_back(EGL_RED_SIZE);
        attribs.push_back(int(config.red));
        attribs.push_back(EGL_GREEN_SIZE);
        attribs.push_back(int(config.green));
        attribs.push_back(EGL_BLUE_SIZE);
        attribs.push_back(int(config.blue));
        attribs.push_back(EGL_ALPHA_SIZE);
        attribs.push_back(int(config.alpha));

        attribs.push_back(EGL_DEPTH_SIZE);
        attribs.push_back(config.depth);

        attribs.push_back(EGL_STENCIL_SIZE);
        attribs.push_back(config.stencil);

        attribs.push_back(EGL_SURFACE_TYPE);
        attribs.push_back(EGL_WINDOW_BIT);

        attribs.push_back(EGL_RENDERABLE_TYPE);
        attribs.push_back(EGL_OPENGL_BIT);

        if (config.samples > 1)
        {
            // MANGO TODO
        }

        attribs.push_back(EGL_NONE);
        return attribs;
    }

    EGLConfigChoice chooseEGLConfig(EGLDisplay display, const std::vector<EGLint>& attribs,
                                    EGLNativeWindowType native_window)
    {
        EGLConfigChoice choice;

        EGLint num_configs = 0;
        if (!eglChooseConfig(display, attribs.data(), nullptr, 0, &num_configs) || num_configs <= 0)
        {
            return choice;
        }

        std::vector<EGLConfig> configs{ size_t(num_configs) };
        if (!eglChooseConfig(display, attribs.data(), configs.data(), num_configs, &num_configs) || num_configs <= 0)
        {
            return choice;
        }
        configs.resize(size_t(num_configs));

        const EGLint srgb_attribs[] =
        {
            EGL_GL_COLORSPACE_KHR, EGL_GL_COLORSPACE_SRGB_KHR,
            EGL_NONE,
        };

        if (native_window && eglHasExtension(display, "EGL_KHR_gl_colorspace"))
        {
            for (EGLConfig config : configs)
            {
                eglClearErrors();
                EGLSurface test = eglCreateWindowSurface(display, config, native_window, srgb_attribs);
                if (test != EGL_NO_SURFACE)
                {
                    eglDestroySurface(display, test);
                    choice.config = config;
                    choice.srgb = true;
                    return choice;
                }
            }

            mango::printLine(mango::Print::Warning, "[EGL] no EGLConfig supports sRGB colorspace (0x{:x}), using linear",
                eglGetError());
        }

        choice.config = configs[0];
        return choice;
    }

    EGLSurface createEGLWindowSurface(EGLDisplay display, EGLConfig config,
                                      EGLNativeWindowType native_window, bool srgb)
    {
        if (srgb)
        {
            const EGLint attribs[] =
            {
                EGL_GL_COLORSPACE_KHR, EGL_GL_COLORSPACE_SRGB_KHR,
                EGL_NONE,
            };

            eglClearErrors();
            EGLSurface surface = eglCreateWindowSurface(display, config, native_window, attribs);
            if (surface != EGL_NO_SURFACE)
            {
                mango::printLine("[EGL] EGL_KHR_gl_colorspace : sRGB");
                return surface;
            }

            mango::printLine(mango::Print::Warning, "[EGL] sRGB surface creation failed (0x{:x}), using linear colorspace",
                eglGetError());
        }

        eglClearErrors();
        return eglCreateWindowSurface(display, config, native_window, nullptr);
    }

} // namespace

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

        WindowContext* window;

        OpenGLContextEGL(OpenGLContext* theContext, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* theShared)
            : window(*theContext)
        {

            //egl_display = eglGetDisplay((EGLNativeDisplayType)window->display);
#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)
            egl_display = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(window->display));
#else
            egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#endif
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

            const std::vector<EGLint> configAttribs = buildConfigAttribs(config);

            eglBindAPI(EGL_OPENGL_API);

            OpenGLContextEGL* shared = reinterpret_cast<OpenGLContextEGL*>(theShared);
            EGLContext shared_context = shared ? shared->egl_context : EGL_NO_CONTEXT;

            const EGLint contextAttribs[] =
            {
                //EGL_CONTEXT_MAJOR_VERSION, 4,
                //EGL_CONTEXT_MINOR_VERSION, 6,
                //EGL_CONTEXT_OPENGL_PROFILE_MASK,  EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                EGL_NONE
            };

            EGLConfig eglConfig = nullptr;
            bool srgb_surface = false;

#if defined(MANGO_WINDOW_SYSTEM_XLIB)

            if (!window->init(0, 0, nullptr, width, height, flags, "OpenGL|ES"))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] createWindow() failed.");
            }

            {
                EGLConfigChoice choice = chooseEGLConfig(egl_display, configAttribs, *window);
                if (!choice.config)
                {
                    shutdown();
                    MANGO_EXCEPTION("[OpenGLContextEGL] eglChooseConfig() failed.");
                }
                eglConfig = choice.config;
                srgb_surface = choice.srgb;
            }

#endif

#if defined(MANGO_WINDOW_SYSTEM_XCB)

            if (!window->init(width, height, flags, "OpenGL|ES"))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] createWindow() failed.");
            }

            {
                EGLConfigChoice choice = chooseEGLConfig(egl_display, configAttribs, *window);
                if (!choice.config)
                {
                    shutdown();
                    MANGO_EXCEPTION("[OpenGLContextEGL] eglChooseConfig() failed.");
                }
                eglConfig = choice.config;
                srgb_surface = choice.srgb;
            }

#endif

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

            if (!window->surface)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] Wayland surface is not ready.");
            }

            const int egl_width = std::max(1, window->size[0] > 0 ? window->size[0] : width);
            const int egl_height = std::max(1, window->size[1] > 0 ? window->size[1] : height);

            window->egl_window = wl_egl_window_create(window->surface, egl_width, egl_height);
            if (!window->egl_window)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] wl_egl_window_create() failed.");
            }

            {
                EGLConfigChoice choice = chooseEGLConfig(egl_display, configAttribs,
                    static_cast<EGLNativeWindowType>(window->egl_window));
                if (!choice.config)
                {
                    shutdown();
                    MANGO_EXCEPTION("[OpenGLContextEGL] eglChooseConfig() failed.");
                }
                eglConfig = choice.config;
                srgb_surface = choice.srgb;
            }

#endif

            egl_context = eglCreateContext(egl_display, eglConfig, shared_context, contextAttribs);
            if (egl_context == EGL_NO_CONTEXT)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglCreateContext() failed.");
            }

            printLine("[EGL] eglCreateContext() : OK");

#if defined(MANGO_WINDOW_SYSTEM_XLIB)

            egl_surface = createEGLWindowSurface(egl_display, eglConfig, *window, srgb_surface);
            if (egl_surface == EGL_NO_SURFACE)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglCreateWindowSurface() failed.");
            }

#endif

#if defined(MANGO_WINDOW_SYSTEM_XCB)

            egl_surface = createEGLWindowSurface(egl_display, eglConfig, *window, srgb_surface);
            if (egl_surface == EGL_NO_SURFACE)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglCreateWindowSurface() failed.");
            }

#endif

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

            egl_surface = createEGLWindowSurface(egl_display, eglConfig,
                static_cast<EGLNativeWindowType>(window->egl_window), srgb_surface);
            if (egl_surface == EGL_NO_SURFACE)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglCreateWindowSurface() failed.");
            }

            wl_display_roundtrip(window->display);
            window->syncEGLWindow();

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
#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)
            window->syncEGLWindow();
#endif
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

            window->toggleFullscreen();

            // Enable rendering now that all the tricks are done
            window->busy = false;
            eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
        }

#endif

#if defined(MANGO_WINDOW_SYSTEM_XCB)

    void toggleFullscreen() override
    {
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        window->busy = true;

        window->toggleFullscreen();

        window->busy = false;
        eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    }

#endif

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

    void toggleFullscreen() override
    {
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        window->busy = true;

        window->toggleFullscreen();

        window->busy = false;
        eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    }

#endif

        bool isFullscreen() const override
        {
            return window->isFullscreen();
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

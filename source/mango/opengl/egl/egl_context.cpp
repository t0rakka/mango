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

#include "../../window/window_backend.hpp"

// This translation unit serves every Linux backend (xlib / xcb / wayland), so it
// must not pull in any backend-specific native window header. Force EGL's native
// handle types to be opaque (void*/integer) instead of the X11 or Wayland types,
// and exchange the real handles with the backend through WindowBackend's
// eglNativeDisplay()/eglNativeWindow() hooks.
#define MESA_EGL_NO_X11_HEADERS
#define EGL_NO_X11
#define USE_OZONE

#include <cstdint>
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

        WindowBackend* window;

        OpenGLContextEGL(OpenGLContext* theContext, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* theShared)
            : window(theContext->backend())
        {
            // Native display: nullptr selects EGL_DEFAULT_DISPLAY (X11); Wayland
            // hands back its wl_display.
            void* native_display = window->eglNativeDisplay();
            egl_display = eglGetDisplay(native_display
                ? reinterpret_cast<EGLNativeDisplayType>(native_display)
                : EGL_DEFAULT_DISPLAY);
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

            // Ask the backend to create the OS window (X11) or wl_egl_window
            // (Wayland) and return the EGLNativeWindowType packed into a void*.
            void* native = window->eglNativeWindow(width, height, flags);
            if (!native)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] native window creation failed.");
            }

            EGLNativeWindowType native_window = (EGLNativeWindowType)(std::uintptr_t)native;

            {
                EGLConfigChoice choice = chooseEGLConfig(egl_display, configAttribs, native_window);
                if (!choice.config)
                {
                    shutdown();
                    MANGO_EXCEPTION("[OpenGLContextEGL] eglChooseConfig() failed.");
                }
                eglConfig = choice.config;
                srgb_surface = choice.srgb;
            }

            egl_context = eglCreateContext(egl_display, eglConfig, shared_context, contextAttribs);
            if (egl_context == EGL_NO_CONTEXT)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglCreateContext() failed.");
            }

            printLine("[EGL] eglCreateContext() : OK");

            egl_surface = createEGLWindowSurface(egl_display, eglConfig, native_window, srgb_surface);
            if (egl_surface == EGL_NO_SURFACE)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglCreateWindowSurface() failed.");
            }

            // Wayland needs a roundtrip + egl-window resync once the surface exists.
            window->eglPresent();

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
            window->eglPresent();
            eglSwapBuffers(egl_display, egl_surface);
        }

        void swapInterval(int interval) override
        {
            eglSwapInterval(egl_display, interval);
        }

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

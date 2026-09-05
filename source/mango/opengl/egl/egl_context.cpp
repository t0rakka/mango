/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>
#include <mango/window/window.hpp>

#include <algorithm>
#include <cstring>
#include <vector>

#if defined(MANGO_OPENGL_CONTEXT_EGL)

#include "../../window/window_backend.hpp"
#include "egl_surface.hpp"
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

    std::vector<EGLint> buildConfigAttribs(const mango::OpenGLWindow::Config& config)
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
            attribs.push_back(EGL_SAMPLE_BUFFERS);
            attribs.push_back(1);
            attribs.push_back(EGL_SAMPLES);
            attribs.push_back(int(config.samples));
        }

        attribs.push_back(EGL_NONE);
        return attribs;
    }

    EGLConfigChoice chooseEGLConfig(EGLDisplay display, const std::vector<EGLint>& attribs)
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

        // Do not probe sRGB by creating a temporary window surface: on Wayland/Mesa
        // destroying that surface leaves wl_egl_window unusable for the real create.
        // createEGLWindowSurface() tries sRGB and falls back to linear.
        choice.config = configs[0];
        choice.srgb = eglHasExtension(display, "EGL_KHR_gl_colorspace");
        return choice;
    }

    EGLConfigChoice chooseEGLConfigWithRetry(EGLDisplay display, mango::OpenGLWindow::Config& config,
                                             bool present_opaque)
    {
        const auto saved_alpha = config.alpha;
        if (present_opaque)
        {
            // Wayland compositors blend using buffer alpha; prefer configs without
            // an alpha channel so partial alpha cannot punch through to the desktop.
            config.alpha = 0;
        }

        EGLConfigChoice choice = chooseEGLConfig(display, buildConfigAttribs(config));

        if (!choice.config && present_opaque && saved_alpha > 0)
        {
            config.alpha = saved_alpha;
            choice = chooseEGLConfig(display, buildConfigAttribs(config));
        }

        if (!choice.config && config.samples > 1)
        {
            mango::printLine(mango::Print::Warning, "[EGL] {}x multisample not available, falling back to 1x",
                config.samples);
            config.samples = 1;
            choice = chooseEGLConfig(display, buildConfigAttribs(config));
        }
        else if (choice.config && config.samples > 1)
        {
            EGLint samples = 0;
            if (eglGetConfigAttrib(display, choice.config, EGL_SAMPLES, &samples) && samples > 0)
            {
                mango::printLine(mango::Print::Info, "[EGL] multisample : {}x", samples);
            }
        }

        return choice;
    }

    EGLSurface createEGLWindowSurface(EGLDisplay display, EGLConfig config,
                                      mango::opengl::egl::NativeWindowBinding& binding,
                                      bool srgb, bool present_opaque, EGLint* out_error)
    {
        // Use the libEGL-linked eglCreateWindowSurface entry point only.
        // Pointers from eglGetProcAddress("eglCreatePlatformWindowSurface*") can
        // bypass glvnd display dispatch and return EGL_BAD_PARAMETER (0x300c)
        // when multiple EGL vendors are installed (common on Linux).
        auto tryCreate = [&](const EGLint* attribs) -> EGLSurface
        {
            if (!binding.native_window)
            {
                if (out_error)
                {
                    *out_error = EGL_BAD_NATIVE_WINDOW;
                }
                return EGL_NO_SURFACE;
            }

            eglClearErrors();

            EGLSurface surface = eglCreateWindowSurface(
                display, config,
                (EGLNativeWindowType)(std::uintptr_t)binding.native_window,
                attribs);

            if (surface == EGL_NO_SURFACE && out_error)
            {
                *out_error = eglGetError();
            }
            return surface;
        };

        auto afterFailure = [&]()
        {
            // Wayland/Mesa: a failed create can leave wl_egl_window unusable.
            if (binding.recreate)
            {
                binding.recreate(binding);
            }
        };

        const bool use_present_opaque = present_opaque && eglHasExtension(display, "EGL_EXT_present_opaque");

        if (srgb && use_present_opaque)
        {
            const EGLint attribs[] =
            {
                EGL_GL_COLORSPACE_KHR, EGL_GL_COLORSPACE_SRGB_KHR,
                EGL_PRESENT_OPAQUE_EXT, EGL_TRUE,
                EGL_NONE,
            };

            EGLSurface surface = tryCreate(attribs);
            if (surface != EGL_NO_SURFACE)
            {
                mango::printLine("[EGL] EGL_KHR_gl_colorspace : sRGB + EGL_EXT_present_opaque");
                return surface;
            }

            mango::printLine(mango::Print::Warning, "[EGL] sRGB+opaque surface creation failed (0x{:x}), retrying",
                out_error ? *out_error : eglGetError());
            afterFailure();
        }

        if (use_present_opaque)
        {
            const EGLint attribs[] =
            {
                EGL_PRESENT_OPAQUE_EXT, EGL_TRUE,
                EGL_NONE,
            };

            EGLSurface surface = tryCreate(attribs);
            if (surface != EGL_NO_SURFACE)
            {
                mango::printLine("[EGL] EGL_EXT_present_opaque");
                return surface;
            }

            mango::printLine(mango::Print::Warning, "[EGL] opaque surface creation failed (0x{:x}), retrying",
                out_error ? *out_error : eglGetError());
            afterFailure();
        }

        if (srgb)
        {
            const EGLint attribs[] =
            {
                EGL_GL_COLORSPACE_KHR, EGL_GL_COLORSPACE_SRGB_KHR,
                EGL_NONE,
            };

            EGLSurface surface = tryCreate(attribs);
            if (surface != EGL_NO_SURFACE)
            {
                mango::printLine("[EGL] EGL_KHR_gl_colorspace : sRGB");
                return surface;
            }

            mango::printLine(mango::Print::Warning, "[EGL] sRGB surface creation failed (0x{:x}), using linear",
                out_error ? *out_error : eglGetError());
            afterFailure();
        }

        return tryCreate(nullptr);
    }

    EGLDisplay openEGLDisplay(void* native_display)
    {
        // Prefer an explicit platform display via the libEGL-linked 1.5 entry
        // point (glvnd-safe). eglGetDisplay(wl_display) can bind the wrong
        // platform on dual X11/Wayland systems.
        // Do not use eglGetProcAddress for these — same glvnd dispatch pitfall.
#if defined(EGL_VERSION_1_5)
#if defined(MANGO_HAS_WAYLAND_WINDOW)
        if (mango::Window::getWindowSystem() == mango::WindowSystem::Wayland && native_display)
        {
            EGLDisplay display = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, native_display, nullptr);
            if (display != EGL_NO_DISPLAY)
            {
                return display;
            }
        }
#endif

#if defined(MANGO_HAS_XLIB_WINDOW) || defined(MANGO_HAS_XCB_WINDOW)
        {
            const mango::WindowSystem ws = mango::Window::getWindowSystem();
            if (native_display &&
                (ws == mango::WindowSystem::Xlib || ws == mango::WindowSystem::Xcb))
            {
                const EGLenum platform =
#if defined(MANGO_HAS_XCB_WINDOW)
                    (ws == mango::WindowSystem::Xcb) ? EGL_PLATFORM_XCB_EXT :
#endif
                    EGL_PLATFORM_X11_KHR;

                EGLDisplay display = eglGetPlatformDisplay(platform, native_display, nullptr);
                if (display != EGL_NO_DISPLAY)
                {
                    return display;
                }
            }
        }
#endif
#endif // defined(EGL_VERSION_1_5)

        return eglGetDisplay(native_display
            ? reinterpret_cast<EGLNativeDisplayType>(native_display)
            : EGL_DEFAULT_DISPLAY);
    }

} // namespace

namespace mango
{
    using namespace math;

    // -----------------------------------------------------------------------
    // OpenGLContextEGL
    // -----------------------------------------------------------------------

    struct OpenGLContextEGL : OpenGLContext
    {
        EGLDisplay egl_display = EGL_NO_DISPLAY;
        EGLContext egl_context = EGL_NO_CONTEXT;
        EGLSurface egl_surface = EGL_NO_SURFACE;

        WindowBackend* window;

        opengl::egl::NativeWindowBinding m_native_binding;

        OpenGLContextEGL(OpenGLWindow* theContext, int width, int height, u32 flags, const OpenGLWindow::Config* configPtr, OpenGLWindow* theShared)
            : window(theContext->backend())
        {
            void* native_display = opengl::egl::getNativeDisplay(window);
            egl_display = openEGLDisplay(native_display);
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
            OpenGLWindow::Config config;
            if (configPtr)
            {
                // Override defaults
                config = *configPtr;
            }

            // Configure attributes

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

            m_native_binding = opengl::egl::createNativeWindow(window, width, height, flags);
            if (!m_native_binding.native_window)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] native window creation failed.");
            }

            {
                EGLConfigChoice choice = chooseEGLConfigWithRetry(egl_display, config,
                    m_native_binding.present_opaque);
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
                MANGO_EXCEPTION("[OpenGLContextEGL] eglCreateContext() failed (0x{:x}).", eglGetError());
            }

            printLine("[EGL] eglCreateContext() : OK");
            printLine("[EGL] native window: {} x {}", width, height);

            EGLint surface_error = EGL_SUCCESS;
            egl_surface = createEGLWindowSurface(egl_display, eglConfig, m_native_binding, srgb_surface,
                m_native_binding.present_opaque, &surface_error);
            if (egl_surface == EGL_NO_SURFACE)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextEGL] eglCreateWindowSurface() failed (0x{:x}).", surface_error);
            }

            window->presentGraphicsSurface();

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
                    egl_surface = EGL_NO_SURFACE;
                }

                if (egl_context != EGL_NO_CONTEXT)
                {
                    eglDestroyContext(egl_display, egl_context);
                    egl_context = EGL_NO_CONTEXT;
                }

                eglTerminate(egl_display);
                egl_display = EGL_NO_DISPLAY;
            }

            opengl::egl::destroyNativeWindow(m_native_binding);
            if (window)
            {
                window->graphics_hooks = {};
            }
        }

        void makeCurrent() override
        {
            eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
        }

        void swapBuffers() override
        {
            window->presentGraphicsSurface();
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

    OpenGLContext* createOpenGLContextEGL(OpenGLWindow* parent, int width, int height, u32 flags, const OpenGLWindow::Config* configPtr, OpenGLWindow* shared)
    {
        auto* context = new OpenGLContextEGL(parent, width, height, flags, configPtr, shared);
        return context;
    }

} // namespace mango

#endif // defined(MANGO_OPENGL_CONTEXT_EGL)

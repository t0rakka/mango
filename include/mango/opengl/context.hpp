/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <set>
#include <string>
#include <mango/core/configure.hpp>
#include <mango/math/math.hpp>

// -----------------------------------------------------------------------
// OpenGL Configuration
// -----------------------------------------------------------------------

#if defined(MANGO_PLATFORM_WINDOWS)

    #define MANGO_OPENGL_CONTEXT_WGL
    #define MANGO_OPENGL_FRAMEBUFFER

    #include <GL/gl.h>
    #include <mango/opengl/khronos/GL/glext.h>
    #include <mango/opengl/khronos/GL/wgl.h>
    #include <mango/opengl/khronos/GL/wglext.h>

    #define GLEXT_PROC(proc, name) MANGO_API extern proc name

    #include <mango/opengl/func/glext.hpp>
    #include <mango/opengl/func/wglext.hpp>

    #undef GLEXT_PROC

#endif

#if defined(MANGO_PLATFORM_MACOS)

    #define MANGO_OPENGL_CONTEXT_COCOA
    #define MANGO_OPENGL_FRAMEBUFFER

    #define GL_SILENCE_DEPRECATION

    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>

    #define GL_GLEXT_PROTOTYPES
    #include <mango/opengl/khronos/GL/glext.h>

#endif

#if defined(MANGO_PLATFORM_LINUX)

    #if defined(MANGO_HAS_XLIB_WINDOW) || defined(MANGO_HAS_XCB_WINDOW)
        #define MANGO_OPENGL_CONTEXT_GLX
    #endif

    #if defined(MANGO_OPENGL_CONTEXT_EGL)
        // Set by mango-opengl when EGL support is enabled.
    #elif defined(MANGO_HAS_WAYLAND_WINDOW) || defined(MANGO_OPENGL_ENABLE_EGL)
        #define MANGO_OPENGL_CONTEXT_EGL
    #endif

    #define MANGO_OPENGL_FRAMEBUFFER

    #define GL_GLEXT_PROTOTYPES
    #include <GL/gl.h>
    #include <GL/glext.h>

#endif

// -----------------------------------------------------------------------
// OpenGL API
// -----------------------------------------------------------------------

#include <mango/image/compression.hpp>
#include <mango/window/window.hpp>

namespace mango
{

    // -------------------------------------------------------------------
    // OpenGLContext
    // -------------------------------------------------------------------

    struct OpenGLContext
    {
        OpenGLContext() {}
        virtual ~OpenGLContext() {}

        virtual void makeCurrent() = 0;
        virtual void swapBuffers() = 0;
        virtual void swapInterval(int interval) = 0;
        virtual void toggleFullscreen() = 0;
        virtual bool isFullscreen() const = 0;
        virtual math::int32x2 getWindowSize() const = 0;
    };

    // -------------------------------------------------------------------
    // OpenGLWindow
    // -------------------------------------------------------------------

    class OpenGLWindow : public Window
    {
    public:
        enum Flags : u32
        {
            EGL = Window::API_EGL
        };

        struct Config
        {
            u32 version  = 0;
            u32 red      = 8;
            u32 green    = 8;
            u32 blue     = 8;
            u32 alpha    = 8;
            u32 depth    = 24;
            u32 stencil  = 8;
            u32 samples  = 1;
        };

        struct InternalFormat
        {
            GLenum internalFormat;
            image::Format format;
            const char* name;
        };

    protected:
        std::unique_ptr<OpenGLContext> m_context;
        std::set<std::string> m_extensions;
        bool m_is_gles = false;
        int m_version = 0;
        bool m_context_ready = false;

        void initExtensionMask();
        void initContext(int width, int height, u32 flags, const Config* configPtr, OpenGLWindow* shared);

    public:
        OpenGLWindow(int width, int height, u32 flags = 0, const Config* config = nullptr, OpenGLWindow* shared = nullptr);
        OpenGLWindow(math::int32x2 extent, u32 flags = 0, const Config* config = nullptr, OpenGLWindow* shared = nullptr);
        ~OpenGLWindow();

        bool isExtension(const std::string& name) const;
        bool isGLES() const;
        int getVersion() const;
        bool isCompressedTextureSupported(u32 compression) const;
        const InternalFormat* getInternalFormat(GLenum internalFormat) const;

        void makeCurrent();
        void swapBuffers();
        void swapInterval(int interval);
        void toggleFullscreen() override;
        bool isFullscreen() const override;
        math::int32x2 getWindowSize() const override;

        void enterEventLoop();
        void enterEventLoop(const EventLoopConfig& config);

        // Called once before the window is shown and the event loop starts.
        virtual void onContextReady();

        // extension masks

        struct
        {
            #define GL_EXTENSION(Name) u32 Name : 1;
            #include <mango/opengl/func/glext.hpp>
            #undef GL_EXTENSION
        } ext;

        struct
        {
            #define CORE_EXTENSION(Version, Name) u32 Name : 1;
            #include <mango/opengl/func/glcorearb.hpp>
            #undef CORE_EXTENSION

            u32 texture_compression_dxt1 : 1;
            u32 texture_compression_dxt3 : 1;
            u32 texture_compression_dxt5 : 1;
            u32 texture_compression_etc2 : 1;
            u32 texture_compression_eac  : 1;
            u32 texture_compression_latc : 1;
            u32 texture_compression_atc  : 1;
            u32 texture_compression_astc_ldr : 1;
            u32 texture_compression_astc_hdr : 1;
        } core;
    };

} // namespace mango

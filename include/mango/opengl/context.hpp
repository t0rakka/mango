/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <set>
#include <string>
#include <mango/core/configure.hpp>
#include <mango/math/math.hpp>

// -----------------------------------------------------------------------
// OpenGL Configuration
// -----------------------------------------------------------------------

#if defined(MANGO_WINDOW_SYSTEM_WIN32)

    // -----------------------------------------------------------------------
    // WGL
    // -----------------------------------------------------------------------

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

#if defined(MANGO_WINDOW_SYSTEM_COCOA)

    // -----------------------------------------------------------------------
    // Cocoa
    // -----------------------------------------------------------------------

    #define MANGO_OPENGL_CONTEXT_COCOA
    #define MANGO_OPENGL_FRAMEBUFFER

    #define GL_SILENCE_DEPRECATION /* macOS 10.14 deprecated OpenGL API */

    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>

    #define GL_GLEXT_PROTOTYPES
    #include <mango/opengl/khronos/GL/glext.h>

#endif

#if defined(MANGO_WINDOW_SYSTEM_XLIB) || defined(MANGO_WINDOW_SYSTEM_XCB)

    // -----------------------------------------------------------------------
    // GLX | EGL
    // -----------------------------------------------------------------------

    #if defined(MANGO_ENABLE_EGL)
        #define MANGO_OPENGL_CONTEXT_EGL
    #else
        #define MANGO_OPENGL_CONTEXT_GLX
    #endif

    #define MANGO_OPENGL_FRAMEBUFFER

    #define GL_GLEXT_PROTOTYPES
    #include <GL/gl.h>
    #include <GL/glext.h>

    #define GLX_GLXEXT_PROTOTYPES
    #include <GL/glx.h>
    #include <GL/glxext.h>

    #if defined(Status)
        #undef Status
        typedef int Status;
    #endif

#endif

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

    // -----------------------------------------------------------------------
    // EGL
    // -----------------------------------------------------------------------

    #if defined(MANGO_ENABLE_EGL)
        #define MANGO_OPENGL_CONTEXT_EGL
        #define MANGO_OPENGL_FRAMEBUFFER
    #else
        #define MANGO_OPENGL_CONTEXT_NONE
    #endif

    #define GL_GLEXT_PROTOTYPES
    #include <GL/gl.h>
    #include <GL/glext.h>

#endif

#if defined(MANGO_WINDOW_SYSTEM_NONE)

    // -----------------------------------------------------------------------
    // NONE
    // -----------------------------------------------------------------------

    #define MANGO_OPENGL_CONTEXT_NONE

#endif

// -----------------------------------------------------------------------
// OpenGL API
// -----------------------------------------------------------------------

#if !defined(MANGO_OPENGL_CONTEXT_NONE)

#include <mango/image/compression.hpp>
#include <mango/window/window.hpp>

namespace mango
{

    // -------------------------------------------------------------------
    // OpenGLContextHandle
    // -------------------------------------------------------------------

    struct OpenGLContextHandle
    {
        OpenGLContextHandle() {}
        virtual ~OpenGLContextHandle() {}

        virtual void makeCurrent() = 0;
        virtual void swapBuffers() = 0;
        virtual void swapInterval(int interval) = 0;
        virtual void toggleFullscreen() = 0;
        virtual bool isFullscreen() const = 0;
        virtual math::int32x2 getWindowSize() const = 0;
    };

    // -------------------------------------------------------------------
    // OpenGLContext
    // -------------------------------------------------------------------

    class OpenGLContext : public Window
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
        std::unique_ptr<OpenGLContextHandle> m_context;
        std::set<std::string> m_extensions;
        bool m_is_gles = false;
        int m_version = 0;

        void initExtensionMask();
        void initContext(int width, int height, u32 flags, const Config* configPtr, OpenGLContext* shared);

    public:
        OpenGLContext(int width, int height, u32 flags = 0, const Config* config = nullptr, OpenGLContext* shared = nullptr);
        OpenGLContext(math::int32x2 extent, u32 flags = 0, const Config* config = nullptr, OpenGLContext* shared = nullptr);
        ~OpenGLContext();

        bool isExtension(const std::string& name) const;
        bool isGLES() const;
        int getVersion() const;
        bool isCompressedTextureSupported(u32 compression) const;
        const InternalFormat* getInternalFormat(GLenum internalFormat) const;

        void makeCurrent();
        void swapBuffers();
        void swapInterval(int interval);
        void toggleFullscreen();
        bool isFullscreen() const;
        math::int32x2 getWindowSize() const override;

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

#if defined(MANGO_OPENGL_CONTEXT_WGL)
        struct
        {
            #define WGL_EXTENSION(Name) u32 Name : 1;
            #include <mango/opengl/func/wglext.hpp>
            #undef WGL_EXTENSION
        } wgl;
#endif

#if defined(MANGO_OPENGL_CONTEXT_GLX)
        struct
        {
            #define GLX_EXTENSION(Name) u32 Name : 1;
            #include <mango/opengl/func/glxext.hpp>
            #undef GLX_EXTENSION
        } glx;
#endif
    };

} // namespace mango

#endif // !defined(MANGO_OPENGL_CONTEXT_NONE)

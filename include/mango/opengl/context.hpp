/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <set>
#include <string>
#include <mango/core/configure.hpp>

// -----------------------------------------------------------------------
// OpenGL API
// -----------------------------------------------------------------------

#if defined(MANGO_PLATFORM_WINDOWS)

    // -----------------------------------------------------------------------
    // WGL
    // -----------------------------------------------------------------------

    #define MANGO_OPENGL_CONTEXT_WGL
    #define MANGO_OPENGL_FRAMEBUFFER
    #define MANGO_OPENGL_JPEG

    #define GLEXT_PROC(proc, name) extern proc name

    #include <GL/gl.h>
    #include <mango/opengl/khronos/GL/glext.h>
    #include <mango/opengl/func/glext.hpp>

    #include <mango/opengl/khronos/GL/wgl.h>
    #include <mango/opengl/khronos/GL/wglext.h>
    #include <mango/opengl/func/wglext.hpp>

    #undef GLEXT_PROC

#elif defined(MANGO_PLATFORM_OSX)

    #if defined(__ppc__)
        #define MANGO_OPENGL_CONTEXT_NONE
    #else

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

#elif defined(MANGO_PLATFORM_IOS)

    // -----------------------------------------------------------------------
    // EGL
    // -----------------------------------------------------------------------

    #define MANGO_OPENGL_CONTEXT_NONE

    //#include <OpenGLES/ES1/gl.h>
    //#include <OpenGLES/ES1/glext.h>

#elif defined(MANGO_PLATFORM_ANDROID)

    // -----------------------------------------------------------------------
    // EGL
    // -----------------------------------------------------------------------

    #define MANGO_OPENGL_CONTEXT_NONE
    /*
    #define MANGO_OPENGL_CONTEXT_EGL

    //#include <GLES/gl.h>
    //#include <GLES/glext.h>
    //#include <GLES2/gl2.h>
    #include <GLES3/gl32.h>
    */

#elif defined(MANGO_PLATFORM_EMSCRIPTEN)

    // -----------------------------------------------------------------------
    // Emscripten / WASM
    // -----------------------------------------------------------------------

    #define MANGO_OPENGL_CONTEXT_NONE

#elif defined(MANGO_PLATFORM_UNIX)

    // -----------------------------------------------------------------------
    // GLX | EGL
    // -----------------------------------------------------------------------

    #if defined(MANGO_ENABLE_EGL)
        #define MANGO_OPENGL_CONTEXT_EGL
    #endif

    #define MANGO_OPENGL_CONTEXT_GLX
    #define MANGO_OPENGL_FRAMEBUFFER
    #define MANGO_OPENGL_JPEG

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

#else

    #define MANGO_OPENGL_CONTEXT_NONE

#endif

#ifndef MANGO_OPENGL_CONTEXT_NONE

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
            EGL     = 0x00010000,
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
            bool srgb    = false;
            bool hdr     = false;
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
            u32 texture_compression_eac : 1;
            u32 texture_compression_latc : 1;
            u32 texture_compression_atc : 1;
            u32 texture_compression_astc : 1;
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

#endif // MANGO_OPENGL_CONTEXT_NONE

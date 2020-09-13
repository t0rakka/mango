/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <set>
#include <string>
#include "../core/configure.hpp"

// -----------------------------------------------------------------------
// OpenGL API
// -----------------------------------------------------------------------

//#define MANGO_OPENGL_DISABLE_PLATFORM_API

//#define MANGO_OPENGL_LEGACY_PROFILE
//#define MANGO_OPENGL_CORE_PROFILE

#if defined(MANGO_PLATFORM_WINDOWS)

    // -----------------------------------------------------------------------
    // WGL
    // -----------------------------------------------------------------------

    #define MANGO_OPENGL_CONTEXT_WGL

    #define GLEXT_PROC(proc, name) extern proc name

    #ifdef MANGO_OPENGL_CORE_PROFILE
        #include "khronos/GL/glcorearb.h"
        #include "func/glcorearb.hpp"
    #else
        #include <GL/gl.h>
        #include "khronos/GL/glext.h"
        #include "func/glext.hpp"
    #endif

    #ifndef MANGO_OPENGL_DISABLE_PLATFORM_API
        #include "khronos/GL/wgl.h"
        #include "khronos/GL/wglext.h"
        #include "func/wglext.hpp"
    #endif

    #undef GLEXT_PROC

#elif defined(MANGO_PLATFORM_OSX)

    // -----------------------------------------------------------------------
    // Cocoa
    // -----------------------------------------------------------------------

    #define MANGO_OPENGL_CONTEXT_COCOA

    #define GL_SILENCE_DEPRECATION /* macOS 10.14 deprecated OpenGL API */

    #if defined(MANGO_OPENGL_LEGACY_PROFILE)
        #include <OpenGL/gl.h>
        #include <OpenGL/glext.h>
    #else
        #include "OpenGL/gl3.h"
        #include "OpenGL/gl3ext.h"

        #define GL_GLEXT_PROTOTYPES
        #include "khronos/GL/glext.h"
    #endif

#elif defined(MANGO_PLATFORM_IOS)

    // -----------------------------------------------------------------------
    // EGL
    // -----------------------------------------------------------------------

    #define MANGO_OPENGL_CONTEXT_EGL

    //#include <OpenGLES/ES1/gl.h>
    //#include <OpenGLES/ES1/glext.h>

    // TODO: EGL context

#elif defined(MANGO_PLATFORM_ANDROID)

    // -----------------------------------------------------------------------
    // EGL
    // -----------------------------------------------------------------------

    #define MANGO_OPENGL_CONTEXT_EGL

    //#include <GLES/gl.h>
    //#include <GLES/glext.h>
    //#include <GLES2/gl2.h>
    #include <GLES3/gl3.h>

    // TODO: EGL context

#elif defined(MANGO_PLATFORM_UNIX)

    // -----------------------------------------------------------------------
    // GLX
    // -----------------------------------------------------------------------

    #define MANGO_OPENGL_CONTEXT_GLX

    #ifdef MANGO_OPENGL_CORE_PROFILE
        #define GL_GLEXT_PROTOTYPES
        #include "khronos/GL/glcorearb.h"
    #else
        #define GL_GLEXT_PROTOTYPES
        #include <GL/gl.h>
        #include <GL/glext.h>
    #endif

    #ifndef MANGO_OPENGL_DISABLE_PLATFORM_API
        #define GLX_GLXEXT_PROTOTYPES
        #include <GL/glx.h>
        #include <GL/glxext.h>
        #if defined (Status)
        #undef Status
            typedef int Status;
        #endif
    #endif

#else

    //#error "Unsupported OpenGL implementation."

#endif

#include "../image/compression.hpp"
#include "../window/window.hpp"

namespace mango {
namespace opengl {

    // -----------------------------------------------------------------------
    // ContextAttribute
    // -----------------------------------------------------------------------

    struct ContextAttribute
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

    // -------------------------------------------------------------------
    // Context
    // -------------------------------------------------------------------

    class Context : public Window
    {
    protected:
        struct ContextHandle* m_context;
        std::set<std::string> m_extensions;

        void initExtensionMask();

    public:
        Context(int width, int height, u32 flags = 0, const ContextAttribute* attrib = nullptr, Context* shared = nullptr);
        ~Context();

        bool isExtension(const std::string& name) const;
        bool isGLES() const;
        int getVersion() const;

        void makeCurrent();
        void swapBuffers();
        void swapInterval(int interval);
        void toggleFullscreen();
        bool isFullscreen() const;
        int32x2 getWindowSize() const override;
    };

    // -------------------------------------------------------------------
    // glext
    // -------------------------------------------------------------------

    struct glExtensionMask
    {
#define GL_EXTENSION(Name) u32 Name : 1;
#include "func/glext.hpp"
#undef GL_EXTENSION
    };

    extern glExtensionMask glext;

    // -------------------------------------------------------------------
    // core
    // -------------------------------------------------------------------

    struct coreExtensionMask
    {
#define CORE_EXTENSION(Version, Name) u32 Name : 1;
#include "func/glcorearb.hpp"
#undef CORE_EXTENSION

        // custom extension flags
        u32 texture_compression_dxt1 : 1;
        u32 texture_compression_dxt3 : 1;
        u32 texture_compression_dxt5 : 1;
        u32 texture_compression_etc2 : 1;
        u32 texture_compression_eac : 1;
        u32 texture_compression_latc : 1;
        u32 texture_compression_atc : 1;
    };

    extern coreExtensionMask core;

    // -------------------------------------------------------------------
    // helper functions ; require active context
    // -------------------------------------------------------------------

    struct InternalFormat
    {
        GLenum internal_format;
        Format format;
        bool srgb;
        const char* name;
    };

    bool isCompressedTextureSupported(TextureCompression compression);
    const InternalFormat* getInternalFormat(GLenum internalFormat);

#ifndef MANGO_OPENGL_DISABLE_PLATFORM_API

    // -------------------------------------------------------------------
    // wglext
    // -------------------------------------------------------------------

#ifdef MANGO_OPENGL_CONTEXT_WGL

    struct wglExtensionMask
    {
#define WGL_EXTENSION(Name) u32 Name : 1;
#include "func/wglext.hpp"
#undef WGL_EXTENSION
    };

    extern wglExtensionMask wglext;

#endif // MANGO_OPENGL_CONTEXT_WGL

    // -------------------------------------------------------------------
    // glxext
    // -------------------------------------------------------------------

#ifdef MANGO_OPENGL_CONTEXT_GLX

    struct glxExtensionMask
    {
#define GLX_EXTENSION(Name) u32 Name : 1;
#include "func/glxext.hpp"
#undef GLX_EXTENSION
    };

    extern glxExtensionMask glxext;

#endif // MANGO_OPENGL_CONTEXT_GLX

#endif // MANGO_OPENGL_DISABLE_PLATFORM_API

} // namespace opengl
} // namespace mango

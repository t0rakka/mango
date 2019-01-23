/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <set>
#include <string>
#include "../core/configure.hpp"
#include "../image/compression.hpp"
#include "../window/window.hpp"

// -----------------------------------------------------------------------
// OpenGL API
// -----------------------------------------------------------------------

#if defined(MANGO_PLATFORM_WINDOWS)

    //#define MANGO_CORE_PROFILE
    #define GLEXT_PROC(proc, name) extern proc name

    #ifdef MANGO_CORE_PROFILE
        #include "khronos/glcorearb.h"
        #include "khronos/wglext.h"
        #include "func/glcorearb.hpp"
        #include "func/wglext.hpp"
    #else
        #include <GL/gl.h>
        #include "khronos/glext.h"
        #include "khronos/wglext.h"
        #include "func/glext.hpp"
        #include "func/wglext.hpp"
    #endif

    #undef GLEXT_PROC

    #define MANGO_CONTEXT_WGL

#elif defined(MANGO_PLATFORM_IOS)

    //#include <OpenGLES/ES1/gl.h>
    //#include <OpenGLES/ES1/glext.h>

    #define MANGO_CONTEXT_EGL
	// TODO: EGL context

#elif defined(MANGO_PLATFORM_OSX)

    #define GL_SILENCE_DEPRECATION /* macOS 10.14 deprecated OpenGL API */

    #define MANGO_CORE_PROFILE

    #ifdef MANGO_CORE_PROFILE
        #include "OpenGL/gl3.h"
        #include "OpenGL/gl3ext.h"
    #else
        #include "OpenGL/gl.h"
    #endif

    #define GL_GLEXT_PROTOTYPES
    #include "khronos/glext.h"

    #define MANGO_CONTEXT_COCOA

#elif defined(MANGO_PLATFORM_ANDROID)

    //#include <GLES/gl.h>
    //#include <GLES/glext.h>
    //#include <GLES2/gl2.h>
    #include <GLES3/gl3.h>

    #define MANGO_CONTEXT_EGL
	// TODO: EGL context

#elif defined(MANGO_PLATFORM_UNIX)

    #define MANGO_CORE_PROFILE
    #define GL_GLEXT_PROTOTYPES

    #include <GL/gl.h>
    #include <GL/glx.h>

#if 0 // NOTE: use platform headers
    #define GL_GLEXT_PROTOTYPES
    #include "khronos/glext.h"

    #define GLX_GLXEXT_PROTOTYPES
    #include "khronos/glxext.h"
#endif

    #define MANGO_CONTEXT_GLX

#else

	//#error "Unsupported OpenGL implementation."

#endif

namespace mango {
namespace opengl {

    // -----------------------------------------------------------------------
    // ContextAttribute
    // -----------------------------------------------------------------------

    struct ContextAttribute
    {
        uint32 red      = 8;
        uint32 green    = 8;
        uint32 blue     = 8;
        uint32 alpha    = 8;
        uint32 depth    = 24;
        uint32 stencil  = 8;
        uint32 samples  = 1;
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
        Context(int width, int height, const ContextAttribute* attrib = nullptr, Context* shared = nullptr);
        ~Context();

        bool isExtension(const std::string& name) const;
        bool isGLES() const;
        int getVersion() const;

        void makeCurrent();
        void swapBuffers();
        void swapInterval(int interval);
        void toggleFullscreen();
        bool isFullscreen() const;
    };

	// -------------------------------------------------------------------
	// glext
	// -------------------------------------------------------------------

    struct glExtensionMask
    {
#define GL_EXTENSION(Name) uint32 Name : 1;
#include "func/glext.hpp"
#undef GL_EXTENSION
    };

    extern glExtensionMask glext;

    // -------------------------------------------------------------------
    // core
    // -------------------------------------------------------------------

    struct coreExtensionMask
    {
#define CORE_EXTENSION(Version, Name) uint32 Name : 1;
#include "func/glcorearb.hpp"
#undef CORE_EXTENSION

        // custom extension flags
        uint32 texture_compression_dxt1 : 1;
        uint32 texture_compression_dxt3 : 1;
        uint32 texture_compression_dxt5 : 1;
        uint32 texture_compression_etc2 : 1;
        uint32 texture_compression_eac : 1;
        uint32 texture_compression_latc : 1;
        uint32 texture_compression_atc : 1;
    };

    extern coreExtensionMask core;

	// -------------------------------------------------------------------
	// wglext
	// -------------------------------------------------------------------

#ifdef MANGO_CONTEXT_WGL

    struct wglExtensionMask
    {
#define WGL_EXTENSION(Name) uint32 Name : 1;
#include "func/wglext.hpp"
#undef WGL_EXTENSION
    };

    extern wglExtensionMask wglext;

#endif

	// -------------------------------------------------------------------
	// glxext
	// -------------------------------------------------------------------

#ifdef MANGO_CONTEXT_GLX

    struct glxExtensionMask
    {
#define GLX_EXTENSION(Name) uint32 Name : 1;
#include "func/glxext.hpp"
#undef GLX_EXTENSION
    };

    extern glxExtensionMask glxext;

#endif

	// -------------------------------------------------------------------
    // helper functions ; require active context
	// -------------------------------------------------------------------

    struct InternalFormat
    {
        GLenum internalFormat;
        Format format;
        bool srgb;
        const char* name;
    };

    bool isCompressedTextureSupported(TextureCompression compression);
    const InternalFormat* getInternalFormat(GLenum internalFormat);

} // namespace opengl
} // namespace mango

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
        #undef GL_GLEXT_PROTOTYPES
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
        #undef GL_GLEXT_PROTOTYPES
    #endif

    // #define GLX_GLXEXT_PROTOTYPES
    // #include <GL/glx.h>
    // #include <GL/glxext.h>

#else

	//#error "Unsupported OpenGL implementation."

#endif


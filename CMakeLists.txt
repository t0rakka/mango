cmake_minimum_required(VERSION 3.19)
project(mango
        VERSION 3.0.0
        DESCRIPTION "C++ Graphics Library for the NÖRDS"
        LANGUAGES CXX C)

# ------------------------------------------------------------------------------
# options
# ------------------------------------------------------------------------------

OPTION(BUILD_SHARED_LIBS    "Build shared libraries"                     ON)

OPTION(BUILD_OPENGL         "Build OpenGL support"                      OFF)
OPTION(BUILD_VULKAN         "Build Vulkan support"                      OFF)
OPTION(BUILD_IMPORT3D       "Build Import3D support"                    OFF)
OPTION(BUILD_EXAMPLES       "Build examples"                            OFF)

# Window system backends (Linux). All compiled and linked together so the active
# backend is chosen at runtime via the WindowSystem enum; toggle one off to drop
# it from the binary (and its link-time dependency).
OPTION(ENABLE_XLIB          "Build the Xlib window backend (Linux)"     ON)
OPTION(ENABLE_XCB           "Build the Xcb window backend (Linux)"      ON)
OPTION(ENABLE_WAYLAND       "Build the Wayland window backend (Linux)"  ON)

# EGL is used for the Wayland GL context and, optionally, for the X11 backends.
# It is enabled by default on Linux so Wayland GL always works; the GLX path
# remains available for X11 regardless.
OPTION(ENABLE_EGL           "Use EGL for OpenGL context management"     ON)

OPTION(LIBRARY_BZIP2        "Enable BZIP2 compression support"          ON)
OPTION(LIBRARY_LZ4          "Enable LZ4 compression support"            ON)
OPTION(LIBRARY_LZMA         "Enable LZMA compression support"           ON)
OPTION(LIBRARY_LZFSE        "Enable LZFSE compression support"          ON)
OPTION(LIBRARY_ISAL         "Enable Intel ISA-L compression support"    ON)

OPTION(LIBRARY_JXR          "Enable JPEG XR image format support"       ON)
OPTION(LIBRARY_JXL          "Enable jpeg-xl image format support"       ON)
OPTION(LIBRARY_JP2          "Enable jpeg2000 image format support"      ON)
OPTION(LIBRARY_WEBP         "Enable WebP image format support"          ON)
OPTION(LIBRARY_WP2          "Enable WP2 image format support"           ON)
OPTION(LIBRARY_AVIF         "Enable avif image format support"          ON)
OPTION(LIBRARY_HEIF         "Enable heif image format support"          ON)
OPTION(LIBRARY_LIBRAW       "Enable camera raw support (LibRaw)"        ON)
OPTION(LIBRARY_ASTC         "Enable ASTC block compression support"     ON)
OPTION(IMAGE_FORMAT_RETRO   "Enable RETRO image format support"         ON)

OPTION(ENABLE_NEON          "Enable ARM NEON instructions"              ON)
OPTION(ENABLE_SSE2          "Enable SSE2 instructions"                  OFF)
OPTION(ENABLE_SSE4          "Enable SSE4.1 and SSE4.2 instructions"     ON)
OPTION(ENABLE_AVX           "Enable AVX instructions"                   OFF)
OPTION(ENABLE_AVX2          "Enable AVX2 instructions"                  OFF)
OPTION(ENABLE_AVX512        "Enable AVX-512 instructions"               OFF)

OPTION(ENABLE_PCLMUL        "Enable PCLMUL"                             OFF)
OPTION(ENABLE_POPCNT        "Enable POPCNT"                             OFF)
OPTION(ENABLE_AES           "Enable AES"                                OFF)
OPTION(ENABLE_F16C          "Enable F16C conversion instructions"       OFF)
OPTION(ENABLE_LZCNT         "Enable LZCNT"                              OFF)
OPTION(ENABLE_BMI           "Enable BMI"                                OFF)
OPTION(ENABLE_BMI2          "Enable BMI2"                               OFF)
OPTION(ENABLE_FMA           "Enable FMA"                                OFF)
OPTION(ENABLE_SHA           "Enable SHA"                                OFF)

# Option to disable every optional library
option(LIBRARY_DISABLE,     "Disable ALL optional libraries"            OFF)
if (LIBRARY_DISABLE)
    set(LIBRARY_JXR    OFF)
    set(LIBRARY_JXL    OFF)
    set(LIBRARY_JP2    OFF)
    set(LIBRARY_WEBP   OFF)
    set(LIBRARY_WP2    OFF)
    set(LIBRARY_AVIF   OFF)
    set(LIBRARY_HEIF   OFF)
    set(LIBRARY_LIBRAW OFF)
    set(LIBRARY_ISAL   OFF)
endif ()

# Option to build everything
option(BUILD_ALL,           "Build everything"                          OFF)
if (BUILD_ALL)
    set(BUILD_OPENGL     ON)
    set(BUILD_VULKAN     ON)
    set(BUILD_IMPORT3D   ON)
    set(BUILD_EXAMPLES   ON)
endif ()

if (BUILD_OPENGL OR BUILD_VULKAN)
    set(BUILD_MANGO_WINDOW ON)
else ()
    set(BUILD_MANGO_WINDOW OFF)
endif ()

# Option to enable every Intel ISA extension
option(INTEL_DELUXE         "Enable ALL Intel ISA extensions"           OFF)
if (INTEL_DELUXE OR ENABLE_AVX512)
    set(ENABLE_AVX2     ON)
    set(ENABLE_PCLMUL   ON)
    set(ENABLE_POPCNT   ON)
    set(ENABLE_AES      ON)
    set(ENABLE_F16C     ON)
    set(ENABLE_LZCNT    ON)
    set(ENABLE_BMI      ON)
    set(ENABLE_BMI2     ON)
    set(ENABLE_FMA      ON)
    set(ENABLE_SHA      ON)
endif ()

# Linker selection
set(LINKER "default" CACHE STRING "Linker to use (default, mold)")
set_property(CACHE LINKER PROPERTY STRINGS default mold)

# Emscripten
if (EMSCRIPTEN)
    # Host macOS toolchain variables must not leak into em++ invocations.
    set(CMAKE_OSX_ARCHITECTURES "" CACHE STRING "" FORCE)
    set(CMAKE_OSX_SYSROOT "" CACHE PATH "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "" CACHE STRING "" FORCE)

    if (BUILD_OPENGL)
        set(BUILD_OPENGL OFF)
    endif ()
    if (BUILD_VULKAN)
        set(BUILD_VULKAN OFF)
    endif ()
    if (BUILD_IMPORT3D)
        set(BUILD_IMPORT3D OFF)
    endif ()

    message(STATUS "Configuring for Emscripten with pthreads")

    # Enable pthreads in compiler and linker
    add_compile_options(-pthread)
    add_link_options(
        "-pthread"
        "-sUSE_PTHREADS=1"
        "-sPTHREAD_POOL_SIZE=32"
        "-sPTHREAD_POOL_SIZE_STRICT=0"
    )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -sUSE_PTHREADS=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sPTHREAD_POOL_SIZE=32")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sPTHREAD_POOL_SIZE_STRICT=0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sALLOW_MEMORY_GROWTH=1")

    # Enable WebAssembly SIMD
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msimd128")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msimd128")

    # Enable Intel intrinsic transcoding to WASM SIMD
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2")

    # Enable native filesystem with node
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sENVIRONMENT=node -sNODERAWFS=1")
endif ()

# ------------------------------------------------------------------------------
# build settings
# ------------------------------------------------------------------------------

# c++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    # Microsoft compiler does not generate __cplusplus automatically
    add_compile_options(/Zc:__cplusplus)
endif ()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

if (WIN32 OR CYGWIN)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

find_package(Threads)

if (BUILD_OPENGL)
    if (WIN32)
        set(OpenGL_GL_PREFERENCE LEGACY)
    else ()
        set(OpenGL_GL_PREFERENCE GLVND)
    endif ()

    find_package(OpenGL QUIET)
    if (NOT OPENGL_FOUND)
        set(BUILD_OPENGL OFF)
    endif ()
endif ()

if (BUILD_VULKAN)
    find_package(Vulkan QUIET)
    if (NOT VULKAN_FOUND)
        set(BUILD_VULKAN OFF)
    endif ()
endif ()

message("[${PROJECT_NAME}: ${PROJECT_VERSION}]")
list(APPEND CMAKE_MESSAGE_INDENT "    ")

message("Processor: ${CMAKE_SYSTEM_PROCESSOR}")

if (CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
    set(X86_64 1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "i686.*|i386.*|x86.*")
    set(X86 1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*|arm64.*)")
    set(ARM64 1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)")
    set(ARM 1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "PPC64*|ppc64*|powerpc64*")
    set(PPC64 1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "PPC*|ppc*|powerpc*")
    set(PPC 1)
endif ()

message("Compiler: ${CMAKE_CXX_COMPILER_ID}")

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # msvc is set by cmake
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CLANG 1)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Intel")
    set(GCC 1)
endif ()

# Configure linker
if (LINKER STREQUAL "mold")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=mold")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=mold")
    else()
        message(WARNING "mold linker is only supported with GCC and Clang compilers")
    endif()
endif()

if (BUILD_SHARED_LIBS)
    message("Build: " ${CMAKE_BUILD_TYPE} " (shared)")
    set(INTERFACE PRIVATE)
else ()
    message("Build: " ${CMAKE_BUILD_TYPE} " (static)")
    set(INTERFACE PUBLIC)
endif ()

include_directories(BEFORE ${PROJECT_SOURCE_DIR}/include)

# ------------------------------------------------------------------------------
# install prefix
# ------------------------------------------------------------------------------

# vcpkg
if (DEFINED ENV{VCPKG_ROOT})

    if (DEFINED VCPKG_TARGET_TRIPLET AND NOT VCPKG_TARGET_TRIPLET STREQUAL "")
        # Prefer cache variable (from preset or CLI)
        set(TRIPLET "${VCPKG_TARGET_TRIPLET}")
    elseif (DEFINED ENV{VCPKG_TARGET_TRIPLET} AND NOT "$ENV{VCPKG_TARGET_TRIPLET}" STREQUAL "")
        # Fallback to environment variable
        set(TRIPLET "$ENV{VCPKG_TARGET_TRIPLET}")
    elseif (DEFINED VCPKG_DEFAULT_TRIPLET AND NOT VCPKG_DEFAULT_TRIPLET STREQUAL "")
        # Fallback to default triplet (cache)
        set(TRIPLET "${VCPKG_DEFAULT_TRIPLET}")
    elseif (DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT "$ENV{VCPKG_DEFAULT_TRIPLET}" STREQUAL "")
        # Fallback to default triplet (env)
        set(TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}")
    else ()
        # Last fallback
        set(TRIPLET "")
    endif ()

    if (TRIPLET)
        set(CMAKE_INSTALL_PREFIX "$ENV{VCPKG_ROOT}/installed/${TRIPLET}" CACHE PATH "Install path prefix" FORCE)
    endif ()

    message("VCPKG_ROOT: " $ENV{VCPKG_ROOT})
    message("VCPKG_TARGET_TRIPLET: ${TRIPLET}")

endif ()

message("Installation Directory: ${CMAKE_INSTALL_PREFIX}")

# ------------------------------------------------------------------------------
# sources: mango
# ------------------------------------------------------------------------------

set(MANGO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/mango")
set(MANGO_SOURCE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/source/mango")

# Linux window backends: enabled by default, soft-disable when dev packages are missing.
if (UNIX AND NOT APPLE AND NOT EMSCRIPTEN AND BUILD_MANGO_WINDOW)
    if (ENABLE_XLIB)
        find_package(X11)
        if (NOT X11_FOUND)
            message(WARNING "mango-window: Xlib dev packages not found; disabling ENABLE_XLIB")
            set(ENABLE_XLIB OFF CACHE BOOL "Build the Xlib window backend (Linux)" FORCE)
        endif ()
    endif ()
    if (ENABLE_XCB)
        find_package(PkgConfig QUIET)
        if (PkgConfig_FOUND)
            pkg_check_modules(XCB QUIET xcb)
            if (NOT XCB_FOUND)
                message(WARNING "mango-window: Xcb dev packages not found; disabling ENABLE_XCB")
                set(ENABLE_XCB OFF CACHE BOOL "Build the Xcb window backend (Linux)" FORCE)
            endif ()
        else ()
            set(ENABLE_XCB OFF CACHE BOOL "Build the Xcb window backend (Linux)" FORCE)
        endif ()
    endif ()
    if (ENABLE_WAYLAND)
        find_package(PkgConfig QUIET)
        set(_mango_wayland_ok TRUE)
        if (PkgConfig_FOUND)
            pkg_check_modules(WAYLAND_CLIENT QUIET wayland-client)
            pkg_check_modules(WAYLAND_PROTOCOLS QUIET wayland-protocols)
            find_program(WAYLAND_SCANNER wayland-scanner)
            if (NOT WAYLAND_CLIENT_FOUND OR NOT WAYLAND_PROTOCOLS_FOUND OR NOT WAYLAND_SCANNER)
                set(_mango_wayland_ok FALSE)
            endif ()
        else ()
            set(_mango_wayland_ok FALSE)
        endif ()
        if (NOT _mango_wayland_ok)
            message(WARNING "mango-window: Wayland dev packages not found; disabling ENABLE_WAYLAND")
            set(ENABLE_WAYLAND OFF CACHE BOOL "Build the Wayland window backend (Linux)" FORCE)
        endif ()
    endif ()
    if (NOT ENABLE_XLIB AND NOT ENABLE_XCB AND NOT ENABLE_WAYLAND)
        message(FATAL_ERROR "mango-window: no Linux window backend available. Install X11, Xcb, and/or Wayland dev packages.")
    endif ()
    if (ENABLE_XLIB)
        set(MANGO_HAS_XLIB_WINDOW ON)
    endif ()
    if (ENABLE_XCB)
        set(MANGO_HAS_XCB_WINDOW ON)
    endif ()
    if (ENABLE_WAYLAND)
        set(MANGO_HAS_WAYLAND_WINDOW ON)
    endif ()
endif ()

if (BUILD_MANGO_WINDOW)

    if (NOT EMSCRIPTEN)

        FILE(GLOB WINDOW_HEADERS
            "${MANGO_INCLUDE_DIR}/window/*.hpp"
        )

    endif ()

    if (WIN32 OR CYGWIN)

        FILE(GLOB WINDOW_SOURCES
            "${MANGO_SOURCE_DIR}/window/win32/*.cpp"
        )
        list(APPEND WINDOW_SOURCES ${MANGO_SOURCE_DIR}/window/window.cpp)

    elseif (APPLE)

        FILE(GLOB WINDOW_SOURCES
            "${MANGO_SOURCE_DIR}/window/cocoa/*.mm"
        )
        list(APPEND WINDOW_SOURCES ${MANGO_SOURCE_DIR}/window/window.cpp)

    elseif (NOT EMSCRIPTEN)

        # Linux: compile every enabled backend together; the active one is chosen
        # at runtime. Each backend keeps its native headers in its own TU.
        set(WINDOW_SOURCES ${MANGO_SOURCE_DIR}/window/window.cpp)

        if (ENABLE_XLIB)
            list(APPEND WINDOW_SOURCES
                ${MANGO_SOURCE_DIR}/window/xlib/xlib_window.hpp
                ${MANGO_SOURCE_DIR}/window/xlib/xlib_window.cpp
            )
        endif ()

        if (ENABLE_XCB)
            list(APPEND WINDOW_SOURCES
                ${MANGO_SOURCE_DIR}/window/xcb/xcb_window.hpp
                ${MANGO_SOURCE_DIR}/window/xcb/xcb_window.cpp
            )
        endif ()

        if (ENABLE_WAYLAND)
            list(APPEND WINDOW_SOURCES
                ${MANGO_SOURCE_DIR}/window/wayland/wayland_window.hpp
                ${MANGO_SOURCE_DIR}/window/wayland/wayland_window.cpp
            )

            pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
            pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

            set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
            set(XDG_SHELL_CLIENT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h")
            set(XDG_SHELL_PRIVATE_CODE "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c")

            add_custom_command(
                OUTPUT ${XDG_SHELL_CLIENT_HEADER} ${XDG_SHELL_PRIVATE_CODE}
                COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_XML} ${XDG_SHELL_CLIENT_HEADER}
                COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_XML} ${XDG_SHELL_PRIVATE_CODE}
                DEPENDS ${XDG_SHELL_XML}
                COMMENT "Generating xdg-shell Wayland protocol"
                VERBATIM
            )

            list(APPEND WINDOW_SOURCES ${XDG_SHELL_PRIVATE_CODE})
        endif()

    endif ()

endif ()

if (BUILD_OPENGL)

    FILE(GLOB OPENGL_HEADERS
        "${MANGO_INCLUDE_DIR}/opengl/*.hpp"
    )

    if (WIN32 OR CYGWIN)

        FILE(GLOB OPENGL_SOURCES
            "${MANGO_SOURCE_DIR}/opengl/*.cpp"
            "${MANGO_SOURCE_DIR}/opengl/wgl/*.cpp"
        )

    elseif (APPLE)

        FILE(GLOB OPENGL_SOURCES
            "${MANGO_SOURCE_DIR}/opengl/*.cpp"
            "${MANGO_SOURCE_DIR}/opengl/cocoa/*.mm"
        )

    elseif (EMSCRIPTEN)

        FILE(GLOB OPENGL_SOURCES
            "${MANGO_SOURCE_DIR}/opengl/*.cpp"
        )

    else ()

        FILE(GLOB OPENGL_SOURCES
            "${MANGO_SOURCE_DIR}/opengl/*.cpp"
            "${MANGO_SOURCE_DIR}/opengl/glx/*.cpp"
            "${MANGO_SOURCE_DIR}/opengl/egl/*.cpp"
        )

    endif ()

endif ()

if (BUILD_VULKAN)

    FILE(GLOB VULKAN_HEADERS
        "${MANGO_INCLUDE_DIR}/vulkan/*.hpp"
    )

    if (WIN32 OR CYGWIN OR APPLE)

        FILE(GLOB VULKAN_SOURCES
            "${MANGO_SOURCE_DIR}/vulkan/*.cpp"
        )

    elseif (NOT EMSCRIPTEN)

        FILE(GLOB VULKAN_SOURCES
            "${MANGO_SOURCE_DIR}/vulkan/*.cpp"
        )

    endif ()

endif ()

if (WIN32 OR CYGWIN)

    FILE(GLOB CORE_SOURCES
        "${MANGO_SOURCE_DIR}/core/*.cpp"
    )

    FILE(GLOB FILESYSTEM_SOURCES
        "${MANGO_SOURCE_DIR}/filesystem/*.cpp"
        "${MANGO_SOURCE_DIR}/filesystem/win32/*.cpp"
    )

elseif (EMSCRIPTEN)

    FILE(GLOB CORE_SOURCES
        "${MANGO_SOURCE_DIR}/core/*.cpp"
    )

    FILE(GLOB FILESYSTEM_SOURCES
        "${MANGO_SOURCE_DIR}/filesystem/*.cpp"
        "${MANGO_SOURCE_DIR}/filesystem/unix/*.cpp"
    )

elseif (APPLE)

    FILE(GLOB CORE_SOURCES
        "${MANGO_SOURCE_DIR}/core/*.cpp"
    )

    FILE(GLOB FILESYSTEM_SOURCES
        "${MANGO_SOURCE_DIR}/filesystem/*.cpp"
        "${MANGO_SOURCE_DIR}/filesystem/unix/*.cpp"
    )

else ()

    FILE(GLOB CORE_SOURCES
        "${MANGO_SOURCE_DIR}/core/*.cpp"
    )

    FILE(GLOB FILESYSTEM_SOURCES
        "${MANGO_SOURCE_DIR}/filesystem/*.cpp"
        "${MANGO_SOURCE_DIR}/filesystem/unix/*.cpp"
    )

endif ()

SET(MAIN_HEADER               "${MANGO_INCLUDE_DIR}/mango.hpp")
FILE(GLOB CORE_HEADERS        "${MANGO_INCLUDE_DIR}/core/*.hpp")
FILE(GLOB FILESYSTEM_HEADERS  "${MANGO_INCLUDE_DIR}/filesystem/*.hpp")
FILE(GLOB IMAGE_HEADERS       "${MANGO_INCLUDE_DIR}/image/*.hpp")
FILE(GLOB JPEG_HEADERS        "${MANGO_SOURCE_DIR}/jpeg/*.hpp")
FILE(GLOB MATH_HEADERS        "${MANGO_INCLUDE_DIR}/math/*.hpp")
FILE(GLOB SIMD_HEADERS        "${MANGO_INCLUDE_DIR}/simd/*.hpp")
FILE(GLOB IMPORT3D_HEADERS    "${MANGO_INCLUDE_DIR}/import3d/*.hpp")

FILE(GLOB IMAGE_SOURCES       "${MANGO_SOURCE_DIR}/image/*.cpp")
FILE(GLOB JPEG_SOURCES        "${MANGO_SOURCE_DIR}/jpeg/*.cpp")
FILE(GLOB MATH_SOURCES        "${MANGO_SOURCE_DIR}/math/*.cpp")
FILE(GLOB SIMD_SOURCES        "${MANGO_SOURCE_DIR}/simd/*.cpp")
FILE(GLOB IMPORT3D_SOURCES    "${MANGO_SOURCE_DIR}/import3d/*.cpp")

SOURCE_GROUP("include"              FILES ${MAIN_HEADER})
SOURCE_GROUP("include/core"         FILES ${CORE_HEADERS})
SOURCE_GROUP("include/filesystem"   FILES ${FILESYSTEM_HEADERS})
SOURCE_GROUP("include/image"        FILES ${IMAGE_HEADERS})
SOURCE_GROUP("include/jpeg"         FILES ${JPEG_HEADERS})
SOURCE_GROUP("include/math"         FILES ${MATH_HEADERS})
SOURCE_GROUP("include/simd"         FILES ${SIMD_HEADERS})
SOURCE_GROUP("include/window"       FILES ${WINDOW_HEADERS})
SOURCE_GROUP("include/opengl"       FILES ${OPENGL_HEADERS})
SOURCE_GROUP("include/vulkan"       FILES ${VULKAN_HEADERS})
SOURCE_GROUP("include/import3d"     FILES ${IMPORT3D_HEADERS})

SOURCE_GROUP("source/core"          FILES ${CORE_SOURCES})
SOURCE_GROUP("source/filesystem"    FILES ${FILESYSTEM_SOURCES})
SOURCE_GROUP("source/image"         FILES ${IMAGE_SOURCES})
SOURCE_GROUP("source/jpeg"          FILES ${JPEG_SOURCES})
SOURCE_GROUP("source/math"          FILES ${MATH_SOURCES})
SOURCE_GROUP("source/window"        FILES ${WINDOW_SOURCES})
SOURCE_GROUP("source/opengl"        FILES ${OPENGL_SOURCES})
SOURCE_GROUP("source/vulkan"        FILES ${VULKAN_SOURCES})
SOURCE_GROUP("source/import3d"      FILES ${IMPORT3D_SOURCES})

# ------------------------------------------------------------------------------
# sources: external
# ------------------------------------------------------------------------------

set(EXTERNAL_SOURCE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/source/external")

FILE(GLOB EXTERNAL_AES
    "${EXTERNAL_SOURCE_DIR}/aes/*.h"
    "${EXTERNAL_SOURCE_DIR}/aes/*.cpp"
)

FILE(GLOB EXTERNAL_BC
    "${EXTERNAL_SOURCE_DIR}/bc/*.h"
    "${EXTERNAL_SOURCE_DIR}/bc/*.cpp"
)

FILE(GLOB EXTERNAL_CONCURRENT_QUEUE
    "${EXTERNAL_SOURCE_DIR}/concurrentqueue/*.h"
)

if (LIBRARY_ASTC)
    FILE(GLOB EXTERNAL_ASTC
        "${EXTERNAL_SOURCE_DIR}/astc/*.cpp"
        "${EXTERNAL_SOURCE_DIR}/astc/*.h"
    )
endif ()

FILE(GLOB EXTERNAL_GOOGLE
    "${EXTERNAL_SOURCE_DIR}/google/*.hpp"
    "${EXTERNAL_SOURCE_DIR}/google/*.cpp"
    "${EXTERNAL_SOURCE_DIR}/google/*.h"
)

if (LIBRARY_LZMA)
    FILE(GLOB EXTERNAL_LZMA
        "${EXTERNAL_SOURCE_DIR}/lzma/*.h"
        "${EXTERNAL_SOURCE_DIR}/lzma/*.c"
    )
endif ()

if (LIBRARY_LZFSE)
    FILE(GLOB EXTERNAL_LZFSE
        "${EXTERNAL_SOURCE_DIR}/lzfse/*.h"
        "${EXTERNAL_SOURCE_DIR}/lzfse/*.c"
    )
endif ()

FILE(GLOB EXTERNAL_UNRAR
    "${EXTERNAL_SOURCE_DIR}/unrar/*.hpp"
    "${EXTERNAL_SOURCE_DIR}/unrar/*.cpp"
)

FILE(GLOB EXTERNAL_INFLATELIB
    "${EXTERNAL_SOURCE_DIR}/inflatelib/*.h"
    "${EXTERNAL_SOURCE_DIR}/inflatelib/*.c"
)

FILE(GLOB EXTERNAL_ZPNG
    "${EXTERNAL_SOURCE_DIR}/zpng/*.h"
    "${EXTERNAL_SOURCE_DIR}/zpng/*.cpp"
)

FILE(GLOB EXTERNAL_BASISU
    "${EXTERNAL_SOURCE_DIR}/basisu/transcoder/*.h"
    "${EXTERNAL_SOURCE_DIR}/basisu/transcoder/*.cpp"
)

FILE(GLOB EXTERNAL_IMPORT3D
    "${EXTERNAL_SOURCE_DIR}/mikktspace/*.h"
    "${EXTERNAL_SOURCE_DIR}/mikktspace/*.c"
    "${EXTERNAL_SOURCE_DIR}/fastgltf/include/fastgltf/*.hpp"
    "${EXTERNAL_SOURCE_DIR}/fastgltf/src/*.cpp"
)

SOURCE_GROUP("external/aes"         FILES ${EXTERNAL_AES})
SOURCE_GROUP("external/bc"          FILES ${EXTERNAL_BC})
SOURCE_GROUP("external/concurrent"  FILES ${EXTERNAL_CONCURRENT_QUEUE})
if (LIBRARY_ASTC)
    SOURCE_GROUP("external/astc"        FILES ${EXTERNAL_ASTC})
endif ()
SOURCE_GROUP("external/google"      FILES ${EXTERNAL_GOOGLE})
if (LIBRARY_LZMA)
    SOURCE_GROUP("external/lzma"    FILES ${EXTERNAL_LZMA})
endif ()
if (LIBRARY_LZFSE)
    SOURCE_GROUP("external/lzfse"   FILES ${EXTERNAL_LZFSE})
endif ()
SOURCE_GROUP("external/unrar"       FILES ${EXTERNAL_UNRAR})
SOURCE_GROUP("external/inflatelib"  FILES ${EXTERNAL_INFLATELIB})
SOURCE_GROUP("external/zpng"        FILES ${EXTERNAL_ZPNG})
SOURCE_GROUP("external/basisu"      FILES ${EXTERNAL_BASISU})
SOURCE_GROUP("external/import3d"    FILES ${EXTERNAL_IMPORT3D})

if (NOT IMAGE_FORMAT_RETRO)
    list(REMOVE_ITEM IMAGE_SOURCES
        "${MANGO_SOURCE_DIR}/image/image_atari.cpp"
        "${MANGO_SOURCE_DIR}/image/image_atari8.cpp"
        "${MANGO_SOURCE_DIR}/image/image_c64.cpp"
        "${MANGO_SOURCE_DIR}/image/image_msx.cpp"
        "${MANGO_SOURCE_DIR}/image/image_retro.cpp"
    )
endif ()

if (NOT LIBRARY_ASTC)
    list(REMOVE_ITEM IMAGE_SOURCES
        "${MANGO_SOURCE_DIR}/image/block_astc.cpp"
    )
endif ()

# ------------------------------------------------------------------------------
# libraries
# ------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/cmake/mango_libraries.cmake)

if (LIBRARY_LZMA)
    target_compile_definitions(mango-core PRIVATE "MANGO_ENABLE_LZMA")
endif ()

if (LIBRARY_LZFSE)
    target_compile_definitions(mango-core PRIVATE "MANGO_ENABLE_LZFSE")
endif ()

if (LIBRARY_ASTC)
    target_compile_definitions(mango-image PRIVATE "MANGO_ENABLE_ASTC")
endif ()

if (IMAGE_FORMAT_RETRO)
    target_compile_definitions(mango-image PRIVATE "IMAGE_FORMAT_RETRO")
endif ()

# ------------------------------------------------------------------------------
# find_module()
# ------------------------------------------------------------------------------

function (resolve_version OutVar Value)
    if ("${Value}" STREQUAL "")
        set(${OutVar} "no version" PARENT_SCOPE)
    else ()
        set(${OutVar} "${Value}" PARENT_SCOPE)
    endif ()
endfunction()

function (check_required Prefix PackageName)
    if (Mode STREQUAL "REQUIRED")
        message(FATAL_ERROR "Failed to locate REQUIRED package: " ${Prefix})
    else ()
        resolve_version(package_version "${${PackageName}_VERSION}")
        message("    " ${Prefix} ": " ${package_version} " [not found]")
    endif ()
endfunction()

function (apply_cmake_target Target Interface Prefix PackageName ImportedTarget)
    resolve_version(package_version "${${PackageName}_VERSION}")
    message("    " ${Prefix} ": " ${package_version} " [cmake]")

    target_compile_definitions(${Target} PRIVATE "MANGO_ENABLE_${Prefix}")
    target_link_libraries(${Target} ${Interface} ${ImportedTarget})
endfunction()

function (apply_pkgconfig_target Target Interface Prefix)
    resolve_version(package_version "${${Prefix}_VERSION}")
    message("    " ${Prefix} ": " ${package_version} " [pkgconfig]")

    target_compile_definitions(${Target} PRIVATE "MANGO_ENABLE_${Prefix}")
    target_include_directories(${Target} ${Interface} ${${Prefix}_INCLUDE_DIRS})
    target_link_directories(${Target} ${Interface} ${${Prefix}_LIBRARY_DIRS})
    target_link_libraries(${Target} ${Interface} ${${Prefix}_LIBRARIES})
endfunction()

function (apply_manual_target Target Interface Prefix)
    resolve_version(package_version "no version")
    message("    " ${Prefix} ": " ${package_version} " [manual]")

    target_compile_definitions(${Target} PRIVATE "MANGO_ENABLE_${Prefix}")
    target_include_directories(${Target} ${Interface} ${${Prefix}_INCLUDE_DIRS})
    target_link_directories(${Target} ${Interface} ${${Prefix}_LIBRARY_DIRS})
    target_link_libraries(${Target} ${Interface} ${${Prefix}_LIBRARIES})
endfunction()

function (check_manual_override Prefix Result)
    if (DEFINED ${Prefix}_LIBRARIES AND DEFINED ${Prefix}_INCLUDE_DIRS)
        set(${Result} TRUE PARENT_SCOPE)
    else ()
        set(${Result} FALSE PARENT_SCOPE)
    endif ()
endfunction()

# ------------------------------------------------------------------------------
# find_module
# ------------------------------------------------------------------------------

function (find_module Target Prefix Interface Mode ModuleName PackageName ImportedTarget Version)

    # find_package

    if (NOT PackageName STREQUAL "")
        if (Version STREQUAL "0")
            find_package(${PackageName} QUIET)
        else ()
            find_package(${PackageName} ${Version} QUIET)
        endif ()

        if (${PackageName}_FOUND)

            # Fix imported targets

            if (${Prefix} STREQUAL "LZ4")
                # Debian 13 on Raspberry Pi 5 uses 'LZ4::lz4'
                if (TARGET LZ4::lz4 AND NOT TARGET lz4::lz4)
                    add_library(lz4::lz4 ALIAS LZ4::lz4)
                endif ()
            endif ()

            if (${Prefix} STREQUAL "ZSTD")
                if (TARGET zstd::libzstd)
                    # brew / vcpkg imported target is 'zstd::libzstd'
                else ()
                    # apt / pacman don't correctly export target but this works
                    set(ImportedTarget "zstd")
                endif ()
            endif ()

            if (NOT ImportedTarget STREQUAL "")
                apply_cmake_target(${Target} ${Interface} ${Prefix} ${PackageName} ${ImportedTarget})
                return ()
            endif ()

            # FindJXR.cmake (vcpkg jxrlib) exposes JXR_INCLUDE_DIRS / JXR_LIBRARIES only
            if (DEFINED ${Prefix}_INCLUDE_DIRS AND DEFINED ${Prefix}_LIBRARIES)
                apply_manual_target(${Target} ${Interface} ${Prefix})
                return ()
            endif ()
        endif ()
    endif ()

    # pkg_check_modules

    if (PkgConfig_FOUND AND NOT ModuleName STREQUAL "")
        if (Version STREQUAL "0")
            pkg_check_modules(${Prefix} QUIET ${ModuleName})
        else ()
            pkg_check_modules(${Prefix} QUIET ${ModuleName}>=${Version})
        endif ()

        if (${Prefix}_FOUND)
            apply_pkgconfig_target(${Target} ${Interface} ${Prefix})
            return ()
        endif ()
    endif ()

    # manual override

    check_manual_override(${Prefix} MANUAL_OVERRIDE_FOUND)
    if (MANUAL_OVERRIDE_FOUND)
        apply_manual_target(${Target} ${Interface} ${Prefix})
        return ()
    endif ()

    # Arch Linux jxrlib ships headers and libs but no pkg-config or cmake config
    if (${Prefix} STREQUAL "JXR")
        find_path(JXR_INCLUDE_DIRS NAMES JXRGlue.h PATH_SUFFIXES jxrlib)
        find_library(JXR_JPEGXR_LIBRARY NAMES jpegxr)
        find_library(JXR_JXRGLUE_LIBRARY NAMES jxrglue)

        if (JXR_INCLUDE_DIRS AND JXR_JPEGXR_LIBRARY AND JXR_JXRGLUE_LIBRARY)
            set(JXR_LIBRARIES ${JXR_JXRGLUE_LIBRARY} ${JXR_JPEGXR_LIBRARY})
            apply_manual_target(${Target} ${Interface} ${Prefix})
            return ()
        endif ()
    endif ()

    if (NOT PackageName STREQUAL "")
        check_required(${Prefix} ${PackageName})
    endif ()

endfunction()

# ------------------------------------------------------------------------------
# find_libdeflate
# ------------------------------------------------------------------------------

function (find_libdeflate Target Interface Mode)
    if (BUILD_SHARED_LIBS)
        # For shared builds, prefer shared libraries
        set(TARGET_NAMES
            "libdeflate::libdeflate_shared"    # Standard CMake target (Linux/macOS)
            "libdeflate"                       # VCPKG target name (usually shared)
            "libdeflate::libdeflate"           # Alternative VCPKG target
            "libdeflate::libdeflate_static"    # Fallback to static if shared not available
        )
    else ()
        # For static builds, prefer static libraries
        set(TARGET_NAMES
            "libdeflate::libdeflate_static"    # Standard CMake target (Linux/macOS)
            "libdeflate"                       # VCPKG target name (usually static)
            "libdeflate::libdeflate"           # Alternative VCPKG target
            "libdeflate::libdeflate_shared"    # Fallback to shared if static not available
        )
    endif ()

    # find_package

    find_package(libdeflate QUIET)
    if (libdeflate_FOUND)
        foreach(ImportedTarget ${TARGET_NAMES})
            if (TARGET ${ImportedTarget})
                apply_cmake_target(${Target} ${Interface} "LIBDEFLATE" "libdeflate" ${ImportedTarget})
                return()
            endif ()
        endforeach ()
    endif ()

    # pkg_check_modules

    if (PkgConfig_FOUND)
        pkg_check_modules(LIBDEFLATE QUIET libdeflate)
        if (LIBDEFLATE_FOUND)
            apply_pkgconfig_target(${Target} ${Interface} LIBDEFLATE)
            return()
        endif ()
    endif ()

    # manual override

    check_manual_override("LIBDEFLATE" MANUAL_OVERRIDE_FOUND)
    if (MANUAL_OVERRIDE_FOUND)
        apply_manual_target(${Target} ${Interface} "LIBDEFLATE")
        return()
    endif ()

    check_required("LIBDEFLATE" "libdeflate")

endfunction()

# ------------------------------------------------------------------------------
# Required Libraries
# ------------------------------------------------------------------------------

find_package(PkgConfig QUIET)

message("[Required Libraries: core]")

find_module(mango-core ZLIB ${INTERFACE} REQUIRED zlib ZLIB ZLIB::ZLIB 0)
find_module(mango-core ZSTD ${INTERFACE} REQUIRED libzstd zstd zstd::libzstd 0)
find_module(mango-core FMT PUBLIC REQUIRED "" fmt fmt::fmt 8.0)
find_libdeflate(mango-core ${INTERFACE} REQUIRED)

message("[Required Libraries: image]")

find_module(mango-image ZLIB ${INTERFACE} REQUIRED zlib ZLIB ZLIB::ZLIB 0)
find_module(mango-image ZSTD ${INTERFACE} REQUIRED libzstd zstd zstd::libzstd 0)
find_module(mango-image LCMS2 PRIVATE REQUIRED lcms2 lcms2 lcms2::lcms2 0)

if (BUILD_IMPORT3D)
    message("[Required Libraries: import3d]")
    find_module(mango-import3d SIMDJSON ${INTERFACE} REQUIRED simdjson simdjson simdjson::simdjson 0)
endif ()

# ------------------------------------------------------------------------------
# Optional Libraries
# ------------------------------------------------------------------------------

message("[Optional Libraries: core]")

if (LIBRARY_BZIP2)
    find_module(mango-core BZIP2 ${INTERFACE} OPTIONAL "" BZip2 BZip2::BZip2 0)
endif ()

if (LIBRARY_LZ4)
    find_module(mango-core LZ4 ${INTERFACE} OPTIONAL liblz4 lz4 lz4::lz4 0)
endif ()

if (LIBRARY_ISAL)
    find_module(mango-core ISAL ${INTERFACE} OPTIONAL libisal unofficial-isal unofficial::isal::isal 0)
endif ()

message("[Optional Libraries: image]")

if (LIBRARY_ISAL)
    find_module(mango-image ISAL ${INTERFACE} OPTIONAL libisal unofficial-isal unofficial::isal::isal 0)
endif ()

if (LIBRARY_JXR)
    find_module(mango-image JXR ${INTERFACE} OPTIONAL jxrlib JXR "" 0)
endif ()

if (LIBRARY_JXL)
    find_module(mango-image JXL ${INTERFACE} OPTIONAL libjxl "" "" 0.7)
    find_module(mango-image JXL_THREADS ${INTERFACE} OPTIONAL libjxl_threads "" "" 0.7)
endif ()

if (LIBRARY_JP2)
    find_module(mango-image JP2 ${INTERFACE} OPTIONAL libopenjp2 OpenJPEG openjp2 2.4.0)
endif ()

if (LIBRARY_WEBP)
    find_module(mango-image WEBP ${INTERFACE} OPTIONAL libwebp WebP WebP::webp 1.3.2)
    if (TARGET WebP::webpdemux)
        target_link_libraries(mango-image ${INTERFACE} WebP::webpdemux)
    endif ()
endif ()

if (LIBRARY_WP2)
    find_module(mango-image WP2 ${INTERFACE} OPTIONAL wp2 wp2 webp2 0)
endif ()

if (LIBRARY_AVIF)
    find_module(mango-image AVIF ${INTERFACE} OPTIONAL libavif libavif avif 1.0.0)
endif ()

if (LIBRARY_HEIF)
    find_module(mango-image HEIF ${INTERFACE} OPTIONAL libheif libheif heif 1.13.0)
endif ()

if (LIBRARY_LIBRAW)
    # LibRaw ships only a pkg-config file (no CMake package config). Its .pc pulls in
    # bare link tokens (-lstdc++, OpenMP, lcms2 via Requires); adding those as plain
    # library names defeats CMake's link de-duplication and ends up duplicating every
    # other pkg-config library on the link line. Linking through an imported target
    # keeps those flags encapsulated and the link line clean.
    if (PkgConfig_FOUND)
        pkg_check_modules(LIBRAW QUIET IMPORTED_TARGET libraw>=0.20.0)
        if (LIBRAW_FOUND)
            resolve_version(package_version "${LIBRAW_VERSION}")
            message("    LIBRAW: " ${package_version} " [pkgconfig]")

            # LibRaw's .pc lists an explicit C++ runtime (-lstdc++). We always link
            # through the C++ compiler driver, which provides it implicitly, so drop
            # it to avoid a duplicate-library warning (it aliases libc++ on macOS).
            if (TARGET PkgConfig::LIBRAW)
                get_target_property(_libraw_libs PkgConfig::LIBRAW INTERFACE_LINK_LIBRARIES)
                if (_libraw_libs)
                    list(REMOVE_ITEM _libraw_libs "stdc++")
                    set_target_properties(PkgConfig::LIBRAW PROPERTIES INTERFACE_LINK_LIBRARIES "${_libraw_libs}")
                endif ()
            endif ()

            target_compile_definitions(mango-image PRIVATE "MANGO_ENABLE_LIBRAW")
            target_link_libraries(mango-image ${INTERFACE} PkgConfig::LIBRAW)
        else ()
            message("    LIBRAW: no version [not found]")
        endif ()
    endif ()
endif ()

# ------------------------------------------------------------------------------
# configuration
# ------------------------------------------------------------------------------

set(SIMD "NONE")

if (BUILD_SHARED_LIBS)
    # MANGO_SHARED / MANGO_API_EXPORT are set per-library in cmake/mango_libraries.cmake
endif ()

if (WIN32)
    # UNICODE is set per-library in cmake/mango_libraries.cmake
endif ()

if ((ARM AND ENABLE_NEON) OR ARM64)
    set(SIMD "NEON")
endif ()

if (EMSCRIPTEN)

    set(SIMD "WASM")
    target_compile_options(mango-core PUBLIC "-msimd128")

elseif (MSVC)

    target_compile_options(mango-core PUBLIC $<$<CONFIG:Release>:/Ox>)
    target_compile_options(mango-core PUBLIC "/utf-8")
    target_compile_options(mango-core PUBLIC "/MP")

    if (X86 OR X86_64)

        if (ENABLE_PCLMUL)
            target_compile_definitions(mango-core PUBLIC "__PCLMUL__")
        endif ()

        if (ENABLE_POPCNT)
            target_compile_definitions(mango-core PUBLIC "__POPCNT__")
        endif ()

        if (ENABLE_AES)
            target_compile_definitions(mango-core PUBLIC "__AES__")
        endif ()

        if (ENABLE_F16C)
            target_compile_definitions(mango-core PUBLIC "__F16C__")
        endif ()

        if (ENABLE_LZCNT)
            target_compile_definitions(mango-core PUBLIC "__LZCNT__")
        endif ()

        if (ENABLE_BMI)
            target_compile_definitions(mango-core PUBLIC "__BMI__")
        endif ()

        if (ENABLE_BMI2)
            target_compile_definitions(mango-core PUBLIC "__BMI2__")
        endif ()

        if (ENABLE_FMA)
            target_compile_definitions(mango-core PUBLIC "__FMA__")
        endif ()

        if (ENABLE_SHA)
            target_compile_definitions(mango-core PUBLIC "__SHA__")
        endif ()

        # enable only one (the most recent) SIMD extension
        if (ENABLE_AVX512)
            set(SIMD "AVX-512 (2015)")
            target_compile_options(mango-core PUBLIC "/arch:AVX512")
            target_compile_definitions(mango-core PUBLIC "__AVX512F__" "__AVX512DQ__")
        elseif (ENABLE_AVX2)
            set(SIMD "AVX2 (2013)")
            target_compile_options(mango-core PUBLIC "/arch:AVX2")
        elseif (ENABLE_AVX)
            set(SIMD "AVX (2008)")
            target_compile_options(mango-core PUBLIC "/arch:AVX")
        elseif (ENABLE_SSE2 OR ENABLE_SSE4)
            # MSVC does not have SSE4 option; at least enable SSE2
            set(SIMD "SSE2 (2001)")
            if (NOT X86_64)
                # MSVC enables SSE2 by default on X86_64
                target_compile_options(mango-core PUBLIC "/arch:SSE2")
            endif ()
        endif ()

    endif ()

else ()

    add_compile_options(-Wall)
    target_compile_options(mango-core PRIVATE $<$<CONFIG:Release>:-O3>)

    if (CYGWIN)
        target_compile_options(mango-core PUBLIC "-municode")
    endif ()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(mango-core PUBLIC "-ftree-vectorize")
        set(CMAKE_CXX_FLAGS "-Wno-psabi")
    endif ()

    if (ARM AND (NOT APPLE))
        # The compiler for Apple Silicon does not recognize these
        add_definitions (-mfpu=neon -mfloat-abi=hard)
    endif ()

    if (X86 OR X86_64)

        if (ENABLE_PCLMUL)
            target_compile_options(mango-core PUBLIC "-mpclmul")
        endif ()

        if (ENABLE_POPCNT)
            target_compile_options(mango-core PUBLIC "-mpopcnt")
        endif ()

        if (ENABLE_AES)
            target_compile_options(mango-core PUBLIC "-maes")
        endif ()

        if (ENABLE_F16C)
            target_compile_options(mango-core PUBLIC "-mf16c")
        endif ()

        if (ENABLE_LZCNT)
            target_compile_options(mango-core PUBLIC "-mlzcnt")
        endif ()

        if (ENABLE_BMI)
            target_compile_options(mango-core PUBLIC "-mbmi")
        endif ()

        if (ENABLE_BMI2)
            target_compile_options(mango-core PUBLIC "-mbmi2")
        endif ()

        if (ENABLE_FMA)
            target_compile_options(mango-core PUBLIC "-mfma")
        endif ()

        if (ENABLE_SHA)
            target_compile_options(mango-core PUBLIC "-msha")
        endif ()

        # enable only one (the most recent) SIMD extension
        if (ENABLE_AVX512)
            set(SIMD "AVX-512 (2015)")
            target_compile_options(mango-core PUBLIC "-mavx512dq")
            target_compile_options(mango-core PUBLIC "-mavx512vl")
            target_compile_options(mango-core PUBLIC "-mavx512bw")
        elseif (ENABLE_AVX2)
            set(SIMD "AVX2 (2013)")
            target_compile_options(mango-core PUBLIC "-mavx2")
        elseif (ENABLE_AVX)
            set(SIMD "AVX (2008)")
            target_compile_options(mango-core PUBLIC "-mavx")
        elseif (ENABLE_SSE4)
            set(SIMD "SSE4.2 (2006)")
            target_compile_options(mango-core PUBLIC "-msse4")
        elseif (ENABLE_SSE2)
            set(SIMD "SSE2 (2001)")
            target_compile_options(mango-core PUBLIC "-msse2")
        endif ()

    endif ()
endif ()

# ------------------------------------------------------------------------------
# ISA Extensions
# ------------------------------------------------------------------------------

message("[ISA Extensions]")

if (X86 OR X86_64)

    if (ENABLE_PCLMUL)
        message("    PCLMUL (2008)")
    endif ()

    if (ENABLE_POPCNT)
        message("    POPCNT (2008)")
    endif ()

    if (ENABLE_AES)
        message("    AES (2010)")
    endif ()

    if (ENABLE_F16C)
        message("    F16C (2012)")
    endif ()

    if (ENABLE_LZCNT)
        message("    LZCNT (2013)")
    endif ()

    if (ENABLE_BMI)
        message("    BMI (2013)")
    endif ()

    if (ENABLE_BMI2)
        message("    BMI2 (2013)")
    endif ()

    if (ENABLE_FMA)
        message("    FMA (2013)")
    endif ()

    if (ENABLE_SHA)
        message("    SHA (2013)")
    endif ()

endif()

message("    SIMD: " ${SIMD})

# ------------------------------------------------------------------------------
# Graphics Libraries
# ------------------------------------------------------------------------------

if (BUILD_MANGO_WINDOW AND NOT EMSCRIPTEN)
    if (WIN32 OR CYGWIN)
        set(MANGO_VULKAN_WINDOW_SYSTEMS "Win32")
        set(MANGO_OPENGL_WINDOW_SYSTEMS "WGL")
    elseif (APPLE)
        set(MANGO_VULKAN_WINDOW_SYSTEMS "MoltenVK (Metal)")
        set(MANGO_OPENGL_WINDOW_SYSTEMS "Cocoa")
    else ()
        set(_mango_vk_ws "")
        set(_mango_gl_ws "")
        if (MANGO_HAS_WAYLAND_WINDOW)
            list(APPEND _mango_vk_ws "Wayland")
            list(APPEND _mango_gl_ws "Wayland (EGL)")
        endif ()
        if (MANGO_HAS_XCB_WINDOW)
            list(APPEND _mango_vk_ws "Xcb")
            list(APPEND _mango_gl_ws "Xcb (GLX)")
        endif ()
        if (MANGO_HAS_XLIB_WINDOW)
            list(APPEND _mango_vk_ws "Xlib")
            list(APPEND _mango_gl_ws "Xlib (GLX)")
        endif ()
        if (_mango_vk_ws)
            list(JOIN _mango_vk_ws ", " MANGO_VULKAN_WINDOW_SYSTEMS)
        endif ()
        if (_mango_gl_ws)
            list(JOIN _mango_gl_ws ", " MANGO_OPENGL_WINDOW_SYSTEMS)
        endif ()
    endif ()

    if (BUILD_VULKAN OR BUILD_OPENGL)
        message("[Graphics]")
    endif ()

    if (BUILD_VULKAN)
        if (MANGO_VULKAN_WINDOW_SYSTEMS)
            message("    Vulkan: " ${MANGO_VULKAN_WINDOW_SYSTEMS})
        else ()
            message("    Vulkan: (no window backends)")
        endif ()
    endif ()
    if (BUILD_OPENGL)
        if (MANGO_OPENGL_WINDOW_SYSTEMS)
            message("    OpenGL: " ${MANGO_OPENGL_WINDOW_SYSTEMS})
        else ()
            message("    OpenGL: (no window backends)")
        endif ()
    endif ()
endif ()

# ------------------------------------------------------------------------------
# Build Options
# ------------------------------------------------------------------------------

message("[Build Options]")
message("    BUILD_OPENGL:    " ${BUILD_OPENGL})
message("    BUILD_VULKAN:    " ${BUILD_VULKAN})
message("    BUILD_IMPORT3D:  " ${BUILD_IMPORT3D})
message("    BUILD_EXAMPLES:  " ${BUILD_EXAMPLES})

# ------------------------------------------------------------------------------
# install
# ------------------------------------------------------------------------------

include(GNUInstallDirs)

# install mango headers
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h*"
)

set(MANGO_INSTALL_BINDIR "$<IF:$<CONFIG:Debug>,debug/${CMAKE_INSTALL_BINDIR},${CMAKE_INSTALL_BINDIR}>")
set(MANGO_INSTALL_LIBDIR "$<IF:$<CONFIG:Debug>,debug/${CMAKE_INSTALL_LIBDIR},${CMAKE_INSTALL_LIBDIR}>")

install(TARGETS ${MANGO_LIBRARY_TARGETS}
    EXPORT mango-config
    RUNTIME DESTINATION ${MANGO_INSTALL_BINDIR}
    LIBRARY DESTINATION ${MANGO_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${MANGO_INSTALL_LIBDIR}
)

# install mango pdb files (only for windows)
if (WIN32 AND TARGET_PDB_FILE)
    install(FILES $<TARGET_PDB_FILE:mango>
        DESTINATION $<IF:$<CONFIG:Debug>,debug/bin,bin> OPTIONAL
    )
endif ()

# install mango cmake config
install(
    EXPORT "mango-config"
    NAMESPACE mango::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mango
    FILE "mango-targets.cmake"
)

# configure and install config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/mango-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/mango-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mango
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/mango-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/mango-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/mango-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mango
)

# ------------------------------------------------------------------------------
# examples
# ------------------------------------------------------------------------------

list(POP_BACK CMAKE_MESSAGE_INDENT)

if (BUILD_EXAMPLES)

    add_subdirectory(examples)

    if (WIN32 AND BUILD_SHARED_LIBS)
        foreach (x ${EXAMPLE_BINARY_FOLDERS})

            get_filename_component(folder_name ${x} NAME)
            set(destination_folder ${x}/${CMAKE_CFG_INTDIR})

            foreach (lib ${MANGO_LIBRARY_TARGETS})
                add_custom_target(copy_${lib}_to_${folder_name} ALL
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${destination_folder}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${lib}>" "${destination_folder}"
                    DEPENDS ${lib}
                )
            endforeach ()

            get_directory_property(example_targets DIRECTORY ${x} BUILDSYSTEM_TARGETS)
            foreach (target ${example_targets})
                if (NOT TARGET ${target})
                    continue()
                endif ()
                get_target_property(target_type ${target} TYPE)
                if (target_type STREQUAL "EXECUTABLE")
                    foreach (lib ${MANGO_LIBRARY_TARGETS})
                        add_dependencies(${target} copy_${lib}_to_${folder_name})
                    endforeach ()
                endif ()
            endforeach ()
        endforeach ()
    endif ()

endif ()

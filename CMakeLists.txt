cmake_minimum_required(VERSION 3.19)
project(mango
        VERSION 3.3.0
        DESCRIPTION "C++ Graphics Library for the NÖRDS"
        LANGUAGES CXX C)

if (CMAKE_GENERATOR MATCHES "Visual Studio")
    if (EXISTS "${CMAKE_BINARY_DIR}/build.ninja"
        OR EXISTS "${CMAKE_BINARY_DIR}/.ninja_log"
        OR EXISTS "${CMAKE_BINARY_DIR}/.ninja_deps")
        message(WARNING
            "Ninja build artifacts found in a Visual Studio build directory.\n"
            "    Use build-ninja/ for Ninja (cmake --preset windows-ninja-release) and build/ for VS.\n"
            "    Remove build.ninja, .ninja_log, and .ninja_deps from this folder, or wipe CMakeCache.txt.")
    endif ()
elseif (CMAKE_GENERATOR MATCHES "Ninja")
    if (EXISTS "${CMAKE_BINARY_DIR}/mango.sln")
        message(WARNING
            "Visual Studio build artifacts found in a Ninja build directory.\n"
            "    Use build-ninja/ for Ninja and build/ for Visual Studio to avoid reconfigure loops.")
    endif ()
endif ()

# ------------------------------------------------------------------------------
# options
# ------------------------------------------------------------------------------

include(CMakeDependentOption)

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

OPTION(LIBRARY_ASTC         "Enable ASTC block compression support"     ON)

# Vulkan FontRenderer: FreeType (+ HarfBuzz) for deluxe text; stb_truetype fallback when OFF.
OPTION(LIBRARY_FREETYPE     "Use FreeType for Vulkan FontRenderer (OFF = stb_truetype even if installed)" ON)
cmake_dependent_option(LIBRARY_HARFBUZZ "Use HarfBuzz text shaping (OFF even if installed; requires FreeType)" ON "LIBRARY_FREETYPE" OFF)

OPTION(IMAGE_FORMAT_IFF     "Enable IFF image format support"           ON)
OPTION(IMAGE_FORMAT_EXR     "Enable OpenEXR image format support"       ON)
OPTION(IMAGE_FORMAT_TIFF    "Enable TIFF image format support"          ON)
OPTION(IMAGE_FORMAT_PSD     "Enable Photoshop PSD image format support" ON)
OPTION(IMAGE_FORMAT_JXR     "Enable JPEG XR image format support"       ON)
OPTION(IMAGE_FORMAT_JXL     "Enable JPEG-XL image format support"       ON)
OPTION(IMAGE_FORMAT_JP2     "Enable JPEG2000 image format support"      ON)
OPTION(IMAGE_FORMAT_WEBP    "Enable WebP image format support"          ON)
OPTION(IMAGE_FORMAT_WP2     "Enable WebP 2 image format support"        ON)
OPTION(IMAGE_FORMAT_AVIF    "Enable AVIF image format support"          ON)
OPTION(IMAGE_FORMAT_HEIF    "Enable HEIF image format support"          ON)
OPTION(IMAGE_FORMAT_CAMERA  "Enable camera raw image format (LibRaw)"   ON)
OPTION(IMAGE_FORMAT_RETRO   "Enable retro formats: Atari,C64,MSX, ..."  ON)
OPTION(IMAGE_FORMAT_KTX2    "Enable KTX2 image format support"          ON)

OPTION(ENABLE_NEON          "Enable ARM NEON instructions"              ON)
OPTION(ENABLE_SSE2          "Enable SSE2 instructions"                 OFF)
OPTION(ENABLE_SSE4          "Enable SSE4.1 and SSE4.2 instructions"     ON)
OPTION(ENABLE_AVX           "Enable AVX instructions"                  OFF)
OPTION(ENABLE_AVX2          "Enable AVX2 instructions"                 OFF)
OPTION(ENABLE_AVX512        "Enable AVX-512 instructions"              OFF)

OPTION(ENABLE_PCLMUL        "Enable PCLMUL"                            OFF)
OPTION(ENABLE_POPCNT        "Enable POPCNT"                            OFF)
OPTION(ENABLE_AES           "Enable AES"                               OFF)
OPTION(ENABLE_F16C          "Enable F16C conversion instructions"      OFF)
OPTION(ENABLE_LZCNT         "Enable LZCNT"                             OFF)
OPTION(ENABLE_BMI           "Enable BMI"                               OFF)
OPTION(ENABLE_BMI2          "Enable BMI2"                              OFF)
OPTION(ENABLE_FMA           "Enable FMA"                               OFF)
OPTION(ENABLE_SHA           "Enable SHA"                               OFF)

# Option to disable every optional library
option(LIBRARY_DISABLE,     "Disable ALL optional libraries"           OFF)
if (LIBRARY_DISABLE)
    set(IMAGE_FORMAT_IFF    OFF)
    set(IMAGE_FORMAT_EXR    OFF)
    set(IMAGE_FORMAT_TIFF   OFF)
    set(IMAGE_FORMAT_PSD    OFF)
    set(IMAGE_FORMAT_JXR    OFF)
    set(IMAGE_FORMAT_JXL    OFF)
    set(IMAGE_FORMAT_JP2    OFF)
    set(IMAGE_FORMAT_WEBP   OFF)
    set(IMAGE_FORMAT_WP2    OFF)
    set(IMAGE_FORMAT_AVIF   OFF)
    set(IMAGE_FORMAT_HEIF   OFF)
    set(IMAGE_FORMAT_CAMERA OFF)
    set(IMAGE_FORMAT_RETRO  OFF)
    set(IMAGE_FORMAT_KTX2   OFF)
    set(LIBRARY_BZIP2       OFF)
    set(LIBRARY_LZ4         OFF)
    set(LIBRARY_LZMA        OFF)
    set(LIBRARY_LZFSE       OFF)
    set(LIBRARY_ISAL        OFF)
    set(LIBRARY_FREETYPE    OFF)
    set(LIBRARY_HARFBUZZ    OFF)
endif ()

# Option to build everything
option(BUILD_ALL,           "Build everything"                         OFF)
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
option(INTEL_DELUXE         "Enable ALL Intel ISA extensions"          OFF)
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
    # VS generator sets /EHsc by default; Ninja + MSVC does not.
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/EHsc>")
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

include(${CMAKE_CURRENT_LIST_DIR}/cmake/mango_find_module.cmake)

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
            if (WAYLAND_PROTOCOLS_FOUND)
                pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
            endif ()
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
            "${MANGO_SOURCE_DIR}/vulkan/font/*.cpp"
        )

    elseif (NOT EMSCRIPTEN)

        FILE(GLOB VULKAN_SOURCES
            "${MANGO_SOURCE_DIR}/vulkan/*.cpp"
            "${MANGO_SOURCE_DIR}/vulkan/font/*.cpp"
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

if (IMAGE_FORMAT_KTX2)
    FILE(GLOB EXTERNAL_BASISU
        "${EXTERNAL_SOURCE_DIR}/basisu/transcoder/*.h"
        "${EXTERNAL_SOURCE_DIR}/basisu/transcoder/*.cpp"
    )
endif ()

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
if (IMAGE_FORMAT_KTX2)
    SOURCE_GROUP("external/basisu"      FILES ${EXTERNAL_BASISU})
endif ()
SOURCE_GROUP("external/import3d"    FILES ${EXTERNAL_IMPORT3D})

mango_probe_image_format_dependencies()
mango_probe_font_dependencies()

if (MANGO_HAS_FREETYPE)
    list(REMOVE_ITEM VULKAN_SOURCES "${MANGO_SOURCE_DIR}/vulkan/font/face_stb.cpp")
else ()
    list(REMOVE_ITEM VULKAN_SOURCES "${MANGO_SOURCE_DIR}/vulkan/font/face_freetype.cpp")
endif ()

if (NOT IMAGE_FORMAT_RETRO)
    list(REMOVE_ITEM IMAGE_SOURCES
        "${MANGO_SOURCE_DIR}/image/image_atari.cpp"
        "${MANGO_SOURCE_DIR}/image/image_atari8.cpp"
        "${MANGO_SOURCE_DIR}/image/image_c64.cpp"
        "${MANGO_SOURCE_DIR}/image/image_msx.cpp"
        "${MANGO_SOURCE_DIR}/image/image_pict.cpp"
        "${MANGO_SOURCE_DIR}/image/image_retro.cpp"
    )
endif ()

if (NOT LIBRARY_ASTC)
    list(REMOVE_ITEM IMAGE_SOURCES
        "${MANGO_SOURCE_DIR}/image/block_astc.cpp"
    )
endif ()

if (NOT IMAGE_FORMAT_KTX2)
    list(REMOVE_ITEM IMAGE_SOURCES
        "${MANGO_SOURCE_DIR}/image/image_ktx2.cpp"
    )
endif ()

if (NOT IMAGE_FORMAT_IFF)
    list(REMOVE_ITEM IMAGE_SOURCES "${MANGO_SOURCE_DIR}/image/image_iff.cpp")
endif ()

if (NOT IMAGE_FORMAT_EXR)
    list(REMOVE_ITEM IMAGE_SOURCES "${MANGO_SOURCE_DIR}/image/image_exr.cpp")
endif ()

if (NOT IMAGE_FORMAT_TIFF)
    list(REMOVE_ITEM IMAGE_SOURCES
        "${MANGO_SOURCE_DIR}/image/image_tiff.cpp"
        "${MANGO_SOURCE_DIR}/image/ccitt_fax_decode.cpp"
    )
endif ()

if (NOT IMAGE_FORMAT_PSD)
    list(REMOVE_ITEM IMAGE_SOURCES "${MANGO_SOURCE_DIR}/image/image_psd.cpp")
endif ()

if (NOT IMAGE_FORMAT_JXR)
    list(REMOVE_ITEM IMAGE_SOURCES "${MANGO_SOURCE_DIR}/image/image_jxr.cpp")
endif ()

if (NOT IMAGE_FORMAT_JXL)
    list(REMOVE_ITEM IMAGE_SOURCES "${MANGO_SOURCE_DIR}/image/image_jxl.cpp")
endif ()

if (NOT IMAGE_FORMAT_JP2)
    list(REMOVE_ITEM IMAGE_SOURCES "${MANGO_SOURCE_DIR}/image/image_jp2.cpp")
endif ()

if (NOT IMAGE_FORMAT_WEBP)
    list(REMOVE_ITEM IMAGE_SOURCES "${MANGO_SOURCE_DIR}/image/image_webp.cpp")
endif ()

if (NOT IMAGE_FORMAT_WP2)
    list(REMOVE_ITEM IMAGE_SOURCES "${MANGO_SOURCE_DIR}/image/image_wp2.cpp")
endif ()

if (NOT IMAGE_FORMAT_AVIF)
    list(REMOVE_ITEM IMAGE_SOURCES "${MANGO_SOURCE_DIR}/image/image_avif.cpp")
endif ()

if (NOT IMAGE_FORMAT_HEIF)
    list(REMOVE_ITEM IMAGE_SOURCES "${MANGO_SOURCE_DIR}/image/image_heif.cpp")
endif ()

if (NOT IMAGE_FORMAT_CAMERA)
    list(REMOVE_ITEM IMAGE_SOURCES "${MANGO_SOURCE_DIR}/image/image_camera.cpp")
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

foreach (_image_format IN ITEMS IFF EXR TIFF PSD RETRO KTX2 JXR JXL JP2 WEBP WP2 AVIF HEIF CAMERA)
    if (IMAGE_FORMAT_${_image_format})
        target_compile_definitions(mango-image PRIVATE "IMAGE_FORMAT_${_image_format}")
    endif ()
endforeach ()

mango_link_libraries()

# ------------------------------------------------------------------------------
# configuration
# ------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/cmake/mango_simd.cmake)
mango_configure_isa()

# ------------------------------------------------------------------------------
# Build Options
# ------------------------------------------------------------------------------

message("[Build Options]")
message("    BUILD_OPENGL:    " ${BUILD_OPENGL})
message("    BUILD_VULKAN:    " ${BUILD_VULKAN})
message("    BUILD_IMPORT3D:  " ${BUILD_IMPORT3D})
message("    BUILD_EXAMPLES:  " ${BUILD_EXAMPLES})

list(POP_BACK CMAKE_MESSAGE_INDENT)

# ------------------------------------------------------------------------------
# details
# ------------------------------------------------------------------------------

if (BUILD_VULKAN)
    message("[Vulkan]")
    if (MANGO_HAS_FREETYPE)
        message("    FontRenderer:    FreeType")
    else ()
        message("    FontRenderer:    stb_truetype")
    endif ()
    if (MANGO_HAS_HARFBUZZ)
        message("    Text shaping:    HarfBuzz")
    elseif (LIBRARY_HARFBUZZ AND MANGO_HAS_FREETYPE)
        message("    Text shaping:    disabled (not found)")
    elseif (LIBRARY_FREETYPE AND MANGO_HAS_FREETYPE)
        message("    Text shaping:    disabled (LIBRARY_HARFBUZZ=OFF)")
    endif ()
endif ()

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

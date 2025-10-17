cmake_minimum_required(VERSION 3.19)
project(mango
        VERSION 2.4.0
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

# Window system selection
set(WINDOW_SYSTEM "xlib" CACHE STRING "Window system to use (xlib, xcb, wayland)")
set_property(CACHE WINDOW_SYSTEM PROPERTY STRINGS xlib xcb wayland)

# Select EGL for context management for OpenGL
OPTION(ENABLE_EGL           "Use EGL for OpenGL context management"    OFF)

OPTION(LIBRARY_BZIP2        "Enable BZIP2 compression support"          ON)
OPTION(LIBRARY_LZ4          "Enable LZ4 compression support"            ON)
OPTION(LIBRARY_LZMA         "Enable LZMA compression support"           ON)
OPTION(LIBRARY_LZFSE        "Enable LZFSE compression support"          ON)

OPTION(LIBRARY_JXL          "Enable jpeg-xl image codec support"        ON)
OPTION(LIBRARY_JP2          "Enable jpeg2000 image codec support"       ON)
OPTION(LIBRARY_WEBP         "Enable WebP image codec support"           ON)
OPTION(LIBRARY_AVIF         "Enable avif image codec support"           ON)
OPTION(LIBRARY_HEIF         "Enable heif image codec support"           ON)
OPTION(LIBRARY_ISAL         "Enable Intel ISA-L compression support"    ON)

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
    set(LIBRARY_JXL    OFF)
    set(LIBRARY_JP2    OFF)
    set(LIBRARY_WEBP   OFF)
    set(LIBRARY_AVIF   OFF)
    set(LIBRARY_HEIF   OFF)
    set(LIBRARY_ISAL   OFF)
endif ()

# Option to enable every Intel ISA extension
option(INTEL_DELUXE         "Enable ALL Intel ISA extensions"           OFF)
if (INTEL_DELUXE OR ENABLE_AVX512)
    set(ENABLE_AVX2    ON)
    set(ENABLE_PCLMUL  ON)
    set(ENABLE_POPCNT  ON)
    set(ENABLE_AES     ON)
    set(ENABLE_F16C    ON)
    set(ENABLE_LZCNT   ON)
    set(ENABLE_BMI     ON)
    set(ENABLE_BMI2    ON)
    set(ENABLE_FMA     ON)
    set(ENABLE_SHA     ON)
endif ()

# Linker selection
set(LINKER "default" CACHE STRING "Linker to use (default, mold)")
set_property(CACHE LINKER PROPERTY STRINGS default mold)

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

message("[ ${PROJECT_NAME} ]")
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

    if (DEFINED ENV{VCPKG_TARGET_TRIPLET})
        set(DEFAULT_INSTALL_PATH "$ENV{VCPKG_ROOT}/installed/$ENV{VCPKG_TARGET_TRIPLET}")
    elseif (DEFINED ENV{VCPKG_DEFAULT_TRIPLET})
        set(DEFAULT_INSTALL_PATH "$ENV{VCPKG_ROOT}/installed/$ENV{VCPKG_DEFAULT_TRIPLET}")
    else ()
        set(DEFAULT_INSTALL_PATH "${CMAKE_BINARY_DIR}/install")
    endif ()

    message("VCPKG_ROOT: " $ENV{VCPKG_ROOT})
    message("VCPKG_TARGET_TRIPLET: " $ENV{VCPKG_TARGET_TRIPLET})
    message("VCPKG_DEFAULT_TRIPLET: " $ENV{VCPKG_DEFAULT_TRIPLET})

    set(CMAKE_INSTALL_PREFIX "${DEFAULT_INSTALL_PATH}" CACHE PATH "Install path prefix" FORCE)

endif ()

message("Installation Directory: ${CMAKE_INSTALL_PREFIX}")

# ------------------------------------------------------------------------------
# sources: mango
# ------------------------------------------------------------------------------

set(MANGO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/mango")
set(MANGO_SOURCE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/source/mango")

function(select_window_system_sources system)
    if (${system} STREQUAL "xlib")
        set(WINDOW_SOURCES
            ${MANGO_SOURCE_DIR}/window/xlib/xlib_window.hpp
            ${MANGO_SOURCE_DIR}/window/xlib/xlib_window.cpp
            PARENT_SCOPE
        )
    elseif (${system} STREQUAL "xcb")
        set(WINDOW_SOURCES
            ${MANGO_SOURCE_DIR}/window/xcb/xcb_window.hpp
            ${MANGO_SOURCE_DIR}/window/xcb/xcb_window.cpp
            PARENT_SCOPE
        )
    elseif (${system} STREQUAL "wayland")
        set(WINDOW_SOURCES
            ${MANGO_SOURCE_DIR}/window/wayland/wayland_window.hpp
            ${MANGO_SOURCE_DIR}/window/wayland/wayland_window.cpp
            PARENT_SCOPE
        )
    else ()
        message(FATAL_ERROR "Unsupported window system: ${system}")
    endif ()
endfunction()

if (BUILD_OPENGL)

    if (NOT EMSCRIPTEN)

        FILE(GLOB WINDOW_HEADERS
            "${MANGO_INCLUDE_DIR}/window/*.hpp"
        )

    endif ()

    FILE(GLOB OPENGL_HEADERS
        "${MANGO_INCLUDE_DIR}/opengl/*.hpp"
    )

    if (WIN32 OR CYGWIN)

        FILE(GLOB WINDOW_SOURCES
            "${MANGO_SOURCE_DIR}/window/win32/*.cpp"
        )

        FILE(GLOB OPENGL_SOURCES
            "${MANGO_SOURCE_DIR}/opengl/*.cpp"
            "${MANGO_SOURCE_DIR}/opengl/wgl/*.cpp"
        )

    elseif (APPLE)

        FILE(GLOB WINDOW_SOURCES
            "${MANGO_SOURCE_DIR}/window/cocoa/*.mm"
        )

        FILE(GLOB OPENGL_SOURCES
            "${MANGO_SOURCE_DIR}/opengl/*.cpp"
            "${MANGO_SOURCE_DIR}/opengl/cocoa/*.mm"
        )

    elseif (EMSCRIPTEN)

        FILE(GLOB OPENGL_SOURCES
            "${MANGO_SOURCE_DIR}/opengl/*.cpp"
        )

    else ()

        select_window_system_sources(${WINDOW_SYSTEM})

        FILE(GLOB OPENGL_SOURCES
            "${MANGO_SOURCE_DIR}/opengl/*.cpp"
            "${MANGO_SOURCE_DIR}/opengl/glx/*.cpp"
            "${MANGO_SOURCE_DIR}/opengl/egl/*.cpp"
        )

    endif ()

endif ()

if (BUILD_VULKAN)

    if (NOT (APPLE OR EMSCRIPTEN))

        FILE(GLOB WINDOW_HEADERS
            "${MANGO_INCLUDE_DIR}/window/*.hpp"
        )

    endif ()

    FILE(GLOB VULKAN_HEADERS
        "${MANGO_INCLUDE_DIR}/vulkan/*.hpp"
    )

    if (WIN32 OR CYGWIN)

        FILE(GLOB WINDOW_SOURCES
            "${MANGO_SOURCE_DIR}/window/win32/*.cpp"
        )

        FILE(GLOB VULKAN_SOURCES
            "${MANGO_SOURCE_DIR}/vulkan/*.cpp"
        )

    elseif (APPLE OR EMSCRIPTEN)

        # NOT SUPPORTED

    else ()

        select_window_system_sources(${WINDOW_SYSTEM})

        FILE(GLOB VULKAN_SOURCES
            "${MANGO_SOURCE_DIR}/vulkan/*.cpp"
        )

    endif ()

endif ()

if (WIN32 OR CYGWIN)

    FILE(GLOB CORE_SOURCES
        "${MANGO_SOURCE_DIR}/core/*.cpp"
        "${MANGO_SOURCE_DIR}/core/win32/*.cpp"
    )

    FILE(GLOB FILESYSTEM_SOURCES
        "${MANGO_SOURCE_DIR}/filesystem/*.cpp"
        "${MANGO_SOURCE_DIR}/filesystem/win32/*.cpp"
    )

elseif (APPLE)

    FILE(GLOB CORE_SOURCES
        "${MANGO_SOURCE_DIR}/core/*.cpp"
        "${MANGO_SOURCE_DIR}/core/unix/*.cpp"
    )

    FILE(GLOB FILESYSTEM_SOURCES
        "${MANGO_SOURCE_DIR}/filesystem/*.cpp"
        "${MANGO_SOURCE_DIR}/filesystem/unix/*.cpp"
    )

else ()

    FILE(GLOB CORE_SOURCES
        "${MANGO_SOURCE_DIR}/core/*.cpp"
        "${MANGO_SOURCE_DIR}/core/unix/*.cpp"
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

FILE(GLOB EXTERNAL_ASTC
    "${EXTERNAL_SOURCE_DIR}/astc/*.cpp"
    "${EXTERNAL_SOURCE_DIR}/astc/*.h"
)

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
    "${EXTERNAL_SOURCE_DIR}/simdjson/*.h"
    "${EXTERNAL_SOURCE_DIR}/simdjson/*.cpp"
    "${EXTERNAL_SOURCE_DIR}/fastgltf/include/fastgltf/*.hpp"
    "${EXTERNAL_SOURCE_DIR}/fastgltf/src/*.cpp"
)

SOURCE_GROUP("external/aes"         FILES ${EXTERNAL_AES})
SOURCE_GROUP("external/bc"          FILES ${EXTERNAL_BC})
SOURCE_GROUP("external/concurrent"  FILES ${EXTERNAL_CONCURRENT_QUEUE})
SOURCE_GROUP("external/astc"        FILES ${EXTERNAL_ASTC})
SOURCE_GROUP("external/google"      FILES ${EXTERNAL_GOOGLE})
if (LIBRARY_LZMA)
    SOURCE_GROUP("external/lzma"    FILES ${EXTERNAL_LZMA})
endif ()
if (LIBRARY_LZFSE)
    SOURCE_GROUP("external/lzfse"   FILES ${EXTERNAL_LZFSE})
endif ()
SOURCE_GROUP("external/unrar"       FILES ${EXTERNAL_UNRAR})
SOURCE_GROUP("external/zpng"        FILES ${EXTERNAL_ZPNG})
SOURCE_GROUP("external/basisu"      FILES ${EXTERNAL_BASISU})
SOURCE_GROUP("external/import3d"    FILES ${EXTERNAL_IMPORT3D})

# ------------------------------------------------------------------------------
# library: mango
# ------------------------------------------------------------------------------

set(MANGO_SOURCES
    ${MAIN_HEADER}
    ${CORE_HEADERS}
    ${CORE_SOURCES}
    ${FILESYSTEM_HEADERS}
    ${FILESYSTEM_SOURCES}
    ${IMAGE_HEADERS}
    ${IMAGE_SOURCES}
    ${JPEG_HEADERS}
    ${JPEG_SOURCES}
    ${MATH_HEADERS}
    ${MATH_SOURCES}
    ${SIMD_HEADERS}
    ${SIMD_SOURCES}
    ${EXTERNAL_AES}
    ${EXTERNAL_BC}
    ${EXTERNAL_CONCURRENT_QUEUE}
    ${EXTERNAL_ASTC}
    ${EXTERNAL_GOOGLE}
    ${EXTERNAL_UNRAR}
    ${EXTERNAL_BASISU}
    ${EXTERNAL_ZPNG}
)

if (LIBRARY_LZMA)
    list(APPEND MANGO_SOURCES
        ${EXTERNAL_LZMA}
    )
endif ()

if (LIBRARY_LZFSE)
    list(APPEND MANGO_SOURCES
        ${EXTERNAL_LZFSE}
    )
endif ()

if (BUILD_OPENGL)
    list(APPEND MANGO_SOURCES
        ${WINDOW_HEADERS}
        ${WINDOW_SOURCES}
        ${OPENGL_HEADERS}
        ${OPENGL_SOURCES}
    )
endif ()

if (BUILD_VULKAN)
    list(APPEND MANGO_SOURCES
        ${WINDOW_HEADERS}
        ${WINDOW_SOURCES}
        ${VULKAN_HEADERS}
        ${VULKAN_SOURCES}
    )
endif ()

if (BUILD_IMPORT3D)
    list(APPEND MANGO_SOURCES
        ${IMPORT3D_HEADERS}
        ${IMPORT3D_SOURCES}
        ${EXTERNAL_IMPORT3D}
    )
    include_directories(BEFORE
        "${EXTERNAL_SOURCE_DIR}/fastgltf/include"
        "${EXTERNAL_SOURCE_DIR}/simdjson"
    )
endif ()

add_library(mango ${MANGO_SOURCES})

set_target_properties(mango PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION}
)

if (APPLE)
    target_compile_options(mango PUBLIC "-mmacosx-version-min=10.15")
endif ()

# ------------------------------------------------------------------------------
# Platform Libraries
# ------------------------------------------------------------------------------

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    message("PkgConfig: " ${PKG_CONFIG_VERSION_STRING})
endif ()

if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(mango PUBLIC "${CMAKE_THREAD_LIBS_INIT}")
endif ()

if (BUILD_OPENGL OR BUILD_VULKAN)
    message("[Platform Libraries]")
endif ()

# ------------------------------------------------------------------------------
# OpenGL
# ------------------------------------------------------------------------------

if (BUILD_OPENGL)
    if (OPENGL_FOUND)
        if (APPLE)
            message("    OpenGL: Cocoa")
            target_link_libraries(mango PUBLIC "-framework Cocoa -framework OpenGL")
        elseif (WIN32)
            message("    OpenGL: WIN32")
            target_link_libraries(mango PUBLIC ${OPENGL_gl_LIBRARY})
        else ()
            if (WINDOW_SYSTEM STREQUAL "xlib")
                target_compile_definitions(mango PRIVATE "MANGO_ENABLE_XLIB")
                target_link_libraries(mango PUBLIC X11 GL ${OPENGL_glx_LIBRARY})
                if (ENABLE_EGL)
                    message("    OpenGL: EGL | XLIB")
                    target_compile_definitions(mango PRIVATE "MANGO_ENABLE_EGL")
                    target_link_libraries(mango PUBLIC EGL)
                else ()
                    message("    OpenGL: GLX | XLIB")
                endif ()
            elseif (WINDOW_SYSTEM STREQUAL "xcb")
                target_compile_definitions(mango PRIVATE "MANGO_ENABLE_XCB")
                target_link_libraries(mango PUBLIC xcb xcb-xkb xcb-keysyms xcb-icccm xkbcommon xcb-glx GL ${OPENGL_glx_LIBRARY})
                if (ENABLE_EGL)
                    message("    OpenGL: EGL | XCB")
                    target_compile_definitions(mango PRIVATE "MANGO_ENABLE_EGL")
                    target_link_libraries(mango PUBLIC EGL)
                else ()
                    message("    OpenGL: GLX | XCB")
                endif ()
            elseif (WINDOW_SYSTEM STREQUAL "wayland")
                message("    OpenGL: EGL | Wayland")
                target_compile_definitions(mango PRIVATE "MANGO_ENABLE_WAYLAND")
                target_compile_definitions(mango PRIVATE "MANGO_ENABLE_EGL")
                find_package(PkgConfig REQUIRED)
                pkg_check_modules(WAYLAND REQUIRED wayland-client)
                pkg_check_modules(XKB REQUIRED xkbcommon)
                target_link_libraries(mango PUBLIC EGL GL wayland-client xkbcommon)
            endif()
        endif ()
    endif ()
endif ()

# ------------------------------------------------------------------------------
# Vulkan
# ------------------------------------------------------------------------------

if (BUILD_VULKAN)
    if (VULKAN_FOUND)

        find_package(glslang CONFIG REQUIRED)
        target_link_libraries(mango PRIVATE
            glslang::glslang
            glslang::SPIRV
            #glslang::OSDependent
            #glslang::MachineIndependent
            #glslang::GenericCodeGen
            #glslang::glslang-default-resource-limits
        )

        # Link default resource limits provider for GetDefaultResources()
        if (TARGET glslang::glslang-default-resource-limits)
            target_link_libraries(mango PRIVATE glslang::glslang-default-resource-limits)
        elseif (TARGET glslang-default-resource-limits)
            target_link_libraries(mango PRIVATE glslang-default-resource-limits)
        elseif (TARGET glslang::ResourceLimits)
            target_link_libraries(mango PRIVATE glslang::ResourceLimits)
        endif ()

        if (WIN32)
            message("    Vulkan: WIN32")
            target_include_directories(mango PUBLIC ${Vulkan_INCLUDE_DIRS})
            target_link_libraries(mango PUBLIC ${Vulkan_LIBRARIES})
        else ()
            if (WINDOW_SYSTEM STREQUAL "xlib")
                target_compile_definitions(mango PRIVATE "MANGO_ENABLE_XLIB")
                target_link_libraries(mango PUBLIC X11 vulkan)
                message("    Vulkan: XLIB")
            elseif (WINDOW_SYSTEM STREQUAL "xcb")
                target_compile_definitions(mango PRIVATE "MANGO_ENABLE_XCB")
                target_link_libraries(mango PUBLIC vulkan xcb xcb-xkb xcb-keysyms xcb-icccm xkbcommon)
                message("    Vulkan: XCB")
            elseif (WINDOW_SYSTEM STREQUAL "wayland")
                message("    Vulkan: Wayland")
                target_compile_definitions(mango PRIVATE "MANGO_ENABLE_WAYLAND")
                find_package(PkgConfig REQUIRED)
                pkg_check_modules(WAYLAND REQUIRED wayland-client)
                pkg_check_modules(XKB REQUIRED xkbcommon)
                target_link_libraries(mango PUBLIC vulkan wayland-client xkbcommon)
            endif()
        endif ()
    endif ()
endif ()

# ------------------------------------------------------------------------------
# find_module()
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
            message("    " ${Prefix} ": " ${${PackageName}_VERSION} " [cmake]")
            target_compile_definitions(${Target} PRIVATE "MANGO_ENABLE_${Prefix}")
            target_link_libraries(${Target} ${Interface} ${ImportedTarget})
            return ()
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
            message("    " ${Prefix} ": " ${${Prefix}_VERSION} " [pkgconfig]")
            target_compile_definitions(${Target} PRIVATE "MANGO_ENABLE_${Prefix}")
            target_include_directories(${Target} ${Interface} ${${Prefix}_INCLUDE_DIRS})
            target_link_directories(${Target} ${Interface} ${${Prefix}_LIBRARY_DIRS})
            target_link_libraries(${Target} ${Interface} ${${Prefix}_LIBRARIES})
            return ()
        endif ()
    endif ()

    # check required

    if (Mode STREQUAL "REQUIRED")
        message(FATAL_ERROR "Failed to locate REQUIRED package: " ${Prefix})
    else ()
        message("    " ${Prefix} ": " ${${PackageName}_VERSION} " [not found]")
    endif ()

endfunction()

# ------------------------------------------------------------------------------
# Required Libraries
# ------------------------------------------------------------------------------

message("[Required Libraries]")

find_module(mango FMT PUBLIC REQUIRED "" fmt fmt::fmt 9.1)
find_module(mango ZLIB ${INTERFACE} REQUIRED zlib ZLIB ZLIB::ZLIB 0)
find_module(mango LCMS2 ${INTERFACE} REQUIRED lcms2 lcms2 lcms2::lcms2 0)

if (BUILD_SHARED_LIBS OR WIN32)
    find_module(mango LIBDEFLATE ${INTERFACE} REQUIRED libdeflate libdeflate libdeflate::libdeflate_shared 0)
else ()
    find_module(mango LIBDEFLATE ${INTERFACE} REQUIRED libdeflate libdeflate libdeflate::libdeflate_static 0)
endif ()

if (APPLE OR WIN32)
    # brew/vcpkg imported target is 'zstd::libzstd'
    find_module(mango ZSTD ${INTERFACE} REQUIRED libzstd zstd zstd::libzstd 0)
else ()
    # apt/pacman imported target is 'zstd'
    find_module(mango ZSTD ${INTERFACE} REQUIRED libzstd zstd zstd 0)
endif ()

# ------------------------------------------------------------------------------
# Optional Libraries
# ------------------------------------------------------------------------------

message("[Optional Libraries]")

if (LIBRARY_BZIP2)
    find_module(mango BZIP2 ${INTERFACE} OPTIONAL "" BZip2 BZip2::BZip2 0)
endif ()

if (LIBRARY_LZ4)
    find_module(mango LZ4 ${INTERFACE} OPTIONAL liblz4 lz4 lz4::lz4 0)
endif ()

if (LIBRARY_ISAL)
    find_module(mango ISAL ${INTERFACE} OPTIONAL libisal unofficial-isal unofficial::isal::isal 0)
endif ()

if (LIBRARY_JXL)
    find_module(mango JXL ${INTERFACE} OPTIONAL libjxl "" "" 0.8.2)
    find_module(mango JXL_THREADS ${INTERFACE} OPTIONAL libjxl_threads "" "" 0.7)
endif ()

if (LIBRARY_JP2)
    find_module(mango JP2 ${INTERFACE} OPTIONAL libopenjp2 OpenJPEG openjp2 2.4.0)
endif ()

if (LIBRARY_WEBP)
    find_module(mango WEBP ${INTERFACE} OPTIONAL libwebp WebP WebP::webp 1.3.2)
endif ()

if (LIBRARY_AVIF)
    find_module(mango AVIF ${INTERFACE} OPTIONAL libavif libavif avif 1.0.0)
endif ()

if (LIBRARY_HEIF)
    find_module(mango HEIF ${INTERFACE} OPTIONAL libheif libheif heif 1.13.0)
endif ()

# ------------------------------------------------------------------------------
# configuration
# ------------------------------------------------------------------------------

set(SIMD "NONE")

if (BUILD_SHARED_LIBS)
    target_compile_definitions(mango PUBLIC "MANGO_SHARED")
    target_compile_definitions(mango PRIVATE "MANGO_API_EXPORT")
endif ()

if (WIN32)
    target_compile_definitions(mango PUBLIC UNICODE)
endif ()

if ((ARM AND ENABLE_NEON) OR ARM64)
    set(SIMD "NEON")
endif ()

if (EMSCRIPTEN)

    set(SIMD "WASM")
    target_compile_options(mango PUBLIC "-msimd128")

elseif (MSVC)

    target_compile_options(mango PUBLIC $<$<CONFIG:Release>:/Ox>)
    target_compile_options(mango PUBLIC "/utf-8")
    target_compile_options(mango PUBLIC "/MP")

    if (X86 OR X86_64)

        if (ENABLE_PCLMUL)
            target_compile_definitions(mango PUBLIC "__PCLMUL__")
        endif ()

        if (ENABLE_POPCNT)
            target_compile_definitions(mango PUBLIC "__POPCNT__")
        endif ()

        if (ENABLE_AES)
            target_compile_definitions(mango PUBLIC "__AES__")
        endif ()

        if (ENABLE_F16C)
            target_compile_definitions(mango PUBLIC "__F16C__")
        endif ()

        if (ENABLE_LZCNT)
            target_compile_definitions(mango PUBLIC "__LZCNT__")
        endif ()

        if (ENABLE_BMI)
            target_compile_definitions(mango PUBLIC "__BMI__")
        endif ()

        if (ENABLE_BMI2)
            target_compile_definitions(mango PUBLIC "__BMI2__")
        endif ()

        if (ENABLE_FMA)
            target_compile_definitions(mango PUBLIC "__FMA__")
        endif ()

        if (ENABLE_SHA)
            target_compile_definitions(mango PUBLIC "__SHA__")
        endif ()

        # enable only one (the most recent) SIMD extension
        if (ENABLE_AVX512)
            set(SIMD "AVX-512 (2015)")
            target_compile_options(mango PUBLIC "/arch:AVX512")
            target_compile_definitions(mango PUBLIC "__AVX512F__" "__AVX512DQ__")
        elseif (ENABLE_AVX2)
            set(SIMD "AVX2 (2013)")
            target_compile_options(mango PUBLIC "/arch:AVX2")
        elseif (ENABLE_AVX)
            set(SIMD "AVX (2008)")
            target_compile_options(mango PUBLIC "/arch:AVX")
        elseif (ENABLE_SSE2 OR ENABLE_SSE4)
            # MSVC does not have SSE4 option; at least enable SSE2
            set(SIMD "SSE2 (2001)")
            if (NOT X86_64)
                # MSVC enables SSE2 by default on X86_64
                target_compile_options(mango PUBLIC "/arch:SSE2")
            endif ()
        endif ()

    endif ()

else ()

    add_compile_options(-Wall)
    target_compile_options(mango PRIVATE $<$<CONFIG:Release>:-O3>)

    if (CYGWIN)
        target_compile_options(mango PUBLIC "-municode")
    endif ()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(mango PUBLIC "-ftree-vectorize")
        set(CMAKE_CXX_FLAGS "-Wno-psabi")
    endif ()

    if (ARM AND (NOT APPLE))
        # The compiler for Apple Silicon does not recognize these
        add_definitions (-mfpu=neon -mfloat-abi=hard)
    endif ()

    if (X86 OR X86_64)

        if (ENABLE_PCLMUL)
            target_compile_options(mango PUBLIC "-mpclmul")
        endif ()

        if (ENABLE_POPCNT)
            target_compile_options(mango PUBLIC "-mpopcnt")
        endif ()

        if (ENABLE_AES)
            target_compile_options(mango PUBLIC "-maes")
        endif ()

        if (ENABLE_F16C)
            target_compile_options(mango PUBLIC "-mf16c")
        endif ()

        if (ENABLE_LZCNT)
            target_compile_options(mango PUBLIC "-mlzcnt")
        endif ()

        if (ENABLE_BMI)
            target_compile_options(mango PUBLIC "-mbmi")
        endif ()

        if (ENABLE_BMI2)
            target_compile_options(mango PUBLIC "-mbmi2")
        endif ()

        if (ENABLE_FMA)
            target_compile_options(mango PUBLIC "-mfma")
        endif ()

        if (ENABLE_SHA)
            target_compile_options(mango PUBLIC "-msha")
        endif ()

        # enable only one (the most recent) SIMD extension
        if (ENABLE_AVX512)
            set(SIMD "AVX-512 (2015)")
            target_compile_options(mango PUBLIC "-mavx512dq")
            target_compile_options(mango PUBLIC "-mavx512vl")
            target_compile_options(mango PUBLIC "-mavx512bw")
        elseif (ENABLE_AVX2)
            set(SIMD "AVX2 (2013)")
            target_compile_options(mango PUBLIC "-mavx2")
        elseif (ENABLE_AVX)
            set(SIMD "AVX (2008)")
            target_compile_options(mango PUBLIC "-mavx")
        elseif (ENABLE_SSE4)
            set(SIMD "SSE4.2 (2006)")
            target_compile_options(mango PUBLIC "-msse4")
        elseif (ENABLE_SSE2)
            set(SIMD "SSE2 (2001)")
            target_compile_options(mango PUBLIC "-msse2")
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

add_library(mango::mango ALIAS mango)

target_include_directories(mango
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    INTERFACE $<INSTALL_INTERFACE:include/>
)

# install mango headers
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h*"
)

# install mango library
set(MANGO_INSTALL_BINDIR "$<IF:$<CONFIG:Debug>,debug/${CMAKE_INSTALL_BINDIR},${CMAKE_INSTALL_BINDIR}>")
set(MANGO_INSTALL_LIBDIR "$<IF:$<CONFIG:Debug>,debug/${CMAKE_INSTALL_LIBDIR},${CMAKE_INSTALL_LIBDIR}>")

install(TARGETS mango
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

    if (WIN32)
        foreach (x ${EXAMPLE_BINARY_FOLDERS})
            set(source_file $<TARGET_FILE:mango>)
            set(destination_folder ${x}/${CMAKE_CFG_INTDIR})
            add_custom_command(TARGET mango POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Copying: ${source_file}  -->  ${destination_folder}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${destination_folder}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${source_file}" "${destination_folder}"
            )
        endforeach ()
    endif ()

endif ()

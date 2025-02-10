cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0091 NEW)
project(mango
        VERSION 1.7.0
        DESCRIPTION "A multi-platform development framework for graphics programmers"
        LANGUAGES CXX C ASM)

# ------------------------------------------------------------------------------
# compiler
# ------------------------------------------------------------------------------

message("[ ${PROJECT_NAME} ]")
list(APPEND CMAKE_MESSAGE_INDENT "    ")

message("Compiler: ${CMAKE_CXX_COMPILER_ID}")

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # msvc is set by cmake
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CLANG 1)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Intel")
    set(GCC 1)
endif ()

# ------------------------------------------------------------------------------
# processor
# ------------------------------------------------------------------------------

message("Processor: ${CMAKE_SYSTEM_PROCESSOR}")

if (CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
    set(X86_64 1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "i686.*|i386.*|x86.*")
    set(X86 1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*|arm64.*)")
    set(AARCH64 1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)")
    set(ARM 1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "PPC64*|ppc64*|powerpc64*")
    set(PPC64 1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "PPC*|ppc*|powerpc*")
    set(PPC 1)
endif ()

# ------------------------------------------------------------------------------
# options
# ------------------------------------------------------------------------------

OPTION(BUILD_SHARED_LIBS    "Build as shared library (so/dll/dylib)"    ON)

OPTION(BUILD_OPENGL         "Build OpenGL library"                      ON)
OPTION(BUILD_IMPORT3D       "Build Import3D library"                    ON)
OPTION(BUILD_EXAMPLES       "Build examples"                            ON)

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

OPTION(ENABLE_AES           "Enable AES"                                OFF)
OPTION(ENABLE_PCLMUL        "Enable PCLMUL"                             OFF)
OPTION(ENABLE_POPCNT        "Enable POPCNT"                             OFF)
OPTION(ENABLE_F16C          "Enable F16C conversion instructions"       OFF)
OPTION(ENABLE_BMI           "Enable BMI + LZCNT"                        OFF)
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
if (INTEL_DELUXE)
    set(ENABLE_AVX2    ON)
    set(ENABLE_AES     ON)
    set(ENABLE_PCLMUL  ON)
    set(ENABLE_POPCNT  ON)
    set(ENABLE_F16C    ON)
    set(ENABLE_BMI     ON)
    set(ENABLE_BMI2    ON)
    set(ENABLE_FMA     ON)
    set(ENABLE_SHA     ON)
endif ()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

if (WIN32 OR CYGWIN)
    # disable shared library build on windows
    set(BUILD_SHARED_LIBS OFF)
endif ()

if (BUILD_SHARED_LIBS)
    if (WIN32 OR CYGWIN)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif ()
    message("Build: Shared, " ${CMAKE_BUILD_TYPE})
else ()
    message("Build: Static, " ${CMAKE_BUILD_TYPE})
endif ()

# ------------------------------------------------------------------------------
# sources: mango
# ------------------------------------------------------------------------------

if (BUILD_OPENGL)

    if (NOT EMSCRIPTEN)

        FILE(GLOB WINDOW_HEADERS
            "${CMAKE_CURRENT_SOURCE_DIR}/include/mango/window/*.hpp"
        )

    endif ()

    FILE(GLOB OPENGL_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/mango/opengl/*.hpp"
    )

    if (WIN32 OR CYGWIN)

        FILE(GLOB WINDOW_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/window/win32/*.cpp"
        )

        FILE(GLOB OPENGL_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/opengl/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/opengl/wgl/*.cpp"
        )

    elseif (APPLE)

        FILE(GLOB WINDOW_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/window/cocoa/*.mm"
        )

        FILE(GLOB OPENGL_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/opengl/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/opengl/cocoa/*.mm"
        )

    elseif (EMSCRIPTEN)

        FILE(GLOB OPENGL_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/opengl/*.cpp"
        )

    else ()

        FILE(GLOB WINDOW_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/window/xlib/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/window/xcb/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/window/wayland/*.cpp"
        )

        FILE(GLOB OPENGL_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/opengl/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/opengl/glx/*.cpp"
        )

    endif ()

endif ()

if (WIN32 OR CYGWIN)

    FILE(GLOB CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/core/win32/*.cpp"
    )

    FILE(GLOB FILESYSTEM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/filesystem/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/filesystem/win32/*.cpp"
    )

elseif (APPLE)

    FILE(GLOB CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/core/unix/*.cpp"
    )

    FILE(GLOB FILESYSTEM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/filesystem/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/filesystem/unix/*.cpp"
    )

elseif (EMSCRIPTEN)

    FILE(GLOB CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/core/unix/*.cpp"
    )

    FILE(GLOB FILESYSTEM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/filesystem/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/filesystem/unix/*.cpp"
    )

else ()

    FILE(GLOB CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/core/unix/*.cpp"
    )

    FILE(GLOB FILESYSTEM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/filesystem/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/filesystem/unix/*.cpp"
    )

endif ()

SET(MAIN_HEADER               "${CMAKE_CURRENT_SOURCE_DIR}/include/mango/mango.hpp")
FILE(GLOB CORE_HEADERS        "${CMAKE_CURRENT_SOURCE_DIR}/include/mango/core/*.hpp")
FILE(GLOB FILESYSTEM_HEADERS  "${CMAKE_CURRENT_SOURCE_DIR}/include/mango/filesystem/*.hpp")
FILE(GLOB IMAGE_HEADERS       "${CMAKE_CURRENT_SOURCE_DIR}/include/mango/image/*.hpp")
FILE(GLOB JPEG_HEADERS        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/jpeg/*.hpp")
FILE(GLOB MATH_HEADERS        "${CMAKE_CURRENT_SOURCE_DIR}/include/mango/math/*.hpp")
FILE(GLOB SIMD_HEADERS        "${CMAKE_CURRENT_SOURCE_DIR}/include/mango/simd/*.hpp")
FILE(GLOB IMPORT3D_HEADERS    "${CMAKE_CURRENT_SOURCE_DIR}/include/mango/import3d/*.hpp")

FILE(GLOB IMAGE_SOURCES       "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/image/*.cpp")
FILE(GLOB JPEG_SOURCES        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/jpeg/*.cpp")
FILE(GLOB MATH_SOURCES        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/math/*.cpp")
FILE(GLOB SIMD_SOURCES        "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/simd/*.cpp")
FILE(GLOB IMPORT3D_SOURCES    "${CMAKE_CURRENT_SOURCE_DIR}/source/mango/import3d/*.cpp")

SOURCE_GROUP("include"              FILES ${MAIN_HEADER})
SOURCE_GROUP("include/core"         FILES ${CORE_HEADERS})
SOURCE_GROUP("include/filesystem"   FILES ${FILESYSTEM_HEADERS})
SOURCE_GROUP("include/image"        FILES ${IMAGE_HEADERS})
SOURCE_GROUP("include/jpeg"         FILES ${JPEG_HEADERS})
SOURCE_GROUP("include/math"         FILES ${MATH_HEADERS})
SOURCE_GROUP("include/simd"         FILES ${SIMD_HEADERS})
SOURCE_GROUP("include/window"       FILES ${WINDOW_HEADERS})
SOURCE_GROUP("include/opengl"       FILES ${OPENGL_HEADERS})
SOURCE_GROUP("include/import3d"     FILES ${IMPORT3D_HEADERS})

SOURCE_GROUP("source/core"          FILES ${CORE_SOURCES})
SOURCE_GROUP("source/filesystem"    FILES ${FILESYSTEM_SOURCES})
SOURCE_GROUP("source/image"         FILES ${IMAGE_SOURCES})
SOURCE_GROUP("source/jpeg"          FILES ${JPEG_SOURCES})
SOURCE_GROUP("source/math"          FILES ${MATH_SOURCES})
SOURCE_GROUP("source/window"        FILES ${WINDOW_SOURCES})
SOURCE_GROUP("source/opengl"        FILES ${OPENGL_SOURCES})
SOURCE_GROUP("source/import3d"      FILES ${IMPORT3D_SOURCES})

# ------------------------------------------------------------------------------
# sources: external
# ------------------------------------------------------------------------------

FILE(GLOB EXTERNAL_LZMA
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/lzma/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/lzma/*.c"
)

FILE(GLOB EXTERNAL_AES
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/aes/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/aes/*.cpp"
)

FILE(GLOB EXTERNAL_BC
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/bc/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/bc/*.cpp"
)

FILE(GLOB EXTERNAL_BZIP2
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/bzip2/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/bzip2/*.c"
)

FILE(GLOB EXTERNAL_CONCURRENT_QUEUE
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/concurrentqueue/*.h"
)

FILE(GLOB EXTERNAL_ASTC
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/astc/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/astc/*.h"
)

FILE(GLOB EXTERNAL_GOOGLE
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/google/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/google/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/google/*.h"
)

FILE(GLOB EXTERNAL_LZFSE
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/lzfse/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/lzfse/*.c"
)

FILE(GLOB EXTERNAL_UNRAR
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/unrar/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/unrar/*.cpp"
)

FILE(GLOB EXTERNAL_ZPNG
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/zpng/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/zpng/*.cpp"
)

FILE(GLOB EXTERNAL_BASISU
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/basisu/transcoder/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/basisu/transcoder/*.cpp"
)

FILE(GLOB EXTERNAL_FMT
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/fmt/src/*.cc"
)

FILE(GLOB EXTERNAL_IMPORT3D
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/mikktspace/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/mikktspace/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/simdjson/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/simdjson/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/fastgltf/include/fastgltf/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/external/fastgltf/src/*.cpp"
)

SOURCE_GROUP("external/lzma"        FILES ${EXTERNAL_LZMA})
SOURCE_GROUP("external/aes"         FILES ${EXTERNAL_AES})
SOURCE_GROUP("external/bc"          FILES ${EXTERNAL_BC})
SOURCE_GROUP("external/bzip2"       FILES ${EXTERNAL_BZIP2})
SOURCE_GROUP("external/concurrent"  FILES ${EXTERNAL_CONCURRENT_QUEUE})
SOURCE_GROUP("external/astc"        FILES ${EXTERNAL_ASTC})
SOURCE_GROUP("external/google"      FILES ${EXTERNAL_GOOGLE})
SOURCE_GROUP("external/lzfse"       FILES ${EXTERNAL_LZFSE})
SOURCE_GROUP("external/unrar"       FILES ${EXTERNAL_UNRAR})
SOURCE_GROUP("external/zpng"        FILES ${EXTERNAL_ZPNG})
SOURCE_GROUP("external/basisu"      FILES ${EXTERNAL_BASISU})
SOURCE_GROUP("external/fmt"         FILES ${EXTERNAL_FMT})
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
    ${EXTERNAL_LZMA}
    ${EXTERNAL_AES}
    ${EXTERNAL_BC}
    ${EXTERNAL_BZIP2}
    ${EXTERNAL_CONCURRENT_QUEUE}
    ${EXTERNAL_ASTC}
    ${EXTERNAL_GOOGLE}
    ${EXTERNAL_LZFSE}
    ${EXTERNAL_UNRAR}
    ${EXTERNAL_ZPNG}
    ${EXTERNAL_BASISU}
    ${EXTERNAL_FMT}
)

if (BUILD_OPENGL)
    list(APPEND MANGO_SOURCES
        ${WINDOW_HEADERS}
        ${WINDOW_SOURCES}
        ${OPENGL_HEADERS}
        ${OPENGL_SOURCES}
    )
endif ()

if (BUILD_IMPORT3D)
    list(APPEND MANGO_SOURCES
        ${IMPORT3D_HEADERS}
        ${IMPORT3D_SOURCES}
        ${EXTERNAL_IMPORT3D}
    )
endif ()

add_library(mango ${MANGO_SOURCES})

set_target_properties(mango PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if (APPLE)
    if (BUILD_OPENGL AND BUILD_SHARED_LIBS)
        target_link_libraries(mango PUBLIC "-framework Cocoa" "-framework OpenGL")
    endif ()

    target_compile_options(mango PUBLIC "-mmacosx-version-min=10.15")
endif ()

# ------------------------------------------------------------------------------
# platform libraries
# ------------------------------------------------------------------------------

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    message("PkgConfig: " ${PKG_CONFIG_VERSION_STRING})
endif ()

find_package(Threads)
if (CMAKE_THREAD_LIBS_INIT)
    message("   + Threads")
    target_link_libraries(mango PUBLIC "${CMAKE_THREAD_LIBS_INIT}")
endif ()

if (BUILD_OPENGL)

    if (WIN32)
        set(OpenGL_GL_PREFERENCE LEGACY)
    else ()
        set(OpenGL_GL_PREFERENCE GLVND)
    endif ()

    find_package(OpenGL REQUIRED)
    if (OPENGL_FOUND)
        message("  + OpenGL")
        if (APPLE)
            target_link_libraries(mango PUBLIC "-framework Cocoa -framework OpenGL")
        elseif (WIN32)
            target_link_libraries(mango PUBLIC ${OPENGL_gl_LIBRARY})
        else ()
            message("  + XLIB")
            target_compile_definitions(mango PUBLIC "MANGO_ENABLE_XLIB")
            target_link_libraries(mango PUBLIC X11 GL ${OPENGL_glx_LIBRARY})
            endif ()
    endif ()

endif ()

# ------------------------------------------------------------------------------
# package finding
# ------------------------------------------------------------------------------

function (find_module_with_pkg Target Prefix ItemAndVersion)
    if (PkgConfig_FOUND)
        pkg_check_modules(${Prefix} QUIET ${ItemAndVersion})
        if (${Prefix}_FOUND)
            set(${Prefix}_ENABLE 1 PARENT_SCOPE)
            message("  + " ${Prefix} ": " ${${Prefix}_VERSION} " [pkgconfig]")
            target_compile_definitions(${Target} PRIVATE "MANGO_ENABLE_${Prefix}")
            target_include_directories(${Target} PRIVATE ${${Prefix}_INCLUDE_DIRS})
            target_link_directories(${Target} PRIVATE ${${Prefix}_LIBRARY_DIRS})
            target_link_libraries(${Target} PRIVATE ${${Prefix}_LIBRARIES})
        endif ()
    endif ()
endfunction ()

function (find_module_with_cmake Target Prefix PackageName ItemName Version)
    if (NOT ${Prefix}_ENABLE)
        if (Version STREQUAL "0")
            find_package(${PackageName} QUIET)
        else ()
            find_package(${PackageName} ${Version} QUIET)
        endif ()

        if (${PackageName}_FOUND)
            set(${Prefix}_ENABLE 1 PARENT_SCOPE)
            message("  + " ${Prefix} ": " ${${PackageName}_VERSION} " [cmake]")
            target_compile_definitions(${Target} PRIVATE "MANGO_ENABLE_${Prefix}")
            target_link_libraries(${Target} PRIVATE ${ItemName})
        endif ()
    endif ()
endfunction ()

function (check_required_package Prefix)
    if (NOT ${Prefix}_ENABLE)
        message(FATAL_ERROR "Failed to locate REQUIRED package: " ${Prefix})
        endif ()
endfunction()

# ------------------------------------------------------------------------------
# required libraries
# ------------------------------------------------------------------------------

find_module_with_pkg(mango ZLIB zlib)
find_module_with_cmake(mango ZLIB ZLIB ZLIB::ZLIB 0)
check_required_package(ZLIB)

find_module_with_pkg(mango LIBDEFLATE libdeflate)
find_module_with_cmake(mango LIBDEFLATE libdeflate libdeflate::libdeflate_shared 0)
check_required_package(LIBDEFLATE)

find_module_with_pkg(mango ZSTD libzstd)
find_module_with_cmake(mango ZSTD zstd zstd::libzstd 0)
check_required_package(ZSTD)

find_module_with_pkg(mango LZ4 liblz4)
find_module_with_cmake(mango LZ4 lz4 lz4::lz4 0)
check_required_package(ZSTD)

find_module_with_pkg(mango LCMS2 lcms2)
find_module_with_cmake(mango LCMS2 lcms2 lcms2::lcms2 0)
check_required_package(LCMS2)

# ------------------------------------------------------------------------------
# optional libraries
# ------------------------------------------------------------------------------

if (LIBRARY_JXL)
    find_module_with_pkg(mango JXL libjxl>=0.8.2)
    find_module_with_pkg(mango JXL_THREADS libjxl_threads>=0.7)
endif ()

if (LIBRARY_JP2)
    find_module_with_pkg(mango JP2 libopenjp2>=2.4.0)
    find_module_with_cmake(mango JP2 OpenJPEG openjp2 2.4.0)
endif ()

if (LIBRARY_WEBP)
    find_module_with_pkg(mango WEBP libwebp>=1.3.2)
    find_module_with_cmake(mango WEBP WebP WebP::webp 1.3.2)
endif ()

if (LIBRARY_AVIF)
    find_module_with_pkg(mango AVIF libavif>=1.0.0)
    find_module_with_cmake(mango AVIF libavif avif 1.0.0)
endif ()

if (LIBRARY_HEIF)
    find_module_with_pkg(mango HEIF libheif>=1.13.0)
    find_module_with_cmake(mango HEIF libheif heif 0)
endif ()

if (LIBRARY_ISAL)
    find_module_with_pkg(mango ISAL libisal)
    find_module_with_cmake(mango ISAL unofficial-isal unofficial::isal::isal 0)
endif ()

# ------------------------------------------------------------------------------
# configuration
# ------------------------------------------------------------------------------

if (BUILD_SHARED_LIBS)
    target_compile_definitions(mango PUBLIC "MANGO_API_EXPORT")
endif ()

if (EMSCRIPTEN)

    message("SIMD: WASM")
    target_compile_options(mango PUBLIC "-msimd128")

elseif (MSVC)

    #target_compile_options(mango PUBLIC "/Wall")
    #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox")
    #set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Ox")
    target_compile_options(mango PUBLIC "/DUNICODE")
    target_compile_options(mango PUBLIC "/utf-8")
    target_compile_options(mango PUBLIC "/MP")

    # enable AES (2010)
    if (ENABLE_AES)
        message("Enable: AES (2010)")
        target_compile_definitions(mango PUBLIC "__AES__")
    endif ()

    # enable CLMUL (2008)
    if (ENABLE_PCLMUL)
        message("Enable: PCLMUL (2008)")
        target_compile_definitions(mango PUBLIC "__PCLMUL__")
    endif ()

    # popcnt
    if (ENABLE_POPCNT)
        message("Enable: POPCNT (2008)")
        target_compile_definitions(mango PUBLIC "__POPCNT__")
    endif ()

    # half conversion instructions
    if (ENABLE_F16C)
        message("Enable: F16C (2012)")
        target_compile_definitions(mango PUBLIC "__F16C__")
    endif ()

    # bit manipulation instruction set 1
    if (ENABLE_BMI)
        message("Enable: BMI+LZCNT (2013)")
        target_compile_definitions(mango PUBLIC "__BMI__" "__LZCNT__")
    endif ()

    # bit manipulation instruction set 2
    if (ENABLE_BMI2)
        message("Enable: BMI2 (2013)")
        target_compile_definitions(mango PUBLIC "__BMI2__")
    endif ()

    # fused multiply-add
    if (ENABLE_FMA)
        message("Enable: FMA (2013)")
        target_compile_definitions(mango PUBLIC "__FMA__")
    endif ()

    # sha
    if (ENABLE_SHA)
        message("Enable: SHA (2013)")
        target_compile_definitions(mango PUBLIC "__SHA__")
    endif ()

    if (ENABLE_AVX512)
        message("SIMD: AVX-512 (2015)")
        target_compile_definitions(mango PUBLIC "__AVX512F__" "__AVX512DQ__")
    elseif (ENABLE_AVX2)
        message("SIMD: AVX2 (2013)")
        target_compile_options(mango PUBLIC "/arch:AVX2")
    elseif (ENABLE_AVX)
        message("SIMD: AVX (2008)")
        target_compile_options(mango PUBLIC "/arch:AVX")
    elseif (ENABLE_SSE2 OR ENABLE_SSE4)
        # MSVC does not have SSE4 option; at least enable SSE2
        message("SIMD: SSE2 (2001)")
        if (NOT X86_64)
            # MSVC enables SSE2 by default on X86_64
            target_compile_options(mango PUBLIC "/arch:SSE2")
        endif ()
    endif ()

else ()

    set(CMAKE_CXX_FLAGS "-Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

    target_compile_definitions(mango PUBLIC $<$<PLATFORM_ID:Windows>:UNICODE>)

    if (CYGWIN)
        target_compile_options(mango PUBLIC "-municode")
    endif ()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(mango PUBLIC "-ftree-vectorize")
        set(CMAKE_CXX_FLAGS "-Wno-psabi")
    endif ()

    if ((ARM AND ENABLE_NEON) OR AARCH64)
        message("SIMD: NEON")
        if (ARM)
            if (APPLE)
                # fastgltf doesn't compile without this on Apple Silicon
                set(CMAKE_CXX_FLAGS "-Wno-unknown-attributes")
            else ()
                # The compiler for Apple Silicon does not recognize these
                add_definitions (-mfpu=neon -mfloat-abi=hard)
            endif ()
            set(CMAKE_CXX_FLAGS "-fpermissive -Wno-psabi")
        endif ()
    endif ()

    if (X86 OR X86_64)

        # enable AES (2010)
        if (ENABLE_AES)
            message("Enable: AES (2010)")
            target_compile_options(mango PUBLIC "-maes")
        endif ()

        # enable CLMUL (2008)
        if (ENABLE_PCLMUL)
            message("Enable: PCLMUL (2008)")
            target_compile_options(mango PUBLIC "-mpclmul")
        endif ()

        # popcnt
        if (ENABLE_POPCNT)
            message("Enable: POPCNT (2008)")
            target_compile_options(mango PUBLIC "-mpopcnt")
        endif ()

        # half conversion instructions
        if (ENABLE_F16C)
            message("Enable: F16C (2012)")
            target_compile_options(mango PUBLIC "-mf16c")
        endif ()

        # bit manipulation instruction set 1
        if (ENABLE_BMI)
            message("Enable: BMI+LZCNT (2013)")
            target_compile_options(mango PUBLIC "-mbmi")
            target_compile_options(mango PUBLIC "-mlzcnt")
        endif ()

        # bit manipulation instruction set 2
        if (ENABLE_BMI2)
            message("Enable: BMI2 (2013)")
            target_compile_options(mango PUBLIC "-mbmi2")
        endif ()

        # fused multiply-add
        if (ENABLE_FMA)
            message("Enable: FMA (2013)")
            target_compile_options(mango PUBLIC "-mfma")
        endif ()

        # sha
        if (ENABLE_SHA)
            message("Enable: SHA (2013)")
            target_compile_options(mango PUBLIC "-msha")
        endif ()

        # enable only one (the most recent) SIMD extension
        if (ENABLE_AVX512)
            message("SIMD: AVX-512 (2015)")
            target_compile_options(mango PUBLIC "-mavx512dq")
            target_compile_options(mango PUBLIC "-mavx512vl")
            target_compile_options(mango PUBLIC "-mavx512bw")
        elseif (ENABLE_AVX2)
            message("SIMD: AVX2 (2013)")
            target_compile_options(mango PUBLIC "-mavx2")
        elseif (ENABLE_AVX)
            message("SIMD: AVX (2008)")
            target_compile_options(mango PUBLIC "-mavx")
        elseif (ENABLE_SSE4)
            message("SIMD: SSE4.2 (2006)")
            target_compile_options(mango PUBLIC "-msse4")
        elseif (ENABLE_SSE2)
            message("SIMD: SSE2 (2001)")
            target_compile_options(mango PUBLIC "-msse2")
        endif ()
    endif ()
endif ()

message("OPENGL: " ${BUILD_OPENGL})
message("IMPORT3D: " ${BUILD_IMPORT3D})
message("EXAMPLES: " ${BUILD_EXAMPLES})

# ------------------------------------------------------------------------------
# install target
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

    set(CMAKE_INSTALL_PREFIX "${DEFAULT_INSTALL_PATH}" CACHE PATH "Install path prefix" FORCE)

endif ()

message("Installation path: ${CMAKE_INSTALL_PREFIX}")

# ------------------------------------------------------------------------------
# install
# ------------------------------------------------------------------------------

target_include_directories(mango
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    INTERFACE
        $<INSTALL_INTERFACE:include/mango/>
)

target_include_directories(mango
    PRIVATE
        "${PROJECT_SOURCE_DIR}/source/external/fastgltf/include"
        "${PROJECT_SOURCE_DIR}/source/external/simdjson"
)

set_target_properties(mango PROPERTIES INTERFACE_LINK_LIBRARIES "")

# install mango headers
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*"
)

install(TARGETS mango
    EXPORT "mango-config"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
)

install(
    EXPORT "mango-config"
    DESTINATION "lib/cmake/mango"
)

list(POP_BACK CMAKE_MESSAGE_INDENT)

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

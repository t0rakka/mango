CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(mango CXX C ASM)

# ------------------------------------------------------------------------------
# version
# ------------------------------------------------------------------------------

set(MANGO_VERSION_MAJOR 1)
set(MANGO_VERSION_MINOR 3)
set(MANGO_VERSION_PATCH 0)
set(MANGO_VERSION_STRING ${MANGO_VERSION_MAJOR}.${MANGO_VERSION_MINOR}.${MANGO_VERSION_PATCH})

set(MANGO_OPENGL_VERSION_MAJOR 0)
set(MANGO_OPENGL_VERSION_MINOR 8)
set(MANGO_OPENGL_VERSION_PATCH 3)
set(MANGO_OPENGL_VERSION_STRING ${MANGO_OPENGL_VERSION_MAJOR}.${MANGO_OPENGL_VERSION_MINOR}.${MANGO_OPENGL_VERSION_PATCH})

set(MANGO_IMPORT3D_VERSION_MAJOR 0)
set(MANGO_IMPORT3D_VERSION_MINOR 1)
set(MANGO_IMPORT3D_VERSION_PATCH 4)
set(MANGO_IMPORT3D_VERSION_STRING ${MANGO_IMPORT3D_VERSION_MAJOR}.${MANGO_IMPORT3D_VERSION_MINOR}.${MANGO_IMPORT3D_VERSION_PATCH})

# ------------------------------------------------------------------------------
# compiler
# ------------------------------------------------------------------------------

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    set(COMPILER_MSVC 1)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(COMPILER_CLANG 1)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
    set(COMPILER_INTEL 1)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    set(COMPILER_GCC 1)
else()
    message(STATUS "Compiler is not supported!")
endif()

# ------------------------------------------------------------------------------
# processor
# ------------------------------------------------------------------------------

message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
    set(X86_64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i686.*|i386.*|x86.*|amd64.*|AMD64.*")
    set(X86 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*)")
    set(AARCH64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)")
    set(ARM 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "PPC64*|ppc64*|powerpc64*")
    set(PPC64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "PPC*|ppc*|powerpc*")
    set(PPC 1)
else()
    message(STATUS "Processor is not supported!")
endif()

# ------------------------------------------------------------------------------
# build
# ------------------------------------------------------------------------------

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build: ${CMAKE_BUILD_TYPE}")

# ------------------------------------------------------------------------------
# options
# ------------------------------------------------------------------------------

OPTION(BUILD_SHARED_LIBS    "Build as shared library (so/dll/dylib)"    ON)

OPTION(ENABLE_SSE2          "Enable SSE2 instructions"                  OFF)
OPTION(ENABLE_SSE4          "Enable SSE4.1 and SSE4.2 instructions"     ON)
OPTION(ENABLE_AVX           "Enable AVX instructions"                   OFF)
OPTION(ENABLE_AVX2          "Enable AVX2 instructions"                  OFF)
OPTION(ENABLE_AVX512        "Enable AVX-512 instructions"               OFF)
OPTION(ENABLE_NEON          "Enable ARM NEON instructions"              ON)
OPTION(ENABLE_F16C          "Enable F16C conversion instructions"       OFF)
OPTION(ENABLE_BMI           "Enable BMI"                                OFF)
OPTION(ENABLE_BMI2          "Enable BMI2"                               OFF)
OPTION(ENABLE_FMA           "Enable FMA"                                OFF)

# ------------------------------------------------------------------------------
# source directories
# ------------------------------------------------------------------------------

if (WIN32 OR CYGWIN)

    FILE(GLOB CORE_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/core/*.hpp"
    )
    FILE(GLOB FILESYSTEM_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/*.hpp"
    )
    FILE(GLOB WINDOW_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/window/*.hpp"
    )
    FILE(GLOB OPENGL_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/opengl/*.hpp"
    )

    FILE(GLOB CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/win32/*.cpp"
    )
    FILE(GLOB FILESYSTEM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/win32/*.cpp"
    )
    FILE(GLOB WINDOW_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/window/win32/*.cpp"
    )
    FILE(GLOB OPENGL_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/wgl/*.cpp"
    )

elseif (APPLE)

    FILE(GLOB CORE_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/core/*.hpp"
    )
    FILE(GLOB FILESYSTEM_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/*.hpp"
    )
    FILE(GLOB WINDOW_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/window/*.hpp"
    )
    FILE(GLOB OPENGL_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/opengl/*.hpp"
    )

    FILE(GLOB CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/unix/*.cpp"
    )
    FILE(GLOB FILESYSTEM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/unix/*.cpp"
    )
    FILE(GLOB WINDOW_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/window/cocoa/*.mm"
    )
    FILE(GLOB OPENGL_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/cocoa/*.mm"
    )

elseif (EMSCRIPTEN)

    FILE(GLOB CORE_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/core/*.hpp"
    )
    FILE(GLOB FILESYSTEM_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/*.hpp"
    )
    FILE(GLOB OPENGL_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/opengl/*.hpp"
    )

    FILE(GLOB CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/unix/*.cpp"
    )
    FILE(GLOB FILESYSTEM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/unix/*.cpp"
    )
    FILE(GLOB OPENGL_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/*.cpp"
    )

else()

    FILE(GLOB CORE_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/core/*.hpp"
    )
    FILE(GLOB FILESYSTEM_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/*.hpp"
    )
    FILE(GLOB WINDOW_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/window/*.hpp"
    )
    FILE(GLOB OPENGL_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/opengl/*.hpp"
    )

    FILE(GLOB CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/unix/*.cpp"
    )
    FILE(GLOB FILESYSTEM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/unix/*.cpp"
    )
    FILE(GLOB WINDOW_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/window/xlib/*.cpp"
    )
    FILE(GLOB OPENGL_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/glx/*.cpp"
    )

endif()

SET(MAIN_HEADER             "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/mango.hpp")
FILE(GLOB IMAGE_HEADERS     "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/image/*.hpp")
FILE(GLOB JPEG_HEADERS      "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/jpeg/*.hpp")
FILE(GLOB MATH_HEADERS      "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/math/*.hpp")
FILE(GLOB SIMD_HEADERS      "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/simd/*.hpp")
FILE(GLOB IMPORT3D_HEADERS  "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/import3d/*.hpp")

FILE(GLOB IMAGE_SOURCES     "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/image/*.cpp")
FILE(GLOB JPEG_SOURCES      "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/jpeg/*.cpp")
FILE(GLOB MATH_SOURCES      "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/math/*.cpp")
FILE(GLOB SIMD_SOURCES      "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/simd/*.cpp")
FILE(GLOB IMPORT3D_SOURCES  "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/import3d/*.cpp")

SOURCE_GROUP("include"            FILES ${MAIN_HEADER})
SOURCE_GROUP("include/core"       FILES ${CORE_HEADERS})
SOURCE_GROUP("include/filesystem" FILES ${FILESYSTEM_HEADERS})
SOURCE_GROUP("include/image"      FILES ${IMAGE_HEADERS})
SOURCE_GROUP("include/jpeg"       FILES ${JPEG_HEADERS})
SOURCE_GROUP("include/math"       FILES ${MATH_HEADERS})
SOURCE_GROUP("include/simd"       FILES ${SIMD_HEADERS})
SOURCE_GROUP("include/window"     FILES ${WINDOW_HEADERS})
SOURCE_GROUP("include/opengl"     FILES ${OPENGL_HEADERS})
SOURCE_GROUP("include/import3d"   FILES ${IMPORT3D_HEADERS})

SOURCE_GROUP("source/core"        FILES ${CORE_SOURCES})
SOURCE_GROUP("source/filesystem"  FILES ${FILESYSTEM_SOURCES})
SOURCE_GROUP("source/image"       FILES ${IMAGE_SOURCES})
SOURCE_GROUP("source/jpeg"        FILES ${JPEG_SOURCES})
SOURCE_GROUP("source/math"        FILES ${MATH_SOURCES})
SOURCE_GROUP("source/simd"        FILES ${SIMD_SOURCES})
SOURCE_GROUP("source/window"      FILES ${WINDOW_SOURCES})
SOURCE_GROUP("source/opengl"      FILES ${OPENGL_SOURCES})
SOURCE_GROUP("source/import3d"    FILES ${IMPORT3D_SOURCES})

FILE(GLOB EXTERNAL_LZMA
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzma/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzma/*.c"
)
FILE(GLOB EXTERNAL_AES
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/aes/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/aes/*.cpp"
)
FILE(GLOB EXTERNAL_BC
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bc/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bc/*.cpp"
)
FILE(GLOB EXTERNAL_BZIP2
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bzip2/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bzip2/*.c"
)
FILE(GLOB EXTERNAL_CONCURRENT_QUEUE
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/concurrentqueue/*.h"
)
FILE(GLOB EXTERNAL_ASTC
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/astc/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/astc/*.h"
)
FILE(GLOB EXTERNAL_GOOGLE
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/google/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/google/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/google/*.h"
)
FILE(GLOB EXTERNAL_LZ4
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lz4/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lz4/*.c"
)
FILE(GLOB EXTERNAL_LZFSE
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzfse/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzfse/*.c"
)
FILE(GLOB EXTERNAL_LZO
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzo/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzo/*.c"
)
FILE(GLOB EXTERNAL_DEFLATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/common/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/lib/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/lib/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/lib/x86/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/lib/x86/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/lib/arm/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/lib/arm/*.c"
)
FILE(GLOB EXTERNAL_ZLIB
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zlib/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zlib/*.c"
)
FILE(GLOB EXTERNAL_LCMS
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lcms/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lcms/*.c"
)
FILE(GLOB EXTERNAL_UNRAR
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/unrar/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/unrar/*.cpp"
)

if (X86_64)
    FILE(GLOB_RECURSE EXTERNAL_ZSTD
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zstd/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zstd/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zstd/*.S"
    )
else ()
    FILE(GLOB_RECURSE EXTERNAL_ZSTD
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zstd/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zstd/*.c"
    )
endif ()

FILE(GLOB_RECURSE EXTERNAL_WEBP
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libwebp/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libwebp/src/*.c"
)
FILE(GLOB EXTERNAL_ZPNG
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zpng/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zpng/*.cpp"
)
FILE(GLOB EXTERNAL_BASISU
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/basisu/transcoder/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/basisu/transcoder/*.cpp"
)
FILE(GLOB EXTERNAL_FMT
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/fmt/src/*.cc"
)
FILE(GLOB EXTERNAL_IMPORT3D
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/mikktspace/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/mikktspace/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/simdjson/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/simdjson/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/fastgltf/include/fastgltf/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/fastgltf/src/*.cpp"
)

SOURCE_GROUP("external" FILES
    ${EXTERNAL_LZMA}
    ${EXTERNAL_AES}
    ${EXTERNAL_BC}
    ${EXTERNAL_BZIP2}
    ${EXTERNAL_CONCURRENT_QUEUE}
    ${EXTERNAL_ASTC}
    ${EXTERNAL_GOOGLE}
    ${EXTERNAL_LZ4}
    ${EXTERNAL_LZFSE}
    ${EXTERNAL_LZO}
    ${EXTERNAL_DEFLATE}
    ${EXTERNAL_ZLIB}
    ${EXTERNAL_LCMS}
    ${EXTERNAL_UNRAR}
    ${EXTERNAL_ZSTD}
    ${EXTERNAL_WEBP}
    ${EXTERNAL_ZPNG}
    ${EXTERNAL_BASISU}
    ${EXTERNAL_FMT}
    ${EXTERNAL_IMPORT3D}
)

# ------------------------------------------------------------------------------
# libraries
# ------------------------------------------------------------------------------

if (BUILD_SHARED_LIBS AND COMPILER_MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

add_library(mango
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
    ${EXTERNAL_LZ4}
    ${EXTERNAL_LZFSE}
    ${EXTERNAL_LZO}
    ${EXTERNAL_DEFLATE}
    ${EXTERNAL_ZLIB}
    ${EXTERNAL_LCMS}
    ${EXTERNAL_UNRAR}
    ${EXTERNAL_ZSTD}
    ${EXTERNAL_WEBP}
    ${EXTERNAL_ZPNG}
    ${EXTERNAL_BASISU}
    ${EXTERNAL_FMT}
)

set_target_properties(mango PROPERTIES
    VERSION ${MANGO_VERSION_STRING}
    SOVERSION ${MANGO_VERSION_MAJOR}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_library(mango-opengl
    ${WINDOW_SOURCES}
    ${OPENGL_SOURCES}
)

set_target_properties(mango-opengl PROPERTIES
    VERSION ${MANGO_OPENGL_VERSION_STRING}
    SOVERSION ${MANGO_OPENGL_VERSION_MAJOR}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(mango-opengl PUBLIC mango)

add_library(mango-import3d
    ${IMPORT3D_HEADERS}
    ${IMPORT3D_SOURCES}
    ${EXTERNAL_IMPORT3D}
)

set_target_properties(mango-import3d PROPERTIES
    VERSION ${MANGO_IMPORT3D_VERSION_STRING}
    SOVERSION ${MANGO_IMPORT3D_VERSION_MAJOR}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(mango-import3d PUBLIC mango)

if (APPLE)
    if (BUILD_SHARED_LIBS)
        target_link_libraries(mango-opengl PUBLIC "-framework Cocoa" "-framework OpenGL")
    endif ()

    target_compile_options(mango PUBLIC "-mmacosx-version-min=10.15")
    target_compile_options(mango-opengl PUBLIC "-mmacosx-version-min=10.15")
endif ()

# add "-lpthread" linker option when platform has support
find_package(Threads)
if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(mango "${CMAKE_THREAD_LIBS_INIT}")
endif ()

# ------------------------------------------------------------------------------
# external libraries
# ------------------------------------------------------------------------------

if (BUILD_SHARED_LIBS AND WIN32)

    set(OpenGL_GL_PREFERENCE LEGACY)
    find_package(OpenGL REQUIRED)

    if (OPENGL_FOUND)
        message(STATUS "Enable: OpenGL")
        target_link_libraries(mango ${OPENGL_gl_LIBRARY})
    endif()

else ()

    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(OpenGL)

    if (OPENGL_FOUND)
        message(STATUS "Enable: OpenGL")
        message(STATUS "Enable: XLIB")
        target_link_libraries(mango ${OPENGL_gl_LIBRARY})
        target_link_libraries(mango ${OPENGL_glx_LIBRARY})
        target_compile_definitions(mango PUBLIC "-DMANGO_ENABLE_XLIB")
    endif()

endif ()

# ------------------------------------------------------------------------------
# configuration
# ------------------------------------------------------------------------------

if (EMSCRIPTEN)

    message(STATUS "SIMD: WASM")
    target_compile_options(mango PUBLIC "-msimd128")

elseif (COMPILER_MSVC)

    target_compile_options(mango PUBLIC "/DUNICODE")
    target_compile_options(mango PUBLIC "/MP")

    if (ENABLE_AVX512)
        message(STATUS "SIMD: AVX-512 (2015)")
        target_compile_definitions(mango PUBLIC "-D__AVX512F__" "-D__AVX512DQ__")
    elseif (ENABLE_AVX2)
        message(STATUS "SIMD: AVX2 (2013)")
        target_compile_options(mango PUBLIC "/arch:AVX2")
    elseif (ENABLE_AVX)
        message(STATUS "SIMD: AVX (2008)")
        target_compile_options(mango PUBLIC "/arch:AVX")
    elseif (ENABLE_SSE2 OR ENABLE_SSE4)
        # MSVC does not have SSE4 compiler switch; at least enable SSE2
        message(STATUS "SIMD: SSE2 (2001)")
        if (NOT X86_64)
            # MSVC enables SSE2 by default on X86_64
            target_compile_options(mango PUBLIC "/arch:SSE2")
        endif ()
    endif ()

else ()

    set(CMAKE_CXX_FLAGS "-Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    #set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

    target_compile_definitions(mango PUBLIC $<$<PLATFORM_ID:Windows>:UNICODE>)

    if (CYGWIN)
        target_compile_options(mango PUBLIC "-municode")
    endif ()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(mango PUBLIC "-ftree-vectorize")
        set(CMAKE_CXX_FLAGS "-Wno-psabi")
    endif ()

    if ((ARM AND ENABLE_NEON) OR AARCH64)
        message(STATUS "SIMD: NEON")
        if (ARM)
            if (NOT APPLE)
                # The compiler for Apple Mx CPUs don't recognize these
                add_definitions (-mfpu=neon -mfloat-abi=hard)
            endif()
            set(CMAKE_CXX_FLAGS "-fpermissive -Wno-psabi")
            # fastgltf doesn't compile without this on M1
            set(CMAKE_CXX_FLAGS "-Wno-unknown-attributes")
        endif ()
    endif ()

    if (X86 OR X86_64)
        # enable AES (2008) by default
        target_compile_options(mango PUBLIC "-maes")

        # enable CLMUL (2008) by default
        target_compile_options(mango PUBLIC "-mpclmul")

        # half conversion instructions
        if (ENABLE_F16C)
            message(STATUS "F16C: half conversion enabled (2012)")
            target_compile_options(mango PUBLIC "-mf16c")
        endif ()

        # bit manipulation instruction set 1
        if (ENABLE_BMI)
            message(STATUS "BMI+LZCNT: enabled (2013)")
            target_compile_options(mango PUBLIC "-mbmi")
            target_compile_options(mango PUBLIC "-mlzcnt")
        endif ()

        # bit manipulation instruction set 2
        if (ENABLE_BMI2)
            message(STATUS "BMI2: enabled (2013)")
            target_compile_options(mango PUBLIC "-mbmi2")
        endif ()

        # fused multiply-add
        if (ENABLE_FMA)
            message(STATUS "FMA: enabled (2013)")
            target_compile_options(mango PUBLIC "-mfma")
        endif ()

        # enable only one (the most recent) SIMD extension
        if (ENABLE_AVX512)
            message(STATUS "SIMD: AVX-512 (2015)")
            target_compile_options(mango PUBLIC "-mavx512dq")
            target_compile_options(mango PUBLIC "-mavx512vl")
            target_compile_options(mango PUBLIC "-mavx512bw")
        elseif (ENABLE_AVX2)
            message(STATUS "SIMD: AVX2 (2013)")
            target_compile_options(mango PUBLIC "-mavx2")
        elseif (ENABLE_AVX)
            message(STATUS "SIMD: AVX (2008)")
            target_compile_options(mango PUBLIC "-mavx")
        elseif (ENABLE_SSE4)
            message(STATUS "SIMD: SSE4.2 (2006)")
            target_compile_options(mango PUBLIC "-msse4")
        elseif (ENABLE_SSE2)
            message(STATUS "SIMD: SSE2 (2001)")
            target_compile_options(mango PUBLIC "-msse2")
        endif ()
    endif ()
endif ()

# ------------------------------------------------------------------------------
# install
# ------------------------------------------------------------------------------

target_include_directories(mango
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../include/>
  INTERFACE
    $<INSTALL_INTERFACE:include/mango/>
  PRIVATE
    "${PROJECT_SOURCE_DIR}/../source/external/libwebp"
)

target_include_directories(mango-import3d
  PRIVATE
    "${PROJECT_SOURCE_DIR}/../source/external/fastgltf/include"
    "${PROJECT_SOURCE_DIR}/../source/external/simdjson"
)

install(TARGETS mango
    EXPORT "mango-config"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
)

install(TARGETS mango-opengl
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
)

install(TARGETS mango-import3d
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
)

install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/../include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*"
)

install(
    EXPORT "mango-config"
    DESTINATION "lib/cmake/mango"
)

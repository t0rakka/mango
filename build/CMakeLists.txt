CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(mango CXX C)

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

OPTION(BUILD_SHARED_LIBS    "Build as shared library (so/dll/dylib)"    OFF)

OPTION(ENABLE_FAST_MATH     "Use relaxed-precision floating point"      ON)
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
# include directories
# ------------------------------------------------------------------------------

include_directories("../include/")
include_directories("../source/external/libwebp/")

# ------------------------------------------------------------------------------
# source directories
# ------------------------------------------------------------------------------

if (WIN32)

    FILE(GLOB CORE
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/core/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/win32/*.cpp")
    FILE(GLOB FILESYSTEM
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/win32/*.cpp")
    FILE(GLOB WINDOW
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/window/win32/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/window/win32/*.cpp")
    FILE(GLOB OPENGL
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/opengl/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/wgl/*.cpp")

elseif (APPLE)

    FILE(GLOB CORE
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/core/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/unix/*.cpp")
    FILE(GLOB FILESYSTEM
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/unix/*.cpp")
    FILE(GLOB WINDOW
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/window/cocoa/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/window/cocoa/*.mm")
    FILE(GLOB OPENGL
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/opengl/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/cocoa/*.mm")

else()

    FILE(GLOB CORE
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/core/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/unix/*.cpp")
    FILE(GLOB FILESYSTEM
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/unix/*.cpp")
    FILE(GLOB WINDOW
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/window/xlib/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/window/xlib/*.cpp")
    FILE(GLOB OPENGL
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/opengl/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/glx/*.cpp")

endif()

FILE(GLOB IMAGE
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/image/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/image/*.cpp")
FILE(GLOB JPEG
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/jpeg/*.cpp")
FILE(GLOB MATH
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/math/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/math/*.cpp")
FILE(GLOB SIMD
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/simd/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/simd/*.cpp")

SOURCE_GROUP("core" FILES ${CORE})
SOURCE_GROUP("filesystem" FILES ${FILESYSTEM})
SOURCE_GROUP("image" FILES ${IMAGE})
SOURCE_GROUP("jpeg" FILES ${JPEG})
SOURCE_GROUP("math" FILES ${MATH})
SOURCE_GROUP("simd" FILES ${SIMD})
SOURCE_GROUP("window" FILES ${WINDOW})
SOURCE_GROUP("opengl" FILES ${OPENGL})

FILE(GLOB EXTERNAL_LZMA
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzma/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzma/*.c")
FILE(GLOB EXTERNAL_AES
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/aes/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/aes/*.cpp")
FILE(GLOB EXTERNAL_BC
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bc/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bc/*.cpp")
FILE(GLOB EXTERNAL_BZIP2
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bzip2/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bzip2/*.c")
FILE(GLOB EXTERNAL_CONCURRENT_QUEUE
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/concurrentqueue/*.h")
FILE(GLOB EXTERNAL_GOOGLE
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/google/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/google/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/google/*.h")
FILE(GLOB EXTERNAL_LZ4
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lz4/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lz4/*.c")
FILE(GLOB EXTERNAL_LZFSE
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzfse/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzfse/*.c")
FILE(GLOB EXTERNAL_LZO
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzo/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzo/*.c")
FILE(GLOB EXTERNAL_DEFLATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/common/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/lib/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/lib/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/lib/x86/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/lib/x86/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/lib/arm/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libdeflate/lib/arm/*.c")
FILE(GLOB EXTERNAL_UNRAR
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/unrar/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/unrar/*.cpp")
FILE(GLOB_RECURSE EXTERNAL_ZSTD
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zstd/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zstd/*.c")
FILE(GLOB_RECURSE EXTERNAL_WEBP
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libwebp/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/libwebp/src/*.c")
FILE(GLOB EXTERNAL_ZPNG
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zpng/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zpng/*.cpp")

SOURCE_GROUP("external" FILES
    ${EXTERNAL_LZMA}
    ${EXTERNAL_AES}
    ${EXTERNAL_BC}
    ${EXTERNAL_BZIP2}
    ${EXTERNAL_CONCURRENT_QUEUE}
    ${EXTERNAL_GOOGLE}
    ${EXTERNAL_LZ4}
    ${EXTERNAL_LZFSE}
    ${EXTERNAL_LZO}
    ${EXTERNAL_DEFLATE}
    ${EXTERNAL_UNRAR}
    ${EXTERNAL_ZSTD}
    ${EXTERNAL_WEBP}
    ${EXTERNAL_ZPNG}
)

# ------------------------------------------------------------------------------
# libraries
# ------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

ADD_LIBRARY(mango
    ${CORE}
    ${FILESYSTEM}
    ${IMAGE}
    ${JPEG}
    ${MATH}
    ${SIMD}
    ${EXTERNAL_LZMA}
    ${EXTERNAL_AES}
    ${EXTERNAL_BC}
    ${EXTERNAL_BZIP2}
    ${EXTERNAL_CONCURRENT_QUEUE}
    ${EXTERNAL_GOOGLE}
    ${EXTERNAL_LZ4}
    ${EXTERNAL_LZFSE}
    ${EXTERNAL_LZO}
    ${EXTERNAL_DEFLATE}
    ${EXTERNAL_UNRAR}
    ${EXTERNAL_ZSTD}
    ${EXTERNAL_WEBP}
    ${EXTERNAL_ZPNG}
)

ADD_LIBRARY(mango-opengl
    ${WINDOW}
    ${OPENGL}
)

target_link_libraries(mango-opengl PUBLIC mango)

if (APPLE)
    if (BUILD_SHARED_LIBS)
        target_link_libraries(mango-opengl PUBLIC "-framework Cocoa" "-framework OpenGL")
    endif ()

    target_compile_options(mango PUBLIC "-mmacosx-version-min=10.13")
    target_compile_options(mango-opengl PUBLIC "-mmacosx-version-min=10.13")
endif ()

# add "-lpthread" linker option when platform has support
find_package(Threads)
if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(mango "${CMAKE_THREAD_LIBS_INIT}")
endif ()

# ------------------------------------------------------------------------------
# configuration
# ------------------------------------------------------------------------------

if (COMPILER_MSVC)
    target_compile_options(mango PUBLIC "/DUNICODE")

    if (ENABLE_FAST_MATH)
        target_compile_options(mango PUBLIC "/fp:fast")
    endif ()

    if (ENABLE_AVX512)
        message(STATUS "SIMD: AVX-512 (2015)")
        target_compile_definitions(mango PUBLIC "-D__AVX512F__" "-D__AVX512DQ__")
    elseif (ENABLE_AVX2)
        message(STATUS "SIMD: AVX2 (2013)")
        target_compile_options(mango PUBLIC "/arch:AVX2")
    elseif (ENABLE_AVX)
        message(STATUS "SIMD: AVX (2008)")
        target_compile_options(mango PUBLIC "/arch:AVX")
    endif ()
else ()
    set(CMAKE_CXX_FLAGS "-Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")

    target_compile_definitions(mango PUBLIC $<$<PLATFORM_ID:Windows>:UNICODE>)

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(mango PUBLIC "-ftree-vectorize")
    endif ()

    if (ENABLE_FAST_MATH)
        target_compile_options(mango PUBLIC "-ffast-math")
    endif ()

    if ((ARM AND ENABLE_NEON) OR AARCH64)
        message(STATUS "SIMD: NEON")
        if (ARM)
            if (!APPLE)
                # The compiler for Apple Mx CPUs don't recognize these
                add_definitions (-mfpu=neon -mfloat-abi=hard)
            endif()
            set(CMAKE_CXX_FLAGS "-fpermissive -Wno-psabi")
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

TARGET_INCLUDE_DIRECTORIES(mango PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mango>
)

INSTALL(TARGETS mango
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
)

INSTALL(TARGETS mango-opengl
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
)

INSTALL(
    DIRECTORY ${CMAKE_SOURCE_DIR}/../include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*"
)

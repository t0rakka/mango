CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(mango CXX C)

# ------------------------------------------------------------------------------
# compiler
# ------------------------------------------------------------------------------

message("-- Compiler is ${CMAKE_CXX_COMPILER_ID}")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    set(COMPILER_MSVC 1)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(COMPILER_CLANG 1)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
    set(COMPILER_INTEL 1)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    set(COMPILER_GCC 1)
else()
    message("-- Compiler is not supported!")
endif()

# ------------------------------------------------------------------------------
# processor
# ------------------------------------------------------------------------------

message("-- Processor is ${CMAKE_SYSTEM_PROCESSOR}")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
    set(X86_64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i686.*|i386.*|x86.*|amd64.*|AMD64.*")
    set(X86 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)")
    set(ARM 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*)")
    set(AARCH64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "PPC64*|ppc64*|powerpc64*")
    set(PPC64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "PPC*|ppc*|powerpc*")
    set(PPC 1)
else()
    message("-- Processor is not supported!")
endif()

# ------------------------------------------------------------------------------
# source directories
# ------------------------------------------------------------------------------

include_directories("../include/")

FILE(GLOB CORE "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/core/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/*.cpp")
FILE(GLOB FILESYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/*.cpp")
IF(WIN32)
    FILE(GLOB FILESYSTEM_PLATFORM "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/win32/*.hpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/win32/*.cpp")
    FILE(GLOB GUI "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/gui/win32/*.hpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/gui/win32/*.cpp")
    FILE(GLOB OPENGL "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/opengl/*.hpp" 
                     "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/*.cpp"
                     "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/wgl/*.cpp")
ELSEIF(APPLE)
    FILE(GLOB FILESYSTEM_PLATFORM "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/unix/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/unix/*.cpp")
    FILE(GLOB OPENGL "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/opengl/*.hpp" 
                     "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/*.cpp"
                     "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/cocoa/*.mm")
ELSE()
    FILE(GLOB FILESYSTEM_PLATFORM "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/unix/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/unix/*.cpp")
    FILE(GLOB GUI "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/gui/xlib/*.hpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/gui/xlib/*.cpp")
    FILE(GLOB OPENGL "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/opengl/*.hpp" 
                     "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/*.cpp"
                     "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/glx/*.cpp")
ENDIF()
FILE(GLOB IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/image/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/image/*.cpp")
FILE(GLOB JPEG "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/jpeg/*.cpp")
FILE(GLOB MATH "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/math/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/math/*.cpp")
FILE(GLOB SIMD "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/simd/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/simd/*.cpp")
FILE(GLOB VULKAN "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/vulkan/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/vulkan/*.cpp")

SOURCE_GROUP("core" FILES ${CORE})
SOURCE_GROUP("filesystem" FILES ${FILESYSTEM} ${FILESYSTEM_PLATFORM})
SOURCE_GROUP("image" FILES ${IMAGE} ${JPEG})
SOURCE_GROUP("math" FILES ${MATH})
SOURCE_GROUP("gui" FILES ${GUI})
SOURCE_GROUP("opengl" FILES ${OPENGL})
SOURCE_GROUP("vulkan" FILES ${VULKAN})

FILE(GLOB AES "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/aes/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/aes/*.cpp")
FILE(GLOB BC "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bc/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bc/*.cpp")
FILE(GLOB BZIP2 "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bzip2/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bzip2/*.c")
FILE(GLOB CONCURRENT_QUEUE "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/google/*.h")
FILE(GLOB GOOGLE "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/google/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/google/etc1.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/google/*.cpp")
FILE(GLOB LZ4 "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lz4/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lz4/*.c")
FILE(GLOB LZFSE "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzfse/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzfse/*.c")
FILE(GLOB LZO "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzo/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzo/*.c")
FILE(GLOB MINIZ "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/miniz/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/miniz/*.c")
FILE(GLOB UNRAR "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/unrar/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/unrar/*.cpp")
FILE(GLOB_RECURSE ZSTD "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zstd/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zstd/*.c")
FILE(GLOB_RECURSE ZPNG "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zpng/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zpng/*.cpp")

SOURCE_GROUP("external" FILES ${AES} ${BC} ${BZIP2} ${CONCURRENT_QUEUE} ${GOOGLE} ${LZ4} ${LZFSE} ${LZO} ${MINIZ} ${UNRAR} ${ZSTD} ${ZPNG})

# ------------------------------------------------------------------------------
# libraries
# ------------------------------------------------------------------------------

ADD_LIBRARY(mango
    ${CORE} ${FILESYSTEM} ${FILESYSTEM_PLATFORM} ${IMAGE} ${JPEG} ${MATH} ${SIMD}
    ${AES} ${BC} ${BZIP2} ${CONCURRENT_QUEUE} ${GOOGLE} ${LZ4} ${LZFSE} ${LZO} ${MINIZ} ${UNRAR} ${ZSTD} ${ZPNG}
    )

ADD_LIBRARY(mango-opengl
    ${GUI}
    ${OPENGL}
    )

ADD_LIBRARY(mango-vulkan
    ${GUI}
    ${VULKAN}
    )

target_link_libraries(mango-opengl PUBLIC mango)
target_link_libraries(mango-vulkan PUBLIC mango)

if (APPLE AND BUILD_SHARED_LIBS)
    target_link_libraries(mango-opengl PUBLIC "-framework Cocoa" "-framework OpenGL")
endif ()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

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

# ------------------------------------------------------------------------------
# configuration
# ------------------------------------------------------------------------------

if(COMPILER_MSVC)
    # /Gm enables partial rebuild, is all.
    TARGET_COMPILE_OPTIONS(mango PUBLIC "/std:c++14" "/Gm")
    IF (ENABLE_FAST_MATH)
        TARGET_COMPILE_OPTIONS(mango PUBLIC "/fp:fast")
    ENDIF()

    TARGET_COMPILE_OPTIONS(mango PUBLIC "/DUNICODE")

    if (ENABLE_AVX512)
        message("-- SIMD: AVX-512 (2015)")
        TARGET_COMPILE_DEFINITIONS(mango PUBLIC "-D__AVX512F__" "-D__AVX512DQ__")
    elseif (ENABLE_AVX2)
        message("-- SIMD: AVX2 (2013)")
        TARGET_COMPILE_OPTIONS(mango PUBLIC "/arch:AVX2")
    elseif (ENABLE_AVX)
        message("-- SIMD: AVX (2008)")
        TARGET_COMPILE_OPTIONS(mango PUBLIC "/arch:AVX")
    endif()
ELSE()
    set(CMAKE_CXX_FLAGS "-Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")

    # Property only works for unix-style systems iirc
    SET_PROPERTY(TARGET mango        PROPERTY CXX_STANDARD 14)
    SET_PROPERTY(TARGET mango-opengl PROPERTY CXX_STANDARD 14)
    SET_PROPERTY(TARGET mango-vulkan PROPERTY CXX_STANDARD 14)

    TARGET_COMPILE_DEFINITIONS(mango PUBLIC $<$<PLATFORM_ID:Windows>:UNICODE>)

    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        TARGET_COMPILE_OPTIONS(mango PUBLIC "-ftree-vectorize")
    ENDIF()

    IF (ENABLE_FAST_MATH) 
        TARGET_COMPILE_OPTIONS(mango PUBLIC "-ffast-math")
    ENDIF()

    if (ARM OR ARCH64)
        add_definitions (-mfpu=neon -mfloat-abi=hard)
        set(CMAKE_CXX_FLAGS "-fpermissive -Wno-psabi")
    endif()

    if (X86 OR X86_64)
        # enable AES (2008) by default
        TARGET_COMPILE_OPTIONS(mango PUBLIC "-maes")

        # enable only one (the most recent) SIMD extension
        if (ENABLE_AVX512)
            message("-- SIMD: AVX-512 (2015)")
            TARGET_COMPILE_OPTIONS(mango PUBLIC "-mavx512dq")
            TARGET_COMPILE_OPTIONS(mango PUBLIC "-mavx512vl")
            TARGET_COMPILE_OPTIONS(mango PUBLIC "-mavx512bw")
        elseif (ENABLE_AVX2)
            message("-- SIMD: AVX2 (2013)")
            TARGET_COMPILE_OPTIONS(mango PUBLIC "-mavx2")
        elseif (ENABLE_AVX)
            message("-- SIMD: AVX (2008)")
            TARGET_COMPILE_OPTIONS(mango PUBLIC "-mavx")
        elseif (ENABLE_SSE4)
            message("-- SIMD: SSE4.2 (2006)")
            TARGET_COMPILE_OPTIONS(mango PUBLIC "-msse4")
        elseif (ENABLE_SSE2)
            message("-- SIMD: SSE2 (2001)")
            TARGET_COMPILE_OPTIONS(mango PUBLIC "-msse2")
        endif()
    endif()
ENDIF()

TARGET_INCLUDE_DIRECTORIES(mango PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mango>)

INSTALL(TARGETS mango mango-opengl mango-vulkan LIBRARY DESTINATION "lib" ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin")

INSTALL(
    DIRECTORY ${CMAKE_SOURCE_DIR}/../include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*")

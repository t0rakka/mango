CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(mango CXX C)

FILE(GLOB CORE "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/core/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/core/*.cpp")
FILE(GLOB FILESYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/*.cpp")
IF(WIN32)
    FILE(GLOB FILESYSTEM_PLATFORM "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/win32/*.hpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/win32/*.cpp")
ELSE()
    FILE(GLOB FILESYSTEM_PLATFORM "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/filesystem/unix/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/filesystem/unix/*.cpp")
ENDIF()
FILE(GLOB IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/image/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/image/*.cpp")
FILE(GLOB JPEG "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/jpeg/*.cpp")
FILE(GLOB MATH "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/math/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/math/*.cpp")
FILE(GLOB OPENGL "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/opengl/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/opengl/*.cpp")
FILE(GLOB SIMD "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/simd/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/simd/*.cpp")
FILE(GLOB VULKAN "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/vulkan/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/mango/vulkan/*.cpp")

SOURCE_GROUP("core" FILES ${CORE})
SOURCE_GROUP("filesystem" FILES ${FILESYSTEM} ${FILESYSTEM_PLATFORM})
SOURCE_GROUP("image" FILES ${IMAGE} ${JPEG})
SOURCE_GROUP("math" FILES ${MATH})
SOURCE_GROUP("opengl" FILES ${OPENGL})
SOURCE_GROUP("vulkan" FILES ${VULKAN})

FILE(GLOB AES "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/aes/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/aes/*.cpp")
FILE(GLOB BC "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/bc/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bc/*.cpp")
FILE(GLOB BZIP2 "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/bzip2/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/bzip2/*.c")
FILE(GLOB CONCURRENT_QUEUE "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/google/*.h")
FILE(GLOB GOOGLE "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/google/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/google/etc1.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/google/*.cpp")
FILE(GLOB LZ4 "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/lz4/*h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lz4/*.c")
FILE(GLOB LZFSE "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/lzfse/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzfse/*.c")
FILE(GLOB LZO "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/lzo/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/lzo/*.c")
FILE(GLOB miniz "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/miniz/miniz.cpp")
FILE(GLOB UNRAR "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/unrar/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/unrar/*.cpp")
FILE(GLOB_RECURSE ZSTD "${CMAKE_CURRENT_SOURCE_DIR}/../include/external/zstd/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../source/external/zstd/*.c")

ADD_LIBRARY(mango STATIC "${CMAKE_CURRENT_SOURCE_DIR}/../include/mango/mango.hpp" ${CORE} ${FILESYSTEM} ${FILESYSTEM_PLATFORM} 
    ${IMAGE} ${JPEG} ${MATH} ${OPENGL} ${SIMD} ${VULKAN} ${AES} ${BC} ${BZIP2} ${CONCURRENT_QUEUE} ${GOOGLE} ${LZ4} ${LZFSE} ${LZO} 
    ${miniz} ${UNRAR} ${ZSTD})

OPTION(ENABLE_FAST_MATH "Use fast math / relaxed-precision floating point operations" ON)
OPTION(ENABLE_AVX2 "Enable AVX2 instructions. Most x64 CPUs should have this." ON)

IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # /Gm enables partial rebuild, is all.
    TARGET_COMPILE_OPTIONS(mango PRIVATE "/std:c++14" "/Gm")
    IF (ENABLE_FAST_MATH)
        TARGET_COMPILE_OPTIONS(mango PRIVATE "/fp:fast")
    ENDIF()
    IF (ENABLE_AVX2) 
        TARGET_COMPILE_OPTIONS(mango PRIVATE "/arch:AVX2")
    ENDIF()
ELSE()
    # Property only works for unix-style systems iirc
    SET_PROPERTY(TARGET mango PROPERTY CXX_STANDARD 14)
    IF (ENABLE_FAST_MATH) 
        TARGET_COMPILE_OPTIONS(mango PRIVATE "-ffast-math")
    ENDIF()
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        TARGET_COMPILE_OPTIONS(mango PRIVATE "-ftree-vectorize")
    ENDIF()
ENDIF()

TARGET_INCLUDE_DIRECTORIES(mango PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mango>)

OPTION(USE_AVX512 "Enable AVX-512 intrinsics if the current hardware supports it" OFF)
IF(USE_AVX512)
    TARGET_COMPILE_DEFINITIONS(mango PRIVATE "-D__AVX512F__" "-D__AVX512DQ__")
ENDIF()

INSTALL(DIRECTORY "include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mango/")
INSTALL(TARGETS mango LIBRARY DESTINATION "lib" ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin")
INSTALL(FILES "include/mango/mango.hpp" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mango")


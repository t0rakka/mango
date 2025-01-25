# ----------------------------------------------------------------------
# NOTE:
# apt-get install libpng-dev libz-dev
#
# TODO:
# - Clean up
# - Fix png_benchmark Windows build
# - Fix find_package() for libpng, libz
# - Compiler options below (-mavx, -mavx2, -mpclmul)
# - Fix find_package(mango) ; when uses libwebp it can't be found even if installed with vcpkg (uses .pc instead of .cmake)
# ----------------------------------------------------------------------

cmake_minimum_required(VERSION 3.10)
project(mango-examples)

# build options
option(BUILD_OPENGL      "Enables build of OpenGL examples"      OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    #add_compile_options(-mavx2)
    #add_compile_options(-mavx2 -mpclmul)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "(arm)")
    if (!APPLE)
        # The compiler for Apple Mx CPUs don't recognize these
        add_definitions (-mfpu=neon -mfloat-abi=hard)
    endif()
    set(CMAKE_CXX_FLAGS "-fpermissive -Wno-psabi")
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    #add_compile_options(/Ox)
    add_compile_options(""
        "$<$<CONFIG:Release>:/Ox>"
    )
else ()
    add_compile_options(-Wall -O3)
    #set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()

# TODO: fix this
#find_package(mango REQUIRED)
link_libraries(mango)

if (APPLE)
    # fix wrapped compiler dropping /usr/local
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/local/include")
    link_directories("/usr/local/lib")
endif ()

# ------------------------------------------------------------------------------
# package finding
# ------------------------------------------------------------------------------

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    #message(STATUS "PkgConfig: " ${PKG_CONFIG_VERSION_STRING})
endif ()

function (find_module_with_pkg target name module)
    if (PkgConfig_FOUND)
        pkg_check_modules(${name} QUIET ${module})
        if (${name}_FOUND)
            message("   + " ${name} ": " ${${name}_VERSION})
            target_compile_definitions(${target} PUBLIC "MANGO_ENABLE_${name}")
            target_include_directories(${target} PRIVATE ${${name}_INCLUDE_DIRS})
            target_link_directories(${target} PRIVATE ${${name}_LIBRARY_DIRS})
            target_link_libraries(${target} PRIVATE ${${name}_LIBRARIES})
        endif ()
    endif ()
endfunction ()

function (find_module_with_cmake target name module libname version)
    if (NOT ${name}_FOUND)
        if (version STREQUAL "0")
            find_package(${module} CONFIG QUIET)
        else ()
            find_package(${module} ${version} CONFIG QUIET)
        endif ()

        if (${module}_FOUND)
            message("   + " ${name} ": " ${${module}_VERSION})
            target_compile_definitions(${target} PUBLIC "MANGO_ENABLE_${name}")
            target_link_libraries(${target} PRIVATE ${libname})
        endif ()
    endif ()
endfunction ()

# ----------------------------------------------------------------------
# build examples + utilities
# ----------------------------------------------------------------------

add_subdirectory(test)
add_subdirectory(misc)
add_subdirectory(image)
add_subdirectory(utils)

# ----------------------------------------------------------------------
# build OpenGL examples
# ----------------------------------------------------------------------

if (BUILD_OPENGL)
    add_subdirectory(opengl)
endif ()

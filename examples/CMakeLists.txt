cmake_minimum_required(VERSION 3.14)
project(mango-examples)

link_libraries(mango)

# build options
option(BUILD_OPENGL      "Enables build of OpenGL examples"      OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(arm)")
    if (!APPLE)
        # The compiler for Apple Mx CPUs don't recognize these
        add_definitions (-mfpu=neon -mfloat-abi=hard)
    endif()
    set(CMAKE_CXX_FLAGS "-fpermissive -Wno-psabi")
endif ()

if (MSVC)
    add_compile_options("$<$<CONFIG:Release>:/Ox>")
else ()
    add_compile_options(-Wall -O3)
endif ()

find_package(mango REQUIRED)
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
    message(STATUS "PkgConfig: " ${PKG_CONFIG_VERSION_STRING})
endif ()

function (find_module_with_pkg Target Prefix ItemAndVersion)
    if (PkgConfig_FOUND)
        pkg_check_modules(${Prefix} QUIET ${ItemAndVersion})
        if (${Prefix}_FOUND)
            set(${Prefix}_ENABLE 1 PARENT_SCOPE)
            message("   + " ${Prefix} ": " ${${Prefix}_VERSION} " [pkgconfig]")
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
            message("   + " ${Prefix} ": " ${${PackageName}_VERSION} " [cmake]")
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

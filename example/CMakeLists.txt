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
    add_compile_options(/Ox)
else ()
    add_compile_options(-Wall -O3)
    #set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()


#find_package(mango REQUIRED)
link_libraries(mango)

# add "-lpthread" linker option when platform has support
if (UNIX)
    #set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    #find_package(Threads REQUIRED)
    #if (CMAKE_USE_PTHREADS_INIT)
    #    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    #endif ()
    #link_libraries("${CMAKE_THREAD_LIBS_INIT}")
endif ()

if (APPLE)
    # fix wrapped compiler dropping /usr/local
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/local/include")
    link_directories(/usr/local/lib)

    if (CMAKE_SYSTEM_PROCESSOR MATCHES "(arm)")
        # fix brew paths on M1 silicon (when libraries were compiled with meson)
        include_directories("/opt/homebrew/include")
        link_directories("/opt/homebrew/lib")
    endif ()
elseif (UNIX)
    #link_libraries(dl)
endif ()

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

    # TODO: link to mango-opengl
    if (WIN32)
        #link_libraries(opengl32)
    endif ()

endif ()

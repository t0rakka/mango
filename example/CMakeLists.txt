# ----------------------------------------------------------------------
# NOTE:
# apt-get install libpng-dev libz-dev
#
# ----------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)
project(mango-examples)

# build options
option(BUILD_OPENGL      "Enables build of OpenGL examples"      OFF)
option(BUILD_FRAMEBUFFER "Enables build of framebuffer examples" OFF)

add_compile_options(-Wall -O3 -g)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    add_compile_options(-mavx)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "(arm)")
    add_definitions(-mfpu=neon -mfloat-abi=hard)
    set(CMAKE_CXX_FLAGS "-fpermissive -Wno-psabi")
endif ()

link_libraries(mango)

# add "-lpthread" linker option when platform has support
if (UNIX)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    find_package(Threads REQUIRED)
    if (CMAKE_USE_PTHREADS_INIT)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    endif ()
    link_libraries("${CMAKE_THREAD_LIBS_INIT}")
endif ()

if (APPLE)
    # fix wrapped compiler dropping /usr/local
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/local/include")
    link_directories(/usr/local/lib)
else ()
    link_libraries(dl)
endif ()

# ----------------------------------------------------------------------
# build
# ----------------------------------------------------------------------

add_subdirectory(test)
add_subdirectory(misc)
add_subdirectory(image)

# ----------------------------------------------------------------------
# build framebuffer
# ----------------------------------------------------------------------

if (BUILD_FRAMEBUFFER)
    add_subdirectory(framebuffer)
endif ()

# ----------------------------------------------------------------------
# build opengl
# ----------------------------------------------------------------------

if (BUILD_OPENGL)
    add_subdirectory(opengl)
endif ()

# NOTE:
# apt-get install libpng-dev libz-dev
#
cmake_minimum_required(VERSION 3.5)
project(mango-examples)

add_compile_options(-Wall -O3 -g)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    add_compile_options(-mavx)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "(arm)|(aarch64)")
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

add_executable(threads  test/threads.cpp)
add_executable(pathtest test/pathtest.cpp)
add_executable(particle test/particle.cpp)

add_executable(bitmap      misc/bitmap.cpp)
add_executable(checksum    misc/checksum.cpp)
add_executable(compress    misc/compress.cpp)
add_executable(concurrency misc/concurrency.cpp)
add_executable(endian      misc/endian.cpp)
add_executable(filesystem  misc/filesystem.cpp)
add_executable(math        misc/math.cpp)
add_executable(memory      misc/memory.cpp)

add_executable(webp_test image/webp/webp.cpp)
add_executable(jpeg_test image/jpeg/jpeg.cpp)
add_executable(png_benchmark
    image/png_benchmark/png_benchmark.cpp
    image/png_benchmark/lodepng/lodepng.cpp
    image/png_benchmark/spng/spng.c
)
add_executable(jpeg_benchmark
    image/jpeg_benchmark/jpeg_benchmark.cpp
    image/jpeg_benchmark/jpeg-compressor/jpgd.cpp
    image/jpeg_benchmark/jpeg-compressor/jpge.cpp
)

add_executable(framebuffer framebuffer/framebuffer.cpp)
add_executable(mandelbrot  framebuffer/mandelbrot.cpp)
add_executable(animation   framebuffer/animation.cpp)

add_executable(gltest opengl/gltest.cpp)

target_link_libraries(png_benchmark png z)
target_link_libraries(jpeg_benchmark jpeg)

if (APPLE)
    target_link_libraries(framebuffer mango-framebuffer)
    target_link_libraries(mandelbrot  mango-framebuffer)
    target_link_libraries(animation   mango-framebuffer)
    target_link_libraries(gltest mango-opengl "-framework OpenGL")
elseif (UNIX)
    target_link_libraries(framebuffer mango-framebuffer X11)
    target_link_libraries(mandelbrot  mango-framebuffer X11)
    target_link_libraries(animation   mango-framebuffer X11)
    target_link_libraries(gltest mango-opengl X11 GL)
endif ()

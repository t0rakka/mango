
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
    add_compile_options(-mavx2 -mpclmul)
endif()

add_executable(webp_test webp_test.cpp)
add_executable(bulk_decode bulk_decode.cpp)
add_executable(icc_p3_test icc/p3.cpp)
add_executable(blitter blitter/blitter.cpp)
add_executable(palette palette.cpp)
add_executable(async_decode async_decode.cpp)

file(COPY icc/DisplayP3-v2-micro.icc DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY blitter/conquer.jpg DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(png_benchmark
    png_benchmark/png_benchmark.cpp
    png_benchmark/lodepng/lodepng.cpp
    png_benchmark/spng/spng.c
    png_benchmark/fpng/fpng.cpp
    png_benchmark/fpnge/fpnge.cpp
)
target_link_libraries(png_benchmark png z)

add_executable(jpeg_benchmark
    jpeg_benchmark/jpeg_benchmark.cpp
    jpeg_benchmark/jpeg-compressor/jpgd.cpp
    jpeg_benchmark/jpeg-compressor/jpge.cpp
    jpeg_benchmark/jpegdec/JPEGDEC.cpp
    jpeg_benchmark/toojpeg/toojpeg.cpp
)
target_link_libraries(jpeg_benchmark jpeg)

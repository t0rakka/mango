
add_executable(webp_test webp/webp.cpp)
add_executable(jpeg_test jpeg/jpeg.cpp)

add_executable(png_benchmark
    png_benchmark/png_benchmark.cpp
    png_benchmark/lodepng/lodepng.cpp
    png_benchmark/spng/spng.c
)
target_link_libraries(png_benchmark png z)

add_executable(jpeg_benchmark
    jpeg_benchmark/jpeg_benchmark.cpp
    jpeg_benchmark/jpeg-compressor/jpgd.cpp
    jpeg_benchmark/jpeg-compressor/jpge.cpp
)
target_link_libraries(jpeg_benchmark jpeg)

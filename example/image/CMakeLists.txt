
add_executable(webp_test webp_test.cpp)
add_executable(bulk_decode bulk_decode.cpp)
add_executable(icc_p3_test icc/p3.cpp)
add_executable(blitter blitter/blitter.cpp)
add_executable(palette palette.cpp)
add_executable(async_decode async_decode.cpp)

file(COPY icc/DisplayP3-v2-micro.icc DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY blitter/conquer.jpg DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# TODO: fix Windows build
if (UNIX)
    add_executable(png_benchmark
        png_benchmark/png_benchmark.cpp
        png_benchmark/lodepng/lodepng.cpp
        png_benchmark/spng/spng.c
        png_benchmark/fpng/fpng.cpp
        png_benchmark/fpnge/fpnge.cpp
    )
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
        target_compile_options(png_benchmark PRIVATE -mavx2 -mpclmul)
    endif ()
endif ()

if (UNIX)
    #target_link_libraries(png_benchmark png z)
else ()
    #target_link_libraries(png_benchmark libpng16)
    #target_compile_options(png_benchmark PRIVATE "/DSPNG_STATIC")
endif ()

#find_package(ZLIB REQUIRED)
#target_link_libraries(png_benchmark PRIVATE ZLIB::ZLIB)

find_package(PNG REQUIRED)
target_link_libraries(png_benchmark PNG::PNG)

add_executable(jpeg_benchmark
    jpeg_benchmark/jpeg_benchmark.cpp
    jpeg_benchmark/jpeg-compressor/jpgd.cpp
    jpeg_benchmark/jpeg-compressor/jpge.cpp
    jpeg_benchmark/toojpeg/toojpeg.cpp
)
#target_link_libraries(jpeg_benchmark jpeg)

find_package(JPEG REQUIRED)
target_link_libraries(jpeg_benchmark JPEG::JPEG)

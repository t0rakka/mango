
add_executable(webp_test webp_test.cpp)
add_executable(bulk_decode bulk_decode.cpp)
add_executable(icc_p3_test icc/p3.cpp)
add_executable(blitter blitter/blitter.cpp)
add_executable(palette palette.cpp)
add_executable(async_decode async_decode.cpp)

set_target_properties(webp_test PROPERTIES FOLDER "examples/image")
set_target_properties(bulk_decode PROPERTIES FOLDER "examples/image")
set_target_properties(icc_p3_test PROPERTIES FOLDER "examples/image")
set_target_properties(blitter PROPERTIES FOLDER "examples/image")
set_target_properties(palette PROPERTIES FOLDER "examples/image")
set_target_properties(async_decode PROPERTIES FOLDER "examples/image")

file(COPY icc/DisplayP3-v2-micro.icc DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY blitter/conquer.jpg DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# ----------------------------------------------------------------------
# png_benchmark
# ----------------------------------------------------------------------

add_executable(png_benchmark
    png_benchmark/png_benchmark.cpp
    png_benchmark/lodepng/lodepng.cpp
    png_benchmark/spng/spng.c
    png_benchmark/fpng/fpng.cpp
    png_benchmark/fpnge/fpnge.cpp
)

set_target_properties(png_benchmark PROPERTIES FOLDER "examples/image")

if (MSVC)

    target_compile_definitions(png_benchmark PUBLIC "__PCLMUL__")
    target_compile_options(png_benchmark PRIVATE "/arch:AVX2")

else ()

    if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
        target_compile_options(png_benchmark PRIVATE -mavx2 -mpclmul)
    endif ()

endif ()

find_module(png_benchmark BLEND2D PRIVATE OPTIONAL "" blend2d blend2d::blend2d 0)
find_module(png_benchmark PNG PRIVATE REQUIRED libpng PNG PNG::PNG 0)
find_module(png_benchmark ZLIB PRIVATE REQUIRED zlib ZLIB ZLIB::ZLIB 0)

target_compile_definitions(png_benchmark PRIVATE "SPNG_STATIC")

# ----------------------------------------------------------------------
# jpeg_benchmark
# ----------------------------------------------------------------------

add_executable(jpeg_benchmark
    jpeg_benchmark/jpeg_benchmark.cpp
    jpeg_benchmark/jpeg-compressor/jpgd.cpp
    jpeg_benchmark/jpeg-compressor/jpge.cpp
    jpeg_benchmark/toojpeg/toojpeg.cpp
)

set_target_properties(jpeg_benchmark PROPERTIES FOLDER "examples/image")

if (MSVC)

    target_compile_options(jpeg_benchmark PRIVATE "/arch:AVX2")

endif ()

find_module(jpeg_benchmark BLEND2D PRIVATE OPTIONAL "" blend2d blend2d::blend2d 0)
find_module(jpeg_benchmark JPEG PRIVATE REQUIRED libjpeg JPEG JPEG::JPEG 0)

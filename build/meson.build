# ---------------------------------------------------------------------------
# mango meson.build
#
# Copyright (C) 2012-2021 Twilight 3D Finland Oy Ltd.
# ---------------------------------------------------------------------------

project(
    'mango', 'c', 'cpp',
    meson_version: '>= 0.48.0',
    default_options : [
        'c_std=c11',
        'cpp_std=c++17',
        'objcpp_args=-std=c++17 -Wno-deprecated',
        'default_library=static'
    ]
)

if host_machine.system() == 'darwin'
    add_languages('objcpp')
endif

enable_fast_math = get_option('enable_fast_math')
enable_sse2      = get_option('enable_sse2')
enable_sse4      = get_option('enable_sse4')
enable_avx       = get_option('enable_avx')
enable_avx2      = get_option('enable_avx2')
enable_avx512    = get_option('enable_avx512')
enable_neon      = get_option('enable_neon')
enable_f16c      = get_option('enable_f16c')
enable_bmi       = get_option('enable_bmi')
enable_bmi2      = get_option('enable_bmi2')
enable_fma       = get_option('enable_fma')

is_arm = host_machine.cpu_family() == 'arm' or host_machine.cpu_family() == 'aarch64'
is_x86 = host_machine.cpu_family() == 'x86' or host_machine.cpu_family() == 'x86_64'

is_windows = host_machine.system() == 'windows'
is_darwin = host_machine.system() == 'darwin'

mango_public_cpp_args = []
mango_interface_cpp_args = []

# ------------------------------------------------------------------------------
# source directories
# ------------------------------------------------------------------------------

mango_headers = files(
    '../include/mango/mango.hpp',
    '../include/mango/core/aes.hpp',
    '../include/mango/core/atomic.hpp',
    '../include/mango/core/bits.hpp',
    '../include/mango/core/buffer.hpp',
    '../include/mango/core/compress.hpp',
    '../include/mango/core/configure.hpp',
    '../include/mango/core/core.hpp',
    '../include/mango/core/cpuinfo.hpp',
    '../include/mango/core/crc32.hpp',
    '../include/mango/core/dynamic_library.hpp',
    '../include/mango/core/endian.hpp',
    '../include/mango/core/exception.hpp',
    '../include/mango/core/half.hpp',
    '../include/mango/core/hash.hpp',
    '../include/mango/core/memory.hpp',
    '../include/mango/core/pointer.hpp',
    '../include/mango/core/stream.hpp',
    '../include/mango/core/string.hpp',
    '../include/mango/core/system.hpp',
    '../include/mango/core/thread.hpp',
    '../include/mango/core/timer.hpp',
    '../include/mango/filesystem/file.hpp',
    '../include/mango/filesystem/fileobserver.hpp',
    '../include/mango/filesystem/filesystem.hpp',
    '../include/mango/filesystem/mapper.hpp',
    '../include/mango/filesystem/path.hpp',
    '../include/mango/image/blitter.hpp',
    '../include/mango/image/color.hpp',
    '../include/mango/image/compression.hpp',
    '../include/mango/image/decoder.hpp',
    '../include/mango/image/encoder.hpp',
    '../include/mango/image/exif.hpp',
    '../include/mango/image/format.hpp',
    '../include/mango/image/fourcc.hpp',
    '../include/mango/image/image.hpp',
    '../include/mango/image/quantize.hpp',
    '../include/mango/image/surface.hpp',
    '../include/mango/math/accessor.hpp',
    '../include/mango/math/geometry.hpp',
    '../include/mango/math/math.hpp',
    '../include/mango/math/matrix4x4.hpp',
    '../include/mango/math/matrix.hpp',
    '../include/mango/math/quaternion.hpp',
    '../include/mango/math/spline.hpp',
    '../include/mango/math/srgb.hpp',
    '../include/mango/math/vector_float16x4.hpp',
    '../include/mango/math/vector_float32x16.hpp',
    '../include/mango/math/vector_float32x2.hpp',
    '../include/mango/math/vector_float32x3.hpp',
    '../include/mango/math/vector_float32x4.hpp',
    '../include/mango/math/vector_float32x8.hpp',
    '../include/mango/math/vector_float64x2.hpp',
    '../include/mango/math/vector_float64x4.hpp',
    '../include/mango/math/vector_float64x8.hpp',
    '../include/mango/math/vector_gather.hpp',
    '../include/mango/math/vector.hpp',
    '../include/mango/math/vector128_int16x8.hpp',
    '../include/mango/math/vector128_int32x4.hpp',
    '../include/mango/math/vector128_int64x2.hpp',
    '../include/mango/math/vector128_int8x16.hpp',
    '../include/mango/math/vector128_uint16x8.hpp',
    '../include/mango/math/vector128_uint32x4.hpp',
    '../include/mango/math/vector128_uint64x2.hpp',
    '../include/mango/math/vector128_uint8x16.hpp',
    '../include/mango/math/vector256_int16x16.hpp',
    '../include/mango/math/vector256_int32x8.hpp',
    '../include/mango/math/vector256_int64x4.hpp',
    '../include/mango/math/vector256_int8x32.hpp',
    '../include/mango/math/vector256_uint16x16.hpp',
    '../include/mango/math/vector256_uint32x8.hpp',
    '../include/mango/math/vector256_uint64x4.hpp',
    '../include/mango/math/vector256_uint8x32.hpp',
    '../include/mango/math/vector512_int16x32.hpp',
    '../include/mango/math/vector512_int32x16.hpp',
    '../include/mango/math/vector512_int64x8.hpp',
    '../include/mango/math/vector512_int8x64.hpp',
    '../include/mango/math/vector512_uint16x32.hpp',
    '../include/mango/math/vector512_uint32x16.hpp',
    '../include/mango/math/vector512_uint64x8.hpp',
    '../include/mango/math/vector512_uint8x64.hpp',
    '../include/mango/opengl/func/glcorearb.hpp',
    '../include/mango/opengl/func/glext.hpp',
    '../include/mango/opengl/func/glxext.hpp',
    '../include/mango/opengl/func/wglext.hpp',
    '../include/mango/opengl/khronos/EGL/egl.h',
    '../include/mango/opengl/khronos/EGL/eglext.h',
    '../include/mango/opengl/khronos/EGL/eglplatform.h',
    '../include/mango/opengl/khronos/GL/glcorearb.h',
    '../include/mango/opengl/khronos/GL/glext.h',
    '../include/mango/opengl/khronos/GL/glxext.h',
    '../include/mango/opengl/khronos/GL/wgl.h',
    '../include/mango/opengl/khronos/GL/wglext.h',
    '../include/mango/opengl/khronos/GLES/egl.h',
    '../include/mango/opengl/khronos/GLES/gl.h',
    '../include/mango/opengl/khronos/GLES/glext.h',
    '../include/mango/opengl/khronos/GLES/glplatform.h',
    '../include/mango/opengl/khronos/GLES2/gl2.h',
    '../include/mango/opengl/khronos/GLES2/gl2ext.h',
    '../include/mango/opengl/khronos/GLES2/gl2platform.h',
    '../include/mango/opengl/khronos/GLES3/gl3.h',
    '../include/mango/opengl/khronos/GLES3/gl31.h',
    '../include/mango/opengl/khronos/GLES3/gl32.h',
    '../include/mango/opengl/khronos/GLES3/gl3platform.h',
    '../include/mango/opengl/khronos/KHR/khrplatform.h',
    '../include/mango/opengl/opengl.hpp',
    '../include/mango/simd/altivec_convert.hpp',
    '../include/mango/simd/altivec_double128.hpp',
    '../include/mango/simd/altivec_float128.hpp',
    '../include/mango/simd/altivec_int128.hpp',
    '../include/mango/simd/avx_convert.hpp',
    '../include/mango/simd/avx_double256.hpp',
    '../include/mango/simd/avx_float256.hpp',
    '../include/mango/simd/avx_int256.hpp',
    '../include/mango/simd/avx2_gather.hpp',
    '../include/mango/simd/avx2_int256.hpp',
    '../include/mango/simd/avx512_convert.hpp',
    '../include/mango/simd/avx512_double128.hpp',
    '../include/mango/simd/avx512_double256.hpp',
    '../include/mango/simd/avx512_double512.hpp',
    '../include/mango/simd/avx512_float128.hpp',
    '../include/mango/simd/avx512_float256.hpp',
    '../include/mango/simd/avx512_float512.hpp',
    '../include/mango/simd/avx512_gather.hpp',
    '../include/mango/simd/avx512_int128.hpp',
    '../include/mango/simd/avx512_int256.hpp',
    '../include/mango/simd/avx512_int512.hpp',
    '../include/mango/simd/common_gather.hpp',
    '../include/mango/simd/common_mask.hpp',
    '../include/mango/simd/common.hpp',
    '../include/mango/simd/composite_double256.hpp',
    '../include/mango/simd/composite_double512.hpp',
    '../include/mango/simd/composite_float256.hpp',
    '../include/mango/simd/composite_float512.hpp',
    '../include/mango/simd/composite_int256.hpp',
    '../include/mango/simd/composite_int512.hpp',
    '../include/mango/simd/msa_convert.hpp',
    '../include/mango/simd/msa_double128.hpp',
    '../include/mango/simd/msa_float128.hpp',
    '../include/mango/simd/msa_int128.hpp',
    '../include/mango/simd/neon_convert.hpp',
    '../include/mango/simd/neon_double128.hpp',
    '../include/mango/simd/neon_float128.hpp',
    '../include/mango/simd/neon_int128.hpp',
    '../include/mango/simd/scalar_convert.hpp',
    '../include/mango/simd/scalar_detail.hpp',
    '../include/mango/simd/scalar_double128.hpp',
    '../include/mango/simd/scalar_float128.hpp',
    '../include/mango/simd/scalar_float64.hpp',
    '../include/mango/simd/scalar_int128.hpp',
    '../include/mango/simd/scalar_int64.hpp',
    '../include/mango/simd/simd.hpp',
    '../include/mango/simd/sse2_convert.hpp',
    '../include/mango/simd/sse2_double128.hpp',
    '../include/mango/simd/sse2_float128.hpp',
    '../include/mango/simd/sse2_int128.hpp',
    '../include/mango/window/window.hpp',
)

core_sources = files(
    '../source/mango/core/aes.cpp',
    '../source/mango/core/buffer.cpp',
    '../source/mango/core/compress.cpp',
    '../source/mango/core/cpuinfo.cpp',
    '../source/mango/core/crc32.cpp',
    '../source/mango/core/hash.cpp',
    '../source/mango/core/md5.cpp',
    '../source/mango/core/memory.cpp',
    '../source/mango/core/sha1.cpp',
    '../source/mango/core/sha2.cpp',
    '../source/mango/core/string.cpp',
    '../source/mango/core/system.cpp',
    '../source/mango/core/thread.cpp',
    '../source/mango/core/timer.cpp'
)

if is_windows
    core_sources += files(
        '../source/mango/core/win32/dynamic_library.cpp'
    )
else
    core_sources += files(
        '../source/mango/core/unix/dynamic_library.cpp'
    )
endif

filesystem_sources = files(
    '../source/mango/filesystem/file.cpp',
    '../source/mango/filesystem/mapper.cpp',
    '../source/mango/filesystem/mapper_mgx.cpp',
    '../source/mango/filesystem/mapper_rar.cpp',
    '../source/mango/filesystem/mapper_zip.cpp',
    '../source/mango/filesystem/path.cpp'
)

if is_windows
    filesystem_sources += files(
        '../source/mango/filesystem/win32/file_observer.cpp',
        '../source/mango/filesystem/win32/file_stream.cpp',
        '../source/mango/filesystem/win32/mapper_file.cpp'
    )
else
    filesystem_sources += files(
        '../source/mango/filesystem/unix/file_observer.cpp',
        '../source/mango/filesystem/unix/file_stream.cpp',
        '../source/mango/filesystem/unix/mapper_file.cpp'
    )
endif

opengl_sources = files(
    '../source/mango/opengl/opengl.cpp'
)

if is_windows
    window_sources = files(
        '../source/mango/window/win32/win32_window.cpp'
    )
    opengl_sources += files(
        '../source/mango/opengl/wgl/wgl_context.cpp'
    )
elif is_darwin
    window_sources = files(
        '../source/mango/window/cocoa/cocoa_window.mm'
    )
    opengl_sources += files(
        '../source/mango/opengl/cocoa/cocoa_context.mm',
        '../source/mango/opengl/cocoa/CustomOpenGLView.mm'
    )
else
    window_sources = files(
        '../source/mango/window/xlib/xlib_window.cpp'
    )
    opengl_sources += files(
        '../source/mango/opengl/glx/glx_context.cpp'
    )
endif

image_sources = files(
    '../source/mango/image/blitter.cpp',
    '../source/mango/image/block.cpp',
    '../source/mango/image/block_dxt.cpp',
    '../source/mango/image/block_fxt1.cpp',
    '../source/mango/image/block_pvrtc.cpp',
    '../source/mango/image/block_yuv.cpp',
    '../source/mango/image/exif.cpp',
    '../source/mango/image/format.cpp',
    '../source/mango/image/image_astc.cpp',
    '../source/mango/image/image_atari.cpp',
    '../source/mango/image/image_bmp.cpp',
    '../source/mango/image/image_c64.cpp',
    '../source/mango/image/image.cpp',
    '../source/mango/image/image_dds.cpp',
    '../source/mango/image/image_gif.cpp',
    '../source/mango/image/image_hdr.cpp',
    '../source/mango/image/image_iff.cpp',
    '../source/mango/image/image_jpg.cpp',
    '../source/mango/image/image_ktx.cpp',
    '../source/mango/image/image_pcx.cpp',
    '../source/mango/image/image_pkm.cpp',
    '../source/mango/image/image_png.cpp',
    '../source/mango/image/image_pnm.cpp',
    '../source/mango/image/image_pvr.cpp',
    '../source/mango/image/image_sgi.cpp',
    '../source/mango/image/image_tga.cpp',
    '../source/mango/image/image_webp.cpp',
    '../source/mango/image/image_zpng.cpp',
    '../source/mango/image/quantize.cpp',
    '../source/mango/image/surface.cpp'
)

jpeg_sources = files(
    '../source/mango/jpeg/jpeg_arithmetic.cpp',
    '../source/mango/jpeg/jpeg_decode.cpp',
    '../source/mango/jpeg/jpeg_encode.cpp',
    '../source/mango/jpeg/jpeg_huffman.cpp',
    '../source/mango/jpeg/jpeg_idct.cpp',
    '../source/mango/jpeg/jpeg_process.cpp'
)

math_sources = files(
    '../source/mango/math/geometry.cpp',
    '../source/mango/math/math.cpp',
    '../source/mango/math/simd.cpp'
)

external_headers = files(
    '../source/external/lzfse/lzfse_fse.h',
    '../source/external/lzfse/lzfse.h',
    '../source/external/lzfse/lzfse_internal.h',
    '../source/external/lzfse/lzvn_encode_base.h',
    '../source/external/lzfse/lzvn_decode_base.h',
    '../source/external/lzfse/lzfse_tunables.h',
    '../source/external/lzfse/lzfse_encode_tables.h',
    '../source/external/libdeflate/libdeflate.h',
    '../source/external/libdeflate/common/compiler_msc.h',
    '../source/external/libdeflate/common/common_defs.h',
    '../source/external/libdeflate/common/compiler_gcc.h',
    '../source/external/libdeflate/lib/lib_common.h',
    '../source/external/libdeflate/lib/arm/crc32_impl.h',
    '../source/external/libdeflate/lib/arm/matchfinder_impl.h',
    '../source/external/libdeflate/lib/arm/cpu_features.h',
    '../source/external/libdeflate/lib/arm/adler32_impl.h',
    '../source/external/libdeflate/lib/bt_matchfinder.h',
    '../source/external/libdeflate/lib/decompress_template.h',
    '../source/external/libdeflate/lib/hc_matchfinder.h',
    '../source/external/libdeflate/lib/matchfinder_common.h',
    '../source/external/libdeflate/lib/zlib_constants.h',
    '../source/external/libdeflate/lib/gzip_constants.h',
    '../source/external/libdeflate/lib/deflate_constants.h',
    '../source/external/libdeflate/lib/adler32_vec_template.h',
    '../source/external/libdeflate/lib/x86/crc32_pclmul_template.h',
    '../source/external/libdeflate/lib/x86/decompress_impl.h',
    '../source/external/libdeflate/lib/x86/crc32_impl.h',
    '../source/external/libdeflate/lib/x86/matchfinder_impl.h',
    '../source/external/libdeflate/lib/x86/cpu_features.h',
    '../source/external/libdeflate/lib/x86/adler32_impl.h',
    '../source/external/libdeflate/lib/crc32_vec_template.h',
    '../source/external/libdeflate/lib/crc32_table.h',
    '../source/external/libdeflate/lib/deflate_compress.h',
    '../source/external/libdeflate/lib/unaligned.h',
    '../source/external/zstd/common/huf.h',
    '../source/external/zstd/common/fse.h',
    '../source/external/zstd/common/pool.h',
    '../source/external/zstd/common/xxhash.h',
    '../source/external/zstd/common/zstd_internal.h',
    '../source/external/zstd/common/bitstream.h',
    '../source/external/zstd/common/mem.h',
    '../source/external/zstd/common/threading.h',
    '../source/external/zstd/common/error_private.h',
    '../source/external/zstd/common/cpu.h',
    '../source/external/zstd/common/debug.h',
    '../source/external/zstd/common/compiler.h',
    '../source/external/zstd/compress/zstd_double_fast.h',
    '../source/external/zstd/compress/zstd_lazy.h',
    '../source/external/zstd/compress/hist.h',
    '../source/external/zstd/compress/zstd_compress_literals.h',
    '../source/external/zstd/compress/zstd_fast.h',
    '../source/external/zstd/compress/zstd_compress_internal.h',
    '../source/external/zstd/compress/zstd_ldm.h',
    '../source/external/zstd/compress/zstd_compress_sequences.h',
    '../source/external/zstd/compress/zstd_compress_superblock.h',
    '../source/external/zstd/compress/zstd_opt.h',
    '../source/external/zstd/compress/zstdmt_compress.h',
    '../source/external/zstd/compress/zstd_cwksp.h',
    '../source/external/zstd/zstd.h',
    '../source/external/zstd/zstd_errors.h',
    '../source/external/zstd/decompress/zstd_ddict.h',
    '../source/external/zstd/decompress/zstd_decompress_block.h',
    '../source/external/zstd/decompress/zstd_decompress_internal.h',
    '../source/external/aes/bc_aes.h',
    '../source/external/unrar/rarvm.hpp',
    '../source/external/unrar/suballoc.hpp',
    '../source/external/unrar/array.hpp',
    '../source/external/unrar/crypt.hpp',
    '../source/external/unrar/compress.hpp',
    '../source/external/unrar/rdwrfn.hpp',
    '../source/external/unrar/model.hpp',
    '../source/external/unrar/unpack.hpp',
    '../source/external/unrar/crc.hpp',
    '../source/external/unrar/rar.hpp',
    '../source/external/unrar/getbits.hpp',
    '../source/external/unrar/rijndael.hpp',
    '../source/external/bc/BC.h',
    '../source/external/zpng/zpng.h',
    '../source/external/bzip2/bzlib_private.h',
    '../source/external/bzip2/bzlib.h',
    '../source/external/lz4/lz4hc.h',
    '../source/external/lz4/lz4.h',
    '../source/external/libwebp/src/enc/cost_enc.h',
    '../source/external/libwebp/src/enc/histogram_enc.h',
    '../source/external/libwebp/src/enc/backward_references_enc.h',
    '../source/external/libwebp/src/enc/vp8i_enc.h',
    '../source/external/libwebp/src/enc/vp8li_enc.h',
    '../source/external/libwebp/src/mux/animi.h',
    '../source/external/libwebp/src/mux/muxi.h',
    '../source/external/libwebp/src/dsp/mips_macro.h',
    '../source/external/libwebp/src/dsp/quant.h',
    '../source/external/libwebp/src/dsp/lossless.h',
    '../source/external/libwebp/src/dsp/lossless_common.h',
    '../source/external/libwebp/src/dsp/common_sse41.h',
    '../source/external/libwebp/src/dsp/dsp.h',
    '../source/external/libwebp/src/dsp/common_sse2.h',
    '../source/external/libwebp/src/dsp/neon.h',
    '../source/external/libwebp/src/dsp/yuv.h',
    '../source/external/libwebp/src/dsp/msa_macro.h',
    '../source/external/libwebp/src/webp/mux_types.h',
    '../source/external/libwebp/src/webp/demux.h',
    '../source/external/libwebp/src/webp/encode.h',
    '../source/external/libwebp/src/webp/format_constants.h',
    '../source/external/libwebp/src/webp/decode.h',
    '../source/external/libwebp/src/webp/types.h',
    '../source/external/libwebp/src/webp/mux.h',
    '../source/external/libwebp/src/dec/webpi_dec.h',
    '../source/external/libwebp/src/dec/alphai_dec.h',
    '../source/external/libwebp/src/dec/vp8i_dec.h',
    '../source/external/libwebp/src/dec/vp8li_dec.h',
    '../source/external/libwebp/src/dec/vp8_dec.h',
    '../source/external/libwebp/src/dec/common_dec.h',
    '../source/external/libwebp/src/utils/quant_levels_dec_utils.h',
    '../source/external/libwebp/src/utils/random_utils.h',
    '../source/external/libwebp/src/utils/color_cache_utils.h',
    '../source/external/libwebp/src/utils/endian_inl_utils.h',
    '../source/external/libwebp/src/utils/utils.h',
    '../source/external/libwebp/src/utils/quant_levels_utils.h',
    '../source/external/libwebp/src/utils/bit_writer_utils.h',
    '../source/external/libwebp/src/utils/huffman_utils.h',
    '../source/external/libwebp/src/utils/bit_reader_inl_utils.h',
    '../source/external/libwebp/src/utils/bit_reader_utils.h',
    '../source/external/libwebp/src/utils/huffman_encode_utils.h',
    '../source/external/libwebp/src/utils/thread_utils.h',
    '../source/external/libwebp/src/utils/rescaler_utils.h',
    '../source/external/libwebp/src/utils/filters_utils.h',
    '../source/external/lzo/lzoconf.h',
    '../source/external/lzo/minilzo.h',
    '../source/external/lzo/lzodefs.h',
    '../source/external/concurrentqueue/atomicops.h',
    '../source/external/concurrentqueue/concurrentqueue.h',
    '../source/external/concurrentqueue/readerwriterqueue.h',
    '../source/external/lzma/CpuArch.h',
    '../source/external/lzma/Bra.h',
    '../source/external/lzma/7zCrc.h',
    '../source/external/lzma/LzmaLib.h',
    '../source/external/lzma/Ppmd7.h',
    '../source/external/lzma/BwtSort.h',
    '../source/external/lzma/Precomp.h',
    '../source/external/lzma/7zTypes.h',
    '../source/external/lzma/Compiler.h',
    '../source/external/lzma/LzmaDec.h',
    '../source/external/lzma/Sort.h',
    '../source/external/lzma/RotateDefs.h',
    '../source/external/lzma/Lzma2Enc.h',
    '../source/external/lzma/7zAlloc.h',
    '../source/external/lzma/Lzma2Dec.h',
    '../source/external/lzma/7zBuf.h',
    '../source/external/lzma/Alloc.h',
    '../source/external/lzma/Delta.h',
    '../source/external/lzma/7z.h',
    '../source/external/lzma/Bcj2.h',
    '../source/external/lzma/Blake2.h',
    '../source/external/lzma/LzHash.h',
    '../source/external/lzma/LzFind.h',
    '../source/external/lzma/LzmaEnc.h',
    '../source/external/lzma/Ppmd8.h',
    '../source/external/lzma/Ppmd.h',
    '../source/external/lzma/7zVersion.h'
)

external_lzma_sources = files(
    '../source/external/lzma/7zAlloc.c',
    '../source/external/lzma/7zArcIn.c',
    '../source/external/lzma/7zBuf2.c',
    '../source/external/lzma/7zBuf.c',
    '../source/external/lzma/7zCrc.c',
    '../source/external/lzma/7zCrcOpt.c',
    '../source/external/lzma/7zDec.c',
    '../source/external/lzma/7zStream.c',
    '../source/external/lzma/Alloc.c',
    '../source/external/lzma/Bcj2.c',
    '../source/external/lzma/Bcj2Enc.c',
    '../source/external/lzma/Blake2s.c',
    '../source/external/lzma/Bra86.c',
    '../source/external/lzma/Bra.c',
    '../source/external/lzma/BraIA64.c',
    '../source/external/lzma/BwtSort.c',
    '../source/external/lzma/CpuArch.c',
    '../source/external/lzma/Delta.c',
    '../source/external/lzma/LzFind.c',
    '../source/external/lzma/Lzma2Dec.c',
    '../source/external/lzma/Lzma2Enc.c',
    '../source/external/lzma/LzmaDec.c',
    '../source/external/lzma/LzmaEnc.c',
    '../source/external/lzma/LzmaLib.c',
    '../source/external/lzma/Ppmd7.c',
    '../source/external/lzma/Ppmd7Dec.c',
    '../source/external/lzma/Ppmd7Enc.c',
    '../source/external/lzma/Ppmd8.c',
    '../source/external/lzma/Ppmd8Dec.c',
    '../source/external/lzma/Ppmd8Enc.c',
    '../source/external/lzma/Sort.c'
)

external_aes_sources = files(
    '../source/external/aes/bc_aes.cpp'
)

external_bc_sources = files(
    '../source/external/bc/BC4BC5.cpp',
    '../source/external/bc/BC6HBC7.cpp',
    '../source/external/bc/BC.cpp'
)

external_bzip2_sources = files(
    '../source/external/bzip2/bz_blocksort.c',
    '../source/external/bzip2/bz_compress.c',
    '../source/external/bzip2/bz_crctable.c',
    '../source/external/bzip2/bz_decompress.c',
    '../source/external/bzip2/bz_huffman.c',
    '../source/external/bzip2/bzlib.c',
    '../source/external/bzip2/bz_randtable.c'
)

external_google_sources = files(
    '../source/external/google/astc.cpp',
    '../source/external/google/etc1.cpp',
    '../source/external/google/etc.cpp'
)

external_lz4_sources = files(
    '../source/external/lz4/lz4.c',
    '../source/external/lz4/lz4hc.c'
)

external_lzfse_sources = files(
    '../source/external/lzfse/lzfse_decode_base.c',
    '../source/external/lzfse/lzfse_decode.c',
    '../source/external/lzfse/lzfse_encode_base.c',
    '../source/external/lzfse/lzfse_encode.c',
    '../source/external/lzfse/lzfse_fse.c',
    '../source/external/lzfse/lzvn_decode_base.c',
    '../source/external/lzfse/lzvn_encode_base.c',
)

external_lzo_sources = files(
    '../source/external/lzo/minilzo.c'
)

external_deflate_sources = files(
    '../source/external/libdeflate/lib/deflate_compress.c',
    '../source/external/libdeflate/lib/deflate_decompress.c',
    '../source/external/libdeflate/lib/gzip_compress.c',
    '../source/external/libdeflate/lib/gzip_decompress.c',
    '../source/external/libdeflate/lib/libd_adler32.c',
    '../source/external/libdeflate/lib/libd_crc32.c',
    '../source/external/libdeflate/lib/libd_utils.c',
    '../source/external/libdeflate/lib/zlib_compress.c',
    '../source/external/libdeflate/lib/zlib_decompress.c',
)

if is_arm
    external_deflate_sources += files(
        '../source/external/libdeflate/lib/arm/cpu_arm_features.c'
    )
elif is_x86
    external_deflate_sources += files(
        '../source/external/libdeflate/lib/x86/cpu_x86_features.c',
    )
endif

external_unrar_sources = files(
    '../source/external/unrar/crc.cpp',
    '../source/external/unrar/crypt.cpp',
    '../source/external/unrar/getbits.cpp',
    '../source/external/unrar/rarvm.cpp',
    '../source/external/unrar/rdwrfn.cpp',
    '../source/external/unrar/rijndael.cpp',
    '../source/external/unrar/unpack.cpp'
)

external_zstd_sources = files(
    '../source/external/zstd/common/zstd_common.c',
    '../source/external/zstd/common/fse_decompress.c',
    '../source/external/zstd/common/pool.c',
    '../source/external/zstd/common/error_private.c',
    '../source/external/zstd/common/threading.c',
    '../source/external/zstd/common/entropy_common.c',
    '../source/external/zstd/common/debug.c',
    '../source/external/zstd/common/xxhash.c',
    '../source/external/zstd/compress/zstd_fast.c',
    '../source/external/zstd/compress/zstd_compress_literals.c',
    '../source/external/zstd/compress/huf_compress.c',
    '../source/external/zstd/compress/zstd_compress_superblock.c',
    '../source/external/zstd/compress/zstd_opt.c',
    '../source/external/zstd/compress/hist.c',
    '../source/external/zstd/compress/zstd_ldm.c',
    '../source/external/zstd/compress/fse_compress.c',
    '../source/external/zstd/compress/zstd_compress.c',
    '../source/external/zstd/compress/zstd_compress_sequences.c',
    '../source/external/zstd/compress/zstd_double_fast.c',
    '../source/external/zstd/compress/zstdmt_compress.c',
    '../source/external/zstd/compress/zstd_lazy.c',
    '../source/external/zstd/decompress/huf_decompress.c',
    '../source/external/zstd/decompress/zstd_decompress.c',
    '../source/external/zstd/decompress/zstd_decompress_block.c',
    '../source/external/zstd/decompress/zstd_ddict.c'
)

external_webp_sources = files(
    '../source/external/libwebp/src/demux/demux.c',
    '../source/external/libwebp/src/demux/anim_decode.c',
    '../source/external/libwebp/src/enc/tree_enc.c',
    '../source/external/libwebp/src/enc/webp_enc.c',
    '../source/external/libwebp/src/enc/quant_enc.c',
    '../source/external/libwebp/src/enc/predictor_enc.c',
    '../source/external/libwebp/src/enc/picture_psnr_enc.c',
    '../source/external/libwebp/src/enc/picture_csp_enc.c',
    '../source/external/libwebp/src/enc/frame_enc.c',
    '../source/external/libwebp/src/enc/picture_tools_enc.c',
    '../source/external/libwebp/src/enc/config_enc.c',
    '../source/external/libwebp/src/enc/cost_enc.c',
    '../source/external/libwebp/src/enc/token_enc.c',
    '../source/external/libwebp/src/enc/vp8l_enc.c',
    '../source/external/libwebp/src/enc/near_lossless_enc.c',
    '../source/external/libwebp/src/enc/picture_enc.c',
    '../source/external/libwebp/src/enc/histogram_enc.c',
    '../source/external/libwebp/src/enc/filter_enc.c',
    '../source/external/libwebp/src/enc/analysis_enc.c',
    '../source/external/libwebp/src/enc/backward_references_cost_enc.c',
    '../source/external/libwebp/src/enc/alpha_enc.c',
    '../source/external/libwebp/src/enc/syntax_enc.c',
    '../source/external/libwebp/src/enc/iterator_enc.c',
    '../source/external/libwebp/src/enc/picture_rescale_enc.c',
    '../source/external/libwebp/src/enc/backward_references_enc.c',
    '../source/external/libwebp/src/mux/muxinternal.c',
    '../source/external/libwebp/src/mux/anim_encode.c',
    '../source/external/libwebp/src/mux/muxedit.c',
    '../source/external/libwebp/src/mux/muxread.c',
    '../source/external/libwebp/src/dsp/upsampling_neon.c',
    '../source/external/libwebp/src/dsp/upsampling.c',
    '../source/external/libwebp/src/dsp/lossless_enc_sse2.c',
    '../source/external/libwebp/src/dsp/upsampling_msa.c',
    '../source/external/libwebp/src/dsp/cost_mips32.c',
    '../source/external/libwebp/src/dsp/enc_sse2.c',
    '../source/external/libwebp/src/dsp/lossless_enc_mips_dsp_r2.c',
    '../source/external/libwebp/src/dsp/lossless_enc_neon.c',
    '../source/external/libwebp/src/dsp/cost_mips_dsp_r2.c',
    '../source/external/libwebp/src/dsp/lossless_mips_dsp_r2.c',
    '../source/external/libwebp/src/dsp/rescaler.c',
    '../source/external/libwebp/src/dsp/cost.c',
    '../source/external/libwebp/src/dsp/enc.c',
    '../source/external/libwebp/src/dsp/rescaler_sse2.c',
    '../source/external/libwebp/src/dsp/dec_mips_dsp_r2.c',
    '../source/external/libwebp/src/dsp/enc_sse41.c',
    '../source/external/libwebp/src/dsp/filters_mips_dsp_r2.c',
    '../source/external/libwebp/src/dsp/enc_mips_dsp_r2.c',
    '../source/external/libwebp/src/dsp/dec_neon.c',
    '../source/external/libwebp/src/dsp/yuv_mips32.c',
    '../source/external/libwebp/src/dsp/lossless_enc_sse41.c',
    '../source/external/libwebp/src/dsp/lossless_enc_msa.c',
    '../source/external/libwebp/src/dsp/rescaler_neon.c',
    '../source/external/libwebp/src/dsp/alpha_processing.c',
    '../source/external/libwebp/src/dsp/dec.c',
    '../source/external/libwebp/src/dsp/lossless_neon.c',
    '../source/external/libwebp/src/dsp/yuv.c',
    '../source/external/libwebp/src/dsp/enc_neon.c',
    '../source/external/libwebp/src/dsp/rescaler_mips_dsp_r2.c',
    '../source/external/libwebp/src/dsp/lossless_msa.c',
    '../source/external/libwebp/src/dsp/enc_mips32.c',
    '../source/external/libwebp/src/dsp/dec_clip_tables.c',
    '../source/external/libwebp/src/dsp/dec_sse41.c',
    '../source/external/libwebp/src/dsp/yuv_sse41.c',
    '../source/external/libwebp/src/dsp/alpha_processing_sse2.c',
    '../source/external/libwebp/src/dsp/cpu.c',
    '../source/external/libwebp/src/dsp/filters_neon.c',
    '../source/external/libwebp/src/dsp/dec_mips32.c',
    '../source/external/libwebp/src/dsp/lossless.c',
    '../source/external/libwebp/src/dsp/yuv_sse2.c',
    '../source/external/libwebp/src/dsp/rescaler_msa.c',
    '../source/external/libwebp/src/dsp/ssim.c',
    '../source/external/libwebp/src/dsp/alpha_processing_neon.c',
    '../source/external/libwebp/src/dsp/cost_sse2.c',
    '../source/external/libwebp/src/dsp/upsampling_sse41.c',
    '../source/external/libwebp/src/dsp/ssim_sse2.c',
    '../source/external/libwebp/src/dsp/filters.c',
    '../source/external/libwebp/src/dsp/enc_msa.c',
    '../source/external/libwebp/src/dsp/dec_sse2.c',
    '../source/external/libwebp/src/dsp/lossless_sse2.c',
    '../source/external/libwebp/src/dsp/lossless_sse41.c',
    '../source/external/libwebp/src/dsp/lossless_enc.c',
    '../source/external/libwebp/src/dsp/yuv_mips_dsp_r2.c',
    '../source/external/libwebp/src/dsp/lossless_enc_mips32.c',
    '../source/external/libwebp/src/dsp/upsampling_sse2.c',
    '../source/external/libwebp/src/dsp/cost_neon.c',
    '../source/external/libwebp/src/dsp/alpha_processing_mips_dsp_r2.c',
    '../source/external/libwebp/src/dsp/alpha_processing_sse41.c',
    '../source/external/libwebp/src/dsp/upsampling_mips_dsp_r2.c',
    '../source/external/libwebp/src/dsp/rescaler_mips32.c',
    '../source/external/libwebp/src/dsp/dec_msa.c',
    '../source/external/libwebp/src/dsp/filters_sse2.c',
    '../source/external/libwebp/src/dsp/yuv_neon.c',
    '../source/external/libwebp/src/dsp/filters_msa.c',
    '../source/external/libwebp/src/dec/idec_dec.c',
    '../source/external/libwebp/src/dec/tree_dec.c',
    '../source/external/libwebp/src/dec/io_dec.c',
    '../source/external/libwebp/src/dec/webp_dec.c',
    '../source/external/libwebp/src/dec/buffer_dec.c',
    '../source/external/libwebp/src/dec/frame_dec.c',
    '../source/external/libwebp/src/dec/vp8l_dec.c',
    '../source/external/libwebp/src/dec/alpha_dec.c',
    '../source/external/libwebp/src/dec/vp8_dec.c',
    '../source/external/libwebp/src/dec/quant_dec.c',
    '../source/external/libwebp/src/utils/quant_levels_dec_utils.c',
    '../source/external/libwebp/src/utils/bit_writer_utils.c',
    '../source/external/libwebp/src/utils/huffman_utils.c',
    '../source/external/libwebp/src/utils/color_cache_utils.c',
    '../source/external/libwebp/src/utils/rescaler_utils.c',
    '../source/external/libwebp/src/utils/filters_utils.c',
    '../source/external/libwebp/src/utils/quant_levels_utils.c',
    '../source/external/libwebp/src/utils/huffman_encode_utils.c',
    '../source/external/libwebp/src/utils/utils.c',
    '../source/external/libwebp/src/utils/thread_utils.c',
    '../source/external/libwebp/src/utils/bit_reader_utils.c',
    '../source/external/libwebp/src/utils/random_utils.c'
)

external_zpng_sources = files(
    '../source/external/zpng/zpng.cpp'
)

cpp = meson.get_compiler('cpp')

message('cpp = @0@'.format(cpp.get_id()))

if host_machine.system() == 'windows'
    add_project_arguments('-DUNICODE', '-D_UNICODE', language: ['c', 'cpp'])
endif

if cpp.get_id() == 'msvc'
    if enable_fast_math
        add_project_arguments('/fp:fast', language: ['c', 'cpp'])
    endif

    if enable_avx512
        message('SIMD: AVX-512 (2015)')
        add_project_arguments('/D__AVX512F__', '/D__AVX512DQ__', language: ['c', 'cpp'])
    elif enable_avx2
        message('SIMD: AVX2 (2013)')
        add_project_arguments('/arch:AVX2', language: ['c', 'cpp'])
    elif enable_avx
        message('SIMD: AVX (2008)')
        add_project_arguments('/arch:AVX', language: ['c', 'cpp'])
    endif
else
    if cpp.get_id() == 'gcc'
        add_project_arguments('-ftree-vectorize', language: ['c', 'cpp'])
    endif

    if enable_fast_math
        add_project_arguments('-ffast-math', language: ['c', 'cpp'])
    endif

    if host_machine.cpu_family() == 'arm' and enable_neon
        message('SIMD: NEON')
        add_project_arguments('-mfpu=neon', '-mfloat-abi=hard', language: ['c', 'cpp'])
        add_project_arguments('-fpermissive', '-Wno-psabi', language: ['c', 'cpp'])
    elif host_machine.cpu_family() == 'aarch64'
        message('SIMD: NEON64')
    endif

    if host_machine.cpu_family() == 'x86' or host_machine.cpu_family() == 'x86_64'
        # enable AES (2008) by default
        add_project_arguments('-maes', language: ['c', 'cpp'])

        # enable CLMUL (2008) by default
        add_project_arguments('-mpclmul', language: ['c', 'cpp'])

        # half conversion instructions
        if enable_f16c
            message('F16C: half conversion enabled (2012)')
            add_project_arguments('-mf16c', language: ['c', 'cpp'])
        endif

        # bit manipulation instruction set 1
        if enable_bmi
            message('BMI+LZCNT: enabled (2013)')
            add_project_arguments('-mbmi', language: ['c', 'cpp'])
            add_project_arguments('-mlzcnt', language: ['c', 'cpp'])
        endif

        # bit manipulation instruction set 2
        if enable_bmi2
            message('BMI2: enabled (2013)')
            add_project_arguments('-mbmi2', language: ['c', 'cpp'])
        endif

        # fused multiply-add
        if enable_fma
            message('FMA: enabled (2013)')
            add_project_arguments('-mfma', language: ['c', 'cpp'])
        endif

        # enable only one (the most recent) SIMD extension
        if enable_avx512
            message('SIMD: AVX-512 (2015)')
            add_project_arguments('-mavx512dq', language: ['c', 'cpp'])
            add_project_arguments('-mavx512vl', language: ['c', 'cpp'])
            add_project_arguments('-mavx512bw', language: ['c', 'cpp'])
        elif enable_avx2
            message('SIMD: AVX2 (2013)')
            add_project_arguments('-mavx2', language: ['c', 'cpp'])
        elif enable_avx
            message('SIMD: AVX (2008)')
            add_project_arguments('-mavx', language: ['c', 'cpp'])
        elif enable_sse4
            message('SIMD: SSE4.2 (2006)')
            add_project_arguments('-msse4', language: ['c', 'cpp'])
        elif enable_sse2
            message('SIMD: SSE2 (2001)')
            add_project_arguments('-msse2', language: ['c', 'cpp'])
        endif
    endif
endif

libtype = get_option('default_library')

threads_dep = dependency('threads')

mango_deps             = [ threads_dep ]
mango_public_deps      = [ ]
mango_opengl_deps      = [ ]

opengl_dep = dependency('gl')
mango_public_deps += opengl_dep

if host_machine.system() != 'windows'
    dl_dep = cpp.find_library('dl', required : false)
    mango_deps += [ dl_dep ]
endif

if host_machine.system() == 'linux'
    m_dep = cpp.find_library('m', required : true)
    x11_dep = cpp.find_library('X11', required : true)
    mango_deps += [ m_dep ]
    mango_public_deps += [ m_dep ]
    mango_opengl_deps += [ x11_dep ]
endif

mango_inc = include_directories('../include')

mango = static_library('mango',
    extra_files : [
        mango_headers,
        external_headers
    ],

    sources : [
        core_sources,
        filesystem_sources,
        image_sources,
        jpeg_sources,
        math_sources,
        external_lzma_sources,
        external_aes_sources,
        external_bc_sources,
        external_bzip2_sources,
        external_google_sources,
        external_lz4_sources,
        external_lzfse_sources,
        external_lzo_sources,
        external_deflate_sources,
        external_unrar_sources,
        external_zstd_sources,
        external_webp_sources,
        external_zpng_sources
    ],
    include_directories : [ mango_inc, '../source/external/libwebp' ],
    dependencies        : mango_deps,
    cpp_args            : mango_public_cpp_args,
    install             : true
)

mango_dep = declare_dependency(
    link_with           : mango,
    include_directories : mango_inc,
    dependencies        : mango_public_deps,
    compile_args        : mango_public_cpp_args + mango_interface_cpp_args
)

mango_opengl = static_library('mango-opengl',
    sources             : window_sources + opengl_sources,
    link_with           : mango,
    dependencies        : mango_opengl_deps,
    include_directories : mango_inc,
    cpp_args            : mango_public_cpp_args,
    install             : true
)

mango_opengl_dep = declare_dependency(
    link_with           : mango_opengl,
    include_directories : mango_inc,
    compile_args        : mango_public_cpp_args + mango_interface_cpp_args
)

install_subdir('../include/mango', install_dir : 'include')

pkg = import('pkgconfig')

pkg.generate(mango,
    libraries_private : [ mango_deps ],
    description : 'A multi-platform low-level development framework for graphics programmers - Core'
)

pkg.generate(mango_opengl,
    name: 'mango-opengl',
    libraries_private : [ mango_opengl_deps ],
    description : 'A multi-platform low-level development framework for graphics programmers - OpenGL'
)

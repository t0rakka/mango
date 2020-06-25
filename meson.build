# ---------------------------------------------------------------------------
# mango meson.build
#
# Copyright (C) 2012-2020 Twilight 3D Finland Oy Ltd.
# ---------------------------------------------------------------------------

project(
    'mango', 'c', 'cpp',

    default_options : [
        'c_std=c11',
        'cpp_std=c++14'
    ]
)

mango_extra_cpp_options = []

cpp = meson.get_compiler('cpp')

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

mango_all_archive_formats = [ 'ZIP', 'RAR', 'MGX' ]
foreach archive_format : mango_all_archive_formats
    key = 'mango_disable_archive_@0@'.format(archive_format)
    if get_option(key)
        mango_extra_cpp_options += [ '-D@0@'.format(key) ]
    endif
endforeach

mango_all_image_formats = [ 'ASTC', 'ATARI', 'BMP', 'C64', 'DDS', 'GIF', 'HDR', 'IFF', 'JPG', 'KTX',
                            'PCX', 'PKM', 'PNG', 'PNM', 'PVR', 'SGI', 'TGA', 'WEBP', 'ZPNG' ]
foreach image_format : mango_all_image_formats
    key = 'mango_disable_image_@0@'.format(image_format)
    if get_option(key)
        mango_extra_cpp_options += [ '-D@0@'.format(key) ]
    endif
endforeach

if get_option('mango_disable_license_gpl')
    mango_extra_cpp_options += [ '-DMANGO_DISABLE_LICENSE_GPL' ]
endif

# ------------------------------------------------------------------------------
# source directories
# ------------------------------------------------------------------------------

core_sources = files(
    'source/mango/core/aes.cpp',
    'source/mango/core/buffer.cpp',
    'source/mango/core/compress.cpp',
    'source/mango/core/cpuinfo.cpp',
    'source/mango/core/crc32.cpp',
    'source/mango/core/hash.cpp',
    'source/mango/core/md5.cpp',
    'source/mango/core/memory.cpp',
    'source/mango/core/sha1.cpp',
    'source/mango/core/sha2.cpp',
    'source/mango/core/string.cpp',
    'source/mango/core/system.cpp',
    'source/mango/core/thread.cpp',
    'source/mango/core/timer.cpp'
)
filesystem_sources = files(
    'source/mango/filesystem/file.cpp',
    'source/mango/filesystem/mapper.cpp',
    'source/mango/filesystem/mapper_mgx.cpp',
    'source/mango/filesystem/mapper_rar.cpp',
    'source/mango/filesystem/mapper_zip.cpp',
    'source/mango/filesystem/path.cpp'
)
opengl_sources = files(
    'source/mango/opengl/opengl.cpp'
)
if host_machine.system() == 'windows'
    core_sources += files(
        'source/mango/core/win32/dynamic_library.cpp'
    )
    filesystem_sources += files(
        'source/mango/filesystem/win32/file_observer.cpp',
        'source/mango/filesystem/win32/file_stream.cpp',
        'source/mango/filesystem/win32/mapper_file.cpp'
    )
    window_sources = files(
        'source/mango/window/win32/win32_window.cpp'
    )
    opengl_sources += files(
        'source/mango/opengl/wgl/wgl_context.cpp'
    )
    framebuffer_sources = files(
        'source/mango/framebuffer/win32/d3d9_framebuffer.cpp'
    )
elif host_machine.system() == 'darwin'
    core_sources += files(
        'source/mango/core/unix/dynamic_library.cpp'
    )
    filesystem_sources += files(
        'source/mango/filesystem/unix/file_observer.cpp',
        'source/mango/filesystem/unix/file_stream.cpp',
        'source/mango/filesystem/unix/mapper_file.cpp'
    )
    window_sources = files(
        'source/mango/window/cocoa/cocoa_window.mm'
    )
    opengl_sources += files(
        'source/mango/opengl/cocoa/cocoa_context.mm',
        'source/mango/opengl/cocoa/CustomOpenGLView.mm'
    )
    framebuffer_sources = files(
        'source/mango/framebuffer/cocoa/cocoa_framebuffer.mm'
    )
else
    core_sources += files(
        'source/mango/core/unix/dynamic_library.cpp'
    )
    filesystem_sources += files(
        'source/mango/filesystem/unix/file_observer.cpp',
        'source/mango/filesystem/unix/file_stream.cpp',
        'source/mango/filesystem/unix/mapper_file.cpp'
    )
    window_sources = files(
        'source/mango/window/xlib/xlib_window.cpp'
    )
    opengl_sources += files(
        'source/mango/opengl/glx/glx_context.cpp'
    )
    framebuffer_sources = files(
        'source/mango/framebuffer/xlib/xlib_framebuffer.cpp'
    )
endif

image_sources = files(
    'source/mango/image/blitter.cpp',
    'source/mango/image/block.cpp',
    'source/mango/image/block_dxt.cpp',
    'source/mango/image/block_pvrtc.cpp',
    'source/mango/image/block_yuv.cpp',
    'source/mango/image/exif.cpp',
    'source/mango/image/format.cpp',
    'source/mango/image/image_astc.cpp',
    'source/mango/image/image_atari.cpp',
    'source/mango/image/image_bmp.cpp',
    'source/mango/image/image_c64.cpp',
    'source/mango/image/image.cpp',
    'source/mango/image/image_dds.cpp',
    'source/mango/image/image_gif.cpp',
    'source/mango/image/image_hdr.cpp',
    'source/mango/image/image_iff.cpp',
    'source/mango/image/image_jpg.cpp',
    'source/mango/image/image_ktx.cpp',
    'source/mango/image/image_pcx.cpp',
    'source/mango/image/image_pkm.cpp',
    'source/mango/image/image_png.cpp',
    'source/mango/image/image_pnm.cpp',
    'source/mango/image/image_pvr.cpp',
    'source/mango/image/image_sgi.cpp',
    'source/mango/image/image_tga.cpp',
    'source/mango/image/image_webp.cpp',
    'source/mango/image/image_zpng.cpp',
    'source/mango/image/quantize.cpp',
    'source/mango/image/surface.cpp'
)
jpeg_sources = files(
    'source/mango/jpeg/jpeg_arithmetic.cpp',
    'source/mango/jpeg/jpeg_decode.cpp',
    'source/mango/jpeg/jpeg_encode.cpp',
    'source/mango/jpeg/jpeg_huffman.cpp',
    'source/mango/jpeg/jpeg_idct.cpp',
    'source/mango/jpeg/jpeg_process.cpp'
)
math_sources = files(
    'source/mango/math/geometry.cpp',
    'source/mango/math/math.cpp',
    'source/mango/math/simd.cpp'
)
vulkan_sources = files(
    'source/mango/vulkan/vulkan.cpp'
)
external_lzma_sources = files(
    'source/external/lzma/7zAes.c',
    'source/external/lzma/7zAlloc.c',
    'source/external/lzma/7zArcIn.c',
    'source/external/lzma/7zBuf2.c',
    'source/external/lzma/7zBuf.c',
    'source/external/lzma/7zCrc.c',
    'source/external/lzma/7zCrcOpt.c',
    'source/external/lzma/7zDec.c',
    'source/external/lzma/7zFile.c',
    'source/external/lzma/7zSha1.c',
    'source/external/lzma/7zSha256.c',
    'source/external/lzma/7zStream.c',
    'source/external/lzma/AesOpt.c',
    'source/external/lzma/Alloc.c',
    'source/external/lzma/Bcj2.c',
    'source/external/lzma/Bcj2Enc.c',
    'source/external/lzma/Blake2s.c',
    'source/external/lzma/Bra86.c',
    'source/external/lzma/Bra.c',
    'source/external/lzma/BraIA64.c',
    'source/external/lzma/BwtSort.c',
    'source/external/lzma/CpuArch.c',
    'source/external/lzma/Delta.c',
    'source/external/lzma/DllSecur.c',
    'source/external/lzma/HuffEnc.c',
    'source/external/lzma/LzFind.c',
    'source/external/lzma/Lzma2Dec.c',
    'source/external/lzma/Lzma2Enc.c',
    'source/external/lzma/Lzma86Dec.c',
    'source/external/lzma/Lzma86Enc.c',
    'source/external/lzma/LzmaDec.c',
    'source/external/lzma/LzmaEnc.c',
    'source/external/lzma/LzmaLib.c',
    'source/external/lzma/Ppmd7.c',
    'source/external/lzma/Ppmd7Dec.c',
    'source/external/lzma/Ppmd7Enc.c',
    'source/external/lzma/Ppmd8.c',
    'source/external/lzma/Ppmd8Dec.c',
    'source/external/lzma/Ppmd8Enc.c',
    'source/external/lzma/Sort.c',
    'source/external/lzma/Xz.c',
    'source/external/lzma/XzCrc64.c',
    'source/external/lzma/XzCrc64Opt.c',
    'source/external/lzma/XzDec.c',
    'source/external/lzma/XzEnc.c',
    'source/external/lzma/XzIn.c'
)
external_aes_sources = files(
    'source/external/aes/bc_aes.cpp'
)
external_bc_sources = files(
    'source/external/bc/BC4BC5.cpp',
    'source/external/bc/BC6HBC7.cpp',
    'source/external/bc/BC.cpp'
)
external_bzip2_sources = files(
    'source/external/bzip2/bz_blocksort.c',
    'source/external/bzip2/bz_compress.c',
    'source/external/bzip2/bz_crctable.c',
    'source/external/bzip2/bz_decompress.c',
    'source/external/bzip2/bz_huffman.c',
    'source/external/bzip2/bzlib.c',
    'source/external/bzip2/bz_randtable.c'
)
external_google_sources = files(
    'source/external/google/astc.cpp',
    'source/external/google/etc1.cpp',
    'source/external/google/etc.cpp'
)

external_lz4_sources = files(
    'source/external/lz4/lz4.c',
    'source/external/lz4/lz4hc.c'
)
external_lzfse_sources = files(
    'source/external/lzfse/lzfse_decode_base.c',
    'source/external/lzfse/lzfse_decode.c',
    'source/external/lzfse/lzfse_encode_base.c',
    'source/external/lzfse/lzfse_encode.c',
    'source/external/lzfse/lzfse_fse.c',
    'source/external/lzfse/lzvn_decode_base.c',
    'source/external/lzfse/lzvn_encode_base.c',
)
external_lzo_sources = files(
    'source/external/lzo/minilzo.c'
)
external_deflate_sources = files(
    'source/external/libdeflate/lib/deflate_compress.c',
    'source/external/libdeflate/lib/deflate_decompress.c',
    'source/external/libdeflate/lib/gzip_compress.c',
    'source/external/libdeflate/lib/gzip_decompress.c',
    'source/external/libdeflate/lib/libd_adler32.c',
    'source/external/libdeflate/lib/libd_crc32.c',
    'source/external/libdeflate/lib/libd_utils.c',
    'source/external/libdeflate/lib/zlib_compress.c',
    'source/external/libdeflate/lib/zlib_decompress.c',
    'source/external/libdeflate/lib/x86/cpu_x86_features.c',
    'source/external/libdeflate/lib/arm/cpu_arm_features.c'
)
external_unrar_sources = files(
    'source/external/unrar/crc.cpp',
    'source/external/unrar/crypt.cpp',
    'source/external/unrar/getbits.cpp',
    'source/external/unrar/rarvm.cpp',
    'source/external/unrar/rdwrfn.cpp',
    'source/external/unrar/rijndael.cpp',
    'source/external/unrar/unpack.cpp'
)
external_zstd_sources = files(
    'source/external/zstd/common/zstd_common.c',
    'source/external/zstd/common/fse_decompress.c',
    'source/external/zstd/common/pool.c',
    'source/external/zstd/common/error_private.c',
    'source/external/zstd/common/threading.c',
    'source/external/zstd/common/entropy_common.c',
    'source/external/zstd/common/debug.c',
    'source/external/zstd/common/xxhash.c',
    'source/external/zstd/compress/zstd_fast.c',
    'source/external/zstd/compress/zstd_compress_literals.c',
    'source/external/zstd/compress/huf_compress.c',
    'source/external/zstd/compress/zstd_compress_superblock.c',
    'source/external/zstd/compress/zstd_opt.c',
    'source/external/zstd/compress/hist.c',
    'source/external/zstd/compress/zstd_ldm.c',
    'source/external/zstd/compress/fse_compress.c',
    'source/external/zstd/compress/zstd_compress.c',
    'source/external/zstd/compress/zstd_compress_sequences.c',
    'source/external/zstd/compress/zstd_double_fast.c',
    'source/external/zstd/compress/zstdmt_compress.c',
    'source/external/zstd/compress/zstd_lazy.c',
    'source/external/zstd/decompress/huf_decompress.c',
    'source/external/zstd/decompress/zstd_decompress.c',
    'source/external/zstd/decompress/zstd_decompress_block.c',
    'source/external/zstd/decompress/zstd_ddict.c'
)
external_webp_sources = files(
    'source/external/libwebp/src/demux/demux.c',
    'source/external/libwebp/src/demux/anim_decode.c',
    'source/external/libwebp/src/enc/tree_enc.c',
    'source/external/libwebp/src/enc/webp_enc.c',
    'source/external/libwebp/src/enc/quant_enc.c',
    'source/external/libwebp/src/enc/predictor_enc.c',
    'source/external/libwebp/src/enc/picture_psnr_enc.c',
    'source/external/libwebp/src/enc/picture_csp_enc.c',
    'source/external/libwebp/src/enc/frame_enc.c',
    'source/external/libwebp/src/enc/picture_tools_enc.c',
    'source/external/libwebp/src/enc/config_enc.c',
    'source/external/libwebp/src/enc/cost_enc.c',
    'source/external/libwebp/src/enc/token_enc.c',
    'source/external/libwebp/src/enc/vp8l_enc.c',
    'source/external/libwebp/src/enc/near_lossless_enc.c',
    'source/external/libwebp/src/enc/picture_enc.c',
    'source/external/libwebp/src/enc/histogram_enc.c',
    'source/external/libwebp/src/enc/filter_enc.c',
    'source/external/libwebp/src/enc/analysis_enc.c',
    'source/external/libwebp/src/enc/backward_references_cost_enc.c',
    'source/external/libwebp/src/enc/alpha_enc.c',
    'source/external/libwebp/src/enc/syntax_enc.c',
    'source/external/libwebp/src/enc/iterator_enc.c',
    'source/external/libwebp/src/enc/picture_rescale_enc.c',
    'source/external/libwebp/src/enc/backward_references_enc.c',
    'source/external/libwebp/src/mux/muxinternal.c',
    'source/external/libwebp/src/mux/anim_encode.c',
    'source/external/libwebp/src/mux/muxedit.c',
    'source/external/libwebp/src/mux/muxread.c',
    'source/external/libwebp/src/dsp/upsampling_neon.c',
    'source/external/libwebp/src/dsp/upsampling.c',
    'source/external/libwebp/src/dsp/lossless_enc_sse2.c',
    'source/external/libwebp/src/dsp/upsampling_msa.c',
    'source/external/libwebp/src/dsp/cost_mips32.c',
    'source/external/libwebp/src/dsp/enc_sse2.c',
    'source/external/libwebp/src/dsp/lossless_enc_mips_dsp_r2.c',
    'source/external/libwebp/src/dsp/lossless_enc_neon.c',
    'source/external/libwebp/src/dsp/cost_mips_dsp_r2.c',
    'source/external/libwebp/src/dsp/lossless_mips_dsp_r2.c',
    'source/external/libwebp/src/dsp/rescaler.c',
    'source/external/libwebp/src/dsp/cost.c',
    'source/external/libwebp/src/dsp/enc.c',
    'source/external/libwebp/src/dsp/rescaler_sse2.c',
    'source/external/libwebp/src/dsp/dec_mips_dsp_r2.c',
    'source/external/libwebp/src/dsp/enc_sse41.c',
    'source/external/libwebp/src/dsp/filters_mips_dsp_r2.c',
    'source/external/libwebp/src/dsp/enc_mips_dsp_r2.c',
    'source/external/libwebp/src/dsp/dec_neon.c',
    'source/external/libwebp/src/dsp/yuv_mips32.c',
    'source/external/libwebp/src/dsp/lossless_enc_sse41.c',
    'source/external/libwebp/src/dsp/lossless_enc_msa.c',
    'source/external/libwebp/src/dsp/rescaler_neon.c',
    'source/external/libwebp/src/dsp/alpha_processing.c',
    'source/external/libwebp/src/dsp/dec.c',
    'source/external/libwebp/src/dsp/lossless_neon.c',
    'source/external/libwebp/src/dsp/yuv.c',
    'source/external/libwebp/src/dsp/enc_neon.c',
    'source/external/libwebp/src/dsp/rescaler_mips_dsp_r2.c',
    'source/external/libwebp/src/dsp/lossless_msa.c',
    'source/external/libwebp/src/dsp/enc_mips32.c',
    'source/external/libwebp/src/dsp/dec_clip_tables.c',
    'source/external/libwebp/src/dsp/dec_sse41.c',
    'source/external/libwebp/src/dsp/yuv_sse41.c',
    'source/external/libwebp/src/dsp/alpha_processing_sse2.c',
    'source/external/libwebp/src/dsp/cpu.c',
    'source/external/libwebp/src/dsp/filters_neon.c',
    'source/external/libwebp/src/dsp/dec_mips32.c',
    'source/external/libwebp/src/dsp/lossless.c',
    'source/external/libwebp/src/dsp/yuv_sse2.c',
    'source/external/libwebp/src/dsp/rescaler_msa.c',
    'source/external/libwebp/src/dsp/ssim.c',
    'source/external/libwebp/src/dsp/alpha_processing_neon.c',
    'source/external/libwebp/src/dsp/cost_sse2.c',
    'source/external/libwebp/src/dsp/upsampling_sse41.c',
    'source/external/libwebp/src/dsp/ssim_sse2.c',
    'source/external/libwebp/src/dsp/filters.c',
    'source/external/libwebp/src/dsp/enc_msa.c',
    'source/external/libwebp/src/dsp/dec_sse2.c',
    'source/external/libwebp/src/dsp/lossless_sse2.c',
    'source/external/libwebp/src/dsp/lossless_enc.c',
    'source/external/libwebp/src/dsp/yuv_mips_dsp_r2.c',
    'source/external/libwebp/src/dsp/lossless_enc_mips32.c',
    'source/external/libwebp/src/dsp/upsampling_sse2.c',
    'source/external/libwebp/src/dsp/cost_neon.c',
    'source/external/libwebp/src/dsp/alpha_processing_mips_dsp_r2.c',
    'source/external/libwebp/src/dsp/alpha_processing_sse41.c',
    'source/external/libwebp/src/dsp/upsampling_mips_dsp_r2.c',
    'source/external/libwebp/src/dsp/rescaler_mips32.c',
    'source/external/libwebp/src/dsp/dec_msa.c',
    'source/external/libwebp/src/dsp/filters_sse2.c',
    'source/external/libwebp/src/dsp/yuv_neon.c',
    'source/external/libwebp/src/dsp/filters_msa.c',
    'source/external/libwebp/src/dec/idec_dec.c',
    'source/external/libwebp/src/dec/tree_dec.c',
    'source/external/libwebp/src/dec/io_dec.c',
    'source/external/libwebp/src/dec/webp_dec.c',
    'source/external/libwebp/src/dec/buffer_dec.c',
    'source/external/libwebp/src/dec/frame_dec.c',
    'source/external/libwebp/src/dec/vp8l_dec.c',
    'source/external/libwebp/src/dec/alpha_dec.c',
    'source/external/libwebp/src/dec/vp8_dec.c',
    'source/external/libwebp/src/dec/quant_dec.c',
    'source/external/libwebp/src/utils/quant_levels_dec_utils.c',
    'source/external/libwebp/src/utils/bit_writer_utils.c',
    'source/external/libwebp/src/utils/huffman_utils.c',
    'source/external/libwebp/src/utils/color_cache_utils.c',
    'source/external/libwebp/src/utils/rescaler_utils.c',
    'source/external/libwebp/src/utils/filters_utils.c',
    'source/external/libwebp/src/utils/quant_levels_utils.c',
    'source/external/libwebp/src/utils/huffman_encode_utils.c',
    'source/external/libwebp/src/utils/utils.c',
    'source/external/libwebp/src/utils/thread_utils.c',
    'source/external/libwebp/src/utils/bit_reader_utils.c',
    'source/external/libwebp/src/utils/random_utils.c'
)
external_zpng_sources = files(
    'source/external/zpng/zpng.cpp'
)

#SOURCE_GROUP("external" FILES
#    ${EXTERNAL_LZMA}
#    ${EXTERNAL_AES}
#    ${EXTERNAL_BC}
#    ${EXTERNAL_BZIP2}
#    ${EXTERNAL_CONCURRENT_QUEUE}
#    ${EXTERNAL_GOOGLE}
#    ${EXTERNAL_LZ4}
#    ${EXTERNAL_LZFSE}
#    ${EXTERNAL_LZO}
#    ${EXTERNAL_DEFLATE}
#    ${EXTERNAL_UNRAR}
#    ${EXTERNAL_ZSTD}
#    ${EXTERNAL_WEBP}
#    ${EXTERNAL_ZPNG}
#)


# ------------------------------------------------------------------------------
# configuration
# ------------------------------------------------------------------------------

if cpp.get_id() == 'msvc'
    # target_compile_options(mango PUBLIC "/Gm") deprecated
    mango_extra_cpp_options += '/DUNICODE'

    if enable_fast_math
        mango_extra_cpp_options += '/fp:fast'
    endif

    if enable_avx512
        message('SIMD: AVX-512 (2015)')
        mango_extra_cpp_options += [ '-D__AVX512F__', '-D__AVX512DQ__' ]
    elif enable_avx2
        message('SIMD: AVX2 (2013)')
        mango_extra_cpp_options += [ '/arch:AVX2' ]
    elif enable_avx
        message('SIMD: AVX (2008)')
        mango_extra_cpp_options += [ '/arch:AVX' ]
    endif
else
    mango_extra_cpp_options += [ '-Wall' ]
    # set(CMAKE_CXX_FLAGS_DEBUG "-g")
    # set(CMAKE_CXX_FLAGS_RELEASE "-O3")

    if host_machine.system() == 'windows'
        mango_extra_cpp_options += [ '-DUNICODE' ]
    endif

    if cpp.get_id() == 'gcc'
        mango_extra_cpp_options += [ '-ftree-vectorize' ]
    endif

    if enable_fast_math
        mango_extra_cpp_options += [ '-ffast-math' ]
    endif

    if (host_machine.cpu_family() == 'arm' and enable_neon) or host_machine.cpu_family() == 'aarch64'
        message('SIMD: NEON')
        mango_extra_cpp_options += [ '-mfpu=neon', '-mfloat-abi=hard' ] # add_definitions
        mango_extra_cpp_options += [ '-fpermissive', '-Wno-psabi' ] # set(CMAKE_CXX_FLAGS
    endif

    # 'x86_64' 'aarch64' 'arm' 'ppc' 'ppc64' 'x86'
    if host_machine.cpu_family() == 'x86'  or host_machine.cpu_family() == 'x86'
        # enable AES (2008) by default
        mango_extra_cpp_options += [ '-maes' ]

        # enable CLMUL (2008) by default
        mango_extra_cpp_options += [ '-mpclmul' ]

        # half conversion instructions
        if enable_f16c
            message('F16C: half conversion enabled (2012)')
            mango_extra_cpp_options += [ '-mf16c' ]
        endif

        # bit manipulation instruction set 1
        if enable_bmi
            message('BMI+LZCNT: enabled (2013)')
            mango_extra_cpp_options += [ '-mbmi' ]
            mango_extra_cpp_options += [ '-mlzcnt' ]
        endif

        # bit manipulation instruction set 2
        if enable_bmi2
            message('BMI2: enabled (2013)')
            mango_extra_cpp_options += [ '-mbmi2' ]
        endif

        # fused multiply-add
        if enable_fma
            message('FMA: enabled (2013)')
            mango_extra_cpp_options += [ '-mfma' ]
        endif

        # enable only one (the most recent) SIMD extension
        if enable_avx512
            message('SIMD: AVX-512 (2015)')
            mango_extra_cpp_options += [ '-mavx512dq' ]
            mango_extra_cpp_options += [ '-mavx512vl' ]
            mango_extra_cpp_options += [ '-mavx512bw' ]
        elif enable_avx2
            message('SIMD: AVX2 (2013)')
            mango_extra_cpp_options += [ '-mavx2' ]
        elif enable_avx
            message('SIMD: AVX (2008)')
            mango_extra_cpp_options += [ '-mavx' ]
        elif enable_sse4
            message('SIMD: SSE4.2 (2006)')
            mango_extra_cpp_options += [ '-msse4' ]
        elif enable_sse2
            message('SIMD: SSE2 (2001)')
            mango_extra_cpp_options += [ '-msse2' ]
        endif
    endif
endif

libtype = get_option('default_library')

threads_dep = dependency('threads')
dl_dep = cpp.find_library('dl',  required : false)

mango_deps             = [ threads_dep ]
mango_framebuffer_deps = [ ]
mango_opengl_deps      = [ ]
mango_vulkan_deps      = [ ]

if host_machine.system() == 'windows'
    mango_deps += cpp.find_library('opengl32', required : true)
    mango_deps += cpp.find_library('gdi32',    required : true)
elif host_machine.system() == 'darwin'
    opengl_dep = dependency('appleframeworks', modules: ['OpenGL'], required : true)
    cocoa_dep  = dependency('appleframeworks', modules: ['Cocoa'],  required : true)
    if libtype == 'shared'
        mango_framebuffer_deps += [ cocoa_dep ]
        mango_opengl_deps      += [ cocoa_dep, opengl_dep ]
        mango_vulkan_deps      += [ cocoa_dep ]
    endif
    mango_deps += [ dl_dep ]
    mango_extra_cpp_options += '-mmacosx-version-min=10.13'
elif host_machine.system() == 'linux'
    gl_dep  = cpp.find_library('GL',  required : true)
    x11_dep = cpp.find_library('X11', required : true)
    mango_deps        += [ dl_dep, gl_dep, x11_dep ]
    mango_opengl_deps += [ dl_dep, gl_dep, x11_dep ]
endif

mango = library('mango',
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
    include_directories: [
        'include',
        'source/external/libwebp'
    ],
    dependencies : mango_deps,
    cpp_args     : mango_extra_cpp_options
)

mango_dep = declare_dependency(
    link_with           : mango,
    include_directories : include_directories('include'),
    dependencies        : mango_deps
)

mango_framebuffer_deps += [ mango_dep ]
mango_opengl_deps      += [ mango_dep ]
mango_vulkan_deps      += [ mango_dep ]

mango_framebuffer = library('mango-framebuffer',
    sources      : window_sources + framebuffer_sources,
    dependencies : mango_framebuffer_deps
)

mango_opengl = library('mango-opengl',
    sources      : window_sources + opengl_sources,
    dependencies : mango_opengl_deps
)

mango_vulkan = library('mango-vulkan',
    sources      : window_sources + vulkan_sources,
    dependencies : mango_vulkan_deps
)


# archivers

# ------------------------------------------------------------------------------
# install
# ------------------------------------------------------------------------------

# TARGET_INCLUDE_DIRECTORIES(mango PUBLIC
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
#     $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mango>
# )
# 
# INSTALL(TARGETS mango
#     LIBRARY DESTINATION "lib"
#     ARCHIVE DESTINATION "lib"
#     RUNTIME DESTINATION "bin"
# )
# 
# INSTALL(TARGETS mango-framebuffer
#     LIBRARY DESTINATION "lib"
#     ARCHIVE DESTINATION "lib"
#     RUNTIME DESTINATION "bin"
# )
# INSTALL(TARGETS mango-opengl
#     LIBRARY DESTINATION "lib"
#     ARCHIVE DESTINATION "lib"
#     RUNTIME DESTINATION "bin"
# )
# INSTALL(TARGETS mango-vulkan
#     LIBRARY DESTINATION "lib"
#     ARCHIVE DESTINATION "lib"
#     RUNTIME DESTINATION "bin"
# )
# 
# INSTALL(
#     DIRECTORY ${CMAKE_SOURCE_DIR}/../include/
#     DESTINATION include
#     FILES_MATCHING PATTERN "*.h*"
# )

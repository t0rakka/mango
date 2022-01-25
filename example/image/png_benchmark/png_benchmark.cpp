/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;
using namespace mango::image;

#define ENABLE_LIBPNG
#define ENABLE_LODEPNG
#define ENABLE_STB
#define ENABLE_SPNG
#define ENABLE_WUFFS
#define ENABLE_MANGO

#include "fpnge/fpnge.h"
#ifdef CAN_COMPILE_FPNGE
#define ENABLE_FPNGE
#endif

#ifdef MANGO_CPU_INTEL
#define ENABLE_FPNG
#endif

// ----------------------------------------------------------------------
// options
// ----------------------------------------------------------------------

bool g_option_multithread = true;
int g_option_compression = 4;
int g_option_count = 1;

// ----------------------------------------------------------------------
// utils
// ----------------------------------------------------------------------

size_t get_file_size(const char* filename)
{
    File file(filename);
    return file.size();
}

void load_none(Memory memory)
{
    MANGO_UNREFERENCED(memory);
}

size_t save_none(const Bitmap& bitmap)
{
    MANGO_UNREFERENCED(bitmap);
    return 0;
}

template <typename Load, typename Save>
void test(const char* name, Load load, Save save, Memory memory, const Bitmap& bitmap)
{
    printf("%s", name);
    u64 time0 = Time::us();

    for (int i = 0; i < g_option_count; ++i)
    {
        load(memory);
    }

    u64 time1 = Time::us();

    size_t size = 0;

    for (int i = 0; i < g_option_count; ++i)
    {
        size = save(bitmap);
    }

    u64 time2 = Time::us();

    if (load != load_none)
        printf("  %7d.%d ", int((time1 - time0) / 1000), int((time1 - time0) % 10));
    else
        printf("        N/A ");

    if (save != save_none)
        printf("  %7d.%d ", int((time2 - time1) / 1000), int((time2 - time1) % 10));
    else
        printf("        N/A ");

    printf("  %8d", int(size / 1024));
    printf("\n");
}

// ----------------------------------------------------------------------
// libpng
// ----------------------------------------------------------------------

#if defined ENABLE_LIBPNG

#include <png.h>

struct png_source
{
    u8* data;
    int size;
    int offset;
};

static void png_read_callback(png_structp png_ptr, png_bytep data, png_size_t length)
{
    png_source* source = (png_source*) png_get_io_ptr(png_ptr);
    std::memcpy(data, source->data + source->offset, length);
    source->offset += length;
}

void load_libpng(Memory memory)
{
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);

    png_source source;
    source.data = memory.address + 8;
    source.size = memory.size;
    source.offset = 0;
    png_set_read_fn(png_ptr, &source, png_read_callback);

    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    //int width = png_get_image_width(png_ptr, info_ptr);
    int height = png_get_image_height(png_ptr, info_ptr);
    //png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    //png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    int number_of_passes = png_set_interlace_handling(png_ptr);
    (void) number_of_passes;
    png_read_update_info(png_ptr, info_ptr);

    int stride = png_get_rowbytes(png_ptr, info_ptr);
    u8* image = (u8*)malloc(stride * height);
    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);


    for (int y = 0; y < height; y++)
    {
        row_pointers[y] = image + stride * y;
    }

    png_read_image(png_ptr, row_pointers);

    free(row_pointers);
    free(image);
}

size_t save_libpng(const Bitmap& bitmap)
{
    const char* filename = "output-libpng.png";

    int width = bitmap.width;
    int height = bitmap.height;

    std::vector<u8*> row_pointer_array(height);
    for (int y = 0; y < height; ++y)
    {
        row_pointer_array[y] = bitmap.address(0, y);
    }

    u8** row_pointers = row_pointer_array.data();

    FILE *fp = fopen(filename, "wb");
    if(!fp)
        abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
        abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();

    if (setjmp(png_jmpbuf(png)))
        abort();

    png_init_io(png, fp);

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
        png,
        info,
        width, height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    //png_set_filler(png, 0, PNG_FILLER_AFTER);

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    fclose(fp);

    png_destroy_write_struct(&png, &info);

    return get_file_size(filename);
}

#endif

// ----------------------------------------------------------------------
// lodepng
// ----------------------------------------------------------------------

#if defined ENABLE_LODEPNG

#include "lodepng/lodepng.h"

void load_lodepng(Memory memory)
{
    u32 width, height;
    u8* image;
    int error = lodepng_decode32(&image, &width, &height, memory.address, memory.size);
    MANGO_UNREFERENCED(error); // NOTE: ignored in benchmarking situation
    free(image);
}

size_t save_lodepng(const Bitmap& bitmap)
{
    const char* filename = "output-lodepng.png";

    LodePNGEncoderSettings settings;
    settings.filter_strategy = LFS_ZERO;
    lodepng_encoder_settings_init(&settings);

    lodepng_encode32_file(filename, bitmap.image, bitmap.width, bitmap.height);
    return get_file_size(filename);
}

#endif

// ----------------------------------------------------------------------
// stb
// ----------------------------------------------------------------------

#if defined(ENABLE_STB)

#define STB_IMAGE_IMPLEMENTATION
#include "../jpeg_benchmark/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../jpeg_benchmark/stb_image_write.h"

void load_stb(Memory memory)
{
    int width, height, bpp;
    u8* image = stbi_load_from_memory(memory.address, memory.size, &width, &height, &bpp, 4);
    free(image);
}

size_t save_stb(const Bitmap& bitmap)
{
    const char* filename = "output-stb.png";
    stbi_write_png(filename, bitmap.width, bitmap.height, 4, bitmap.image, bitmap.width * 4);
    return get_file_size(filename);
}

#endif

// ----------------------------------------------------------------------
// spng
// ----------------------------------------------------------------------

// https://libspng.org/

#if defined(ENABLE_SPNG)

#include "spng/spng.h"

struct read_fn_state
{
    unsigned char *data;
    size_t bytes_left;
};

int read_fn(struct spng_ctx *ctx, void *user, void *data, size_t n)
{
    struct read_fn_state *state = (struct read_fn_state *) user;
    if(n > state->bytes_left) return SPNG_IO_EOF;

    unsigned char *dst = (u8*)data;
    unsigned char *src = state->data;

    memcpy(dst, src, n);

    state->bytes_left -= n;
    state->data += n;

    return 0;
}

unsigned char *getimage_libspng(unsigned char *buf, size_t size, size_t *out_size, int fmt, int flags, struct spng_ihdr *info)
{
    int r;
    size_t siz;
    unsigned char *out = NULL;
    struct spng_ihdr ihdr;

    spng_ctx *ctx = spng_ctx_new(0);

    if(ctx==NULL)
    {
        printf("spng_ctx_new() failed\n");
        return NULL;
    }

    spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);

    r = spng_set_png_buffer(ctx, buf, size);

    if(r)
    {
        printf("spng_set_png_buffer() error: %s\n", spng_strerror(r));
        goto err;
    }

    r = spng_get_ihdr(ctx, &ihdr);

    if(r)
    {
        printf("spng_get_ihdr() error: %s\n", spng_strerror(r));
        goto err;
    }

    memcpy(info, &ihdr, sizeof(struct spng_ihdr));

    r = spng_decoded_image_size(ctx, fmt, &siz);
    if(r) goto err;

    *out_size = siz;

    out = (unsigned char*)malloc(siz);
    if(out==NULL) goto err;

    r = spng_decode_image(ctx, out, siz,  fmt, flags);

    if(r)
    {
        printf("spng_decode_image() error: %s\n", spng_strerror(r));
        goto err;
    }

    spng_ctx_free(ctx);

goto skip_err;

err:
    spng_ctx_free(ctx);
    if(out !=NULL) free(out);
    return NULL;

skip_err:

    return out;
}

void load_spng(Memory memory)
{
    struct spng_ihdr ihdr;
    size_t img_spng_size;

    u8* image = getimage_libspng(memory.address, memory.size, &img_spng_size, SPNG_FMT_RGBA8, 0, &ihdr);
    free(image);
}

size_t save_spng(const Bitmap& bitmap)
{
    const char* filename = "output-spng.png";

    struct spng_ihdr ihdr;

    ihdr.width = bitmap.width;
    ihdr.height = bitmap.height;
    ihdr.bit_depth = 8;
    ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
    ihdr.compression_method = 0;
    ihdr.filter_method = SPNG_DISABLE_FILTERING;
    ihdr.interlace_method = SPNG_INTERLACE_NONE;

    unsigned char *image = bitmap.image;
    size_t image_size = bitmap.width * bitmap.height * 4;

    spng_ctx *enc = spng_ctx_new(SPNG_CTX_ENCODER);

    spng_set_option(enc, SPNG_ENCODE_TO_BUFFER, 1);
    spng_set_option(enc, SPNG_FILTER_CHOICE, SPNG_FILTER_CHOICE_UP); // same as fpng uses
    spng_set_option(enc, SPNG_IMG_COMPRESSION_LEVEL, 3);

    spng_set_ihdr(enc, &ihdr);
    int r = spng_encode_image(enc, image, image_size, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
    if(r)
    {
        printf("spng_encode_image() error: %s\n", spng_strerror(r));
        spng_ctx_free(enc);
        return 0;
    }

    size_t png_size;
    void *png_buf = NULL;

    png_buf = spng_get_png_buffer(enc, &png_size, &r);
    if(png_buf == NULL)
    {
        printf("spng_get_png_buffer() error: %s\n", spng_strerror(r));
    }

    OutputFileStream file(filename);
    file.write(png_buf, png_size);

    free(png_buf);
    spng_ctx_free(enc);
    return get_file_size(filename);
}

#endif

// ----------------------------------------------------------------------
// FPNG
// ----------------------------------------------------------------------

#if defined(ENABLE_FPNG)

#include "fpng/fpng.h"

#if 0
// NOTE: disabled as can only decode fpng encoded files

void load_fpng(Memory memory)
{
    u32 width;
    u32 height;
    u32 channels;
    std::vector<u8> data;
    int x = fpng::fpng_decode_memory(memory.address, memory.size, data, width, height, channels, 4);

    const char* error = nullptr;
    switch (x)
    {
        case fpng::FPNG_DECODE_SUCCESS:
            break;
        case fpng::FPNG_DECODE_NOT_FPNG:
            error = "FPNG_DECODE_NOT_FPNG";
            break;
        case fpng::FPNG_DECODE_INVALID_ARG:
            error = "FPNG_DECODE_INVALID_ARG";
            break;
        case fpng::FPNG_DECODE_FAILED_NOT_PNG:
            error = "FPNG_DECODE_FAILED_NOT_PNG";
            break;
        case fpng::FPNG_DECODE_FAILED_HEADER_CRC32:
            error = "FPNG_DECODE_FAILED_HEADER_CRC32";
            break;
        case fpng::FPNG_DECODE_FAILED_INVALID_DIMENSIONS:
            error = "FPNG_DECODE_FAILED_INVALID_DIMENSIONS";
            break;
        case fpng::FPNG_DECODE_FAILED_DIMENSIONS_TOO_LARGE:
            error = "FPNG_DECODE_FAILED_DIMENSIONS_TOO_LARGE";
            break;
        case fpng::FPNG_DECODE_FAILED_CHUNK_PARSING:
            error = "FPNG_DECODE_FAILED_CHUNK_PARSING";
            break;
        case fpng::FPNG_DECODE_FAILED_INVALID_IDAT:
            error = "FPNG_DECODE_FAILED_INVALID_IDAT";
            break;
    }

    if (error)
    {
        printf("%s\n", error);
    }
}

#endif // 0

size_t save_fpng(const Bitmap& bitmap)
{
    const char* filename = "output-fpng.png";
    fpng::fpng_encode_image_to_file(filename, bitmap.image, bitmap.width, bitmap.height, 4, false);
    return get_file_size(filename);
}

#endif

// ----------------------------------------------------------------------
// fpnge
// ----------------------------------------------------------------------

#if defined(ENABLE_FPNGE)

size_t save_fpnge(const Bitmap& bitmap)
{
    const char* filename = "output-fpnge.png";

    u8* output = nullptr;
    size_t size = FPNGEEncode(1, 4, bitmap.image, bitmap.width, bitmap.stride, bitmap.height, &output);

    OutputFileStream file(filename);
    file.write(output, size);
    free(output);

    return size;
}

#endif

// ----------------------------------------------------------------------
// wuffs
// ----------------------------------------------------------------------

#if defined(ENABLE_WUFFS)

#define WUFFS_IMPLEMENTATION

#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__ADLER32
#define WUFFS_CONFIG__MODULE__AUX__BASE
#define WUFFS_CONFIG__MODULE__AUX__IMAGE
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__BMP
#define WUFFS_CONFIG__MODULE__CRC32
#define WUFFS_CONFIG__MODULE__DEFLATE
#define WUFFS_CONFIG__MODULE__GIF
#define WUFFS_CONFIG__MODULE__LZW
#define WUFFS_CONFIG__MODULE__PNG
#define WUFFS_CONFIG__MODULE__ZLIB

#include "wuffs/wuffs-unsupported-snapshot.c"

class WuffsCallbacks : public wuffs_aux::DecodeImageCallbacks
{
public:
    Buffer buffer;
    Surface surface;

    WuffsCallbacks()
    {
    }

    ~WuffsCallbacks()
    {
    }

private:
    wuffs_base__pixel_format SelectPixfmt(const wuffs_base__image_config& image_config) override
    {
        return wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL);
    }

    AllocPixbufResult AllocPixbuf(const wuffs_base__image_config& image_config, bool allow_uninitialized_memory) override
    {
        u32 width = image_config.pixcfg.width();
        u32 height = image_config.pixcfg.height();
        buffer.resize(width * height * 4);

        surface = Surface(width, height, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), width * 4, buffer);

        wuffs_base__pixel_buffer pixbuf;
        wuffs_base__status status = pixbuf.set_interleaved(
            &image_config.pixcfg,
            wuffs_base__make_table_u8(buffer, surface.stride, surface.height, surface.stride),
            wuffs_base__empty_slice_u8());
        if (!status.is_ok())
        {
            return AllocPixbufResult(status.message());
        }
        return AllocPixbufResult(wuffs_aux::MemOwner(NULL, &free), pixbuf);
    }
};

void load_wuffs(Memory memory)
{
    wuffs_aux::sync_io::MemoryInput input(memory.address, memory.size);
    WuffsCallbacks cb;

    wuffs_aux::DecodeImageResult res = wuffs_aux::DecodeImage(cb, input);
    if (!res.error_message.empty())
    {
        printf("%s\n", res.error_message.c_str());
    }
}

#endif

// ----------------------------------------------------------------------
// mango
// ----------------------------------------------------------------------

#if defined(ENABLE_MANGO)

void load_mango(Memory memory)
{
    ImageDecoder decoder(memory, ".png");

    ImageHeader header = decoder.header();
    Bitmap bitmap(header.width, header.height, header.format);

    ImageDecodeOptions options;
    options.multithread = g_option_multithread;

    decoder.decode(bitmap, options);
}

size_t save_mango(const Bitmap& bitmap)
{
    const char* filename = "output-mango.png";

    ImageEncodeOptions options;
    options.compression = g_option_compression;
    bitmap.save(filename, options);

    return get_file_size(filename);
}

#endif

// ----------------------------------------------------------------------
// main()
// ----------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printf("Too few arguments. usage: <filename.png>\n");
        exit(1);
    }

    printf("%s\n", getSystemInfo().c_str());

    const char* filename = argv[1];

    for (int i = 2; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--nomt"))
        {
            g_option_multithread = false;
        }
        else if (!strcmp(argv[i], "-compression") && i <= (argc - 2))
        {
            g_option_compression = std::atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--debug"))
        {
            debugPrintEnable(true);
        }
        else
        {
            g_option_count = std::atoi(argv[i]);
        }
    }

    Bitmap bitmap(filename, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

    File file(filename);
    Buffer buffer(file);

    printf("image: %d x %d (%d KB)\n", bitmap.width, bitmap.height, int(file.size() / 1024));
    printf("---------------------------------------------------\n");
    printf("          decode(ms)  encode(ms)   size(KB)        \n");
    printf("---------------------------------------------------\n");

#if defined ENABLE_LIBPNG
    test("libpng:  ", load_libpng, save_libpng, buffer, bitmap);
#endif

#if defined ENABLE_LODEPNG
    test("lodepng: ", load_lodepng, save_lodepng, buffer, bitmap);
#endif

#if defined(ENABLE_STB)
    test("stb:     ", load_stb, save_stb, buffer, bitmap);
#endif

#if defined(ENABLE_SPNG)
    test("spng:    ", load_spng, save_spng, buffer, bitmap);
#endif

#if defined(ENABLE_FPNG)
    test("fpng:    ", load_none, save_fpng, buffer, bitmap);
#endif

#if defined(ENABLE_FPNGE)
    test("fpnge:   ", load_none, save_fpnge, buffer, bitmap);
#endif

#if defined(ENABLE_WUFFS)
    test("wuffs:   ", load_wuffs, save_none, buffer, bitmap);
#endif

#if defined(ENABLE_MANGO)
    test("mango:   ", load_mango, save_mango, buffer, bitmap);
#endif

}

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;
using namespace mango::image;

#define TEST_LIBJPEG
#define TEST_STB
//#define TEST_JPEG_COMPRESSOR
//#define TEST_JPEGDEC
#define TEST_TOOJPEG
#define TEST_WUFFS

// ----------------------------------------------------------------------
// utils
// ----------------------------------------------------------------------

static constexpr u64 NOT_AVAILABLE = ~0;

static
size_t get_file_size(const char* filename)
{
    File file(filename);
    return file.size();
}

static
std::string format_time(u64 time)
{
    if (time != NOT_AVAILABLE)
        return fmt::format("{:7}.{} ms ", time / 1000, (time % 1000) / 100);
    else
        return("         N/A ");
}

static
void print(const char* name, u64 load, u64 save, u64 size)
{
    std::string s;
    s = fmt::format("{}", name);
    s += format_time(load);
    s += format_time(save);
    s += fmt::format("  {:8}", size / 1024);
    printLine(s);
}

static
void warmup(const char* filename)
{
    File file(filename);
    ConstMemory memory = file;
    std::vector<char> buffer(memory.size);
    std::memcpy(buffer.data(), memory.address, memory.size);

    ImageDecoder decoder(memory, filename);
    ImageHeader header = decoder.header();
    printLine("image: {} x {} ({} KB)", header.width, header.height, memory.size / 1024);
}

// ----------------------------------------------------------------------
// libjpeg
// ----------------------------------------------------------------------

#ifdef TEST_LIBJPEG

#include <jpeglib.h>
#include <jerror.h>

Surface load_jpeg(const char* filename)
{
    FILE* file = fopen(filename, "rb" );
    if (!file)
    {
        return Surface();
    }

    struct jpeg_decompress_struct info;
    struct jpeg_error_mgr err;

    info.err = jpeg_std_error(&err);
    jpeg_create_decompress(&info);

    jpeg_stdio_src(&info, file);
    jpeg_read_header(&info, TRUE);

    jpeg_start_decompress(&info);

    int w = info.output_width;
    int h = info.output_height;
    int numChannels = info.num_components; // 3 = RGB, 4 = RGBA
    unsigned long dataSize = w * h * numChannels;

    // read scanlines one at a time & put bytes in jdata[] array (assumes an RGB image)
    unsigned char *data = new u8[dataSize];;
    unsigned char *rowptr[ 1 ]; // array or pointers
    for ( ; info.output_scanline < info.output_height ; )
    {
        rowptr[ 0 ] = data + info.output_scanline * w * numChannels;
        jpeg_read_scanlines( &info, rowptr, 1 );
    }

    jpeg_finish_decompress(&info);

    fclose(file);

    Format format = Format(24, Format::UNORM, Format::RGB, 8, 8, 9);
    if (numChannels == 4)
        format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

    return Surface(w, h, format, w * numChannels, data);
}

size_t save_jpeg(const char* filename, const Surface& surface)
{
    struct jpeg_compress_struct cinfo;
    jpeg_create_compress(&cinfo);

    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);

    FILE* outfile;
    if ((outfile = fopen(filename, "wb")) == NULL)
    {
        printLine("can't open {}", filename);
        exit(1);
    }
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = surface.width;
    cinfo.image_height = surface.height;
    cinfo.input_components = surface.format.bytes();
    //cinfo.in_color_space = surface.format.bytes() == 3 ? JCS_RGB : JCS_EXT_RGBA;
    cinfo.in_color_space = JCS_RGB;

    int quality = 95;
    bool progressive = false;

    jpeg_set_defaults(&cinfo);
    if (progressive)
    {
        jpeg_simple_progression(&cinfo);
    }
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1];

    while (cinfo.next_scanline < cinfo.image_height)
    {
        row_pointer[0] = surface.image + cinfo.next_scanline * surface.stride;
        int x = jpeg_write_scanlines(&cinfo, row_pointer, 1);
        (void) x;
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(outfile);

    delete[] surface.image;

    return get_file_size(filename);
}

#endif

// ----------------------------------------------------------------------
// stb
// ----------------------------------------------------------------------

#ifdef TEST_STB

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Surface stb_load_jpeg(const char* filename)
{
    int width, height, components;
    u8* rgb = stbi_load(filename, &width, &height, &components, 3);
    if (!rgb)
    {
        printLine("  decoding failure.");
    }

    return Surface(width, height, Format(24, Format::UNORM, Format::RGB, 8, 8, 8), width * 3, rgb);
}

size_t stb_save_jpeg(const char* filename, const Surface& surface)
{
    stbi_write_jpg(filename, surface.width, surface.height, 3, surface.image, surface.width * 3);
    stbi_image_free(surface.image);
    return get_file_size(filename);
}

#endif

// ----------------------------------------------------------------------
// jpeg-compressor
// ----------------------------------------------------------------------

#ifdef TEST_JPEG_COMPRESSOR

#include "jpeg-compressor/jpgd.h"
#include "jpeg-compressor/jpge.h"

Surface jpgd_load(const char* filename)
{
    int width;
    int height;
    int comps;
    u8* image = jpgd::decompress_jpeg_image_from_file(filename, &width, &height, &comps, 4);
    return Surface(width, height, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), width * 4, image);
}

size_t jpge_save(const char* filename, const Surface& surface)
{
    jpge::compress_image_to_jpeg_file(filename, surface.width, surface.height, 4, surface.image);
    free(surface.image);
    return get_file_size(filename);
}

#endif

// ----------------------------------------------------------------------
// jpegdec
// ----------------------------------------------------------------------

#ifdef TEST_JPEGDEC

#include "jpegdec/JPEGDEC.h"

Bitmap* jpegdec_bitmap = nullptr;

int jpegdec_draw(JPEGDRAW *draw)
{
    u16* src = draw->pPixels;
    u8* dest = jpegdec_bitmap->address(draw->x, draw->y);
    size_t stride = jpegdec_bitmap->stride;

    for (int y = 0; y < draw->iHeight; ++y)
    {
        std::memcpy(dest, src, draw->iWidth * 2);
        src += draw->iWidth;
        dest += stride;
    }

    return 1;
}

Surface jpegdec_load(const char* filename)
{
    File file(filename);

    JPEGDEC decoder;

    if (!decoder.openRAM(const_cast<u8*>(file.data()), int(file.size()), jpegdec_draw))
    {
        printLine("JPEGDEC::openRAM() failed.");
        return Surface();
    }

    int width = decoder.getWidth();
    int height = decoder.getHeight();
    Bitmap bitmap(width, height, Format(16, Format::UNORM, Format::BGR, 5, 6, 5, 0));

    jpegdec_bitmap = &bitmap;

    if (!decoder.decode(0, 0, 0))
    {
        printLine("JPEGDEC::decode() failed.");
        return Surface();
    }

    return Surface();
}

size_t jpegdec_save(const char* filename, const Surface& surface)
{
    // NOT SUPPORTED
    return 0;
}

#endif

// ----------------------------------------------------------------------
// toojpeg
// ----------------------------------------------------------------------

#ifdef TEST_TOOJPEG

#include "toojpeg/toojpeg.h"

OutputFileStream* toojpeg_stream = nullptr;
u8 toojpeg_temp[4096];
u32 toojpeg_temp_offset = 0;

void toojpeg_write_byte(u8 value)
{
    toojpeg_temp[toojpeg_temp_offset++] = value;

    if (toojpeg_temp_offset == 4096)
    {
        toojpeg_stream->write(toojpeg_temp, 4096);
        toojpeg_temp_offset = 0;
    }
}

size_t toojpeg_save(const char* filename, const Surface& surface)
{
    OutputFileStream file(filename);

    toojpeg_stream = &file;
    toojpeg_temp_offset = 0;

    bool status = TooJpeg::writeJpeg(toojpeg_write_byte, surface.image, surface.width, surface.height);
    if (!status)
    {
        printLine("Encode failed.");
    }

    // flush temp buffer
    toojpeg_stream->write(toojpeg_temp, toojpeg_temp_offset);

    return file.size();
}

#endif

// ----------------------------------------------------------------------
// wuffs
// ----------------------------------------------------------------------

#ifdef TEST_WUFFS

#define WUFFS_IMPLEMENTATION

#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__AUX__BASE
#define WUFFS_CONFIG__MODULE__AUX__IMAGE
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__JPEG

#include "../png_benchmark/wuffs/wuffs-v0.4.c"

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

void load_wuffs(const char* filename)
{
    File file(filename);
    ConstMemory memory = file;

    wuffs_aux::sync_io::MemoryInput input(memory.address, memory.size);
    WuffsCallbacks cb;

    wuffs_aux::DecodeImageResult res = wuffs_aux::DecodeImage(cb, input);
    if (!res.error_message.empty())
    {
        printLine(res.error_message);
    }
}

#endif

// ----------------------------------------------------------------------
// main()
// ----------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printLine("Too few arguments. usage: <filename.jpg>");
        exit(1);
    }

    printLine(getSystemInfo());

    const char* filename = argv[1];
    warmup(filename);

    int test_count = 0;
    bool multithread = true;
    bool tracing = false;

    for (int i = 2; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--nomt"))
        {
            multithread = false;
        }
        else if (!strcmp(argv[i], "--debug"))
        {
            printEnable(Print::Info, true);
        }
        else if (!strcmp(argv[i], "--trace"))
        {
            tracing = true;
        }
        else
        {
            test_count = std::atoi(argv[i]);
        }
    }

    printLine("-----------------------------------------------------");
    printLine("           decode(ms)   encode(ms)   size(KB)        ");
    printLine("-----------------------------------------------------");

    u64 time0;
    u64 time1;
    u64 time2;
    size_t size;

    // ------------------------------------------------------------------

#ifdef TEST_LIBJPEG

    time0 = Time::us();
    Surface s = load_jpeg(filename);

    time1 = Time::us();
    size = save_jpeg("output-libjpeg.jpg", s);

    time2 = Time::us();
    ::print("libjpeg: ", time1 - time0, time2 - time1, size);

#endif

    // ------------------------------------------------------------------

#ifdef TEST_STB

    time0 = Time::us();
    Surface s_stb = stb_load_jpeg(filename);

    time1 = Time::us();
    size = stb_save_jpeg("output-stb.jpg", s_stb);

    time2 = Time::us();
    ::print("stb:     ", time1 - time0, time2 - time1, size);

#endif

    // ------------------------------------------------------------------

#ifdef TEST_JPEG_COMPRESSOR

    time0 = Time::us();
    Surface s_jpgd = jpgd_load(filename);

    time1 = Time::us();
    size = jpge_save("output-jpge.jpg", s_jpgd);

    time2 = Time::us();
    ::print("jpgd:    ", time1 - time0, time2 - time1, size);

#endif

    // ------------------------------------------------------------------

#ifdef TEST_JPEGDEC

    time0 = Time::us();
    Surface s_jpegdec = jpegdec_load(filename);

    time1 = Time::us();
    size = jpegdec_save("output-jpegdec.jpg", s_jpegdec);

    time2 = Time::us();
    ::print("jpgdec:  ", time1 - time0, NOT_AVAILABLE, size);

#endif

    // ------------------------------------------------------------------

#ifdef TEST_TOOJPEG

    // toojpeg is encoder-only so we'll provide the input for it
    Bitmap toojpeg_bitmap(filename, Format(24, Format::UNORM, Format::RGB, 8, 8, 8));

    time1 = Time::us();
    size = toojpeg_save("output-toojpeg.jpg", toojpeg_bitmap);

    time2 = Time::us();
    ::print("toojpeg: ", NOT_AVAILABLE, time2 - time1, size);

#endif

    // ------------------------------------------------------------------

#ifdef TEST_WUFFS

    time0 = Time::us();
    load_wuffs(filename);

    time1 = Time::us();
    ::print("wuffs:   ", time1 - time0, NOT_AVAILABLE, 0);

#endif

    // ------------------------------------------------------------------

    time0 = Time::us();

    ImageDecodeOptions decode_options;
    decode_options.simd = true;
    decode_options.multithread = multithread;

    std::unique_ptr<filesystem::OutputFileStream> output;

    if (tracing)
    {
        output = std::make_unique<filesystem::OutputFileStream>("result.trace");
        startTrace(output.get());
    }

    Bitmap bitmap(filename, decode_options);

    if (tracing)
    {
        stopTrace();
    }

    time1 = Time::us();

    ImageEncodeOptions encode_options;
    encode_options.quality = 0.70f;
    encode_options.simd = true;
    encode_options.multithread = multithread;

    bitmap.save("output-mango.jpg", encode_options);
    size = get_file_size("output-mango.jpg");

    time2 = Time::us();
    ::print("mango:   ", time1 - time0, time2 - time1, size);

    // ------------------------------------------------------------------

    if (test_count > 0)
    {
        u64 load_total = time1 - time0;
        u64 save_total = time2 - time1;
        u64 load_lowest = load_total;
        u64 save_lowest = save_total;

        for (int i = 0; i < test_count; ++i)
        {
            time0 = Time::us();
            Bitmap bitmap(filename, decode_options);

            time1 = Time::us();
            bitmap.save("output-mango.jpg", encode_options);

            time2 = Time::us();

            u64 load = time1 - time0;
            u64 save = time2 - time1;
            load_total += load;
            save_total += save;
            load_lowest = std::min(load_lowest, load);
            save_lowest = std::min(save_lowest, save);
            ::print("         ", load, save, size);
        }

        printLine("----------------------------------------------");
        ::print("average: ", load_total / (test_count + 1), save_total / (test_count + 1), size);
        ::print("lowest : ", load_lowest, save_lowest, size);
        printLine("----------------------------------------------");
    }

}

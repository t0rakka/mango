/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_JP2)

#include <openjpeg.h>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // OpenJPEG interface
    // ------------------------------------------------------------

    struct MemoryStreamReader
    {
        ConstMemory memory;
        size_t offset;
        opj_stream_t* stream;

        MemoryStreamReader(ConstMemory memory)
            : memory(memory)
            , offset(0)
            , stream(nullptr)
        {
            constexpr bool is_reader = true;

            stream = opj_stream_default_create(is_reader);
            if (stream)
            {
                opj_stream_set_read_function(stream, stream_read);
                opj_stream_set_seek_function(stream, stream_seek);
                opj_stream_set_skip_function(stream, stream_skip);
                opj_stream_set_user_data(stream, this, stream_free_user_data);
                opj_stream_set_user_data_length(stream, memory.size);
            }
        }

        ~MemoryStreamReader()
        {
            if (stream)
            {
                opj_stream_destroy(stream);
            }
        }

        static
        OPJ_SIZE_T stream_read(void* buffer, OPJ_SIZE_T bytes, void* data)
        {
            MemoryStreamReader& reader = *reinterpret_cast<MemoryStreamReader*>(data);

            if (reader.offset >= reader.memory.size)
                return (OPJ_SIZE_T) -1;

            if (bytes > (reader.memory.size - reader.offset))
                bytes = reader.memory.size - reader.offset;

            std::memcpy(buffer, reader.memory + reader.offset, bytes);
            reader.offset += bytes;

            return bytes;
        }

        static
        OPJ_OFF_T stream_skip(OPJ_OFF_T bytes, void* data)
        {
            MemoryStreamReader& reader = *reinterpret_cast<MemoryStreamReader*>(data);

            if (bytes < 0)
                return -1;

            if (bytes > reader.memory.size - reader.offset)
                bytes = reader.memory.size - reader.offset;

            reader.offset += bytes;

            return bytes;
        }

        static
        OPJ_BOOL stream_seek(OPJ_OFF_T bytes, void* data)
        {
            MemoryStreamReader& reader = *reinterpret_cast<MemoryStreamReader*>(data);

            if (bytes < 0 || bytes > (OPJ_OFF_T)reader.memory.size)
                return OPJ_FALSE;

            reader.offset = (OPJ_SIZE_T)bytes;

            return OPJ_TRUE;
        }

        static
        void stream_free_user_data(void* data)
        {
            OPJ_ARG_NOT_USED(data);
        }
    };

    struct MemoryStreamWriter
    {
        opj_stream_t* stream;

        MemoryStreamWriter()
            : stream(nullptr)
        {
            constexpr bool is_reader = false;

            opj_stream_t* stream = opj_stream_default_create(is_reader);
            if (stream)
            {
                opj_stream_set_write_function(stream, stream_write);
                // TODO: other callbacks
            }
        }

        ~MemoryStreamWriter()
        {
            if (stream)
            {
                opj_stream_destroy(stream);
            }
        }

        static
        OPJ_SIZE_T stream_write(void* buffer, OPJ_SIZE_T bytes, void* data)
        {
            /* TODO
            opj_memory_stream* memory = (opj_memory_stream*)user_data;

            if (memory->offset >= memory->size)
                return (OPJ_SIZE_T) -1;

            if (bytes > (memory->size - memory->offset))
                bytes = memory->size - memory->offset;

            std::memcpy(memory->data + memory->offset, buffer, bytes);
            memory->offset += bytes;

            return bytes;
            */
            return 0;
        }
    };

    static constexpr
    u8 JP2_RFC3745_MAGIC [] =
    {
        0x00, 0x00, 0x00, 0x0c,
        0x6a, 0x50, 0x20, 0x20,
        0x0d, 0x0a, 0x87, 0x0a
    };

    static constexpr
    u8 J2K_CODESTREAM_MAGIC [] =
    {
        0xff, 0x4f, 0xff, 0x51
    };

    static
    opj_codec_t* create_codec(ConstMemory memory)
    {
        if (memory.size < 12)
        {
            return nullptr;
        }

        debugPrint("[header]\n");
        debugPrint("  magic: | ");
        for (int i = 0; i < 12; ++i)
        {
            debugPrint("%.2x ", memory[i]);
            if (!((i - 3) % 4))
                debugPrint("| ");
        }
        debugPrint("\n");

        opj_codec_t* codec = nullptr;

        if (!std::memcmp(memory.address, JP2_RFC3745_MAGIC, 12) ||
            !std::memcmp(memory.address, JP2_RFC3745_MAGIC + 8, 4))
        {
            debugPrint("  codec: JP2\n");
            codec = opj_create_decompress(OPJ_CODEC_JP2);
        }
        else if (!std::memcmp(memory.address, J2K_CODESTREAM_MAGIC, 4))
        {
            debugPrint("  codec: J2K\n");
            codec = opj_create_decompress(OPJ_CODEC_J2K);
        }

        return codec;
    }

    using ImageProcessFunc = void (*)(const Surface& surface, const opj_image_t& image);

    static
    void process_generic_1_comp(const Surface& surface, const opj_image_t& image)
    {
        int width = surface.width;
        int height = surface.height;
        size_t stride = surface.stride;

        for (int y = 0; y < height; ++y)
        {
            u8* dest = surface.image + y * stride;

            s32* src0 = image.comps[0].data + (y / image.comps[0].dy) * image.comps[0].w;
            u32 prec0 = image.comps[0].prec;
            u32 bias0 = image.comps[0].sgnd ? 1 << (prec0 - 1) : 0;

            for (int x = 0; x < width; ++x)
            {
                u32 s = src0[x / image.comps[0].dx];
                s = u32_scale(s + bias0, prec0, 8);
                dest[x] = u8(s);
            }

            dest += stride;
        }
    }

    static
    void process_generic_2_comp(const Surface& surface, const opj_image_t& image)
    {
        int width = surface.width;
        int height = surface.height;
        size_t stride = surface.stride;

        for (int y = 0; y < height; ++y)
        {
            u16* dest = reinterpret_cast<u16*>(surface.image + y * stride);

            s32* src0 = image.comps[0].data + (y / image.comps[0].dy) * image.comps[0].w;
            s32* src1 = image.comps[1].data + (y / image.comps[1].dy) * image.comps[1].w;

            u32 prec0 = image.comps[0].prec;
            u32 prec1 = image.comps[1].prec;

            u32 bias0 = image.comps[0].sgnd ? 1 << (prec0 - 1) : 0;
            u32 bias1 = image.comps[1].sgnd ? 1 << (prec1 - 1) : 0;

            for (int x = 0; x < width; ++x)
            {
                u32 s = src0[x / image.comps[0].dx];
                u32 a = src1[x / image.comps[1].dx];
                s = u32_scale(s + bias0, prec0, 8);
                a = u32_scale(a + bias1, prec1, 8);
                dest[x] = u16((a << 8) | s);
            }

            dest += stride;
        }
    }

    static
    void process_generic_3_comp(const Surface& surface, const opj_image_t& image)
    {
        int width = surface.width;
        int height = surface.height;
        size_t stride = surface.stride;

        bool is_yuv = (image.comps[0].dx == 1 && image.comps[0].dy == 1) &&
                       image.comps[1].dx != 1;

        for (int y = 0; y < height; ++y)
        {
            u32* dest = reinterpret_cast<u32*>(surface.image + y * stride);

            s32* src0 = image.comps[0].data + (y / image.comps[0].dy) * image.comps[0].w;
            s32* src1 = image.comps[1].data + (y / image.comps[1].dy) * image.comps[1].w;
            s32* src2 = image.comps[2].data + (y / image.comps[2].dy) * image.comps[2].w;

            u32 prec0 = image.comps[0].prec;
            u32 prec1 = image.comps[1].prec;
            u32 prec2 = image.comps[2].prec;

            u32 bias0 = image.comps[0].sgnd ? 1 << (prec0 - 1) : 0;
            u32 bias1 = image.comps[1].sgnd ? 1 << (prec1 - 1) : 0;
            u32 bias2 = image.comps[2].sgnd ? 1 << (prec2 - 1) : 0;

            for (int x = 0; x < width; ++x)
            {
                u32 s0 = src0[x / image.comps[0].dx];
                u32 s1 = src1[x / image.comps[1].dx];
                u32 s2 = src2[x / image.comps[2].dx];

                s0 = u32_scale(s0 + bias0, prec0, 8);
                s1 = u32_scale(s1 + bias1, prec1, 8);
                s2 = u32_scale(s2 + bias2, prec2, 8);

                u32 r = s0;
                u32 g = s1;
                u32 b = s2;
                u32 a = 0xff;

                if (is_yuv)
                {
                    s32 cb = s1;
                    s32 cr = s2;
                    r = s0 + ((cr * 91750 - 11711232) >> 16);
                    g = s0 + ((cb * -22479 + cr * -46596 + 8874368) >> 16);
                    b = s0 + ((cb * 115671 - 14773120) >> 16);
                    r = byteclamp(r);
                    g = byteclamp(g);
                    b = byteclamp(b);
                }

                dest[x] = makeRGBA(r, g, b, a);
            }

            dest += stride;
        }
    }

    static
    void process_generic_4_comp(const Surface& surface, const opj_image_t& image)
    {
        int width = surface.width;
        int height = surface.height;
        size_t stride = surface.stride;

        bool is_yuv = (image.comps[0].dx == 1 && image.comps[0].dy == 1) &&
                       image.comps[1].dx != 1;

        for (int y = 0; y < height; ++y)
        {
            u32* dest = reinterpret_cast<u32*>(surface.image + y * stride);

            s32* src0 = image.comps[0].data + (y / image.comps[0].dy) * image.comps[0].w;
            s32* src1 = image.comps[1].data + (y / image.comps[1].dy) * image.comps[1].w;
            s32* src2 = image.comps[2].data + (y / image.comps[2].dy) * image.comps[2].w;
            s32* src3 = image.comps[3].data + (y / image.comps[3].dy) * image.comps[3].w;

            u32 prec0 = image.comps[0].prec;
            u32 prec1 = image.comps[1].prec;
            u32 prec2 = image.comps[2].prec;
            u32 prec3 = image.comps[3].prec;

            u32 bias0 = image.comps[0].sgnd ? 1 << (prec0 - 1) : 0;
            u32 bias1 = image.comps[1].sgnd ? 1 << (prec1 - 1) : 0;
            u32 bias2 = image.comps[2].sgnd ? 1 << (prec2 - 1) : 0;
            u32 bias3 = image.comps[3].sgnd ? 1 << (prec3 - 1) : 0;

            for (int x = 0; x < width; ++x)
            {
                u32 s0 = src0[x / image.comps[0].dx];
                u32 s1 = src1[x / image.comps[1].dx];
                u32 s2 = src2[x / image.comps[2].dx];
                u32 s3 = src3[x / image.comps[3].dx];

                s0 = u32_scale(s0 + bias0, prec0, 8);
                s1 = u32_scale(s1 + bias1, prec1, 8);
                s2 = u32_scale(s2 + bias2, prec2, 8);
                s3 = u32_scale(s3 + bias3, prec3, 8);

                u32 r = s0;
                u32 g = s1;
                u32 b = s2;
                u32 a = s3;

                if (is_yuv)
                {
                    s32 cb = s1;
                    s32 cr = s2;
                    r = s0 + ((cr * 91750 - 11711232) >> 16);
                    g = s0 + ((cb * -22479 + cr * -46596 + 8874368) >> 16);
                    b = s0 + ((cb * 115671 - 14773120) >> 16);
                    r = byteclamp(r);
                    g = byteclamp(g);
                    b = byteclamp(b);
                }

                dest[x] = makeRGBA(r, g, b, a);
            }

            dest += stride;
        }
    }

    static
    void process_unorm_8bit_y(const Surface& surface, const opj_image_t& image)
    {
        int width = surface.width;
        int height = surface.height;
        size_t stride = surface.stride;

        for (int y = 0; y < height; ++y)
        {
            u8* dest = surface.image + y * stride;
            s32* src0 = image.comps[0].data + y * image.comps[0].w;

            for (int x = 0; x < width; ++x)
            {
                u32 s = src0[x];
                dest[x] = u8(s);
            }

            dest += stride;
        }
    }

    static
    void process_unorm_8bit_ya(const Surface& surface, const opj_image_t& image)
    {
        int width = surface.width;
        int height = surface.height;
        size_t stride = surface.stride;

        for (int y = 0; y < height; ++y)
        {
            u16* dest = reinterpret_cast<u16*>(surface.image + y * stride);
            s32* src0 = image.comps[0].data + y * image.comps[0].w;
            s32* src1 = image.comps[1].data + y * image.comps[1].w;

            for (int x = 0; x < width; ++x)
            {
                u32 s = src0[x];
                u32 a = src1[x];
                dest[x] = u16((a << 8) | s);
            }

            dest += stride;
        }
    }

    static
    void process_unorm_8bit_rgb(const Surface& surface, const opj_image_t& image)
    {
        int width = surface.width;
        int height = surface.height;
        size_t stride = surface.stride;

        for (int y = 0; y < height; ++y)
        {
            u32* dest = reinterpret_cast<u32*>(surface.image + y * stride);

            s32* src0 = image.comps[0].data + y * image.comps[0].w;
            s32* src1 = image.comps[1].data + y * image.comps[1].w;
            s32* src2 = image.comps[2].data + y * image.comps[2].w;

            for (int x = 0; x < width; ++x)
            {
                s32 r = src0[x];
                s32 g = src1[x];
                s32 b = src2[x];
                s32 a = 0xff;
                dest[x] = makeRGBA(r, g, b, a);
            }

            dest += stride;
        }
    }

    static
    void process_unorm_8bit_rgba(const Surface& surface, const opj_image_t& image)
    {
        int width = surface.width;
        int height = surface.height;
        size_t stride = surface.stride;

        for (int y = 0; y < height; ++y)
        {
            u32* dest = reinterpret_cast<u32*>(surface.image + y * stride);

            s32* src0 = image.comps[0].data + y * image.comps[0].w;
            s32* src1 = image.comps[1].data + y * image.comps[1].w;
            s32* src2 = image.comps[2].data + y * image.comps[2].w;
            s32* src3 = image.comps[3].data + y * image.comps[3].w;

            for (int x = 0; x < width; ++x)
            {
                s32 r = src0[x];
                s32 g = src1[x];
                s32 b = src2[x];
                s32 a = src3[x];
                dest[x] = makeRGBA(r, g, b, a);
            }

            dest += stride;
        }
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ImageHeader m_header;
        Memory m_icc;

        ImageProcessFunc m_process_func = nullptr;
        MemoryStreamReader m_memory_reader;

        opj_codec_t* m_codec = nullptr;
        opj_stream_t* m_stream = nullptr;
        opj_image_t* m_image = nullptr;

        Interface(ConstMemory memory)
            : m_memory_reader(memory)
        {
            m_codec = create_codec(memory);
            if (!m_codec)
            {
                m_header.setError("[ImageDecoder.JP2] Incorrect identifier.");
                return;
            }

            m_stream = m_memory_reader.stream;
            if (!m_stream)
            {
                m_header.setError("[ImageDecoder.JP2] opj_stream_create_default_memory_stream FAILED.");
                return;
            }

            opj_dparameters_t parameters;
            opj_set_default_decoder_parameters(&parameters);

            if (!opj_setup_decoder(m_codec, &parameters))
            {
                m_header.setError("[ImageDecoder.JP2] opj_setup_decoder FAILED.");
                return;
            }

            //opj_set_info_handler(m_codec, info_callback, nullptr);
            //opj_set_warning_handler(m_codec, warning_callback, nullptr);
            //opj_set_error_handler(m_codec, error_callback, nullptr);

            size_t num_thread = std::max(std::thread::hardware_concurrency(), 1u);
            opj_codec_set_threads(m_codec, num_thread);

            if (!opj_read_header(m_stream, m_codec, &m_image))
            {
                m_header.setError("[ImageDecoder.JP2] opj_read_header FAILED.");
                return;
            }

            if (!m_image)
            {
                m_header.setError("[ImageDecoder.JP2] Incorrect image.");
                return;
            }

            if (m_image->x0 || m_image->y0)
            {
                debugPrint("WARNING: unsupported offset (%d, %d)\n", m_image->x0, m_image->y0);
                return;
            }

            // ICC color profile
            m_icc.address = m_image->icc_profile_buf;
            m_icc.size = m_image->icc_profile_len;

            int width = m_image->x1; // - m_image->x0;
            int height = m_image->y1; // - m_image->y0;
            int components = m_image->numcomps;

            Format format;

            switch (components)
            {
                case 1:
                    format = LuminanceFormat(8, Format::UNORM, 8, 0);
                    m_process_func = process_generic_1_comp;
                    break;

                case 2:
                    format = LuminanceFormat(16, Format::UNORM, 8, 8);
                    m_process_func = process_generic_2_comp;
                    break;

                case 3:
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    m_process_func = process_generic_3_comp;
                    break;

                case 4:
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    m_process_func = process_generic_4_comp;
                    break;

                default:
                    m_header.setError("[ImageDecoder.JP2] Incorrect number of components (%d).", components);
                    return;
            }

            debugPrint("[image]\n");
            debugPrint("  dimensions: %d x %d\n", width, height);
            debugPrint("  color space: %d\n", m_image->color_space);

            debugPrint("[components]\n");

            bool is_signed = false;
            bool is_subsampled = false;
            bool is_8bit = true;

            for (int i = 0; i < components; ++i)
            {
                const opj_image_comp_t& comp = m_image->comps[i];

                debugPrint("  #%d: %d x %d, bits: %d, alpha: %d, sgnd: %d, dx: %d, dy: %d\n", 
                    i, comp.w, comp.h, comp.prec, comp.alpha, comp.sgnd, comp.dx, comp.dy);

                if (comp.w != width || comp.h != height || comp.dx != 1 || comp.dy != 1)
                {
                    is_subsampled = true;
                }

                if (comp.sgnd)
                {
                    is_signed = true;
                }

                if (comp.prec != 8)
                {
                    is_8bit = false;
                }
            }

            bool is_standard = is_8bit && !is_signed && !is_subsampled;

            switch (m_image->color_space)
            {
                case OPJ_CLRSPC_UNKNOWN:
                    m_header.setError("[ImageDecoder.JP2] Unknown color space (%d).", m_image->color_space);
                    return;

                case OPJ_CLRSPC_UNSPECIFIED:
                    // Determine heuristically
                    switch (components)
                    {
                        case 1:
                            if (is_standard)
                            {
                                m_process_func = process_unorm_8bit_y;
                            }
                            break;
                        case 2:
                            if (is_standard)
                            {
                                m_process_func = process_unorm_8bit_ya;
                            }
                            break;
                        case 3:
                            if (is_standard)
                            {
                                m_process_func = process_unorm_8bit_rgb;
                            }
                            break;
                        case 4:
                            if (is_standard)
                            {
                                m_process_func = process_unorm_8bit_rgba;
                            }
                            break;
                    }
                    break;

                case OPJ_CLRSPC_SRGB:
                    if (components < 3)
                    {
                        m_header.setError("[ImageDecoder.JP2] Incorrect number of components (%d).", components);
                        return;
                    }
                    if (is_standard)
                    {
                        if (components == 3)
                            m_process_func = process_unorm_8bit_rgb;
                        else
                            m_process_func = process_unorm_8bit_rgba;
                    }
                    break;

                case OPJ_CLRSPC_GRAY:
                    if (components > 2)
                    {
                        m_header.setError("[ImageDecoder.JP2] Incorrect number of components (%d).", components);
                        return;
                    }
                    if (is_standard)
                    {
                        if (components == 1)
                        {
                            m_process_func = process_unorm_8bit_y;
                        }
                        else
                        {
                            m_process_func = process_unorm_8bit_ya;
                        }
                    }
                    break;

                case OPJ_CLRSPC_SYCC:
                case OPJ_CLRSPC_EYCC:
                case OPJ_CLRSPC_CMYK:
                    m_header.setError("[ImageDecoder.JP2] Unsupported color space (%d).", m_image->color_space);
                    return;

                default:
                    m_header.setError("[ImageDecoder.JP2] Incorrect color space (%d).", m_image->color_space);
                    return;
            }

            m_header.width   = width;
            m_header.height  = height;
            m_header.depth   = 0;
            m_header.levels  = 0;
            m_header.faces   = 0;
            m_header.palette = false;
            m_header.format  = format;
            m_header.compression = TextureCompression::NONE;
        }

        ~Interface()
        {
            if (m_image)
            {
                opj_image_destroy(m_image);
            }

            if (m_codec)
            {
                opj_destroy_codec(m_codec);
            }
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ConstMemory icc() override
        {
            return m_icc;
        }

        ConstMemory exif() override
        {
            return ConstMemory();
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            if (options.multithread)
            {
                // NOTE: It is too late to configure the number of threads here;
                //       it has to be done before opj_read_header is called.
            }

            ImageDecodeStatus status;

            if (!opj_set_decode_area(m_codec, m_image, 0, 0, 0, 0))
            {
                status.setError("[ImageDecoder.JP2] opj_set_decode_area FAILED.");
                return status;
            }

            if (!opj_decode(m_codec, m_stream, m_image))
            {
                status.setError("[ImageDecoder.JP2] opj_decode FAILED.");
                return status;
            }

            if (!opj_end_decompress(m_codec, m_stream))
            {
                status.setError("[ImageDecoder.JP2] opj_end_decompress FAILED.");
                return status;
            }

            if (m_process_func)
            {
                int width = m_header.width;
                int height = m_header.height;
                Bitmap bitmap(width, height, m_header.format);

                m_process_func(bitmap, *m_image);
                dest.blit(0, 0, bitmap);
            }

            return status;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    /*
    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;
        // TODO

        return status;
    }
    */

} // namespace

namespace mango::image
{

    void registerImageCodecJP2()
    {
        // TODO
        registerImageDecoder(createInterface, ".jp2");

        // TODO: seperate register call
        registerImageDecoder(createInterface, ".j2k");
        registerImageDecoder(createInterface, ".j2c");
        registerImageDecoder(createInterface, ".jpc");

        // TODO
        //registerImageEncoder(imageEncode, ".jp2");
    }

} // namespace mango::image

#endif // defined(MANGO_ENABLE_JP2)

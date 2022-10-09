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

    /*
    static
    void process_none(const Surface& surface, const opj_image_t& image)
    {
    }
    */

    static
    void process_unorm_8bit_rgb(const Surface& surface, const opj_image_t& image)
    {
        int width = surface.width;
        int height = surface.height;
        size_t stride = surface.stride;

        for (int y = 0; y < height; ++y)
        {
            s32* src0 = image.comps[0].data + y * image.comps[0].w;
            s32* src1 = image.comps[1].data + y * image.comps[1].w;
            s32* src2 = image.comps[2].data + y * image.comps[2].w;
            Color* dest = reinterpret_cast<Color*>(surface.image + y * stride);

            for (int x = 0; x < width; ++x)
            {
                s32 r = src0[x];
                s32 g = src1[x];
                s32 b = src2[x];
                s32 a = 0xff;
                dest[x] = Color(r, g, b, a);
            }

            dest += stride;
        }
    }

#if 0

    void color_conversion(const Surface& dest)
    {
        // TODO: support different component resolutions
        // TODO: support image offset (x, y)
        // TODO: support different color spaces
        // TODO: support different channel precisions
        // TODO: support sgnd flag
        // TODO: support sRGB flag (add into header)
        // TODO: support direct decoding

        // channel decoder
        const opj_image_comp_t& comp0 = m_image->comps[0];
        const opj_image_comp_t& comp1 = m_image->comps[1];
        const opj_image_comp_t& comp2 = m_image->comps[2];
        //const opj_image_comp_t& comp3 = m_image->comps[3];

        // TODO: should be image->x1 x image->y1
        int width = m_header.width;
        int height = m_header.height;

        Bitmap bitmap(width, height, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        for (int y = 0; y < height; ++y)
        {
            s32* src0 = comp0.data + y * comp0.w;
            s32* src1 = comp1.data + y/2 * comp1.w;
            s32* src2 = comp2.data + y/2 * comp2.w;
            //s32* src3 = comp3.data + y * comp3.w;

            Color* dest = bitmap.address<Color>(0, y);

            for (int x = 0; x < width; ++x)
            {
                /*
                // SNORM Y 16
                s32 s = src0[x];
                s += 32768;
                s = (s * 255) / 65535;
                s32 r = s;
                s32 g = s;
                s32 b = s;
                s32 a = 0xff;
                */

                /*
                // UNORM YA 8 8
                s32 r = src0[x];
                s32 g = r;
                s32 b = r;
                s32 a = src1[x];
                */

                /*
                // UNORM RGBA 8 8 8 8
                s32 r = src0[x];
                s32 g = src1[x];
                s32 b = src2[x];
                s32 a = src3[x];
                */

                //* YUV:
                s32 y0 = src0[x/1];
                s32 cb = src1[x/2];
                s32 cr = src2[x/2];
                s32 r = y0 + ((cr * 91750 - 11711232) >> 16);
                s32 g = y0 + ((cb * -22479 + cr * -46596 + 8874368) >> 16);
                s32 b = y0 + ((cb * 115671 - 14773120) >> 16);
                r = byteclamp(r);
                g = byteclamp(g);
                b = byteclamp(b);
                s32 a = 0xff;
                //*/

                dest[x] = Color(r, g, b, a);
            }
        }

        dest.blit(0, 0, bitmap);
    }

#endif

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

            if (components < 1 || components > 4)
            {
                m_header.setError("[ImageDecoder.JP2] Incorrect number of components (%d).", components);
                return;
            }

            debugPrint("[image]\n");
            debugPrint("  dimensions: %d x %d\n", width, height);
            debugPrint("  color space: %d\n", m_image->color_space);

            bool is_signed = false;
            bool is_subsampled = false;
            bool is_8bit = true;

            debugPrint("[components]\n");

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

            switch (m_image->color_space)
            {
                case OPJ_CLRSPC_UNKNOWN:
                    m_header.setError("[ImageDecoder.JP2] Unknown color space (%d).", m_image->color_space);
                    return;

                case OPJ_CLRSPC_UNSPECIFIED:
                    // Determine heuristically
                    break;

                case OPJ_CLRSPC_SRGB:
                    if (components < 3)
                    {
                        m_header.setError("[ImageDecoder.JP2] Incorrect number of components (%d).", components);
                        return;
                    }
                    break;

                case OPJ_CLRSPC_GRAY:
                    if (components > 2)
                    {
                        m_header.setError("[ImageDecoder.JP2] Incorrect number of components (%d).", components);
                        return;
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

            /*
            switch (m_image->comps)
            {
                case 1:
                    break;
                case 2:
                    break;
                case 3:
                    break;
                case 4:
                    break;
                default:
                    break;
            }

            */

            m_process_func = process_unorm_8bit_rgb;

            Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

            switch (m_image->color_space)
            {
                case OPJ_CLRSPC_GRAY:
                    format = LuminanceFormat(8, Format::UNORM, 8, 0);
                    break;
                default:
                    break;
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
                opj_image_destroy(m_image);

            if (m_codec)
                opj_destroy_codec(m_codec);
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
            MANGO_UNREFERENCED(options); // TODO: MT decoding option
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

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
                Bitmap bitmap(width, height, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

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
        /*
        // TODO
        registerImageDecoder(createInterface, ".jp2");

        // TODO: seperate register call
        registerImageDecoder(createInterface, ".j2k");
        registerImageDecoder(createInterface, ".j2c");
        registerImageDecoder(createInterface, ".jpc");
        */

        // TODO
        //registerImageEncoder(imageEncode, ".jp2");
    }

} // namespace mango::image

#endif // defined(MANGO_ENABLE_JP2)

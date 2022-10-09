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

    struct opj_memory_stream
    {
        u8* data;
        size_t size;
        size_t offset;
    };

    static
    OPJ_SIZE_T opj_memory_stream_read(void* buffer, OPJ_SIZE_T bytes, void* user_data)
    {
        opj_memory_stream* memory = (opj_memory_stream*)user_data;

        if (memory->offset >= memory->size)
            return (OPJ_SIZE_T) -1;

        if (bytes > (memory->size - memory->offset))
            bytes = memory->size - memory->offset;

        std::memcpy(buffer, memory->data + memory->offset, bytes);
        memory->offset += bytes;

        return bytes;
    }

    static
    OPJ_SIZE_T opj_memory_stream_write(void* buffer, OPJ_SIZE_T bytes, void* user_data)
    {
        opj_memory_stream* memory = (opj_memory_stream*)user_data;

        if (memory->offset >= memory->size)
            return (OPJ_SIZE_T) -1;

        if (bytes > (memory->size - memory->offset))
            bytes = memory->size - memory->offset;

        std::memcpy(memory->data + memory->offset, buffer, bytes);
        memory->offset += bytes;

        return bytes;
    }

    static
    OPJ_OFF_T opj_memory_stream_skip(OPJ_OFF_T bytes, void* user_data)
    {
        opj_memory_stream* memory = (opj_memory_stream*)user_data;

        if (bytes < 0)
            return -1;

        if (bytes > memory->size - memory->offset)
            bytes = memory->size - memory->offset;

        memory->offset += bytes;

        return bytes;
    }

    static
    OPJ_BOOL opj_memory_stream_seek(OPJ_OFF_T bytes, void* user_data)
    {
        opj_memory_stream* memory = (opj_memory_stream*)user_data;

        if (bytes < 0 || bytes > (OPJ_OFF_T)memory->size)
            return OPJ_FALSE;

        memory->offset = (OPJ_SIZE_T)bytes;

        return OPJ_TRUE;
    }

    static
    void opj_memory_stream_do_nothing(void* user_data)
    {
        OPJ_ARG_NOT_USED(user_data);
    }

    opj_stream_t* opj_stream_create_default_memory_stream(opj_memory_stream* memory, OPJ_BOOL is_read_stream)
    {
        opj_stream_t* stream = opj_stream_default_create(is_read_stream);
        if (stream)
        {
            if (is_read_stream)
                opj_stream_set_read_function(stream, opj_memory_stream_read);
            else
                opj_stream_set_write_function(stream, opj_memory_stream_write);

            opj_stream_set_seek_function(stream, opj_memory_stream_seek);
            opj_stream_set_skip_function(stream, opj_memory_stream_skip);
            opj_stream_set_user_data(stream, memory, opj_memory_stream_do_nothing);
            opj_stream_set_user_data_length(stream, memory->size);
        }

        return stream;
    }

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

        printf("magic: | ");
        for (int i = 0; i < 12; ++i)
        {
            printf("%.2x ", memory[i]);
            if (!((i - 3) % 4)) printf("| ");
        }
        printf("\n");

        opj_codec_t* codec = nullptr;

        if (!std::memcmp(memory.address, JP2_RFC3745_MAGIC, 12) ||
            !std::memcmp(memory.address, JP2_RFC3745_MAGIC + 8, 4))
        {
            printf("codec: JP2\n");
            codec = opj_create_decompress(OPJ_CODEC_JP2);
        }
        else if (!std::memcmp(memory.address, J2K_CODESTREAM_MAGIC, 4))
        {
            printf("codec: J2K\n");
            codec = opj_create_decompress(OPJ_CODEC_J2K);
        }

        return codec;
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ImageHeader m_header;
        Memory m_icc;

        opj_memory_stream m_memory_stream;

        opj_codec_t* m_codec = nullptr;
        opj_stream_t* m_stream = nullptr;
        opj_image_t* m_image = nullptr;

        Interface(ConstMemory memory)
        {
            m_codec = create_codec(memory);
            if (!m_codec)
            {
                m_header.setError("[ImageDecoder.JP2] Incorrect identifier.");
                return;
            }

            m_memory_stream.data = const_cast<u8*>(memory.address);
            m_memory_stream.size = memory.size;
            m_memory_stream.offset = 0;

            bool is_reader = true;

            m_stream = opj_stream_create_default_memory_stream(&m_memory_stream, is_reader);
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

            m_icc.address = m_image->icc_profile_buf;
            m_icc.size = m_image->icc_profile_len;

            Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

            switch (m_image->color_space)
            {
                case OPJ_CLRSPC_GRAY:
                    format = LuminanceFormat(8, Format::UNORM, 8, 0);
                    break;
                default:
                    break;
            }

            m_header.width   = m_image->x1;
            m_header.height  = m_image->y1;
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

            if (m_stream)
                opj_stream_destroy(m_stream);

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

            printf("[image]\n");
            printf("  dimensions: %d x %d\n", m_header.width, m_header.height);
            printf("  offset: %d, %d\n", m_image->x0, m_image->y0);
            printf("  color space: %d\n", m_image->color_space);

            printf("[components]\n");
            for (u32 i = 0; i < m_image->numcomps; ++i)
            {
                opj_image_comp_t* comp = m_image->comps + i;
                printf("  #%d: %d x %d, bits: %d, alpha: %d, sgnd: %d\n", 
                    i, comp->w, comp->h, comp->prec, comp->alpha, comp->sgnd);
            }

            color_conversion(dest);

            return status;
        }

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

            // TODO: should be image->x1 x image->y1
            int width = m_header.width;
            int height = m_header.height;

            Bitmap bitmap(width, height, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

            for (int y = 0; y < height; ++y)
            {
                s32* src0 = comp0.data + y * comp0.w;
                s32* src1 = comp1.data + y * comp1.w;
                s32* src2 = comp2.data + y * comp2.w;

                Color* dest = bitmap.address<Color>(0, y);

                /* color spaces:
                OPJ_CLRSPC_UNKNOWN = -1,    // xxx
                OPJ_CLRSPC_UNSPECIFIED = 0, // xxxx
                OPJ_CLRSPC_SRGB = 1,        // sRGB
                OPJ_CLRSPC_GRAY = 2,        // grayscale
                OPJ_CLRSPC_SYCC = 3,        // YUV
                OPJ_CLRSPC_EYCC = 4,        // e-YCC
                OPJ_CLRSPC_CMYK = 5         // CMYK
                */

                for (int x = 0; x < width; ++x)
                {
                    s32 r = src0[x];
                    s32 g = src1[x];
                    s32 b = src2[x];

                    /* YUV:
                    s32 y0 = src0[x];
                    s32 cb = src1[x];
                    s32 cr = src2[x];

                    s32 r = y0 + ((cr * 91750 - 11711232) >> 16);
                    s32 g = y0 + ((cb * -22479 + cr * -46596 + 8874368) >> 16);
                    s32 b = y0 + ((cb * 115671 - 14773120) >> 16);
                    */

                    dest[x] = Color(r, g, b, 255);
                }
            }

            dest.blit(0, 0, bitmap);

            /*
            if (image->color_space == OPJ_CLRSPC_SYCC)
            {
                //color_sycc_to_rgb(image);
            }

            if (image->color_space != OPJ_CLRSPC_SYCC &&
                image->numcomps == 3 &&
                image->comps[0].dx == image->comps[0].dy &&
                image->comps[1].dx != 1)
            {
                image->color_space = OPJ_CLRSPC_SYCC;
            }
            else if (image->numcomps <= 2)
            {
                image->color_space = OPJ_CLRSPC_GRAY;
            }
            */
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

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_HEIF) && defined(MANGO_LICENSE_ENABLE_GPL)

#include <libheif/heif.h>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ImageHeader m_header;

        heif_context* m_context = nullptr;
        heif_image_handle* m_image_handle = nullptr;

        Interface(ConstMemory memory)
        {
            heif_error error;

            error = heif_init(nullptr);
            if (error.code != heif_error_Ok)
            {
                m_header.setError("[ImageDecoder.HEIF] heif_init FAILED (%s).", error.message);
                return;
            }

            if (memory.size < 12)
            {
                m_header.setError("[ImageDecoder.HEIF] Not enough data (%d bytes).", int(memory.size));
                return;
            }

            /*
            heif_filetype_result filetype = heif_check_filetype(memory.address, memory.size);
            if (filetype != heif_filetype_yes_supported)
            {
                // ...
                debugPrint("not heif.\n");
                return;
            }
            */

            const char* mime = heif_get_file_mime_type(memory.address, memory.size);
            debugPrint("MIME: %s\n", mime);

            m_context = heif_context_alloc();
            if (!m_context)
            {
                m_header.setError("[ImageDecoder.HEIF]heif_context_alloc FAILED.");
                return;
            }

            error = heif_context_read_from_memory_without_copy(m_context, memory.address, memory.size, nullptr);
            if (error.code != heif_error_Ok)
            {
                m_header.setError("[ImageDecoder.HEIF] heif_context_read_from_memory_without_copy FAILED (%s).", error.message);
                return;
            }

            error = heif_context_get_primary_image_handle(m_context, &m_image_handle);
            if (error.code != heif_error_Ok)
            {
                m_header.setError("[ImageDecoder.HEIF] heif_context_get_primary_image_handle FAILED (%s).", error.message);
                return;
            }

            int width = heif_image_handle_get_width(m_image_handle);
            int height = heif_image_handle_get_height(m_image_handle);
            int bpp = heif_image_handle_get_luma_bits_per_pixel(m_image_handle);
            int cbpp = heif_image_handle_get_chroma_bits_per_pixel(m_image_handle);
            int alpha = heif_image_handle_has_alpha_channel(m_image_handle);
            int luma = heif_image_handle_get_luma_bits_per_pixel(m_image_handle);

            debugPrint("image: %d x %d, bits: %d, chroma: %d, alpha: %d, luma: %d\n", 
                width, height, bpp, cbpp, alpha, luma);

            Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

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
            if (m_image_handle)
            {
                heif_image_handle_release(m_image_handle);
            }

            if (m_context)
            {
                heif_context_free(m_context);
            }

            heif_deinit();
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ConstMemory icc() override
        {
            return ConstMemory();
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

            ImageDecodeStatus status;

            heif_decoding_options* decode_options = heif_decoding_options_alloc();

            decode_options->convert_hdr_to_8bit = true;
            decode_options->ignore_transformations = true;

            heif_image* image = nullptr;
            heif_error error = heif_decode_image(m_image_handle, &image, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, nullptr);

            heif_decoding_options_free(decode_options);

            if (error.code != heif_error_Ok)
            {
                status.setError("[ImageDecoder.HEIF] heif_decode_image FAILED (%s).", error.message);
                return status;
            }

            // MANGO TODO: hdr image, > 8 bits per channel

            /*
            heif_channel_Y = 0,
            heif_channel_Cb = 1,
            heif_channel_Cr = 2,

            heif_channel_R = 3,
            heif_channel_G = 4,
            heif_channel_B = 5,
            heif_channel_Alpha = 6,

            heif_channel_interleaved = 10
            */
            heif_channel ch = heif_channel_interleaved;
            int s0 = heif_image_get_bits_per_pixel(image, ch);
            int s1 = heif_image_get_bits_per_pixel_range(image, ch);
            debugPrint("s0: %d, s1: %d\n", s0, s1);
            if (s0 != 32)
            {
                heif_image_release(image);
                status.setError("[ImageDecoder.HEIF] Unsupported format.");
                return status;
            }

            int stride = 0;
            const u8* p = heif_image_get_plane_readonly(image, heif_channel_interleaved, &stride);
            if (!p)
            {
                heif_image_release(image);
                status.setError("[ImageDecoder.HEIF] heif_image_get_plane_readonly FAILED.");
                return status;
            }

            Surface temp(m_header.width, m_header.height, m_header.format, stride, p);
            dest.blit(0, 0, temp);

            heif_image_release(image);

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

    static
    heif_error heif_stream_write(heif_context* context, const void* data, size_t size, void* userdata)
    {
        MANGO_UNREFERENCED(context);
        Stream* stream = reinterpret_cast<Stream*>(userdata);
        stream->write(data, size);
        debugPrint("heif.write: %d KB\n", int(size / 1024));

        heif_error error;
        error.code = heif_error_Ok;
        error.message = "";
        return error;
    }

    ImageEncodeStatus imageEncode(Stream& output, const Surface& surface, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status;

        int width = surface.width;
        int height = surface.height;

        heif_error error;

        error = heif_init(nullptr);
        if (error.code != heif_error_Ok)
        {
            status.setError("[ImageEncoder.HEIF] heif_init FAILED (%s).", error.message);
            return status;
        }

        // create heif_image

        heif_image* image = nullptr;
        error = heif_image_create(width, height, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, &image);

        heif_image_add_plane(image, heif_channel_interleaved, width, height, 32);

        int stride;
        u8* p = heif_image_get_plane(image, heif_channel_interleaved, &stride);

        Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        Surface temp(width, height, format, stride, p);
        temp.blit(0, 0, surface);

        // ..........................

        heif_context* context = heif_context_alloc();
        if (!context)
        {
            heif_image_release(image);
            heif_deinit();
            status.setError("[ImageEncoder.HEIF]heif_context_alloc FAILED.");
            return status;
        }

        heif_encoder* encoder;
        error = heif_context_get_encoder_for_format(context, heif_compression_HEVC, &encoder);
        if (error.code != heif_error_Ok)
        {
            heif_context_free(context);
            heif_image_release(image);
            heif_deinit();
            status.setError("[ImageEncoder.HEIF] heif_context_get_encoder_for_format FAILED (%s).", error.message);
            return status;
        }

        if (options.lossless)
        {
            error = heif_encoder_set_lossless(encoder, 1);
        }
        else
        {
            int quality = u32_clamp(int(options.quality * 100), 0, 100);
            error = heif_encoder_set_lossy_quality(encoder, quality);
        }

        heif_image_handle* image_handle = nullptr;
        error = heif_context_encode_image(context, image, encoder, nullptr, &image_handle);
        if (error.code != heif_error_Ok)
        {
            heif_encoder_release(encoder);
            heif_context_free(context);
            heif_image_release(image);
            heif_deinit();
            status.setError("[ImageEncoder.HEIF] heif_context_encode_image FAILED (%s).", error.message);
            return status;
        }

        heif_writer writer;
        writer.writer_api_version = 1;
        writer.write = heif_stream_write;

        error = heif_context_write(context, &writer, &output);
        if (error.code != heif_error_Ok)
        {
            heif_image_handle_release(image_handle);
            heif_encoder_release(encoder);
            heif_context_free(context);
            heif_image_release(image);
            heif_deinit();
            status.setError("[ImageEncoder.HEIF] heif_context_write FAILED (%s).", error.message);
            return status;
        }

        heif_image_handle_release(image_handle);
        heif_encoder_release(encoder);
        heif_context_free(context);
        heif_image_release(image);
        heif_deinit();

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecHEIF()
    {
        registerImageDecoder(createInterface, ".heif");
        registerImageDecoder(createInterface, ".heic");
        registerImageEncoder(imageEncode, ".heif");
        registerImageEncoder(imageEncode, ".heic");
    }

} // namespace mango::image

#endif // defined(MANGO_ENABLE_HEIF)

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#include <libheif/heif.h>

namespace
{
    using namespace mango;
    using namespace mango::image;

    static
    bool isHdrTransfer(TransferFunction transfer)
    {
        return transfer == TransferFunction::PQ || transfer == TransferFunction::HLG;
    }

    static
    heif_chroma selectHeifChroma(bool high_precision, bool has_alpha)
    {
        if (high_precision)
        {
#ifdef MANGO_BIG_ENDIAN
            return has_alpha ? heif_chroma_interleaved_RRGGBBAA_BE : heif_chroma_interleaved_RRGGBB_BE;
#else
            return has_alpha ? heif_chroma_interleaved_RRGGBBAA_LE : heif_chroma_interleaved_RRGGBB_LE;
#endif
        }

        return heif_chroma_interleaved_RGBA;
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        heif_context* m_context = nullptr;
        heif_image_handle* m_image_handle = nullptr;
        Buffer m_icc;
        bool m_heif_initialized = false;
        int m_luma_bpp = 0;
        int m_chroma_bpp = 0;
        bool m_has_alpha = false;
        bool m_high_precision = false;

        Interface(ConstMemory memory)
        {
            heif_error error;

            error = heif_init(nullptr);
            if (error.code != heif_error_Ok)
            {
                header.setError("[ImageDecoder.HEIF] heif_init FAILED ({}).", error.message);
                return;
            }

            m_heif_initialized = true;

            if (memory.size < 12)
            {
                header.setError("[ImageDecoder.HEIF] Not enough data ({} bytes).", memory.size);
                return;
            }

            /*
            heif_filetype_result filetype = heif_check_filetype(memory.address, memory.size);
            if (filetype != heif_filetype_yes_supported)
            {
                // ...
                printLine(Print::Error, "not heif.");
                return;
            }
            */

            m_context = heif_context_alloc();
            if (!m_context)
            {
                header.setError("[ImageDecoder.HEIF] heif_context_alloc FAILED.");
                return;
            }

            error = heif_context_read_from_memory_without_copy(m_context, memory.address, memory.size, nullptr);
            if (error.code != heif_error_Ok)
            {
                header.setError("[ImageDecoder.HEIF] heif_context_read_from_memory_without_copy FAILED ({}).", error.message);
                return;
            }

            error = heif_context_get_primary_image_handle(m_context, &m_image_handle);
            if (error.code != heif_error_Ok)
            {
                header.setError("[ImageDecoder.HEIF] heif_context_get_primary_image_handle FAILED ({}).", error.message);
                return;
            }

            int width = heif_image_handle_get_width(m_image_handle);
            int height = heif_image_handle_get_height(m_image_handle);
            int bpp = heif_image_handle_get_luma_bits_per_pixel(m_image_handle);
            int cbpp = heif_image_handle_get_chroma_bits_per_pixel(m_image_handle);
            int alpha = heif_image_handle_has_alpha_channel(m_image_handle);

            m_luma_bpp = bpp;
            m_chroma_bpp = cbpp;
            m_has_alpha = alpha != 0;

            int luma = heif_image_handle_get_luma_bits_per_pixel(m_image_handle);

            printLine(Print::Debug, "image: {} x {}, bits: {}, chroma: {}, alpha: {}, luma: {}", 
                width, height, bpp, cbpp, alpha, luma);

            header.width   = width;
            header.height  = height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
            header.compression = TextureCompression::NONE;

            readColorProfile();

            m_high_precision = (m_luma_bpp > 8) || isHdrTransfer(header.color.transfer);
            header.format = m_high_precision
                ? Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32)
                : Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }

        void readColorProfile()
        {
            ColorInfo& color = header.color;

            // Forward an embedded ICC profile when present; it defines the color space and
            // takes precedence over the NCLX signalling.
            heif_color_profile_type type = heif_image_handle_get_color_profile_type(m_image_handle);
            if (type == heif_color_profile_type_rICC || type == heif_color_profile_type_prof)
            {
                size_t size = heif_image_handle_get_raw_color_profile_size(m_image_handle);
                if (size)
                {
                    m_icc.resize(size);
                    heif_error error = heif_image_handle_get_raw_color_profile(m_image_handle, m_icc.data());
                    if (error.code == heif_error_Ok)
                    {
                        icc = m_icc;
                        color.primaries = ColorPrimaries::Unspecified;
                        color.transfer = TransferFunction::Unspecified;
                        return;
                    }

                    m_icc.reset();
                }
            }

            // Otherwise map the NCLX (CICP) signalling. libheif also decodes the exact
            // chromaticities, which we forward as well.
            heif_color_profile_nclx* nclx = nullptr;
            heif_error error = heif_image_handle_get_nclx_color_profile(m_image_handle, &nclx);
            if (error.code == heif_error_Ok && nclx)
            {
                ColorPrimaries primaries = colorPrimariesFromCICP(u8(nclx->color_primaries));
                TransferFunction transfer = transferFunctionFromCICP(u8(nclx->transfer_characteristics));

                if (primaries != ColorPrimaries::Unspecified)
                {
                    color.primaries = primaries;
                    fillChromaticitiesFromPrimaries(color, primaries);
                }
                if (transfer != TransferFunction::Unspecified)
                {
                    color.transfer = transfer;
                }

                color.has_chromaticities = true;
                color.white = { nclx->color_primary_white_x, nclx->color_primary_white_y };
                color.red   = { nclx->color_primary_red_x,   nclx->color_primary_red_y   };
                color.green = { nclx->color_primary_green_x, nclx->color_primary_green_y };
                color.blue  = { nclx->color_primary_blue_x,  nclx->color_primary_blue_y  };

                heif_nclx_color_profile_free(nclx);
            }

            header.linear = color.isLinear();
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

            if (m_heif_initialized)
            {
                heif_deinit();
            }
        }

        void populateInspect(ImageInspect& report) const override
        {
            if (!m_image_handle)
                return;

            report.bit_depth = m_luma_bpp;
            report.alpha = m_has_alpha;
            report.progressive = InspectTriState::No;
            report.tiling.tiled = InspectTriState::Unknown;
            report.lossless = InspectTriState::Unknown;
            report.encoding = "HEVC";

            if (m_chroma_bpp > 0 && m_luma_bpp > 0)
            {
                if (m_chroma_bpp == m_luma_bpp)
                    report.chroma_subsampling = "4:4:4";
                else if (m_chroma_bpp + 1 == m_luma_bpp)
                    report.chroma_subsampling = "4:2:2";
                else
                    report.chroma_subsampling = "4:2:0";
            }

            syncHdrInspectFromHeader(report);
        }

        void copyHeifPlane(const Surface& dest, heif_image* image) const
        {
            const int width = header.width;
            const int height = header.height;

            int stride = 0;
            const u8* plane = heif_image_get_plane_readonly(image, heif_channel_interleaved, &stride);
            if (!plane)
            {
                return;
            }

            if (!m_high_precision)
            {
                Surface temp(width, height, header.format, stride, plane);
                dest.blit(0, 0, temp);
                return;
            }

            const int bits_range = heif_image_get_bits_per_pixel_range(image, heif_channel_interleaved);
            const int storage_bits = heif_image_get_bits_per_pixel(image, heif_channel_interleaved);
            const int channels = storage_bits / bits_range;

            float scale = 1.0f / 65535.0f;
            if (bits_range > 0 && bits_range <= 16)
            {
                scale = 1.0f / float((1u << bits_range) - 1u);
            }

            for (int y = 0; y < height; ++y)
            {
                const u16* src = reinterpret_cast<const u16*>(plane + y * stride);
                float* dst = dest.address<float>(0, y);

                for (int x = 0; x < width; ++x)
                {
                    dst[0] = float(src[0]) * scale;
                    dst[1] = float(src[1]) * scale;
                    dst[2] = float(src[2]) * scale;

                    if (channels >= 4 && m_has_alpha)
                    {
                        dst[3] = float(src[3]) * scale;
                    }
                    else
                    {
                        dst[3] = 1.0f;
                    }

                    src += channels;
                    dst += 4;
                }
            }
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            heif_decoding_options* decode_options = heif_decoding_options_alloc();

            decode_options->convert_hdr_to_8bit = m_high_precision ? 0 : 1;
            decode_options->ignore_transformations = true;

            const heif_chroma chroma = selectHeifChroma(m_high_precision, m_has_alpha);

            heif_image* image = nullptr;
            heif_error error = heif_decode_image(m_image_handle, &image, heif_colorspace_RGB, chroma, decode_options);

            heif_decoding_options_free(decode_options);

            if (error.code != heif_error_Ok)
            {
                status.setError("[ImageDecoder.HEIF] heif_decode_image FAILED ({}).", error.message);
                return status;
            }

            const int storage_bits = heif_image_get_bits_per_pixel(image, heif_channel_interleaved);
            const int bits_range = heif_image_get_bits_per_pixel_range(image, heif_channel_interleaved);
            printLine(Print::Debug, "storage_bits: {}, bits_range: {}", storage_bits, bits_range);

            if (m_high_precision)
            {
                if (bits_range <= 8 || storage_bits < 48)
                {
                    heif_image_release(image);
                    status.setError("[ImageDecoder.HEIF] Expected >8-bit storage, got {} (range {}).",
                        storage_bits, bits_range);
                    return status;
                }
            }
            else if (storage_bits != 32)
            {
                heif_image_release(image);
                status.setError("[ImageDecoder.HEIF] Expected 8-bit RGBA, got {} storage bits.", storage_bits);
                return status;
            }

            copyHeifPlane(dest, image);

            heif_image_release(image);

            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
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
        printLine(Print::Debug, "heif.write: {} KB", size / 1024);

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
            status.setError("[ImageEncoder.HEIF] heif_init FAILED ({}).", error.message);
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
            status.setError("[ImageEncoder.HEIF] heif_context_get_encoder_for_format FAILED ({}).", error.message);
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
            status.setError("[ImageEncoder.HEIF] heif_context_encode_image FAILED ({}).", error.message);
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
            status.setError("[ImageEncoder.HEIF] heif_context_write FAILED ({}).", error.message);
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

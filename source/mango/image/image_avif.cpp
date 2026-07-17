/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#include <avif/avif.h>

namespace
{
    using namespace mango;
    using namespace mango::image;

    float unsignedFraction(const avifUnsignedFraction& f) noexcept
    {
        return f.d ? float(f.n) / float(f.d) : 0.0f;
    }

    void fillChromaticities(ColorInfo& color, avifColorPrimaries cicp) noexcept
    {
        // outPrimaries: rX, rY, gX, gY, bX, bY, wX, wY
        float xy[8] = {};
        avifColorPrimariesGetValues(cicp, xy);

        color.has_chromaticities = true;
        color.red   = { xy[0], xy[1] };
        color.green = { xy[2], xy[3] };
        color.blue  = { xy[4], xy[5] };
        color.white = { xy[6], xy[7] };
    }

    void fillContentLightLevel(ColorInfo& color, const avifContentLightLevelInformationBox& clli) noexcept
    {
        // (0, 0) means unknown / not present (libavif convention).
        if (!clli.maxCLL && !clli.maxPALL)
            return;

        color.has_content_light_level = true;
        color.content_light_level.max_cll = float(clli.maxCLL);
        color.content_light_level.max_fall = float(clli.maxPALL);
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        avifDecoder* m_decoder = nullptr;
        avifRGBImage m_rgb {};
        bool m_has_gain_map = false;
        float m_hdr_headroom = 0.0f;
        avifColorPrimaries m_output_primaries = AVIF_COLOR_PRIMARIES_BT709;

        Interface(ConstMemory memory)
        {
            m_decoder = avifDecoderCreate();
            if (!m_decoder)
            {
                header.setError("[ImageDecoder.AVIF] avifDecoderCreate FAILED.");
                return;
            }

            m_decoder->maxThreads = int(std::thread::hardware_concurrency());
            m_decoder->strictFlags = AVIF_STRICT_DISABLED;
            // Decode gain-map pixels when present (ISO 21496-1 / UltraHDR-style AVIF).
            m_decoder->imageContentToDecode = AVIF_IMAGE_CONTENT_ALL;

            avifResult result = avifDecoderSetIOMemory(m_decoder, memory.address, memory.size);
            if (result != AVIF_RESULT_OK)
            {
                header.setError("[ImageDecoder.AVIF] avifDecoderSetIOMemory FAILED.");
                return;
            }

            result = avifDecoderParse(m_decoder);
            if (result != AVIF_RESULT_OK)
            {
                header.setError("[ImageDecoder.AVIF] avifDecoderParse FAILED.");
                return;
            }

            avifImage* image = m_decoder->image;
            if (!image)
            {
                header.setError("[ImageDecoder.AVIF] No image after parse.");
                return;
            }

            // avifDecoderParse enforces libavif's dimension/size limits, but a malformed
            // file can still parse to a degenerate 0-sized image; reject it here so we never
            // allocate or blit a zero/garbage surface.
            if (!image->width || !image->height)
            {
                header.setError("[ImageDecoder.AVIF] Invalid image dimensions ({} x {}).",
                    image->width, image->height);
                return;
            }

            icc = ConstMemory(image->icc.data, image->icc.size);
            exif = ConstMemory(image->exif.data, image->exif.size);
            //xmp = ConstMemory(image->xmp.data, image->xmp.size);

            header.width   = image->width;
            header.height  = image->height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
            header.compression = TextureCompression::NONE;

            // Prefer reconstructing the HDR alternate when a gain map is present
            // (same client-facing contract as UltraHDR JPEG: scene-linear FLOAT16).
            if (image->gainMap)
            {
                configureGainMap(image);
            }
            else
            {
                Format format = image->depth > 8 ?
                    Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16) :
                    Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                header.format = format;
                readColorInfo(image);
            }
        }

        void readColorInfo(const avifImage* image)
        {
            // AVIF signals color with CICP code points (avifColorPrimaries /
            // avifTransferCharacteristics share the ITU-T H.273 values). An attached ICC
            // profile, when present, takes precedence for the named space and is forwarded
            // separately — matching HEIF / HDR PNG. CLLI is independent of that precedence.
            ColorInfo& color = header.color;

            if (image->icc.size)
            {
                color.primaries = ColorPrimaries::Unspecified;
                color.transfer = TransferFunction::Unspecified;
            }
            else
            {
                ColorPrimaries primaries = colorPrimariesFromCICP(u8(image->colorPrimaries));
                TransferFunction transfer = transferFunctionFromCICP(u8(image->transferCharacteristics));

                // Keep the sRGB default when the file signals "unspecified".
                if (primaries != ColorPrimaries::Unspecified)
                {
                    color.primaries = primaries;
                    fillChromaticities(color, image->colorPrimaries);
                }
                if (transfer != TransferFunction::Unspecified)
                {
                    color.transfer = transfer;
                }
            }

            fillContentLightLevel(color, image->clli);
            header.linear = color.isLinear();
        }

        void configureGainMap(const avifImage* image)
        {
            const avifGainMap* gm = image->gainMap;

            // Reconstruct the full alternate (HDR) rendition, as UltraHDR JPEG does.
            m_hdr_headroom = unsignedFraction(gm->alternateHdrHeadroom);
            m_has_gain_map = true;

            // Tone-map into the color space the gain map was authored for.
            m_output_primaries = gm->useBaseColorSpace
                ? image->colorPrimaries
                : gm->altColorPrimaries;

            ColorPrimaries primaries = colorPrimariesFromCICP(u8(m_output_primaries));
            if (primaries == ColorPrimaries::Unspecified)
                primaries = ColorPrimaries::BT709;

            header.format = Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16);
            header.color = ColorInfo { primaries, TransferFunction::Linear };
            fillChromaticities(header.color, m_output_primaries);

            // Prefer the alternate image's CLLI; fall back to the base when absent.
            fillContentLightLevel(header.color, gm->altCLLI);
            if (!header.color.has_content_light_level)
                fillContentLightLevel(header.color, image->clli);

            header.linear = true;
        }

        ~Interface()
        {
            if (m_decoder)
            {
                avifDecoderDestroy(m_decoder);
            }

            if (m_rgb.pixels)
            {
                avifRGBImageFreePixels(&m_rgb);
            }
        }

        ImageDecodeStatus decodeGainMap(const Surface& dest)
        {
            ImageDecodeStatus status;

            avifImage* image = m_decoder->image;
            if (!image || !image->gainMap)
            {
                status.setError("[ImageDecoder.AVIF] Missing gain map.");
                return status;
            }

            avifResult result = avifDecoderNextImage(m_decoder);
            if (result != AVIF_RESULT_OK)
            {
                status.setError("[ImageDecoder.AVIF] avifDecoderNextImage FAILED.");
                return status;
            }

            image = m_decoder->image;
            if (!image || !image->gainMap)
            {
                status.setError("[ImageDecoder.AVIF] No image after decode.");
                return status;
            }

            // Scene-linear half-float RGBA — same delivery as UltraHDR JPEG.
            avifRGBImageSetDefaults(&m_rgb, image);
            m_rgb.depth = 16;
            m_rgb.isFloat = AVIF_TRUE;
            m_rgb.format = AVIF_RGB_FORMAT_RGBA;
            m_rgb.maxThreads = m_decoder->maxThreads;

            result = avifRGBImageAllocatePixels(&m_rgb);
            if (result != AVIF_RESULT_OK)
            {
                status.setError("[ImageDecoder.AVIF] avifRGBImageAllocatePixels FAILED.");
                return status;
            }

            avifContentLightLevelInformationBox clli {};
            result = avifImageApplyGainMap(
                image,
                image->gainMap,
                m_hdr_headroom,
                m_output_primaries,
                AVIF_TRANSFER_CHARACTERISTICS_LINEAR,
                &m_rgb,
                &clli,
                &m_decoder->diag);

            if (result != AVIF_RESULT_OK)
            {
                avifRGBImageFreePixels(&m_rgb);
                status.setError("[ImageDecoder.AVIF] avifImageApplyGainMap FAILED.");
                return status;
            }

            // Refresh CLLI from the tone-mapped result when libavif reports it.
            fillContentLightLevel(header.color, clli);

            const size_t expected_row = size_t(image->width) * 4 * sizeof(u16);
            if (!m_rgb.pixels ||
                m_rgb.width < image->width ||
                m_rgb.height < image->height ||
                m_rgb.rowBytes < expected_row)
            {
                avifRGBImageFreePixels(&m_rgb);
                status.setError("[ImageDecoder.AVIF] Unexpected gain-map RGB buffer geometry.");
                return status;
            }

            Surface temp(image->width, image->height, header.format, m_rgb.rowBytes, m_rgb.pixels);
            dest.blit(0, 0, temp);
            return status;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!m_decoder)
            {
                status.setError("[ImageDecoder.AVIF] avifDecoderCreate FAILED.");
                return status;
            }

            if (m_has_gain_map)
            {
                if (m_rgb.pixels)
                {
                    Surface temp(header.width, header.height, header.format, m_rgb.rowBytes, m_rgb.pixels);
                    dest.blit(0, 0, temp);
                    return status;
                }
                return decodeGainMap(dest);
            }

            avifImage* image = m_decoder->image;
            if (!image)
            {
                status.setError("[ImageDecoder.AVIF] No image.");
                return status;
            }

            if (!m_rgb.pixels)
            {
                avifResult result = avifDecoderNextImage(m_decoder);
                if (result != AVIF_RESULT_OK)
                {
                    status.setError("[ImageDecoder.AVIF] avifDecoderNextImage FAILED.");
                    return status;
                }

                // Re-fetch: avifDecoderNextImage may update the active image pointer.
                image = m_decoder->image;
                if (!image)
                {
                    status.setError("[ImageDecoder.AVIF] No image after decode.");
                    return status;
                }

                avifRGBImageSetDefaults(&m_rgb, image);

                result = avifRGBImageAllocatePixels(&m_rgb);
                if (result != AVIF_RESULT_OK)
                {
                    status.setError("[ImageDecoder.AVIF] avifRGBImageAllocatePixels FAILED.");
                    return status;
                }

                if (avifImageYUVToRGB(image, &m_rgb) != AVIF_RESULT_OK)
                {
                    avifRGBImageFreePixels(&m_rgb);
                    status.setError("[ImageDecoder.AVIF] avifImageYUVToRGB FAILED.");
                    return status;
                }
            }

            printLine(Print::Info, "image: {} x {}, depth: {}, stride: {}",
                image->width, image->height, image->depth, m_rgb.rowBytes);

            const int precision = image->depth;

            // The pixel copy below reads packed RGBA from m_rgb. Verify the buffer libavif
            // produced actually matches our assumptions (4 channels, sufficient stride and
            // height) so a surprising format/size cannot turn into an out-of-bounds read.
            const size_t expected_row = size_t(image->width) * 4 * (precision > 8 ? 2 : 1);
            if (!m_rgb.pixels ||
                m_rgb.width < image->width ||
                m_rgb.height < image->height ||
                m_rgb.rowBytes < expected_row)
            {
                status.setError("[ImageDecoder.AVIF] Unexpected RGB buffer geometry.");
                return status;
            }

            if (precision > 8)
            {
                Bitmap temp(image->width, image->height, header.format);

                for (u32 y = 0; y < image->height; ++y)
                {
                    u16* d = temp.address<u16>(0, y);
                    const u16* s = reinterpret_cast<const u16*>(m_rgb.pixels + y * m_rgb.rowBytes);

                    for (u32 x = 0; x < image->width; ++x)
                    {
                        d[0] = u16_extend(s[0], precision, 16);
                        d[1] = u16_extend(s[1], precision, 16);
                        d[2] = u16_extend(s[2], precision, 16);
                        d[3] = u16_extend(s[3], precision, 16);
                        d += 4;
                        s += 4;
                    }
                }

                dest.blit(0, 0, temp);
            }
            else
            {
                Surface temp(image->width, image->height, header.format, m_rgb.rowBytes, m_rgb.pixels);
                dest.blit(0, 0, temp);
            }

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

    ImageEncodeStatus imageEncode(Stream& output, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

        int width = surface.width;
        int height = surface.height;

        avifImage* image = avifImageCreate(width, height, 8, AVIF_PIXEL_FORMAT_YUV444);
        if (!image)
        {
            status.setError("[ImageEncoder.AVIF] avifImageCreate FAILED.");
            return status;
        }

        avifRGBImage rgb;
        std::memset(&rgb, 0, sizeof(rgb));

        avifRGBImageSetDefaults(&rgb, image);
        avifResult result = avifRGBImageAllocatePixels(&rgb);
        if (result != AVIF_RESULT_OK)
        {
            avifImageDestroy(image);
            status.setError("[ImageEncoder.AVIF] avifRGBImageAllocatePixels FAILED.");
            return status;
        }

        Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        Surface temp(width, height, format, rgb.rowBytes, rgb.pixels);
        temp.blit(0, 0, surface);

        avifResult convertResult = avifImageRGBToYUV(image, &rgb);
        if (convertResult != AVIF_RESULT_OK)
        {
            avifRGBImageFreePixels(&rgb);
            avifImageDestroy(image);
            status.setError("[ImageEncoder.AVIF] avifImageRGBToYUV FAILED.");
            return status;
        }

        avifEncoder* encoder = avifEncoderCreate();
        if (!encoder)
        {
            avifRGBImageFreePixels(&rgb);
            avifImageDestroy(image);
            status.setError("[ImageEncoder.AVIF] avifEncoderCreate FAILED.");
            return status;
        }

        encoder->maxThreads = int(std::thread::hardware_concurrency());

        avifResult addImageResult = avifEncoderAddImage(encoder, image, 1, AVIF_ADD_IMAGE_FLAG_SINGLE);
        if (addImageResult != AVIF_RESULT_OK)
        {
            avifRGBImageFreePixels(&rgb);
            avifEncoderDestroy(encoder);
            avifImageDestroy(image);
            status.setError("[ImageEncoder.AVIF] avifEncoderAddImage FAILED.");
            return status;
        }

        avifRWData avifOutput = AVIF_DATA_EMPTY;
        avifResult finishResult = avifEncoderFinish(encoder, &avifOutput);
        if (finishResult != AVIF_RESULT_OK)
        {
            avifRWDataFree(&avifOutput);
            avifRGBImageFreePixels(&rgb);
            avifEncoderDestroy(encoder);
            avifImageDestroy(image);
            status.setError("[ImageEncoder.AVIF] avifEncoderFinish FAILED.");
            return status;
        }

        printLine(Print::Info, "AVIF encoded: {} bytes", avifOutput.size);

        output.write(avifOutput.data, avifOutput.size);

        avifRWDataFree(&avifOutput);
        avifRGBImageFreePixels(&rgb);
        avifEncoderDestroy(encoder);
        avifImageDestroy(image);

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecAVIF()
    {
        registerImageDecoder(createInterface, ".avif");
        registerImageEncoder(imageEncode, ".avif");
    }

} // namespace mango::image

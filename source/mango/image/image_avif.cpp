/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_AVIF)

#include <avif/avif.h>

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
        ConstMemory m_icc;
        ConstMemory m_exif;

        avifDecoder* m_decoder = nullptr;
        avifRGBImage m_rgb;

        Interface(ConstMemory memory)
        {
            m_decoder = avifDecoderCreate();
            if (!m_decoder)
            {
                m_header.setError("[ImageDecoder.AVIF] avifDecoderCreate FAILED.");
                return;
            }

            m_decoder->maxThreads = int(std::thread::hardware_concurrency());

            avifResult result = avifDecoderSetIOMemory(m_decoder, memory.address, memory.size);
            if (result != AVIF_RESULT_OK)
            {
                m_header.setError("[ImageDecoder.AVIF] avifDecoderSetIOMemory FAILED.");
                return;
            }

            result = avifDecoderParse(m_decoder);
            if (result != AVIF_RESULT_OK)
            {
                m_header.setError("[ImageDecoder.AVIF] avifDecoderParse FAILED.");
                return;
            }

            std::memset(&m_rgb, 0, sizeof(m_rgb));

            avifImage* image = m_decoder->image;

            m_icc = ConstMemory(image->icc.data, image->icc.size);
            m_exif = ConstMemory(image->exif.data, image->exif.size);
            //m_xmp = ConstMemory(image->xmp.data, image->xmp.size);

            m_header.width   = image->width;
            m_header.height  = image->height;
            m_header.depth   = 0;
            m_header.levels  = 0;
            m_header.faces   = 0;
            m_header.palette = false;
            m_header.format  = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            m_header.compression = TextureCompression::NONE;
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
            return m_exif;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!m_decoder)
            {
                status.setError("[ImageDecoder.AVIF] avifDecoderCreate FAILED.");
                return status;
            }

            avifImage* image = m_decoder->image;

            if (!m_rgb.pixels)
            {
                avifResult result = avifDecoderNextImage(m_decoder);
                if (result != AVIF_RESULT_OK)
                {
                    status.setError("[ImageDecoder.AVIF] avifDecoderNextImage FAILED.");
                    return status;
                }

                avifRGBImageSetDefaults(&m_rgb, image);
                avifRGBImageAllocatePixels(&m_rgb);

                if (avifImageYUVToRGB(image, &m_rgb) != AVIF_RESULT_OK)
                {
                    avifRGBImageFreePixels(&m_rgb);
                    status.setError("[ImageDecoder.AVIF] avifImageYUVToRGB FAILED.");
                    return status;
                }
            }

            Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            Surface temp(image->width, image->height, format, m_rgb.rowBytes, m_rgb.pixels);
            dest.blit(0, 0, temp);

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

    ImageEncodeStatus imageEncode(Stream& output, const Surface& surface, const ImageEncodeOptions& options)
    {
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
        avifRGBImageAllocatePixels(&rgb);

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

        // TODO: debugPrint
        printf("Encode success: %zu total bytes\n", avifOutput.size);

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

#endif // defined(MANGO_ENABLE_AVIF)

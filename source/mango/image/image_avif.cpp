/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_AVIF) && defined(MANGO_LICENSE_ENABLE_APACHE)

#include <avif/avif.h>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        avifDecoder* m_decoder = nullptr;
        avifRGBImage m_rgb;

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

            std::memset(&m_rgb, 0, sizeof(m_rgb));

            avifImage* image = m_decoder->image;

            icc = ConstMemory(image->icc.data, image->icc.size);
            exif = ConstMemory(image->exif.data, image->exif.size);
            //xmp = ConstMemory(image->xmp.data, image->xmp.size);

            Format format = image->depth > 8 ?
                Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16) :
                Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

            header.width   = image->width;
            header.height  = image->height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
            header.palette = false;
            header.format  = format;
            header.compression = TextureCompression::NONE;
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

#endif // defined(MANGO_ENABLE_AVIF)

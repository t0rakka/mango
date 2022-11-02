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

        Interface(ConstMemory memory)
        {
            // TODO
            int width = 100;
            int height = 100;
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

            // TODO
            ImageDecodeStatus status;
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

        Surface temp(width, height, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), rgb.rowBytes, rgb.pixels);
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
        // TODO
        MANGO_UNREFERENCED(createInterface);
        /*
        registerImageDecoder(createInterface, ".xxx");
        */
        registerImageEncoder(imageEncode, ".avif");
    }

} // namespace mango::image

#endif // defined(MANGO_ENABLE_AVIF)

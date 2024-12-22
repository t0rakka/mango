/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/system.hpp>
#include <mango/math/math.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;

    std::string getLowerCaseExtension(const std::string& filename)
    {
        // strip the filename away and make the extension (.ext) lower-case only
        std::string extension = filesystem::getExtension(filename);
        return toLower(extension.empty() ? std::string(".") + filename : extension);
    }

    std::string getImageFormatExtension(ConstMemory memory)
    {

        // Recognize image format from header signature
        struct Signature
        {
            u8 data[16];
            u32 mask;
            const char* name;
        };

        static
        const Signature signatures [] =
        {
            { { 0xff, 0xd8, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x0007, ".jpg" },
            { { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x00ff, ".png" },
            { { 0x00, 0x00, 0x00, 0x0c, 0x6a, 0x50, 0x20, 0x20, 0x0d, 0x0a, 0x87, 0x0a, 0x00, 0x00, 0x00, 0x00 }, 0x0fff, ".jp2" },
            { { 0x00, 0x00, 0x00, 0x0c, 0x4a, 0x58, 0x4c, 0x20, 0x0d, 0x0a, 0x87, 0x0a, 0x00, 0x00, 0x00, 0x00 }, 0x0fff, ".jxl" },
            { { 0x23, 0x3f, 0x52, 0x41, 0x44, 0x49, 0x41, 0x4e, 0x43, 0x45, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x07ff, ".hdr" },
            { { 0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x003f, ".gif" },
            { { 0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x003f, ".gif" },
            { { 0x71, 0x6f, 0x69, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".qoi" },
            { { 0x76, 0x2f, 0x31, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".exr" },
            { { 0x42, 0x50, 0x47, 0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".bpg" },
            { { 0x50, 0x31, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x0007, ".pbm" },
            { { 0x50, 0x34, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x0007, ".pbm" },
            { { 0x50, 0x32, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x0007, ".pgm" },
            { { 0x50, 0x35, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x0007, ".pgm" },
            { { 0x50, 0x33, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x0007, ".ppm" },
            { { 0x50, 0x36, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x0007, ".ppm" },
            { { 0xab, 0x4b, 0x54, 0x58, 0x20, 0x31, 0x31, 0xbb, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x00 }, 0x0fff, ".ktx" },
            { { 0xab, 0x4b, 0x54, 0x58, 0x20, 0x32, 0x30, 0xbb, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x00 }, 0x0fff, ".ktx2" },
            { { 0x13, 0xab, 0xa1, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".astc" },
            { { 0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x45, 0x42, 0x50, 0x00, 0x00, 0x00, 0x00 }, 0x0f0f, ".webp" },
            { { 0x46, 0x4f, 0x52, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x49, 0x4c, 0x42, 0x4d, 0x00, 0x00, 0x00, 0x00 }, 0x0f0f, ".ilbm" },
            { { 0x44, 0x44, 0x53, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".dds" },
            { { 0x50, 0x4b, 0x4d, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".pkm" },
            { { 0x50, 0x56, 0x52, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".pvr" },
            { { 0x03, 0x52, 0x56, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".pvr" },
            { { 0x42, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x0003, ".bmp" },
            { { 0x53, 0x80, 0xf6, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".pic" },
            { { 0x38, 0x42, 0x50, 0x53, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x001f, ".psd" },
            { { 0x38, 0x42, 0x50, 0x53, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x001f, ".psb" },
            { { 0x49, 0x49, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".tiff" },
            { { 0x4d, 0x4d, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".tiff" },
            { { 0x49, 0x49, 0x2b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".tiff" },
            { { 0x4d, 0x4d, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0x000f, ".tiff" },
        };

        if (memory.size >= 16)
        {
            math::uint8x16 current = math::u8x16_uload(memory.address);

            for (const Signature& signature : signatures)
            {
                math::uint8x16 data = math::u8x16_uload(signature.data);
                u32 mask = math::maskToInt(current == data);

                if (signature.mask == (mask & signature.mask))
                {
                    return signature.name;
                }
            }
        }

        return "";
    }

} // namespace

namespace mango::image
{

    // ----------------------------------------------------------------------------
    // ImageServer
    // ----------------------------------------------------------------------------

    void registerImageCodecTGA();
    void registerImageCodecPKM();
    void registerImageCodecDDS();
    void registerImageCodecPNG();
    void registerImageCodecJPG();
    void registerImageCodecBMP();
    void registerImageCodecPCX();
    void registerImageCodecIFF();
    void registerImageCodecHDR();
    void registerImageCodecGIF();
    void registerImageCodecKTX();
    void registerImageCodecKTX2();
    void registerImageCodecPVR();
    void registerImageCodecASTC();
    void registerImageCodecZPNG();
    void registerImageCodecSGI();
    void registerImageCodecPNM();
    void registerImageCodecATARI();
    void registerImageCodecC64();
    void registerImageCodecEXR();
    void registerImageCodecQOI();
    void registerImageCodecPIC();
    void registerImageCodecPSD();
    void registerImageCodecRAW();

#if defined(MANGO_ENABLE_JXL) && defined(MANGO_LICENSE_ENABLE_BSD)
    void registerImageCodecJXL();
#endif

#if defined(MANGO_ENABLE_JP2) && defined(MANGO_LICENSE_ENABLE_BSD)
    void registerImageCodecJP2();
#endif

#if defined(MANGO_ENABLE_HEIF) && defined(MANGO_LICENSE_ENABLE_GPL)
    void registerImageCodecHEIF();
#endif

#if defined(MANGO_ENABLE_AVIF) && defined(MANGO_LICENSE_ENABLE_APACHE)
    void registerImageCodecAVIF();
#endif

#if defined(MANGO_ENABLE_WEBP) && defined(MANGO_LICENSE_ENABLE_BSD)
    void registerImageCodecWEBP();
#endif

    class ImageServer
    {
    protected:
        std::map<std::string, ImageDecoder::CreateDecodeFunc> m_decoders;
        std::map<std::string, ImageEncoder::EncodeFunc> m_encoders;

    public:
        ImageServer()
        {
            registerImageCodecTGA();
            registerImageCodecPKM();
            registerImageCodecDDS();
            registerImageCodecPNG();
            registerImageCodecJPG();
            registerImageCodecBMP();
            registerImageCodecPCX();
            registerImageCodecIFF();
            registerImageCodecHDR();
            registerImageCodecGIF();
            registerImageCodecKTX();
            registerImageCodecKTX2();
            registerImageCodecPVR();
            registerImageCodecASTC();
            registerImageCodecZPNG();
            registerImageCodecSGI();
            registerImageCodecPNM();
            registerImageCodecATARI();
            registerImageCodecC64();
            registerImageCodecEXR();
            registerImageCodecQOI();
            registerImageCodecPIC();
            registerImageCodecPSD();
            registerImageCodecRAW();

#if defined(MANGO_ENABLE_JXL) && defined(MANGO_LICENSE_ENABLE_BSD)
            registerImageCodecJXL();
#endif

#if defined(MANGO_ENABLE_JP2) && defined(MANGO_LICENSE_ENABLE_BSD)
            registerImageCodecJP2();
#endif

#if defined(MANGO_ENABLE_HEIF) && defined(MANGO_LICENSE_ENABLE_GPL)
            registerImageCodecHEIF();
#endif

#if defined(MANGO_ENABLE_AVIF) && defined(MANGO_LICENSE_ENABLE_APACHE)
            registerImageCodecAVIF();
#endif

#if defined(MANGO_ENABLE_WEBP) && defined(MANGO_LICENSE_ENABLE_BSD)
            registerImageCodecWEBP();
#endif

        }

        ~ImageServer()
        {
        }

        void registerImageDecoder(ImageDecoder::CreateDecodeFunc func, const std::string& extension)
        {
            m_decoders[toLower(extension)] = func;
        }

        void registerImageEncoder(ImageEncoder::EncodeFunc func, const std::string& extension)
        {
            m_encoders[toLower(extension)] = func;
        }

        ImageDecoder::CreateDecodeFunc getImageDecoder(const std::string& extension) const
        {
            auto i = m_decoders.find(extension);
            return i != m_decoders.end() ? i->second : nullptr;
        }

        ImageEncoder::EncodeFunc getImageEncoder(const std::string& extension) const
        {
            auto i = m_encoders.find(extension);
            return i != m_encoders.end() ? i->second : nullptr;
        }
    } g_imageServer;

    void registerImageDecoder(ImageDecoder::CreateDecodeFunc func, const std::string& extension)
    {
        g_imageServer.registerImageDecoder(func, extension);
    }

    void registerImageEncoder(ImageEncoder::EncodeFunc func, const std::string& extension)
    {
        g_imageServer.registerImageEncoder(func, extension);
    }

    bool isImageDecoder(const std::string& filename)
    {
        std::string extension = getLowerCaseExtension(filename);
        auto func = g_imageServer.getImageDecoder(extension);
        return func != nullptr;
    }

    bool isImageEncoder(const std::string& filename)
    {
        std::string extension = getLowerCaseExtension(filename);
        auto func = g_imageServer.getImageEncoder(extension);
        return func != nullptr;
    }

    // ----------------------------------------------------------------------------
    // ImageDecodeInterface
    // ----------------------------------------------------------------------------

    ConstMemory ImageDecodeInterface::memory(int level, int depth, int face)
    {
        MANGO_UNREFERENCED(level);
        MANGO_UNREFERENCED(depth);
        MANGO_UNREFERENCED(face);
        return ConstMemory();
    }

    // ----------------------------------------------------------------------------
    // ImageDecoder
    // ----------------------------------------------------------------------------

    ImageDecoder::ImageDecoder(ConstMemory memory, const std::string& filename)
    {
        // Inspect signature to determine image format
        std::string extension = getImageFormatExtension(memory);
        if (extension.empty())
        {
            // signature wasn't recognized -> trust the filename
            extension = getLowerCaseExtension(filename);
        }

        ImageDecoder::CreateDecodeFunc create = g_imageServer.getImageDecoder(extension);
        if (create)
        {
            ImageDecodeInterface* x = create(memory);
            x->name = fmt::format("ImageDecoder:{}", filesystem::removePath(filename));
            m_interface.reset(x);
        }
    }

    ImageDecoder::~ImageDecoder()
    {
    }

    bool ImageDecoder::isDecoder() const
    {
        return m_interface != nullptr;
    }

    bool ImageDecoder::isAsyncDecoder() const
    {
        return m_interface ? m_interface->async : false;
    }

    ImageHeader ImageDecoder::header()
    {
        ImageHeader header;

        if (m_interface)
        {
            header = m_interface->header;
        }
        else
        {
            header.setError("[WARNING] header() is not supported for this extension.");
        }

        return header;
    }

    ImageDecodeStatus ImageDecoder::decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face)
    {
        ImageDecodeStatus status;

        if (m_interface)
        {
            Trace trace("ImageDecoder", m_interface->name);
            status = m_interface->decode(dest, options, level, depth, face);
        }
        else
        {
            status.setError("[WARNING] decode() is not supported for this extension.");
        }

        return status;
    }

    ImageDecodeFuture ImageDecoder::launch(ImageDecodeCallback callback, const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face)
    {
        if (m_interface)
        {
            if (m_interface.use_count() > 1)
            {
                MANGO_EXCEPTION("[ImageDecoder] async decoding already in progress.");
            }

            m_interface->callback = std::move(callback);
        }

        return std::async(std::launch::async, [=] (std::shared_ptr<ImageDecodeInterface> interface)
        {
            ImageDecodeStatus status;

            if (interface)
            {
                Trace trace("ImageDecoder", interface->name);
                status = interface->decode(dest, options, level, depth, face);

                if (!interface->async)
                {
                    ImageDecodeRect rect;

                    rect.x = 0;
                    rect.y = 0;
                    rect.width = interface->header.width;
                    rect.height = interface->header.height;
                    rect.progress = 1.0f;

                    interface->callback(rect);
                }
            }
            else
            {
                status.setError("[WARNING] decode() is not supported for this extension.");
            }

            return status;
        }, m_interface);
    }

    void ImageDecoder::cancel()
    {
        if (m_interface)
        {
            m_interface->cancelled = true;
        }
    }

    ConstMemory ImageDecoder::memory(int level, int depth, int face)
    {
        ConstMemory memory;

        if (m_interface)
        {
            memory = m_interface->memory(level, depth, face);
        }

        return memory;
    }

    ConstMemory ImageDecoder::icc()
    {
        ConstMemory memory;

        if (m_interface)
        {
            memory = m_interface->icc;
        }

        return memory;
    }

    ConstMemory ImageDecoder::exif()
    {
        ConstMemory memory;

        if (m_interface)
        {
            memory = m_interface->exif;
        }

        return memory;
    }

    // ----------------------------------------------------------------------------
    // ImageEncoder
    // ----------------------------------------------------------------------------

    ImageEncoder::ImageEncoder(const std::string& filename)
    {
        std::string extension = getLowerCaseExtension(filename);
        m_encode_func = g_imageServer.getImageEncoder(extension);
    }

    ImageEncoder::~ImageEncoder()
    {
    }

    bool ImageEncoder::isEncoder() const
    {
        return m_encode_func != nullptr;
    }

    ImageEncodeStatus ImageEncoder::encode(Stream& output, const Surface& source, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status;

        if (m_encode_func)
        {
            status = m_encode_func(output, source, options);
        }
        else
        {
            status.setError("[WARNING] ImageEncoder::encode() is not supported for this extension.");
        }

        return status;
    }

} // namespace mango::image

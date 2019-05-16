/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/string.hpp>
#include <mango/core/timer.hpp>
#include <mango/image/image.hpp>

namespace mango
{

    // ----------------------------------------------------------------------------
    // UnsupportedImageDecoderInterface
    // ----------------------------------------------------------------------------

    struct UnsupportedImageDecoderInterface : ImageDecoderInterface
    {
        UnsupportedImageDecoderInterface(Memory memory)
        {
            MANGO_UNREFERENCED_PARAMETER(memory);
        }

        ~UnsupportedImageDecoderInterface()
        {
        }

        ImageHeader header() override
        {
            printf("[WARNING] ImageDecoder::header() is not supported for this extension.");
            return ImageHeader();
        }

        void decode(Surface& surface, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(surface);
            MANGO_UNREFERENCED_PARAMETER(palette);
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);
            printf("[WARNING] ImageDecoder::decode() is not supported for this extension.");
        }
    };

    ImageDecoderInterface* createUnsupportedImageDecoderInterface(Memory memory)
    {
        ImageDecoderInterface* x = new UnsupportedImageDecoderInterface(memory);
        return x;
    }

    // ----------------------------------------------------------------------------
    // unsupportedImageEncoder
    // ----------------------------------------------------------------------------

    void unsupportedImageEncoder(Stream& output, const Surface& source, const ImageEncoderOptions* options)
    {
        MANGO_UNREFERENCED_PARAMETER(output);
        MANGO_UNREFERENCED_PARAMETER(source);
        MANGO_UNREFERENCED_PARAMETER(options);
        printf("[WARNING] ImageEncoder::encode() is not supported for this extension.");
    }

    // ----------------------------------------------------------------------------
    // ImageServer
    // ----------------------------------------------------------------------------

    void registerImageDecoderTGA();
    void registerImageDecoderPKM();
    void registerImageDecoderDDS();
    void registerImageDecoderPNG();
    void registerImageDecoderJPG();
    void registerImageDecoderBMP();
    void registerImageDecoderPCX();
    void registerImageDecoderIFF();
    void registerImageDecoderHDR();
    void registerImageDecoderGIF();
    void registerImageDecoderKTX();
    void registerImageDecoderPVR();
    void registerImageDecoderASTC();
    void registerImageDecoderZPNG();
    void registerImageDecoderSGI();
    void registerImageDecoderPNM();
    void registerImageDecoderATARI();
    void registerImageDecoderC64();

    class ImageServer
    {
    protected:
        std::map<std::string, ImageDecoder::CreateFunc> m_decoders;
        std::map<std::string, ImageEncoder::CreateFunc> m_encoders;

    public:
        ImageServer()
        {
            registerImageDecoderTGA();
            registerImageDecoderPKM();
            registerImageDecoderDDS();
            registerImageDecoderPNG();
            registerImageDecoderJPG();
            registerImageDecoderBMP();
            registerImageDecoderPCX();
            registerImageDecoderIFF();
            registerImageDecoderHDR();
            registerImageDecoderGIF();
            registerImageDecoderKTX();
            registerImageDecoderPVR();
            registerImageDecoderASTC();
            registerImageDecoderZPNG();
            registerImageDecoderSGI();
            registerImageDecoderPNM();
            registerImageDecoderATARI();
            registerImageDecoderC64();
        }

        ~ImageServer()
        {
        }

        std::string getLowerCaseExtension(const std::string& filename) const
        {
            // strip the filename away and make the extension (.ext) lower-case only
            std::string extension = filesystem::getExtension(filename);
            return toLower(extension.empty() ? filename : extension);
        }

        void registerImageDecoder(ImageDecoder::CreateFunc func, const std::string& extension)
        {
            m_decoders[toLower(extension)] = func;
        }

        void registerImageEncoder(ImageEncoder::CreateFunc func, const std::string& extension)
        {
            m_encoders[toLower(extension)] = func;
        }

        ImageDecoder::CreateFunc getImageDecoder(const std::string& extension) const
        {
            auto i = m_decoders.find(getLowerCaseExtension(extension));
            if (i != m_decoders.end())
            {
                return i->second;
            }

            return createUnsupportedImageDecoderInterface;
        }

        ImageEncoder::CreateFunc getImageEncoder(const std::string& extension) const
        {
            auto i = m_encoders.find(getLowerCaseExtension(extension));
            if (i != m_encoders.end())
            {
                return i->second;
            }

            return unsupportedImageEncoder;
        }
    } g_imageServer;

    void registerImageDecoder(ImageDecoder::CreateFunc func, const std::string& extension)
    {
        g_imageServer.registerImageDecoder(func, extension);
    }

    void registerImageEncoder(ImageEncoder::CreateFunc func, const std::string& extension)
    {
        g_imageServer.registerImageEncoder(func, extension);
    }

    bool isImageDecoder(const std::string& extension)
    {
        auto func = g_imageServer.getImageDecoder(extension);
        return func != createUnsupportedImageDecoderInterface;
    }

    bool isImageEncoder(const std::string& extension)
    {
        auto func = g_imageServer.getImageEncoder(extension);
        return func != unsupportedImageEncoder;
    }

    // ----------------------------------------------------------------------------
    // ImageDecoderInterface
    // ----------------------------------------------------------------------------

    Exif ImageDecoderInterface::exif()
    {
        return Exif();
    }

    Memory ImageDecoderInterface::memory(int level, int depth, int face)
    {
        MANGO_UNREFERENCED_PARAMETER(level);
        MANGO_UNREFERENCED_PARAMETER(depth);
        MANGO_UNREFERENCED_PARAMETER(face);
        return Memory();
    }

    // ----------------------------------------------------------------------------
    // ImageDecoder
    // ----------------------------------------------------------------------------

    ImageDecoder::ImageDecoder(Memory memory, const std::string& filename)
    {
        ImageDecoder::CreateFunc func = g_imageServer.getImageDecoder(filename);
        m_interface = func(memory);
        m_is_decoder = func != createUnsupportedImageDecoderInterface;
    }

    ImageDecoder::~ImageDecoder()
    {
        delete m_interface;
    }

    bool ImageDecoder::isDecoder() const
    {
        return m_is_decoder;
    }

    ImageHeader ImageDecoder::header()
    {
        return m_interface->header();
    }

    Exif ImageDecoder::exif()
    {
        return m_interface->exif();
    }

    Memory ImageDecoder::memory(int level, int depth, int face)
    {
        return m_interface->memory(level, depth, face);
    }

    void ImageDecoder::decode(Surface& dest, Palette* palette, int level, int depth, int face)
    {
        m_interface->decode(dest, palette, level, depth, face);
    }

    // ----------------------------------------------------------------------------
    // ImageEncoder
    // ----------------------------------------------------------------------------

    ImageEncoder::ImageEncoder(const std::string& extension)
    {
        m_encode = g_imageServer.getImageEncoder(extension);
    }

    ImageEncoder::~ImageEncoder()
    {
    }

    bool ImageEncoder::isEncoder() const
    {
        return m_encode != unsupportedImageEncoder;
    }

    void ImageEncoder::encode(Stream& output, const Surface& source, const ImageEncoderOptions* options)
    {
        m_encode(output, source, options);
    }

} // namespace mango

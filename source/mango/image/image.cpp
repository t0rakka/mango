/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/string.hpp>
#include <mango/core/timer.hpp>
#include <mango/image/image.hpp>

namespace mango
{

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

        ImageDecoder::CreateFunc getImageDecoder(const std::string& filename) const
        {
            std::string extension = getLowerCaseExtension(filename);

            auto i = m_decoders.find(extension);
            if (i != m_decoders.end())
            {
                return i->second;
            }

            return nullptr;
        }

        ImageEncoder::CreateFunc getImageEncoder(const std::string& filename) const
        {
            std::string extension = getLowerCaseExtension(filename);

            auto i = m_encoders.find(extension);
            if (i != m_encoders.end())
            {
                return i->second;
            }

            return nullptr;
        }

        ImageDecoderInterface* createImageDecoder(Memory memory, const std::string& filename)
        {
            ImageDecoderInterface* decoder = nullptr;
            ImageDecoder::CreateFunc func = getImageDecoder(filename);
            if (func)
                decoder = func(memory);
            return decoder;
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
        return func != nullptr;
    }

    bool isImageEncoder(const std::string& extension)
    {
        auto func = g_imageServer.getImageEncoder(extension);
        return func != nullptr;
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
        m_interface = g_imageServer.createImageDecoder(memory, filename);
    }

    ImageDecoder::~ImageDecoder()
    {
        delete m_interface;
    }

    bool ImageDecoder::isDecoder() const
    {
        return m_interface != nullptr;
    }

    ImageHeader ImageDecoder::header()
    {
        return m_interface ? m_interface->header() : ImageHeader();
    }

    Exif ImageDecoder::exif()
    {
        return m_interface ? m_interface->exif() : Exif();
    }

    Memory ImageDecoder::memory(int level, int depth, int face)
    {
        return m_interface ? m_interface->memory(level, depth, face) : Memory();
    }

    void ImageDecoder::decode(Surface& dest, Palette* palette, int level, int depth, int face)
    {
        if (m_interface)
        {
            m_interface->decode(dest, palette, level, depth, face);
        }
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
        return m_encode != nullptr;
    }

    void ImageEncoder::encode(Stream& output, const Surface& source, float quality)
    {
        if (m_encode)
        {
            m_encode(output, source,quality);
        }
    }

} // namespace mango

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
    void registerImageDecoderWEBP();

    class ImageServer
    {
    protected:
        std::map<std::string, ImageDecoder::CreateDecoderFunc> m_decoders;
        std::map<std::string, ImageEncoder::EncodeFunc> m_encoders;

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
            registerImageDecoderWEBP();
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

        void registerImageDecoder(ImageDecoder::CreateDecoderFunc func, const std::string& extension)
        {
            m_decoders[toLower(extension)] = func;
        }

        void registerImageEncoder(ImageEncoder::EncodeFunc func, const std::string& extension)
        {
            m_encoders[toLower(extension)] = func;
        }

        ImageDecoder::CreateDecoderFunc getImageDecoder(const std::string& extension) const
        {
            auto i = m_decoders.find(getLowerCaseExtension(extension));
            if (i != m_decoders.end())
            {
                return i->second;
            }

            return nullptr;
        }

        ImageEncoder::EncodeFunc getImageEncoder(const std::string& extension) const
        {
            auto i = m_encoders.find(getLowerCaseExtension(extension));
            if (i != m_encoders.end())
            {
                return i->second;
            }

            return nullptr;
        }
    } g_imageServer;

    void registerImageDecoder(ImageDecoder::CreateDecoderFunc func, const std::string& extension)
    {
        g_imageServer.registerImageDecoder(func, extension);
    }

    void registerImageEncoder(ImageEncoder::EncodeFunc func, const std::string& extension)
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
        ImageDecoder::CreateDecoderFunc create_decoder_func = g_imageServer.getImageDecoder(filename);
        ImageDecoderInterface* x = create_decoder_func(memory);
        m_interface.reset(x);
    }

    ImageDecoder::~ImageDecoder()
    {
    }

    bool ImageDecoder::isDecoder() const
    {
        return m_interface != nullptr;
    }

    ImageHeader ImageDecoder::header()
    {
        if (!m_interface)
        {
            printf("[WARNING] ImageDecoder::header() is not supported for this extension.");
            return ImageHeader();
        }
        return m_interface->header();
    }

    Exif ImageDecoder::exif()
    {
        if (!m_interface)
        {
            printf("[WARNING] ImageDecoder::exit() is not supported for this extension.");
            return Exif();
        }
        return m_interface->exif();
    }

    Memory ImageDecoder::memory(int level, int depth, int face)
    {
        if (!m_interface)
        {
            printf("[WARNING] ImageDecoder::memory() is not supported for this extension.");
            return Memory();
        }
        return m_interface->memory(level, depth, face);
    }

    void ImageDecoder::decode(Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face)
    {
        if (!m_interface)
        {
            printf("[WARNING] ImageDecoder::decode() is not supported for this extension.");
            return;
        }
        m_interface->decode(dest, options.palette, level, depth, face);
    }

    // ----------------------------------------------------------------------------
    // ImageEncoder
    // ----------------------------------------------------------------------------

    ImageEncoder::ImageEncoder(const std::string& extension)
    {
        m_encode_func = g_imageServer.getImageEncoder(extension);
    }

    ImageEncoder::~ImageEncoder()
    {
    }

    bool ImageEncoder::isEncoder() const
    {
        return m_encode_func != nullptr;
    }

    void ImageEncoder::encode(Stream& output, const Surface& source, const ImageEncodeOptions& options)
    {
        if (!m_encode_func)
        {
            printf("[WARNING] ImageEncoder::encode() is not supported for this extension.");
            return;
        }
        m_encode_func(output, source, options);
    }

} // namespace mango

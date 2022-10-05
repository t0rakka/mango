/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/string.hpp>
#include <mango/core/timer.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;

    std::string getLowerCaseExtension(const std::string& filename)
    {
        // strip the filename away and make the extension (.ext) lower-case only
        std::string extension = filesystem::getExtension(filename);
        return toLower(extension.empty() ? filename : extension);
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
    void registerImageCodecWEBP();
    void registerImageCodecEXR();
    void registerImageCodecQOI();

#if defined(MANGO_ENABLE_JXL)
    void registerImageCodecJXL();
#endif

#if defined(MANGO_ENABLE_JP2)
    void registerImageCodecJP2();
#endif

    class ImageServer
    {
    protected:
        std::map<std::string, ImageDecoder::CreateDecoderFunc> m_decoders;
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
            registerImageCodecWEBP();
            registerImageCodecEXR();
            registerImageCodecQOI();

#if defined(MANGO_ENABLE_JXL)
            registerImageCodecJXL();
#endif

#if defined(MANGO_ENABLE_JP2)
            registerImageCodecJP2();
#endif
        }

        ~ImageServer()
        {
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
            auto i = m_decoders.find(extension);
            return i != m_decoders.end() ? i->second : nullptr;
        }

        ImageEncoder::EncodeFunc getImageEncoder(const std::string& extension) const
        {
            auto i = m_encoders.find(extension);
            return i != m_encoders.end() ? i->second : nullptr;
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
    // ImageDecoderInterface
    // ----------------------------------------------------------------------------

    ConstMemory ImageDecoderInterface::memory(int level, int depth, int face)
    {
        MANGO_UNREFERENCED(level);
        MANGO_UNREFERENCED(depth);
        MANGO_UNREFERENCED(face);
        return ConstMemory();
    }

    ConstMemory ImageDecoderInterface::icc()
    {
        return ConstMemory();
    }

    ConstMemory ImageDecoderInterface::exif()
    {
        return ConstMemory();
    }

    // ----------------------------------------------------------------------------
    // ImageDecoder
    // ----------------------------------------------------------------------------

    ImageDecoder::ImageDecoder(ConstMemory memory, const std::string& filename)
    {
        std::string extension = getLowerCaseExtension(filename);
        ImageDecoder::CreateDecoderFunc create = g_imageServer.getImageDecoder(extension);
        if (create)
        {
            ImageDecoderInterface* x = create(memory);
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

    ImageHeader ImageDecoder::header()
    {
        ImageHeader header;

        if (m_interface)
        {
            header = m_interface->header();
        }
        else
        {
            header.setError("[WARNING] ImageDecoder::header() is not supported for this extension.");
        }

        return header;
    }

    ImageDecodeStatus ImageDecoder::decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face)
    {
        ImageDecodeStatus status;

        if (m_interface)
        {
            status = m_interface->decode(dest, options, level, depth, face);
        }
        else
        {
            status.setError("[WARNING] ImageDecoder::decode() is not supported for this extension.");
        }

        return status;
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
            memory = m_interface->icc();
        }

        return memory;
    }

    ConstMemory ImageDecoder::exif()
    {
        ConstMemory memory;

        if (m_interface)
        {
            memory = m_interface->exif();
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

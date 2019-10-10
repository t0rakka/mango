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

#ifdef MANGO_ENABLE_IMAGE_TGA
    void registerImageDecoderTGA();
#endif
#ifdef MANGO_ENABLE_IMAGE_PKM
    void registerImageDecoderPKM();
#endif
#ifdef MANGO_ENABLE_IMAGE_DDS
    void registerImageDecoderDDS();
#endif
#ifdef MANGO_ENABLE_IMAGE_PNG
    void registerImageDecoderPNG();
#endif
#ifdef MANGO_ENABLE_IMAGE_JPG
    void registerImageDecoderJPG();
#endif
#ifdef MANGO_ENABLE_IMAGE_BMP
    void registerImageDecoderBMP();
#endif
#ifdef MANGO_ENABLE_IMAGE_PCX
    void registerImageDecoderPCX();
#endif
#ifdef MANGO_ENABLE_IMAGE_IFF
    void registerImageDecoderIFF();
#endif
#ifdef MANGO_ENABLE_IMAGE_HDR
    void registerImageDecoderHDR();
#endif
#ifdef MANGO_ENABLE_IMAGE_GIF
    void registerImageDecoderGIF();
#endif
#ifdef MANGO_ENABLE_IMAGE_KTX
    void registerImageDecoderKTX();
#endif
#ifdef MANGO_ENABLE_IMAGE_PVR
    void registerImageDecoderPVR();
#endif
#ifdef MANGO_ENABLE_IMAGE_ASTC
    void registerImageDecoderASTC();
#endif
#ifdef MANGO_ENABLE_IMAGE_ZPNG
    void registerImageDecoderZPNG();
#endif
#ifdef MANGO_ENABLE_IMAGE_SGI
    void registerImageDecoderSGI();
#endif
#ifdef MANGO_ENABLE_IMAGE_PNM
    void registerImageDecoderPNM();
#endif
#ifdef MANGO_ENABLE_IMAGE_ATARI
    void registerImageDecoderATARI();
#endif
#ifdef MANGO_ENABLE_IMAGE_C64
    void registerImageDecoderC64();
#endif
#ifdef MANGO_ENABLE_IMAGE_WEBP
    void registerImageDecoderWEBP();
#endif

    class ImageServer
    {
    protected:
        std::map<std::string, ImageDecoder::CreateDecoderFunc> m_decoders;
        std::map<std::string, ImageEncoder::EncodeFunc> m_encoders;

    public:
        ImageServer()
        {
#ifdef MANGO_ENABLE_IMAGE_TGA
            registerImageDecoderTGA();
#endif
#ifdef MANGO_ENABLE_IMAGE_PKM
            registerImageDecoderPKM();
#endif
#ifdef MANGO_ENABLE_IMAGE_DDS
            registerImageDecoderDDS();
#endif
#ifdef MANGO_ENABLE_IMAGE_PNG
            registerImageDecoderPNG();
#endif
#ifdef MANGO_ENABLE_IMAGE_JPG
            registerImageDecoderJPG();
#endif
#ifdef MANGO_ENABLE_IMAGE_BMP
            registerImageDecoderBMP();
#endif
#ifdef MANGO_ENABLE_IMAGE_PCX
            registerImageDecoderPCX();
#endif
#ifdef MANGO_ENABLE_IMAGE_IFF
            registerImageDecoderIFF();
#endif
#ifdef MANGO_ENABLE_IMAGE_HDR
            registerImageDecoderHDR();
#endif
#ifdef MANGO_ENABLE_IMAGE_GIF
            registerImageDecoderGIF();
#endif
#ifdef MANGO_ENABLE_IMAGE_KTX
            registerImageDecoderKTX();
#endif
#ifdef MANGO_ENABLE_IMAGE_PVR
            registerImageDecoderPVR();
#endif
#ifdef MANGO_ENABLE_IMAGE_ASTC
            registerImageDecoderASTC();
#endif
#ifdef MANGO_ENABLE_IMAGE_ZPNG
            registerImageDecoderZPNG();
#endif
#ifdef MANGO_ENABLE_IMAGE_SGI
            registerImageDecoderSGI();
#endif
#ifdef MANGO_ENABLE_IMAGE_PNM
            registerImageDecoderPNM();
#endif
#ifdef MANGO_ENABLE_IMAGE_ATARI
            registerImageDecoderATARI();
#endif
#ifdef MANGO_ENABLE_IMAGE_C64
            registerImageDecoderC64();
#endif
#ifdef MANGO_ENABLE_IMAGE_WEBP
            registerImageDecoderWEBP();
#endif
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

    Memory ImageDecoderInterface::memory(int level, int depth, int face)
    {
        MANGO_UNREFERENCED(level);
        MANGO_UNREFERENCED(depth);
        MANGO_UNREFERENCED(face);
        return Memory();
    }

    Memory ImageDecoderInterface::icc()
    {
        return Memory();
    }

    Memory ImageDecoderInterface::exif()
    {
        return Memory();
    }

    // ----------------------------------------------------------------------------
    // ImageDecoder
    // ----------------------------------------------------------------------------

    ImageDecoder::ImageDecoder(Memory memory, const std::string& filename)
    {
        ImageDecoder::CreateDecoderFunc create_decoder_func = g_imageServer.getImageDecoder(filename);
        if (create_decoder_func)
        {
            ImageDecoderInterface* x = create_decoder_func(memory);
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

        if (!m_interface)
        {
            header.setError("[WARNING] ImageDecoder::header() is not supported for this extension.");
        }
        else
        {
            header = m_interface->header();
        }

        return header;
    }

    ImageDecodeStatus ImageDecoder::decode(Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face)
    {
        ImageDecodeStatus status;

        if (!m_interface)
        {
            status.setError("[WARNING] ImageDecoder::decode() is not supported for this extension.");
        }
        else
        {
            status = m_interface->decode(dest, options.palette, level, depth, face);
        }

        return status;
    }

    Memory ImageDecoder::memory(int level, int depth, int face)
    {
        Memory memory;

        if (m_interface)
        {
            memory = m_interface->memory(level, depth, face);
        }

        return memory;
    }

    Memory ImageDecoder::icc()
    {
        Memory memory;

        if (m_interface)
        {
            memory = m_interface->icc();
        }

        return memory;
    }

    Memory ImageDecoder::exif()
    {
        Memory memory;

        if (m_interface)
        {
            memory = m_interface->exif();
        }

        return memory;
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

    ImageEncodeStatus ImageEncoder::encode(Stream& output, const Surface& source, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status;

        if (!m_encode_func)
        {
            status.setError("[WARNING] ImageEncoder::encode() is not supported for this extension.");
        }
        else
        {
            status = m_encode_func(output, source, options);
        }

        return status;
    }

} // namespace mango

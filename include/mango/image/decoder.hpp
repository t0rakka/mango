/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <mango/core/memory.hpp>
#include <mango/core/exception.hpp>
#include <mango/image/format.hpp>
#include <mango/image/compression.hpp>
#include <mango/image/exif.hpp>

namespace mango::image
{
    class Surface;

    /*
        These flags indicate when supercompressed blocks are stored in the image file.
        This kind of data can be extracted from the file by two mechanisms:

        1. decoded into Surface
        2. the compressed memory block and TextureCompression format can be queried

    */
    enum : u32
    {
        SUPERCOMPRESS_ETC1_RGB         = 0x00000001,
        SUPERCOMPRESS_BC1_UNORM        = 0x00000002,
        SUPERCOMPRESS_BC4_UNORM        = 0x00000004,
        SUPERCOMPRESS_BC7_UNORM        = 0x00000008,
        SUPERCOMPRESS_PVRTC_RGB_4BPP   = 0x00000010,
        SUPERCOMPRESS_PVRTC_RGBA_4BPP  = 0x00000020,
        SUPERCOMPRESS_ASTC_RGBA_4x4    = 0x00000030,
        SUPERCOMPRESS_ATC_RGB          = 0x00000040,
        SUPERCOMPRESS_ATC_RGBA         = 0x00000100,
        SUPERCOMPRESS_FXT1_RGB         = 0x00000200,
        SUPERCOMPRESS_PVRTC2_RGBA_4BPP = 0x00000300,
        SUPERCOMPRESS_EAC_R11          = 0x00000400,
        SUPERCOMPRESS_EAC_RG11         = 0x00001000,
        SUPERCOMPRESS_BASISU_ETC1S     = 0x00001fff,
        SUPERCOMPRESS_BASISU_UASTC     = 0x00001fff,
    };

    struct ImageHeader : Status
    {
        int     width = 0;   // width
        int     height = 0;  // height
        int     depth = 0;   // depth
        int     levels = 0;  // mipmap levels
        int     faces = 0;   // cubemap faces
        bool    palette = false; // palette is available
        bool    premultiplied = false; // alpha is premultiplied
        bool    linear = false; // linear colorspace (non-linear is assumed to be sRGB)
        Format  format; // preferred format (fastest available "direct" decoding is possible)
        u32     compression = TextureCompression::NONE;
        u32     supercompression = 0; // mask of supported compression formats
    };

    struct ImageDecodeStatus : Status
    {
        bool direct = false;

        // animation information
        // NOTE: we would love to simply return number of animation frames in the ImageHeader
        //       but some formats do not provide this information without decompressing frames
        //       until running out of data.
        int current_frame_index = 0;
        int next_frame_index = 0;

        // animation frame duration in (numerator / denominator) seconds
        int frame_delay_numerator = 1;    // 1 frame...
        int frame_delay_denominator = 60; // ... every 60th of a second
    };

    struct ImageDecodeOptions
    {
        // request indexed decoding
        // - palette is resolved into the provided palette object
        // - decode() destination surface must be indexed
        Palette* palette = nullptr; // enable indexed decoding by pointing to a palette

        bool simd = true;
        bool multithread = true;
        bool icc = false; // apply ICC profile
    };

    class ImageDecoderInterface : protected NonCopyable
    {
    public:
        std::string name;

        ImageDecoderInterface() = default;
        virtual ~ImageDecoderInterface() = default;

        virtual ImageHeader header() = 0;
        virtual ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) = 0;

        // optional
        virtual ConstMemory memory(int level, int depth, int face); // get compressed data
        virtual ConstMemory icc(); // get ICC data
        virtual ConstMemory exif(); // get exif data
    };

    class ImageDecoder : protected NonCopyable
    {
    public:
        ImageDecoder(ConstMemory memory, const std::string& extension);
        ~ImageDecoder();

        bool isDecoder() const;
        ImageHeader header();
        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options = ImageDecodeOptions(), int level = 0, int depth = 0, int face = 0);

        ConstMemory memory(int level, int depth, int face);
        ConstMemory icc();
        ConstMemory exif();

        using CreateDecoderFunc = ImageDecoderInterface* (*)(ConstMemory memory);

    protected:
        std::unique_ptr<ImageDecoderInterface> m_interface;
    };

    void registerImageDecoder(ImageDecoder::CreateDecoderFunc func, const std::string& extension);
    bool isImageDecoder(const std::string& extension);

} // namespace mango::image

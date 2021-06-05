/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
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

    struct ImageHeader : Status
    {
        int     width = 0;   // width
        int     height = 0;  // height
        int     depth = 0;   // depth
        int     levels = 0;  // mipmap levels
        int     faces = 0;   // cubemap faces
        bool    palette = false; // palette is available
        bool    premultiplied = false; // alpha is premultiplied
        Format  format; // preferred format (fastest available "direct" decoding is possible)
        TextureCompression compression = TextureCompression::NONE;
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
    };

    class ImageDecoderInterface : protected NonCopyable
    {
    public:
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

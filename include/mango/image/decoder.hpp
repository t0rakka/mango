/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include "../core/object.hpp"
#include "format.hpp"
#include "compression.hpp"
#include "exif.hpp"

namespace mango
{

    struct ImageHeader
    {
        int     width = 0;   // width
        int     height = 0;  // height
        int     depth = 0;   // depth
        int     levels = 0;  // mipmap levels
        int     faces = 0;   // cubemap faces
        bool    palette = false; // decoder supports palette export
        Format  format;
        TextureCompression compression = TextureCompression::NONE;
    };

    class ImageDecoderInterface : protected NonCopyable
    {
    public:
        ImageDecoderInterface() = default;
        virtual ~ImageDecoderInterface() = default;

        virtual ImageHeader header() = 0;
        virtual void decode(Surface& dest, Palette* palette, int level, int depth, int face) = 0;
        virtual Exif exif(); // get exif data (optional)
        virtual Memory memory(int level, int depth, int face); // get compressed data (optional)
    };

    class ImageDecoder : protected NonCopyable
    {
    public:
        ImageDecoder(Memory memory, const std::string& extension);
        ~ImageDecoder();

        bool isDecoder() const;
        ImageHeader header();
        Exif exif();
        Memory memory(int level, int depth, int face);
        void decode(Surface& dest, Palette* palette = nullptr, int level = 0, int depth = 0, int face = 0);

        typedef ImageDecoderInterface* (*CreateDecoderFunc)(Memory memory);

    protected:
        ImageDecoderInterface* m_interface;
        bool m_is_decoder;
    };

    void registerImageDecoder(ImageDecoder::CreateDecoderFunc func, const std::string& extension);
    bool isImageDecoder(const std::string& extension);

} // namespace mango

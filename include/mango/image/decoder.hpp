/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <future>
#include <functional>
#include <mango/core/memory.hpp>
#include <mango/core/exception.hpp>
#include <mango/image/format.hpp>
#include <mango/image/color.hpp>
#include <mango/image/compression.hpp>
#include <mango/image/exif.hpp>

namespace mango::filesystem
{
    class Path;
} // namespace mango::filesystem

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
        int     frames = 0;  // animation frame count: 0 = not animated, > 0 = animation (1 is a degenerate animation, aliases with 0)
        bool    premultiplied = false; // alpha is premultiplied
        bool    linear = false; // linear colorspace (non-linear is sRGB); mirrors color.isLinear()

        // Color space signalling (primaries, transfer function, exact chromaticities).
        // Defaults to sRGB: integer image formats are sRGB by near-universal convention,
        // so a decoder that does not override this reports sRGB. Linear/HDR/float decoders
        // set TransferFunction::Linear (and keep 'linear' consistent).
        ColorInfo color { ColorPrimaries::BT709, TransferFunction::sRGB };

        Format  format; // preferred format (fastest available "direct" decoding is possible)
        u32     compression = TextureCompression::NONE;
        u32     supercompression = 0; // mask of supported compression formats
    };

    struct ImageDecodeStatus : Status
    {
        bool direct = false; // decoding doesn't use temporary storage

        // animation information
        // NOTE: ImageHeader::frames reports the total frame count up front when the format
        //       can provide it (e.g. GIF, APNG). These indices remain useful for streaming
        //       playback and for formats that only discover the count while decoding.
        int current_frame_index = 0;
        int next_frame_index = 0;

        // animation frame duration in (numerator / denominator) seconds
        int frame_delay_numerator = 1;    // 1 frame...
        int frame_delay_denominator = 60; // ... every 60th of a second
    };

    struct ImageDecodeOptions
    {
        bool simd = true;
        bool multithread = true;
        bool jpeg_colorspace_rgb = false; // assumes channel data is RGB instead of YCbCr
    };

    struct ImageDecodeRect
    {
        int x;
        int y;
        int width;
        int height;
        float progress;
    };

    using ImageDecodeCallback = std::function<void(const ImageDecodeRect& rect)>;
    using ImageDecodeFuture = std::future<ImageDecodeStatus>;

    class ImageDecodeInterface : protected NonCopyable
    {
    public:
        bool async = false;
        ImageDecodeCallback callback;
        std::atomic<bool> cancelled { false };
        std::string name;
        ImageHeader header;
        ConstMemory icc;
        ConstMemory exif;

        // Full virtual path of the source image (empty for memory-only sources).
        std::string filename;

        // Loads a companion / sidecar file located next to the primary image,
        // identified by a replacement extension (e.g. ".pl5"). Resolves through
        // the same virtual filesystem (folders and mounted archives) the primary
        // image came from, and the returned bytes stay valid for the decoder's
        // lifetime. Returns empty memory when the source is memory-only or the
        // companion does not exist. May be null - always test before calling.
        std::function<ConstMemory(const std::string& extension)> acquireCompanion;

        ImageDecodeInterface() = default;
        virtual ~ImageDecodeInterface() = default;

        virtual ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face);
        virtual ConstMemory memory(int level, int depth, int face);
    };

    class ImageDecoder : protected NonCopyable
    {
    public:
        ImageDecoder(ConstMemory memory, const std::string& filename);

        // Companion files (sidecars) are resolved relative to 'path', reusing its
        // already-mounted mapper. Prefer this overload when the image lives inside
        // mounted containers (zip/rar/iso, possibly stacked) - it avoids re-parsing
        // the whole container chain just to read a small sidecar. 'path' must
        // outlive this decoder. 'filename' is the image name relative to 'path'.
        ImageDecoder(ConstMemory memory, const filesystem::Path& path, const std::string& filename);

        ~ImageDecoder();

        bool isDecoder() const;
        bool isAsyncDecoder() const;

        ImageHeader header();
        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options = ImageDecodeOptions(), int level = 0, int depth = 0, int face = 0);
        ImageDecodeFuture launch(ImageDecodeCallback callback, const Surface& dest, const ImageDecodeOptions& options = ImageDecodeOptions(), int level = 0, int depth = 0, int face = 0);
        void cancel();

        ConstMemory memory(int level, int depth, int face);
        ConstMemory icc();
        ConstMemory exif();

        using CreateDecodeFunc = ImageDecodeInterface* (*)(ConstMemory memory);

    protected:
        std::shared_ptr<ImageDecodeInterface> m_interface;
    };

    void registerImageDecoder(ImageDecoder::CreateDecodeFunc func, const std::string& extension);
    bool isImageDecoder(const std::string& extension);

} // namespace mango::image

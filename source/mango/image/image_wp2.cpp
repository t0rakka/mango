/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_WP2)

#include <wp2/base.h>
#include <wp2/decode.h>
#include <wp2/encode.h>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            u32 width = 0;
            u32 height = 0;

            int s = WP2Parse(m_memory.address, m_memory.size, &width, &height);
            if (s != WP2_STATUS_OK)
            {
                const char* message = WP2GetStatusMessage(WP2Status(s));
                header.setError("[ImageDecoder.WP2] WP2Parse() -> {}", message);
                return;
            }

            // TODO: support more output formats, including HDR (floating point)
            // TODO: support premult alpha flag

            header.width = width;
            header.height = height;
            header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }

        ~Interface()
        {
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            DecodeTargetBitmap target(dest, header.width, header.height, header.format);

            // TODO: support more input formats, including HDR (floating point)
            WP2::ArgbBuffer buffer(WP2_RGBA_32);

            WP2Status s;
            s = buffer.SetExternal(target.width, target.height, target.image, target.stride, false);
            if (s != WP2_STATUS_OK)
            {
                status.setError("[ImageEncoder.WP2] WP2::ArgbBuffer::SetExternal() -> {}", WP2GetStatusMessage(s));
                return status;
            }

            WP2::DecoderConfig config = WP2::DecoderConfig::kDefault;
            config.thread_level = options.multithread ? std::thread::hardware_concurrency() : 0;

            s = WP2::Decode(m_memory.address, m_memory.size, &buffer, config);
            if (s != WP2_STATUS_OK)
            {
                const char* message = WP2GetStatusMessage(WP2Status(s));
                status.setError("[ImageDecoder.WP2] WP2::Decode() -> {}", message);
                return status;
            }

            target.resolve();

            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status;

        // convert to correct format when required
        Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        TemporaryBitmap temp(surface, format);

        WP2Status s;

        // TODO: support more input formats, including HDR (floating point)
        WP2::ArgbBuffer buffer(WP2_RGBA_32);

        s = buffer.SetExternal(temp.width, temp.height, temp.image, temp.stride, false);
        if (s != WP2_STATUS_OK)
        {
            status.setError("[ImageEncoder.WP2] WP2::ArgbBuffer::SetExternal() -> {}", WP2GetStatusMessage(s));
            return status;
        }

        WP2::EncoderConfig config;

        // NOTE: effort is between 0 and 9, but anything higher than 6 gets stuck on version 0.1.0
        config.effort = std::clamp(options.compression * 3 / 4, 0, 6);
        config.quality = std::clamp(options.quality, 0.0f, 1.0f) * 100.0f;
        config.thread_level = options.multithread ? std::thread::hardware_concurrency() : 0;

        if (options.lossless)
        {
            config.quality = 100.0f;
        }

        WP2::MemoryWriter writer;

        s = WP2::Encode(buffer, &writer, config);
        if (s != WP2_STATUS_OK)
        {
            status.setError("[ImageEncoder.WP2] WP2::Encodel() -> {}", WP2GetStatusMessage(s));
            return status;
        }

        stream.write(writer.mem_, writer.size_);

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecWP2()
    {
        registerImageDecoder(createInterface, ".wp2");
        registerImageEncoder(imageEncode, ".wp2");
    }

} // namespace mango::image

#else

namespace mango::image
{

    void registerImageCodecWP2()
    {
        // WP2 codec is disabled
    }

} // namespace mango::image

#endif // defined(MANGO_ENABLE_WP2)

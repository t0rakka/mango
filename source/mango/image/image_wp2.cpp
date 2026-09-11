/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#include <wp2/base.h>
#include <wp2/decode.h>
#include <wp2/encode.h>

namespace
{
    using namespace mango;
    using namespace mango::image;

    static
    TransferFunction wp2TransferFunction(WP2::TransferFunction transfer)
    {
        switch (transfer)
        {
            case WP2::WP2_TF_LINEAR:
                return TransferFunction::Linear;
            case WP2::WP2_TF_SMPTE_ST_2084:
                return TransferFunction::PQ;
            case WP2::WP2_TF_ARIB_STD_B67_HLG:
                return TransferFunction::HLG;
            case WP2::WP2_TF_IEC_61966_2_1:
                return TransferFunction::sRGB;
            case WP2::WP2_TF_GAMMA_22:
                return TransferFunction::Gamma22;
            case WP2::WP2_TF_GAMMA_28:
                return TransferFunction::Gamma28;
            case WP2::WP2_TF_SMPTE_170M:
            case WP2::WP2_TF_SMPTE_240M:
                return TransferFunction::BT709;
            default:
                return TransferFunction::Unspecified;
        }
    }

    static
    bool isHdrWp2(const WP2::BitstreamFeatures& features)
    {
        if (features.rgb_bit_depth > 8)
            return true;

        switch (features.transfer_function)
        {
            case WP2::WP2_TF_LINEAR:
            case WP2::WP2_TF_SMPTE_ST_2084:
            case WP2::WP2_TF_ARIB_STD_B67_HLG:
            case WP2::WP2_TF_ITU_R_BT2020_12BIT:
                return true;
            default:
                return false;
        }
    }

    static
    void copyRgba64ToFloat(const Surface& dest, const Surface& source, float scale)
    {
        const int width = dest.width;
        const int height = dest.height;

        for (int y = 0; y < height; ++y)
        {
            const u16* src = source.address<u16>(0, y);
            float* dst = dest.address<float>(0, y);

            for (int x = 0; x < width; ++x)
            {
                dst[0] = float(src[0]) * scale;
                dst[1] = float(src[1]) * scale;
                dst[2] = float(src[2]) * scale;
                dst[3] = float(src[3]) * scale;
                src += 4;
                dst += 4;
            }
        }
    }

    static
    void copyFloatToRgba64(const Surface& dest, const Surface& source, float scale)
    {
        const int width = dest.width;
        const int height = dest.height;

        for (int y = 0; y < height; ++y)
        {
            const float* src = source.address<float>(0, y);
            u16* dst = dest.address<u16>(0, y);

            for (int x = 0; x < width; ++x)
            {
                dst[0] = u16(std::clamp(src[0] * scale, 0.0f, 65535.0f));
                dst[1] = u16(std::clamp(src[1] * scale, 0.0f, 65535.0f));
                dst[2] = u16(std::clamp(src[2] * scale, 0.0f, 65535.0f));
                dst[3] = u16(std::clamp(src[3] * scale, 0.0f, 65535.0f));
                src += 4;
                dst += 4;
            }
        }
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        WP2::BitstreamFeatures m_features;
        bool m_features_valid = false;
        bool m_high_precision = false;
        float m_decode_scale = 1.0f / 255.0f;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            WP2Status s = m_features.Read(m_memory.address, m_memory.size);
            if (s != WP2_STATUS_OK)
            {
                const char* message = WP2GetStatusMessage(s);
                header.setError("[ImageDecoder.WP2] WP2Parse() -> {}", message);
                return;
            }

            m_features_valid = true;
            m_high_precision = isHdrWp2(m_features);

            header.width = m_features.raw_width;
            header.height = m_features.raw_height;
            header.premultiplied = m_features.is_premultiplied;
            header.color.transfer = wp2TransferFunction(m_features.transfer_function);
            header.linear = header.color.isLinear();

            if (m_high_precision)
            {
                m_decode_scale = 1.0f / float((1u << m_features.rgb_bit_depth) - 1u);
                header.format = Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32);
            }
            else
            {
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }

            size_t num_frames = 1;
            if (WP2::GetNumFrames(m_memory.address, m_memory.size, &num_frames) == WP2_STATUS_OK)
                header.frames = int(num_frames);
        }

        ~Interface()
        {
        }

        void populateInspect(ImageInspect& report) const override
        {
            if (!m_features_valid)
                return;

            report.lossless = InspectTriState::No;
            report.progressive = InspectTriState::No;
            report.tiling.tiled = InspectTriState::Unknown;
            report.chroma_subsampling = "4:4:4";
            report.encoding = m_features.is_animation ? "WebP2 animated" : "WebP2";
            report.bit_depth = int(m_features.rgb_bit_depth);
            report.alpha = !m_features.is_opaque;

            syncHdrInspectFromHeader(report);
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (m_high_precision)
            {
                TemporaryBitmap temp(dest, header.width, header.height,
                    Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16));

                WP2::ArgbBuffer buffer(WP2_RGBA_64);
                WP2Status s = buffer.SetExternal(temp.width, temp.height,
                    reinterpret_cast<uint16_t*>(temp.image), temp.stride / 2, false);
                if (s != WP2_STATUS_OK)
                {
                    const char* message = WP2GetStatusMessage(s);
                    status.setError("[ImageDecoder.WP2] WP2::ArgbBuffer::SetExternal() -> {}", message);
                    return status;
                }

                WP2::DecoderConfig config = WP2::DecoderConfig::kDefault;
                config.thread_level = options.multithread ? std::thread::hardware_concurrency() : 0;

                s = WP2::Decode(m_memory.address, m_memory.size, &buffer, config);
                if (s != WP2_STATUS_OK)
                {
                    const char* message = WP2GetStatusMessage(s);
                    status.setError("[ImageDecoder.WP2] WP2::Decode() -> {}", message);
                    return status;
                }

                copyRgba64ToFloat(dest, temp, m_decode_scale);
                return status;
            }

            DecodeTargetBitmap target(dest, header.width, header.height, header.format);

            WP2::ArgbBuffer buffer(WP2_RGBA_32);

            WP2Status s;
            s = buffer.SetExternal(target.width, target.height, target.image, target.stride, false);
            if (s != WP2_STATUS_OK)
            {
                const char* message = WP2GetStatusMessage(s);
                status.setError("[ImageDecoder.WP2] WP2::ArgbBuffer::SetExternal() -> {}", message);
                return status;
            }

            WP2::DecoderConfig config = WP2::DecoderConfig::kDefault;
            config.thread_level = options.multithread ? std::thread::hardware_concurrency() : 0;

            s = WP2::Decode(m_memory.address, m_memory.size, &buffer, config);
            if (s != WP2_STATUS_OK)
            {
                const char* message = WP2GetStatusMessage(s);
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

        WP2Status s;

        if (surface.format.isFloat())
        {
            TemporaryBitmap temp(surface, Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16));
            const float scale = 65535.0f;
            copyFloatToRgba64(temp, surface, scale);

            WP2::ArgbBuffer buffer(WP2_RGBA_64);
            s = buffer.SetExternal(temp.width, temp.height,
                reinterpret_cast<uint16_t*>(temp.image), temp.stride / 2, false);
            if (s != WP2_STATUS_OK)
            {
                const char* message = WP2GetStatusMessage(s);
                status.setError("[ImageEncoder.WP2] WP2::ArgbBuffer::SetExternal() -> {}", message);
                return status;
            }

            WP2::EncoderConfig config;
            config.effort = std::clamp(options.compression * 3 / 4, 0, 6);
            config.quality = std::clamp(options.quality, 0.0f, 1.0f) * 100.0f;
            config.thread_level = options.multithread ? std::thread::hardware_concurrency() : 0;
            config.transfer_function = WP2::WP2_TF_LINEAR;

            if (options.lossless)
                config.quality = 100.0f;

            WP2::MemoryWriter writer;
            s = WP2::Encode(buffer, &writer, config);
            if (s != WP2_STATUS_OK)
            {
                const char* message = WP2GetStatusMessage(s);
                status.setError("[ImageEncoder.WP2] WP2::Encode() -> {}", message);
                return status;
            }

            stream.write(writer.mem_, writer.size_);
            return status;
        }

        Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        TemporaryBitmap temp(surface, format);

        WP2::ArgbBuffer buffer(WP2_RGBA_32);

        s = buffer.SetExternal(temp.width, temp.height, temp.image, temp.stride, false);
        if (s != WP2_STATUS_OK)
        {
            const char* message = WP2GetStatusMessage(s);
            status.setError("[ImageEncoder.WP2] WP2::ArgbBuffer::SetExternal() -> {}", message);
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
            const char* message = WP2GetStatusMessage(s);
            status.setError("[ImageEncoder.WP2] WP2::Encodel() -> {}", message);
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

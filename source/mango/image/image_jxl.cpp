/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_JXL) && defined(MANGO_ENABLE_JXL_THREADS)

#include "jxl/decode.h"
#include "jxl/decode_cxx.h"
#include "jxl/encode.h"
#include "jxl/encode_cxx.h"
#include "jxl/resizable_parallel_runner.h"
#include "jxl/resizable_parallel_runner_cxx.h"

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    static bool hasAlphaChannel(const JxlBasicInfo& info)
    {
        return info.alpha_bits != 0 || info.num_extra_channels > 0;
    }

    static bool isFloatingPoint(const JxlBasicInfo& info)
    {
        return info.exponent_bits_per_sample != 0;
    }

    // libjxl float input/output is scene-linear. Scan the peak RGB value so HDR
    // images get a sensible intensity_target in the codestream metadata.
    static float scanPeakRGB(const Surface& surface)
    {
        const Format& format = surface.format;
        float peak = 0.0f;

        for (int y = 0; y < surface.height; ++y)
        {
            const u8* scan = surface.address<u8>(0, y);
            const int pixel_bytes = format.bytes();

            for (int x = 0; x < surface.width; ++x)
            {
                const u8* pixel = scan + x * pixel_bytes;

                for (int c = 0; c < 3; ++c)
                {
                    if (!format.size[c])
                        continue;

                    const float* sample = reinterpret_cast<const float*>(pixel + format.offset[c] / 8);
                    peak = std::max(peak, *sample);
                }
            }
        }

        return peak;
    }

    struct Interface : ImageDecodeInterface
    {
        JxlDecoderPtr m_decoder;
        JxlResizableParallelRunnerPtr m_runner;

        Surface m_surface;
        Buffer m_buffer;
        Buffer m_icc;

        JxlBasicInfo m_info {};
        JxlPixelFormat m_pixel_format {};
        size_t m_bytes_per_pixel = 0;

        bool m_is_parsed = false;
        ImageDecodeStatus m_status;

        Interface(ConstMemory memory)
            : m_decoder(JxlDecoderMake(nullptr))
            , m_runner(JxlResizableParallelRunnerMake(nullptr))
        {
            JxlDecoder* decoder = m_decoder.get();
            void* runner = m_runner.get();

            if (JxlDecoderSubscribeEvents(decoder, JXL_DEC_BASIC_INFO |
                                                   JXL_DEC_COLOR_ENCODING |
                                                   JXL_DEC_FULL_IMAGE) != JXL_DEC_SUCCESS)
            {
                header.setError("JxlDecoderSubscribeEvents : FAILED");
                return;
            }

            if (JxlDecoderSetParallelRunner(decoder, JxlResizableParallelRunner, m_runner.get()) != JXL_DEC_SUCCESS)
            {
                header.setError("JxlDecoderSetParallelRunner : FAILED");
                return;
            }

            JxlDecoderSetInput(decoder, memory.address, memory.size);

            for (;;)
            {
                JxlDecoderStatus status = JxlDecoderProcessInput(decoder);
                switch (status)
                {
                    case JXL_DEC_ERROR:
                        header.setError("JxlDecoderProcessInput : JXL_DEC_ERROR");
                        return;

                    case JXL_DEC_NEED_MORE_INPUT:
                        header.setError("JxlDecoderProcessInput : JXL_DEC_NEED_MORE_INPUT");
                        return;

                    case JXL_DEC_BASIC_INFO:
                    {
                        if (JxlDecoderGetBasicInfo(decoder, &m_info) != JXL_DEC_SUCCESS)
                        {
                            header.setError("JxlDecoderGetBasicInfo : FAILED");
                            return;
                        }

                        uint32_t n = JxlResizableParallelRunnerSuggestThreads(m_info.xsize, m_info.ysize);
                        JxlResizableParallelRunnerSetThreads(runner, n);

                        header.width = m_info.xsize;
                        header.height = m_info.ysize;
                        break;
                    }

                    case JXL_DEC_COLOR_ENCODING:
                    {
                        readColorEncoding();
                        return;
                    }

                    default:
                        header.setError("JxlDecoderProcessInput : ERROR");
                        return;
                }
            }
        }

        void readColorEncoding()
        {
            JxlDecoder* decoder = m_decoder.get();
            ColorInfo& color = header.color;

            // Map the structured color encoding of the decoded pixels (preferred for
            // identifying the nominal color space, including HDR PQ/HLG).
            JxlColorEncoding enc;
            if (JxlDecoderGetColorAsEncodedProfile(decoder, JXL_COLOR_PROFILE_TARGET_DATA, &enc) == JXL_DEC_SUCCESS)
            {
                switch (enc.transfer_function)
                {
                    case JXL_TRANSFER_FUNCTION_709:    color.transfer = TransferFunction::BT709; break;
                    case JXL_TRANSFER_FUNCTION_LINEAR: color.transfer = TransferFunction::Linear; break;
                    case JXL_TRANSFER_FUNCTION_SRGB:   color.transfer = TransferFunction::sRGB; break;
                    case JXL_TRANSFER_FUNCTION_PQ:     color.transfer = TransferFunction::PQ; break;
                    case JXL_TRANSFER_FUNCTION_HLG:    color.transfer = TransferFunction::HLG; break;
                    case JXL_TRANSFER_FUNCTION_GAMMA:
                        color.gamma = float(enc.gamma);
                        color.transfer = TransferFunction::Unspecified;
                        break;
                    default:
                        color.transfer = TransferFunction::Unspecified;
                        break;
                }

                if (enc.color_space == JXL_COLOR_SPACE_RGB)
                {
                    // libjxl fills the numerical chromaticities regardless of the enum, so
                    // forward the exact coordinates alongside the named primaries.
                    color.has_chromaticities = true;
                    color.white = { float(enc.white_point_xy[0]),    float(enc.white_point_xy[1]) };
                    color.red   = { float(enc.primaries_red_xy[0]),   float(enc.primaries_red_xy[1]) };
                    color.green = { float(enc.primaries_green_xy[0]), float(enc.primaries_green_xy[1]) };
                    color.blue  = { float(enc.primaries_blue_xy[0]),  float(enc.primaries_blue_xy[1]) };

                    switch (enc.primaries)
                    {
                        case JXL_PRIMARIES_SRGB: color.primaries = ColorPrimaries::BT709; break;
                        case JXL_PRIMARIES_2100: color.primaries = ColorPrimaries::BT2020; break;
                        case JXL_PRIMARIES_P3:
                            color.primaries = (enc.white_point == JXL_WHITE_POINT_DCI)
                                ? ColorPrimaries::DCI_P3 : ColorPrimaries::DisplayP3;
                            break;
                        default:
                            color.primaries = identifyPrimaries(color.white, color.red, color.green, color.blue);
                            break;
                    }
                }
            }

            configureDecodeOutput();

            // libjxl returns an ICC profile for DATA even for plain sRGB, but the decoded
            // pixels are already in the correct output encoding (uint8 = gamma-encoded,
            // float = scene-linear). Forwarding ICC makes imageview apply a second CMS
            // transform and blows out SDR images.
            icc = ConstMemory();
            m_icc.reset();

            const bool needs_icc = !isFloatingPoint(m_info) && (
                color.transfer == TransferFunction::PQ ||
                color.transfer == TransferFunction::HLG ||
                color.transfer == TransferFunction::Unspecified ||
                (color.primaries != ColorPrimaries::BT709 &&
                 color.primaries != ColorPrimaries::Unspecified));

            if (needs_icc)
            {
                size_t icc_size = 0;
                if (JxlDecoderGetICCProfileSize(decoder, JXL_COLOR_PROFILE_TARGET_DATA, &icc_size) == JXL_DEC_SUCCESS && icc_size)
                {
                    m_icc.resize(icc_size);
                    if (JxlDecoderGetColorAsICCProfile(decoder, JXL_COLOR_PROFILE_TARGET_DATA, m_icc.data(), m_icc.size()) == JXL_DEC_SUCCESS)
                    {
                        icc = m_icc;
                    }
                    else
                    {
                        m_icc.reset();
                    }
                }
            }
        }

        void configureDecodeOutput()
        {
            const bool fp = isFloatingPoint(m_info);

            if (fp)
            {
                const bool has_alpha = hasAlphaChannel(m_info);
                const uint32_t channels = has_alpha ? 4u : 3u;

                header.format = has_alpha
                    ? Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32, Format::LINEAR)
                    : Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32, 0, Format::LINEAR);
                m_pixel_format = { channels, JXL_TYPE_FLOAT, JXL_NATIVE_ENDIAN, 0 };

                // libjxl always returns scene-linear pixels for float output.
                header.linear = true;
            }
            else if (m_info.bits_per_sample > 8)
            {
                header.format = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
                m_pixel_format = { 4, JXL_TYPE_UINT16, JXL_NATIVE_ENDIAN, 0 };
                header.linear = false;
            }
            else
            {
                // Match PNG convention: always 8-bit RGBA UNORM for the mango pipeline.
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                m_pixel_format = { 4, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0 };
                header.linear = false;
            }

            m_bytes_per_pixel = header.format.bytes();
        }

        ~Interface()
        {
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            if (!m_is_parsed)
            {
                m_is_parsed = true;
                parse();
            }

            if (m_surface.image)
            {
                dest.blit(0, 0, m_surface);
            }

            return m_status;
        }

        void parse()
        {
            JxlDecoder* decoder = m_decoder.get();

            for (;;)
            {
                JxlDecoderStatus jxstat = JxlDecoderProcessInput(decoder);
                switch (jxstat)
                {
                    case JXL_DEC_ERROR:
                        m_status.setError("JxlDecoderProcessInput : JXL_DEC_ERROR");
                        return;

                    case JXL_DEC_NEED_MORE_INPUT:
                        m_status.setError("JxlDecoderProcessInput : JXL_DEC_NEED_MORE_INPUT");
                        return;

                    case JXL_DEC_BASIC_INFO:
                        break;

                    case JXL_DEC_COLOR_ENCODING:
                        break;

                    case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
                    {
                        size_t bytes;

                        if (JxlDecoderImageOutBufferSize(decoder, &m_pixel_format, &bytes) != JXL_DEC_SUCCESS)
                        {
                            m_status.setError("JxlDecoderImageOutBufferSize : FAILED");
                            return;
                        }

                        if (bytes != header.width * header.height * m_bytes_per_pixel)
                        {
                            m_status.setError("Incorrect buffer size request.");
                            return;
                        }

                        m_buffer.resize(bytes);

                        if (JxlDecoderSetImageOutBuffer(decoder, &m_pixel_format, m_buffer.data(), m_buffer.size()) != JXL_DEC_SUCCESS)
                        {
                            m_status.setError("JxlDecoderSetImageOutBuffer : FAILED");
                            return;
                        }

                        break;
                    }

                    case JXL_DEC_FULL_IMAGE:
                    {
                        break;
                    }

                    case JXL_DEC_SUCCESS:
                    {
                        m_surface = Surface(header.width, header.height, header.format,
                                            header.width * m_bytes_per_pixel, m_buffer.data());
                        return;
                    }

                    default:
                    {
                        m_status.setError("JxlDecoderProcessInput : ERROR");
                        return;
                    }
                }
            }
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

    struct JxlEncodeConfig
    {
        Format temp_format;
        JxlPixelFormat pixel_format;
        JxlBasicInfo basic_info;
    };

    JxlEncodeConfig makeEncodeConfig(const Surface& surface, bool lossless)
    {
        const bool has_alpha = surface.format.isAlpha();
        const bool use_float = surface.format.isFloat();

        JxlEncodeConfig config;
        JxlEncoderInitBasicInfo(&config.basic_info);

        config.basic_info.xsize = surface.width;
        config.basic_info.ysize = surface.height;
        config.basic_info.uses_original_profile = lossless ? JXL_TRUE : JXL_FALSE;

        if (use_float)
        {
            config.temp_format = has_alpha
                ? Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32)
                : Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32);
            config.pixel_format = { has_alpha ? 4u : 3u, JXL_TYPE_FLOAT, JXL_NATIVE_ENDIAN, 0 };
            config.basic_info.bits_per_sample = 32;
            config.basic_info.exponent_bits_per_sample = 8;

            if (has_alpha)
            {
                config.basic_info.num_extra_channels = 1;
                config.basic_info.alpha_bits = 32;
                config.basic_info.alpha_exponent_bits = 8;
                config.basic_info.alpha_premultiplied = surface.format.isPreMultiplied() ? JXL_TRUE : JXL_FALSE;
            }
        }
        else
        {
            int sample_bits = 8;
            for (int i = 0; i < 4; ++i)
            {
                sample_bits = std::max(sample_bits, int(surface.format.size[i]));
            }

            if (sample_bits > 16)
            {
                sample_bits = 16;
            }

            if (sample_bits > 8)
            {
                config.temp_format = has_alpha
                    ? Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16)
                    : Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0);
                config.pixel_format = { has_alpha ? 4u : 3u, JXL_TYPE_UINT16, JXL_NATIVE_ENDIAN, 0 };
                config.basic_info.bits_per_sample = 16;
            }
            else
            {
                config.temp_format = has_alpha
                    ? Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8)
                    : Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0);
                config.pixel_format = { has_alpha ? 4u : 3u, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0 };
            }

            config.basic_info.exponent_bits_per_sample = 0;

            if (has_alpha)
            {
                config.basic_info.num_extra_channels = 1;
                config.basic_info.alpha_bits = sample_bits;
                config.basic_info.alpha_exponent_bits = 0;
                config.basic_info.alpha_premultiplied = surface.format.isPreMultiplied() ? JXL_TRUE : JXL_FALSE;
            }
        }

        return config;
    }

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status;

        auto enc = JxlEncoderMake(nullptr);
        auto runner = JxlResizableParallelRunnerMake(nullptr);

        if (JxlEncoderSetParallelRunner(enc.get(), JxlResizableParallelRunner, runner.get()) != JXL_ENC_SUCCESS)
        {
            status.setError("JxlEncoderSetParallelRunner : FAILED");
            return status;
        }

        JxlEncodeConfig config = makeEncodeConfig(surface, options.lossless);

        Bitmap temp(surface, config.temp_format);

        // Float encode assumes Rec.709 scene-linear input. Surface has no ColorInfo, so
        // callers must linearize and convert wide-gamut sources beforehand (or pass
        // options.icc for the original profile). Format::LINEAR only marks scene-linear
        // float; integer input is assumed gamma-encoded sRGB.
        if (surface.format.isFloat() && !surface.format.isLinear())
        {
            ColorInfo color;
            color.transfer = TransferFunction::sRGB;
            color.primaries = ColorPrimaries::BT709;

            LinearizeOptions linearize_options;
            linearize_options.preserve_gamut = true;
            linearize(temp, temp, color, linearize_options);
        }

        if (config.pixel_format.data_type == JXL_TYPE_FLOAT)
        {
            const float peak = scanPeakRGB(temp);
            config.basic_info.intensity_target = peak > 1.01f ? peak * 80.0f : 255.0f;
        }

        if (JxlEncoderSetBasicInfo(enc.get(), &config.basic_info) != JXL_ENC_SUCCESS)
        {
            status.setError("JxlEncoderSetBasicInfo : FAILED");
            return status;
        }

        const bool is_gray = config.pixel_format.num_channels < 3;

        if (options.icc.size > 0)
        {
            if (JxlEncoderSetICCProfile(enc.get(), options.icc.address, options.icc.size) != JXL_ENC_SUCCESS)
            {
                status.setError("JxlEncoderSetICCProfile : FAILED");
                return status;
            }
        }
        else
        {
            JxlColorEncoding color_encoding = {};

            if (config.pixel_format.data_type == JXL_TYPE_FLOAT)
            {
                // Rec.709 primaries, linear transfer (libjxl "linear sRGB").
                JxlColorEncodingSetToLinearSRGB(&color_encoding, is_gray);
            }
            else
            {
                JxlColorEncodingSetToSRGB(&color_encoding, is_gray);
            }

            if (JxlEncoderSetColorEncoding(enc.get(), &color_encoding) != JXL_ENC_SUCCESS)
            {
                status.setError("JxlEncoderSetColorEncoding : FAILED");
                return status;
            }
        }

        size_t image_pixels = temp.width * temp.height;
        size_t image_bytes = image_pixels * temp.format.bytes();

        JxlEncoderFrameSettings* frame_settings = JxlEncoderFrameSettingsCreate(enc.get(), nullptr);

        const int effort = std::clamp(options.compression, 1, 10);
        if (JxlEncoderFrameSettingsSetOption(frame_settings, JXL_ENC_FRAME_SETTING_EFFORT, effort) != JXL_ENC_SUCCESS)
        {
            status.setError("JxlEncoderFrameSettingsSetOption(EFFORT) : FAILED");
            return status;
        }

        if (options.lossless)
        {
            if (JxlEncoderSetFrameLossless(frame_settings, JXL_TRUE) != JXL_ENC_SUCCESS)
            {
                status.setError("JxlEncoderSetFrameLossless : FAILED");
                return status;
            }
        }
        else
        {
            if (JxlEncoderSetFrameLossless(frame_settings, JXL_FALSE) != JXL_ENC_SUCCESS)
            {
                status.setError("JxlEncoderSetFrameLossless : FAILED");
                return status;
            }

            const float quality = std::clamp(options.quality, 0.0f, 1.0f) * 100.0f;
            const float distance = JxlEncoderDistanceFromQuality(quality);

            if (JxlEncoderSetFrameDistance(frame_settings, distance) != JXL_ENC_SUCCESS)
            {
                status.setError("JxlEncoderSetFrameDistance : FAILED");
                return status;
            }
        }

        if (JxlEncoderAddImageFrame(frame_settings, &config.pixel_format,
                                    reinterpret_cast<void*> (temp.image),
                                    image_bytes) != JXL_ENC_SUCCESS)
        {
            status.setError("JxlEncoderAddImageFrame : FAILED");
            return status;
        }

        JxlEncoderCloseInput(enc.get());

        Buffer compressed(1024 + image_pixels / 16);

        u8* next_out = compressed.data();
        size_t avail_out = compressed.size();

        JxlEncoderStatus result = JXL_ENC_NEED_MORE_OUTPUT;
        while (result == JXL_ENC_NEED_MORE_OUTPUT)
        {
            result = JxlEncoderProcessOutput(enc.get(), &next_out, &avail_out);
            if (result == JXL_ENC_NEED_MORE_OUTPUT)
            {
                stream.write(compressed.data(), compressed.size() - avail_out);
                next_out = compressed.data();
                avail_out = compressed.size();
            }
        }

        if (result != JXL_ENC_SUCCESS)
        {
            status.setError("JxlEncoderProcessOutput : FAILED");
            return status;
        }

        stream.write(compressed.data(), compressed.size() - avail_out);

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecJXL()
    {
        registerImageDecoder(createInterface, ".jxl");
        registerImageEncoder(imageEncode, ".jxl");
    }

} // namespace mango::image

#else

namespace mango::image
{

    void registerImageCodecJXL()
    {
    }

} // namespace mango::image

#endif // defined(MANGO_ENABLE_JXL) && defined(MANGO_ENABLE_JXL_THREADS)

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_JXL)

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

    struct Interface : ImageDecoderInterface
    {
        JxlDecoderPtr m_decoder;
        JxlResizableParallelRunnerPtr m_runner;

        ImageHeader m_header;
        Surface m_surface;
        Buffer m_buffer;
        Buffer m_icc;

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
                m_header.setError("JxlDecoderSubscribeEvents : FAILED");
                return;
            }

            if (JxlDecoderSetParallelRunner(decoder, JxlResizableParallelRunner, m_runner.get()) != JXL_DEC_SUCCESS)
            {
                m_header.setError("JxlDecoderSetParallelRunner : FAILED");
                return;
            }

            JxlDecoderSetInput(decoder, memory.address, memory.size);

            JxlBasicInfo info;

            for (;;)
            {
                JxlDecoderStatus status = JxlDecoderProcessInput(decoder);
                switch (status)
                {
                    case JXL_DEC_ERROR:
                        m_header.setError("JxlDecoderProcessInput : JXL_DEC_ERROR");
                        return;

                    case JXL_DEC_NEED_MORE_INPUT:
                        m_header.setError("JxlDecoderProcessInput : JXL_DEC_NEED_MORE_INPUT");
                        return;

                    case JXL_DEC_BASIC_INFO:
                    {
                        if (JxlDecoderGetBasicInfo(decoder, &info) != JXL_DEC_SUCCESS)
                        {
                            m_header.setError("JxlDecoderGetBasicInfo : FAILED");
                            return;
                        }

                        uint32_t n = JxlResizableParallelRunnerSuggestThreads(info.xsize, info.ysize);
                        JxlResizableParallelRunnerSetThreads(runner, n);

                        m_header.width = info.xsize;
                        m_header.height = info.ysize;
                        m_header.format = Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32);

                        return;
                    }

                    default:
                        m_header.setError("JxlDecoderProcessInput : ERROR");
                        return;
                }
            }
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ConstMemory icc() override
        {
            return m_icc;
        }

        ConstMemory exif() override
        {
            return ConstMemory();
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!m_surface.image)
            {
                status = parse();
            }

            if (m_surface.image)
            {
                dest.blit(0, 0, m_surface);
            }

            return status;
        }

        ImageDecodeStatus parse()
        {
            JxlDecoder* decoder = m_decoder.get();

            JxlPixelFormat format = { 4, JXL_TYPE_FLOAT, JXL_NATIVE_ENDIAN, 0 };
            size_t bpp = 16;

            ImageDecodeStatus status;

            for (;;)
            {
                JxlDecoderStatus jxstat = JxlDecoderProcessInput(decoder);
                switch (jxstat)
                {
                    case JXL_DEC_ERROR:
                        status.setError("JxlDecoderProcessInput : JXL_DEC_ERROR");
                        return status;

                    case JXL_DEC_NEED_MORE_INPUT:
                        status.setError("JxlDecoderProcessInput : JXL_DEC_NEED_MORE_INPUT");
                        return status;

                    case JXL_DEC_BASIC_INFO:
                        break;

                    case JXL_DEC_COLOR_ENCODING:
                    {
                        size_t bytes;

                        if (JxlDecoderGetICCProfileSize(decoder, &format, JXL_COLOR_PROFILE_TARGET_DATA, &bytes) != JXL_DEC_SUCCESS)
                        {
                            // Silently fail
                            break;
                        }

                        m_icc.resize(bytes);

                        if (JxlDecoderGetColorAsICCProfile(decoder, &format, JXL_COLOR_PROFILE_TARGET_DATA, m_icc.data(), m_icc.size()) != JXL_DEC_SUCCESS)
                        {
                            // Silently fail
                            m_icc.reset();
                            break;
                        }

                        break;
                    }

                    case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
                    {
                        size_t bytes;

                        if (JxlDecoderImageOutBufferSize(decoder, &format, &bytes) != JXL_DEC_SUCCESS)
                        {
                            status.setError("JxlDecoderImageOutBufferSize : FAILED");
                            return status;
                        }

                        if (bytes != m_header.width * m_header.height * bpp)
                        {
                            status.setError("Incorrect buffer size request.");
                            return status;
                        }

                        m_buffer.resize(bytes);

                        if (JxlDecoderSetImageOutBuffer(decoder, &format, m_buffer.data(), m_buffer.size()) != JXL_DEC_SUCCESS)
                        {
                            status.setError("JxlDecoderSetImageOutBuffer : FAILED");
                            return status;
                        }

                        break;
                    }

                    case JXL_DEC_FULL_IMAGE:
                        break;

                    case JXL_DEC_SUCCESS:
                    {
                        m_surface = Surface(m_header.width, m_header.height, m_header.format, m_header.width * 16, m_buffer.data());
                        return status;
                    }

                    default:
                        status.setError("JxlDecoderProcessInput : ERROR");
                        return status;
                }
            }
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

        auto enc = JxlEncoderMake(nullptr);
        auto runner = JxlResizableParallelRunnerMake(nullptr);

        if (JxlEncoderSetParallelRunner(enc.get(), JxlResizableParallelRunner, runner.get()) != JXL_ENC_SUCCESS)
        {
            status.setError("JxlEncoderSetParallelRunner : FAILED");
            return status;
        }

#if 1
        Bitmap temp(surface, Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32));
        JxlPixelFormat pixel_format = { 3, JXL_TYPE_FLOAT, JXL_NATIVE_ENDIAN, 0 };
#else
        // TODO: doesn't work, fix
        Bitmap temp(surface, Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32));
        JxlPixelFormat pixel_format = { 4, JXL_TYPE_FLOAT, JXL_NATIVE_ENDIAN, 0 };
#endif

        JxlBasicInfo basic_info;
        JxlEncoderInitBasicInfo(&basic_info);

        basic_info.xsize = surface.width;
        basic_info.ysize = surface.height;
        basic_info.bits_per_sample = 32;
        basic_info.exponent_bits_per_sample = 8;
        basic_info.uses_original_profile = JXL_FALSE;

        if (JxlEncoderSetBasicInfo(enc.get(), &basic_info) != JXL_ENC_SUCCESS)
        {
            status.setError("JxlEncoderSetBasicInfo : FAILED");
            return status;
        }

        JxlColorEncoding color_encoding = {};
        //JxlColorEncodingSetToSRGB(&color_encoding, pixel_format.num_channels < 3);
        //JxlColorEncodingSetToSRGB(&color_encoding, true);
        JxlColorEncodingSetToSRGB(&color_encoding, false);

        if (JxlEncoderSetColorEncoding(enc.get(), &color_encoding) != JXL_ENC_SUCCESS)
        {
            status.setError("JxlEncoderSetColorEncoding : FAILED");
            return status;
        }

        size_t image_pixels = temp.width * temp.height;
        size_t image_bytes = image_pixels * temp.format.bytes();

        JxlEncoderFrameSettings* frame_settings = JxlEncoderFrameSettingsCreate(enc.get(), nullptr);

        if (JxlEncoderAddImageFrame(frame_settings, &pixel_format,
                                    reinterpret_cast<void*> (temp.image),
                                    image_bytes) != JXL_ENC_SUCCESS)
        {
            status.setError("JxlEncoderAddImageFrame : FAILED");
            return status;
        }

        JxlEncoderCloseInput(enc.get());

        Buffer compressed(image_pixels);

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

    void registerImageDecoderJXL()
    {
        registerImageDecoder(createInterface, ".jxl");
        registerImageEncoder(imageEncode, ".jxl");
    }

} // namespace mango::image

#endif

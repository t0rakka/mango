/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_JXL)

#include "jxl/decode.h"
#include "jxl/decode_cxx.h"
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
                m_header.setError("JxlDecoderSubscribeEvents : FAILED.");
                return;
            }

            if (JxlDecoderSetParallelRunner(decoder, 
                JxlResizableParallelRunner, m_runner.get()) != JXL_DEC_SUCCESS)
            {
                m_header.setError("JxlDecoderSetParallelRunner : FAILED.");
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
                            m_header.setError("JxlDecoderGetBasicInfo : FAILED.");
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
                        m_header.setError("JxlDecoderProcessInput : ERROR.");
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
                        status.setError("JxlDecoderProcessInput : ERROR.");
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

    /*
    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        // TODO
        ImageEncodeStatus status;
        return status;
    }
    */

} // namespace

namespace mango::image
{

    void registerImageDecoderJXL()
    {
        registerImageDecoder(createInterface, ".jxl");
    }

} // namespace mango::image

#endif

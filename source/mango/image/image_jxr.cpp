/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_JXR)

#ifndef __ANSI__
#define __ANSI__
#endif

#include <JXRGlue.h>

#undef max
#undef min

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // helpers
    // ------------------------------------------------------------

    static
    ImageDecodeStatus makeError(const char* message)
    {
        ImageDecodeStatus status;
        status.setError(message);
        return status;
    }

    static
    ImageEncodeStatus makeEncodeError(const char* message)
    {
        ImageEncodeStatus status;
        status.setError(message);
        return status;
    }

    static
    bool check(ERR err, const char* context, ImageDecodeStatus& status)
    {
        if (err == WMP_errSuccess)
            return true;

        status.setError("[JXR] {} failed ({})", context, int(err));
        return false;
    }

    static
    bool check(ERR err, const char* context, ImageEncodeStatus& status)
    {
        if (err == WMP_errSuccess)
            return true;

        status.setError("[JXR] {} failed ({})", context, int(err));
        return false;
    }

    static
    u32 computeStride(const PKPixelInfo& pi, u32 width)
    {
        if (pi.bdBitDepth == BD_1)
            return (pi.cbitUnit * width + 7) >> 3;

        return (((pi.cbitUnit + 7) >> 3) * width);
    }

    static
    bool isFloatDepth(BITDEPTH_BITS depth)
    {
        return depth == BD_16F || depth == BD_32F;
    }

    static
    bool isEqualFormat(const PKPixelFormatGUID& a, const PKPixelFormatGUID& b)
    {
        return IsEqualGUID(a, b) != 0;
    }

    static
    Format mangoFormatForOutput(const PKPixelFormatGUID& guid)
    {
        if (isEqualFormat(guid, GUID_PKPixelFormat128bppRGBAFloat))
            return Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32);
        if (isEqualFormat(guid, GUID_PKPixelFormat128bppRGBFloat))
            return Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32, 0);
        if (isEqualFormat(guid, GUID_PKPixelFormat128bppRGBAFixedPoint))
            return Format(128, Format::UNORM, Format::RGBA, 32, 32, 32, 32);
        if (isEqualFormat(guid, GUID_PKPixelFormat96bppRGBFixedPoint))
            return Format(96, Format::UNORM, Format::RGB, 32, 32, 32, 0);
        if (isEqualFormat(guid, GUID_PKPixelFormat64bppRGBA))
            return Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
        if (isEqualFormat(guid, GUID_PKPixelFormat64bppRGBAFixedPoint))
            return Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
        if (isEqualFormat(guid, GUID_PKPixelFormat64bppRGBAHalf))
            return Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16);
        if (isEqualFormat(guid, GUID_PKPixelFormat48bppRGB))
            return Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0);
        if (isEqualFormat(guid, GUID_PKPixelFormat48bppRGBFixedPoint))
            return Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0);
        if (isEqualFormat(guid, GUID_PKPixelFormat48bppRGBHalf))
            return Format(48, Format::FLOAT16, Format::RGB, 16, 16, 16, 0);
        if (isEqualFormat(guid, GUID_PKPixelFormat32bppRGBA))
            return Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        if (isEqualFormat(guid, GUID_PKPixelFormat32bppBGRA))
            return Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
        if (isEqualFormat(guid, GUID_PKPixelFormat32bppRGB))
            return Format(32, Format::UNORM, Format::RGB, 8, 8, 8, 0);
        if (isEqualFormat(guid, GUID_PKPixelFormat32bppBGR))
            return Format(32, Format::UNORM, Format::BGR, 8, 8, 8, 0);
        if (isEqualFormat(guid, GUID_PKPixelFormat24bppRGB))
            return Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0);
        if (isEqualFormat(guid, GUID_PKPixelFormat24bppBGR))
            return Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0);
        if (isEqualFormat(guid, GUID_PKPixelFormat32bppGrayFloat))
            return Format(32, Format::FLOAT32, Format::R, 32, 0, 0, 0, Format::LUMINANCE);
        if (isEqualFormat(guid, GUID_PKPixelFormat32bppGrayFixedPoint))
            return Format(32, Format::UNORM, Format::R, 32, 0, 0, 0, Format::LUMINANCE);
        if (isEqualFormat(guid, GUID_PKPixelFormat16bppGrayHalf))
            return Format(16, Format::FLOAT16, Format::R, 16, 0, 0, 0, Format::LUMINANCE);
        if (isEqualFormat(guid, GUID_PKPixelFormat16bppGrayFixedPoint))
            return Format(16, Format::UNORM, Format::R, 16, 0, 0, 0, Format::LUMINANCE);
        if (isEqualFormat(guid, GUID_PKPixelFormat16bppGray))
            return Format(16, Format::UNORM, Format::R, 16, 0, 0, 0, Format::LUMINANCE);
        if (isEqualFormat(guid, GUID_PKPixelFormat8bppGray))
            return Format(8, Format::UNORM, Format::R, 8, 0, 0, 0, Format::LUMINANCE);
        if (isEqualFormat(guid, GUID_PKPixelFormatBlackWhite))
            return Format(1, Format::UNORM, Format::R, 1, 0, 0, 0, Format::LUMINANCE);

        return Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
    }

    static
    void setHeaderColor(ImageHeader& header, const PKPixelInfo& source, const PKPixelFormatGUID& output)
    {
        ColorInfo& color = header.color;

        if (isFloatDepth(source.bdBitDepth) || isEqualFormat(output, GUID_PKPixelFormat128bppRGBAFloat) ||
            isEqualFormat(output, GUID_PKPixelFormat128bppRGBFloat))
        {
            color.transfer = TransferFunction::Linear;
            color.primaries = ColorPrimaries::BT709;
            return;
        }

        if (source.cfColorFormat == Y_ONLY)
        {
            color.transfer = TransferFunction::sRGB;
            color.primaries = ColorPrimaries::Unspecified;
            return;
        }

        color.transfer = TransferFunction::sRGB;
        color.primaries = ColorPrimaries::BT709;
    }

    // ------------------------------------------------------------
    // custom WMPStream -> mango::Stream (growable, zero-filled holes)
    // ------------------------------------------------------------
    //
    // libjxr encoders reserve container space with SetPos/Write pairs and
    // explicit zero padding (see JXRGlueJxr.c). Holes between the current
    // size and a later Write must read as zero, matching FILE semantics.

    struct OutputContext
    {
        Buffer buffer;
        Stream* output = nullptr;
        size_t offset = 0;
        size_t size = 0;
    };

    static
    void growBuffer(OutputContext* ctx, size_t required)
    {
        const size_t current = ctx->buffer.size();
        if (required <= current)
            return;

        ctx->buffer.resize(required);
        std::memset(ctx->buffer.data() + current, 0, required - current);
    }

    static
    ERR CloseOutputStream(WMPStream** ppWS)
    {
        if (ppWS && *ppWS)
        {
            auto* ctx = reinterpret_cast<OutputContext*>((*ppWS)->state.pvObj);
            if (ctx->output && ctx->size > 0)
                ctx->output->write(ctx->buffer.data(), ctx->size);

            delete ctx;
            delete *ppWS;
            *ppWS = nullptr;
        }
        return WMP_errSuccess;
    }

    static
    Bool EOSOutputStream(WMPStream* pWS)
    {
        auto* ctx = reinterpret_cast<OutputContext*>(pWS->state.pvObj);
        return ctx->offset >= ctx->size;
    }

    static
    ERR ReadOutputStream(WMPStream* pWS, void* pv, size_t cb)
    {
        auto* ctx = reinterpret_cast<OutputContext*>(pWS->state.pvObj);
        if (ctx->offset >= ctx->size)
            return WMP_errFail;

        size_t remain = ctx->size - ctx->offset;
        if (cb > remain)
            cb = remain;

        std::memcpy(pv, ctx->buffer.data() + ctx->offset, cb);
        ctx->offset += cb;
        return WMP_errSuccess;
    }

    static
    ERR WriteOutputStream(WMPStream* pWS, const void* pv, size_t cb)
    {
        auto* ctx = reinterpret_cast<OutputContext*>(pWS->state.pvObj);
        if (!cb)
            return WMP_errSuccess;

        const size_t end = ctx->offset + cb;
        if (end < ctx->offset)
            return WMP_errFail;

        growBuffer(ctx, end);

        if (ctx->offset > ctx->size)
            std::memset(ctx->buffer.data() + ctx->size, 0, ctx->offset - ctx->size);

        std::memcpy(ctx->buffer.data() + ctx->offset, pv, cb);
        ctx->offset += cb;
        if (ctx->offset > ctx->size)
            ctx->size = ctx->offset;

        return WMP_errSuccess;
    }

    static
    ERR SetPosOutputStream(WMPStream* pWS, size_t offPos)
    {
        auto* ctx = reinterpret_cast<OutputContext*>(pWS->state.pvObj);
        ctx->offset = offPos;
        return WMP_errSuccess;
    }

    static
    ERR GetPosOutputStream(WMPStream* pWS, size_t* poffPos)
    {
        auto* ctx = reinterpret_cast<OutputContext*>(pWS->state.pvObj);
        *poffPos = ctx->offset;
        return WMP_errSuccess;
    }

    static
    ERR CreateOutputStream(WMPStream** ppWS, Stream* output)
    {
        auto* ctx = new OutputContext();
        ctx->output = output;
        auto* pWS = new WMPStream {};

        pWS->state.pvObj = ctx;
        pWS->fMem = !FALSE;
        pWS->Close = CloseOutputStream;
        pWS->EOS = EOSOutputStream;
        pWS->Read = ReadOutputStream;
        pWS->Write = WriteOutputStream;
        pWS->SetPos = SetPosOutputStream;
        pWS->GetPos = GetPosOutputStream;

        *ppWS = pWS;
        return WMP_errSuccess;
    }

    // ------------------------------------------------------------
    // custom WMPStream <- ConstMemoryStream (decode)
    // ------------------------------------------------------------
    //
    // libjxr bitstream IO expects CreateWS_File read semantics: fread must
    // return the full request or fail. CreateWS_Memory returns success on
    // partial EOF reads, which leaves stale data in the bitstream buffer.

    struct InputContext
    {
        ConstMemoryStream stream;

        InputContext(ConstMemory memory)
            : stream(memory)
        {
        }
    };

    static
    ERR CloseInputStream(WMPStream** ppWS)
    {
        if (ppWS && *ppWS)
        {
            delete reinterpret_cast<InputContext*>((*ppWS)->state.pvObj);
            delete *ppWS;
            *ppWS = nullptr;
        }
        return WMP_errSuccess;
    }

    static
    Bool EOSInputStream(WMPStream* pWS)
    {
        auto* ctx = reinterpret_cast<InputContext*>(pWS->state.pvObj);
        return ctx->stream.offset() >= ctx->stream.size();
    }

    static
    ERR ReadInputStream(WMPStream* pWS, void* pv, size_t cb)
    {
        auto* ctx = reinterpret_cast<InputContext*>(pWS->state.pvObj);
        if (!cb)
            return WMP_errSuccess;

        const u64 bytes = ctx->stream.read(pv, u64(cb));
        return bytes == cb ? WMP_errSuccess : WMP_errFileIO;
    }

    static
    ERR WriteInputStream(WMPStream* pWS, const void* pv, size_t cb)
    {
        MANGO_UNREFERENCED(pWS);
        MANGO_UNREFERENCED(pv);
        MANGO_UNREFERENCED(cb);
        return WMP_errFileIO;
    }

    static
    ERR SetPosInputStream(WMPStream* pWS, size_t offPos)
    {
        auto* ctx = reinterpret_cast<InputContext*>(pWS->state.pvObj);
        ctx->stream.seek(s64(offPos), Stream::SeekMode::Begin);
        return WMP_errSuccess;
    }

    static
    ERR GetPosInputStream(WMPStream* pWS, size_t* poffPos)
    {
        auto* ctx = reinterpret_cast<InputContext*>(pWS->state.pvObj);
        *poffPos = size_t(ctx->stream.offset());
        return WMP_errSuccess;
    }

    static
    ERR CreateInputStream(WMPStream** ppWS, ConstMemory memory)
    {
        auto* ctx = new InputContext(memory);
        auto* pWS = new WMPStream {};

        pWS->state.pvObj = ctx;
        pWS->Close = CloseInputStream;
        pWS->EOS = EOSInputStream;
        pWS->Read = ReadInputStream;
        pWS->Write = WriteInputStream;
        pWS->SetPos = SetPosInputStream;
        pWS->GetPos = GetPosInputStream;

        *ppWS = pWS;
        return WMP_errSuccess;
    }

    // ------------------------------------------------------------
    // encoder quality tables (from JxrEncApp.c)
    // ------------------------------------------------------------

    static int DPK_QPS_420[11][6] =
    {
        { 66, 65, 70, 72, 72, 77 },
        { 59, 58, 63, 64, 63, 68 },
        { 52, 51, 57, 56, 56, 61 },
        { 48, 48, 54, 51, 50, 55 },
        { 43, 44, 48, 46, 46, 49 },
        { 37, 37, 42, 38, 38, 43 },
        { 26, 28, 31, 27, 28, 31 },
        { 16, 17, 22, 16, 17, 21 },
        { 10, 11, 13, 10, 10, 13 },
        {  5,  5,  6,  5,  5,  6 },
        {  2,  2,  3,  2,  2,  2 }
    };

    static int DPK_QPS_8[12][6] =
    {
        { 67, 79, 86, 72, 90, 98 },
        { 59, 74, 80, 64, 83, 89 },
        { 53, 68, 75, 57, 76, 83 },
        { 49, 64, 71, 53, 70, 77 },
        { 45, 60, 67, 48, 67, 74 },
        { 40, 56, 62, 42, 59, 66 },
        { 33, 49, 55, 35, 51, 58 },
        { 27, 44, 49, 28, 45, 50 },
        { 20, 36, 42, 20, 38, 44 },
        { 13, 27, 34, 13, 28, 34 },
        {  7, 17, 21,  8, 17, 21 },
        {  2,  5,  6,  2,  5,  6 }
    };

    static int DPK_QPS_16[11][6] =
    {
        { 197, 203, 210, 202, 207, 213 },
        { 174, 188, 193, 180, 189, 196 },
        { 152, 167, 173, 156, 169, 174 },
        { 135, 152, 157, 137, 153, 158 },
        { 119, 137, 141, 119, 138, 142 },
        { 102, 120, 125, 100, 120, 124 },
        {  82,  98, 104,  79,  98, 103 },
        {  60,  76,  81,  58,  76,  81 },
        {  39,  52,  58,  36,  52,  58 },
        {  16,  27,  33,  14,  27,  33 },
        {   5,   8,   9,   4,   7,   8 }
    };

    static int DPK_QPS_16f[11][6] =
    {
        { 148, 177, 171, 165, 187, 191 },
        { 133, 155, 153, 147, 172, 181 },
        { 114, 133, 138, 130, 157, 167 },
        {  97, 118, 120, 109, 137, 144 },
        {  76,  98, 103,  85, 115, 121 },
        {  63,  86,  91,  62,  96,  99 },
        {  46,  68,  71,  43,  73,  75 },
        {  29,  48,  52,  27,  48,  51 },
        {  16,  30,  35,  14,  29,  34 },
        {   8,  14,  17,   7,  13,  17 },
        {   3,   5,   7,   3,   5,   6 }
    };

    static int DPK_QPS_32f[11][6] =
    {
        { 194, 206, 209, 204, 211, 217 },
        { 175, 187, 196, 186, 193, 205 },
        { 157, 170, 177, 167, 180, 190 },
        { 133, 152, 156, 144, 163, 168 },
        { 116, 138, 142, 117, 143, 148 },
        {  98, 120, 123,  96, 123, 126 },
        {  80,  99, 102,  78,  99, 102 },
        {  65,  79,  84,  63,  79,  84 },
        {  48,  61,  67,  45,  60,  66 },
        {  27,  41,  46,  24,  40,  45 },
        {   3,  22,  24,   2,  21,  22 }
    };

    static
    void configureQuality(CWMIStrCodecParam& scp, const PKPixelInfo& pi, float quality, bool lossless, int width)
    {
        std::memset(&scp, 0, sizeof(scp));
        scp.bfBitstreamFormat = FREQUENCY;
        scp.bProgressiveMode = FALSE;

        if (lossless || quality >= 1.0f)
        {
            scp.olOverlap = OL_NONE;
            scp.cfColorFormat = YUV_444;
            scp.uiDefaultQPIndex = 1;
            scp.uiDefaultQPIndexU = 1;
            scp.uiDefaultQPIndexV = 1;
            scp.uiDefaultQPIndexYHP = 1;
            scp.uiDefaultQPIndexUHP = 1;
            scp.uiDefaultQPIndexVHP = 1;
            return;
        }

        float q = std::clamp(quality, 0.0f, 0.999f);

        scp.olOverlap = (q >= 0.5f || width < 2 * MB_WIDTH_PIXEL) ? OL_ONE : OL_TWO;
        scp.cfColorFormat = (q >= 0.5f || pi.uBitsPerSample > 8) ? YUV_444 : YUV_420;

        if (pi.bdBitDepth == BD_1)
        {
            scp.uiDefaultQPIndex = U8(8 - 5.0f * q + 0.5f);
            return;
        }

        if (q > 0.8f && pi.bdBitDepth == BD_8 &&
            scp.cfColorFormat != YUV_420 && scp.cfColorFormat != YUV_422)
        {
            q = 0.8f + (q - 0.8f) * 1.5f;
        }

        int qi = int(10.f * q);
        float qf = 10.f * q - float(qi);

        int* pQPs =
            (scp.cfColorFormat == YUV_420 || scp.cfColorFormat == YUV_422) ? DPK_QPS_420[qi] :
            (pi.bdBitDepth == BD_8 ? DPK_QPS_8[qi] :
            (pi.bdBitDepth == BD_16 ? DPK_QPS_16[qi] :
            (pi.bdBitDepth == BD_16F ? DPK_QPS_16f[qi] :
            DPK_QPS_32f[qi])));

        scp.uiDefaultQPIndex   = U8(0.5f + float(pQPs[0]) * (1.f - qf) + float((pQPs + 6)[0]) * qf);
        scp.uiDefaultQPIndexU  = U8(0.5f + float(pQPs[1]) * (1.f - qf) + float((pQPs + 6)[1]) * qf);
        scp.uiDefaultQPIndexV  = U8(0.5f + float(pQPs[2]) * (1.f - qf) + float((pQPs + 6)[2]) * qf);
        scp.uiDefaultQPIndexYHP = U8(0.5f + float(pQPs[3]) * (1.f - qf) + float((pQPs + 6)[3]) * qf);
        scp.uiDefaultQPIndexUHP = U8(0.5f + float(pQPs[4]) * (1.f - qf) + float((pQPs + 6)[4]) * qf);
        scp.uiDefaultQPIndexVHP = U8(0.5f + float(pQPs[5]) * (1.f - qf) + float((pQPs + 6)[5]) * qf);
    }

    struct JxrEncodeConfig
    {
        Format temp_format;
        PKPixelFormatGUID pixel_format;
        CWMIStrCodecParam scp;
    };

    static
    JxrEncodeConfig makeEncodeConfig(const Surface& surface, const ImageEncodeOptions& options)
    {
        const bool has_alpha = surface.format.isAlpha();
        const bool use_float = surface.format.isFloat();
        const bool lossless = options.lossless;

        JxrEncodeConfig config;
        std::memset(&config.scp, 0, sizeof(config.scp));

        if (use_float)
        {
            config.temp_format = has_alpha
                ? Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32)
                : Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32, 0);
            config.pixel_format = has_alpha ? GUID_PKPixelFormat128bppRGBAFloat : GUID_PKPixelFormat128bppRGBFloat;
        }
        else
        {
            int sample_bits = 8;
            for (int i = 0; i < 4; ++i)
                sample_bits = std::max(sample_bits, int(surface.format.size[i]));

            if (sample_bits > 16)
                sample_bits = 16;

            if (sample_bits > 8)
            {
                config.temp_format = has_alpha
                    ? Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16)
                    : Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0);
                config.pixel_format = has_alpha ? GUID_PKPixelFormat64bppRGBA : GUID_PKPixelFormat48bppRGB;
            }
            else
            {
                config.temp_format = has_alpha
                    ? Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8)
                    : Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0);
                config.pixel_format = has_alpha ? GUID_PKPixelFormat32bppRGBA : GUID_PKPixelFormat24bppRGB;
            }
        }

        PKPixelInfo pi;
        pi.pGUIDPixFmt = &config.pixel_format;
        PixelFormatLookup(&pi, LOOKUP_FORWARD);

        if (has_alpha && config.scp.uAlphaMode == 0)
            config.scp.uAlphaMode = 2;

        configureQuality(config.scp, pi, options.quality, lossless, surface.width);
        return config;
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        Buffer m_memory;
        WMPStream* m_input_stream = nullptr;
        PKCodecFactory* m_codec_factory = nullptr;
        PKImageDecode* m_decoder = nullptr;
        PKFormatConverter* m_converter = nullptr;

        PKPixelFormatGUID m_output_guid = GUID_PKPixelFormatDontCare;
        Format m_output_format;

        Interface(ConstMemory memory)
        {
            async = false;

            if (memory.size < 4 || memory.address[0] != 'I' || memory.address[1] != 'I' ||
                memory.address[2] != 0xbc || memory.address[3] != 0x01)
            {
                header.setError("Incorrect JPEG XR signature.");
                return;
            }

            m_memory.resize(memory.size);
            std::memcpy(m_memory.data(), memory.address, memory.size);

            ImageDecodeStatus status;
            if (!initialize(status))
                header.setError(status.info);
        }

        ~Interface()
        {
            if (m_converter)
                m_converter->Release(&m_converter);
            if (m_decoder)
                m_decoder->Release(&m_decoder);
            if (m_input_stream)
                CloseInputStream(&m_input_stream);
            if (m_codec_factory)
                m_codec_factory->Release(&m_codec_factory);
        }

        bool initialize(ImageDecodeStatus& status)
        {
            if (!check(PKCreateCodecFactory(&m_codec_factory, WMP_SDK_VERSION), "PKCreateCodecFactory", status))
                return false;

            ConstMemory input = m_memory;
            if (!check(CreateInputStream(&m_input_stream, input), "CreateInputStream", status))
                return false;

            if (!check(m_codec_factory->CreateCodec(&IID_PKImageWmpDecode, (void**)&m_decoder), "CreateCodec", status))
                return false;

            if (!check(m_decoder->Initialize(m_decoder, m_input_stream), "Initialize", status))
                return false;

            PKPixelFormatGUID source_guid;
            if (!check(m_decoder->GetPixelFormat(m_decoder, &source_guid), "GetPixelFormat", status))
                return false;

            PKPixelInfo source_info;
            source_info.pGUIDPixFmt = &source_guid;
            if (!check(PixelFormatLookup(&source_info, LOOKUP_FORWARD), "PixelFormatLookup", status))
                return false;

            PKPixelInfo output_info;
            output_info.pGUIDPixFmt = &source_guid;
            if (!check(PixelFormatLookup(&output_info, LOOKUP_FORWARD), "PixelFormatLookup", status))
                return false;
            if (!check(PixelFormatLookup(&output_info, LOOKUP_BACKWARD_TIF), "PixelFormatLookup", status))
                return false;

            m_output_guid = *output_info.pGUIDPixFmt;
            output_info.pGUIDPixFmt = &m_output_guid;
            if (!check(PixelFormatLookup(&output_info, LOOKUP_FORWARD), "PixelFormatLookup", status))
                return false;

            m_output_format = mangoFormatForOutput(m_output_guid);

            if (!check(m_codec_factory->CreateFormatConverter(&m_converter), "CreateFormatConverter", status))
                return false;

            if (!configureDecoder(status))
                return false;

            if (!check(m_converter->Initialize(m_converter, m_decoder, nullptr, m_output_guid),
                "Initialize", status))
                return false;

            I32 width = 0;
            I32 height = 0;
            if (!check(m_converter->GetSize(m_converter, &width, &height), "GetSize", status))
                return false;

            header.width = width;
            header.height = height;
            header.format = m_output_format;
            setHeaderColor(header, source_info, m_output_guid);
            return true;
        }

        bool configureDecoder(ImageDecodeStatus& status)
        {
            PKPixelInfo output_info;
            output_info.pGUIDPixFmt = &m_output_guid;

            if (isEqualFormat(m_output_guid, GUID_PKPixelFormat8bppGray) ||
                isEqualFormat(m_output_guid, GUID_PKPixelFormat16bppGray))
            {
                m_decoder->guidPixFormat = m_output_guid;
                m_decoder->WMP.wmiI.cfColorFormat = Y_ONLY;
            }
            else if (isEqualFormat(m_output_guid, GUID_PKPixelFormat24bppRGB) &&
                     m_decoder->WMP.wmiI.cfColorFormat == CMYK)
            {
                m_decoder->WMP.wmiI.cfColorFormat = CF_RGB;
                m_decoder->guidPixFormat = m_output_guid;
                m_decoder->WMP.wmiI.bRGB = TRUE;
            }

            if (!check(PixelFormatLookup(&output_info, LOOKUP_FORWARD), "PixelFormatLookup", status))
                return false;

            U8 alpha_mode = 0;
            if (!!(output_info.grBit & PK_pixfmtHasAlpha))
                alpha_mode = 2;

            m_decoder->WMP.wmiSCP.bfBitstreamFormat = SPATIAL;
            m_decoder->WMP.wmiSCP.uAlphaMode = alpha_mode;
            m_decoder->WMP.wmiSCP.sbSubband = SB_ALL;
            m_decoder->WMP.bIgnoreOverlap = FALSE;

            m_decoder->WMP.wmiI.cfColorFormat = output_info.cfColorFormat;
            m_decoder->WMP.wmiI.bdBitDepth = output_info.bdBitDepth;
            m_decoder->WMP.wmiI.cBitsPerUnit = output_info.cbitUnit;

            m_decoder->WMP.wmiI.cThumbnailWidth = m_decoder->WMP.wmiI.cWidth;
            m_decoder->WMP.wmiI.cThumbnailHeight = m_decoder->WMP.wmiI.cHeight;
            m_decoder->WMP.wmiI.bSkipFlexbits = FALSE;

            m_decoder->WMP.wmiI.cROILeftX = 0;
            m_decoder->WMP.wmiI.cROITopY = 0;
            m_decoder->WMP.wmiI.cROIWidth = m_decoder->WMP.wmiI.cThumbnailWidth;
            m_decoder->WMP.wmiI.cROIHeight = m_decoder->WMP.wmiI.cThumbnailHeight;
            m_decoder->WMP.wmiI.oOrientation = O_NONE;
            m_decoder->WMP.wmiI.cPostProcStrength = 0;

            return true;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!header)
                return status;

            if (!m_converter)
            {
                status.setError("JPEG XR decoder is not initialized.");
                return status;
            }

            PKPixelInfo output_info;
            output_info.pGUIDPixFmt = &m_output_guid;
            if (!check(PixelFormatLookup(&output_info, LOOKUP_FORWARD), "PixelFormatLookup", status))
                return status;

            const u32 stride = computeStride(output_info, u32(header.width));
            const size_t bytes = size_t(stride) * header.height;

            u8* pixels = nullptr;
            if (!check(PKAllocAligned((void**)&pixels, bytes, 128), "PKAllocAligned", status))
                return status;

            PKRect rect = { 0, 0,
                (I32)m_decoder->WMP.wmiI.cROIWidth,
                (I32)m_decoder->WMP.wmiI.cROIHeight };

            if (!check(m_converter->Copy(m_converter, &rect, pixels, stride), "Copy", status))
            {
                PKFreeAligned((void**)&pixels);
                return status;
            }

            Surface temp(header.width, header.height, m_output_format, stride, pixels);
            dest.blit(0, 0, temp);

            PKFreeAligned((void**)&pixels);

            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        return new Interface(memory);
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status;

        if (surface.width <= 0 || surface.height <= 0)
        {
            status.setError("[JXR] Invalid surface dimensions.");
            return status;
        }

        JxrEncodeConfig config = makeEncodeConfig(surface, options);

        TemporaryBitmap temp(surface, config.temp_format);
        const Surface& source = temp;

        PKCodecFactory* codec_factory = nullptr;
        WMPStream* output_stream = nullptr;
        PKImageEncode* encoder = nullptr;

        if (!check(PKCreateCodecFactory(&codec_factory, WMP_SDK_VERSION), "PKCreateCodecFactory", status))
            return status;

        if (!check(CreateOutputStream(&output_stream, &stream), "CreateOutputStream", status))
            goto cleanup;

        if (!check(codec_factory->CreateCodec(&IID_PKImageWmpEncode, (void**)&encoder), "CreateCodec", status))
            goto cleanup;

        if (!check(encoder->Initialize(encoder, output_stream, &config.scp, sizeof(config.scp)), "Initialize", status))
            goto cleanup;

        if (!check(encoder->SetPixelFormat(encoder, config.pixel_format), "SetPixelFormat", status))
            goto cleanup;

        if (!check(encoder->SetSize(encoder, source.width, source.height), "SetSize", status))
            goto cleanup;

        if (!check(encoder->WritePixels(encoder, source.height, source.image, source.stride), "WritePixels", status))
            goto cleanup;

        if (!check(encoder->Terminate(encoder), "Terminate", status))
            goto cleanup;

    cleanup:
        if (encoder)
            encoder->Release(&encoder);
        else if (output_stream)
            CloseOutputStream(&output_stream);
        if (codec_factory)
            codec_factory->Release(&codec_factory);

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecJXR()
    {
        registerImageDecoder(createInterface, ".jxr");
        registerImageDecoder(createInterface, ".wdp");
        registerImageEncoder(imageEncode, ".jxr");
    }

} // namespace mango::image

#else

namespace mango::image
{

    void registerImageCodecJXR()
    {
    }

} // namespace mango::image

#endif // defined(MANGO_ENABLE_JXR)

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <mango/core/core.hpp>
#include <mango/image/decoder.hpp>
#include <mango/image/surface.hpp>

namespace mango::image::jpeg
{

    // ------------------------------------------------------------------------
    // Lightweight public gateway into the JPEG parser / entropy decoder.
    // GPU ingest (e.g. hybrid Huffman→compute) registers a coefficient sink
    // here; mango::image::ImageDecoder remains the pixel Surface path.
    // ------------------------------------------------------------------------

    // Layout of each 8x8 coefficient block handed to the sink / used by IDCT.
    enum class CoeffOrder
    {
        // Spatial 8x8 (zigzag applied during Huffman). Matches mango CPU SIMD IDCT.
        Natural,

        // Zigzag / spectral index order (no zigzag scatter in Huffman).
        // Preferred for GPU dequant+IDCT compute.
        Spectral,
    };

    struct ComponentInfo
    {
        int id = 0;
        int hsf = 1;   // horizontal sampling factor
        int vsf = 1;   // vertical sampling factor
        int qt = 0;    // quantization table index (0..3)
    };

    struct JpegInfo
    {
        bool valid = false;
        std::string error;

        int width = 0;
        int height = 0;
        int precision = 8;     // 8 or 12
        int components = 0;    // 1..4

        bool baseline = false;
        bool progressive = false;
        bool lossless = false;
        bool arithmetic = false;
        bool differential = false;
        bool multiscan = false;
        bool ultrahdr = false; // ISO / Apple gain-map present (base+map)

        bool restart_intervals = false; // DRI
        bool mango1 = false;            // APP14:'Mango1' parallel offsets

        std::string encoding;           // e.g. "Baseline DCT"
        std::string compression;        // "Huffman" / "Arithmetic"
        std::string chroma;             // e.g. "4:2:0"

        int xblock = 0;  // MCU width in samples
        int yblock = 0;  // MCU height in samples
        int xmcu = 0;
        int ymcu = 0;
        int blocks_in_mcu = 0;

        ComponentInfo component[4] {};

        // Quantization tables (64 s16 each). Indexing matches CoeffOrder:
        // Natural  → spatial 8x8 (mango CPU IDCT)
        // Spectral → zigzag/file order (GPU compute)
        // Pointers remain valid for the lifetime of JpegDecoder.
        const s16* quant[4] = {};
        int quant_bits[4] = {}; // 8 or 16; 0 = table unused

        CoeffOrder coeff_order = CoeffOrder::Natural;
    };

    // One finished entropy work unit (MCU row band). Ownership of 'data' is
    // transferred to the recipient; destroying the unique_ptr returns the
    // storage to the decoder's recycle pool (no new/delete on the hot path).
    struct CoeffBand
    {
        // Custom deleter returns storage to the decoder recycle pool.
        std::unique_ptr<s16, std::function<void(s16*)>> data;
        size_t samples = 0;   // number of s16 values (MCU rows * xmcu * blocks_in_mcu * 64)

        int mcu_y0 = 0;       // first MCU row (inclusive)
        int mcu_y1 = 0;       // last MCU row (exclusive)
        int index = 0;        // band index in decode order
        bool last = false;

        s16* ptr() const { return data.get(); }
        explicit operator bool() const { return data != nullptr; }
    };

    using CoeffSink = std::function<void(CoeffBand&& band)>;

    class JpegDecoder : private NonCopyable
    {
    public:
        explicit JpegDecoder(ConstMemory memory);
        ~JpegDecoder();

        // Parsed header / capability snapshot. Safe after construction.
        const JpegInfo& info() const;

        // Select coefficient layout for decodeCoefficients() and for exported QT.
        // decodePixels() always uses Natural internally for the SIMD IDCT path.
        void setCoeffOrder(CoeffOrder order);
        CoeffOrder coeffOrder() const;

        // Register coefficient handoff. Required for decodeCoefficients().
        void setCoeffSink(CoeffSink sink);

        // Full CPU decode into a Surface (existing ImageDecoder behaviour).
        ImageDecodeStatus decodePixels(const Surface& dest,
            const ImageDecodeOptions& options = ImageDecodeOptions());

        // Entropy only → CoeffSink bands. Skips IDCT/color. Uses setCoeffOrder().
        // Progressive: one band with the full coefficient buffer after the last scan.
        // Sequential: one band per MCU-row strip (and at DRI / Mango1 boundaries).
        ImageDecodeStatus decodeCoefficients(
            const ImageDecodeOptions& options = ImageDecodeOptions());

        // Optional cancellation (same flag ImageDecodeInterface uses).
        void cancel();
        bool cancelled() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m;
    };

} // namespace mango::image::jpeg

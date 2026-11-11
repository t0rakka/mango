/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

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

    // Non-owning view of one finished entropy segment inside setCoeffStorage().
    // The decoder does not read or write these samples after the callback returns.
    // The receiver must not free data. It stays valid until the caller frees the storage.
    struct CoeffView
    {
        const s16* data = nullptr;
        size_t samples = 0;   // s16 count (MCU rows * xmcu * blocks_in_mcu * 64)
        int mcu_y0 = 0;       // first MCU row (inclusive)
        int mcu_y1 = 0;       // last MCU row (exclusive)
        int index = 0;        // completion order, not MCU order
        bool last = false;
    };

    using CoeffSink = void (*)(const CoeffView& view, void* user);

    // JPEG DHT table in file order (BITS[1..16] + HUFFVAL). Enough to rebuild
    // a GPU Huffman decoder. Valid for the lifetime of the EntropyStream only
    // as a value copy — not a view into the file.
    struct HuffmanCodeTable
    {
        int cls = 0; // 0 = DC, 1 = AC
        int id = 0;  // Th
        u8 bits[16] {};
        u8 values[256] {};
        int nvalues = 0;
    };

    // One 8x8 block inside an MCU, in scan order.
    struct McuBlock
    {
        int component = 0; // frame index (DC predictor slot)
        int dc = 0;        // DC Huffman table id
        int ac = 0;        // AC Huffman table id
    };

    // One independent entropy interval. bytes views the caller's JPEG memory
    // (no copy). Mango1: rows_per_segment MCU rows, trailing RSTn removed.
    struct EntropySegment
    {
        ConstMemory bytes;
        int mcu_y0 = 0;          // inclusive MCU row
        int mcu_y1 = 0;          // exclusive
        int mcus = 0;            // full MCUs in this segment
        size_t packed_offset = 0; // byte offset in a packed upload (RSTs omitted)
    };

    // Huffman bitstream for a full-GPU decode. Segments are non-contiguous in
    // the file; packed_offset is where each one lands if copied back-to-back
    // into a device buffer. Views stay valid while the JPEG memory does.
    struct EntropyStream
    {
        bool ok = false;
        std::string error;

        std::vector<EntropySegment> segments;
        size_t packed_bytes = 0;

        std::vector<HuffmanCodeTable> huffman;
        McuBlock block[10] {};
        int blocks_in_mcu = 0;

        int width = 0;
        int height = 0;
        int xblock = 0;
        int yblock = 0;
        int xmcu = 0;
        int ymcu = 0;
        int rows_per_segment = 0; // MCU rows per segment (Mango1 interval, DRI / xmcu, or ymcu)
    };

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
        // sink may be null. user is passed through unchanged.
        void setCoeffSink(CoeffSink sink, void* user);

        // Full-frame destination for decodeCoefficients(). Required.
        // Spectral workers write their MCU strips here. Progressive scans accumulate here.
        // samples is ymcu * xmcu * blocks_in_mcu * 64. Not used by decodePixels().
        void setCoeffStorage(s16* data, size_t samples);

        // Full CPU decode into a Surface (existing ImageDecoder behaviour).
        ImageDecodeStatus decodePixels(const Surface& dest,
            const ImageDecodeOptions& options = ImageDecodeOptions());

        // Entropy only → CoeffView callbacks. Skips IDCT/color. Uses setCoeffOrder().
        // Progressive: one band with the full coefficient buffer after the last scan.
        // Sequential: one band per MCU-row strip (and at DRI / Mango1 boundaries).
        ImageDecodeStatus decodeCoefficients(
            const ImageDecodeOptions& options = ImageDecodeOptions());

        // Binds DHT + SOS, then returns entropy views. Does not Huffman-decode.
        // Mango1 uses the stored offsets, one view per interval, trailing RSTn stripped.
        // DRI walks the scan with seekMarker (the CPU decoder's marker scan) and
        // requires intervals that end on an MCU row.
        // No restarts: one view of the whole scan, up to the next marker.
        // Segment bytes alias the memory passed to the constructor.
        EntropyStream extractEntropy();

        // Optional cancellation (same flag ImageDecodeInterface uses).
        void cancel();
        bool cancelled() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m;
    };

} // namespace mango::image::jpeg

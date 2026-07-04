/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>

namespace mango::image::jpeg
{

    // ----------------------------------------------------------------------------
    // Specifications
    // ----------------------------------------------------------------------------

    // https://www.w3.org/Graphics/JPEG/itu-t81.pdf

    // ----------------------------------------------------------------------------
    // markers
    // ----------------------------------------------------------------------------

    enum
    {
        // Start of Frame markers, non-differential, Huffman coding
        MARKER_SOF0     = 0xffc0,  // Baseline DCT
        MARKER_SOF1     = 0xffc1,  // Extended sequential DCT
        MARKER_SOF2     = 0xffc2,  // Progressive DCT
        MARKER_SOF3     = 0xffc3,  // Lossless (sequential)

        // Huffman table specification
        MARKER_DHT      = 0xffc4,  // Define Huffman table(s)

        // Start of Frame markers, differential, Huffman coding
        MARKER_SOF5     = 0xffc5,  // Differential sequential DCT
        MARKER_SOF6     = 0xffc6,  // Differential progressive DCT
        MARKER_SOF7     = 0xffc7,  // Differential lossless (sequential)

        // Start of Frame markers, non-differential, arithmetic coding
        MARKER_JPG      = 0xffc8,  // Reserved for JPEG extensions
        MARKER_SOF9     = 0xffc9,  // Extended sequential DCT
        MARKER_SOF10    = 0xffca,  // Progressive DCT
        MARKER_SOF11    = 0xffcb,  // Lossless (sequential)

        // Arithmetic coding conditioning specification
        MARKER_DAC      = 0xffcc,  // Define arithmetic coding conditioning(s)

        // Start of Frame markers, differential, arithmetic coding
        MARKER_SOF13    = 0xffcd,  // Differential sequential DCT
        MARKER_SOF14    = 0xffce,  // Differential progressive DCT
        MARKER_SOF15    = 0xffcf,  // Differential lossless (sequential)

        // Restart interval termination
        MARKER_RST0     = 0xffd0,  // Restart with modulo 8 count 0
        MARKER_RST1     = 0xffd1,  // Restart with modulo 8 count 1
        MARKER_RST2     = 0xffd2,  // Restart with modulo 8 count 2
        MARKER_RST3     = 0xffd3,  // Restart with modulo 8 count 3
        MARKER_RST4     = 0xffd4,  // Restart with modulo 8 count 4
        MARKER_RST5     = 0xffd5,  // Restart with modulo 8 count 5
        MARKER_RST6     = 0xffd6,  // Restart with modulo 8 count 6
        MARKER_RST7     = 0xffd7,  // Restart with modulo 8 count 7

        // Other markers
        MARKER_SOI      = 0xffd8,  // Start of image
        MARKER_EOI      = 0xffd9,  // End of image
        MARKER_SOS      = 0xffda,  // Start of scan
        MARKER_DQT      = 0xffdb,  // Define quantization table(s)
        MARKER_DNL      = 0xffdc,  // Define number of lines
        MARKER_DRI      = 0xffdd,  // Define restart interval
        MARKER_DHP      = 0xffde,  // Define hierarchical progression
        MARKER_EXP      = 0xffdf,  // Expand reference component(s)
        MARKER_APP0     = 0xffe0,  // Reserved for application segments
        MARKER_APP1     = 0xffe1,  // Reserved for application segments
        MARKER_APP2     = 0xffe2,  // Reserved for application segments
        MARKER_APP3     = 0xffe3,  // Reserved for application segments
        MARKER_APP4     = 0xffe4,  // Reserved for application segments
        MARKER_APP5     = 0xffe5,  // Reserved for application segments
        MARKER_APP6     = 0xffe6,  // Reserved for application segments
        MARKER_APP7     = 0xffe7,  // Reserved for application segments
        MARKER_APP8     = 0xffe8,  // Reserved for application segments
        MARKER_APP9     = 0xffe9,  // Reserved for application segments
        MARKER_APP10    = 0xffea,  // Reserved for application segments
        MARKER_APP11    = 0xffeb,  // Reserved for application segments
        MARKER_APP12    = 0xffec,  // Reserved for application segments
        MARKER_APP13    = 0xffed,  // Reserved for application segments
        MARKER_APP14    = 0xffee,  // Reserved for application segments
        MARKER_APP15    = 0xffef,  // Reserved for application segments
        MARKER_JPG0     = 0xfff0,  // Reserved for JPEG extensions
        MARKER_JPG1     = 0xfff1,  // Reserved for JPEG extensions
        MARKER_JPG2     = 0xfff2,  // Reserved for JPEG extensions
        MARKER_JPG3     = 0xfff3,  // Reserved for JPEG extensions
        MARKER_JPG4     = 0xfff4,  // Reserved for JPEG extensions
        MARKER_JPG5     = 0xfff5,  // Reserved for JPEG extensions
        MARKER_JPG6     = 0xfff6,  // Reserved for JPEG extensions
        MARKER_JPG7     = 0xfff7,  // Reserved for JPEG extensions
        MARKER_JPG8     = 0xfff8,  // Reserved for JPEG extensions
        MARKER_JPG9     = 0xfff9,  // Reserved for JPEG extensions
        MARKER_JPG10    = 0xfffa,  // Reserved for JPEG extensions
        MARKER_JPG11    = 0xfffb,  // Reserved for JPEG extensions
        MARKER_JPG12    = 0xfffc,  // Reserved for JPEG extensions
        MARKER_JPG13    = 0xfffd,  // Reserved for JPEG extensions
        MARKER_COM      = 0xfffe,  // Comment

        // Reserved markers
        MARKER_TEM      = 0xff01,  // For temporary private use in arithmetic coding
        MARKER_RES      = 0xff02   // Reserved (0x02 .. 0xbf)
    };

    // ----------------------------------------------------------------------------
    // types
    // ----------------------------------------------------------------------------

#ifdef MANGO_CPU_64BIT

    using HuffmanType = u64;

    #define JPEG_REGISTER_BITS  64
    #define bextr mango::u64_extract_bits

#else

    using HuffmanType = u32;

    #define JPEG_REGISTER_BITS  32
    #define bextr mango::u32_extract_bits

#endif

    //#define JPEG_ENABLE_IGJ_HUFFMAN

    static constexpr int JPEG_MAX_BLOCKS_IN_MCU  = 10;  // Maximum # of blocks per MCU in the JPEG specification
    static constexpr int JPEG_MAX_SAMPLES_IN_MCU = 64 * JPEG_MAX_BLOCKS_IN_MCU;
    static constexpr int JPEG_MAX_COMPS_IN_SCAN  = 4;   // JPEG limit on # of components in one scan
    static constexpr int JPEG_NUM_ARITH_TBLS     = 16;  // Arith-coding tables are numbered 0..15
    static constexpr int JPEG_DC_STAT_BINS       = 64;  // ...
    static constexpr int JPEG_AC_STAT_BINS       = 256; // ...
    static constexpr int JPEG_HUFF_LOOKUP_BITS   = 9;   // Huffman look-ahead table log2 size
    static constexpr int JPEG_HUFF_LOOKUP_SIZE   = (1 << JPEG_HUFF_LOOKUP_BITS);

    static
    constexpr u8 zigzagTable [] =
    {
         0,  1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63,
    };

    // supported external data formats (encode from, decode to)
    enum class SampleType
    {
        U8_Y,
        U8_BGR,
        U8_RGB,
        U8_BGRA,
        U8_RGBA,
    };

    enum class ColorSpace
    {
        CMYK = 0,
        YCBCR = 1,
        YCCK = 2
    };

    struct SampleFormat
    {
        SampleType sample;
        Format format;
    };

    struct QuantTable
    {
        s16* table;  // Quantization table
        int  bits;   // Quantization table precision (8 or 16 bits)
    };

    struct BitBuffer
    {
        const u8* ptr;
        const u8* end;

        HuffmanType data;
        int remain;

        void restart();
        void fill();

        void ensure()
        {
            if (remain < 16)
            {
                fill();
            }
        }

        int getBits(int nbits)
        {
            ensure();
            return int(bextr(data, remain -= nbits, nbits));
        }

        int peekBits(int nbits)
        {
            return int(bextr(data, remain - nbits, nbits));
        }

        int extend(int value, int nbits) const
        {
            return value - ((((value + value) >> nbits) - 1) & ((1 << nbits) - 1));
            //return value - int(bextr(((value + value) >> nbits) - 1, nbits, nbits));
        }

        int receive(int nbits)
        {
            int value = getBits(nbits);
            return extend(value, nbits);
        }
    };

    struct HuffmanTable
    {
        u8 size[17];
        u8 value[256];

        // acceleration tables
#if defined(JPEG_ENABLE_IGJ_HUFFMAN)
        int maxcode[18];
        int valoffset[18];
        int lookup[JPEG_HUFF_LOOKUP_SIZE];
#else
        HuffmanType maxcode[18];
        HuffmanType valueOffset[19];
        u8 lookupSize[JPEG_HUFF_LOOKUP_SIZE];
        u8 lookupValue[JPEG_HUFF_LOOKUP_SIZE];
#endif

        bool configure();
        int decode(BitBuffer& buffer) const;
    };

    struct HuffmanDecoder
    {
        int last_dc_value[JPEG_MAX_COMPS_IN_SCAN];
        int eob_run;

        HuffmanTable table[2][JPEG_MAX_COMPS_IN_SCAN];
        int maxTc = 0;
        int maxTh = 0;

        void restart();
    };

    struct ArithmeticDecoder
    {
        u32 c;
        u32 a;
        int ct;

        int last_dc_value[JPEG_MAX_COMPS_IN_SCAN]; // Last DC coef for each component
        int dc_context[JPEG_MAX_COMPS_IN_SCAN]; // Context index for DC conditioning

        u8 dc_L[JPEG_NUM_ARITH_TBLS]; // L values for DC arith-coding tables
        u8 dc_U[JPEG_NUM_ARITH_TBLS]; // U values for DC arith-coding tables
        u8 ac_K[JPEG_NUM_ARITH_TBLS]; // K values for AC arith-coding tables

        u8 dc_stats[JPEG_NUM_ARITH_TBLS][JPEG_DC_STAT_BINS];
        u8 ac_stats[JPEG_NUM_ARITH_TBLS][JPEG_AC_STAT_BINS];
        u8 fixed_bin[4]; // Statistics bin for coding with fixed probability 0.5

        ArithmeticDecoder();
        ~ArithmeticDecoder();

        void restart(BitBuffer& buffer);
    };

    struct Frame
    {
        int compid; // Component identifier
        int hsf;    // Horizontal sampling factor
        int vsf;    // Vertical sampling factor
        int tq;     // Quantization table destination selector
        int offset;
    };

    struct DecodeBlock
    {
        int offset;
        int pred;
        int dc;
        int ac;
    };

    struct DecodeState
    {
        BitBuffer buffer;
        HuffmanDecoder huffman;
        ArithmeticDecoder arithmetic;
        bool is_arithmetic = false;

        DecodeBlock block[JPEG_MAX_BLOCKS_IN_MCU];
        int blocks;
        int comps_in_scan;

        int spectral_start;
        int spectral_end;
        int successive_high;
        int successive_low;
        int precision = 8;

        void restart()
        {
            if (is_arithmetic)
            {
                buffer.restart();
                arithmetic.restart(buffer);
            }
            else
            {
                buffer.restart();
                huffman.restart();
            }
        }

        void (*decode)(s16* output, DecodeState* state);
    };

    struct Block
    {
        s16* qt;
    };

    struct ProcessState
    {
        // NOTE: this is just quantization tables
        Block block[JPEG_MAX_BLOCKS_IN_MCU];
        int blocks;

        Frame frame[JPEG_MAX_COMPS_IN_SCAN];
        int frames;

        ColorSpace colorspace = ColorSpace::CMYK; // default

        void (*idct) (u8* dest, const s16* data, const s16* qt);
        void (*process) (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    };

    // ----------------------------------------------------------------------------
    // BlitSink
    // ----------------------------------------------------------------------------
    //
    // BlitSink is the engine's output binding: it owns the working surface a
    // StreamDecoder writes MCUs into and knows how to deliver a finished band to
    // the caller (an optional blit when the working surface is not the target,
    // plus the progress callback). Decoupling this from the entropy/IDCT state is
    // what lets one stream target the caller surface while another targets a
    // private scratch buffer (e.g. an UltraHDR gain map).

    struct BlitSink
    {
        const Surface* target = nullptr;   // caller destination surface
        const Surface* surface = nullptr;  // working (MCU-aligned) surface; may alias target
        bool direct = true;                // when true, writes land directly in target
        ImageDecodeInterface* interface = nullptr;

        void finalize(const ImageDecodeRect& rect) const
        {
            if (!direct && surface != target)
            {
                Surface source(*surface, rect.x, rect.y, rect.width, rect.height);
                target->blit(rect.x, rect.y, source);
            }

            if (interface && interface->callback)
            {
                interface->callback(rect);
            }
        }
    };

    // ----------------------------------------------------------------------------
    // GainMapMetadata
    // ----------------------------------------------------------------------------
    //
    // ISO 21496-1 / Adobe "hdrgm" gain map parameters used to reconstruct an HDR
    // rendition from an SDR base image and a gain map. All boost/capacity values
    // live in the log2 domain. Arrays are per channel (R, G, B); single-channel
    // metadata is replicated into all three on parse. Defaults match the spec so
    // an absent field still produces a sane result.

    struct GainMapMetadata
    {
        float gainMapMin[3]   = { 0.0f, 0.0f, 0.0f };                       // log2
        float gainMapMax[3]   = { 1.0f, 1.0f, 1.0f };                       // log2
        float gainMapGamma[3] = { 1.0f, 1.0f, 1.0f };
        float offsetSdr[3]    = { 1.0f / 64.0f, 1.0f / 64.0f, 1.0f / 64.0f };
        float offsetHdr[3]    = { 1.0f / 64.0f, 1.0f / 64.0f, 1.0f / 64.0f };
        float hdrCapacityMin  = 0.0f;                                       // log2
        float hdrCapacityMax  = 1.0f;                                       // log2
        bool  baseRenditionIsHDR = false;
    };

    // Which gain map convention a file uses. The reconstruction math and the
    // color space the result lives in differ between them.
    enum class GainMapKind
    {
        None,
        ISO,    // ISO 21496-1 / Adobe "hdrgm" (Android UltraHDR). Output: Rec.709 linear.
        Apple,  // Apple HDRGainMap (urn:com:apple:photo:2020:aux:hdrgainmap). Output: Display-P3 linear.
    };

    // Collapse an SDR base image and its gain map into a scene-linear fp16 HDR
    // image. 'dest' must be FLOAT16 RGBA sized to the base image; 'base' and
    // 'gain' are UNORM RGBA8 (gain may be a smaller resolution and is sampled
    // bilinearly). The result is in the base image's own primaries (the decoder
    // signals these via ColorInfo; any conversion is left to the client).

    // ISO 21496-1 / Adobe per-channel log-boost reconstruction (display weight = 1).
    void applyGainMap(const Surface& dest, const Surface& base, const Surface& gain,
                      const GainMapMetadata& metadata, ImageDecodeInterface* interface);

    // Apple HDRGainMap reconstruction: linear blend toward 'headroom' (linear ratio).
    void applyAppleGainMap(const Surface& dest, const Surface& base, const Surface& gain,
                           float headroom, ImageDecodeInterface* interface);

    // ----------------------------------------------------------------------------
    // StreamDecoder
    // ----------------------------------------------------------------------------
    //
    // StreamDecoder owns ALL per-stream decoding state: the entropy decoder,
    // quantization / Huffman tables, frame geometry, restart information and the
    // scan byte range. It parses one JPEG bitstream and decodes it into a target
    // surface. Two of these can run side by side (e.g. an UltraHDR base image and
    // its gain map) under a single orchestrating Parser.

    class StreamDecoder
    {
    protected:
        ConstMemory m_memory;
        ImageDecodeInterface* m_interface = nullptr;
        QuantTable quantTable[JPEG_MAX_COMPS_IN_SCAN];

        AlignedStorage<s16> quantTableVector;
        AlignedStorage<s16> blockVector;

        int m_decode_interval;
        std::vector<u32> m_restart_offsets;

        std::vector<Frame> frames;
        Frame* scanFrame = nullptr; // current Progressive AC scan frame

        DecodeState decodeState;
        ProcessState processState;

        int restartInterval;
        int restartCounter;

        int m_hardware_concurrency;

        std::string m_encoding;
        std::string m_compression;
        std::string m_idct_name;
        std::string m_ycbcr_name;

        ImageDecodeStatus m_decode_status;

        BlitSink m_sink;                    // output binding (working surface + delivery)

        int m_aligned_width;
        int m_aligned_height;
        int m_width;
        int m_height;

        bool m_rgb_colorspace = false;
        bool m_relaxed_parser = false;
        Buffer m_source_icc;
        bool m_cmyk_store_mode = false;
        bool m_cmyk_icc_applied = false;

        ConstMemory getCmykIcc() const;

        int m_precision; // 8 or 12 bits
        int m_components; // 1..4

        bool is_baseline = true;
        bool is_progressive = false;
        bool is_multiscan = false;
        bool is_lossless = false;
        bool is_differential = false;

        int Hmax;
        int Vmax;
        int blocks_in_mcu;
        int xblock;
        int yblock;
        int xmcu;
        int ymcu;
        int mcus;

        bool isJPEG(ConstMemory memory) const;

        const u8* stepMarker(const u8* p, const u8* end) const;
        const u8* seekMarker(const u8* p, const u8* end) const;
        const u8* processSOS(const u8* p, const u8* end);

        void processSOI();
        void processEOI();
        void processCOM(const u8* p);
        void processTEM(const u8* p);
        void processRES(const u8* p);
        void processJPG(const u8* p);
        void processJPG(const u8* p, u16 marker);
        void processAPP(const u8* p, u16 marker);
        void processSOF(const u8* p, u16 marker);
        void processDQT(const u8* p);
        void processDNL(const u8* p);
        void processDRI(const u8* p);
        void processDHT(const u8* p, const u8* end);
        void processDAC(const u8* p);
        void processDHP(const u8* p);
        void processEXP(const u8* p);

        void parse(ConstMemory memory, bool decode);

        bool handleRestart();

        void decodeLossless();
        void decodeSequential();
        void decodeSequentialST();
        void decodeSequentialMT(int N);
        void decodeMultiScan();
        void decodeProgressive();
        void decodeProgressiveDC();
        void decodeProgressiveAC();
        void finishProgressive();

        void process_range(int y0, int y1, const s16* data);
        void process_and_clip(u8* dest, size_t stride, const s16* data, int width, int height);
        void blit_and_update(const ImageDecodeRect& rect);

        int getTaskSize(int count) const;
        void configureCPU(SampleType sample, const ImageDecodeOptions& options);
        std::string getInfo() const;

    public:
        ImageHeader header;
        ConstMemory exif_memory; // Exif block, if one is present
        ConstMemory scan_memory; // Scan block
        Buffer icc_buffer; // ICC color profile block, if one is present

        enum Flags : u32
        {
            RELAXED_PARSER = 0x0001,
        };

        StreamDecoder(ImageDecodeInterface* interface, ConstMemory memory, u32 flags = 0);
        ~StreamDecoder();

        void setMemory(ConstMemory memory);
        void setSourceIcc(ConstMemory icc);

        bool isLossless() const
        {
            return is_lossless;
        }

        bool cmykIccApplied() const
        {
            return m_cmyk_icc_applied;
        }

        ImageDecodeStatus decode(const Surface& target, const ImageDecodeOptions& options);
    };

    // ----------------------------------------------------------------------------
    // Parser
    // ----------------------------------------------------------------------------
    //
    // Parser is the orchestrator. Today it owns a single StreamDecoder and simply
    // forwards to it, preserving the public interface used by the image codec. It
    // is the seam where a second (gain map) stream and the fp16 join will attach.

    class Parser
    {
    protected:
        ImageDecodeInterface* m_interface = nullptr;
        StreamDecoder m_base;

        // UltraHDR: optional second stream (the gain map) decoded alongside the
        // base image and collapsed into a single fp16 HDR surface.
        GainMapKind m_gainmap_kind = GainMapKind::None;
        std::unique_ptr<StreamDecoder> m_gainmap;
        GainMapMetadata m_gainmap_meta;   // ISO path
        float m_apple_headroom = 1.0f;    // Apple path (linear ratio)

        void detectUltraHDR(ConstMemory memory);
        ImageDecodeStatus decodeUltraHDR(const Surface& target, const ImageDecodeOptions& options);

    public:
        ImageHeader& header;
        ConstMemory& exif_memory;
        Buffer& icc_buffer;

        enum Flags : u32
        {
            RELAXED_PARSER = StreamDecoder::RELAXED_PARSER,
        };

        Parser(ImageDecodeInterface* interface, ConstMemory memory, u32 flags = 0);
        ~Parser();

        void setMemory(ConstMemory memory);
        void setSourceIcc(ConstMemory icc);

        bool isLossless() const
        {
            return m_base.isLossless();
        }

        bool cmykIccApplied() const
        {
            return m_base.cmykIccApplied();
        }

        ImageDecodeStatus decode(const Surface& target, const ImageDecodeOptions& options);
    };

    // ----------------------------------------------------------------------------
    // functions
    // ----------------------------------------------------------------------------

    void huff_decode_mcu_lossless       (s32* output, DecodeState* state);
    void huff_decode_mcu                (s16* output, DecodeState* state);
    void huff_decode_dc_first           (s16* output, DecodeState* state);
    void huff_decode_dc_refine          (s16* output, DecodeState* state);
    void huff_decode_ac_first           (s16* output, DecodeState* state);
    void huff_decode_ac_refine          (s16* output, DecodeState* state);

    void arith_decode_mcu_lossless      (s32* output, DecodeState* state);
    void arith_decode_mcu               (s16* output, DecodeState* state);
    void arith_decode_dc_first          (s16* output, DecodeState* state);
    void arith_decode_dc_refine         (s16* output, DecodeState* state);
    void arith_decode_ac_first          (s16* output, DecodeState* state);
    void arith_decode_ac_refine         (s16* output, DecodeState* state);

    void idct8                          (u8* dest, const s16* data, const s16* qt);
    void idct12                         (u8* dest, const s16* data, const s16* qt);

    void process_y_8bit                 (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_y_24bit                (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_y_32bit                (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_cmyk_rgba              (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    // CMYK scanlines stored as C,M,Y,K in RGBA byte slots; converts in-place to display sRGB.
    bool transform_cmyk_surface_to_srgb(Surface& surface, ConstMemory icc);
    void simple_cmyk_surface_to_rgba(Surface& surface);
    void process_ycbcr_8bit             (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    void process_rgb_bgr               (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_rgb_rgb               (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_rgb_bgra              (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_rgb_rgba              (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_bgr              (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_8x8          (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_8x16         (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x8         (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x16        (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgb              (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_8x8          (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_8x16         (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x8         (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x16        (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_bgra             (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_8x8         (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_8x16        (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_16x8        (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_16x16       (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgba             (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_8x8         (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_8x16        (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_16x8        (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_16x16       (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

#if defined(MANGO_ENABLE_NEON)

    void idct_neon                      (u8* dest, const s16* data, const s16* qt);

    void process_ycbcr_bgra_8x8_neon    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_8x16_neon   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_16x8_neon   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_16x16_neon  (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgba_8x8_neon    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_8x16_neon   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_16x8_neon   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_16x16_neon  (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_bgr_8x8_neon     (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_8x16_neon    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x8_neon    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x16_neon   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgb_8x8_neon     (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_8x16_neon    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x8_neon    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x16_neon   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

#endif // MANGO_ENABLE_NEON

#if defined(MANGO_ENABLE_SSE2)

    void idct_sse2                      (u8* dest, const s16* data, const s16* qt);

    void process_ycbcr_bgra_8x8_sse2    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_8x16_sse2   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_16x8_sse2   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_16x16_sse2  (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgba_8x8_sse2    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_8x16_sse2   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_16x8_sse2   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_16x16_sse2  (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

#endif // MANGO_ENABLE_SSE2

#if defined(MANGO_ENABLE_SSE4_1)

    void process_ycbcr_bgr_8x8_sse41    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_8x16_sse41   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x8_sse41   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x16_sse41  (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgb_8x8_sse41    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_8x16_sse41   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x8_sse41   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x16_sse41  (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

#endif // MANGO_ENABLE_SSE4_1

#if defined(MANGO_ENABLE_AVX2)

    void process_ycbcr_rgba_8x8_avx2    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

#endif // MANGO_ENABLE_AVX2

    SampleFormat getSampleFormat(const Format& format);
    ImageEncodeStatus encodeImage(Stream& stream, const Surface& surface, const ImageEncodeOptions& options);

} // namespace mango::jpeg

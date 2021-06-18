/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <string>
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>

namespace mango::jpeg
{

    // ----------------------------------------------------------------------------
    // Specifications
    // ----------------------------------------------------------------------------

    // https://www.w3.org/Graphics/JPEG/itu-t81.pdf

    // ----------------------------------------------------------------------------
    // markers
    // ----------------------------------------------------------------------------

    enum jpegMarker
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

    using mango::u8;
    using mango::u16;
    using mango::u32;
    using mango::u64;
    using mango::s16;

	using mango::Stream;
    using mango::ThreadPool;
    using mango::Memory;
    using mango::ConstMemory;

    using mango::image::Format;
    using mango::image::LuminanceFormat;
    using mango::image::Surface;
    using mango::image::Bitmap;
    using mango::image::ImageHeader;
    using mango::image::ImageDecodeStatus;
    using mango::image::ImageEncodeStatus;
    using mango::image::ImageDecodeOptions;
    using mango::image::ImageEncodeOptions;

#ifdef MANGO_CPU_64BIT

    using DataType = u64;

	#define bextr mango::u64_extract_bits

    #define JPEG_REGISTER_BITS  64
    #define JPEG_REGISTER_BYTES 8
    #define JPEG_REGISTER_FILL  6

#else

    using DataType = u32;

	#define bextr mango::u32_extract_bits

    #define JPEG_REGISTER_BITS  32
    #define JPEG_REGISTER_BYTES 4
    #define JPEG_REGISTER_FILL  2

#endif

    constexpr int JPEG_MAX_BLOCKS_IN_MCU  = 10;  // Maximum # of blocks per MCU in the JPEG specification
    constexpr int JPEG_MAX_SAMPLES_IN_MCU = 64 * JPEG_MAX_BLOCKS_IN_MCU;
    constexpr int JPEG_MAX_COMPS_IN_SCAN  = 4;   // JPEG limit on # of components in one scan
    constexpr int JPEG_NUM_ARITH_TBLS     = 16;  // Arith-coding tables are numbered 0..15
    constexpr int JPEG_DC_STAT_BINS       = 64;  // ...
    constexpr int JPEG_AC_STAT_BINS       = 256; // ...
    constexpr int JPEG_HUFF_LOOKUP_BITS   = 8;   // Huffman look-ahead table log2 size
    constexpr int JPEG_HUFF_LOOKUP_SIZE   = (1 << JPEG_HUFF_LOOKUP_BITS);

    // supported external data formats (encode from, decode to)
    enum SampleType
    {
        JPEG_U8_Y,
        JPEG_U8_BGR,
        JPEG_U8_RGB,
        JPEG_U8_BGRA,
        JPEG_U8_RGBA,
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

        DataType data;
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

    struct HuffTable
    {
        u8 size[17];
        u8 value[256];

        // acceleration tables
        DataType maxcode[18];
        DataType valueOffset[19];
        u8 lookupSize[JPEG_HUFF_LOOKUP_SIZE];
        u8 lookupValue[JPEG_HUFF_LOOKUP_SIZE];

        bool configure();
        int decode(BitBuffer& buffer) const;
    };

    struct Huffman
    {
        int last_dc_value[JPEG_MAX_COMPS_IN_SCAN];
        int eob_run;

        HuffTable table[2][JPEG_MAX_COMPS_IN_SCAN];

        void restart();
    };

    struct Arithmetic
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

        Arithmetic();
        ~Arithmetic();

        void restart(BitBuffer& buffer);
    };

    struct Frame
    {
        int compid; // Component identifier
        int Hsf;    // Horizontal sampling factor
        int Vsf;    // Vertical sampling factor
        int Tq;     // Quantization table destination selector
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
        Huffman huffman;
        Arithmetic arithmetic;

        DecodeBlock block[JPEG_MAX_BLOCKS_IN_MCU];
        int blocks;
        int comps_in_scan;

        const u8* zigzagTable;

        int spectralStart;
        int spectralEnd;
        int successiveHigh;
        int successiveLow;

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

        void (*process            ) (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_y          ) (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_cmyk       ) (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_ycbcr      ) (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_ycbcr_8x8  ) (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_ycbcr_8x16 ) (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_ycbcr_16x8 ) (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_ycbcr_16x16) (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    };

    // ----------------------------------------------------------------------------
    // Parser
    // ----------------------------------------------------------------------------

    class Parser
    {
    protected:
        QuantTable quantTable[JPEG_MAX_COMPS_IN_SCAN];

        AlignedStorage<s16> quantTableVector;
        AlignedStorage<s16> blockVector;

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

        const Surface* m_surface;

        int width;  // Image width, does include alignment
        int height; // Image height, does include alignment
        int xsize;  // Image width, does NOT include alignment
        int ysize;  // Image height, does NOT include alignment
        int precision; // 8 or 12 bits
        int components; // 1..4

        bool is_baseline = true;
        bool is_progressive = false;
        bool is_multiscan = false;
        bool is_arithmetic = false;
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
        const u8* seekRestartMarker(const u8* p, const u8* end) const;

        void processSOI();
        void processEOI();
        void processCOM(const u8* p);
        void processTEM(const u8* p);
        void processRES(const u8* p);
        void processJPG(const u8* p);
        void processJPG(const u8* p, u16 marker);
        void processAPP(const u8* p, u16 marker);
        void processSOF(const u8* p, u16 marker);
        const u8* processSOS(const u8* p, const u8* end);
        void processDQT(const u8* p);
        void processDNL(const u8* p);
        void processDRI(const u8* p);
        void processDHT(const u8* p, const u8* end);
        void processDAC(const u8* p);
        void processDHP(const u8* p);
        void processEXP(const u8* p);

        void parse(ConstMemory memory, bool decode);

        void restart();
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

        int getTaskSize(int count) const;
        void configureCPU(SampleType sample, const ImageDecodeOptions& options);
        std::string getInfo() const;

    public:
        ImageHeader header;
        ConstMemory exif_memory; // Exif block, if one is present
        ConstMemory scan_memory; // Scan block
        Buffer icc_buffer; // ICC color profile block, if one is present

        Parser(ConstMemory memory);
        ~Parser();

        ImageDecodeStatus decode(const Surface& target, const ImageDecodeOptions& options);
    };

    // ----------------------------------------------------------------------------
    // functions
    // ----------------------------------------------------------------------------

    using ProcessFunc = void (*)(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    void huff_decode_mcu_lossless   (s16* output, DecodeState* state);
    void huff_decode_mcu            (s16* output, DecodeState* state);
    void huff_decode_dc_first       (s16* output, DecodeState* state);
    void huff_decode_dc_refine      (s16* output, DecodeState* state);
    void huff_decode_ac_first       (s16* output, DecodeState* state);
    void huff_decode_ac_refine      (s16* output, DecodeState* state);

    void arith_decode_mcu_lossless  (s16* output, DecodeState* state);
    void arith_decode_mcu           (s16* output, DecodeState* state);
    void arith_decode_dc_first      (s16* output, DecodeState* state);
    void arith_decode_dc_refine     (s16* output, DecodeState* state);
    void arith_decode_ac_first      (s16* output, DecodeState* state);
    void arith_decode_ac_refine     (s16* output, DecodeState* state);

    void idct8                          (u8* dest, const s16* data, const s16* qt);
    void idct12                         (u8* dest, const s16* data, const s16* qt);

    void process_y_8bit                 (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_y_24bit                (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_y_32bit                (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_cmyk_bgra              (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_8bit             (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

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

    void process_ycbcr_bgr_8x8_ssse3    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_8x16_ssse3   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x8_ssse3   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x16_ssse3  (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgb_8x8_ssse3    (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_8x16_ssse3   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x8_ssse3   (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x16_ssse3  (u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

#endif // MANGO_ENABLE_SSE4_1

    SampleFormat getSampleFormat(const Format& format);
	ImageEncodeStatus encodeImage(Stream& stream, const Surface& surface, const ImageEncodeOptions& options);

} // namespace mango::jpeg

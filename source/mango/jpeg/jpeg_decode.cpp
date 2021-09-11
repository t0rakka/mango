/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/endian.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/core/thread.hpp>
#include "jpeg.hpp"

namespace mango::jpeg
{

    // ----------------------------------------------------------------------------
    // utilities
    // ----------------------------------------------------------------------------

    static const u8 g_zigzag_table_inverse [] =
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

    inline bool isRestartMarker(const u8* p)
    {
        bool is = false;

        if (p[0] == 0xff)
        {
            int index = p[1] - 0xd0;
            is = index >= 0 && index <= 7;
        }

        return is;
    }

    void jpegPrintMemory(const u8* ptr)
    {
        for (int i = 0; i < 8; ++i)
        {
            printf("%.2x ", ptr[i - 8]);
        }
        printf("| ");
        for (int i = 0; i < 8; ++i)
        {
            printf("%.2x ", ptr[i]);
        }
        printf("\n");
    }

    static
    const SampleFormat g_format_table [] =
    {
        { JPEG_U8_Y,    LuminanceFormat(8, Format::UNORM, 8, 0) },
        { JPEG_U8_BGR,  Format(24, Format::UNORM, Format::BGR, 8, 8, 8) },
        { JPEG_U8_RGB,  Format(24, Format::UNORM, Format::RGB, 8, 8, 8) },
        { JPEG_U8_BGRA, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8) },
        { JPEG_U8_RGBA, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8) },
    };

    SampleFormat getSampleFormat(const Format& format)
    {
        // set default format
        SampleFormat result { JPEG_U8_RGBA, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8) };

        // find better match
        for (auto sf : g_format_table)
        {
            if (format == sf.format)
            {
                result = sf;
                break;
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------------
    // BitBuffer
    // ----------------------------------------------------------------------------

    void BitBuffer::restart()
    {
        data = 0;
        remain = 0;
    }

    void BitBuffer::fill()
    {
#if defined(MANGO_CPU_64BIT) && defined(MANGO_ENABLE_SSE2)
        if (ptr + 8 <= end)
        {
            u64 x = uload64(ptr);
            const __m128i ref = _mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128());
            u32 mask = _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_set_epi64x(0, x), ref));
            if (!mask)
            {
                data = (data << 48) | (byteswap(x) >> 16);
                remain += 48;
                ptr += 6;
                return;
            }
        }
#endif

        for (int i = 0; i < JPEG_REGISTER_FILL; ++i)
        {
            const u8* x = ptr;

            int a = ptr < end ? *ptr++ : 0;
            if (a == 0xff)
            {
                int b = ptr < end ? *ptr++ : 0;
                if (b)
                {
                    // Found a marker; keep returning zero until it has been processed
                    ptr = x;
                    a = 0;
                }
            }

            remain += 8;
            data = (data << 8) | a;
        }
    }

    // ----------------------------------------------------------------------------
    // Parser
    // ----------------------------------------------------------------------------

    Parser::Parser(ConstMemory memory)
        : quantTableVector(64 * JPEG_MAX_COMPS_IN_SCAN)
    {
        restartInterval = 0;
        restartCounter = 0;

        for (int i = 0; i < JPEG_MAX_COMPS_IN_SCAN; ++i)
        {
            quantTable[i].table = quantTableVector.data() + i * 64;
        }

        m_surface = nullptr;

        if (isJPEG(memory))
        {
            parse(memory, false);
        }
        else
        {
            header.setError("Incorrect SOI marker.");
        }
    }

    Parser::~Parser()
    {
    }

    bool Parser::isJPEG(ConstMemory memory) const
    {
        if (!memory.address || memory.size < 4)
            return false;

        if (uload16be(memory.address) != MARKER_SOI)
            return false;

#if 0
        // Scan for EOI marker
        const u8* p = memory.address + memory.size - 2;
        for (int i = 0; i < 32; ++i, --p)
        {
            u16 marker = uload16be(p);
            if (marker == MARKER_EOI)
                return true;
        }

        return false;
#else
        // Let's not be so picky.. EOI marker is optional, right?
        // (A lot of JPEG writers think so and we just have to deal with it :)
        return true;
#endif
    }

    const u8* Parser::stepMarker(const u8* p, const u8* end) const
    {
        if (p + 2 > end)
            return end;

        u16 size = uload16be(p);
        p += size;

        if (p + 1 > end)
            return end;

        p += (p[1] == 0xff); // HACK: some really ancient jpeg encoders encode markers sometimes as
                             // (0xff, 0xff, ID) ; this will skip to the "correct" 0xff (the second one)
        return p;
    }

    const u8* Parser::seekRestartMarker(const u8* start, const u8* end) const
    {
        const u8* p = start;
        --end; // marker is two bytes: don't look at last byte

        while (p < end)
        {
            p = mango::memchr(p, 0xff, end - p);
            if (p[1])
            {
                return p; // found a marker
            }
            p += 2; // skip: 0xff, 0x00
        }

        //if (*p != 0xff)
        ++p; // skip last byte (warning! if it is 0xff a marker can be potentially missed)
        debugPrint("  Seek: %d bytes\n", int(p - start));

        return p;
    }

    void Parser::processSOI()
    {
        debugPrint("[ SOI ]\n");
        restartInterval = 0;
    }

    void Parser::processEOI()
    {
        debugPrint("[ EOI ]\n");
    }

    void Parser::processCOM(const u8* p)
    {
        debugPrint("[ COM ]\n");
        MANGO_UNREFERENCED(p);
    }

    void Parser::processTEM(const u8* p)
    {
        debugPrint("[ TEM ]\n");
        MANGO_UNREFERENCED(p);
    }

    void Parser::processRES(const u8* p)
    {
        debugPrint("[ RES ]\n");

        // Reserved for jpeg extensions
        MANGO_UNREFERENCED(p);
    }

    void Parser::processJPG(const u8* p)
    {
        debugPrint("[ JPG ]\n");

        // Reserved for jpeg extensions
        MANGO_UNREFERENCED(p);
    }

    void Parser::processJPG(const u8* p, u16 marker)
    {
        debugPrint("[ JPG%d ]\n", int(marker - MARKER_JPG0));

        // Reserved for jpeg extensions
        MANGO_UNREFERENCED(p);
        MANGO_UNREFERENCED(marker);
    }

    void Parser::processAPP(const u8* p, u16 marker)
    {
        debugPrint("[ APP%d ]\n", int(marker - MARKER_APP0));

        int size = uload16be(p) - 2;
        p += 2;

        switch (marker)
        {
            case MARKER_APP0:
            {
                const u8 magicJFIF[] = { 0x4a, 0x46, 0x49, 0x46, 0 }; // 'JFIF', 0
                const u8 magicJFXX[] = { 0x4a, 0x46, 0x58, 0x58, 0 }; // 'JFXX', 0

                if (!std::memcmp(p, magicJFIF, 5) || !std::memcmp(p, magicJFXX, 5))
                {
                    p += 5;
                    size -= 5;

                    debugPrint("  JFIF: %i bytes\n", size);

                    int version = p[0] * 100 + p[1];
                    int units = p[2]; // 0 - no units, 1 - dots per inch, 2 - dots per cm
                    int Xdensity = (p[3] << 16) | p[4];
                    int Ydensity = (p[5] << 16) | p[6];
                    int Xthumbnail = p[7];
                    int Ythumbnail = p[8];

                    const char* unit_str = "";

                    switch (units)
                    {
                        case 1: unit_str = "dpi"; break;
                        case 2: unit_str = "cpi"; break;
                    }

                    debugPrint("    version: %i\n", version);
                    debugPrint("    density: %i x %i %s\n", Xdensity, Ydensity, unit_str);
                    debugPrint("    thumbnail: %i x %i\n", Xthumbnail, Ythumbnail);

                    // TODO: process thumbnail / store JFIF block
                    MANGO_UNREFERENCED(version);
                    MANGO_UNREFERENCED(Xdensity);
                    MANGO_UNREFERENCED(Ydensity);
                    MANGO_UNREFERENCED(Xthumbnail);
                    MANGO_UNREFERENCED(Ythumbnail);
                    MANGO_UNREFERENCED(unit_str);
                }

                break;
            }

            case MARKER_APP1:
            {
                const u8 magicExif0[] = { 0x45, 0x78, 0x69, 0x66, 0, 0 }; // 'Exif', 0, 0
                const u8 magicExif255[] = { 0x45, 0x78, 0x69, 0x66, 0, 0xff }; // 'Exif', 0, 0xff

                if (!std::memcmp(p, magicExif0, 6) || !std::memcmp(p, magicExif255, 6))
                {
                    p += 6;
                    size -= 6;
                    exif_memory = ConstMemory(p, size);
                    debugPrint("  EXIF: %d bytes\n", size);
                }

                // TODO: detect and support XMP

                break;
            }

            case MARKER_APP2:
            {
                const u8 magicICC[] = { 0x49, 0x43, 0x43, 0x5f, 0x50, 0x52, 0x4f, 0x46, 0x49, 0x4c, 0x45, 0 }; // 'ICC_PROFILE', 0

                if (!std::memcmp(p, magicICC, 12))
                {
                    // skip magic
                    p += 12;
                    size -= 12;

                    // read sequence information
                    u8 sequence_number = p[0];
                    u8 sequence_total = p[1];
                    p += 2;
                    size -= 2;

                    debugPrint("  ICC: %d / %d (%d bytes)\n", sequence_number, sequence_total, size);
                    MANGO_UNREFERENCED(sequence_number);
                    MANGO_UNREFERENCED(sequence_total);

                    // append ICC segment (JPEG markers have a maximum size and are split)
                    icc_buffer.append(p, size);
                }

                break;
            }

            case MARKER_APP3:
            {
                const u8 magicMETA[] = { 0x4d, 0x45, 0x54, 0x41, 0, 0 }; // 'META', 0, 0
                const u8 magicMeta[] = { 0x4d, 0x65, 0x74, 0x61, 0, 0 }; // 'Meta', 0, 0

                if (!std::memcmp(p, magicMETA, 6) || !std::memcmp(p, magicMeta, 6))
                {
                    p += 6;
                    size -= 6;
                    exif_memory = ConstMemory(p, size);
                    debugPrint("  EXIF: %d bytes\n", size);
                }

                break;
            }

            case MARKER_APP14:
            {
                const u8 magicAdobe[] = { 0x41, 0x64, 0x6f, 0x62, 0x65 }; // 'Adobe'
                if (size == 12 && !std::memcmp(p, magicAdobe, 5))
                {
                    u16 version = uload16be(p + 5);
                    //u16 flags0 = uload16be(p + 7);
                    //u16 flags1 = uload16be(p + 9);
                    u8 color_transform = p[11]; // 0 - CMYK, 1 - YCbCr, 2 - YCCK
                    if (color_transform <= 2)
                    {
                        processState.colorspace = ColorSpace(color_transform);
                    }
                    debugPrint("  Version: %d\n", version);
                    debugPrint("  ColorTransform: %d\n", color_transform);
                    MANGO_UNREFERENCED(version);
                    MANGO_UNREFERENCED(color_transform);
                }
                break;
            }
        }
    }

    void Parser::processSOF(const u8* p, u16 marker)
    {
        debugPrint("[ SOF%d ]\n", int(marker - MARKER_SOF0));

        is_baseline = (marker == MARKER_SOF0);
        is_progressive = false;
        is_multiscan = false;
        is_arithmetic = false;
        is_lossless = false;
        is_differential = false;

        u16 length = uload16be(p + 0);
        precision = p[2];
        ysize = uload16be(p + 3);
        xsize = uload16be(p + 5);
        components = p[7];
        p += 8;

        debugPrint("  Image: %d x %d x %d\n", xsize, ysize, precision);

        u16 correct_length = 8 + 3 * components;
        if (length != correct_length)
        {
            header.setError("Incorrect chunk length (%d, should be %d).", length, correct_length);
            return;
        }

        if (xsize <= 0 || ysize <= 0 || xsize > 65535 || ysize > 65535)
        {
            // NOTE: ysize of 0 is allowed in the specs but we won't
            header.setError("Incorrect dimensions (%d x %d)", xsize, ysize);
            return;
        }

        if (components < 1 || components > 4)
        {
            // NOTE: only progressive is required to have 1..4 components,
            //       other modes allow 1..255 but we are extra strict here :)
            header.setError("Incorrect number of components (%d)", components);
            return;
        }

        is_arithmetic = marker > MARKER_SOF7;
        m_compression = is_arithmetic ? "Arithmetic" : "Huffman";

        const char* encoding = "";
        switch (marker)
        {
            // Huffman
            case MARKER_SOF0:  encoding = "Baseline DCT"; break;
            case MARKER_SOF1:  encoding = "Extended sequential DCT"; break;
            case MARKER_SOF2:  encoding = "Progressive DCT"; break;
            case MARKER_SOF3:  encoding = "Lossless"; break;
            case MARKER_SOF5:  encoding = "Differential sequential DCT"; break;
            case MARKER_SOF6:  encoding = "Differential progressive DCT"; break;
            case MARKER_SOF7:  encoding = "Differential lossless"; break;
            // Arithmetic
            case MARKER_SOF9:  encoding = "Extended sequential DCT"; break;
            case MARKER_SOF10: encoding = "Progressive DCT"; break;
            case MARKER_SOF11: encoding = "Lossless"; break;
            case MARKER_SOF13: encoding = "Differential sequential DCT"; break;
            case MARKER_SOF14: encoding = "Differential progressive DCT"; break;
            case MARKER_SOF15: encoding = "Differential lossless"; break;
        }

        m_encoding = encoding;

        debugPrint("  Encoding: %s\n", m_encoding.c_str());
        debugPrint("  Compression: %s\n", m_compression.c_str());

        switch (marker)
        {
            case MARKER_SOF2:
            case MARKER_SOF6:
            case MARKER_SOF10:
            case MARKER_SOF14:
                is_progressive = true;
                break;
        }

        switch (marker)
        {
            case MARKER_SOF7:
            case MARKER_SOF15:
                is_differential = true;
                // fall-through
            case MARKER_SOF3:
            case MARKER_SOF11:
                is_lossless = true;
                break;
        }

        if (is_baseline)
        {
            if (precision != 8)
            {
                header.setError(makeString("Incorrect precision (%d, allowed: 8)", precision));
                return;
            }
        }
        else if (is_lossless)
        {
            if (precision < 2 || precision > 16)
            {
                header.setError(makeString("Incorrect precision (%d, allowed: 2..16)", precision));
                return;
            }
        }
        else
        {
            if (precision != 8 && precision != 12)
            {
                header.setError(makeString("Incorrect precision (%d, allowed: 8, 12)", precision));
                return;
            }
        }

        Hmax = 0;
        Vmax = 0;
        blocks_in_mcu = 0;
        int offset = 0;

        processState.frames = components;

        for (int i = 0; i < components; ++i)
        {
            if (offset >= JPEG_MAX_BLOCKS_IN_MCU)
            {
                header.setError("Incorrect blocks offset (%d >= %d).", offset, JPEG_MAX_BLOCKS_IN_MCU);
                return;
            }

            Frame& frame = processState.frame[i];

            frame.compid = p[0];
            u8 x = p[1];
            frame.Hsf = (x >> 4) & 0xf;
            frame.Vsf = (x >> 0) & 0xf;
            frame.Tq = p[2];
            frame.offset = offset;
            p += 3;

            u8 max_tq = is_lossless ? 0 : 3;
            if (frame.Tq > max_tq)
            {
                header.setError("Incorrect quantization table index (%d)", frame.Tq);
                return;
            }

            if (components == 1)
            {
                // Optimization: force block size to 8x8 with grayscale images
                frame.Hsf = 1;
                frame.Vsf = 1;
            }

            Hmax = std::max(Hmax, frame.Hsf);
            Vmax = std::max(Vmax, frame.Vsf);
            blocks_in_mcu += frame.Hsf * frame.Vsf;

            if (frame.Hsf < 1 || frame.Hsf > 4 || frame.Vsf < 1 || frame.Vsf > 4)
            {
                header.setError(makeString("Incorrect frame sampling rate (%d x %d)", frame.Hsf, frame.Vsf));
                return;
            }

            if (blocks_in_mcu > JPEG_MAX_BLOCKS_IN_MCU)
            {
                header.setError(makeString("Incorrect number of blocks in MCU (%d >= %d).", blocks_in_mcu, JPEG_MAX_BLOCKS_IN_MCU));
                return;
            }

            for (int y = 0; y < frame.Vsf; ++y)
            {
                for (int x = 0; x < frame.Hsf; ++x)
                {
                    processState.block[offset].qt = quantTable[frame.Tq].table;
                    if (!processState.block[offset].qt)
                    {
                        header.setError("No quantization table for index (%d)", frame.Tq);
                        return;
                    }

                    ++offset;
                }
            }

            debugPrint("  Frame: %d, compid: %d, Hsf: %d, Vsf: %d, Tq: %d, offset: %d\n",
                i, frame.compid, frame.Hsf, frame.Vsf, frame.Tq, frame.offset);

            frames.push_back(frame);
        }

        processState.blocks = offset;

        // Compute frame sampling factors against maximum sampling factor,
        // then convert them into power-of-two presentation.
        for (int i = 0; i < components; ++i)
        {
            Frame& frame = processState.frame[i];
            if (!frame.Hsf || !frame.Vsf)
            {
                header.setError("Incorrect sampling factors (%d x %d)", frame.Hsf, frame.Vsf);
                return;
            }
            frame.Hsf = u32_log2(Hmax / frame.Hsf);
            frame.Vsf = u32_log2(Vmax / frame.Vsf);
        }

        xblock = 8 * Hmax;
        yblock = 8 * Vmax;

        if (!xblock || !yblock)
        {
            header.setError("Incorrect dimensions (%d x %d)", xblock, yblock);
            return;
        }

        debugPrint("  Blocks per MCU: %d\n", blocks_in_mcu);
        debugPrint("  MCU size: %d x %d\n", xblock, yblock);

        // Align to next MCU boundary
        int xmask = xblock - 1;
        int ymask = yblock - 1;
        width  = (xsize + xmask) & ~xmask;
        height = (ysize + ymask) & ~ymask;

        // MCU resolution
        xmcu = width  / xblock;
        ymcu = height / yblock;
        mcus = xmcu * ymcu;

        debugPrint("  %d MCUs (%d x %d) -> (%d x %d)\n", mcus, xmcu, ymcu, xmcu * xblock, ymcu * yblock);
        debugPrint("  Image: %d x %d\n", xsize, ysize);

        // configure header
        header.width = xsize;
        header.height = ysize;
        header.format = components > 1 ? Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8)
                                       : LuminanceFormat(8, Format::UNORM, 8, 0);

        MANGO_UNREFERENCED(length);
    }

    const u8* Parser::processSOS(const u8* p, const u8* end)
    {
        debugPrint("[ SOS ]\n");

        u16 length = uload16be(p);
        u8 components = p[2]; // Ns
        p += 3;

        u16 correct_length = 6 + 2 * components;
        if (length != correct_length)
        {
            header.setError("Incorrect chunk length (%d, should be %d).", length, correct_length);
            return p;
        }

        if (components < 1 || components > 4)
        {
            header.setError("Incorrect number of components (%d).", components);
            return p;
        }

        if (components != processState.frames && !is_progressive)
        {
            is_multiscan = true;

            // allocate blocks
            size_t num_blocks = size_t(mcus) * blocks_in_mcu;
            blockVector.resize(num_blocks * 64);
        }

        decodeState.comps_in_scan = components;

        debugPrint("  components: %i%s\n", components, is_multiscan ? " (MultiScan)" : "");
        MANGO_UNREFERENCED(length);

        decodeState.blocks = 0;

        for (int i = 0; i < components; ++i)
        {
            u8 cs = p[0]; // Scan component selector
            u8 x = p[1];
            p += 2;
            int dc = (x >> 4) & 0xf; // DC entropy coding table destination selector
            int ac = (x >> 0) & 0xf; // AC entropy coding table destination selector

            // default limits
            int max_dc = 3;
            int max_ac = 3;

            if (is_baseline)
            {
                max_dc = 1;
                max_ac = 1;
            }
            else if (is_lossless)
            {
                max_ac = 0;
            }

            if (dc > max_dc || ac > max_ac)
            {
                header.setError(makeString("Incorrect coding table selector (DC: %d, AC: %d).", dc, ac));
                return p;
            }

            // find frame
            Frame* frame = nullptr;
            int pred = 0;

            for (int j = 0; j < int(frames.size()); ++j)
            {
                if (frames[j].compid == cs)
                {
                    pred = j;
                    frame = &frames[j];
                }
            }

            if (!frame)
            {
                header.setError("Incorrect scan component selector (%d)", cs);
                return p;
            }

            scanFrame = frame;

            const int size = frame->Hsf * frame->Vsf;
            int offset = frame->offset;

            std::string cs_name;
            if (cs >= 32 && cs < 128)
            {
                cs_name = makeString(" (%c)", char(cs));
            }

            debugPrint("  Component: %i%s, DC: %i, AC: %i, offset: %d, size: %d\n",
                cs, cs_name.c_str(), dc, ac, frame->offset, size);

            for (int j = 0; j < size; ++j)
            {
                if (offset >= JPEG_MAX_BLOCKS_IN_MCU)
                {
                    header.setError("Incorrect number of blocks in MCU (%d >= %d).", offset, JPEG_MAX_BLOCKS_IN_MCU);
                    return p;
                }

                DecodeBlock& block = decodeState.block[decodeState.blocks];

                block.offset = offset * 64;
                block.pred = pred;
                block.dc = dc;
                block.ac = ac;

                debugPrint("      - offset: %d, pred: %d,\n", offset * 64, pred);
                ++offset;
                ++decodeState.blocks;
            }
        }

        int Ss = p[0];
        int Se = p[1];

        u8 x = p[2];
        int Al = (x >> 0) & 0xf;
        int Ah = (x >> 4) & 0xf;
        p += 3;

        decodeState.spectralStart = Ss;
        decodeState.spectralEnd = Se;
        decodeState.successiveLow = Al;
        decodeState.successiveHigh = Ah;

        debugPrint("  Spectral range: (%d, %d)\n", Ss, Se);

        // default limits
        int min_ss = 0;
        int max_ss = 0;

        int min_se = 63;
        int max_se = 63;

        int min_ah = 0;
        int max_ah = 0;

        int min_al = 0;
        int max_al = 0;

        if (is_progressive)
        {
            max_ss = 63;
            min_se = Ss;
            max_ah = 13;
            max_al = 13;
        }
        else if (is_lossless)
        {
            min_ss = 1; max_ss = 7;
            min_se = 0; max_se = 0;
            max_al = 15;
        }

        if (Ss < min_ss || Ss > max_ss ||
            Se < min_se || Se > max_se ||
            Ah < min_ah || Ah > max_ah ||
            Al < min_al || Al > max_al)
        {
            header.setError("Incorrect spectral range.");
            return p;
        }

        bool dc_scan = (decodeState.spectralStart == 0);
        bool refine_scan = (decodeState.successiveHigh != 0);

        decodeState.zigzagTable = g_zigzag_table_inverse;

        restartCounter = restartInterval;

        if (is_arithmetic)
        {
            Arithmetic& arithmetic = decodeState.arithmetic;

            decodeState.buffer.ptr = p;
            decodeState.buffer.end = end;

            // restart
            decodeState.buffer.restart();

            arithmetic.restart(decodeState.buffer);

            if (is_lossless)
            {
                decodeLossless();
            }
            else if (is_multiscan)
            {
                decodeState.decode = arith_decode_mcu;
                decodeMultiScan();
            }
            else if (is_progressive)
            {
                if (dc_scan)
                {
                    if (!refine_scan)
                    {
                        debugPrint("  * decode_dc_first()\n");
                        decodeState.decode = arith_decode_dc_first;
                    }
                    else
                    {
                        debugPrint("  * decode_dc_refine()\n");
                        decodeState.decode = arith_decode_dc_refine;
                    }
                }
                else
                {
                    if (!refine_scan)
                    {
                        debugPrint("  * decode_ac_first()\n");
                        decodeState.decode = arith_decode_ac_first;
                    }
                    else
                    {
                        debugPrint("  * decode_ac_refine()\n");
                        decodeState.decode = arith_decode_ac_refine;
                    }
                }
                decodeProgressive();
            }
            else
            {
                decodeState.decode = arith_decode_mcu;
                decodeSequential();
            }
        }
        else
        {
            Huffman& huffman = decodeState.huffman;

            decodeState.buffer.ptr = p;
            decodeState.buffer.end = end;

            // restart
            decodeState.buffer.restart();

            huffman.restart();

            if (is_lossless)
            {
                decodeLossless();
            }
            else if (is_multiscan)
            {
                decodeState.decode = huff_decode_mcu;
                decodeMultiScan();
            }
            else if (is_progressive)
            {
                if (dc_scan)
                {
                    if (!refine_scan)
                    {
                        debugPrint("  * decode_dc_first()\n");
                        decodeState.decode = huff_decode_dc_first;
                    }
                    else
                    {
                        debugPrint("  * decode_dc_refine()\n");
                        decodeState.decode = huff_decode_dc_refine;
                    }
                }
                else
                {
                    if (!refine_scan)
                    {
                        debugPrint("  * decode_ac_first()\n");
                        decodeState.decode = huff_decode_ac_first;
                    }
                    else
                    {
                        debugPrint("  * decode_ac_refine()\n");
                        decodeState.decode = huff_decode_ac_refine;
                    }
                }
                decodeProgressive();
            }
            else
            {
                decodeState.decode = huff_decode_mcu;
                decodeSequential();
            }
        }

        if (debugPrintIsEnable())
        {
            jpegPrintMemory(decodeState.buffer.ptr);
        }

        p = decodeState.buffer.ptr;

        return p;
    }

    void Parser::processDQT(const u8* p)
    {
        debugPrint("[ DQT ]\n");

        u16 Lq = uload16be(p); // Quantization table definition length
        p += 2;
        Lq -= 2;

        for ( ; Lq > 0; )
        {
            u8 x = *p++;
            u8 Pq = (x >> 4) & 0xf; // Quantization table element precision (0 = 8 bits, 1 = 16 bits)
            u8 Tq = (x >> 0) & 0xf; // Quantization table destination identifier (0..3)

            const int bits = (Pq + 1) * 8;
            debugPrint("  Quantization table #%i element precision: %i bits\n", Tq, bits);

            if (!is_lossless)
            {
                u8 max_pq = is_baseline ? 0 : 1;

                if (Pq > max_pq)
                {
                    header.setError("Incorrect quantization table element precision (%d bits)", Pq);
                    return;
                }

                if (Tq > 3)
                {
                    header.setError("Incorrect quantization table (%d)", Tq);
                    return;
                }
            }

            QuantTable& table = quantTable[Tq];
            table.bits = bits;

            switch (Pq)
            {
                case 0:
                    for (int i = 0; i < 64; ++i)
                    {
                        table.table[g_zigzag_table_inverse[i]] = *p++;
                    }
                    break;

                case 1:
                    for (int i = 0; i < 64; ++i)
                    {
                        table.table[g_zigzag_table_inverse[i]] = uload16be(p);
                        p += 2;
                    }
                    break;

                default:
                    break;
            }

            Lq -= (1 + (Pq + 1) * 64);
        }
    }

    void Parser::processDHT(const u8* p, const u8* end)
    {
        debugPrint("[ DHT ]\n");

        int Lh = uload16be(p); // Huffman table definition length
        p += 2;
        Lh -= 2;

        std::vector<HuffTable*> tables;

        for ( ; Lh > 0; )
        {
            u8 x = p[0];
            u8 Tc = (x >> 4) & 0xf; // Table class - 0 = DC table or lossless table, 1 = AC table.
            u8 Th = (x >> 0) & 0xf; // Huffman table identifier

            u8 max_tc = is_lossless ? 0 : 1; 
            u8 max_th = is_baseline ? 1 : 3;

            if (Tc > max_tc)
            {
                header.setError(makeString("Incorrect huffman table class (%d)", Tc));
                return;
            }

            if (Th > max_th)
            {
                header.setError(makeString("Incorrect huffman table identifier (%d)", Th));
                return;
            }

            HuffTable& table = decodeState.huffman.table[Tc][Th];

            debugPrint("  Huffman table #%d table class: %d\n", Th, Tc);
            debugPrint("    codes: ");

            if (p >= end - 17)
            {
                header.setError("Data overflow.");
                return;
            }

            int count = 0;

            for (int i = 1; i <= 16; ++i)
            {
                u8 L = p[i]; // Number of Huffman codes of length i bits
                table.size[i] = L;
                count += L;
                debugPrint("%i ", L);
            }

            debugPrint("\n");

            p += 17;
            Lh -= 17;

            if (Lh < 0 || count > 256)
            {
                header.setError("Incorrect huffman table data.");
                return;
            }

            if (p >= end - count)
            {
                header.setError("Data overflow.");
                return;
            }

            std::memcpy(table.value, p, count);
            p += count;
            Lh -= count;

            if (Lh < 0)
            {
                header.setError("Incorrect huffman table data.");
                return;
            }

            tables.push_back(&table);
        }

        if (Lh != 0)
        {
            header.setError("Corrupted DHT data.");
            return;
        }

        // configure tables only after the data is determined to be correct
        for (HuffTable* table : tables)
        {
            if (!table->configure())
            {
                header.setError("Corrupted DHT - Huffman table generation failed.");
                return;
            }
        }
    }

    void Parser::processDAC(const u8* p)
    {
        debugPrint("[ DAC ]\n");

        u16 La = uload16be(p); // Arithmetic coding conditioning definition length
        p += 2;

        if (is_baseline)
        {
            header.setError("BaselineDCT does not support Arithmetic Coding tables.");
            return;
        }

        int n = (La - 2) / 2;

        debugPrint("  n: %i\n", n);

        if (n > 32)
        {
            header.setError("Too many DAC entries (%d).", n);
            return;
        }

        for (int i = 0; i < n; ++i)
        {
            u8 x = p[0];
            u8 Tc = (x >> 4) & 0xf; // Table class - 0 = DC table or lossless table, 1 = AC table
            u8 Tb = (x >> 0) & 0xf; // Arithmetic coding conditioning table destination identifier
            u8 Cs = p[1]; // Conditioning table value
            p += 2;

            u8 max_tc = is_lossless ? 0 : 1;
            u8 max_tb = 3;
            u8 min_cs = (Tc == 0 || is_lossless) ? 0 : 1;
            u8 max_cs = (Tc == 0 || is_lossless) ? 255 : 63;

            if (Tc > max_tc || Tb > max_tb)
            {
                header.setError(makeString("Incorrect Arithmetic table selector (Tc: %d, Tb: %d).", Tc, Tb));
                return;
            }

            if (Cs < min_cs || Cs > max_cs)
            {
                header.setError(makeString("Incorrect Arithmetic conditioning table (%d).", Cs));
                return;
            }

            switch (Tc)
            {
                case 0:
                    // DC table
                    decodeState.arithmetic.dc_L[Tb] = (Cs & 0xf);
                    decodeState.arithmetic.dc_U[Tb] = (Cs >> 4);
                    break;

                case 1:
                    // AC table
                    decodeState.arithmetic.ac_K[Tb] = Cs;
                    break;

                default:
                    header.setError("Incorrect Arithmetic table class (%d).", Tc);
                    return;
            }

            debugPrint("  Tc: %i, Tb: %i, Cs: %i\n", Tc, Tb, Cs);
        }
    }

    void Parser::processDNL(const u8* p)
    {
        debugPrint("[ DNL ]\n");

        u16 Ld = uload16be(p + 0); // Define number of lines segment length
        u16 NL = uload16be(p + 2); // Number of lines
        MANGO_UNREFERENCED(NL); // TODO: ysize = NL, no files to test with found yet..
        MANGO_UNREFERENCED(Ld);
    }

    void Parser::processDRI(const u8* p)
    {
        debugPrint("[ DRI ]\n");

        int Lh = uload16be(p + 0); // length
        if (Lh != 4)
        {
            // signal error
        }

        restartInterval = uload16be(p + 2); // number of MCU in restart interval
        debugPrint("  Restart interval: %i\n", restartInterval);
    }

    void Parser::processDHP(const u8* p)
    {
        debugPrint("[ DHP ]\n");

        // TODO: "Define Hierarchical Progression" marker
        MANGO_UNREFERENCED(p);
    }

    void Parser::processEXP(const u8* p)
    {
        debugPrint("[ EXP ]\n");

        u16 Le = uload16be(p); // Expand reference components segment length
        u8 x = p[2];
        u8 Eh = (x >> 4) & 0xf; // Expand horizontally
        u8 Ev = (x >> 0) & 0xf; // Expand vertically

        // Unsupported marker
        MANGO_UNREFERENCED(Le);
        MANGO_UNREFERENCED(Eh);
        MANGO_UNREFERENCED(Ev);
    }

    void Parser::parse(ConstMemory memory, bool decode)
    {
        const u8* end = memory.address + memory.size;
        const u8* p = memory.address;

        for ( ; p < end; )
        {
            if (!header)
            {
                // we are in error state -> abort parsing
                break;
            }

            u16 marker = uload16be(p);
            p += 2;

            u64 time0 = Time::us();

            switch (marker)
            {
                case MARKER_SOI:
                    processSOI();
                    break;

                case MARKER_EOI:
                    processEOI();
                    p = end; // terminate parsing
                    break;

                case MARKER_DHT:
                    processDHT(p, end);
                    p = stepMarker(p, end);
                    break;

                case MARKER_DAC:
                    processDAC(p);
                    p = stepMarker(p, end);
                    break;

                case MARKER_DQT:
                    processDQT(p);
                    p = stepMarker(p, end);
                    break;

                case MARKER_DNL:
                    processDNL(p);
                    p = stepMarker(p, end);
                    break;

                case MARKER_DRI:
                    processDRI(p);
                    p = stepMarker(p, end);
                    break;

                case MARKER_DHP:
                    processDHP(p);
                    p = stepMarker(p, end);
                    break;

                case MARKER_EXP:
                    processEXP(p);
                    p = stepMarker(p, end);
                    break;

                case MARKER_COM:
                    processCOM(p);
                    p = stepMarker(p, end);
                    break;

                case MARKER_TEM:
                    processTEM(p);
                    p = stepMarker(p, end);
                    break;

                case MARKER_RES:
                    processRES(p);
                    p = stepMarker(p, end);
                    break;

                case MARKER_APP0:
                case MARKER_APP1:
                case MARKER_APP2:
                case MARKER_APP3:
                case MARKER_APP4:
                case MARKER_APP5:
                case MARKER_APP6:
                case MARKER_APP7:
                case MARKER_APP8:
                case MARKER_APP9:
                case MARKER_APP10:
                case MARKER_APP11:
                case MARKER_APP12:
                case MARKER_APP13:
                case MARKER_APP14:
                case MARKER_APP15:
                    processAPP(p, marker);
                    p = stepMarker(p, end);
                    break;

                case MARKER_JPG:
                    processJPG(p);
                    p = stepMarker(p, end);
                    break;

                case MARKER_JPG0:
                case MARKER_JPG1:
                case MARKER_JPG2:
                case MARKER_JPG3:
                case MARKER_JPG4:
                case MARKER_JPG5:
                case MARKER_JPG6:
                case MARKER_JPG7:
                case MARKER_JPG8:
                case MARKER_JPG9:
                case MARKER_JPG10:
                case MARKER_JPG11:
                case MARKER_JPG12:
                case MARKER_JPG13:
                    processJPG(p, marker);
                    p = stepMarker(p, end);
                    break;

                case MARKER_SOF0:
                case MARKER_SOF1:
                case MARKER_SOF2:
                case MARKER_SOF3:
                case MARKER_SOF5:
                case MARKER_SOF6:
                case MARKER_SOF7:
                case MARKER_SOF9:
                case MARKER_SOF10:
                case MARKER_SOF11:
                case MARKER_SOF13:
                case MARKER_SOF14:
                case MARKER_SOF15:
                    processSOF(p, marker);
                    p = stepMarker(p, end);
                    if (!decode)
                    {
                        // parse header mode (no decoding)
                        scan_memory = ConstMemory(p, end - p);
                        p = end; // terminate parsing
                    }
                    break;

                case MARKER_RST0:
                case MARKER_RST1:
                case MARKER_RST2:
                case MARKER_RST3:
                case MARKER_RST4:
                case MARKER_RST5:
                case MARKER_RST6:
                case MARKER_RST7:
                    break;

                case MARKER_SOS:
                    if (decode)
                    {
                        p = processSOS(p, end);
                    }
                    break;

                default:
                    debugPrint("[ 0x%x ]\n", marker);
                    header.setError("Incorrect JPEG data.");
                    p = end; // terminate parsing
                    break;
            }

            u64 time1 = Time::us();
            debugPrint("  Time: %d us\n\n", int(time1 - time0));

            MANGO_UNREFERENCED(time0);
            MANGO_UNREFERENCED(time1);
        }
    }

    void Parser::restart()
    {
        if (is_arithmetic)
        {
            decodeState.arithmetic.restart(decodeState.buffer);
        }
        else
        {
            decodeState.huffman.restart();
        }

        // restart
        decodeState.buffer.restart();
    }

    bool Parser::handleRestart()
    {
        if (restartInterval > 0 && !--restartCounter)
        {
            restartCounter = restartInterval;

            if (isRestartMarker(decodeState.buffer.ptr))
            {
                restart();
                decodeState.buffer.ptr += 2;
                return true;
            }
        }

        return false;
    }

    void Parser::configureCPU(SampleType sample, const ImageDecodeOptions& options)
    {
        const char* simd = "";

        u64 flags = options.simd ? getCPUFlags() : 0;
        MANGO_UNREFERENCED(flags);

        // configure idct

        processState.idct = idct8;

#if defined(MANGO_ENABLE_NEON)
        if (flags & ARM_NEON)
        {
            processState.idct = idct_neon;
            m_idct_name = "iDCT: NEON";
        }
#endif

#if defined(MANGO_ENABLE_SSE2)
        if (flags & INTEL_SSE2)
        {
            processState.idct = idct_sse2;
            m_idct_name = "iDCT: SSE2";
        }
#endif

        if (precision == 12)
        {
            // Force 12 bit idct
            // This will round down to 8 bit precision until we have a 12 bit capable color conversion
            processState.idct = idct12;
            m_idct_name = "iDCT: 12 bit";
        }

        // configure block processing

        switch (sample)
        {
            case JPEG_U8_Y:
                processState.process_y           = process_y_8bit;
                processState.process_ycbcr       = process_ycbcr_8bit;
                processState.process_ycbcr_8x8   = nullptr;
                processState.process_ycbcr_8x16  = nullptr;
                processState.process_ycbcr_16x8  = nullptr;
                processState.process_ycbcr_16x16 = nullptr;
                break;
            case JPEG_U8_BGR:
                processState.process_y           = process_y_24bit;
                processState.process_ycbcr       = process_ycbcr_bgr;
                processState.process_ycbcr_8x8   = process_ycbcr_bgr_8x8;
                processState.process_ycbcr_8x16  = process_ycbcr_bgr_8x16;
                processState.process_ycbcr_16x8  = process_ycbcr_bgr_16x8;
                processState.process_ycbcr_16x16 = process_ycbcr_bgr_16x16;
                break;
            case JPEG_U8_RGB:
                processState.process_y           = process_y_24bit;
                processState.process_ycbcr       = process_ycbcr_rgb;
                processState.process_ycbcr_8x8   = process_ycbcr_rgb_8x8;
                processState.process_ycbcr_8x16  = process_ycbcr_rgb_8x16;
                processState.process_ycbcr_16x8  = process_ycbcr_rgb_16x8;
                processState.process_ycbcr_16x16 = process_ycbcr_rgb_16x16;
                break;
            case JPEG_U8_BGRA:
                processState.process_y           = process_y_32bit;
                processState.process_ycbcr       = process_ycbcr_bgra;
                processState.process_ycbcr_8x8   = process_ycbcr_bgra_8x8;
                processState.process_ycbcr_8x16  = process_ycbcr_bgra_8x16;
                processState.process_ycbcr_16x8  = process_ycbcr_bgra_16x8;
                processState.process_ycbcr_16x16 = process_ycbcr_bgra_16x16;
                break;
            case JPEG_U8_RGBA:
                processState.process_y           = process_y_32bit;
                processState.process_ycbcr       = process_ycbcr_rgba;
                processState.process_ycbcr_8x8   = process_ycbcr_rgba_8x8;
                processState.process_ycbcr_8x16  = process_ycbcr_rgba_8x16;
                processState.process_ycbcr_16x8  = process_ycbcr_rgba_16x8;
                processState.process_ycbcr_16x16 = process_ycbcr_rgba_16x16;
                break;
        }

        // CMYK / YCCK
        processState.process_cmyk = process_cmyk_bgra;

#if defined(MANGO_ENABLE_NEON)

        if (flags & ARM_NEON)
        {
            switch (sample)
            {
                case JPEG_U8_Y:
                    break;
                case JPEG_U8_BGR:
                    processState.process_ycbcr_8x8   = process_ycbcr_bgr_8x8_neon;
                    processState.process_ycbcr_8x16  = process_ycbcr_bgr_8x16_neon;
                    processState.process_ycbcr_16x8  = process_ycbcr_bgr_16x8_neon;
                    processState.process_ycbcr_16x16 = process_ycbcr_bgr_16x16_neon;
                    simd = "NEON";
                    break;
                case JPEG_U8_RGB:
                    processState.process_ycbcr_8x8   = process_ycbcr_rgb_8x8_neon;
                    processState.process_ycbcr_8x16  = process_ycbcr_rgb_8x16_neon;
                    processState.process_ycbcr_16x8  = process_ycbcr_rgb_16x8_neon;
                    processState.process_ycbcr_16x16 = process_ycbcr_rgb_16x16_neon;
                    simd = "NEON";
                    break;
                case JPEG_U8_BGRA:
                    processState.process_ycbcr_8x8   = process_ycbcr_bgra_8x8_neon;
                    processState.process_ycbcr_8x16  = process_ycbcr_bgra_8x16_neon;
                    processState.process_ycbcr_16x8  = process_ycbcr_bgra_16x8_neon;
                    processState.process_ycbcr_16x16 = process_ycbcr_bgra_16x16_neon;
                    simd = "NEON";
                    break;
                case JPEG_U8_RGBA:
                    processState.process_ycbcr_8x8   = process_ycbcr_rgba_8x8_neon;
                    processState.process_ycbcr_8x16  = process_ycbcr_rgba_8x16_neon;
                    processState.process_ycbcr_16x8  = process_ycbcr_rgba_16x8_neon;
                    processState.process_ycbcr_16x16 = process_ycbcr_rgba_16x16_neon;
                    simd = "NEON";
                    break;
            }
        }

#endif

#if defined(MANGO_ENABLE_SSE2)

        if (flags & INTEL_SSE2)
        {
            switch (sample)
            {
                case JPEG_U8_Y:
                    break;
                case JPEG_U8_BGR:
                    break;
                case JPEG_U8_RGB:
                    break;
                case JPEG_U8_BGRA:
                    processState.process_ycbcr_8x8   = process_ycbcr_bgra_8x8_sse2;
                    processState.process_ycbcr_8x16  = process_ycbcr_bgra_8x16_sse2;
                    processState.process_ycbcr_16x8  = process_ycbcr_bgra_16x8_sse2;
                    processState.process_ycbcr_16x16 = process_ycbcr_bgra_16x16_sse2;
                    simd = "SSE2";
                    break;
                case JPEG_U8_RGBA:
                    processState.process_ycbcr_8x8   = process_ycbcr_rgba_8x8_sse2;
                    processState.process_ycbcr_8x16  = process_ycbcr_rgba_8x16_sse2;
                    processState.process_ycbcr_16x8  = process_ycbcr_rgba_16x8_sse2;
                    processState.process_ycbcr_16x16 = process_ycbcr_rgba_16x16_sse2;
                    simd = "SSE2";
                    break;
            }
        }

#endif // MANGO_ENABLE_SSE2

#if defined(MANGO_ENABLE_SSE4_1)

        if (flags & INTEL_SSSE3)
        {
            switch (sample)
            {
                case JPEG_U8_Y:
                    break;
                case JPEG_U8_BGR:
                    processState.process_ycbcr_8x8   = process_ycbcr_bgr_8x8_ssse3;
                    processState.process_ycbcr_8x16  = process_ycbcr_bgr_8x16_ssse3;
                    processState.process_ycbcr_16x8  = process_ycbcr_bgr_16x8_ssse3;
                    processState.process_ycbcr_16x16 = process_ycbcr_bgr_16x16_ssse3;
                    simd = "SSSE3";
                    break;
                case JPEG_U8_RGB:
                    processState.process_ycbcr_8x8   = process_ycbcr_rgb_8x8_ssse3;
                    processState.process_ycbcr_8x16  = process_ycbcr_rgb_8x16_ssse3;
                    processState.process_ycbcr_16x8  = process_ycbcr_rgb_16x8_ssse3;
                    processState.process_ycbcr_16x16 = process_ycbcr_rgb_16x16_ssse3;
                    simd = "SSSE3";
                    break;
                case JPEG_U8_BGRA:
                    break;
                case JPEG_U8_RGBA:
                    break;
            }
        }

#endif // MANGO_ENABLE_SSE4_1

        std::string id;

        // determine jpeg type -> select innerloops
        switch (components)
        {
            case 1:
                processState.process = processState.process_y;
                id = "Color: Y";
                break;

            case 3:
                processState.process = processState.process_ycbcr;
                id = "Color: YCbCr";

                // detect optimized cases
                if (blocks_in_mcu <= 6)
                {
                    if (xblock == 8 && yblock == 8)
                    {
                        if (processState.process_ycbcr_8x8)
                        {
                            processState.process = processState.process_ycbcr_8x8;
                            id = makeString("Color: %s YCbCr 8x8", simd);
                        }
                    }

                    if (xblock == 8 && yblock == 16)
                    {
                        if (processState.process_ycbcr_8x16)
                        {
                            processState.process = processState.process_ycbcr_8x16;
                            id = makeString("Color: %s YCbCr 8x16", simd);
                        }
                    }

                    if (xblock == 16 && yblock == 8)
                    {
                        if (processState.process_ycbcr_16x8)
                        {
                            processState.process = processState.process_ycbcr_16x8;
                            id = makeString("Color: %s YCbCr 16x8", simd);
                        }
                    }

                    if (xblock == 16 && yblock == 16)
                    {
                        if (processState.process_ycbcr_16x16)
                        {
                            processState.process = processState.process_ycbcr_16x16;
                            id = makeString("Color: %s YCbCr 16x16", simd);
                        }
                    }
                }
                break;

            case 4:
                processState.process = processState.process_cmyk;
                id = "Color: CMYK";
                break;
        }

        m_ycbcr_name = id;
        debugPrint("  Decoder: %s\n", id.c_str());
    }

    ImageDecodeStatus Parser::decode(const Surface& target, const ImageDecodeOptions& options)
    {
        ImageDecodeStatus status;

        if (!scan_memory.address || !header)
        {
            status.setError(header.info);
            return status;
        }

        // determine if we need a full-surface temporary storage
        if (is_progressive || is_multiscan)
        {
            // allocate blocks
            size_t num_blocks = size_t(mcus) * blocks_in_mcu;
            blockVector.resize(num_blocks * 64);
        }

        // find best matching format
        SampleFormat sf = getSampleFormat(target.format);

        // configure innerloops based on CPU caps
        configureCPU(sf.sample, options);

        // configure multithreading
        m_hardware_concurrency = options.multithread ? ThreadPool::getHardwareConcurrency() : 1;

        if (is_lossless)
        {
            // lossless only supports L8 and BGRA
            if (components == 1)
            {
                sf.sample = JPEG_U8_Y;
                sf.format = LuminanceFormat(8, Format::UNORM, 8, 0);
            }
            else
            {
                sf.sample = JPEG_U8_BGRA;
                sf.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }
        else if (components == 4)
        {
            // CMYK / YCCK is in the slow-path anyway so force BGRA
            sf.sample = JPEG_U8_BGRA;
            sf.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
        }

        status.direct = true;

        if (target.width != xsize || target.height != ysize)
        {
            status.direct = false;
        }

        if (target.format != sf.format)
        {
            status.direct = false;
        }

        // set decoding target surface
        m_surface = &target;

        std::unique_ptr<Bitmap> temp;

        if (!status.direct)
        {
            // create a temporary decoding target
            temp = std::make_unique<Bitmap>(width, height, sf.format);
            m_surface = temp.get();
        }

        parse(scan_memory, true);

        if (!header)
        {
            status.setError(header.info);
            return status;
        }

        if (is_progressive || is_multiscan)
        {
            finishProgressive();
        }

        if (!status.direct)
        {
            target.blit(0, 0, *m_surface);
        }

        blockVector.resize(0);
        status.info = getInfo();

        return status;
    }

    std::string Parser::getInfo() const
    {
        std::string info = m_encoding;

        info += ", ";
        info += m_compression;

        if (!m_idct_name.empty())
        {
            info += ", ";
            info += m_idct_name;
        }

        if (!m_ycbcr_name.empty())
        {
            info += ", ";
            info += m_ycbcr_name;
        }

        if (restartInterval > 0)
        {
            info += " [RST]";
        }

        return info;
    }

    int Parser::getTaskSize(int tasks) const
    {
        constexpr int max_threads = 64;
        const int threads = std::min(m_hardware_concurrency, max_threads);
        const int tasks_per_thread = threads > 1 ? std::max(tasks / threads, 1) : 0;
        //printf("Scheduling %d tasks in %d threads (%d tasks/thread)\n", tasks, threads, tasks_per_thread);
        return tasks_per_thread;
    }

    void Parser::decodeLossless()
    {
        // NOTE: need more test files to make this more conformant
        // NOTE: color sub-sampling is not supported (need test files)

        int predictor = decodeState.spectralStart;
        int pointTransform = decodeState.successiveLow;

        auto decodeFunction = huff_decode_mcu_lossless;
        int* previousDC = decodeState.huffman.last_dc_value;

        if (is_arithmetic)
        {
            decodeFunction = arith_decode_mcu_lossless;
            previousDC = decodeState.arithmetic.last_dc_value;
        }

        const int width = m_surface->width;
        const int height = m_surface->height;
        const int xlast = width - 1;
        const int components = decodeState.comps_in_scan;

        int initPredictor = 1 << (precision - pointTransform - 1);

        std::vector<int> scanLineCache[JPEG_MAX_BLOCKS_IN_MCU];

        for (int i = 0; i < components; ++i)
        {
            scanLineCache[i] = std::vector<int>(width + 1, 0);
        }

        bool first = true;

        for (int y = 0; y < height; ++y)
        {
            u8* image = m_surface->address<u8>(0, y);

            for (int x = 0; x < width; ++x)
            {
                s16 data[JPEG_MAX_BLOCKS_IN_MCU];

                decodeFunction(data, &decodeState);
                bool restarted = handleRestart();
                bool init = restarted | first;
                first = false;

                for (int currentComponent = 0; currentComponent < components; ++currentComponent)
                {
                    // predictors
                    int* cache = scanLineCache[currentComponent].data();
                    int a = data[currentComponent];
                    int b = cache[x + 1];
                    int c = cache[x + 0];

                    int s;

                    if (init)
                        s = initPredictor;
                    else if (predictor == 0) 
                        s = 0;
                    else if (x == xlast)
                        s = cache[0];
                    else if (predictor == 1 || y == 0 || restarted)
                        s = a;
                    else if (predictor == 2)
                        s = b;
                    else if (predictor == 3)
                        s = c;
                    else if (predictor == 4)
                        s = a + b - c;
                    else if (predictor == 5)
                        s = a + ((b - c) >> 1);
                    else if (predictor == 6)
                        s = b + ((a - c) >> 1);
                    else if (predictor == 7)
                        s = (a + b) >> 1;
                    else
                        s = 0;

                    previousDC[currentComponent] = s;

                    cache[x] = data[currentComponent];
                    data[currentComponent] = data[currentComponent] >> (precision - 8);
                }

                if (components == 1)
                {
                    image[0] = byteclamp(data[0] + 128);
                    image += 1;
                }
                else
                {
                    image[0] = byteclamp(data[2] + 128); // blue
                    image[1] = byteclamp(data[1] + 128); // green
                    image[2] = byteclamp(data[0] + 128); // red
                    image[3] = 0xff;
                    image += 4;
                }
            }
        }
    }

    void Parser::decodeSequential()
    {
        int n = getTaskSize(ymcu);
        if (n)
        {
            decodeSequentialMT(n);
        }
        else
        {
            decodeSequentialST();
        }
    }

    void Parser::decodeSequentialST()
    {
        const size_t stride = m_surface->stride;
        const int bytes_per_pixel = m_surface->format.bytes();
        const size_t xstride = bytes_per_pixel * xblock;
        const size_t ystride = stride * yblock;

        u8* image = m_surface->image;

        ProcessFunc process = processState.process;

        const int xmcu_last = xmcu - 1;
        const int ymcu_last = ymcu - 1;

        const int xclip = xsize % xblock;
        const int yclip = ysize % yblock;
        const int xblock_last = xclip ? xclip : xblock;
        const int yblock_last = yclip ? yclip : yblock;

        s16 data[JPEG_MAX_SAMPLES_IN_MCU];

        for (int y = 0; y < ymcu_last; ++y)
        {
            u8* dest = image;
            image += ystride;

            for (int x = 0; x < xmcu_last; ++x)
            {
                decodeState.decode(data, &decodeState);
                handleRestart();
                process(dest, stride, data, &processState, xblock, yblock);
                dest += xstride;
            }

            // last column
            decodeState.decode(data, &decodeState);
            handleRestart();
            process_and_clip(dest, stride, data, xblock_last, yblock);
            dest += xstride;
        }

        // last row
        for (int x = 0; x < xmcu_last; ++x)
        {
            decodeState.decode(data, &decodeState);
            handleRestart();
            process_and_clip(image, stride, data, xblock, yblock_last);
            image += xstride;
        }

        // last mcu
        decodeState.decode(data, &decodeState);
        handleRestart();
        process_and_clip(image, stride, data, xblock_last, yblock_last);
    }

    void Parser::decodeSequentialMT(int N)
    {
        ConcurrentQueue queue("jpeg.sequential", Priority::HIGH);

        if (!restartInterval)
        {
            const int mcu_data_size = blocks_in_mcu * 64;

            for (int y = 0; y < ymcu; y += N)
            {
                const int y0 = y;
                const int y1 = std::min(y + N, ymcu);
                const int count = (y1 - y0) * xmcu;
                debugPrint("  Process: [%d, %d] --> ThreadPool.\n", y0, y1 - 1);

                void* aligned_ptr = aligned_malloc(count * mcu_data_size * sizeof(s16));
                s16* data = reinterpret_cast<s16*>(aligned_ptr);

                for (int i = 0; i < count; ++i)
                {
                    decodeState.decode(data + i * mcu_data_size, &decodeState);
                }

                // enqueue task
                queue.enqueue([=]
                {
                    process_range(y0, y1, data);
                    aligned_free(data);
                });
            }
        }
        else
        {
            const u8* p = decodeState.buffer.ptr;

            const size_t stride = m_surface->stride;
            const int bytes_per_pixel = m_surface->format.bytes();
            const size_t xstride = bytes_per_pixel * xblock;
            const size_t ystride = stride * yblock;

            u8* image = m_surface->image;

            for (int i = 0; i < mcus; i += restartInterval)
            {
                // enqueue task
                queue.enqueue([=]
                {
                    AlignedStorage<s16> data(JPEG_MAX_SAMPLES_IN_MCU);

                    DecodeState state = decodeState;
                    state.buffer.ptr = p;

                    const int left = std::min(restartInterval, mcus - i);

                    const int xmcu_last = xmcu - 1;
                    const int ymcu_last = ymcu - 1;

                    const int xclip = xsize % xblock;
                    const int yclip = ysize % yblock;
                    const int xblock_last = xclip ? xclip : xblock;
                    const int yblock_last = yclip ? yclip : yblock;

                    for (int j = 0; j < left; ++j)
                    {
                        int n = i + j;

                        state.decode(data, &state);

                        int x = n % xmcu;
                        int y = n / xmcu;
                        u8* dest = image + y * ystride + x * xstride;

                        int width = x == xmcu_last ? xblock_last : xblock;
                        int height = y == ymcu_last ? yblock_last : yblock;

                        process_and_clip(dest, stride, data, width, height);
                    }
                });

                // seek next restart marker
                p = seekRestartMarker(p, decodeState.buffer.end);
                if (isRestartMarker(p))
                    p += 2;
            }

            decodeState.buffer.ptr = p;
        }
    }

    void Parser::decodeMultiScan()
    {
        s16* data = blockVector;
        data += decodeState.block[0].offset;

        for (int i = 0; i < mcus; ++i)
        {
            decodeState.decode(data, &decodeState);
            handleRestart();
            data += blocks_in_mcu * 64;
        }
    }

    void Parser::decodeProgressive()
    {
        if (decodeState.spectralStart == 0)
        {
            if (decodeState.comps_in_scan == 1 && decodeState.blocks > 1)
            {
                decodeState.block[0].offset = 0;
                decodeState.blocks = 1;

                decodeProgressiveAC();
            }
            else
            {
                decodeProgressiveDC();
            }
        }
        else
        {
            decodeProgressiveAC();
        }
    }

    void Parser::decodeProgressiveDC()
    {
        if (restartInterval)
        {
            s16* data = blockVector;

            const u8* p = decodeState.buffer.ptr;

            ConcurrentQueue queue("jpeg.progressive", Priority::HIGH);

            for (int i = 0; i < mcus; i += restartInterval)
            {
                // enqueue task
                queue.enqueue([=]
                {
                    DecodeState state = decodeState;
                    state.buffer.ptr = p;

                    s16* dest = data + i * blocks_in_mcu * 64;

                    const int left = std::min(restartInterval, mcus - i);
                    for (int j = 0; j < left; ++j)
                    {
                        state.decode(dest, &state);
                        dest += blocks_in_mcu * 64;
                    }
                });

                // seek next restart marker
                p = seekRestartMarker(p, decodeState.buffer.end);
                if (isRestartMarker(p))
                    p += 2;
            }

            decodeState.buffer.ptr = p;
        }
        else
        {
            s16* data = blockVector;

            for (int i = 0; i < mcus; ++i)
            {
                decodeState.decode(data, &decodeState);
                data += blocks_in_mcu * 64;
            }
        }
    }

    void Parser::decodeProgressiveAC()
    {
        if (restartInterval)
        {
            s16* data = blockVector;

            const int hsf = u32_log2(scanFrame->Hsf);
            const int vsf = u32_log2(scanFrame->Vsf);
            const int hsize = (Hmax >> hsf) * 8;
            const int vsize = (Vmax >> vsf) * 8;

            debugPrint("    hf: %i x %i, log2: %i x %i\n", 1 << hsf, 1 << vsf, hsf, vsf);
            debugPrint("    bs: %i x %i  scanSize: %d\n", hsize, vsize, decodeState.blocks);

            const int scan_offset = scanFrame->offset;

            const int xs = ((xsize + hsize - 1) / hsize);
            const int ys = ((ysize + vsize - 1) / vsize);
            const int cnt = xs * ys;

            debugPrint("    blocks: %d x %d (%d x %d)\n", xs, ys, xs * hsize, ys * vsize);

            MANGO_UNREFERENCED(xs);
            MANGO_UNREFERENCED(ys);
            MANGO_UNREFERENCED(hsize);
            MANGO_UNREFERENCED(vsize);

            const int HMask = (1 << hsf) - 1;
            const int VMask = (1 << vsf) - 1;

            ConcurrentQueue queue("jpeg.progressive", Priority::HIGH);

            const u8* p = decodeState.buffer.ptr;

            for (int i = 0; i < cnt; i += restartInterval)
            {
                // enqueue task
                queue.enqueue([=]
                {
                    DecodeState state = decodeState;
                    state.buffer.ptr = p;

                    const int left = std::min(restartInterval, mcus - i);
                    for (int j = 0; j < left; ++j)
                    {
                        int n = i + j;
                        int x = n % xmcu;
                        int y = n / xmcu;

                        int mcu_yoffset = (y >> vsf) * xmcu;
                        int block_yoffset = ((y & VMask) << hsf) + scan_offset;

                        int mcu_offset = (mcu_yoffset + (x >> hsf)) * blocks_in_mcu;
                        int block_offset = (x & HMask) + block_yoffset;
                        s16* dest = data + (block_offset + mcu_offset) * 64;

                        state.decode(dest, &state);
                    }
                });

                // seek next restart marker
                p = seekRestartMarker(p, decodeState.buffer.end);
                if (isRestartMarker(p))
                    p += 2;
            }

            decodeState.buffer.ptr = p;
        }
        else
        {
            s16* data = blockVector;

            const int hsf = u32_log2(scanFrame->Hsf);
            const int vsf = u32_log2(scanFrame->Vsf);
            const int hsize = (Hmax >> hsf) * 8;
            const int vsize = (Vmax >> vsf) * 8;

            debugPrint("    hf: %i x %i, log2: %i x %i\n", 1 << hsf, 1 << vsf, hsf, vsf);
            debugPrint("    bs: %i x %i  scanSize: %d\n", hsize, vsize, decodeState.blocks);

            const int scan_offset = scanFrame->offset;

            const int xs = ((xsize + hsize - 1) / hsize);
            const int ys = ((ysize + vsize - 1) / vsize);

            debugPrint("    blocks: %d x %d (%d x %d)\n", xs, ys, xs * hsize, ys * vsize);

            const int HMask = (1 << hsf) - 1;
            const int VMask = (1 << vsf) - 1;

            for (int y = 0; y < ys; ++y)
            {
                int mcu_yoffset = (y >> vsf) * xmcu;
                int block_yoffset = ((y & VMask) << hsf) + scan_offset;

                for (int x = 0; x < xs; ++x)
                {
                    int mcu_offset = (mcu_yoffset + (x >> hsf)) * blocks_in_mcu;
                    int block_offset = (x & HMask) + block_yoffset;
                    s16* mcudata = data + (block_offset + mcu_offset) * 64;

                    decodeState.decode(mcudata, &decodeState);
                }
            }
        }
    }

    void Parser::finishProgressive()
    {
        int n = getTaskSize(ymcu);
        if (n)
        {
            ConcurrentQueue queue("jpeg.progressive", Priority::HIGH);

            size_t mcu_stride = size_t(xmcu) * blocks_in_mcu * 64;

            for (int y = 0; y < ymcu; y += n)
            {
                const int y0 = y;
                const int y1 = std::min(y + n, ymcu);

                s16* data = blockVector + y0 * mcu_stride;

                debugPrint("  Process: [%d, %d] --> ThreadPool.\n", y0, y1 - 1);

                // enqueue task
                queue.enqueue([=]
                {
                    process_range(y0, y1, data);
                });
            }
        }
        else
        {
            s16* data = blockVector;
            process_range(0, ymcu, data);
        }
    }

    void Parser::process_range(int y0, int y1, const s16* data)
    {
        const size_t stride = m_surface->stride;
        const size_t bytes_per_pixel = m_surface->format.bytes();
        const size_t xstride = bytes_per_pixel * xblock;
        const size_t ystride = stride * yblock;

        u8* image = m_surface->image;

        const int mcu_data_size = blocks_in_mcu * 64;

        const int xmcu_last = xmcu - 1;
        const int ymcu_last = ymcu - 1;

        const int xclip = xsize % xblock;
        const int yclip = ysize % yblock;
        const int xblock_last = xclip ? xclip : xblock;
        const int yblock_last = yclip ? yclip : yblock;

        for (int y = y0; y < y1; ++y)
        {
            u8* dest = image + y * ystride;
            int height = y == ymcu_last ? yblock_last : yblock;

            for (int x = 0; x < xmcu_last; ++x)
            {
                process_and_clip(dest, stride, data, xblock, height);
                data += mcu_data_size;
                dest += xstride;
            }

            // last column
            process_and_clip(dest, stride, data, xblock_last, height);
            data += mcu_data_size;
            dest += xstride;
        }
    }

    void Parser::process_and_clip(u8* dest, size_t stride, const s16* data, int width, int height)
    {
        if (xblock != width || yblock != height)
        {
            u8 temp[JPEG_MAX_SAMPLES_IN_MCU * 4];

            const int bytes_per_scan = width * m_surface->format.bytes();
            const int block_stride = xblock * 4;
            u8* src = temp;

            processState.process(temp, block_stride, data, &processState, width, height);

            // clipping
            for (int y = 0; y < height; ++y)
            {
                std::memcpy(dest, src, bytes_per_scan);
                src += block_stride;
                dest += stride;
            }
        }
        else
        {
            // fast-path (no clipping required)
            processState.process(dest, stride, data, &processState, width, height);
        }
    }

} // namespace mango::jpeg

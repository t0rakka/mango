/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/endian.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/core/thread.hpp>
#include "jpeg.hpp"

namespace mango::image::jpeg
{

    // ----------------------------------------------------------------------------
    // utilities
    // ----------------------------------------------------------------------------

    static inline
    bool isRestartMarker(const u8* p)
    {
        bool is = false;

        if (p[0] == 0xff)
        {
            int index = p[1] - 0xd0;
            is = index >= 0 && index <= 7;
        }

        return is;
    }

    static
    void jpegPrintMemory(const u8* ptr)
    {
        if (isEnable(Print::Info))
        {
            printf("  Memory: ");

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
        for (auto sampleFormat : g_format_table)
        {
            if (format == sampleFormat.format)
            {
                return sampleFormat;
            }
        }

        // set default format
        return { JPEG_U8_RGBA, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8) };
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

        while (remain <= (JPEG_REGISTER_BITS - 8))
        {
            const u8* x = ptr;

            int value = ptr < end ? *ptr++ : 0;
            if (value == 0xff)
            {
                int b = ptr < end ? *ptr++ : 0;
                if (b)
                {
                    // Found a marker; keep returning zero until it has been processed
                    ptr = x;
                    value = 0;
                }
            }

            remain += 8;
            data = (data << 8) | value;
        }
    }

    // ----------------------------------------------------------------------------
    // Parser
    // ----------------------------------------------------------------------------

    Parser::Parser(ConstMemory memory)
        : m_memory(memory)
        , quantTableVector(64 * JPEG_MAX_COMPS_IN_SCAN)
    {
        restartInterval = 0;
        restartCounter = 0;

        for (int i = 0; i < JPEG_MAX_COMPS_IN_SCAN; ++i)
        {
            quantTable[i].table = quantTableVector.data() + i * 64;
        }

        m_surface = nullptr;
    }

    Parser::~Parser()
    {
    }

    void Parser::setInterface(ImageDecodeInterface* interface)
    {
        m_interface = interface;

        if (isJPEG(m_memory))
        {
            parse(m_memory, false);
        }
        else
        {
            header.setError("Incorrect SOI marker.");
        }
    }

    bool Parser::isJPEG(ConstMemory memory) const
    {
        if (!memory.address || memory.size < 4)
            return false;

        if (bigEndian::uload16(memory.address) != MARKER_SOI)
            return false;

#if 0
        // Scan for EOI marker
        const u8* p = memory.address + memory.size - 2;
        for (int i = 0; i < 32; ++i, --p)
        {
            u16 marker = bigEndian::uload16(p);
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

        u16 size = bigEndian::uload16(p);
        p += size;

        if (p + 1 > end)
            return end;

        // HACK: some really ancient jpeg encoders encode markers sometimes as
        // (0xff, 0xff, ID) ; this will skip to the "correct" 0xff (the second one)
        p += (p[1] == 0xff);

        return p;
    }

    const u8* Parser::seekMarker(const u8* start, const u8* end) const
    {
        const u8* p = start;
        --end; // markers are two bytes: don't look at the last byte

        while (p < end)
        {
            p = mango::memchr(p, 0xff, end - p);
            if (!p)
            {
                // found nothing
                return end + 1;
            }
            else if (p[1])
            {
                if (p[1] == 0xff)
                {
                    // workaround for ancient encoder using 0xff, 0xff as padding
                    p += 2;
                    continue;
                }

                // found a marker
                return p;
            }

            // skip stuff byte (0xff, 0x00)
            p += 2;
        }

        printLine(Print::Info, "  Seek: {} bytes", p - start);

        // found nothing
        return end + 1;
    }

    void Parser::processSOI()
    {
        printLine(Print::Info, "[ SOI ]");
        restartInterval = 0;
    }

    void Parser::processEOI()
    {
        printLine(Print::Info, "[ EOI ]");
    }

    void Parser::processCOM(const u8* p)
    {
        printLine(Print::Info, "[ COM ]");
        MANGO_UNREFERENCED(p);
    }

    void Parser::processTEM(const u8* p)
    {
        printLine(Print::Info, "[ TEM ]");
        MANGO_UNREFERENCED(p);
    }

    void Parser::processRES(const u8* p)
    {
        printLine(Print::Info, "[ RES ]");

        // Reserved for jpeg extensions
        MANGO_UNREFERENCED(p);
    }

    void Parser::processJPG(const u8* p)
    {
        printLine(Print::Info, "[ JPG ]");

        // Reserved for jpeg extensions
        MANGO_UNREFERENCED(p);
    }

    void Parser::processJPG(const u8* p, u16 marker)
    {
        printLine(Print::Info, "[ JPG{} ]", marker - MARKER_JPG0);

        // Reserved for jpeg extensions
        MANGO_UNREFERENCED(p);
        MANGO_UNREFERENCED(marker);
    }

    void Parser::processAPP(const u8* p, u16 marker)
    {
        printLine(Print::Info, "[ APP{} ]", marker - MARKER_APP0);

        int size = bigEndian::uload16(p) - 2;
        p += 2;

        switch (marker)
        {
            case MARKER_APP0:
            {
                const u8 magicJFIF [] = { 0x4a, 0x46, 0x49, 0x46, 0 }; // 'JFIF', 0
                const u8 magicJFXX [] = { 0x4a, 0x46, 0x58, 0x58, 0 }; // 'JFXX', 0
                const size_t magicJFIF_length = 5;

                if (size >= magicJFIF_length &&
                    (!std::memcmp(p, magicJFIF, magicJFIF_length) || !std::memcmp(p, magicJFXX, magicJFIF_length)))
                {
                    // Skip the identifier
                    p += magicJFIF_length;
                    size -= magicJFIF_length;

                    printLine(Print::Info, "  JFIF: {} bytes", size);

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

                    printLine(Print::Info, "    version: {}", version);
                    printLine(Print::Info, "    density: {} x {} {}", Xdensity, Ydensity, unit_str);
                    printLine(Print::Info, "    thumbnail: {} x {}", Xthumbnail, Ythumbnail);

                    // NOTE: This marker is parsed but JFIF is not supported
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
                const u8 magicExif0 [] = { 0x45, 0x78, 0x69, 0x66, 0, 0 }; // 'Exif', 0, 0
                const u8 magicExif255 [] = { 0x45, 0x78, 0x69, 0x66, 0, 0xff }; // 'Exif', 0, 0xff
                const size_t magicExif_length = 6;

                const char* xmp_id = "http://ns.adobe.com/xap/1.0/";
                const size_t xmp_id_length = std::strlen(xmp_id) + 1; // +1 for null terminator

                if (size >= magicExif_length &&
                    (!std::memcmp(p, magicExif0, magicExif_length) || !std::memcmp(p, magicExif255, magicExif_length)))
                {
                    // Skip the identifier
                    p += magicExif_length;
                    size -= magicExif_length;

                    exif_memory = ConstMemory(p, size);
                    printLine(Print::Info, "  EXIF: {} bytes", size);
                }
                else if (size >= xmp_id_length && !std::memcmp(p, xmp_id, xmp_id_length))
                {
                    // Skip the identifier
                    p += xmp_id_length;
                    size -= xmp_id_length;
                    printLine(Print::Info, "  XMP: {} bytes", size);
                    // NOTE: We don't support XMP but this is where it would be processed
                }

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

                    printLine(Print::Info, "  ICC: {} / {} ({} bytes)", sequence_number, sequence_total, size);
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
                    printLine(Print::Info, "  EXIF: {} bytes", size);
                }

                break;
            }

            case MARKER_APP14:
            {
                const u8 magic_adobe[] = { 0x41, 0x64, 0x6f, 0x62, 0x65 }; // 'Adobe'
                const u8 magic_mango[] = { 0x4d, 0x61, 0x6e, 0x67, 0x6f, 0x31 }; // 'Mango1'

                if (size == 12 && !std::memcmp(p, magic_adobe, 5))
                {
                    u16 version = bigEndian::uload16(p + 5);
                    //u16 flags0 = bigEndian::uload16(p + 7);
                    //u16 flags1 = bigEndian::uload16(p + 9);
                    u8 color_transform = p[11]; // 0 - CMYK, 1 - YCbCr, 2 - YCCK
                    if (color_transform <= 2)
                    {
                        processState.colorspace = ColorSpace(color_transform);
                    }
                    printLine(Print::Info, "  Version: {}", version);
                    printLine(Print::Info, "  ColorTransform: {}", color_transform);
                    MANGO_UNREFERENCED(version);
                    MANGO_UNREFERENCED(color_transform);
                }
                else if (!std::memcmp(p, magic_mango, 6))
                {
                    m_decode_interval = bigEndian::uload32(p + 6);
                    p += 10;
                    int intervals = (size - 10) / sizeof(u32);

                    for (int i = 0; i < intervals; ++i)
                    {
                        u32 offset = bigEndian::uload32(p);
                        p += sizeof(u32);
                        m_restart_offsets.push_back(offset);
                    }
                }
                break;
            }
        }
    }

    void Parser::processSOF(const u8* p, u16 marker)
    {
        printLine(Print::Info, "[ SOF{} ]", marker - MARKER_SOF0);

        is_baseline = (marker == MARKER_SOF0);
        is_progressive = false;
        is_multiscan = false;
        is_lossless = false;
        is_differential = false;

        u16 length = bigEndian::uload16(p + 0);
        m_precision = p[2];
        m_height = bigEndian::uload16(p + 3);
        m_width  = bigEndian::uload16(p + 5);
        m_components = p[7];
        p += 8;

        printLine(Print::Info, "  Image: {} x {} x {}", m_width, m_height, m_precision);

        u16 correct_length = 8 + 3 * m_components;
        if (length != correct_length)
        {
            header.setError("Incorrect chunk length ({}, should be {}).", length, correct_length);
            return;
        }

        if (m_width <= 0 || m_height <= 0 || m_width > 65535 || m_height > 65535)
        {
            // NOTE: ysize of 0 is allowed in the specs but we won't
            header.setError("Incorrect dimensions ({} x {})", m_width, m_height);
            return;
        }

        if (m_components < 1 || m_components > 4)
        {
            // NOTE: only progressive is required to have 1..4 components,
            //       other modes allow 1..255 but we are extra strict here :)
            header.setError("Incorrect number of components ({})", m_components);
            return;
        }

        decodeState.is_arithmetic = marker > MARKER_SOF7;
        m_compression = decodeState.is_arithmetic ? "Arithmetic" : "Huffman";

        switch (marker)
        {
            // Huffman
            case MARKER_SOF0:  m_encoding = "Baseline DCT"; break;
            case MARKER_SOF1:  m_encoding = "Extended sequential DCT"; break;
            case MARKER_SOF2:  m_encoding = "Progressive DCT"; break;
            case MARKER_SOF3:  m_encoding = "Lossless"; break;
            case MARKER_SOF5:  m_encoding = "Differential sequential DCT"; break;
            case MARKER_SOF6:  m_encoding = "Differential progressive DCT"; break;
            case MARKER_SOF7:  m_encoding = "Differential lossless"; break;

            // Arithmetic
            case MARKER_SOF9:  m_encoding = "Extended sequential DCT"; break;
            case MARKER_SOF10: m_encoding = "Progressive DCT"; break;
            case MARKER_SOF11: m_encoding = "Lossless"; break;
            case MARKER_SOF13: m_encoding = "Differential sequential DCT"; break;
            case MARKER_SOF14: m_encoding = "Differential progressive DCT"; break;
            case MARKER_SOF15: m_encoding = "Differential lossless"; break;
        }

        printLine(Print::Info, "  Encoding: {}", m_encoding);
        printLine(Print::Info, "  Compression: {}", m_compression);

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
                [[fallthrough]];
            case MARKER_SOF3:
            case MARKER_SOF11:
                is_lossless = true;
                break;
        }

        if (is_baseline)
        {
            if (m_precision != 8)
            {
                header.setError("Incorrect precision ({}, allowed: 8)", m_precision);
                return;
            }
        }
        else if (is_lossless)
        {
            if (m_precision < 2 || m_precision > 16)
            {
                header.setError("Incorrect precision ({}, allowed: 2..16)", m_precision);
                return;
            }
        }
        else
        {
            if (m_precision != 8 && m_precision != 12)
            {
                header.setError("Incorrect precision ({}, allowed: 8, 12)", m_precision);
                return;
            }
        }

        Hmax = 0;
        Vmax = 0;
        blocks_in_mcu = 0;
        int offset = 0;

        processState.frames = m_components;

        for (int i = 0; i < m_components; ++i)
        {
            if (offset >= JPEG_MAX_BLOCKS_IN_MCU)
            {
                header.setError("Incorrect blocks offset ({} >= {}).", offset, JPEG_MAX_BLOCKS_IN_MCU);
                return;
            }

            Frame& frame = processState.frame[i];

            frame.compid = p[0];
            u8 hv = p[1];
            frame.hsf = (hv >> 4) & 0xf;
            frame.vsf = (hv >> 0) & 0xf;
            frame.tq = p[2];
            frame.offset = offset;
            p += 3;

            u8 max_tq = is_lossless ? 0 : 3;
            if (frame.tq > max_tq)
            {
                header.setError("Incorrect quantization table index ({})", frame.tq);
                return;
            }

            if (m_components == 1)
            {
                // Optimization: force block size to 8x8 with grayscale images
                frame.hsf = 1;
                frame.vsf = 1;
            }

            Hmax = std::max(Hmax, frame.hsf);
            Vmax = std::max(Vmax, frame.vsf);
            blocks_in_mcu += frame.hsf * frame.vsf;

            if (frame.hsf < 1 || frame.hsf > 4 || frame.vsf < 1 || frame.vsf > 4)
            {
                header.setError("Incorrect frame sampling rate ({} x {})", frame.hsf, frame.vsf);
                return;
            }

            if (blocks_in_mcu > JPEG_MAX_BLOCKS_IN_MCU)
            {
                header.setError("Incorrect number of blocks in MCU ({} >= {}).", blocks_in_mcu, JPEG_MAX_BLOCKS_IN_MCU);
                return;
            }

            for (int y = 0; y < frame.vsf; ++y)
            {
                for (int x = 0; x < frame.hsf; ++x)
                {
                    processState.block[offset].qt = quantTable[frame.tq].table;
                    if (!processState.block[offset].qt)
                    {
                        header.setError("No quantization table for index ({})", frame.tq);
                        return;
                    }

                    ++offset;
                }
            }

            printLine(Print::Info, "  Frame: {}, compid: {}, Hsf: {}, Vsf: {}, Tq: {}, offset: {}",
                i, frame.compid, frame.hsf, frame.vsf, frame.tq, frame.offset);

            frames.push_back(frame);
        }

        processState.blocks = offset;

        xblock = 8 * Hmax;
        yblock = 8 * Vmax;

        if (!xblock || !yblock)
        {
            header.setError("Incorrect dimensions ({} x {})", xblock, yblock);
            return;
        }

        printLine(Print::Info, "  Blocks in MCU: {}", blocks_in_mcu);
        printLine(Print::Info, "  MCU: {} x {}", xblock, yblock);

        // Align to next MCU boundary
        int xmask = xblock - 1;
        int ymask = yblock - 1;
        m_aligned_width  = (m_width  + xmask) & ~xmask;
        m_aligned_height = (m_height + ymask) & ~ymask;

        // MCU resolution
        xmcu = m_aligned_width  / xblock;
        ymcu = m_aligned_height / yblock;
        mcus = xmcu * ymcu;

        printLine(Print::Info, "  {} MCUs ({} x {}) -> ({} x {})", mcus, xmcu, ymcu, xmcu * xblock, ymcu * yblock);
        printLine(Print::Info, "  Image: {} x {}", m_width, m_height);

        // configure header
        header.width  = m_width;
        header.height = m_height;
        header.format = m_components > 1 ? Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8)
                                         : LuminanceFormat(8, Format::UNORM, 8, 0);

        MANGO_UNREFERENCED(length);
    }

    const u8* Parser::processSOS(const u8* p, const u8* end)
    {
        printLine(Print::Info, "[ SOS ]");

        u16 length = bigEndian::uload16(p);
        u8 components = p[2]; // Ns
        p += 3;

        u16 correct_length = 6 + 2 * components;
        if (length != correct_length)
        {
            header.setError("Incorrect chunk length ({}, should be {}).", length, correct_length);
            return p;
        }

        if (components < 1 || components > 4)
        {
            header.setError("Incorrect number of components ({}).", components);
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

        printLine(Print::Info, "  components: {}{}", components, is_multiscan ? " (MultiScan)" : "");
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
                header.setError("Incorrect coding table selector (DC: {}, AC: {}).", dc, ac);
                return p;
            }

            // find frame
            Frame* frame = nullptr;
            int pred = 0;

            for (size_t j = 0; j < frames.size(); ++j)
            {
                if (frames[j].compid == cs)
                {
                    pred = int(j);
                    frame = &frames[j];
                }
            }

            if (!frame)
            {
                header.setError("Incorrect scan component selector ({})", cs);
                return p;
            }

            scanFrame = frame;

            const int size = frame->hsf * frame->vsf;
            int offset = frame->offset;

            std::string cs_name;
            if (cs >= 32 && cs < 128)
            {
                cs_name = fmt::format(" ({:c})", cs);
            }

            printLine(Print::Info, "  Component: {}{}, DC: {}, AC: {}, offset: {}, size: {}",
                cs, cs_name, dc, ac, frame->offset, size);

            for (int j = 0; j < size; ++j)
            {
                if (offset >= JPEG_MAX_BLOCKS_IN_MCU)
                {
                    header.setError("Incorrect number of blocks in MCU ({} >= {}).", offset, JPEG_MAX_BLOCKS_IN_MCU);
                    return p;
                }

                DecodeBlock& block = decodeState.block[decodeState.blocks];

                block.offset = offset * 64;
                block.pred = pred;
                block.dc = dc;
                block.ac = ac;

                printLine(Print::Info, "      - offset: {}, pred: {},", offset * 64, pred);
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

        decodeState.spectral_start = Ss;
        decodeState.spectral_end = Se;
        decodeState.successive_low = Al;
        decodeState.successive_high = Ah;

        printLine(Print::Info, "  Spectral range: ({}, {})", Ss, Se);

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

        bool dc_scan = (decodeState.spectral_start == 0);
        bool refine_scan = (decodeState.successive_high != 0);

        restartCounter = restartInterval;

        m_request_blitting = is_lossless || is_multiscan;

        if (decodeState.is_arithmetic)
        {
            ArithmeticDecoder& arithmetic = decodeState.arithmetic;

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
                        printLine(Print::Info, "  * decode_dc_first()");
                        decodeState.decode = arith_decode_dc_first;
                    }
                    else
                    {
                        printLine(Print::Info, "  * decode_dc_refine()");
                        decodeState.decode = arith_decode_dc_refine;
                    }
                }
                else
                {
                    if (!refine_scan)
                    {
                        printLine(Print::Info, "  * decode_ac_first()");
                        decodeState.decode = arith_decode_ac_first;
                    }
                    else
                    {
                        printLine(Print::Info, "  * decode_ac_refine()");
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

            p = decodeState.buffer.ptr;
        }
        else
        {
            HuffmanDecoder& huffman = decodeState.huffman;

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
                        printLine(Print::Info, "  * decode_dc_first()");
                        decodeState.decode = huff_decode_dc_first;
                    }
                    else
                    {
                        printLine(Print::Info, "  * decode_dc_refine()");
                        decodeState.decode = huff_decode_dc_refine;
                    }
                }
                else
                {
                    if (!refine_scan)
                    {
                        printLine(Print::Info, "  * decode_ac_first()");
                        decodeState.decode = huff_decode_ac_first;
                    }
                    else
                    {
                        printLine(Print::Info, "  * decode_ac_refine()");
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

            p = decodeState.buffer.ptr;
        }

        jpegPrintMemory(p);

        return p;
    }

    void Parser::processDQT(const u8* p)
    {
        printLine(Print::Info, "[ DQT ]");

        u16 Lq = bigEndian::uload16(p); // Quantization table definition length
        p += 2;
        Lq -= 2;

        for ( ; Lq > 0; )
        {
            u8 x = *p++;
            u8 Pq = (x >> 4) & 0xf; // Quantization table element precision (0 = 8 bits, 1 = 16 bits)
            u8 Tq = (x >> 0) & 0xf; // Quantization table destination identifier (0..3)

            const int bits = (Pq + 1) * 8;
            printLine(Print::Info, "  Quantization table #{} element precision: {} bits", Tq, bits);

            if (!is_lossless)
            {
                //const u8 max_pq = is_baseline ? 0 : 1;
                const u8 max_pq = 1; // relaxed limit; lots of non-conforming files out in the wild

                if (Pq > max_pq)
                {
                    header.setError("Incorrect quantization table element precision ({})", Pq);
                    return;
                }

                if (Tq > 3)
                {
                    header.setError("Incorrect quantization table ({})", Tq);
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
                        table.table[zigzagTable[i]] = *p++;
                    }
                    break;

                case 1:
                    for (int i = 0; i < 64; ++i)
                    {
                        table.table[zigzagTable[i]] = bigEndian::uload16(p);
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
        printLine(Print::Info, "[ DHT ]");

        int Lh = bigEndian::uload16(p); // Huffman table definition length
        p += 2;
        Lh -= 2;

        std::vector<HuffmanTable*> tables;

        for ( ; Lh > 0; )
        {
            u8 x = p[0];
            u8 Tc = (x >> 4) & 0xf; // Table class - 0 = DC table or lossless table, 1 = AC table.
            u8 Th = (x >> 0) & 0xf; // Huffman table identifier

            u8 max_tc = is_lossless ? 0 : 1; 
            u8 max_th = is_baseline ? 1 : 3;

            if (Tc > max_tc)
            {
                header.setError("Incorrect huffman table class ({})", Tc);
                return;
            }

            if (Th > max_th)
            {
                header.setError("Incorrect huffman table identifier ({})", Th);
                return;
            }

            HuffmanTable& table = decodeState.huffman.table[Tc][Th];
            decodeState.huffman.maxTc = std::max(decodeState.huffman.maxTc, int(Tc));
            decodeState.huffman.maxTh = std::max(decodeState.huffman.maxTh, int(Th));

            printLine(Print::Info, "  Huffman table #{} table class: {}", Th, Tc);

            if (p >= end - 17)
            {
                header.setError("Data overflow.");
                return;
            }

            std::string ms = "    codes: ";

            int count = 0;

            for (int i = 1; i <= 16; ++i)
            {
                u8 L = p[i]; // Number of Huffman codes of length i bits
                table.size[i] = L;
                count += L;
                ms += fmt::format("{} ", L);
            }

            printLine(Print::Info, ms);

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
        for (HuffmanTable* table : tables)
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
        printLine(Print::Info, "[ DAC ]");

        u16 La = bigEndian::uload16(p); // Arithmetic coding conditioning definition length
        p += 2;

        if (is_baseline)
        {
            header.setError("BaselineDCT does not support Arithmetic Coding tables.");
            return;
        }

        int n = (La - 2) / 2;

        printLine(Print::Info, "  n: {}", n);

        if (n > 32)
        {
            header.setError("Too many DAC entries ({}).", n);
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
                header.setError("Incorrect Arithmetic table selector (Tc: {}, Tb: {}).", Tc, Tb);
                return;
            }

            if (Cs < min_cs || Cs > max_cs)
            {
                header.setError("Incorrect Arithmetic conditioning table ({}).", Cs);
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
                    header.setError("Incorrect Arithmetic table class ({}).", Tc);
                    return;
            }

            printLine(Print::Info, "  Tc: {}, Tb: {}, Cs: {}", Tc, Tb, Cs);
        }
    }

    void Parser::processDNL(const u8* p)
    {
        printLine(Print::Info, "[ DNL ]");

        u16 Ld = bigEndian::uload16(p + 0); // Define number of lines segment length
        u16 NL = bigEndian::uload16(p + 2); // Number of lines

        // NOTE: ysize = NL, but cannot test this until find files that use this marker
        MANGO_UNREFERENCED(NL);
        MANGO_UNREFERENCED(Ld);
    }

    void Parser::processDRI(const u8* p)
    {
        printLine(Print::Info, "[ DRI ]");

        int Lh = bigEndian::uload16(p + 0); // length
        if (Lh != 4)
        {
            // signal error
        }

        restartInterval = bigEndian::uload16(p + 2); // number of MCU in restart interval
        printLine(Print::Info, "  Restart interval: {}", restartInterval);
    }

    void Parser::processDHP(const u8* p)
    {
        printLine(Print::Info, "[ DHP ]");

        // NOTE: This marker is parsed but not used

        // Get length of DHP segment (includes the length bytes)
        const int length = bigEndian::uload16(p) - 2;
        p += 2;

        // Get precision (usually 8 or 12 bits)
        const int precision = *p++;

        // Number of components in the frame
        const int ncomponents = *p++;

        printLine(Print::Info, "  Precision: {}", precision);
        printLine(Print::Info, "  Components: {}", ncomponents);

        // Parse component specifications
        for (int i = 0; i < ncomponents; ++i)
        {
            const int id = *p++;
            const int sampling = *p++;
            const int h_sampling = (sampling >> 4) & 0x0F;  // horizontal sampling factor
            const int v_sampling = sampling & 0x0F;         // vertical sampling factor
            const int quant_table_selector = *p++;

            printLine(Print::Info, "  Component[{}]:", i);
            printLine(Print::Info, "    ID: {}", id);
            printLine(Print::Info, "    Sampling: {}x{}", h_sampling, v_sampling);
            printLine(Print::Info, "    Quant Table: {}", quant_table_selector);
        }
    }

    void Parser::processEXP(const u8* p)
    {
        printLine(Print::Info, "[ EXP ]");

        u16 Le = bigEndian::uload16(p); // Expand reference components segment length
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
        const u8* end = memory.end();
        const u8* p = memory.address;

        for ( ; p < end; )
        {
            if (!header)
            {
                // we are in error state -> abort parsing
                break;
            }

            if (m_interface->cancelled)
            {
                // decoding is cancelled
                break;
            }

            u16 marker = bigEndian::uload16(p);
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
                    p = seekMarker(p, end);
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
                    printLine(Print::Info, "[ {:#x} ]", marker);
                    header.setError("Incorrect JPEG data.");
                    p = end; // terminate parsing
                    break;
            }

            u64 time1 = Time::us();
            printLine(Print::Info, "  Time: {} us", time1 - time0);
            printLine(Print::Info, "");

            MANGO_UNREFERENCED(time0);
            MANGO_UNREFERENCED(time1);
        }
    }

    bool Parser::handleRestart()
    {
        if (restartInterval > 0 && !--restartCounter)
        {
            restartCounter = restartInterval;

            if (isRestartMarker(decodeState.buffer.ptr))
            {
                decodeState.restart();
                decodeState.buffer.ptr += 2;
                return true;
            }
        }

        return false;
    }

    void Parser::configureCPU(SampleType sample, const ImageDecodeOptions& options)
    {
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

        if (m_precision == 12)
        {
            // Force 12 bit idct
            // This will round down to 8 bit precision until we have a 12 bit capable color conversion
            processState.idct = idct12;
            m_idct_name = "iDCT: 12 bit";
        }

        // configure block processing

        using ProcessFunc = void (*)(u8* dest, size_t stride, const s16* data, ProcessState* state, int width, int height);

        ProcessFunc process_y           = nullptr;
        ProcessFunc process_cmyk        = process_cmyk_rgba;
        ProcessFunc process_ycbcr       = nullptr;
        ProcessFunc process_ycbcr_8x8   = nullptr;
        ProcessFunc process_ycbcr_8x16  = nullptr;
        ProcessFunc process_ycbcr_16x8  = nullptr;
        ProcessFunc process_ycbcr_16x16 = nullptr;

        switch (sample)
        {
            case JPEG_U8_Y:
                process_y           = process_y_8bit;
                process_ycbcr       = process_ycbcr_8bit;
                process_ycbcr_8x8   = nullptr;
                process_ycbcr_8x16  = nullptr;
                process_ycbcr_16x8  = nullptr;
                process_ycbcr_16x16 = nullptr;
                break;
            case JPEG_U8_BGR:
                process_y           = process_y_24bit;
                process_ycbcr       = process_ycbcr_bgr;
                process_ycbcr_8x8   = process_ycbcr_bgr_8x8;
                process_ycbcr_8x16  = process_ycbcr_bgr_8x16;
                process_ycbcr_16x8  = process_ycbcr_bgr_16x8;
                process_ycbcr_16x16 = process_ycbcr_bgr_16x16;
                break;
            case JPEG_U8_RGB:
                process_y           = process_y_24bit;
                process_ycbcr       = process_ycbcr_rgb;
                process_ycbcr_8x8   = process_ycbcr_rgb_8x8;
                process_ycbcr_8x16  = process_ycbcr_rgb_8x16;
                process_ycbcr_16x8  = process_ycbcr_rgb_16x8;
                process_ycbcr_16x16 = process_ycbcr_rgb_16x16;
                break;
            case JPEG_U8_BGRA:
                process_y           = process_y_32bit;
                process_ycbcr       = process_ycbcr_bgra;
                process_ycbcr_8x8   = process_ycbcr_bgra_8x8;
                process_ycbcr_8x16  = process_ycbcr_bgra_8x16;
                process_ycbcr_16x8  = process_ycbcr_bgra_16x8;
                process_ycbcr_16x16 = process_ycbcr_bgra_16x16;
                break;
            case JPEG_U8_RGBA:
                process_y           = process_y_32bit;
                process_ycbcr       = process_ycbcr_rgba;
                process_ycbcr_8x8   = process_ycbcr_rgba_8x8;
                process_ycbcr_8x16  = process_ycbcr_rgba_8x16;
                process_ycbcr_16x8  = process_ycbcr_rgba_16x8;
                process_ycbcr_16x16 = process_ycbcr_rgba_16x16;
                break;
        }

        const char* simd_8x8   = "";
        const char* simd_8x16  = "";
        const char* simd_16x8  = "";
        const char* simd_16x16 = "";

#if defined(MANGO_ENABLE_NEON)

        if (flags & ARM_NEON)
        {
            switch (sample)
            {
                case JPEG_U8_Y:
                    break;
                case JPEG_U8_BGR:
                    process_ycbcr_8x8   = process_ycbcr_bgr_8x8_neon;
                    process_ycbcr_8x16  = process_ycbcr_bgr_8x16_neon;
                    process_ycbcr_16x8  = process_ycbcr_bgr_16x8_neon;
                    process_ycbcr_16x16 = process_ycbcr_bgr_16x16_neon;
                    simd_8x8   = "NEON";
                    simd_8x16  = "NEON";
                    simd_16x8  = "NEON";
                    simd_16x16 = "NEON";
                    break;
                case JPEG_U8_RGB:
                    process_ycbcr_8x8   = process_ycbcr_rgb_8x8_neon;
                    process_ycbcr_8x16  = process_ycbcr_rgb_8x16_neon;
                    process_ycbcr_16x8  = process_ycbcr_rgb_16x8_neon;
                    process_ycbcr_16x16 = process_ycbcr_rgb_16x16_neon;
                    simd_8x8   = "NEON";
                    simd_8x16  = "NEON";
                    simd_16x8  = "NEON";
                    simd_16x16 = "NEON";
                    break;
                case JPEG_U8_BGRA:
                    process_ycbcr_8x8   = process_ycbcr_bgra_8x8_neon;
                    process_ycbcr_8x16  = process_ycbcr_bgra_8x16_neon;
                    process_ycbcr_16x8  = process_ycbcr_bgra_16x8_neon;
                    process_ycbcr_16x16 = process_ycbcr_bgra_16x16_neon;
                    simd_8x8   = "NEON";
                    simd_8x16  = "NEON";
                    simd_16x8  = "NEON";
                    simd_16x16 = "NEON";
                    break;
                case JPEG_U8_RGBA:
                    process_ycbcr_8x8   = process_ycbcr_rgba_8x8_neon;
                    process_ycbcr_8x16  = process_ycbcr_rgba_8x16_neon;
                    process_ycbcr_16x8  = process_ycbcr_rgba_16x8_neon;
                    process_ycbcr_16x16 = process_ycbcr_rgba_16x16_neon;
                    simd_8x8   = "NEON";
                    simd_8x16  = "NEON";
                    simd_16x8  = "NEON";
                    simd_16x16 = "NEON";
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
                    process_ycbcr_8x8   = process_ycbcr_bgra_8x8_sse2;
                    process_ycbcr_8x16  = process_ycbcr_bgra_8x16_sse2;
                    process_ycbcr_16x8  = process_ycbcr_bgra_16x8_sse2;
                    process_ycbcr_16x16 = process_ycbcr_bgra_16x16_sse2;
                    simd_8x8   = "SSE2";
                    simd_8x16  = "SSE2";
                    simd_16x8  = "SSE2";
                    simd_16x16 = "SSE2";
                    break;
                case JPEG_U8_RGBA:
                    process_ycbcr_8x8   = process_ycbcr_rgba_8x8_sse2;
                    process_ycbcr_8x16  = process_ycbcr_rgba_8x16_sse2;
                    process_ycbcr_16x8  = process_ycbcr_rgba_16x8_sse2;
                    process_ycbcr_16x16 = process_ycbcr_rgba_16x16_sse2;
                    simd_8x8   = "SSE2";
                    simd_8x16  = "SSE2";
                    simd_16x8  = "SSE2";
                    simd_16x16 = "SSE2";
                    break;
            }
        }

#endif // MANGO_ENABLE_SSE2

#if defined(MANGO_ENABLE_SSE4_1)

        if (flags & INTEL_SSE4_1)
        {
            switch (sample)
            {
                case JPEG_U8_Y:
                    break;
                case JPEG_U8_BGR:
                    process_ycbcr_8x8   = process_ycbcr_bgr_8x8_sse41;
                    process_ycbcr_8x16  = process_ycbcr_bgr_8x16_sse41;
                    process_ycbcr_16x8  = process_ycbcr_bgr_16x8_sse41;
                    process_ycbcr_16x16 = process_ycbcr_bgr_16x16_sse41;
                    simd_8x8   = "SSE4.1";
                    simd_8x16  = "SSE4.1";
                    simd_16x8  = "SSE4.1";
                    simd_16x16 = "SSE4.1";
                    break;
                case JPEG_U8_RGB:
                    process_ycbcr_8x8   = process_ycbcr_rgb_8x8_sse41;
                    process_ycbcr_8x16  = process_ycbcr_rgb_8x16_sse41;
                    process_ycbcr_16x8  = process_ycbcr_rgb_16x8_sse41;
                    process_ycbcr_16x16 = process_ycbcr_rgb_16x16_sse41;
                    simd_8x8   = "SSE4.1";
                    simd_8x16  = "SSE4.1";
                    simd_16x8  = "SSE4.1";
                    simd_16x16 = "SSE4.1";
                    break;
                case JPEG_U8_BGRA:
                    break;
                case JPEG_U8_RGBA:
                    break;
            }
        }

#endif // MANGO_ENABLE_SSE4_1

#if defined(MANGO_ENABLE_AVX2)

        if (flags & INTEL_AVX2)
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
                    break;
                case JPEG_U8_RGBA:
                    process_ycbcr_8x8 = process_ycbcr_rgba_8x8_avx2;
                    simd_8x8   = "AVX2";
                    break;
            }
        }

#endif // MANGO_ENABLE_AVX2

        std::string id;

        // determine jpeg type -> select innerloops
        switch (m_components)
        {
            case 1:
                processState.process = process_y;
                id = "Y";
                break;

            case 3:
                processState.process = process_ycbcr;
                id = "YCbCr";

                // detect optimized cases
                if (blocks_in_mcu <= 6)
                {
                    if (xblock == 8 && yblock == 8)
                    {
                        if (process_ycbcr_8x8)
                        {
                            processState.process = process_ycbcr_8x8;
                            id = fmt::format("YCbCr 8x8 {}", simd_8x8);
                        }
                    }

                    if (xblock == 8 && yblock == 16)
                    {
                        if (process_ycbcr_8x16)
                        {
                            processState.process = process_ycbcr_8x16;
                            id = fmt::format("YCbCr 8x16 {}", simd_8x16);
                        }
                    }

                    if (xblock == 16 && yblock == 8)
                    {
                        if (process_ycbcr_16x8)
                        {
                            processState.process = process_ycbcr_16x8;
                            id = fmt::format("YCbCr 16x8 {}", simd_16x8);
                        }
                    }

                    if (xblock == 16 && yblock == 16)
                    {
                        if (process_ycbcr_16x16)
                        {
                            processState.process = process_ycbcr_16x16;
                            id = fmt::format("YCbCr 16x16 {}", simd_16x16);
                        }
                    }
                }
                break;

            case 4:
                processState.process = process_cmyk;
                id = "CMYK";
                break;
        }

        m_ycbcr_name = id;

        printLine(Print::Info, "[ConfigureCPU]");
        printLine(Print::Info, "  Decoder: {}", id);
        printLine(Print::Info, "");
    }

    ImageDecodeStatus Parser::decode(const Surface& target, const ImageDecodeOptions& options)
    {
        m_decode_status = ImageDecodeStatus();

        if (!scan_memory.address || !header)
        {
            m_decode_status.setError(header.info);
            return m_decode_status;
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
        m_hardware_concurrency = int(options.multithread ? ThreadPool::getHardwareConcurrency() : 1);

        if (is_lossless)
        {
            // lossless only supports L8 and RGBA
            if (m_components == 1)
            {
                sf.sample = JPEG_U8_Y;
                sf.format = LuminanceFormat(8, Format::UNORM, 8, 0);
            }
            else
            {
                sf.sample = JPEG_U8_RGBA;
                sf.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
        }
        else if (m_components == 4)
        {
            // CMYK / YCCK is in the slow-path anyway so force RGBA
            sf.sample = JPEG_U8_RGBA;
            sf.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }

        m_decode_status.direct = true;

        if (target.width < m_width || target.height < m_height)
        {
            m_decode_status.direct = false;
        }

        if (target.format != sf.format)
        {
            m_decode_status.direct = false;
        }

        // set decoding target surface
        m_target = &target;
        m_surface = &target;
        m_request_blitting = false;

        std::unique_ptr<Bitmap> temp;

        if (!m_decode_status.direct)
        {
            // create a temporary decoding target
            temp = std::make_unique<Bitmap>(m_aligned_width, m_aligned_height, sf.format);
            m_surface = temp.get();
        }

        // decoding
        parse(scan_memory, true);

        if (!header)
        {
            blockVector.resize(0);
            m_decode_status.setError(header.info);
            return m_decode_status;
        }

        if (m_interface->cancelled)
        {
            blockVector.resize(0);
            m_decode_status.info = getInfo();
            return m_decode_status;
        }

        if (is_progressive || is_multiscan)
        {
            finishProgressive();
        }

        if (m_request_blitting)
        {
            ImageDecodeRect rect;

            rect.x = 0;
            rect.y = 0;
            rect.width = m_width;
            rect.height = m_height;
            rect.progress = 1.0f;

            blit_and_update(rect, true);
        }

        blockVector.resize(0);
        m_decode_status.info = getInfo();

        return m_decode_status;
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

        int predictor = decodeState.spectral_start;
        int pointTransform = decodeState.successive_low;

        auto decodeFunction = huff_decode_mcu_lossless;
        int* previousDC = decodeState.huffman.last_dc_value;

        if (decodeState.is_arithmetic)
        {
            decodeFunction = arith_decode_mcu_lossless;
            previousDC = decodeState.arithmetic.last_dc_value;
        }

        const int xsize = m_surface->width;
        const int ysize = m_surface->height;
        const int xlast = xsize - 1;
        const int n_components = decodeState.comps_in_scan;

        int initPredictor = 1 << (m_precision - pointTransform - 1);

        std::vector<int> scanLineCache[JPEG_MAX_BLOCKS_IN_MCU];

        for (int i = 0; i < n_components; ++i)
        {
            scanLineCache[i] = std::vector<int>(xsize + 1, 0);
        }

        bool first = true;

        for (int y = 0; y < ysize; ++y)
        {
            if (m_interface->cancelled)
            {
                break;
            }

            u8* image = m_surface->address<u8>(0, y);

            for (int x = 0; x < xsize; ++x)
            {
                s16 data[JPEG_MAX_BLOCKS_IN_MCU];

                decodeFunction(data, &decodeState);
                bool restarted = handleRestart();
                bool init = restarted | first;
                first = false;

                for (int currentComponent = 0; currentComponent < n_components; ++currentComponent)
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
                    data[currentComponent] = data[currentComponent] >> (m_precision - 8);
                }

                if (n_components == 1)
                {
                    image[0] = u8_clamp(data[0] + 128);
                    image += 1;
                }
                else
                {
                    image[0] = u8_clamp(data[0] + 128); // red
                    image[1] = u8_clamp(data[1] + 128); // green
                    image[2] = u8_clamp(data[2] + 128); // blue
                    image[3] = 0xff;
                    image += 4;
                }
            }
        }
    }

    void Parser::decodeSequential()
    {
        if (m_hardware_concurrency > 1)
        {
            int n = getTaskSize(ymcu);
            decodeSequentialMT(n);
        }
        else
        {
            decodeSequentialST();
        }
    }

    void Parser::decodeSequentialST()
    {
        if (restartInterval)
        {
            // ---------------------------------------------------------------
            // standard jpeg with DRI marker present
            // ---------------------------------------------------------------

            const size_t stride = m_surface->stride;
            const size_t bytes_per_pixel = m_surface->format.bytes();
            const size_t xstride = bytes_per_pixel * xblock;
            const size_t ystride = stride * yblock;
            const int N = 8;

            AlignedStorage<s16> data(JPEG_MAX_SAMPLES_IN_MCU);

            u8* image = m_surface->image;

            int restart_counter = 0;

            for (int scan = 0; scan < ymcu; scan += N)
            {
                if (m_interface->cancelled)
                {
                    break;
                }

                int y0 = scan;
                int y1 = std::min(scan + N, ymcu);

                for (int y = y0; y < y1; ++y)
                {
                    const int xmcu_last = xmcu - 1;
                    const int ymcu_last = ymcu - 1;

                    const int xclip = m_width  % xblock;
                    const int yclip = m_height % yblock;
                    const int xblock_last = xclip ? xclip : xblock;
                    const int yblock_last = yclip ? yclip : yblock;

                    u8* dest = image + y * ystride;

                    for (int x = 0; x < xmcu; ++x)
                    {
                        decodeState.decode(data, &decodeState);

                        int xsize = x == xmcu_last ? xblock_last : xblock;
                        int ysize = y == ymcu_last ? yblock_last : yblock;

                        process_and_clip(dest, stride, data, xsize, ysize);
                        dest += xstride;

                        if (++restart_counter == restartInterval)
                        {
                            decodeState.restart();
                            restart_counter = 0;

                            const u8* p = decodeState.buffer.ptr;
                            p = seekMarker(p, decodeState.buffer.end);
                            if (isRestartMarker(p))
                            {
                                p += 2;
                            }

                            if (p >= decodeState.buffer.end)
                            {
                                // out of data
                                break;
                            }

                            decodeState.buffer.ptr = p;
                        }
                    }
                }

                ImageDecodeRect rect;

                rect.x = 0;
                rect.y = y0 * yblock;
                rect.width = m_width;
                rect.height = std::min(m_height, y1 * yblock) - y0 * yblock;
                rect.progress = float(rect.height) / m_height;

                blit_and_update(rect);
            }

            // update parser pointer
            const u8* p = seekMarker(decodeState.buffer.ptr - 12, decodeState.buffer.end);
            decodeState.buffer.ptr = p;
        }
        else
        {
            // ---------------------------------------------------------------
            // standard jpeg - Huffman/Arithmetic decoder must be serial
            // ---------------------------------------------------------------

            const int mcu_data_size = blocks_in_mcu * 64;
            const int N = 8;
            const int ncount = N * xmcu;

            void* aligned_ptr = aligned_malloc(ncount * mcu_data_size * sizeof(s16), 64);
            s16* data = reinterpret_cast<s16*>(aligned_ptr);

            for (int y = 0; y < ymcu; y += N)
            {
                if (m_interface->cancelled)
                {
                    break;
                }

                const int y0 = y;
                const int y1 = std::min(y + N, ymcu);
                const int count = (y1 - y0) * xmcu;

                for (int i = 0; i < count; ++i)
                {
                    decodeState.decode(data + i * mcu_data_size, &decodeState);
                }

                if (decodeState.buffer.ptr >= decodeState.buffer.end)
                {
                    // out of data
                    break;
                }

                process_range(y0, y1, data);
            }

            aligned_free(data);

            // update parser pointer
            const u8* p = seekMarker(decodeState.buffer.ptr - 12, decodeState.buffer.end);
            decodeState.buffer.ptr = p;
        }
    }

    void Parser::decodeSequentialMT(int N)
    {
        ConcurrentQueue queue("jpeg:sequential");

        if (!m_restart_offsets.empty())
        {
            // -----------------------------------------------------------------
            // custom mango encoded file (APP14:'Mango1' chunk present)
            // -----------------------------------------------------------------

            const size_t stride = m_surface->stride;
            const size_t bytes_per_pixel = m_surface->format.bytes();
            const size_t xstride = bytes_per_pixel * xblock;
            const size_t ystride = stride * yblock;

            const u8* p = decodeState.buffer.ptr;
            u8* image = m_surface->image;

            const u32* offsets = m_restart_offsets.data();

            for (int y = 0; y < ymcu; y += N)
            {
                if (m_interface->cancelled)
                {
                    queue.cancel();
                    break;
                }

                int y0 = y;
                int y1 = std::min(y + N, ymcu);

                // enqueue task
                queue.enqueue([=, this]
                {
                    AlignedStorage<s16> data(JPEG_MAX_SAMPLES_IN_MCU);

                    const int xmcu_last = xmcu - 1;
                    const int ymcu_last = ymcu - 1;
                    const int xclip = m_width  % xblock;
                    const int yclip = m_height % yblock;
                    const int xblock_last = xclip ? xclip : xblock;
                    const int yblock_last = yclip ? yclip : yblock;

                    const u8* ptr = p;

                    for (int i = y0; i < y1; ++i)
                    {
                        if (m_interface->cancelled)
                        {
                            return;
                        }

                        DecodeState state = decodeState;
                        state.buffer.ptr = ptr;
                        ptr = m_memory.address + offsets[i];

                        u8* dest = image + i * ystride;
                        int height = (i == ymcu_last) ? yblock_last : yblock;

                        for (int x = 0; x < xmcu_last; ++x)
                        {
                            state.decode(data, &state);
                            process_and_clip(dest, stride, data, xblock, height);
                            dest += xstride;
                        }

                        // last column
                        state.decode(data, &state);
                        process_and_clip(dest, stride, data, xblock_last, height);
                    }

                    ImageDecodeRect rect;

                    rect.x = 0;
                    rect.y = y0 * yblock;
                    rect.width = m_width;
                    rect.height = std::min(m_height, y1 * yblock) - y0 * yblock;
                    rect.progress = float(rect.height) / m_height;

                    blit_and_update(rect);
                });

                // use last offset in the range
                p = m_memory.address + offsets[y1 - 1];
            }

            // update parser pointer
            decodeState.buffer.ptr = p;
        }
        else if (restartInterval)
        {
            // ---------------------------------------------------------------
            // standard jpeg with DRI marker present
            // ---------------------------------------------------------------

            //if (restartInterval < xmcu || restartInterval % xmcu)
            if (restartInterval != xmcu)
            {
                // restart markers are in middle of MCU scan which is against the specification
                // we can still handle this in sequential code
                decodeSequentialST();
                return;
            }

            const u8* p = decodeState.buffer.ptr;

            const size_t stride = m_surface->stride;
            const size_t bytes_per_pixel = m_surface->format.bytes();
            const size_t xstride = bytes_per_pixel * xblock;
            const size_t ystride = stride * yblock;

            u8* image = m_surface->image;

            for (int y = 0; y < ymcu; y += N)
            {
                if (m_interface->cancelled)
                {
                    queue.cancel();
                    break;
                }

                int y0 = y;
                int y1 = std::min(y + N, ymcu);

                // enqueue task
                queue.enqueue([=, this] (const u8* p)
                {
                    for (int y = y0; y < y1; ++y)
                    {
                        if (m_interface->cancelled)
                        {
                            return;
                        }

                        AlignedStorage<s16> data(JPEG_MAX_SAMPLES_IN_MCU);

                        DecodeState state = decodeState;
                        state.buffer.ptr = p;

                        const int xmcu_last = xmcu - 1;
                        const int ymcu_last = ymcu - 1;

                        const int xclip = m_width  % xblock;
                        const int yclip = m_height % yblock;
                        const int xblock_last = xclip ? xclip : xblock;
                        const int yblock_last = yclip ? yclip : yblock;

                        for (int x = 0; x < xmcu; ++x)
                        {
                            state.decode(data, &state);

                            u8* dest = image + y * ystride + x * xstride;

                            int width  = x == xmcu_last ? xblock_last : xblock;
                            int height = y == ymcu_last ? yblock_last : yblock;

                            process_and_clip(dest, stride, data, width, height);
                        }

                        p = seekMarker(state.buffer.ptr, state.buffer.end);
                        if (isRestartMarker(p))
                        {
                            p += 2;
                        }

                        if (p >= state.buffer.end)
                        {
                            // out of data
                            return;
                        }
                    }

                    ImageDecodeRect rect;

                    rect.x = 0;
                    rect.y = y0 * yblock;
                    rect.width = m_width;
                    rect.height = std::min(m_height, y1 * yblock) - y0 * yblock;
                    rect.progress = float(rect.height) / m_height;

                    blit_and_update(rect);
                }, p);

                // skip N restart intervals (handled by the task queue)
                for (int i = y0; i < y1; ++i)
                {
                    p = seekMarker(p, decodeState.buffer.end);
                    if (isRestartMarker(p))
                    {
                        p += 2;
                    }
                }

                if (p >= decodeState.buffer.end)
                {
                    // out of data
                    break;
                }
            }

            // update parser pointer
            p = seekMarker(p - 12, decodeState.buffer.end);
            decodeState.buffer.ptr = p;
        }
        else
        {
            // ---------------------------------------------------------------
            // standard jpeg - Huffman/Arithmetic decoder must be serial
            // ---------------------------------------------------------------

            const int mcu_data_size = blocks_in_mcu * 64;

            for (int y = 0; y < ymcu; y += N)
            {
                if (m_interface->cancelled)
                {
                    // NOTE: Don't cancel queue it deallocates memory
                    break;
                }

                const int y0 = y;
                const int y1 = std::min(y + N, ymcu);
                const int count = (y1 - y0) * xmcu;
                printLine(Print::Info, "  Process: [{}, {}] --> ThreadPool.", y0, y1 - 1);

                void* aligned_ptr = aligned_malloc(count * mcu_data_size * sizeof(s16), 64);
                s16* data = reinterpret_cast<s16*>(aligned_ptr);

                for (int i = 0; i < count; ++i)
                {
                    decodeState.decode(data + i * mcu_data_size, &decodeState);
                }

                // enqueue task
                queue.enqueue([=, this]
                {
                    if (!m_interface->cancelled)
                    {
                        process_range(y0, y1, data);
                    }

                    aligned_free(aligned_ptr);
                });
            }

            // update parser pointer
            const u8* p = seekMarker(decodeState.buffer.ptr - 12, decodeState.buffer.end);
            decodeState.buffer.ptr = p;
        }
    }

    void Parser::decodeMultiScan()
    {
        s16* data = blockVector;
        data += decodeState.block[0].offset;

        for (int y = 0; y < ymcu; ++y)
        {
            if (m_interface->cancelled)
            {
                break;
            }

            for (int x = 0; x < xmcu; ++x)
            {
                decodeState.decode(data, &decodeState);
                data += blocks_in_mcu * 64;
            }

            if (isRestartMarker(decodeState.buffer.ptr))
            {
                decodeState.restart();
                decodeState.buffer.ptr += 2;
            }
        }
    }

    void Parser::decodeProgressive()
    {
        if (decodeState.spectral_start == 0)
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

            ConcurrentQueue queue("jpeg:progressive.dc");

            for (int i = 0; i < mcus; i += restartInterval)
            {
                if (m_interface->cancelled)
                {
                    queue.cancel();
                    break;
                }

                // enqueue task
                queue.enqueue([=, this]
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

                p = seekMarker(decodeState.buffer.ptr, decodeState.buffer.end);
                if (isRestartMarker(p))
                {
                    p += 2;
                }
            }

            decodeState.buffer.ptr = p;
        }
        else
        {
            s16* data = blockVector;

            for (int i = 0; i < mcus; ++i)
            {
                if (!(i & 0x200) && m_interface->cancelled)
                {
                    break;
                }

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

            const int hsf = scanFrame->hsf;
            const int vsf = scanFrame->vsf;
            const int hsize = (Hmax / hsf) * 8;
            const int vsize = (Vmax / vsf) * 8;

            printLine(Print::Info, "    hsf: {}, vsf: {}, blocks: {}", hsf, vsf, decodeState.blocks);

            const int scan_offset = scanFrame->offset;
            const int xs = div_ceil(m_width, hsize);
            const int ys = div_ceil(m_height, vsize);
            const int cnt = xs * ys;

            printLine(Print::Info, "    blocks: {} x {} ({} x {})", xs, ys, xs * hsize, ys * vsize);

            MANGO_UNREFERENCED(xs);
            MANGO_UNREFERENCED(ys);
            MANGO_UNREFERENCED(hsize);
            MANGO_UNREFERENCED(vsize);

            ConcurrentQueue queue("jpeg:progressive.ac");

            const u8* p = decodeState.buffer.ptr;

            for (int i = 0; i < cnt; i += restartInterval)
            {
                if (m_interface->cancelled)
                {
                    queue.cancel();
                    break;
                }

                // enqueue task
                queue.enqueue([=, this]
                {
                    DecodeState state = decodeState;
                    state.buffer.ptr = p;

                    const int left = std::min(restartInterval, mcus - i);
                    for (int j = 0; j < left; ++j)
                    {
                        int n = i + j;
                        int x = n % xmcu;
                        int y = n / xmcu;

                        int mcu_yoffset = (y / vsf) * xmcu;
                        int block_yoffset = ((y % vsf) * hsf) + scan_offset;

                        int mcu_offset = (mcu_yoffset + (x / hsf)) * blocks_in_mcu;
                        int block_offset = (x % hsf) + block_yoffset;
                        s16* mcudata = data + (block_offset + mcu_offset) * 64;

                        state.decode(mcudata, &state);
                    }
                });

                p = seekMarker(decodeState.buffer.ptr, decodeState.buffer.end);
                if (isRestartMarker(p))
                {
                    p += 2;
                }
            }

            decodeState.buffer.ptr = p;
        }
        else
        {
            s16* data = blockVector;

            const int hsf = scanFrame->hsf;
            const int vsf = scanFrame->vsf;
            const int hsize = (Hmax / hsf) * 8;
            const int vsize = (Vmax / vsf) * 8;

            printLine(Print::Info, "    hsf: {}, vsf: {}, blocks: {}", hsf, vsf, decodeState.blocks);

            const int scan_offset = scanFrame->offset;
            const int xs = div_ceil(m_width, hsize);
            const int ys = div_ceil(m_height, vsize);

            printLine(Print::Info, "    blocks: {} x {} ({} x {})", xs, ys, xs * hsize, ys * vsize);

            for (int y = 0; y < ys; ++y)
            {
                if (m_interface->cancelled)
                {
                    break;
                }

                int mcu_yoffset = (y / vsf) * xmcu;
                int block_yoffset = ((y % vsf) * hsf) + scan_offset;

                for (int x = 0; x < xs; ++x)
                {
                    int mcu_offset = (mcu_yoffset + (x / hsf)) * blocks_in_mcu;
                    int block_offset = (x % hsf) + block_yoffset;
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
            ConcurrentQueue queue("jpeg:progressive.finish");

            size_t mcu_stride = size_t(xmcu) * blocks_in_mcu * 64;

            for (int y = 0; y < ymcu; y += n)
            {
                if (m_interface->cancelled)
                {
                    queue.cancel();
                    break;
                }

                const int y0 = y;
                const int y1 = std::min(y + n, ymcu);

                s16* data = blockVector + y0 * mcu_stride;

                printLine(Print::Info, "  Process: [{}, {}] --> ThreadPool.", y0, y1 - 1);

                // enqueue task
                queue.enqueue([=, this]
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

        const int xclip = m_width  % xblock;
        const int yclip = m_height % yblock;
        const int xblock_last = xclip ? xclip : xblock;
        const int yblock_last = yclip ? yclip : yblock;

        for (int y = y0; y < y1; ++y)
        {
            if (m_interface->cancelled)
            {
                break;
            }

            u8* dest = image + y * ystride;
            int ysize = y == ymcu_last ? yblock_last : yblock;

            for (int x = 0; x < xmcu_last; ++x)
            {
                process_and_clip(dest, stride, data, xblock, ysize);
                data += mcu_data_size;
                dest += xstride;
            }

            // last column
            process_and_clip(dest, stride, data, xblock_last, ysize);
            data += mcu_data_size;
            dest += xstride;
        }

        ImageDecodeRect rect;

        rect.x = 0;
        rect.y = y0 * yblock;
        rect.width = m_width;
        rect.height = std::min(m_height, y1 * yblock) - y0 * yblock;
        rect.progress = float(rect.height) / m_height;

        blit_and_update(rect);
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

    void Parser::blit_and_update(const ImageDecodeRect& rect, bool force_blit)
    {
        if (!m_decode_status.direct || force_blit)
        {
            // color conversion and clipping
            Surface source(*m_surface, rect.x, rect.y, rect.width, rect.height);
            m_target->blit(rect.x, rect.y, source);
        }

        if (m_interface->callback)
        {
            m_interface->callback(rect);
        }
    }

} // namespace mango::image::jpeg

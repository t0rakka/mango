/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/endian.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/core/thread.hpp>
#include "jpeg.hpp"

namespace jpeg
{

    using namespace mango;

	static const int zigzagTable[] =
	{
        0,  8,  1,  2,  9, 16, 24, 17,
        10,  3,  4, 11, 18, 25, 32, 40,
        33, 26, 19, 12,  5,  6, 13, 20,
        27, 34, 41, 48, 56, 49, 42, 35,
        28, 21, 14,  7, 15, 22, 29, 36,
        43, 50, 57, 58, 51, 44, 37, 30,
        23, 31, 38, 45, 52, 59, 60, 53,
        46, 39, 47, 54, 61, 62, 55, 63,
		63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63,
	};

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
    // jpegBuffer
    // ----------------------------------------------------------------------------

    void jpegBuffer::restart()
    {
        data = 0;
        remain = 0;
        nextFF = reinterpret_cast<const uint8*>(std::memchr(ptr, 0xff, end - ptr));
    }

    void jpegBuffer::bytes(int n)
    {
        for (int i = 0; i < n; ++i)
        {
            int a = ptr < end ? *ptr++ : 0;
            if (a == 0xff)
            {
                int b = ptr < end ? *ptr++ : 0;
                if (b)
                {
                    // Found a marker; keep returning zero until it has been processed
                    ptr -= 2;
                    a = 0;
                }
            }
            data = (data << 8) | a;
        }

        // When nextFF is NULL that means no 0xff bytes left in the stream which means corruted jpeg
        // because the EOI marker should always be present. NULL would prohibit fast path from running
        // and the slow path would re-scan for nextFF for ALL remaining bytes in the jpeg. This check
        // simply puts the decoder into slow path permanently. Fast path would be nicer but since we already
        // do know the stream is corrupted we want to guard every read against EOF condition.
        if (nextFF)
        {
            nextFF = reinterpret_cast<const uint8*>(std::memchr(ptr, 0xff, end - ptr));
        }
    }

    // ----------------------------------------------------------------------------
    // Parser
    // ----------------------------------------------------------------------------

    Parser::Parser(const Memory& memory)
    : quantTableVector(64 * JPEG_MAX_COMPS_IN_SCAN), blockVector(NULL)
    {
        decodeState.zigzagTable = zigzagTable;

        // configure default implementation
		processState.idct = idct;
        processState.process_Y           = process_Y;
        processState.process_YCbCr       = process_YCbCr;
        processState.process_CMYK        = process_CMYK;
        processState.process_YCbCr_8x8   = process_YCbCr_8x8;
        processState.process_YCbCr_8x16  = process_YCbCr_8x16;
        processState.process_YCbCr_16x8  = process_YCbCr_16x8;
        processState.process_YCbCr_16x16 = process_YCbCr_16x16;

		restartInterval = 0;
        restartCounter = 0;

#if defined(JPEG_ENABLE_SSE) && defined(MANGO_ENABLE_SSE4_1)
        uint64 cpuFlags = getCPUFlags();
        if (cpuFlags & CPU_SSE4_1)
        {
            // configure SSE 4.1 implementation
			processState.idct = idct_sse41;
            processState.process_YCbCr_8x8   = process_YCbCr_8x8_sse41;
            processState.process_YCbCr_8x16  = process_YCbCr_8x16_sse41;
            processState.process_YCbCr_16x8  = process_YCbCr_16x8_sse41;
            processState.process_YCbCr_16x16 = process_YCbCr_16x16_sse41;
        }
#endif

        for (int i = 0; i < JPEG_MAX_COMPS_IN_SCAN; ++i)
        {
            quantTable[i].table = &quantTableVector[i * 64];
        }

        exif_memory = Memory(NULL, 0);
        icc_memory = Memory(NULL, 0);
        scan_memory = Memory(NULL, 0);

        m_surface = NULL;

        header.width = 0;
        header.height = 0;
        header.xblock = 0;
        header.yblock = 0;
        header.format = Format();

        if (isJPEG(memory))
        {
            parse(memory, false);
        }
    }

    Parser::~Parser()
    {
        aligned_free(blockVector);
    }

    bool Parser::isJPEG(const Memory& memory) const
    {
        if (!memory.address || memory.size < 4)
            return false;

        if (uload16be(memory.address) != MARKER_SOI)
            return false;

#if 0
        // Scan for EOI marker
        const uint8* p = memory.address + memory.size - 2;
        for (int i = 0; i < 32; ++i, --p)
        {
            uint16 marker = uload16be(p);
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

    const uint8* Parser::stepMarker(const uint8* p)
    {
        uint16 size = uload16be(p);
        return p + size;
    }

    const uint8* Parser::seekMarker(const uint8* p, const uint8* end)
    {
        const uint8* start = p;

        for ( ; p < end; ++p)
        {
            if (p[0] == 0xff)
            {
                if (p[1] != 0)
                {
                    // found marker
                    break;
                }
                else
                {
                    // skip stuff byte
                    ++p;
                }
            }
        }

        jpegPrint("  Seek: %d bytes\n", int(p - start));
        MANGO_UNREFERENCED_PARAMETER(start);

        return p;
    }

    void Parser::processSOI()
    {
        jpegPrint("[ SOI ]\n");
        restartInterval = 0;
    }

    void Parser::processEOI()
    {
        jpegPrint("[ EOI ]\n");
    }

    void Parser::processCOM(const uint8* p)
    {
        jpegPrint("[ COM ]\n");
        MANGO_UNREFERENCED_PARAMETER(p);
    }

    void Parser::processTEM(const uint8* p)
    {
        jpegPrint("[ TEM ]\n");
        MANGO_UNREFERENCED_PARAMETER(p);
    }

    void Parser::processRES(const uint8* p)
    {
        jpegPrint("[ RES ]\n");

        // Reserved for jpeg extensions
        MANGO_UNREFERENCED_PARAMETER(p);
    }

    void Parser::processJPG(const uint8* p)
    {
        jpegPrint("[ JPG ]\n");

        // Reserved for jpeg extensions
        MANGO_UNREFERENCED_PARAMETER(p);
    }

    void Parser::processJPG(const uint8* p, uint16 marker)
    {
        jpegPrint("[ JPG%d ]\n", int(marker - MARKER_JPG0));

        // Reserved for jpeg extensions
        MANGO_UNREFERENCED_PARAMETER(p);
        MANGO_UNREFERENCED_PARAMETER(marker);
    }

    void Parser::processAPP(const uint8* p, uint16 marker)
    {
        jpegPrint("[ APP%d ]\n", int(marker - MARKER_APP0));

        int size = uload16be(p) - 2;
        p += 2;

        switch (marker)
        {
            case MARKER_APP0:
            {
                const uint8 magicJFIF[] = { 0x4a, 0x46, 0x49, 0x46, 0 }; // 'JFIF', 0
                const uint8 magicJFXX[] = { 0x4a, 0x46, 0x58, 0x58, 0 }; // 'JFXX', 0

                if (!std::memcmp(p, magicJFIF, 5) || !std::memcmp(p, magicJFXX, 5))
                {
                    p += 5;
                    size -= 5;

                    jpegPrint("  JFIF: %i bytes\n", size);

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

                    jpegPrint("    version: %i\n", version);
                    jpegPrint("    density: %i x %i %s\n", Xdensity, Ydensity, unit_str);
                    jpegPrint("    thumbnail: %i x %i\n", Xthumbnail, Ythumbnail);

                    // TODO: process thumbnail / store JFIF block
                    MANGO_UNREFERENCED_PARAMETER(version);
                    MANGO_UNREFERENCED_PARAMETER(Xdensity);
                    MANGO_UNREFERENCED_PARAMETER(Ydensity);
                    MANGO_UNREFERENCED_PARAMETER(Xthumbnail);
                    MANGO_UNREFERENCED_PARAMETER(Ythumbnail);
                    MANGO_UNREFERENCED_PARAMETER(unit_str);
                }

                break;
            }

            case MARKER_APP1:
            {
                const uint8 magicExif0[] = { 0x45, 0x78, 0x69, 0x66, 0, 0 }; // 'Exif', 0, 0
                const uint8 magicExif255[] = { 0x45, 0x78, 0x69, 0x66, 0, 0xff }; // 'Exif', 0, 0xff

                if (!std::memcmp(p, magicExif0, 6) || !std::memcmp(p, magicExif255, 6))
                {
                    p += 6;
                    size -= 6;
                    exif_memory = Memory(p, size);
                    jpegPrint("  EXIF: %d bytes\n", size);
                }

                // TODO: detect and support XMP

                break;
            }

            case MARKER_APP2:
            {
                const uint8 magicICC[] = { 0x49, 0x43, 0x43, 0x5f, 0x50, 0x52, 0x4f, 0x46, 0x49, 0x4c, 0x45, 0 }; // 'ICC_PROFILE', 0

                if (!std::memcmp(p, magicICC, 12))
                {
                    p += 12;
                    size -= 12;
                    icc_memory = Memory(p, size);
                    jpegPrint("  ICC: %d bytes\n", size);
                }

                break;
            }

            case MARKER_APP3:
            {
                const uint8 magicMETA[] = { 0x4d, 0x45, 0x54, 0x41, 0, 0 }; // 'META', 0, 0
                const uint8 magicMeta[] = { 0x4d, 0x65, 0x74, 0x61, 0, 0 }; // 'Meta', 0, 0

                if (!std::memcmp(p, magicMETA, 6) || !std::memcmp(p, magicMeta, 6))
                {
                    p += 6;
                    size -= 6;
                    exif_memory = Memory(p, size);
                    jpegPrint("  EXIF: %d bytes\n", size);
                }

                break;
            }
        }
    }

    void Parser::processSOF(const uint8* p, uint16 marker)
    {
        jpegPrint("[ SOF%d ]\n", int(marker - MARKER_SOF0));

        is_progressive = false;
        is_arithmetic = false;
        is_lossless = false;

        uint16 length = uload16be(p + 0);
        precision = p[2];
        ysize = uload16be(p + 3);
        xsize = uload16be(p + 5);
        uint8 comps = p[7];
        p += 8;

        jpegPrint("  Image: %d x %d x %d\n", xsize, ysize, precision);

        is_arithmetic = marker > MARKER_SOF7;
        std::string compression = is_arithmetic ? "Arithmetic" : "Huffman";

        std::string encoding;

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

        jpegPrint("  Encoding: %s\n", encoding.c_str());
        jpegPrint("  Compression: %s\n", compression.c_str());

        m_info = encoding;
        m_info += "  ";
        m_info += compression;

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
            case MARKER_SOF3:
            case MARKER_SOF7:
            case MARKER_SOF11:
            case MARKER_SOF15:
                is_lossless = true;
                break;
        }

        Hmax = 0;
        Vmax = 0;
        blocks_in_mcu = 0;
        int offset = 0;

        processState.frames = comps;

        for (int i = 0; i < comps; ++i)
        {
            Frame& frame = processState.frame[i];

            frame.compid = p[0];
            uint8 x = p[1];
            frame.Hsf = (x >> 4) & 0xf;
            frame.Vsf = (x >> 0) & 0xf;
            frame.Tq = p[2];
            frame.offset = offset;
            p += 3;

            if (comps == 1)
            {
                // Optimization: force block size to 8x8 with grayscale images
                frame.Hsf = 1;
                frame.Vsf = 1;
            }

            Hmax = std::max(Hmax, frame.Hsf);
            Vmax = std::max(Vmax, frame.Vsf);
            blocks_in_mcu += frame.Hsf * frame.Vsf;

            int base = offset * 64;

            for (int y = 0; y < frame.Vsf; ++y)
            {
                for (int x = 0; x < frame.Hsf; ++x)
                {
                    processState.block[offset].qt = &quantTable[frame.Tq];
                    processState.block[offset].offset = base + y * frame.Hsf * 64 + x * 8;
                    processState.block[offset].stride = frame.Hsf * 8;
                    ++offset;
                }
            }

            jpegPrint("  Frame: %d, compid: %d, Hsf: %d, Vsf: %d, Tq: %d, offset: %d\n",
                i, frame.compid, frame.Hsf, frame.Vsf, frame.Tq, frame.offset);

            frames.push_back(frame);
        }

        processState.blocks = offset;

        // Compute frame sampling factors against maximum sampling factor,
        // then convert them into power-of-two presentation.
        for (int i = 0; i < comps; ++i)
        {
            Frame& frame = processState.frame[i];
            frame.Hsf = u32_log2(Hmax / frame.Hsf);
            frame.Vsf = u32_log2(Vmax / frame.Vsf);
        }

        xblock = 8 * Hmax;
        yblock = 8 * Vmax;

        jpegPrint("  Blocks per MCU: %d\n", blocks_in_mcu);
        jpegPrint("  MCU size: %d x %d\n", xblock, yblock);

        // Align to next MCU boundary
        int xmask = xblock - 1;
        int ymask = yblock - 1;
        width  = (xsize + xmask) & ~xmask;
        height = (ysize + ymask) & ~ymask;

        // MCU resolution
        xmcu = width  / xblock;
        ymcu = height / yblock;
        mcus = xmcu * ymcu;

        // clipping
        xclip = xsize % xblock;
        yclip = ysize % yblock;

        jpegPrint("  %d MCUs (%d x %d) -> (%d x %d)\n", mcus, xmcu, ymcu, xmcu*xblock, ymcu*yblock);
        jpegPrint("  Image: %d x %d\n", xsize, ysize);
        jpegPrint("  Clip: %d x %d\n", xclip, yclip);

        // determine jpeg type
        switch (comps)
        {
            case 1:
                processState.process = processState.process_Y;
                processState.clipped = processState.process_Y;
                break;

            case 3:
                processState.process = processState.process_YCbCr;
                processState.clipped = processState.process_YCbCr;

                if (blocks_in_mcu <= 6)
                {
                    // detect optimized cases
                    if (xblock == 8 && yblock == 8)
                    {
                        processState.process = processState.process_YCbCr_8x8;
                    }

                    if (xblock == 8 && yblock == 16)
                    {
                        processState.process = processState.process_YCbCr_8x16;
                    }

                    if (xblock == 16 && yblock == 8)
                    {
                        processState.process = processState.process_YCbCr_16x8;
                    }

                    if (xblock == 16 && yblock == 16)
                    {
                        processState.process = processState.process_YCbCr_16x16;
                    }
                }
                break;

            case 4:
                processState.process = processState.process_CMYK;
                processState.clipped = processState.process_CMYK;
                break;
        }

        // configure header
        header.width = xsize;
        header.height = ysize;
        header.xblock = xblock;
        header.yblock = yblock;
        header.format = comps > 1 ? Format(FORMAT_B8G8R8A8) : Format(FORMAT_L8);

        MANGO_UNREFERENCED_PARAMETER(length);
    }

    const uint8* Parser::processSOS(const uint8* p, const uint8* end)
    {
        jpegPrint("[ SOS ]\n");

        uint16 length = uload16be(p);
        decodeState.comps_in_scan = p[2]; // Ns
        p += 3;

        jpegPrint("  components: %i\n", decodeState.comps_in_scan);
        MANGO_UNREFERENCED_PARAMETER(length);

        decodeState.blocks = 0;

        for (int i = 0; i < decodeState.comps_in_scan; ++i)
        {
            uint8 cs = p[0]; // Scan component selector
            uint8 x = p[1];
            p += 2;
            int dc = (x >> 4) & 0xf; // DC entropy coding table destination selector
            int ac = (x >> 0) & 0xf; // AC entropy coding table destination selector

            int compid = cs; // ...

            // find frame
            Frame* frame = NULL;
            int frameIndex = 0;

            for (int j = 0; j < int(frames.size()); ++j)
            {
                if (frames[j].compid == compid)
                {
                    frameIndex = j;
                    frame = &frames[j];
                }
            }

            scanFrame = frame;

            const int size = frame->Hsf * frame->Vsf;
            int offset = frame->offset;

            jpegPrint("  Component: %i, DC: %i, AC: %i, offset: %d, size: %d\n", cs, dc, ac, frame->offset, size);

            for (int j = 0; j < size; ++j)
            {
                DecodeBlock& block = decodeState.block[decodeState.blocks];

                if (is_arithmetic)
                {
                    block.offset = offset * 64;
                    block.pred = frameIndex;
                    block.index.dc = dc;
                    block.index.ac = ac;
                }
                else
                {
                    block.offset = offset * 64;
                    block.pred = frameIndex;
                    block.table.dc = &huffTable[0][dc];
                    block.table.ac = &huffTable[1][ac];
                }

                jpegPrint("      - offset: %d, pred: %d,\n", offset * 64, frameIndex);
                ++offset;
                ++decodeState.blocks;
            }
        }

        int Ss = p[0];
        int Se = p[1];

        uint8 x = p[2];
        int Al = (x >> 0) & 0xf;
        int Ah = (x >> 4) & 0xf;
        p += 3;

        decodeState.spectralStart = Ss;
        decodeState.spectralEnd = Se;
        decodeState.successiveLow = Al;
        decodeState.successiveHigh = Ah;

        jpegPrint("    Spectral range: (%d, %d)\n", Ss, Se);

        bool dc_scan = (decodeState.spectralStart == 0);
        bool refine_scan = (decodeState.successiveHigh != 0);

        restartCounter = restartInterval;

        if (is_arithmetic)
        {
#ifdef MANGO_ENABLE_LICENSE_BSD
            Arithmetic& arithmetic = decodeState.arithmetic;

            decodeState.buffer.ptr = p;
            decodeState.buffer.end = end;

            // restart
            decodeState.buffer.restart();

            arithmetic.restart(decodeState.buffer);

            if (is_progressive)
            {
                if (dc_scan)
                {
                    if (!refine_scan)
                    {
                        jpegPrint("  * decode_dc_first()\n");
                        decodeState.decode = arith_decode_dc_first;
                    }
                    else
                    {
                        jpegPrint("  * decode_dc_refine()\n");
                        decodeState.decode = arith_decode_dc_refine;
                    }
                }
                else
                {
                    if (!refine_scan)
                    {
                        jpegPrint("  * decode_ac_first()\n");
                        decodeState.decode = arith_decode_ac_first;
                    }
                    else
                    {
                        jpegPrint("  * decode_ac_refine()\n");
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
#endif // MANGO_ENABLE_LICENSE_BSD
        }
        else
        {
            Huffman& huffman = decodeState.huffman;

            decodeState.buffer.ptr = p;
            decodeState.buffer.end = end;

            // restart
            decodeState.buffer.restart();

            huffman.restart();

            if (is_progressive)
            {
                if (dc_scan)
                {
                    if (!refine_scan)
                    {
                        jpegPrint("  * decode_dc_first()\n");
                        decodeState.decode = huff_decode_dc_first;
                    }
                    else
                    {
                        jpegPrint("  * decode_dc_refine()\n");
                        decodeState.decode = huff_decode_dc_refine;
                    }
                }
                else
                {
                    if (!refine_scan)
                    {
                        jpegPrint("  * decode_ac_first()\n");
                        decodeState.decode = huff_decode_ac_first;
                    }
                    else
                    {
                        jpegPrint("  * decode_ac_refine()\n");
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

        // TODO: we should sync here since the decoder has prefetched more bytes that it could consume
        p = decodeState.buffer.ptr;
        p -= 8; // hack

        return p;
    }

    void Parser::processDQT(const uint8* p)
    {
        jpegPrint("[ DQT ]\n");

        uint16 Lq = uload16be(p); // Quantization table definition length
        p += 2;
        Lq -= 2;

        for (; Lq > 0;)
        {
            uint8 x = *p++;
            uint8 Pq = (x >> 4) & 0xf; // Quantization table element precision (0 = 8 bits, 1 = 16 bits)
            uint8 Tq = (x >> 0) & 0xf; // Quantization table destination identifier (0..3)

            const int bits = (Pq + 1) * 8;
            jpegPrint("  Quantization table #%i element precision: %i bits\n", Tq, bits);

            if (Tq >= JPEG_MAX_COMPS_IN_SCAN)
            {
                jpegPrint("  Incorrect quantization table.\n");
                return;
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
                        table.table[zigzagTable[i]] = uload16be(p);
                        p += 2;
                    }
                    break;

                default:
                    jpegPrint("  Incorrect quantization table element precision.\n");
                    break;
            }

            Lq -= (1 + (Pq + 1) * 64);
        }
    }

    void Parser::processDHT(const uint8* p)
    {
        jpegPrint("[ DHT ]\n");

        int Lh = uload16be(p); // Huffman table definition length
        p += 2;
        Lh -= 2;

        for (; Lh > 0;)
        {
            uint8 x = *p++;
            uint8 Tc = (x >> 4) & 0xf; // Table class - 0 = DC table or lossless table, 1 = AC table.
            uint8 Th = (x >> 0) & 0xf; // Huffman table identifier

            if (Tc >= 2 || Th >= JPEG_MAX_COMPS_IN_SCAN)
            {
                jpegPrint("  Incorrect huffman table.\n");
                return;
            }

            HuffTable& table = huffTable[Tc][Th];

            jpegPrint("  Huffman table #%i table class: %i\n", Th, Tc);
            jpegPrint("    codes: ");

            int count = 0;

            for (int i = 1; i <= 16; ++i)
            {
                uint8 L = *p++; // Number of Huffman codes of length i bits
                table.size[i] = L;
                count += L;
                jpegPrint("%i ", L);
            }

            Lh -= 17;
            jpegPrint("\n");

            for (int i = 0; i < count; ++i)
            {
                table.value[i] = *p++;
            }

            Lh -= count;
            table.configure();
        }
    }

    void Parser::processDAC(const uint8* p)
    {
        jpegPrint("[ DAC ]\n");

        uint16 La = uload16be(p); // Arithmetic coding conditioning definition length
        p += 2;

        int n = (La - 2) / 2;

        jpegPrint("  n: %i\n", n);

        for (int i = 0; i < n; ++i)
        {
            uint8 x = p[0];
            uint8 Tc = (x >> 4) & 0xf; // Table class - 0 = DC table or lossless table, 1 = AC table
            uint8 Tb = (x >> 0) & 0xf; // Arithmetic coding conditioning table destination identifier
            uint8 Cs = p[1]; // Conditioning table value
            p += 2;

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
            }

            jpegPrint("  Tc: %i, Tb: %i, Cs: %i\n", Tc, Tb, Cs);
        }
    }

    void Parser::processDNL(const uint8* p)
    {
        jpegPrint("[ DNL ]\n");

        uint16 Ld = uload16be(p + 0); // Define number of lines segment length
        uint16 NL = uload16be(p + 2); // Number of lines
        MANGO_UNREFERENCED_PARAMETER(NL); // TODO: ysize = NL
        MANGO_UNREFERENCED_PARAMETER(Ld);
    }

    void Parser::processDRI(const uint8* p)
    {
        jpegPrint("[ DRI ]\n");

        int Lh = uload16be(p + 0); // length
        if (Lh != 4)
        {
            // signal error
        }

        restartInterval = uload16be(p + 2); // number of MCU in restart interval
        jpegPrint("  Restart interval: %i\n", restartInterval);
    }

    void Parser::processDHP(const uint8* p)
    {
        jpegPrint("[ DHP ]\n");

        // TODO: "Define Hierarchical Progression" marker
        MANGO_UNREFERENCED_PARAMETER(p);
    }

    void Parser::processEXP(const uint8* p)
    {
        jpegPrint("[ EXP ]\n");

        uint16 Le = uload16be(p); // Expand reference components segment length
        uint8 x = p[2];
        uint8 Eh = (x >> 4) & 0xf; // Expand horizontally
        uint8 Ev = (x >> 0) & 0xf; // Expand vertically

        // Unsupported marker
        MANGO_UNREFERENCED_PARAMETER(Le);
        MANGO_UNREFERENCED_PARAMETER(Eh);
        MANGO_UNREFERENCED_PARAMETER(Ev);
    }

    void Parser::parse(const Memory& memory, bool decode)
    {
        const uint8* end = memory.address + memory.size;
        const uint8* p = memory.address;

        for (; p < end;)
        {
            uint16 marker = uload16be(p);
            p += 2;

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
                    processDHT(p);
                    p = stepMarker(p);
                    break;

                case MARKER_DAC:
                    processDAC(p);
                    p = stepMarker(p);
                    break;

                case MARKER_DQT:
                    processDQT(p);
                    p = stepMarker(p);
                    break;

                case MARKER_DNL:
                    processDNL(p);
                    p = stepMarker(p);
                    break;

                case MARKER_DRI:
                    processDRI(p);
                    p = stepMarker(p);
                    break;

                case MARKER_DHP:
                    processDHP(p);
                    p = stepMarker(p);
                    break;

                case MARKER_EXP:
                    processEXP(p);
                    p = stepMarker(p);
                    break;

                case MARKER_COM:
                    processCOM(p);
                    p = stepMarker(p);
                    break;

                case MARKER_TEM:
                    processTEM(p);
                    p = stepMarker(p);
                    break;

                case MARKER_RES:
                    processRES(p);
                    p = stepMarker(p);
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
                    p = stepMarker(p);
                    break;

                case MARKER_JPG:
                    processJPG(p);
                    p = stepMarker(p);
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
                    p = stepMarker(p);
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
                    p = stepMarker(p);
                    if (!decode)
                    {
                        // parse header mode (no decoding)
                        scan_memory = Memory(p, end - p);
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
                    p = seekMarker(p, end);
                    break;
            }
        }
    }

    inline bool isRestartMarker(const uint8* p)
    {
        // TODO: clean up this hack
        bool is = false;
        if (p[0] == 0xff)
        {
            int index = p[1] - 0xd0;
            is = index >= 0 && index <= 7;
        }
        return is;
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
            }
        }

        return true;
    }

    Status Parser::decode(Surface& target)
    {
        Status status;

        status.success = true;
        status.enableDirectDecode = true;

        m_info = "";

        if (!scan_memory.address)
        {
            status.success = false;
            return status;
        }

        // allocate blocks
        int count = mcus * blocks_in_mcu * 64;
        blockVector = reinterpret_cast<BlockType*>(aligned_malloc(count * sizeof(BlockType)));

        // target surface size has to match (clipping isn't yet supported)
        if (target.width != xsize || target.height != ysize)
        {
            status.enableDirectDecode = false;
        }

        // pixel format has to match (we don't yet support convert-on-write)
        if (target.format != header.format)
        {
            status.enableDirectDecode = false;
        }

        if (status.enableDirectDecode)
        {
            m_surface = &target;

            parse(scan_memory, true);

            if (is_progressive)
			{
	            finishProgressive();
			}
        }
        else
        {
            Bitmap temp(width, height, header.format);
            m_surface = &temp;

            parse(scan_memory, true);

            if (is_progressive)
			{
	            finishProgressive();
			}

            target.blit(0, 0, temp);
        }

        status.info = m_info;

        return status;
    }

    void Parser::decodeSequential()
    {
#ifdef JPEG_ENABLE_THREAD
        const int count = ThreadPool::getInstanceSize();
#else
        const int count = 1;
#endif
        if (count > 1)
        {
            decodeSequentialMT();
        }
        else
        {
            decodeSequentialST();
        }
    }

    void Parser::decodeSequentialST()
    {
        const int stride = m_surface->stride;
        const int xstride = m_surface->format.bytes() * xblock;
        const int ystride = stride * yblock;
        uint8* image = m_surface->address<uint8>(0, 0);

        BlockType data[640];

        for (int y = 0; y < ymcu; ++y)
        {
            uint8* dest = image;

            ProcessFunc process = processState.process;
            int width = xblock;
            int height = yblock;

            if (yclip && y == ymcu - 1)
            {
                process = processState.clipped;
                height = yclip;
            }

            for (int x = 0; x < xmcu; ++x)
            {
                decodeState.decode(data, &decodeState);
                handleRestart();

                if (xclip && x == xmcu - 1)
                {
                    process = processState.clipped;
                    width = xclip;
                }

                process(dest, stride, data, &processState, width, height);
                dest += xstride;
            }

            image += ystride;
        }
    }

    void Parser::decodeSequentialMT()
    {
        const int stride = m_surface->stride;
        const int xstride = m_surface->format.bytes() * xblock;
        const int ystride = stride * yblock;
        uint8* image = m_surface->address<uint8>(0, 0);

        ConcurrentQueue queue("jpeg.sequential", Priority::HIGH);

        if (!restartInterval)
        {
            BlockType* data = blockVector;
            const int mcu_data_size = blocks_in_mcu * 64;

            const int pool_size = ThreadPool::getInstanceSize();

            const int S = pool_size > 1 ? 4 * pool_size : 1;
            const int N = std::max(ymcu / S, pool_size);

            // use threadpool to process blocks
            for (int y = 0; y < ymcu; y += N)
            {
                const int y0 = y;
                const int y1 = std::min(y + N, ymcu);
                const int count = (y1 - y0) * xmcu;
                jpegPrint("Process: [%d, %d] --> ThreadPool.\n", y0, y1 - 1);

                BlockType* idata = data + y * (xmcu * mcu_data_size);

                for (int i = 0; i < count; ++i)
                {
                    decodeState.decode(idata + i * mcu_data_size, &decodeState);
                    handleRestart();
                }

                // enqueue task
                queue.enqueue([=] {
                    for (int y = y0; y < y1; ++y)
                    {
                        uint8* dest = image + y * ystride;
                        BlockType* source = data + y * xmcu * mcu_data_size;

                        ProcessFunc process = processState.process;
                        int width = xblock;
                        int height = yblock;

                        if (yclip && y == ymcu - 1)
                        {
                            process = processState.clipped;
                            height = yclip;
                        }

                        for (int x = 0; x < xmcu; ++x)
                        {
                            if (xclip && x == xmcu - 1)
                            {
                                process = processState.clipped;
                                width = xclip;
                            }

                            process(dest, stride, source, &processState, width, height);
                            source += mcu_data_size;
                            dest += xstride;
                        }
                    }
                });
            }
        }
        else
        {
            const uint8* p = decodeState.buffer.ptr;

            for (int i = 0; i < mcus; i += restartInterval)
            {
                // enqueue task
                queue.enqueue([=] {
                    BlockType data[640]; // TODO: alignment
                    DecodeState state = decodeState;
                    state.buffer.ptr = p;

                    const int left = std::min(restartInterval, mcus - i);

                    for (int j = 0; j < left; ++j)
                    {
                        int n = i + j;

                        state.decode(data, &state);

                        int x = n % xmcu;
                        int y = n / xmcu;

                        uint8* dest = image + y * ystride + x * xstride;

                        ProcessFunc process = processState.process;
                        int width = xblock;
                        int height = yblock;

                        if (xclip && x == xmcu - 1)
                        {
                            process = processState.clipped;
                            width = xclip;
                        }

                        if (yclip && y == ymcu - 1)
                        {
                            process = processState.clipped;
                            height = yclip;
                        }

                        process(dest, stride, data, &processState, width, height);
                    }
                });

                // seek next restart marker
                p = seekMarker(p, decodeState.buffer.end);
                p += 2;
            }

            decodeState.buffer.ptr = p;
        }

        // synchronize
        queue.wait();
    }

    void Parser::decodeProgressive()
    {
        const bool dc_scan = (decodeState.spectralStart == 0);
        BlockType* data = blockVector;

        if (dc_scan)
        {
            if (decodeState.comps_in_scan == 1 && decodeState.blocks > 1)
            {
                // HACK: process 8x8 blocks in more expensive ac scanner code below
                // NOTE: we propably should do this only when 8x8 block count is not multiple of MCU block size
                decodeState.block[0].offset = 0;
                decodeState.blocks = 1;
            }
            else
            {
                for (int i = 0; i < mcus; ++i)
                {
                    decodeState.decode(data, &decodeState);
                    handleRestart();
                    data += blocks_in_mcu * 64;
                }

                return;
            }
        }

        const int hsf = u32_log2(scanFrame->Hsf);
        const int vsf = u32_log2(scanFrame->Vsf);
        const int hsize = (Hmax >> hsf) * 8;
        const int vsize = (Vmax >> vsf) * 8;

        jpegPrint("    hf: %i x %i, log2: %i x %i\n", 1 << hsf, 1 << vsf, hsf, vsf);
        jpegPrint("    bs: %i x %i  scanSize: %d\n", hsize, vsize, decodeState.blocks);

        const int scan_offset = scanFrame->offset;

        const int xs = ((xsize + hsize - 1) / hsize);
        const int ys = ((ysize + vsize - 1) / vsize);

        jpegPrint("    blocks: %d x %d (%d x %d)\n", xs, ys, xs * hsize, ys * vsize);

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
                BlockType* mcudata = data + (block_offset + mcu_offset) * 64;

                // decode
                decodeState.decode(mcudata, &decodeState);
                handleRestart();
            }
        }
    }

    void Parser::finishProgressive()
    {
#ifdef JPEG_ENABLE_THREAD
        const int count = ThreadPool::getInstanceSize();
#else
        const int count = 1;
#endif
        if (count > 1)
        {
            finishProgressiveMT();
        }
        else
        {
            finishProgressiveST();
        }
    }

    void Parser::finishProgressiveST()
    {
        const int stride = m_surface->stride;
        const int xstride = m_surface->format.bytes() * xblock;
        const int ystride = stride * yblock;
        uint8* image = m_surface->address<uint8>(0, 0);

        const int mcu_data_size = blocks_in_mcu * 64;
        BlockType* data = blockVector;

        for (int y = 0; y < ymcu; ++y)
        {
            uint8* dest = image + y * ystride;

            ProcessFunc process = processState.process;
            int width = xblock;
            int height = yblock;

            if (yclip && y == ymcu - 1)
            {
                process = processState.clipped;
                height = yclip;
            }

            for (int x = 0; x < xmcu; ++x)
            {
                if (xclip && x == xmcu - 1)
                {
                    process = processState.clipped;
                    width = xclip;
                }

                process(dest, stride, data, &processState, width, height);
                data += mcu_data_size;
                dest += xstride;
            }
        }
    }

    void Parser::finishProgressiveMT()
    {
        const int stride = m_surface->stride;
        const int xstride = m_surface->format.bytes() * xblock;
        const int ystride = stride * yblock;
        uint8* image = m_surface->address<uint8>(0, 0);

        const int mcu_data_size = blocks_in_mcu * 64;
        BlockType* data = blockVector;

        ConcurrentQueue queue("jpeg.progressive", Priority::HIGH);
        const int pool_size = ThreadPool::getInstanceSize();

        const int S = pool_size > 1 ? 4 * pool_size : 1;
        const int N = std::max(ymcu / S, pool_size);

        // use threadpool to process blocks
        for (int y = 0; y < ymcu; y += N)
        {
            const int y0 = y;
            const int y1 = std::min(y + N, ymcu);
            jpegPrint("Process: [%d, %d] --> ThreadPool.\n", y0, y1 - 1);

            // enqueue task
            queue.enqueue([=] {
                for (int y = y0; y < y1; ++y)
                {
                    uint8* dest = image + y * ystride;
                    BlockType* source = data + y * xmcu * mcu_data_size;

                    ProcessFunc process = processState.process;
                    int width = xblock;
                    int height = yblock;

                    if (yclip && y == ymcu - 1)
                    {
                        process = processState.clipped;
                        height = yclip;
                    }

                    for (int x = 0; x < xmcu; ++x)
                    {
                        if (xclip && x == xmcu - 1)
                        {
                            process = processState.clipped;
                            width = xclip;
                        }

                        process(dest, stride, source, &processState, width, height);
                        source += mcu_data_size;
                        dest += xstride;
                    }
                }
            });
        }

        // synchronize
        queue.wait();
    }

} // namespace jpeg

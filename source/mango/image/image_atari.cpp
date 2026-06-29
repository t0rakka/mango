/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    ATARI decoders copyright (C) 2011 Toni Lönnberg. All rights reserved.
*/
#include <cmath>
#include <vector>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

// References:
// - https://www.atari-wiki.com/index.php?title=ST_Picture_Formats
// - https://wiki.multimedia.cx/index.php?title=Degas

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
        }

        ~Interface()
        {
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            DecodeTargetBitmap target(dest, header.width, header.height, header.format);
            decodeImage(target);
            target.resolve();

            status.direct = target.isDirect();

            return status;
        }

        virtual void decodeImage(const Surface& dest) = 0;
    };

    // ------------------------------------------------------------
    // ST helper functions
    // ------------------------------------------------------------

    Color convert_atari_color(u16 atari_color)
    {
        Color color;

        color.r = ((atari_color & 0x700) >> 3) | ((atari_color & 0x800) >> 7) | ((atari_color & 0x700) >> 7) | ((atari_color & 0x800) >> 11);
        color.g = ((atari_color & 0x70 ) << 1) | ((atari_color & 0x80 ) >> 3) | ((atari_color & 0x70 ) >> 3) | ((atari_color & 0x80 ) >> 7);
        color.b = ((atari_color & 0x7  ) << 5) | ((atari_color & 0x8  ) << 1) | ((atari_color & 0x7  ) << 1) | ((atari_color & 0x8  ) >> 3);
        color.a = 0xff;

        return color;
    }

    // ------------------------------------------------------------
    // ImageDecoder: Degas/Degas Elite
    // ------------------------------------------------------------

    void degas_decompress(u8* buffer, const u8* input, const u8* input_end, int scansize)
    {
        u8* buffer_end = buffer + scansize;

        for ( ; buffer < buffer_end && input < input_end; )
        {
            u8 v = *input++;

            if (v > 128)
            {
                if (input >= input_end)
                    break;

                int n = 257 - v;
                if (n > buffer_end - buffer)
                    n = int(buffer_end - buffer);

                std::memset(buffer, *input++, n);
                buffer += n;
            }
            else if (v < 128)
            {
                int n = v + 1;
                if (n > input_end - input)
                    n = int(input_end - input);
                if (n > buffer_end - buffer)
                    n = int(buffer_end - buffer);

                std::memcpy(buffer, input, n);
                input += n;
                buffer += n;
            }
            else
            {
                // 0x80: PackBits no-op (skip the control byte and continue),
                // not an end-of-stream marker.
                continue;
            }
        }
    }

    struct header_degas
    {
        int width = 0;
        int height = 0;
        int bitplanes = 0;
        bool compressed = false;

        std::string error;

        const u8* parse(const u8* data, size_t size)
        {
            // resolution word (2) + 16-entry palette (32) read at decode time
            if (size < 34)
            {
                error = "[ImageDecoder.ATARI] Out of data.";
                return nullptr;
            }

            BigEndianConstPointer p = data;

            u16 resolution_data = p.read16();
            u8 resolution = (resolution_data & 0x3);

            if (resolution == 0)
            {
                width = 320;
                height = 200;
                bitplanes = 4;
            }
            else if (resolution == 1)
            {
                width = 640;
                height = 200;
                bitplanes = 2;
            }
            else if (resolution == 2)
            {
                width = 640;
                height = 400;
                bitplanes = 1;
            }
            else
            {
                error = "[ImageDecoder.ATARI] unsupported resolution.";
                return nullptr;
            }

            compressed = (resolution_data & 0x8000) != 0;

            if (!compressed)
            {
                if (size < 32034)
                {
                    error = "[ImageDecoder.ATARI] Out of data.";
                    return nullptr;
                }
            }

            return p;
        }

        void decode(const Surface& s, Palette& palette, const u8* data, const u8* end)
        {
            Buffer tempImage(width * height, 0);

            const int words_per_scan = bitplanes == 1 ? 40 : 80;

            // High resolution (640x400) is hardware monochrome; the stored
            // palette is not used by the display, so force black/white. This
            // must apply to both compressed (.PC3) and uncompressed (.PI3).
            if (bitplanes == 1)
            {
                palette.color[0] = 0xffeeeeee;
                palette.color[1] = 0xff000000;
            }

            if (compressed)
            {
                Buffer buffer(32000);
                degas_decompress(buffer, data, end, 32000);

                BigEndianConstPointer p = buffer.data();

                for (int y = 0; y < height; ++y)
                {
                    u8* image = tempImage + y * width;

                    for (int j = 0; j < bitplanes; ++j)
                    {
                        for (int k = 0; k < words_per_scan / bitplanes; ++k)
                        {
                            u16 word = p.read16();

                            for (int l = 15; l >= 0; --l)
                            {
                                image[k * 16 + (15 - l)] |= (((word & (1 << l)) >> l) << j);
                            }
                        }
                    }
                }
            }
            else
            {
                const bigEndian::u16* buffer = reinterpret_cast<const bigEndian::u16 *>(data);

                for (int y = 0; y < height; ++y)
                {
                    u8* image = tempImage.data() + y * width;
                    int yoffset = y * (bitplanes == 1 ? 40 : 80);

                    for (int x = 0; x < width; ++x)
                    {
                        int x_offset = 15 - (x & 15);
                        int word_offset = (x >> 4) * bitplanes + yoffset;

                        u8 index = 0;
                        for (int i = 0; i < bitplanes; ++i)
                        {
                            u16 v = buffer[word_offset + i];
                            int bit_pattern = (v >> x_offset) & 0x1;
                            index |= (bit_pattern << i);
                        }

                        image[x] = index;
                    }
                }
            }

            Surface indices(width, height, IndexedFormat(8), width, tempImage);
            indices.palette = &palette;
            resolve(s, indices);
        }
    };

    struct InterfaceDEGAS : Interface
    {
        header_degas m_degas_header;
        const u8* m_data;

        InterfaceDEGAS(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_degas_header.parse(memory.address, memory.size);
            if (m_data)
            {
                header.width  = m_degas_header.width;
                header.height = m_degas_header.height;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
            else
            {
                header.setError(m_degas_header.error);
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.end();
            BigEndianConstPointer p = m_data;

            // read palette
            Palette palette(1 << m_degas_header.bitplanes);

            for (int i = 0; i < 16; ++i)
            {
                u16 color = p.read16();
                palette[i] = convert_atari_color(color);
            }

            // decode image
            m_degas_header.decode(s, palette, p, end);
        }
    };

    ImageDecodeInterface* createInterfaceDEGAS(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceDEGAS(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: NEOchrome
    // ------------------------------------------------------------

    struct header_neo
    {
        int width = 0;
        int height = 0;
        int bitplanes;

        std::string error;

        const u8* parse(const u8* data, size_t size)
        {
            if (size != 32128)
            {
                error = "[ImageDecoder.ATARI] Incorrect number of bytes.";
                return nullptr;
            }

            BigEndianConstPointer p = data;

            u16 flag = p.read16();
            u16 resolution_data = p.read16();

            if (flag)
            {
                error = "[ImageDecoder.ATARI] Incorrect flags.";
                return nullptr;
            }
            else
            {
                if (resolution_data == 0)
                {
                    width = 320;
                    height = 200;
                    bitplanes = 4;
                }
                else if (resolution_data == 1)
                {
                    width = 640;
                    height = 200;
                    bitplanes = 2;
                }
                else if (resolution_data == 2)
                {
                    width = 640;
                    height = 400;
                    bitplanes = 1;
                }
                else
                {
                    error = "[ImageDecoder.ATARI] Unsupported resolution.";
                    return nullptr;
                }
            }

            return p;
        }

        void decode(const Surface& s, const u8* data, const u8* end)
        {
            BigEndianConstPointer p = data;

            // read palette
            Palette palette(16);

            for (int i = 0; i < 16; ++i)
            {
                u16 palette_color = p.read16();
                palette[i] = convert_atari_color(palette_color);
            }

            // patch palette
            if (bitplanes == 1)
            {
                palette.color[0] = 0xffeeeeee;
                palette.color[1] = 0xff000000;
            }

            // Skip unnecessary data
            p += 12 + 2 + 2 + 2 + 2 + 2 + 2 + 2 + 66;

            const int num_words = 16000;
            Buffer buffer(width * height);

            for (int i = 0; i < (num_words / bitplanes); ++i)
            {
                u16 word[4] = { 0 };

                for (int j = 0; j < bitplanes; ++j)
                {
                    word[j] = p.read16();

                    if (p > end)
                    {
                        return;
                    }
                }

                for (int j = 15; j >= 0; --j)
                {
                    u8 index = 0;

                    for (int k = 0; k < bitplanes; ++k)
                    {
                        index |= (((word[k] & (1 << j)) >> j) << k);
                    }

                    buffer[(i * 16) + (15 - j)] = index;
                }
            }

            Surface indices(width, height, IndexedFormat(8), width, buffer);
            indices.palette = &palette;
            resolve(s, indices);
        }
	};

    struct InterfaceNEO : Interface
    {
        header_neo m_neo_header;
        const u8* m_data;

        InterfaceNEO(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_neo_header.parse(memory.address, memory.size);
            if (m_data)
            {
                header.width  = m_neo_header.width;
                header.height = m_neo_header.height;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
            else
            {
                header.setError(m_neo_header.error);
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.end();

            m_neo_header.decode(s, m_data, end);
        }
    };

    ImageDecodeInterface* createInterfaceNEO(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceNEO(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: Spectrum 512
    // ------------------------------------------------------------

    u8 find_spectrum_palette_index(u32 x, u8 c)
    {
        u32 t = 10 * c;

        if (c & 1)
            t -= 5;
        else
            t++;

        if (x < t)
            return c;
        if (x >= t + 160)
            return c + 32;

        return c + 16;
    }

    void spu_decompress(u8* buffer, const u8* input, int scansize, int insize)
    {
        u8* buffer_end = buffer + scansize;
        const u8* input_end = input + insize;

        for ( ; buffer < buffer_end && input < input_end; )
        {
            u8 v = *input++;

            if (v >= 128)
            {
                if (input >= input_end)
                    break;

                int n = 258 - v;
                if (n > buffer_end - buffer)
                    n = int(buffer_end - buffer);

                std::memset(buffer, *input++, n);
                buffer += n;
            }
            else if (v < 128)
            {
                int n = v + 1;
                if (n > input_end - input)
                    n = int(input_end - input);
                if (n > buffer_end - buffer)
                    n = int(buffer_end - buffer);

                std::memcpy(buffer, input, n);
                input += n;
                buffer += n;
            }
        }
    }

    struct header_spu
    {
        int width = 0;
        int height = 0;
        int bitplanes = 4;
        bool compressed = false;
        int length_of_data_bit_map = 0;
        int length_of_color_bit_map = 0;
        
        std::string error;

        const u8* parse(const u8* data, size_t size)
        {
            if (size < 12)
            {
                error = "[ImageDecoder.ATARI] Incorrect header size.";
                return nullptr;
            }

            BigEndianConstPointer p = data;

            u16 flag = p.read16();

            if (flag == 0x5350)
            {
                compressed = true;
                p += 2; // skip reserved
                length_of_data_bit_map = p.read32();
                length_of_color_bit_map = p.read32();

                const size_t total_size = 12 + length_of_data_bit_map + length_of_color_bit_map;
                const size_t total_size_other = total_size + 78; // HACK: some files also have this size..

                if (size != total_size && size != total_size_other)
                {
                    error = "[ImageDecoder.ATARI] Incorrect number of bytes.";
                    return nullptr;
                }
            }
            else
            {
                if (size != 51104)
                {
                    error = "[ImageDecoder.ATARI] Incorrect number of bytes.";
                    return nullptr;
                }

                p -= sizeof(u16);
            }

            width = 320;
            height = 200;

            return p;
        }

        void decode(const Surface& s, const u8* data, const u8* end)
        {
            BigEndianConstPointer p = data;

            Buffer bitmap(width * height, 0);
            std::vector<Color> palette(16 * 3 * (height - 1), Color(0, 0, 0, 0xff));

            int num_words = 16000;
            int words_per_scan = 20;

            if (compressed)
            {
                Buffer buffer(31840);
                spu_decompress(buffer.data(), p, 31840, length_of_data_bit_map);

                p = buffer.data();
                end = buffer.data() + 31840;

                for (int i = 0; i < bitplanes; ++i)
                {
                    for (int j = 1; j < height; ++j)
                    {
                        for (int k = 0; k < words_per_scan; ++k)
                        {
                            u16 word = p.read16();

                            if (p > end)
                            {
                                return;
                            }

                            for (int l = 15; l >= 0; --l)
                            {
                                bitmap[(j * width) + (k * 16) + (15 - l)] |= (((word & (1 << l)) >> l) << i);
                            }
                        }
                    }
                }

                p = data + length_of_data_bit_map;
                end = p + length_of_color_bit_map;

                // palette holds 16 * 3 * (height - 1) entries, written in groups of 16
                int palette_set = 0;
                const int max_palette_sets = 3 * (height - 1);
                while (p + 2 <= end && palette_set < max_palette_sets)
                {
                    u16 vector = p.read16();

                    for (int i = 0; i < 16; ++i)
                    {
                        if (vector & (1 << i))
                        {
                            if (p + 2 > end)
                                break;

                            int index = (palette_set * 16) + i;
                            u16 palette_color = p.read16();
                            palette[index] = convert_atari_color(palette_color);
                        }
                    }

                    ++palette_set;
                }
            }
            else
            {
                for (int i = 0; i < (num_words / bitplanes); ++i)
                {
                    u16 word[4] = { 0 };

                    for (int j = 0; j < bitplanes; ++j)
                    {
                        word[j] = p.read16();

                        if (p > end)
                        {
                            return;
                        }
                    }

                    for (int j = 15; j >= 0; --j)
                    {
                        u8 index = 0;

                        for (int k = 0; k < bitplanes; ++k)
                        {
                            index |= (((word[k] & (1 << j)) >> j) << k);
                        }

                        bitmap[(i * 16) + (15 - j)] = index;
                    }
                }

                for (int i = 0; i < (height - 1); ++i)
                {
                    for (int j = 0; j < 16 * 3; ++j)
                    {
                        u16 palette_color = p.read16();
                        int index = (i * 16 * 3) + j;

                        if (p > end)
                        {
                            return;
                        }

                        palette[index] = convert_atari_color(palette_color);
                    }
                }
            }

            // HACK: clear first scanline - not sure what we should do here..
            std::memset(s.address<u8>(0, 0), 0, width * 4);

            // Resolve palette
            for (int y = 1; y < height; ++y)
            {
                Color* image = s.address<Color>(0, y);

                for (int x = 0; x < width; ++x)
                {
                    u8 palette_index = bitmap[y * width + x];
                    palette_index = find_spectrum_palette_index(x, palette_index);

                    int index = (y - 1) * 16 * 3 + palette_index;
                    image[x] = palette[index];
                }
            }
        }
    };

    struct InterfaceSPU : Interface
    {
        header_spu m_spu_header;
        const u8* m_data;

        InterfaceSPU(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_spu_header.parse(memory.address, memory.size);
            if (m_data)
            {
                header.width  = m_spu_header.width;
                header.height = m_spu_header.height;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
            else
            {
                header.setError(m_spu_header.error);
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* data = m_data;
            const u8* end = m_memory.end();

            m_spu_header.decode(s, data, end);
        }
    };

    ImageDecodeInterface* createInterfaceSPU(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceSPU(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: Spectrum 512 Smooshed (.sps)
    // ------------------------------------------------------------

    // Smooshed compresses the same 320x199 Spectrum 512 picture as .SPC but
    // with a different data RLE and a bit-packed colour map. There are two data
    // layout variants, selected by the LSB of the last colour-map byte:
    //   1 = SPSLIDEX order (identical to .SPC: plane, scanline, word)
    //   0 = ANISPEC order  (byte-wide vertical strips: plane, byte-column, scanline)

    void sps_decompress(u8* buffer, int scansize, const u8* input, const u8* input_end)
    {
        u8* buffer_end = buffer + scansize;

        while (buffer < buffer_end && input < input_end)
        {
            u8 x = *input++;

            if (x <= 127)
            {
                // repeat the next byte (x + 3) times
                if (input >= input_end)
                    break;

                int n = x + 3;
                if (n > buffer_end - buffer)
                    n = int(buffer_end - buffer);

                std::memset(buffer, *input++, n);
                buffer += n;
            }
            else
            {
                // (x - 128 + 1) literal bytes
                int n = x - 127;
                if (n > input_end - input)
                    n = int(input_end - input);
                if (n > buffer_end - buffer)
                    n = int(buffer_end - buffer);

                std::memcpy(buffer, input, n);
                input += n;
                buffer += n;
            }
        }
    }

    // MSB-first bit reader; reads zero-padding once the source is exhausted.
    struct SmooshBitReader
    {
        const u8* p;
        const u8* end;
        u32 buffer = 0;
        int count = 0;

        SmooshBitReader(const u8* begin, const u8* finish)
            : p(begin)
            , end(finish)
        {
        }

        u32 read(int bits)
        {
            while (count < bits)
            {
                u8 b = (p < end) ? *p++ : 0;
                buffer = (buffer << 8) | b;
                count += 8;
            }
            count -= bits;
            return (buffer >> count) & ((1u << bits) - 1);
        }
    };

    struct header_sps
    {
        int width = 320;
        int height = 200;
        int length_of_data_bit_map = 0;
        int length_of_color_bit_map = 0;

        std::string error;

        const u8* parse(const u8* data, size_t size)
        {
            if (size < 12)
            {
                error = "[ImageDecoder.ATARI] Incorrect header size.";
                return nullptr;
            }

            BigEndianConstPointer p = data;

            u16 flag = p.read16();
            if (flag != 0x5350)
            {
                error = "[ImageDecoder.ATARI] Incorrect header.";
                return nullptr;
            }

            p += 2; // reserved
            length_of_data_bit_map = p.read32();
            length_of_color_bit_map = p.read32();

            if (length_of_data_bit_map <= 0 || length_of_color_bit_map <= 0)
            {
                error = "[ImageDecoder.ATARI] Incorrect bitmap length.";
                return nullptr;
            }

            // the declared data + colour sections must be present (reject truncated files)
            const size_t required = size_t(12) + size_t(length_of_data_bit_map) + size_t(length_of_color_bit_map);
            if (required > size)
            {
                error = "[ImageDecoder.ATARI] Truncated file.";
                return nullptr;
            }

            return p; // points at the compressed data bitmap (offset 12)
        }

        void decode(const Surface& s, const u8* data, const u8* end)
        {
            // section boundaries, clamped to the available data
            const u8* data_map = data;
            const u8* data_map_end = data_map + length_of_data_bit_map;
            if (data_map_end > end)
                data_map_end = end;

            const u8* color_map = data + length_of_data_bit_map;
            if (color_map > end)
                color_map = end;
            const u8* color_map_end = color_map + length_of_color_bit_map;
            if (color_map_end > end)
                color_map_end = end;

            // variant select: LSB of the last colour-map byte.
            // 1 = SPSLIDEX (SPC order), 0 = ANISPEC (byte-wide vertical strips).
            // Verified visually against LSB=0 and LSB=1 sample files.
            int variant = 0;
            if (color_map_end > color_map)
                variant = color_map_end[-1] & 1;

            // decompress the data bitmap (199 scanlines x 4 planes x 40 bytes)
            Buffer databuf(31840, 0);
            sps_decompress(databuf.data(), 31840, data_map, data_map_end);

            const u8* b = databuf.data();
            const u8* b_end = b + 31840;

            Buffer bitmap(width * height, 0);

            if (variant == 1)
            {
                // SPSLIDEX: plane, scanline, word
                BigEndianConstPointer p = b;

                for (int i = 0; i < 4; ++i)
                {
                    for (int j = 1; j < height; ++j)
                    {
                        for (int k = 0; k < 20; ++k)
                        {
                            if (p + 2 > b_end)
                                break;

                            u16 word = p.read16();

                            for (int l = 15; l >= 0; --l)
                            {
                                bitmap[j * width + k * 16 + (15 - l)] |= (((word >> l) & 1) << i);
                            }
                        }
                    }
                }
            }
            else
            {
                // ANISPEC: plane, byte-column (8 pixels), scanline
                const u8* q = b;

                for (int plane = 0; plane < 4; ++plane)
                {
                    for (int xcol = 0; xcol < 40; ++xcol)
                    {
                        for (int j = 1; j < height; ++j)
                        {
                            if (q >= b_end)
                                break;

                            u8 byte = *q++;

                            for (int bit = 0; bit < 8; ++bit)
                            {
                                int px = xcol * 8 + bit;
                                bitmap[j * width + px] |= (((byte >> (7 - bit)) & 1) << plane);
                            }
                        }
                    }
                }
            }

            // decompress the bit-packed colour map: 597 records, each a 14-bit
            // inclusion mask (MSB = entry 1) followed by 9-bit rgb (rrrgggbbb)
            // entries. Palette entries 0 and 15 are always black.
            std::vector<Color> palette(16 * 3 * (height - 1), Color(0, 0, 0, 0xff));

            SmooshBitReader br(color_map, color_map_end);
            const int num_sets = 3 * (height - 1);

            for (int set = 0; set < num_sets; ++set)
            {
                u32 mask = br.read(14);

                for (int e = 1; e <= 14; ++e)
                {
                    if (mask & (1u << (14 - e)))
                    {
                        u32 rgb = br.read(9);
                        u8 r = (rgb >> 6) & 7;
                        u8 g = (rgb >> 3) & 7;
                        u8 b3 = rgb & 7;

                        u16 atari = (u16(r) << 8) | (u16(g) << 4) | u16(b3);
                        palette[set * 16 + e] = convert_atari_color(atari);
                    }
                }
            }

            // render (Spectrum 512 per-scanline triple palette)
            std::memset(s.address<u8>(0, 0), 0, width * 4);

            for (int y = 1; y < height; ++y)
            {
                Color* image = s.address<Color>(0, y);

                for (int x = 0; x < width; ++x)
                {
                    u8 palette_index = bitmap[y * width + x];
                    palette_index = find_spectrum_palette_index(x, palette_index);

                    int index = (y - 1) * 16 * 3 + palette_index;
                    image[x] = palette[index];
                }
            }
        }
    };

    struct InterfaceSPS : Interface
    {
        header_sps m_sps_header;
        const u8* m_data;

        InterfaceSPS(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_sps_header.parse(memory.address, memory.size);
            if (m_data)
            {
                header.width  = m_sps_header.width;
                header.height = m_sps_header.height;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
            else
            {
                header.setError(m_sps_header.error);
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.end();
            m_sps_header.decode(s, m_data, end);
        }
    };

    ImageDecodeInterface* createInterfaceSPS(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceSPS(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: Crack Art
    // ------------------------------------------------------------

    void ca_decompress(u8* buffer, const u8* input, const u8* input_end, const int scansize, const u8 escape_char, const u16 offset)
    {
        u8* buffer_start = buffer;
        u8* buffer_end = buffer + scansize;

        int count = scansize;

        while (count > 0 && input < input_end)
        {
            u8 v = *input++;
            if (v != escape_char)
            {
                *buffer = v;
                --count;

                buffer += offset;
                if (buffer >= buffer_end)
                {
                    ++buffer_start;
                    buffer = buffer_start;
                }
            }
            else
            {
                if (input >= input_end)
                    break;

                v = *input++;
                if (v == 0)
                {
                    if (input + 2 > input_end)
                        break;

                    int n = *input++;
                    ++n;

                    v = *input++;

                    if (n > count)
                    {
                        n = count;
                    }

                    count -= n;

                    while (n > 0)
                    {
                        *buffer = v;
                        --n;

                        buffer += offset;
                        if (buffer >= buffer_end)
                        {
                            ++buffer_start;
                            buffer = buffer_start;
                        }
                    }
                }
                else if (v == 1)
                {
                    if (input + 3 > input_end)
                        break;

                    int n = *input++;
                    n <<= 8;
                    n += *input++;
                    ++n;

                    v = *input++;

                    if (n > count)
                    {
                        n = count;
                    }

                    count -= n;

                    while (n > 0)
                    {
                        *buffer = v;
                        --n;

                        buffer += offset;
                        if (buffer >= buffer_end)
                        {
                            ++buffer_start;
                            buffer = buffer_start;
                        }
                    }
                }
                else if (v == 2)
                {
                    int n;

                    if (input >= input_end)
                        break;

                    v = *input++;

                    if (v == 0)
                    {
                        n = count;
                    }
                    else
                    {
                        if (input >= input_end)
                            break;

                        n = v;
                        n <<= 8;
                        n += *input++;
                        ++n;

                        if (n > count)
                        {
                            n = count;
                        }
                    }

                    count -= n;

                    while (n > 0)
                    {
                        --n;

                        buffer += offset;
                        if (buffer >= buffer_end)
                        {
                            ++buffer_start;
                            buffer = buffer_start;
                        }
                    }
                }
                else if (v == escape_char)
                {
                    *buffer = v;
                    --count;

                    buffer += offset;
                    if (buffer >= buffer_end)
                    {
                        ++buffer_start;
                        buffer = buffer_start;
                    }
                }
                else
                {
                    if (input >= input_end)
                        break;

                    int n = v;
                    ++n;

                    v = *input++;

                    if (n > count)
                    {
                        n = count;
                    }

                    count -= n;

                    while (n > 0)
                    {
                        *buffer = v;
                        --n;

                        buffer += offset;
                        if (buffer >= buffer_end)
                        {
                            ++buffer_start;
                            buffer = buffer_start;
                        }
                    }
                }
            }
        }
    }

    struct header_ca
    {
        int width = 0;
        int height = 0;
        int bitplanes = 0;
        bool compressed = false;

        std::string error;

        const u8* parse(const u8* data, size_t size)
        {
            // "CA" magic (2) + compressed flag (1) + resolution (1)
            if (size < 4)
            {
                error = "[ImageDecoder.ATARI] Out of data.";
                return nullptr;
            }

            BigEndianConstPointer p = data;

            if (std::memcmp(p, "CA", 2))
            {
                error = "[ImageDecoder.ATARI] Incorrect header.";
                return nullptr;
            }

            p += 2;

            compressed = p.read8() != 0;
            u8 resolution = p.read8();

            if (resolution == 0)
            {
                width = 320;
                height = 200;
                bitplanes = 4;
            }
            else if (resolution == 1)
            {
                width = 640;
                height = 200;
                bitplanes = 2;
            }
            else if (resolution == 2)
            {
                width = 640;
                height = 400;
                bitplanes = 1;
            }
            else
            {
                error = "[ImageDecoder.ATARI] Unsupported resolution.";
                return nullptr;
            }

            if (!compressed)
            {
                if (size < size_t(32000 + 2 + 1 + 1 + (1 << bitplanes)))
                {
                    error = "[ImageDecoder.ATARI] Out of data.";
                    return nullptr;
                }
            }
            else
            {
                // header (4) + palette ((1<<bitplanes) colors x 2) + escape/initial/offset (4)
                if (size < size_t(4 + (1 << bitplanes) * 2 + 4))
                {
                    error = "[ImageDecoder.ATARI] Out of data.";
                    return nullptr;
                }
            }

            return p;
        }

        void decode(const Surface& s, const u8* data, const u8* end)
        {
            BigEndianConstPointer p = data;

            Palette palette(1 << bitplanes);

            for (u32 i = 0; i < palette.size; ++i)
            {
                u16 palette_color = p.read16();
                palette[i] = convert_atari_color(palette_color);
            }

            // High resolution (640x400) is hardware monochrome; force black/white.
            if (bitplanes == 1)
            {
                palette.color[0] = 0xffeeeeee;
                palette.color[1] = 0xff000000;
            }

            Buffer temp;
            const u8* buffer = p;

            if (compressed)
            {
                const u8 escape_char = p.read8();
                const u8 initial_value = p.read8();
                const u16 offset = p.read16() & 0x7fff;

                temp.reset(32000, initial_value);
                ca_decompress(temp.data(), p, end, 32000, escape_char, offset);

                buffer = temp.data();
                end = temp.data() + 32000;
            }

            p = buffer;

            const int num_words = 16000;
            Buffer image(width * height);

            for (int i = 0; i < (num_words / bitplanes); ++i)
            {
                u16 word[4] = { 0 };

                for (int j = 0; j < bitplanes; ++j)
                {
                    word[j] = p.read16();

                    if (p > end)
                    {
                        return;
                    }
                }

                for (int j = 15; j >= 0; --j)
                {
                    u8 index = 0;

                    for (int k = 0; k < bitplanes; ++k)
                    {
                        index |= (((word[k] & (1 << j)) >> j) << k);
                    }

                    image[(i * 16) + (15 - j)] = index;
                }
            }

            Surface indices(width, height, IndexedFormat(8), width, image);
            indices.palette = &palette;
            resolve(s, indices);
        }
	};

    struct InterfaceCA : Interface
    {
        header_ca m_ca_header;
        const u8* m_data;

        InterfaceCA(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_ca_header.parse(memory.address, memory.size);
            if (m_data)
            {
                header.width  = m_ca_header.width;
                header.height = m_ca_header.height;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
            else
            {
                header.setError(m_ca_header.error);
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.end();
            m_ca_header.decode(s, m_data, end);
        }
    };

    ImageDecodeInterface* createInterfaceCA(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceCA(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: Tiny (Tiny Stuff)
    // ------------------------------------------------------------

    // The Tiny screen is the 32000-byte ST screen RAM (16000 words) stored as a
    // word-oriented RLE over a column-interleaved layout. Each scanline holds W
    // words (80 for low/medium, 40 for high resolution); a "column" is one word
    // position taken across every scanline. The columns are emitted in four
    // interleaved sets { 0,4,8,.. }, { 1,5,9,.. }, { 2,6,10,.. }, { 3,7,11,.. }
    // to improve the run-length compression of vertically coherent content.

    struct header_tiny
    {
        int width = 0;
        int height = 0;
        int bitplanes = 0;
        bool animation = false; // resolution byte was +3 -> colour rotation header present

        std::string error;

        const u8* parse(const u8* data, size_t size)
        {
            if (size < 1)
            {
                error = "[ImageDecoder.ATARI] Out of data.";
                return nullptr;
            }

            u8 resolution = data[0];
            animation = resolution > 2;
            u8 mode = animation ? u8(resolution - 3) : resolution;

            if (mode == 0)
            {
                width = 320;
                height = 200;
                bitplanes = 4;
            }
            else if (mode == 1)
            {
                width = 640;
                height = 200;
                bitplanes = 2;
            }
            else if (mode == 2)
            {
                width = 640;
                height = 400;
                bitplanes = 1;
            }
            else
            {
                error = "[ImageDecoder.ATARI] Unsupported resolution.";
                return nullptr;
            }

            // resolution(1) [+ rotation header(4)] + palette(32) + control_count(2) + data_count(2)
            const size_t header_bytes = 1 + (animation ? 4 : 0) + 32 + 2 + 2;
            if (size < header_bytes)
            {
                error = "[ImageDecoder.ATARI] Out of data.";
                return nullptr;
            }

            return data;
        }

        void decode(const Surface& s, const u8* data, const u8* end)
        {
            BigEndianConstPointer p = data;

            p += 1; // resolution byte (validated in parse)

            // optional colour-animation header (present when resolution byte was +3)
            if (animation)
            {
                p += 4; // limits(1) + direction/speed(1) + duration(2)
            }

            // palette
            Palette palette(16);
            for (int i = 0; i < 16; ++i)
            {
                palette[i] = convert_atari_color(p.read16());
            }

            if (bitplanes == 1)
            {
                palette.color[0] = 0xffeeeeee;
                palette.color[1] = 0xff000000;
            }

            if (p + 4 > end)
                return;

            const int num_control = p.read16();
            const int num_data = p.read16();

            // The control section is followed immediately by the data section.
            const u8* control = p;
            const u8* control_end = control + num_control;
            if (control_end > end)
                control_end = end;

            const u8* data_words = control + num_control;
            const u8* data_end = data_words + size_t(num_data) * 2;
            if (data_end > end)
                data_end = end;
            if (data_words > end)
                data_words = end;

            // Decompress the control/data streams into 16000 screen words.
            std::vector<u16> expanded(16000, 0);
            int out = 0;

            BigEndianConstPointer cp = control;
            BigEndianConstPointer dp = data_words;

            while (out < 16000 && cp < control_end)
            {
                s8 x = s8(cp.read8());

                if (x < 0)
                {
                    // |x| unique words from the data section
                    int n = -int(x);
                    while (n-- > 0 && out < 16000)
                    {
                        if (dp + 2 > data_end)
                            break;
                        expanded[out++] = dp.read16();
                    }
                }
                else if (x == 0)
                {
                    // control word = repeat count for the next data word
                    if (cp + 2 > control_end)
                        break;
                    int count = cp.read16();
                    if (dp + 2 > data_end)
                        break;
                    u16 w = dp.read16();
                    while (count-- > 0 && out < 16000)
                        expanded[out++] = w;
                }
                else if (x == 1)
                {
                    // control word = number of unique words from the data section
                    if (cp + 2 > control_end)
                        break;
                    int count = cp.read16();
                    while (count-- > 0 && out < 16000)
                    {
                        if (dp + 2 > data_end)
                            break;
                        expanded[out++] = dp.read16();
                    }
                }
                else
                {
                    // x > 1: repeat the next data word x times
                    int count = x;
                    if (dp + 2 > data_end)
                        break;
                    u16 w = dp.read16();
                    while (count-- > 0 && out < 16000)
                        expanded[out++] = w;
                }
            }

            // De-interleave the column sets back into linear screen RAM. The
            // partition is a fixed 80-column x 200-row grid over the 16000-word
            // buffer for every resolution (the resolution-specific geometry is
            // applied later by the planar decoder). Verified against high-res
            // samples: a resolution-dependent grid tiles the image 2x2.
            const int rows = 200;
            const int words_per_set = 4000; // 20 columns x 200 rows

            std::vector<u16> screen(16000, 0);

            for (int i = 0; i < 16000; ++i)
            {
                const int set = i / words_per_set;
                const int within = i % words_per_set;
                const int k = within / rows;
                const int y = within % rows;
                const int col = set + 4 * k;
                screen[y * 80 + col] = expanded[i];
            }

            // Planar (interleaved bitplane) screen words -> palette indices.
            Buffer indices_buffer(width * height, 0);
            const int groups = 16000 / bitplanes;

            for (int i = 0; i < groups; ++i)
            {
                u16 word[4] = { 0 };

                for (int j = 0; j < bitplanes; ++j)
                {
                    word[j] = screen[i * bitplanes + j];
                }

                for (int j = 15; j >= 0; --j)
                {
                    u8 index = 0;

                    for (int k = 0; k < bitplanes; ++k)
                    {
                        index |= (((word[k] >> j) & 1) << k);
                    }

                    indices_buffer[i * 16 + (15 - j)] = index;
                }
            }

            Surface indices(width, height, IndexedFormat(8), width, indices_buffer);
            indices.palette = &palette;
            resolve(s, indices);
        }
    };

    struct InterfaceTiny : Interface
    {
        header_tiny m_tiny_header;
        const u8* m_data;

        InterfaceTiny(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_tiny_header.parse(memory.address, memory.size);
            if (m_data)
            {
                header.width  = m_tiny_header.width;
                header.height = m_tiny_header.height;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
            else
            {
                header.setError(m_tiny_header.error);
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.end();
            m_tiny_header.decode(s, m_data, end);
        }
    };

    ImageDecodeInterface* createInterfaceTiny(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceTiny(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecATARI()
    {
        // Degas/Degas Elite
        registerImageDecoder(createInterfaceDEGAS, ".pi1");
        registerImageDecoder(createInterfaceDEGAS, ".pi2");
        registerImageDecoder(createInterfaceDEGAS, ".pi3");
        registerImageDecoder(createInterfaceDEGAS, ".pc1");
        registerImageDecoder(createInterfaceDEGAS, ".pc2");
        registerImageDecoder(createInterfaceDEGAS, ".pc3");

        // NEOchrome
        registerImageDecoder(createInterfaceNEO, ".neo");

        // Spectrum 512
        registerImageDecoder(createInterfaceSPU, ".spu");
        registerImageDecoder(createInterfaceSPU, ".spc");
        registerImageDecoder(createInterfaceSPS, ".sps");

        // Crack Art
        registerImageDecoder(createInterfaceCA, ".ca1");
        registerImageDecoder(createInterfaceCA, ".ca2");
        registerImageDecoder(createInterfaceCA, ".ca3");

        // Tiny (Tiny Stuff)
        registerImageDecoder(createInterfaceTiny, ".tny");
        registerImageDecoder(createInterfaceTiny, ".tn1");
        registerImageDecoder(createInterfaceTiny, ".tn2");
        registerImageDecoder(createInterfaceTiny, ".tn3");
    }

} // namespace mango::image

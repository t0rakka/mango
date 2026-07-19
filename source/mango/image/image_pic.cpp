/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    constexpr size_t PICT_HEADER_SIZE = 104;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_data;

        Interface(ConstMemory memory)
        {
            // Guard the header size before computing the body slice; otherwise
            // (memory.size - PICT_HEADER_SIZE) underflows for short files.
            if (memory.size < PICT_HEADER_SIZE)
            {
                header.setError("[ImageDecoder.PIC] Not enough data.");
                return;
            }

            m_data = ConstMemory(memory.address + PICT_HEADER_SIZE, memory.size - PICT_HEADER_SIZE);

            BigEndianConstPointer p = memory;

            constexpr u8 identifier [] =
            {
                0x53, 0x80, 0xf6, 0x34
            };

            if (std::memcmp(p, identifier, sizeof(identifier)))
            {
                header.setError("[ImageDecoder.PIC] Incorrect identifier.");
                return;
            }

            p += sizeof(identifier);
            p += 4; // skip storage format
            p += 80; // skip comment

            u32 pict = p.read32();
            if (pict != 0x50494354)
            {
                header.setError("[ImageDecoder.PIC] Incorrect PICT.");
                return;
            }

            int width = p.read16();
            int height = p.read16();

            // a zero dimension yields an empty surface and no decodable body
            if (width < 1 || height < 1)
            {
                header.setError("[ImageDecoder.PIC] Invalid image dimensions ({} x {}).", width, height);
                return;
            }

            header.width   = width;
            header.height  = height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
            header.format  = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            header.compression = TextureCompression::NONE;
        }

        ~Interface()
        {
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);
            return ConstMemory();
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            const u8* end = m_data.end();
            const u8* p = m_data;

            constexpr int MAX_PACKETS = 10;

            struct Packet
            {
                u8 chained;
                u8 size;
                u8 type;
                u8 mask;
            };
            std::vector<Packet> packets;

            for (int i = 0; i < MAX_PACKETS; ++i)
            {
                if (p + 4 > end)
                {
                    status.setError("[ImageDecoder.PIC] Out of data.");
                    return status;
                }

                Packet packet;

                packet.chained = p[0];
                packet.size = p[1];
                packet.type = p[2];
                packet.mask = u8_reverse_bits(p[3]);
                p += 4;

                printLine(Print::Debug, "[packet {}] size: {}  type: {}  mask: {:#x}", i,
                    packet.size, packet.type, packet.mask);

                if (packet.size != 8)
                {
                    status.setError("[ImageDecoder.PIC] Incorrect packet size ({}).", packet.size);
                    return status;
                }

                if (packet.type > 2)
                {
                    status.setError("[ImageDecoder.PIC] Incorrect packet type ({}).", packet.type);
                    return status;
                }

                packets.push_back(packet);

                if (!packet.chained)
                {
                    break;
                }
            }

            int width = header.width;
            int height = header.height;

            Bitmap temp(width, height, header.format);
            temp.clear(0xff000000);

            for (int y = 0; y < height; ++y)
            {
                u8* dst = temp.address<u8>(0, y);

                for (const Packet& packet : packets)
                {
                    switch (packet.type)
                    {
                        case 0:
                            p = decode_uncompressed(dst, p, end, width, packet.mask);
                            break;

                        case 1:
                            p = decode_runlength(dst, p, end, width, packet.mask);
                            break;

                        case 2:
                            p = decode_mixed(dst, p, end, width, packet.mask);
                            break;
                    }

                    if (!p)
                    {
                        status.setError("[ImageDecoder.PIC] Truncated image data.");
                        return status;
                    }
                }
            }

            dest.blit(0, 0, temp);

            return status;
        }

        // The decoders below return nullptr on any attempt to read past the end of the
        // input; callers treat that as a truncated-stream error. 'count' is always clamped
        // to the remaining scanline width so the writes through 'dest' stay in bounds.

        const u8* read(u8* dest, const u8* p, const u8* end, u32 mask, int count) const
        {
            for (int channel = 0; channel < 4; ++channel)
            {
                if (mask & (1 << channel))
                {
                    if (p >= end)
                        return nullptr;

                    u8 value = *p++;

                    for (int i = 0; i < count; ++i)
                    {
                        dest[i * 4] = value;
                    }
                }

                ++dest;
            }

            return p;
        }

        const u8* decode_uncompressed(u8* dest, const u8* p, const u8* end, int width, u32 mask) const
        {
            for (int x = 0; x < width; ++x)
            {
                p = read(dest, p, end, mask, 1);
                if (!p)
                    return nullptr;
                dest += 4;
            }

            return p;
        }

        const u8* decode_runlength(u8* dest, const u8* p, const u8* end, int width, u32 mask) const
        {
            while (width > 0)
            {
                if (p >= end)
                    return nullptr;

                int count = *p++;
                count = std::min(count, width);

                p = read(dest, p, end, mask, count);
                if (!p)
                    return nullptr;
                dest += count * 4;
                width -= count;
            }

            return p;
        }

        const u8* decode_mixed(u8* dest, const u8* p, const u8* end, int width, u32 mask) const
        {
            while (width > 0)
            {
                if (p >= end)
                    return nullptr;

                int count = *p++;

                if (count < 128)
                {
                    ++count;
                    count = std::min(count, width);
                    for (int i = 0; i < count; ++i)
                    {
                        p = read(dest, p, end, mask, 1);
                        if (!p)
                            return nullptr;
                        dest += 4;
                    }
                }
                else
                {
                    if (count == 128)
                    {
                        if (p + 2 > end)
                            return nullptr;
                        count = bigEndian::uload16(p);
                        p += 2;
                    }
                    else
                    {
                        count -= 127;
                    }

                    // clamp so 'read' cannot write past the end of the scanline
                    count = std::min(count, width);

                    p = read(dest, p, end, mask, count);
                    if (!p)
                        return nullptr;
                    dest += count * 4;
                }

                width -= count;
            }

            return p;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecPIC()
    {
        registerImageDecoder(createInterface, ".pic");
        registerImageDecoder(createInterface, ".soft");
    }

} // namespace mango::image

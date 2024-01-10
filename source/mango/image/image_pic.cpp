/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
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

    struct Interface : ImageDecoderInterface
    {
        ImageHeader m_header;
        ConstMemory m_data;

        Interface(ConstMemory memory)
            : m_data(memory.address + PICT_HEADER_SIZE, memory.size - PICT_HEADER_SIZE)
        {
            BigEndianConstPointer p = memory;

            if (memory.size < PICT_HEADER_SIZE)
            {
                m_header.setError("[ImageDecoder.PIC] Not enough data.");
                return;
            }

            constexpr u8 identifier [] =
            {
                0x53, 0x80, 0xf6, 0x34
            };

            if (std::memcmp(p, identifier, sizeof(identifier)))
            {
                m_header.setError("[ImageDecoder.PIC] Incorrect identifier.");
                return;
            }

            p += sizeof(identifier);
            p += 4; // skip storage format
            p += 80; // skip comment

            u32 pict = p.read32();
            if (pict != 0x50494354)
            {
                m_header.setError("[ImageDecoder.PIC] Incorrect PICT.");
                return;
            }

            int width = p.read16();
            int height = p.read16();

            m_header.width   = width;
            m_header.height  = height;
            m_header.depth   = 0;
            m_header.levels  = 0;
            m_header.faces   = 0;
            m_header.palette = false;
            m_header.format  = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            m_header.compression = TextureCompression::NONE;
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            return ConstMemory();
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
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

                debugPrintLine("[packet %d] size: %d  type: %d  mask: %x", i,
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

            int width = m_header.width;
            int height = m_header.height;

            Bitmap temp(width, height, m_header.format);
            temp.clear(0xff000000);

            for (int y = 0; y < height; ++y)
            {
                u8* dest = temp.address<u8>(0, y);

                for (const Packet& packet : packets)
                {
                    switch (packet.type)
                    {
                        case 0:
                            p = decode_uncompressed(dest, p, width, packet.mask);
                            break;

                        case 1:
                            p = decode_runlength(dest, p, width, packet.mask);
                            break;

                        case 2:
                            p = decode_mixed(dest, p, width, packet.mask);
                            break;
                    }
                }
            }

            dest.blit(0, 0, temp);

            return status;
        }

        const u8* read(u8* dest, const u8* p, u32 mask, int count) const
        {
            for (int channel = 0; channel < 4; ++channel)
            {
                if (mask & (1 << channel))
                {
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

        const u8* decode_uncompressed(u8* dest, const u8* p, int width, u32 mask) const
        {
            for (int x = 0; x < width; ++x)
            {
                p = read(dest, p, mask, 1);
                dest += 4;
            }

            return p;
        }

        const u8* decode_runlength(u8* dest, const u8* p, int width, u32 mask) const
        {
            while (width > 0)
            {
                int count = *p++;
                count = std::min(count, width);

                p = read(dest, p, mask, count);
                dest += count * 4;
                width -= count;
            }

            return p;
        }

        const u8* decode_mixed(u8* dest, const u8* p, int width, u32 mask) const
        {
            while (width > 0)
            {
                int count = *p++;

                if (count < 128)
                {
                    ++count;
                    for (int i = 0; i < count; ++i)
                    {
                        p = read(dest, p, mask, 1);
                        dest += 4;
                    }
                }
                else
                {
                    if (count == 128)
                    {
                        count = bigEndian::uload16(p);
                        p += 2;
                    }
                    else
                    {
                        count -= 127;
                    }

                    p = read(dest, p, mask, count);
                    dest += count * 4;
                }

                width -= count;
            }

            return p;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
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

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/compress.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>
#include <zlib.h>

#include <cstring>
#include <map>
#include <vector>

namespace
{
    using namespace mango;
    using namespace mango::image;
    using namespace mango::math;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    // Specification:
    // https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/

    enum class ColorMode
    {
        BITMAP  = 0,
        GRAYSCALE = 1,
        INDEXED = 2,
        RGB = 3,
        CMYK = 4,
        MULTICHANNEL = 7,
        DUOTONE = 8,
        LAB = 9
    };

    enum class Compression
    {
        RAW = 0,
        RLE = 1,
        ZIP = 2,
        ZIP_PRED = 3
    };

    struct ZlibChannelResult
    {
        bool success = false;
        const u8* next = nullptr;
    };

    static
    ZlibChannelResult psd_inflate_channel(const u8* src, const u8* end, u8* dest, size_t dest_size)
    {
        ZlibChannelResult result;

        if (!src || !end || src >= end || !dest || !dest_size)
        {
            return result;
        }

        z_stream stream {};
        stream.next_in = const_cast<Bytef*>(src);
        stream.avail_in = uInt(end - src);
        stream.next_out = dest;
        stream.avail_out = uInt(dest_size);

        if (inflateInit(&stream) != Z_OK)
        {
            return result;
        }

        int state = Z_OK;
        while (stream.avail_out > 0 && state == Z_OK)
        {
            state = inflate(&stream, Z_SYNC_FLUSH);
            if (state == Z_STREAM_END)
            {
                break;
            }

            if (state != Z_OK)
            {
                inflateEnd(&stream);
                return result;
            }
        }

        inflateEnd(&stream);

        if (stream.avail_out != 0)
        {
            return result;
        }

        result.success = true;
        result.next = src + (stream.next_in - const_cast<Bytef*>(src));
        return result;
    }

    static
    void psd_undo_prediction(u8* data, int width, int height, int bits)
    {
        if (!data || width < 1 || height < 1)
        {
            return;
        }

        const size_t bytes_per_scan = div_ceil(width * bits, 8);

        switch (bits)
        {
            case 1:
            case 8:
            {
                for (int y = 0; y < height; ++y)
                {
                    u8* row = data + y * bytes_per_scan;
                    for (size_t x = 1; x < bytes_per_scan; ++x)
                    {
                        row[x] = u8(row[x] + row[x - 1]);
                    }
                }
                break;
            }

            case 16:
            {
                for (int y = 0; y < height; ++y)
                {
                    u8* row = data + y * bytes_per_scan;
                    for (int x = 1; x < width; ++x)
                    {
                        const u16 prev = bigEndian::uload16(row + (x - 1) * 2);
                        const u16 cur = bigEndian::uload16(row + x * 2);
                        bigEndian::ustore16(row + x * 2, u16(prev + cur));
                    }
                }
                break;
            }

            case 32:
            {
                for (int y = 0; y < height; ++y)
                {
                    u8* row = data + y * bytes_per_scan;
                    for (size_t x = 1; x < bytes_per_scan; ++x)
                    {
                        row[x] = u8(row[x] + row[x - 1]);
                    }
                }
                break;
            }
        }
    }

    struct PackBits
    {
        std::vector<u32> offsets;
        std::vector<u32> sizes;

        PackBits(int channels, int height)
            : offsets(channels)
            , sizes(channels * height)
        {
        }

        const u8* parse(const u8* p, int channels, int height, int version)
        {
            int offset = 0;
            for (int channel = 0; channel < channels; ++channel)
            {
                offsets[channel] = offset;

                if (version == 1)
                {
                    for (int y = 0; y < height; ++y)
                    {
                        u32 bytes = bigEndian::uload16(p);
                        p += 2;
                        sizes[channel * height + y] = bytes;
                        offset += bytes;
                    }
                }
                else
                {
                    for (int y = 0; y < height; ++y)
                    {
                        u32 bytes = bigEndian::uload32(p);
                        p += 4;
                        sizes[channel * height + y] = bytes;
                        offset += bytes;
                    }
                }
            }

            return p;
        }
    };

    struct LayerChannelInfo
    {
        int id = 0;
        u32 data_length = 0;
    };

    struct LayerRecord
    {
        int top = 0;
        int left = 0;
        int bottom = 0;
        int right = 0;
        std::vector<LayerChannelInfo> channels;
        u8 opacity = 255;
        u8 flags = 0;

        int width() const { return right - left; }
        int height() const { return bottom - top; }
        bool visible() const { return (flags & 0x02) != 0; }
    };

    static
    void blend_pixel_rgba8(u8* dst, u8 r, u8 g, u8 b, u8 a, u8 opacity)
    {
        const u32 sa = (u32(a) * opacity) / 255;
        if (!sa)
        {
            return;
        }

        const u32 da = dst[3];
        const u32 inv_sa = 255 - sa;
        const u32 out_a = sa + (da * inv_sa) / 255;
        if (!out_a)
        {
            return;
        }

        dst[0] = u8((r * sa + u32(dst[0]) * da * inv_sa / 255) / out_a);
        dst[1] = u8((g * sa + u32(dst[1]) * da * inv_sa / 255) / out_a);
        dst[2] = u8((b * sa + u32(dst[2]) * da * inv_sa / 255) / out_a);
        dst[3] = u8(out_a);
    }

    static
    bool decode_channel_plane(Compression compression, int version, int width, int height, int bits,
        const u8*& data, const u8* end, u8* dest, size_t dest_size, ImageDecodeStatus& status)
    {
        const size_t bytes_per_scan = div_ceil(width * bits, 8);
        const size_t plane_size = size_t(height) * bytes_per_scan;

        if (dest_size < plane_size)
        {
            status.setError("[ImageDecoder.PSD] Channel plane buffer too small.");
            return false;
        }

        switch (compression)
        {
            case Compression::RAW:
            {
                if (size_t(end - data) < plane_size)
                {
                    status.setError("[ImageDecoder.PSD] Truncated raw channel data.");
                    return false;
                }

                std::memcpy(dest, data, plane_size);
                data += plane_size;
                break;
            }

            case Compression::RLE:
            {
                const size_t table_bytes = size_t(height) * (version == 1 ? 2 : 4);
                if (size_t(end - data) < table_bytes)
                {
                    status.setError("[ImageDecoder.PSD] Truncated RLE length table.");
                    return false;
                }

                PackBits packbits(1, height);
                const u8* rle_data = packbits.parse(data, 1, height, version);

                for (int y = 0; y < height; ++y)
                {
                    const u32 bytes = packbits.sizes[y];
                    const u8* src = rle_data + packbits.offsets[0];
                    packbits.offsets[0] += bytes;

                    if (src < rle_data || src + bytes > end)
                    {
                        status.setError("[ImageDecoder.PSD] Truncated RLE channel data.");
                        return false;
                    }

                    Memory output(dest + y * bytes_per_scan, bytes_per_scan);
                    ConstMemory input(src, bytes);

                    if (!packbits_decompress(output, input))
                    {
                        status.setError("[ImageDecoder.PSD] packbits decompression failed.");
                        return false;
                    }
                }

                data = rle_data + packbits.offsets[0];
                break;
            }

            case Compression::ZIP:
            case Compression::ZIP_PRED:
            {
                const bool use_prediction = compression == Compression::ZIP_PRED ||
                    (compression == Compression::ZIP && bits == 32);

                ZlibChannelResult inflated = psd_inflate_channel(data, end, dest, plane_size);
                if (!inflated.success)
                {
                    status.setError("[ImageDecoder.PSD] ZIP channel decompression failed.");
                    return false;
                }

                data = inflated.next;

                if (use_prediction)
                {
                    psd_undo_prediction(dest, width, height, bits);
                }

                break;
            }

            default:
                status.setError("[ImageDecoder.PSD] Unsupported channel compression ({}).", int(compression));
                return false;
        }

#ifdef MANGO_LITTLE_ENDIAN
        byteswap(Memory(dest, plane_size), bits);
#endif

        return true;
    }

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        ConstMemory m_layers;

        const u8* m_palette = nullptr;

        int m_version;
        ColorMode m_color_mode;
        Compression m_compression;
        int m_channels;
        int m_bits;

        ConstMemory m_icc_profile;

        Interface(ConstMemory memory)
        {
            const u8* const end = memory.end();

            // The fixed header is 26 bytes through the color mode, plus 4 for the color
            // mode data length; reject anything shorter before reading it.
            if (memory.size < 30)
            {
                header.setError("[ImageDecoder.PSD] Not enough data.");
                return;
            }

            BigEndianConstPointer p = memory;

            u32 magic = p.read32();
            if (magic != 0x38425053)
            {
                header.setError("[ImageDecoder.PSD] Incorrect identifier.");
                return;
            }

            m_version = p.read16();
            if (m_version > 2)
            {
                header.setError("[ImageDecoder.PSD] Incorrect version ({}).", m_version);
                return;
            }

            int width_max = 30000;
            int height_max = 30000;

            if (m_version == 2)
            {
                width_max = 300000;
                height_max = 300000;
            }

            p += 6;

            m_channels = p.read16();
            if (m_channels < 1 || m_channels > 56)
            {
                header.setError("[ImageDecoder.PSD] Incorrect number of channels ({}).", m_channels);
                return;
            }

            int height = p.read32();
            int width = p.read32();
            if (width < 1 || width > width_max || height < 1 || height > height_max)
            {
                header.setError("[ImageDecoder.PSD] Incorrect image dimensions ({} x {}).", width, height);
                return;
            }

            Format format;

            m_bits = p.read16();
            switch (m_bits)
            {
                case 1:
                case 8:
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    break;
                case 16:
                    format = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
                    break;
                case 32:
                    format = Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32);
                    break;
                default:
                    header.setError("[ImageDecoder.PSD] Incorrect number of bits ({}).", m_bits);
                    return;
            }

            m_color_mode = ColorMode(p.read16());
            switch (m_color_mode)
            {
                case ColorMode::BITMAP:
                case ColorMode::GRAYSCALE:
                case ColorMode::INDEXED:
                case ColorMode::RGB:
                case ColorMode::CMYK:
                case ColorMode::MULTICHANNEL:
                case ColorMode::DUOTONE:
                case ColorMode::LAB:
                    break;
                default:
                    header.setError("[ImageDecoder.PSD] Incorrect color mode ({}).", int(m_color_mode));
                    return;
            }

            // The resolve*() paths read a fixed number of channel planes out of the decode
            // buffer (RGB/LAB read 3, CMYK reads 4). Make sure the file actually carries
            // enough channels so those reads stay in bounds.
            {
                int required = 1;
                if (m_color_mode == ColorMode::RGB || m_color_mode == ColorMode::LAB)
                    required = 3;
                else if (m_color_mode == ColorMode::CMYK)
                    required = 4;

                if (m_channels < required)
                {
                    header.setError("[ImageDecoder.PSD] Color mode {} requires at least {} channels (got {}).",
                        int(m_color_mode), required, m_channels);
                    return;
                }
            }

            // ColorMode data
            u32 colormode_size = p.read32();

            if (m_color_mode == ColorMode::INDEXED)
            {
                if (colormode_size == 768)
                {
                    m_palette = p;
                }
                else
                {
                    header.setError("[ImageDecoder.PSD] Incorrect palette size ({}).", colormode_size);
                    return;
                }
            }

            // every variable-length section length is taken from the file; validate that it
            // stays within the buffer before skipping over it
            if (colormode_size > u32(end - p))
            {
                header.setError("[ImageDecoder.PSD] Truncated color mode data.");
                return;
            }
            p += colormode_size;

            // ImageResource data
            if (p + 4 > end)
            {
                header.setError("[ImageDecoder.PSD] Out of data.");
                return;
            }
            u32 image_resource_size = p.read32();
            if (image_resource_size > u32(end - p))
            {
                header.setError("[ImageDecoder.PSD] Truncated image resources.");
                return;
            }
            ConstMemory image_resource_data(p, image_resource_size);
            p += image_resource_size;

            // Layer and Mask data
            if (p + 4 > end)
            {
                header.setError("[ImageDecoder.PSD] Out of data.");
                return;
            }
            u32 layer_size = p.read32();
            if (layer_size > u32(end - p))
            {
                header.setError("[ImageDecoder.PSD] Truncated layer data.");
                return;
            }
            ConstMemory layer_data(p, layer_size);
            m_layers = layer_data;
            p += layer_size;

            if (p + 2 > end)
            {
                header.setError("[ImageDecoder.PSD] Out of data.");
                return;
            }
            m_compression = Compression(p.read16());
            switch (m_compression)
            {
                case Compression::RAW:
                case Compression::RLE:
                case Compression::ZIP:
                case Compression::ZIP_PRED:
                    break;
                default:
                    header.setError("[ImageDecoder.PSD] Incorrect compression ({}).", int(m_compression));
                    return;
            }

            if (p >= end)
            {
                header.setError("[ImageDecoder.PSD] Out of data.");
                return;
            }

            m_memory = ConstMemory(p, end - p);

            printLine(Print::Debug, "[psd]\n");
            printLine(Print::Debug, "  Version:     {}", m_version);
            printLine(Print::Debug, "  Image:       {} x {}", width, height);
            printLine(Print::Debug, "  Channels:    {} ({} bits)", m_channels, m_bits);
            printLine(Print::Debug, "  ColorMode:   {}", int(m_color_mode));
            printLine(Print::Debug, "  Compression: {}", int(m_compression));

            parse_resources(image_resource_data);

            header.width   = width;
            header.height  = height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
            header.format  = format;
            header.compression = TextureCompression::NONE;

            icc = m_icc_profile;
        }

        ~Interface()
        {
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(face);
            MANGO_UNREFERENCED(depth);
            return ConstMemory();
        }

        std::string parse_string(BigEndianConstPointer& p, const u8* end, int alignment)
        {
            std::string str;

            if (p >= end)
            {
                return str;
            }

            u8 length = *p++;
            if (length && p + length <= end)
            {
                const char* s = p.cast<const char>();
                str = std::string(s, s + length);
            }

            p += length;
            p += (0 - (length + 1)) & (alignment - 1);

            return str;
        }

        void parse_resources(ConstMemory resources)
        {
            BigEndianConstPointer p = resources.address;
            const u8* end = resources.end();

            printLine(Print::Debug, "  [ImageResourceBlocks]");

            // Each block is: 4-byte signature, 2-byte id, padded pascal string, 4-byte
            // length, then 'length' bytes. Every read is bounded against the block end.
            while (p + 10 <= end)
            {
                u32 signature = p.read32();
                if (signature != 0x3842494d)
                {
                    return;
                }

                u16 id = p.read16();
                std::string c_name = parse_string(p, end, 2);

                if (p + 4 > end)
                {
                    return;
                }
                u32 length = p.read32();

                // clamp the payload to what remains in the resource block
                if (length > u32(end - p))
                {
                    length = u32(end - p);
                }

                bool supported = true;

                if (id == 1005)
                {
                    printLine(Print::Debug, "    Resolution Info");
                }
                else if (id == 1039)
                {
                    printLine(Print::Debug, "    ICC Profile");
                    m_icc_profile = ConstMemory(p, length);
                }
                else if (id == 1047)
                {
                    printLine(Print::Debug, "    TransparencyIndex");
                    // MANGO TODO: need psd file that uses this feature
                }
                else if (id == 1058 || id == 1059)
                {
                    printLine(Print::Debug, "    EXIF");
                }
                else if (id == 1060)
                {
                    printLine(Print::Debug, "    XMP");
                }
                else
                {
                    supported = false;
                }

                p += length;

                if (supported)
                {
                    printLine(Print::Debug, "      {} bytes", length);
                }
            }
        }

        bool parse_layer_records(std::vector<LayerRecord>& layers, const u8*& channel_data) const
        {
            layers.clear();
            channel_data = nullptr;

            if (m_layers.size < 6)
            {
                return false;
            }

            const u8* base = m_layers.address;
            const u8* const section_end = m_layers.end();

            u32 info_length = bigEndian::uload32(base);
            if (info_length < 2 || base + 4 + info_length > section_end)
            {
                return false;
            }

            const u8* p = base + 4;
            const u8* const info_end = base + 4 + info_length;
            const s16 layer_count = s16(bigEndian::uload16(p)); p += 2;
            if (!layer_count)
            {
                return false;
            }

            const int layer_total = std::abs(int(layer_count));

            for (int i = 0; i < layer_total; ++i)
            {
                if (p + 18 > info_end)
                {
                    return false;
                }

                LayerRecord layer;
                layer.top = bigEndian::uload32(p + 0); p += 4;
                layer.left = bigEndian::uload32(p + 0); p += 4;
                layer.bottom = bigEndian::uload32(p + 0); p += 4;
                layer.right = bigEndian::uload32(p + 0); p += 4;

                const int width = layer.width();
                const int height = layer.height();

                const u16 channel_count = bigEndian::uload16(p); p += 2;
                if (!channel_count || channel_count > 56)
                {
                    return false;
                }

                layer.channels.resize(channel_count);
                for (u16 c = 0; c < channel_count; ++c)
                {
                    if (p + 6 > info_end)
                    {
                        return false;
                    }

                    layer.channels[c].id = s16(bigEndian::uload16(p)); p += 2;
                    layer.channels[c].data_length = bigEndian::uload32(p); p += 4;
                }

                if (p + 16 > info_end)
                {
                    return false;
                }

                p += 8; // blend mode signature + key
                layer.opacity = p[0]; p += 1;
                p += 1; // clipping
                layer.flags = p[0]; p += 1;
                p += 1; // filler

                const u32 extra_length = bigEndian::uload32(p); p += 4;
                if (extra_length > u32(info_end - p))
                {
                    return false;
                }

                p += extra_length;
                layers.push_back(layer);
            }

            // Channel image data follows the entire layer info block (records, global
            // layer mask, and any tagged blocks), not immediately after the records.
            channel_data = info_end;
            return !layers.empty();
        }

        bool decode_layers(Bitmap& dest, ImageDecodeStatus& status)
        {
            if (m_color_mode != ColorMode::RGB && m_color_mode != ColorMode::GRAYSCALE)
            {
                return false;
            }

            if (m_bits != 8)
            {
                return false;
            }

            std::vector<LayerRecord> layers;
            const u8* channel_data = nullptr;
            if (!parse_layer_records(layers, channel_data))
            {
                return false;
            }

            const u8* p = channel_data;
            const u8* const end = m_layers.end();

            std::memset(dest.image, 0, dest.stride * dest.height);

            // Layer records are stored top-to-front; composite bottom-up.
            for (int i = int(layers.size()) - 1; i >= 0; --i)
            {
                const LayerRecord& layer = layers[i];
                const int width = layer.width();
                const int height = layer.height();
                const size_t plane_size = size_t(width) * size_t(height);

                std::map<int, std::vector<u8>> planes;

                for (const LayerChannelInfo& channel : layer.channels)
                {
                    if (p + channel.data_length > end)
                    {
                        status.setError("[ImageDecoder.PSD] Truncated layer channel data.");
                        return false;
                    }

                    const u8* const channel_start = p;

                    if (width <= 0 || height <= 0)
                    {
                        p = channel_start + channel.data_length;
                        continue;
                    }

                    if (channel.data_length < 2)
                    {
                        status.setError("[ImageDecoder.PSD] Invalid layer channel length ({}).", channel.data_length);
                        return false;
                    }

                    const u16 compression_code = bigEndian::uload16(p);
                    const Compression compression = Compression(compression_code);
                    p += 2;

                    std::vector<u8> plane(plane_size);
                    if (!decode_channel_plane(compression, m_version, width, height, m_bits,
                        p, channel_start + channel.data_length, plane.data(), plane.size(), status))
                    {
                        return false;
                    }

                    p = channel_start + channel.data_length;
                    planes[channel.id] = std::move(plane);
                }

                const std::vector<u8>* r = planes.count(0) ? &planes[0] : nullptr;
                const std::vector<u8>* g = planes.count(1) ? &planes[1] : nullptr;
                const std::vector<u8>* b = planes.count(2) ? &planes[2] : nullptr;
                const std::vector<u8>* a = planes.count(-1) ? &planes[-1] : (planes.count(3) ? &planes[3] : nullptr);

                if (width <= 0 || height <= 0)
                {
                    continue;
                }

                for (int y = 0; y < height; ++y)
                {
                    const int doc_y = layer.top + y;
                    if (doc_y < 0 || doc_y >= dest.height)
                    {
                        continue;
                    }

                    u8* scan = dest.image + doc_y * dest.stride;

                    for (int x = 0; x < width; ++x)
                    {
                        const int doc_x = layer.left + x;
                        if (doc_x < 0 || doc_x >= dest.width)
                        {
                            continue;
                        }

                        const int i = y * width + x;
                        const u8 rv = r ? (*r)[i] : 0;
                        const u8 gv = g ? (*g)[i] : rv;
                        const u8 bv = b ? (*b)[i] : rv;
                        const u8 av = a ? (*a)[i] : 255;

                        blend_pixel_rgba8(scan + doc_x * 4, rv, gv, bv, av, layer.opacity);
                    }
                }
            }

            printLine(Print::Debug, "  Composited {} layer(s).", layers.size());
            return true;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(face);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(options);

            ImageDecodeStatus status;

            const u8* p = m_memory.address;
            const u8* const data_end = m_memory.end();

            int width = header.width;
            int height = header.height;
            int channels = std::min(4, m_channels);

            // size math in size_t: PSB dimensions can be large enough to overflow int
            const size_t bytes_per_scan = div_ceil(width * m_bits, 8);
            const size_t bytes_per_channel = size_t(height) * bytes_per_scan;

            //printLine(Print::Debug, "  available: {} bytes", m_memory.size);
            //printLine(Print::Debug, "  request:   {} bytes", channels * bytes_per_channel);

            Bitmap temp(width, height, header.format);
            Buffer buffer(channels * bytes_per_scan);

            if (decode_layers(temp, status))
            {
                dest.blit(0, 0, temp);
                return status;
            }

            // Layer compositing is preferred when present, but fall back to the merged
            // composite when parsing or decoding individual layers fails.
            status = ImageDecodeStatus();

            switch (m_compression)
            {
                case Compression::RAW:
                {
                    // RAW stores m_channels contiguous planes; we read the first 'channels'.
                    const size_t needed = size_t(channels) * bytes_per_channel;
                    if (m_memory.size < needed)
                    {
                        status.setError("[ImageDecoder.PSD] Truncated raw image data.");
                        return status;
                    }

                    for (int y = 0; y < height; ++y)
                    {
                        for (int channel = 0; channel < channels; ++channel)
                        {
                            const u8* src = p + channel * bytes_per_channel + y * bytes_per_scan;
                            u8* dst = buffer + channel * bytes_per_scan;
                            std::memcpy(dst, src, bytes_per_scan);
                        }

#ifdef MANGO_LITTLE_ENDIAN
                        byteswap(buffer, m_bits);
#endif
                        resolve(temp.image + y * temp.stride, buffer, width, channels);
                    }

                    break;
                }

                case Compression::RLE:
                {
                    // The per-scanline length table precedes the compressed data; make sure
                    // it is fully present before PackBits::parse reads it.
                    const size_t table_bytes = size_t(m_channels) * size_t(height) * (m_version == 1 ? 2 : 4);
                    if (size_t(data_end - p) < table_bytes)
                    {
                        status.setError("[ImageDecoder.PSD] Truncated RLE length table.");
                        return status;
                    }

                    PackBits packbits(m_channels, height);
                    p = packbits.parse(p, m_channels, height, m_version);

                    for (int y = 0; y < height; ++y)
                    {
                        for (int channel = 0; channel < channels; ++channel)
                        {
                            u32 bytes = packbits.sizes[channel * height + y];
                            const u8* src = p + packbits.offsets[channel];
                            packbits.offsets[channel] += bytes;

                            // the compressed scanline must lie within the buffer
                            if (src < p || src + bytes > data_end)
                            {
                                status.setError("[ImageDecoder.PSD] Truncated RLE image data.");
                                return status;
                            }

                            u8* dst = buffer + channel * bytes_per_scan;

                            Memory output(dst, bytes_per_scan);
                            ConstMemory input(src, bytes);

                            bool result = packbits_decompress(output, input);
                            if (!result)
                            {
                                status.setError("[ImageDecoder.PSD] packbits decompression failed.");
                                return status;
                            }
                        }

#ifdef MANGO_LITTLE_ENDIAN
                        byteswap(buffer, m_bits);
#endif
                        resolve(temp.image + y * temp.stride, buffer, width, channels);
                    }

                    break;
                }

                case Compression::ZIP:
                case Compression::ZIP_PRED:
                {
                    // Each channel is an independent zlib stream. Photoshop stores 32-bit
                    // files as ZIP with prediction even when the header says ZIP only.
                    const bool use_prediction = m_compression == Compression::ZIP_PRED ||
                        (m_compression == Compression::ZIP && m_bits == 32);

                    Buffer planes(size_t(m_channels) * bytes_per_channel);
                    const u8* zip_end = data_end;

                    for (int channel = 0; channel < m_channels; ++channel)
                    {
                        if (p >= zip_end)
                        {
                            status.setError("[ImageDecoder.PSD] Truncated ZIP image data.");
                            return status;
                        }

                        u8* dest = planes + size_t(channel) * bytes_per_channel;
                        ZlibChannelResult inflated = psd_inflate_channel(p, zip_end, dest, bytes_per_channel);
                        if (!inflated.success)
                        {
                            status.setError("[ImageDecoder.PSD] ZIP decompression failed.");
                            return status;
                        }

                        p = inflated.next;

                        if (use_prediction)
                        {
                            psd_undo_prediction(dest, width, height, m_bits);
                        }
                    }

                    for (int y = 0; y < height; ++y)
                    {
                        for (int channel = 0; channel < channels; ++channel)
                        {
                            const u8* src = planes + size_t(channel) * bytes_per_channel + y * bytes_per_scan;
                            u8* dst = buffer + channel * bytes_per_scan;
                            std::memcpy(dst, src, bytes_per_scan);
                        }

#ifdef MANGO_LITTLE_ENDIAN
                        byteswap(buffer, m_bits);
#endif
                        resolve(temp.image + y * temp.stride, buffer, width, channels);
                    }

                    break;
                }
            }

            dest.blit(0, 0, temp);

            return status;
        }

        void resolve(u8* dest, const u8* src, int width, int channels)
        {
            switch (m_color_mode)
            {
                case ColorMode::BITMAP:
                    resolveBitmap(dest, src, width, channels);
                    break;
                case ColorMode::GRAYSCALE:
                    resolveGrayscale(dest, src, width, channels);
                    break;
                case ColorMode::INDEXED:
                    resolveIndexed(dest, src, width, channels);
                    break;
                case ColorMode::RGB:
                    resolveRGB(dest, src, width, channels);
                    break;
                case ColorMode::CMYK:
                    resolveCMYK(dest, src, width, channels);
                    break;
                case ColorMode::MULTICHANNEL:
                    resolveGrayscale(dest, src, width, channels);
                    break;
                case ColorMode::DUOTONE:
                    resolveGrayscale(dest, src, width, channels);
                    break;
                case ColorMode::LAB:
                    resolveLab(dest, src, width, channels);
                    break;
            }
        }

        void resolveBitmap(u8* dest, const u8* src, int width, int channels)
        {
            MANGO_UNREFERENCED(channels);

            u8 mask = 0;
            u8 data = 0;

            for (int x = 0; x < width; ++x)
            {
                if (!mask)
                {
                    mask = 0x80;
                    data = *src++;
                }

                u8 value = 0xff + !!(data & mask);
                mask >>= 1;

                dest[0] = value;
                dest[1] = value;
                dest[2] = value;
                dest[3] = 0xff;
                dest += 4;
            }
        }

        void resolveGrayscale(u8* dest, const u8* src, int width, int channels)
        {
            MANGO_UNREFERENCED(channels);

            switch (m_bits)
            {
                case 8:
                    pack_grayscale<u8>(dest, src, width, 0xff);
                    break;
                case 16:
                    pack_grayscale<u16>(dest, src, width, 0xffff);
                    break;
                case 32:
                    pack_grayscale<float>(dest, src, width, 1.0f);
                    break;
            }
        }

        void resolveIndexed(u8* dest, const u8* src, int width, int channels)
        {
            MANGO_UNREFERENCED(channels);

            if (m_bits == 8 && m_palette)
            {
                for (int i = 0; i < width; ++i)
                {
                    u8 index = *src++;
                    dest[0] = m_palette[index + 256 * 0];
                    dest[1] = m_palette[index + 256 * 1];
                    dest[2] = m_palette[index + 256 * 2];
                    dest[3] = 0xff;
                    dest += 4;
                }
            }
        }

        void resolveRGB(u8* dest, const u8* src, int width, int channels)
        {
            switch (m_bits)
            {
                case 8:
                    if (channels == 4)
                        pack_rgba<u8>(dest, src, width);
                    else
                        pack_rgb<u8>(dest, src, width, 0xff);
                    break;
                case 16:
                    if (channels == 4)
                        pack_rgba<u16>(dest, src, width);
                    else
                        pack_rgb<u16>(dest, src, width, 0xffff);
                    break;
                case 32:
                    if (channels == 4)
                        pack_rgba<float>(dest, src, width);
                    else
                        pack_rgb<float>(dest, src, width, 1.0f);
                    break;
            }
        }

        void resolveCMYK(u8* dest, const u8* src, int width, int channels)
        {
            MANGO_UNREFERENCED(channels);

            switch (m_bits)
            {
                case 8:
                {
                    for (int i = 0; i < width; ++i)
                    {
                        int c = src[width * 0];
                        int m = src[width * 1];
                        int y = src[width * 2];
                        int k = src[width * 3];
                        dest[0] = (c * k) / 255;
                        dest[1] = (m * k) / 255;
                        dest[2] = (y * k) / 255;
                        dest[3] = 0xff;
                        ++src;
                        dest += 4;
                    }
                    break;
                }

                case 16:
                {
                    u16* d = reinterpret_cast<u16*>(dest);
                    const u16* s = reinterpret_cast<const u16*>(src);

                    for (int i = 0; i < width; ++i)
                    {
                        u32 c = s[width * 0];
                        u32 m = s[width * 1];
                        u32 y = s[width * 2];
                        u32 k = s[width * 3];
                        d[0] = u16((c * k) / 65535);
                        d[1] = u16((m * k) / 65535);
                        d[2] = u16((y * k) / 65535);
                        d[3] = 0xffff;
                        ++s;
                        d += 4;
                    }
                    break;
                }
            }
        }

        void resolveLab(u8* dest, const u8* src, int width, int channels)
        {
            MANGO_UNREFERENCED(channels);

            if (m_bits == 8)
            {
                for (int i = 0; i < width; ++i)
                {
                    float L = src[width * 0 + i];
                    float A = src[width * 1 + i];
                    float B = src[width * 2 + i];

                    L = (L * 100.0f) / 255.0f;
                    A = A - 128;
                    B = B - 128;

                    uint32x4 color = convert<uint32x4>(lab_to_rgb(L, A, B) * 255.0f);
                    ustore32(dest + i * 4, color.pack());
                }
            }
            else if (m_bits == 16)
            {
                const u16* s = reinterpret_cast<const u16*>(src);
                u16* d = reinterpret_cast<u16*>(dest);

                for (int i = 0; i < width; ++i)
                {
                    float L = s[width * 0 + i];
                    float A = s[width * 1 + i];
                    float B = s[width * 2 + i];

                    L = (L * 100.0f) / 65535.0f;
                    A = (A - 32768) / 257.0f;
                    B = (B - 32768) / 257.0f;

                    uint32x4 color = convert<uint32x4>(lab_to_rgb(L, A, B) * 65535.0f);
                    store_low(d + i * 4, simd::narrow(color, color));
                }
            }
        }

        float32x4 lab_to_rgb(float L, float A, float B)
        {
            float y = (L + 16.0f) / 116.0f;
            float x = A / 500.0f + y;
            float z = y - B / 200.0f;

            x = 0.95047f * ((x * x * x > 0.008856f) ? x * x * x : (x - 16/116.0f) / 7.787f);
            y = 1.00000f * ((y * y * y > 0.008856f) ? y * y * y : (y - 16/116.0f) / 7.787f);
            z = 1.08883f * ((z * z * z > 0.008856f) ? z * z * z : (z - 16/116.0f) / 7.787f);

            float r = x *  3.2406f + y * -1.5372f + z * -0.4986f;
            float g = x * -0.9689f + y *  1.8758f + z *  0.0415f;
            float b = x *  0.0557f + y * -0.2040f + z *  1.0570f;
            float a = 1.0f;

            float32x4 color(r, g, b, a);
            color = linear_to_srgb(color);

            return color;
        }

        template <typename T>
        void pack_grayscale(u8* image, const u8* buffer, int width, T alpha) const
        {
            const T* src = reinterpret_cast<const T*>(buffer);
            T* dest = reinterpret_cast<T*>(image);

            for (int i = 0; i < width; ++i)
            {
                T s = *src++;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = alpha;
                dest += 4;
            }
        }

        template <typename T>
        void pack_rgb(u8* image, const u8* buffer, int width, T alpha) const
        {
            const T* src = reinterpret_cast<const T*>(buffer);
            T* dest = reinterpret_cast<T*>(image);

            for (int i = 0; i < width; ++i)
            {
                dest[0] = src[width * 0];
                dest[1] = src[width * 1];
                dest[2] = src[width * 2];
                dest[3] = alpha;
                ++src;
                dest += 4;
            }
        }

        template <typename T>
        void pack_rgba(u8* image, const u8* buffer, int width) const
        {
            const T* src = reinterpret_cast<const T*>(buffer);
            T* dest = reinterpret_cast<T*>(image);

            for (int i = 0; i < width; ++i)
            {
                dest[0] = src[width * 0];
                dest[1] = src[width * 1];
                dest[2] = src[width * 2];
                dest[3] = src[width * 3];
                ++src;
                dest += 4;
            }
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

    void registerImageCodecPSD()
    {
        registerImageDecoder(createInterface, ".psd");
        registerImageDecoder(createInterface, ".psb");
    }

} // namespace mango::image

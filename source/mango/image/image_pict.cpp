/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

// Apple QuickDraw PICT (.pict / .pct). This is not Softimage PIC (.pic).

namespace
{
    using namespace mango;
    using namespace mango::image;

    static constexpr size_t MAC_FILE_HEADER_SIZE = 512;

    // -2: u16 length prefix; -3: PixPat; -4: bitmap opcode
    static constexpr s8 PICT_LEN_SPECIAL = -2;
    static constexpr s8 PICT_LEN_PIXPAT = -3;
    static constexpr s8 PICT_LEN_BITMAP = -4;
    static constexpr s8 PICT_LEN_LENGTH_WORD = -1;

    static
    const s8 g_pict_opcode_length [0xa2] =
    {
        /* 0x00 */ 0,  PICT_LEN_SPECIAL, 8, 2, 1, 2, 4, 4, 2, 8,
        /* 0x0a */ 8, 4, 4, 2, 4, 4, 8, 1, PICT_LEN_PIXPAT, PICT_LEN_PIXPAT,
        /* 0x14 */ PICT_LEN_PIXPAT, 2, 2, 0, 0, 0, 6, 6, 0, 6,
        /* 0x1e */ 0, 6, 8, 4, 6, 2, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD,
        /* 0x28 */ 0, 0, 0, 0, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD, 8, 8,
        /* 0x32 */ 8, 8, 8, 8, 8, 8, 0, 0, 0, 0,
        /* 0x3c */ 0, 0, 0, 0, 8, 8, 8, 8, 8, 8,
        /* 0x46 */ 8, 8, 0, 0, 0, 0, 0, 0, 8, 8,
        /* 0x50 */ 8, 8, 8, 8, 8, 8, 8, 0, 0, 0,
        /* 0x5c */ 0, 0, 0, 0, 0, 12, 12, 12, 12, 12,
        /* 0x66 */ 12, 12, 12, 4, 4, 4, 4, 4, 4, 4,
        /* 0x70 */ PICT_LEN_SPECIAL, PICT_LEN_SPECIAL, PICT_LEN_SPECIAL, PICT_LEN_SPECIAL, PICT_LEN_SPECIAL, 0, 0, 0, 0, 0,
        /* 0x7a */ 0, 0, 0, 0, 0, 0, PICT_LEN_SPECIAL, PICT_LEN_SPECIAL, PICT_LEN_SPECIAL, PICT_LEN_SPECIAL,
        /* 0x84 */ PICT_LEN_SPECIAL, 0, 0, 0, 0, 0, 0, 0, 0, PICT_LEN_BITMAP,
        /* 0x8e */ 0, 0, PICT_LEN_BITMAP, PICT_LEN_BITMAP, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD, PICT_LEN_BITMAP, PICT_LEN_BITMAP,
        /* 0x98 */ PICT_LEN_BITMAP, PICT_LEN_BITMAP, PICT_LEN_BITMAP, PICT_LEN_BITMAP, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD, PICT_LEN_LENGTH_WORD,
        /* 0xa2 */ 0,
    };

    struct EmbeddedSignature
    {
        const char* extension;
        const u8* signature;
        size_t signature_size;
    };

    static
    const EmbeddedSignature g_embedded_signatures [] =
    {
        { ".png",  reinterpret_cast<const u8*>("\x89PNG\r\n\x1a\n"), 8 },
        { ".jpg",  reinterpret_cast<const u8*>("\xff\xd8\xff"), 3 },
        { ".bmp",  reinterpret_cast<const u8*>("BM"), 2 },
        { ".tiff", reinterpret_cast<const u8*>("MM\x00\x2a"), 4 },
        { ".tiff", reinterpret_cast<const u8*>("II\x2a\x00"), 4 },
    };

    struct PictRect
    {
        s16 top;
        s16 left;
        s16 bottom;
        s16 right;

        int width() const { return int(right - left); }
        int height() const { return int(bottom - top); }
        bool valid() const { return width() > 0 && height() > 0; }
    };

    struct PictPixmap
    {
        u16 version = 0;
        u16 pack_type = 0;
        u32 pack_size = 0;
        u16 horizontal_resolution = 72;
        u16 vertical_resolution = 72;
        u16 pixel_type = 0;
        u16 bits_per_pixel = 0;
        u16 component_count = 0;
        u16 component_size = 0;
        u32 plane_bytes = 0;
        u32 table = 0;
        u32 reserved = 0;
    };

    static
    bool isMacBinaryPadding(ConstMemory memory)
    {
        if (memory.size < MAC_FILE_HEADER_SIZE + 12)
        {
            return false;
        }

        for (size_t i = 0; i < 32; ++i)
        {
            if (memory.address[i])
            {
                return false;
            }
        }

        return true;
    }

    static
    bool isApplePictAt(ConstMemory memory, size_t offset)
    {
        if (memory.size < offset + 12)
        {
            return false;
        }

        BigEndianConstPointer p = memory.address + offset;

        p += 2; // picSize
        p += 8; // picFrame
        u16 version_op = p.read16();

        // Extended version 2 ($0011) or version 1 ($1101 / $11 $01).
        return version_op == 0x0011 || version_op == 0x1101;
    }

    static
    size_t applePictOffset(ConstMemory memory)
    {
        if (isMacBinaryPadding(memory) && isApplePictAt(memory, MAC_FILE_HEADER_SIZE))
        {
            return MAC_FILE_HEADER_SIZE;
        }

        if (isApplePictAt(memory, 0))
        {
            return 0;
        }

        return ~size_t(0);
    }

    static
    bool isValidBmp(ConstMemory memory, size_t offset)
    {
        if (memory.size < offset + 26)
        {
            return false;
        }

        u32 file_size = littleEndian::uload32(memory.address + offset + 2);
        if (file_size < 26 || offset + file_size > memory.size)
        {
            return false;
        }

        u32 header_size = littleEndian::uload32(memory.address + offset + 14);
        if (header_size < 40 || header_size > 255)
        {
            return false;
        }

        int width = littleEndian::uload32(memory.address + offset + 18);
        int height = littleEndian::uload32(memory.address + offset + 22);
        if (width < 1 || height < 1 || width > 65535 || height > 65535)
        {
            return false;
        }

        return true;
    }

    struct EmbeddedCandidate
    {
        size_t offset;
        std::string extension;
        int area;
    };

    static
    void tryEmbeddedCandidate(ConstMemory memory, size_t offset, const char* extension,
                              std::vector<EmbeddedCandidate>& candidates)
    {
        if (!std::strcmp(extension, ".bmp") && !isValidBmp(memory, offset))
        {
            return;
        }

        ConstMemory payload(memory.address + offset, memory.size - offset);
        ImageDecoder decoder(payload, std::string("embedded") + extension);
        if (!decoder.isDecoder())
        {
            return;
        }

        ImageHeader embedded = decoder.header();
        if (embedded.width < 1 || embedded.height < 1)
        {
            return;
        }

        candidates.push_back({ offset, extension, embedded.width * embedded.height });
    }

    static
    ConstMemory findEmbeddedPayload(ConstMemory memory, size_t pict_offset, std::string& extension)
    {
        extension.clear();

        size_t search_start = pict_offset + 12;

        std::vector<EmbeddedCandidate> candidates;

        for (const EmbeddedSignature& sig : g_embedded_signatures)
        {
            for (size_t offset = search_start; offset + sig.signature_size <= memory.size; ++offset)
            {
                if (!std::memcmp(memory.address + offset, sig.signature, sig.signature_size))
                {
                    tryEmbeddedCandidate(memory, offset, sig.extension, candidates);
                }
            }
        }

        if (candidates.empty())
        {
            return ConstMemory();
        }

        auto best = std::max_element(candidates.begin(), candidates.end(),
            [] (const EmbeddedCandidate& a, const EmbeddedCandidate& b)
            {
                if (a.area != b.area)
                {
                    return a.area < b.area;
                }

                return a.offset > b.offset;
            });

        extension = best->extension;
        return ConstMemory(memory.address + best->offset, memory.size - best->offset);
    }

    static
    u8 scalePictComponent(u16 value)
    {
        return u8((u32(value) * 255 + 32767) / 65535);
    }

    static
    Color readPictColor(BigEndianConstPointer& p)
    {
        u16 r = p.read16();
        u16 g = p.read16();
        u16 b = p.read16();
        return Color(scalePictComponent(r), scalePictComponent(g), scalePictComponent(b), 255);
    }

    static
    bool readPictRect(BigEndianConstPointer& p, ConstMemory memory, PictRect& rect)
    {
        if (p + 8 > memory.end())
        {
            return false;
        }

        rect.top = p.read16();
        rect.left = p.read16();
        rect.bottom = p.read16();
        rect.right = p.read16();

        return rect.valid();
    }

    static
    bool readPixmap(BigEndianConstPointer& p, ConstMemory memory, PictPixmap& pixmap)
    {
        if (p + 38 > memory.end())
        {
            return false;
        }

        pixmap.version = p.read16();
        pixmap.pack_type = p.read16();
        pixmap.pack_size = p.read32();
        pixmap.horizontal_resolution = p.read16();
        p += 2;
        pixmap.vertical_resolution = p.read16();
        p += 2;
        pixmap.pixel_type = p.read16();
        pixmap.bits_per_pixel = p.read16();
        pixmap.component_count = p.read16();
        pixmap.component_size = p.read16();
        pixmap.plane_bytes = p.read32();
        pixmap.table = p.read32();
        pixmap.reserved = p.read32();

        if (pixmap.bits_per_pixel == 0 || pixmap.bits_per_pixel > 32)
        {
            return false;
        }

        if (pixmap.component_count == 0 || pixmap.component_count > 4)
        {
            return false;
        }

        if (pixmap.component_size == 0)
        {
            return false;
        }

        return true;
    }

    static
    bool skipLengthBlock(BigEndianConstPointer& p, ConstMemory memory)
    {
        if (p + 2 > memory.end())
        {
            return false;
        }

        u16 length = p.read16();
        if (length < 2)
        {
            return false;
        }

        p += length - 2;
        return p <= memory.end();
    }

    static
    bool skipPixPattern(BigEndianConstPointer& p, ConstMemory memory)
    {
        if (p + 10 > memory.end())
        {
            return false;
        }

        u16 pattern = p.read16();
        p += 8;

        if (pattern == 2)
        {
            p += 6;
            return p <= memory.end();
        }

        if (pattern != 1)
        {
            return false;
        }

        if (!skipLengthBlock(p, memory))
        {
            return false;
        }

        PictRect frame;
        if (!readPictRect(p, memory, frame))
        {
            return false;
        }

        PictPixmap pixmap;
        if (!readPixmap(p, memory, pixmap))
        {
            return false;
        }

        p += 4;
        p += 2;
        u16 color_count = p.read16();

        if (p + u32(color_count + 1) * 4 > memory.end())
        {
            return false;
        }

        p += u32(color_count + 1) * 4;

        int width = frame.height();
        int height = frame.width();
        u16 row_bytes = pixmap.bits_per_pixel <= 8 ? u16(width & 0x7fff) : u16(width);

        if (pixmap.bits_per_pixel == 16)
        {
            width *= 2;
        }

        if (row_bytes == 0)
        {
            row_bytes = width;
        }

        if (row_bytes < 8)
        {
            p += size_t(row_bytes) * height;
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                if (p >= memory.end())
                {
                    return false;
                }

                size_t scanline_length = row_bytes > 250 ? p.read16() : p.read8();
                p += scanline_length;
            }
        }

        return p <= memory.end();
    }

    static
    bool skipTextOpcode(BigEndianConstPointer& p, ConstMemory memory, int extra)
    {
        if (p + 4 + extra + 1 > memory.end())
        {
            return false;
        }

        p += 4 + extra;
        u8 count = p.read8();
        p += count;
        return p <= memory.end();
    }

    static
    bool skipReservedOpcode(BigEndianConstPointer& p, ConstMemory memory, u16 opcode)
    {
        if (opcode >= 0x00d0 && opcode <= 0x00fe)
        {
            if (p + 2 > memory.end())
            {
                return false;
            }

            u16 length = p.read16();
            p += length;
            return p <= memory.end();
        }

        if (opcode >= 0x8100 && opcode <= 0xffff)
        {
            if (p + 2 > memory.end())
            {
                return false;
            }

            u16 length = p.read16();
            p += length;
            return p <= memory.end();
        }

        if (opcode >= 0x0100 && opcode <= 0x7fff)
        {
            size_t length = (opcode >> 7) & 0xff;
            p += length;
            return p <= memory.end();
        }

        return false;
    }

    static
    bool skipOpcode(BigEndianConstPointer& p, ConstMemory memory, u16 opcode)
    {
        if (opcode == 0x0001)
        {
            if (p + 2 > memory.end())
            {
                return false;
            }

            u16 length = p.read16();
            if (length < 2)
            {
                return false;
            }

            if (length == 0x000a)
            {
                PictRect rect;
                return readPictRect(p, memory, rect);
            }

            p += length - 2;
            return p <= memory.end();
        }

        if (opcode == 0x00a0)
        {
            // LongText: 2-byte style/size word only (not a length-prefixed block).
            if (p + 2 > memory.end())
            {
                return false;
            }

            p += 2;
            return true;
        }

        if (opcode == 0x00a1)
        {
            if (p + 6 > memory.end())
            {
                return false;
            }

            p += 2; // comment type
            u16 length = p.read16();
            if (length < 4)
            {
                return false;
            }

            p += 4; // reserved
            length -= 4;
            p += length;
            return p <= memory.end();
        }

        if (opcode >= 0x0028 && opcode <= 0x002b)
        {
            int extra = (opcode == 0x0028) ? 0 : (opcode == 0x002b) ? 2 : 1;
            return skipTextOpcode(p, memory, extra);
        }

        if (opcode < 0x00a2)
        {
            s8 length = g_pict_opcode_length[opcode];

            switch (length)
            {
                case PICT_LEN_SPECIAL:
                    return skipLengthBlock(p, memory);

                case PICT_LEN_PIXPAT:
                    return skipPixPattern(p, memory);

                case PICT_LEN_LENGTH_WORD:
                    if (p + 2 > memory.end())
                    {
                        return false;
                    }
                    p += 2;
                    return p <= memory.end();

                default:
                    if (length < 0)
                    {
                        return false;
                    }

                    p += length;
                    return p <= memory.end();
            }
        }

        if ((opcode >= 0x00b0 && opcode <= 0x00cf) ||
            (opcode >= 0x8000 && opcode <= 0x80ff))
        {
            return true;
        }

        return skipReservedOpcode(p, memory, opcode);
    }

    static
    ConstMemory findJpegInBlock(ConstMemory block)
    {
        for (size_t i = 0; i + 3 < block.size; ++i)
        {
            if (block.address[i] == 0xff && block.address[i + 1] == 0xd8 && block.address[i + 2] == 0xff)
            {
                return ConstMemory(block.address + i, block.size - i);
            }
        }

        return ConstMemory();
    }

    static
    bool parseStdPixBlock(ConstMemory block, PictRect& src_rect, size_t& data_offset)
    {
        if (block.size < 68)
        {
            return false;
        }

        BigEndianConstPointer p = block.address;

        p += 2; // version
        p += 36; // matrix
        p += 4; // matte size
        p += 8; // matte rect
        p += 2; // transfer mode

        if (!readPictRect(p, block, src_rect))
        {
            return false;
        }

        p += 4; // accuracy
        u32 mask_size = p.read32();
        if (p + mask_size > block.end())
        {
            return false;
        }

        p += mask_size;

        size_t payload_offset = size_t(p - block.address);
        data_offset = payload_offset;

        if (payload_offset + 8 <= block.size)
        {
            u32 atom_size = bigEndian::uload32(block.address + payload_offset);
            if (atom_size >= 8 && payload_offset + atom_size <= block.size)
            {
                data_offset = payload_offset + atom_size;
            }
        }

        return true;
    }

    static
    std::unique_ptr<Bitmap> decodeChunkyRgbBlock(ConstMemory block, int width, int height, size_t data_offset)
    {
        if (width < 1 || height < 1)
        {
            return nullptr;
        }

        const size_t stride = size_t(width) * 3;
        const size_t need = stride * size_t(height);

        if (data_offset + need > block.size)
        {
            return nullptr;
        }

        const u8* pixels = block.address + data_offset;

        auto output = std::make_unique<Bitmap>(width, height,
            Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        for (int y = 0; y < height; ++y)
        {
            const u8* row = pixels + size_t(y) * stride;
            Color* dest = output->address<Color>(0, y);

            for (int x = 0; x < width; ++x)
            {
                dest[x] = Color(row[x * 3 + 0], row[x * 3 + 1], row[x * 3 + 2], 255);
            }
        }

        return output;
    }

    static
    std::unique_ptr<Bitmap> decodeQtImageBlock(ConstMemory block, int width, int height)
    {
        PictRect src_rect;
        size_t data_offset = 0;

        if (parseStdPixBlock(block, src_rect, data_offset) && src_rect.valid())
        {
            width = src_rect.width();
            height = src_rect.height();
        }

        if (auto bitmap = decodeChunkyRgbBlock(block, width, height, data_offset))
        {
            return bitmap;
        }

        return nullptr;
    }

    static
    void unpackNibbles(const u8* src, u8* dest, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            dest[i * 2 + 0] = (src[i] >> 4) & 0x0f;
            dest[i * 2 + 1] = src[i] & 0x0f;
        }
    }

    static
    bool pixmapUsesPackBits(const PictPixmap& pixmap, bool opcode_packed, int width, int height)
    {
        MANGO_UNREFERENCED(width);
        MANGO_UNREFERENCED(height);

        if (opcode_packed)
        {
            return true;
        }

        // pack_type 4 (and PackBits opcodes) store rows with PackBits compression.
        return pixmap.pack_type == 4 || pixmap.pack_type == 3;
    }

    static
    bool decodePackBitsRows(BigEndianConstPointer& p, ConstMemory memory,
                            int width, int height, int bytes_per_line, int bits_per_pixel,
                            bool use_packbits, std::vector<u8>& pixels)
    {
        size_t bytes_per_pixel = 1;
        size_t row_width = size_t(width);

        if (bits_per_pixel == 16)
        {
            bytes_per_pixel = 2;
            row_width *= 2;
        }
        else if (bits_per_pixel == 32)
        {
            row_width *= 4;
        }

        if (bytes_per_line == 0)
        {
            bytes_per_line = int(row_width);
        }

        if (!use_packbits)
        {
            size_t row_bytes = size_t(bytes_per_line);
            pixels.resize(size_t(height) * row_bytes);

            for (int y = 0; y < height; ++y)
            {
                if (p + row_bytes > memory.end())
                {
                    return false;
                }

                std::memcpy(pixels.data() + size_t(y) * row_bytes, p, row_bytes);
                p += row_bytes;
            }

            return true;
        }

        size_t storage_stride = row_width;
        pixels.assign(size_t(height) * storage_stride, 0);

        std::vector<u8> scanline(8192);
        std::vector<u8> unpack(8192);

        for (int y = 0; y < height; ++y)
        {
            u8* row = pixels.data() + size_t(y) * storage_stride;
            size_t row_offset = 0;

            if (bytes_per_line < 8)
            {
                if (p + bytes_per_line > memory.end())
                {
                    return false;
                }

                const u8* src = p;
                p += bytes_per_line;

                if (bits_per_pixel == 4)
                {
                    unpackNibbles(src, row, size_t(bytes_per_line));
                }
                else if (bits_per_pixel == 8 || bits_per_pixel == 16 || bits_per_pixel == 32)
                {
                    std::memcpy(row, src, std::min(storage_stride, size_t(bytes_per_line)));
                }
                else
                {
                    return false;
                }

                continue;
            }

            if (p >= memory.end())
            {
                return false;
            }

            size_t scanline_length = bytes_per_line > 250 ? p.read16() : p.read8();
            if (scanline_length == 0 || scanline_length > scanline.size())
            {
                return false;
            }

            if (p + scanline_length > memory.end())
            {
                return false;
            }

            std::memcpy(scanline.data(), p, scanline_length);
            p += scanline_length;

            size_t j = 0;
            while (j < scanline_length && row_offset < storage_stride)
            {
                if ((scanline[j] & 0x80) == 0)
                {
                    size_t length = size_t(scanline[j]) + 1;
                    size_t number_pixels = length * bytes_per_pixel;
                    const u8* src = scanline.data() + j + 1;

                    if (j + 1 + number_pixels > scanline_length)
                    {
                        return false;
                    }

                    if (bits_per_pixel == 4)
                    {
                        unpackNibbles(src, unpack.data(), length);
                        number_pixels = length * 2;
                        src = unpack.data();
                    }

                    size_t copy = std::min(number_pixels, storage_stride - row_offset);
                    std::memcpy(row + row_offset, src, copy);
                    row_offset += copy;
                    j += length * bytes_per_pixel + 1;
                }
                else
                {
                    size_t length = size_t((scanline[j] ^ 0xff) & 0xff) + 2;
                    const u8* src = scanline.data() + j + 1;

                    if (j + 1 + bytes_per_pixel > scanline_length)
                    {
                        return false;
                    }

                    for (size_t i = 0; i < length && row_offset < storage_stride; ++i)
                    {
                        if (bits_per_pixel == 4)
                        {
                            unpackNibbles(src, unpack.data(), 1);
                            row[row_offset + 0] = unpack[0];
                            if (row_offset + 1 < storage_stride)
                            {
                                row[row_offset + 1] = unpack[1];
                            }
                            row_offset += 2;
                        }
                        else
                        {
                            size_t copy = std::min(bytes_per_pixel, storage_stride - row_offset);
                            std::memcpy(row + row_offset, src, copy);
                            row_offset += bytes_per_pixel;
                        }
                    }

                    j += bytes_per_pixel + 1;
                }
            }
        }

        return true;
    }

    static
    void blitTileToCanvas(Bitmap& canvas, const PictRect& frame, const PictRect& destination,
                          const Bitmap& tile)
    {
        int x = destination.left - frame.left;
        int y = destination.top - frame.top;
        canvas.blit(x, y, tile);
    }

    static
    std::unique_ptr<Bitmap> convertPixmapToBitmap(const std::vector<u8>& pixels, int width, int height,
                                                    const PictPixmap& pixmap, const std::vector<Color>& palette)
    {
        auto output = std::make_unique<Bitmap>(width, height,
            Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        const bool indexed = pixmap.bits_per_pixel <= 8 && !palette.empty();
        const bool alpha = pixmap.component_count >= 4;

        const size_t row_stride = pixels.size() / size_t(height);

        for (int y = 0; y < height; ++y)
        {
            const u8* row = pixels.data() + size_t(y) * row_stride;

            Color* dest = output->address<Color>(0, y);

            if (indexed)
            {
                for (int x = 0; x < width; ++x)
                {
                    u8 index = row[x];
                    if (index < palette.size())
                    {
                        dest[x] = palette[index];
                    }
                }
            }
            else if (pixmap.bits_per_pixel == 16)
            {
                for (int x = 0; x < width; ++x)
                {
                    u8 i = row[x * 2 + 0];
                    u8 k = row[x * 2 + 1];
                    dest[x] = Color(
                        (i & 0x7c) << 1,
                        u8(((i & 0x03) << 6) | ((k & 0xe0) >> 2)),
                        u8((k & 0x1f) << 3),
                        255);
                }
            }
            else if (pixmap.bits_per_pixel == 32)
            {
                for (int x = 0; x < width; ++x)
                {
                    if (alpha)
                    {
                        dest[x] = Color(
                            row[x + width * 1],
                            row[x + width * 2],
                            row[x + width * 3],
                            row[x + width * 0]);
                    }
                    else
                    {
                        dest[x] = Color(
                            row[x + width * 0],
                            row[x + width * 1],
                            row[x + width * 2],
                            255);
                    }
                }
            }
            else
            {
                return nullptr;
            }
        }

        return output;
    }

    struct NativePictDecoder
    {
        ConstMemory memory;
        size_t pict_offset = 0;
        PictRect frame;
        Color foreground { 0, 0, 0, 255 };
        Color background { 255, 255, 255, 255 };
        std::unique_ptr<Bitmap> canvas;
        bool has_content = false;
        bool m_v1 = false;
        bool m_has_mono_pattern = false;
        bool m_solid_rgb_fill = false;
        u8 m_mono_pattern [8] {};

        void fillRectWithPattern(const PictRect& rect)
        {
            int x0 = rect.left - frame.left;
            int y0 = rect.top - frame.top;
            int x1 = rect.right - frame.left;
            int y1 = rect.bottom - frame.top;

            for (int y = y0; y < y1; ++y)
            {
                Color* row = canvas->address<Color>(0, y);
                for (int x = x0; x < x1; ++x)
                {
                    if (m_solid_rgb_fill || !m_has_mono_pattern)
                    {
                        row[x] = foreground;
                    }
                    else
                    {
                        int bit = (m_mono_pattern[(y & 7)] >> (7 - (x & 7))) & 1;
                        row[x] = bit ? foreground : background;
                    }
                }
            }

            has_content = true;
        }

        bool decodePixPattern(BigEndianConstPointer& p, u16 opcode)
        {
            MANGO_UNREFERENCED(opcode);

            if (p + 10 > memory.end())
            {
                return false;
            }

            u16 pattern = p.read16();
            p += 8;

            if (pattern == 2)
            {
                // ditherPat: 8-byte B&W fallback + desired RGB (solid on color displays).
                if (p + 6 > memory.end())
                {
                    return false;
                }

                Color rgb = readPictColor(p);
                m_has_mono_pattern = false;
                m_solid_rgb_fill = true;

                if (rgb.r | rgb.g | rgb.b)
                {
                    foreground = rgb;
                }

                return true;
            }

            m_solid_rgb_fill = false;

            if (pattern == 0)
            {
                std::memcpy(m_mono_pattern, p - 8, 8);
                m_has_mono_pattern = true;
                return true;
            }

            if (pattern != 1)
            {
                BigEndianConstPointer skip = p - 10;
                return skipPixPattern(skip, memory);
            }

            // Color pixmap pattern (not used by 4/5 test files).
            BigEndianConstPointer skip = p - 10;
            return skipPixPattern(skip, memory);
        }

        bool decodeQuickTimeOpcode(BigEndianConstPointer& p)
        {
            if (p + 4 > memory.end())
            {
                return false;
            }

            const u8* block_start = p;
            u32 length = p.read32();
            if (length < 8 || p + length > memory.end())
            {
                return false;
            }

            ConstMemory block(p, length);

            ConstMemory jpeg = findJpegInBlock(block);
            if (jpeg.size)
            {
                ImageDecoder decoder(jpeg, ".jpg");
                if (decoder.isDecoder())
                {
                    ImageHeader embedded = decoder.header();
                    if (embedded.width > 0 && embedded.height > 0)
                    {
                        Bitmap tile(embedded.width, embedded.height, embedded.format);
                        if (decoder.decode(tile))
                        {
                            blitTileToCanvas(*canvas, frame, frame, tile);
                            has_content = true;
                            p = block_start + 4 + length;
                            return true;
                        }
                    }
                }
            }

            auto qt_image = decodeQtImageBlock(block, frame.width(), frame.height());
            p = block_start + 4 + length;

            if (!qt_image)
            {
                return false;
            }

            blitTileToCanvas(*canvas, frame, frame, *qt_image);
            has_content = true;
            return true;
        }

        bool handleOpcode(BigEndianConstPointer& p, u16 opcode)
        {
            if (opcode == 0x8200)
            {
                return decodeQuickTimeOpcode(p);
            }

            if (opcode >= 0x0090 && opcode <= 0x009b)
            {
                if (!decodePixmapOpcode(p, opcode))
                {
                    return false;
                }

                return true;
            }

            if (opcode == 0x0012 || opcode == 0x0013 || opcode == 0x0014)
            {
                return decodePixPattern(p, opcode);
            }

            if (opcode == 0x001a)
            {
                foreground = readPictColor(p);
                return true;
            }

            if (opcode == 0x001b)
            {
                background = readPictColor(p);
                return true;
            }

            if (opcode == 0x0034 || opcode == 0x003c)
            {
                PictRect rect;
                if (!readPictRect(p, memory, rect))
                {
                    return false;
                }

                fillRectWithPattern(rect);
                return true;
            }

            if (!skipOpcode(p, memory, opcode))
            {
                return skipReservedOpcode(p, memory, opcode);
            }

            return true;
        }

        bool parse()
        {
            BigEndianConstPointer p = memory.address + pict_offset;

            if (p + 12 > memory.end())
            {
                return false;
            }

            p += 2; // picSize
            if (!readPictRect(p, memory, frame))
            {
                return false;
            }

            u16 version = p.read16();
            m_v1 = (version == 0x1101);
            if (!m_v1 && version != 0x0011)
            {
                return false;
            }

            canvas = std::make_unique<Bitmap>(frame.width(), frame.height(),
                Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
            canvas->clear(background);

            if (m_v1)
            {
                while (p < memory.end())
                {
                    u8 op = p.read8();
                    if (op == 0)
                    {
                        continue;
                    }

                    if (op == 0xff)
                    {
                        break;
                    }

                    if (!handleOpcode(p, op))
                    {
                        if (has_content)
                        {
                            break;
                        }

                        return false;
                    }
                }

                return has_content;
            }

            while (p + 2 <= memory.end())
            {
                u16 opcode = p.read16();

                if (opcode == 0)
                {
                    continue;
                }

                if (opcode == 0x00ff)
                {
                    break;
                }

                if (opcode == 0x02ff)
                {
                    continue;
                }

                if (opcode == 0x0c00)
                {
                    if (p + 24 > memory.end())
                    {
                        return false;
                    }

                    p += 24;
                    continue;
                }

                if (!handleOpcode(p, opcode))
                {
                    if (has_content)
                    {
                        break;
                    }

                    return false;
                }
            }

            return has_content;
        }

        bool decodePixmapOpcode(BigEndianConstPointer& p, u16 opcode)
        {
            bool direct = (opcode == 0x009a || opcode == 0x009b);
            bool packed = (opcode == 0x0098 || opcode == 0x0099);
            bool has_region = (opcode == 0x0091 || opcode == 0x0099 || opcode == 0x009b);

            int bytes_per_line = 0;
            bool pixmap_extension = false;

            if (direct)
            {
                p += 2;
                p += 2;
                bytes_per_line = p.read16();
                pixmap_extension = true;
            }
            else
            {
                bytes_per_line = p.read16();
                pixmap_extension = (bytes_per_line & 0x8000) != 0;
            }

            bytes_per_line &= 0x7fff;

            PictRect tile_bounds;
            if (!readPictRect(p, memory, tile_bounds))
            {
                return false;
            }

            PictPixmap pixmap;
            std::vector<Color> palette;

            if (pixmap_extension)
            {
                if (!readPixmap(p, memory, pixmap))
                {
                    return false;
                }
            }
            else
            {
                pixmap.bits_per_pixel = 1;
                pixmap.component_count = 1;
                pixmap.component_size = 8;
            }

            if (!direct && pixmap_extension)
            {
                p += 4;
                u16 flags = p.read16();
                u16 color_count = p.read16();

                for (u16 i = 0; i <= color_count; ++i)
                {
                    u16 index = p.read16();
                    Color color = readPictColor(p);
                    if ((flags & 0x8000) == 0)
                    {
                        index = i;
                    }

                    if (index >= palette.size())
                    {
                        palette.resize(index + 1, Color(0, 0, 0, 255));
                    }

                    palette[index] = color;
                }
            }
            else if (!direct)
            {
                palette = { Color(0, 0, 0, 255), Color(255, 255, 255, 255) };
            }

            PictRect source;
            PictRect destination;
            if (!readPictRect(p, memory, source) || !readPictRect(p, memory, destination))
            {
                return false;
            }

            if (p + 2 > memory.end())
            {
                return false;
            }

            p += 2; // mode

            if (has_region)
            {
                if (!skipLengthBlock(p, memory))
                {
                    return false;
                }
            }

            int tile_width = tile_bounds.width();
            int tile_height = tile_bounds.height();

            std::vector<u8> pixels;
            bool use_packbits = pixmapUsesPackBits(pixmap, packed, tile_width, tile_height);
            if (!decodePackBitsRows(p, memory, tile_width, tile_height, bytes_per_line,
                                    pixmap.bits_per_pixel, use_packbits, pixels))
            {
                return false;
            }

            auto tile = convertPixmapToBitmap(pixels, tile_width, tile_height, pixmap, palette);
            if (!tile)
            {
                return false;
            }

            blitTileToCanvas(*canvas, frame, destination, *tile);
            has_content = true;
            return true;
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        std::unique_ptr<ImageDecoder> m_decoder;
        std::unique_ptr<Bitmap> m_bitmap;

        Interface(ConstMemory memory)
        {
            size_t pict_offset = applePictOffset(memory);
            if (pict_offset == ~size_t(0))
            {
                header.setError("[ImageDecoder.PICT] Not an Apple PICT image.");
                return;
            }

            std::string extension;
            ConstMemory payload = findEmbeddedPayload(memory, pict_offset, extension);
            if (payload.size)
            {
                m_decoder = std::make_unique<ImageDecoder>(payload, "embedded" + extension);
                if (m_decoder->isDecoder())
                {
                    header = m_decoder->header();
                    if (header.width && header.height)
                    {
                        icc = m_decoder->icc();
                        exif = m_decoder->exif();

                        printLine(Print::Info, "[ImageDecoder.PICT] forwarding {} bytes at offset {} as {}",
                            payload.size, payload.address - memory.address, extension);
                        return;
                    }

                    m_decoder.reset();
                }
            }

            NativePictDecoder native;
            native.memory = memory;
            native.pict_offset = pict_offset;

            if (!native.parse() || !native.canvas)
            {
                header.setError("[ImageDecoder.PICT] No embedded image payload and native decode failed.");
                return;
            }

            m_bitmap = std::move(native.canvas);
            header.width = m_bitmap->width;
            header.height = m_bitmap->height;
            header.depth = 0;
            header.levels = 0;
            header.faces = 0;
            header.format = m_bitmap->format;
            header.compression = TextureCompression::NONE;

            printLine(Print::Info, "[ImageDecoder.PICT] native decode {} x {}",
                header.width, header.height);
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

            if (m_decoder)
            {
                return m_decoder->decode(dest, options, level, depth, face);
            }

            ImageDecodeStatus status;
            if (!m_bitmap)
            {
                status.setError("[ImageDecoder.PICT] Decoder not initialized.");
                return status;
            }

            dest.blit(0, 0, *m_bitmap);
            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        return new Interface(memory);
    }

} // namespace

namespace mango::image
{

    void registerImageCodecPICT()
    {
        registerImageDecoder(createInterface, ".pict");
        registerImageDecoder(createInterface, ".pct");
    }

} // namespace mango::image

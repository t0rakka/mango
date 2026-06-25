/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    The lzw_decode() function is based on Jean-Marc Lienher / STB decoder.
    The symbol resolver is iterative instead of recursive like in the original.
*/
#include <algorithm>
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // decoder
    // ------------------------------------------------------------

    // Specification:
    // https://www.w3.org/Graphics/GIF/spec-gif89a.txt

    enum
    {
        GIF_IMAGE      = 0x2c,
        GIF_EXTENSION  = 0x21,
        GIF_TERMINATE  = 0x3b,

        PLAIN_TEXT_EXTENSION       = 0x01,
        GRAPHICS_CONTROL_EXTENSION = 0xf9,
        COMMENT_EXTENSION          = 0xfe,
        APPLICATION_EXTENSION      = 0xff,
    };

    struct gif_logical_screen_descriptor
    {
        u16	width = 0;
        u16	height = 0;
        u8	packed = 0;
        u8	background = 0;
        u8  aspect = 0;
        const u8* palette = nullptr;

        const u8* read(const u8* data, const u8* end)
        {
            LittleEndianConstPointer p = data;

            if (p + 7 <= end)
            {
                width      = p.read16();
                height     = p.read16();
                packed     = p.read8();
                background = p.read8();
                aspect     = p.read8();

                if (color_table_flag())
                {
                    palette = p;
                    p += color_table_size() * 3;
                }
            }

            return p;
        }

        bool color_table_flag() const { return (packed & 0x80) == 0x80; }
        int  color_resolution() const { return (packed >> 4) & 0x07; }
        bool sort_flag()        const { return (packed & 0x08) == 0x08; }
        int  color_table_size() const { return 1 << ((packed & 0x07) + 1); }
    };

    struct gif_image_descriptor
    {
        u16	left = 0;
        u16	top = 0;
        u16	width = 0;
        u16	height = 0;
        u8	field = 0;
        const u8* palette = nullptr;

        const u8* read(const u8* data, const u8* end)
        {
            LittleEndianConstPointer p = data;

            if (p + 9 <= end)
            {
                left   = p.read16();
                top    = p.read16();
                width  = p.read16();
                height = p.read16();
                field  = p.read8();

                if (color_table_flag())
                {
                    palette = p;
                    p += color_table_size() * 3;
                }
            }

            return p;
        }

        bool color_table_flag() const { return (field & 0x80) != 0; }
        bool interlaced()       const { return (field & 0x40) != 0; }
        int  color_table_size() const { return 1 << ((field & 0x07) + 1); }
    };

    struct gif_state
    {
        gif_logical_screen_descriptor screen_desc;

        bool first_frame = true;

        // Graphics Control Extension. A GCE applies ONLY to the image that follows it,
        // so these are reset to defaults before every frame and only overwritten when a
        // GCE is actually present.
        u16 delay = 2; // default: 50 Hz
        int disposal_method = 0;
        int user_input_flag = 0;
        int transparent_color_flag = 0;
        u8 transparent_color = 0;

        // Persistent RGBA canvas (logical screen size), used for animations. Each frame
        // can carry its own local color table, so per the spec the accumulated canvas is
        // a rendered raster of colors, not indices: every frame is resolved through its
        // own palette before compositing. Single-frame GIFs skip this and stay indexed.
        std::unique_ptr<u32[]> canvas;
        std::unique_ptr<u32[]> saved; // snapshot for disposal method 3 (restore to previous)
        bool saved_valid = false;

        // Disposal bookkeeping for the previously drawn frame. Disposal happens between
        // frames, so the previous frame's disposal is applied at the start of the next.
        int prev_disposal = 0;
        int prev_x = 0;
        int prev_y = 0;
        int prev_w = 0;
        int prev_h = 0;
        u32 prev_clear_color = 0; // fill color used by disposal method 2 (restore background)
    };

    const u8* lzw_decode(u8* dest, u8* dest_end, const u8* src, const u8* src_end)
    {
        constexpr int MAX_STACK_SIZE = 8192;
        constexpr int MAX_AVAILABLE = 0xfff;

        struct lzw
        {
            s16 prefix;
            u8 first;
            u8 suffix;
        } codes[MAX_STACK_SIZE];

        const u8 minimum_codesize = *src++;
        if (minimum_codesize > 12)
        {
            return nullptr;
        }

        const s32 code_clear = 1 << minimum_codesize;
        const s32 code_eoi = code_clear + 1;

        for (int i = 0; i < code_clear; ++i)
        {
            const u8 initcode = u8(i);
            codes[i].prefix = -1;
            codes[i].first = initcode;
            codes[i].suffix = initcode;
        }

        s32 codesize = minimum_codesize + 1;
        s32 codemask = (1 << codesize) - 1;

        s32 available = code_clear + 2;
        s32 oldcode = -1;

        s32 data = 0;
        s32 data_bits = 0;
        s32 length = 0;

        for (;;)
        {
            if (data_bits < codesize)
            {
                if (!length)
                {
                    // start compression block
                    length = *src++;
                    if (!length)
                    {
                        // terminator
                        return src;
                    }

                    if (src + length >= src_end)
                    {
                        // overflow
                        src = nullptr;
                        break;
                    }
                }

                // fill register
                --length;
                data |= s32(*src++) << data_bits;
                data_bits += 8;
            }
            else
            {
                // consume register
                s32 code = data & codemask;
                data >>= codesize;
                data_bits -= codesize;

                if (code == code_clear)
                {
                    // clear code
                    codesize = minimum_codesize + 1;
                    codemask = (1 << codesize) - 1;
                    available = code_clear + 2;
                    oldcode = -1;
                }
                else if (code == code_eoi)
                {
                    // end of information
                    src += length;
                    while ((length = *src++) > 0)
                    {
                        src += length;
                    }
                    return src;
                }
                else if (code <= available)
                {
                    if (oldcode >= 0)
                    {
                        lzw* p = &codes[available++];
                        if (available >= MAX_STACK_SIZE)
                        {
                            // too many codes
                            src = nullptr;
                            break;
                        }

                        p->prefix = s16(oldcode);
                        p->first = codes[oldcode].first;
                        p->suffix = (code == available) ? p->first : codes[code].first;
                    }
                    else if (code == available)
                    {
                        // illegal code
                        src = nullptr;
                        break;
                    }

                    oldcode = code;

                    // remember current address
                    u8* start = dest;

                    // resolve symbols
                    for (;;)
                    {
                        if (dest < dest_end)
                        {
                            u8 sample = codes[code].suffix;
                            *dest++ = sample;
                        }

                        if (codes[code].prefix < 0)
                            break;

                        code = codes[code].prefix;
                    }

                    // reverse symbols
                    std::reverse(start, dest);

                    if (available > codemask && available < MAX_AVAILABLE)
                    {
                        ++codesize;
                        codemask |= (codemask + 1);
                    }
                }
                else
                {
                    // illegal code
                    src = nullptr;
                    break;
                }
            }
        }

        return src;
    }

    void deinterlace(u8* dest, const u8* buffer, int width, int height)
    {
        for (int pass = 0; pass < 4; ++pass)
        {
            const int rate = std::min(8, 16 >> pass); // 8, 8, 4, 2
            const int start = (8 >> pass) & 0x7;      // 0, 4, 2, 1

            for (int y = start; y < height; y += rate)
            {
                std::memcpy(dest + y * width, buffer, width);
                buffer += width;
            }
        }
    }

    void canvas_fill_rect(u32* canvas, int cw, int ch, int x, int y, int w, int h, u32 value)
    {
        // NOTE: frame rectangles can extend past the logical screen; clip to canvas.
        int x0 = std::max(0, x);
        int y0 = std::max(0, y);
        int x1 = std::min(cw, x + w);
        int y1 = std::min(ch, y + h);

        for (int yy = y0; yy < y1; ++yy)
        {
            u32* d = canvas + yy * cw;
            for (int xx = x0; xx < x1; ++xx)
            {
                d[xx] = value;
            }
        }
    }

    void canvas_composite(u32* canvas, int cw, int ch, const u8* src, int x, int y, int w, int h,
                          const Palette& palette, bool transparent_flag, u8 transparent)
    {
        // Resolve this frame's indices through its own palette while compositing. Because
        // the canvas is true color, a later frame with a different palette can't corrupt
        // pixels we draw here.
        // NOTE: clipping happens with some image files; don't be too clever and "optimize" this later :)
        for (int sy = 0; sy < h; ++sy)
        {
            int cy = y + sy;
            if (cy < 0 || cy >= ch)
                continue;

            const u8* s = src + sy * w;
            u32* d = canvas + cy * cw;

            for (int sx = 0; sx < w; ++sx)
            {
                int cx = x + sx;
                if (cx < 0 || cx >= cw)
                    continue;

                u8 sample = s[sx];
                if (transparent_flag && sample == transparent)
                    continue; // transparent pixels leave the canvas (previous frame) untouched

                d[cx] = palette[sample];
            }
        }
    }

    Palette build_palette(const gif_image_descriptor& image_desc, gif_state& state)
    {
        Palette palette;

        if (image_desc.color_table_flag())
        {
            // local palette
            palette.size = image_desc.color_table_size();

            for (u32 i = 0; i < palette.size; ++i)
            {
                u32 r = image_desc.palette[i * 3 + 0];
                u32 g = image_desc.palette[i * 3 + 1];
                u32 b = image_desc.palette[i * 3 + 2];
                palette[i] = Color(r, g, b, 0xff);
            }
        }
        else
        {
            // global palette
            palette.size = state.screen_desc.color_table_size();

            for (u32 i = 0; i < palette.size; ++i)
            {
                u32 r = state.screen_desc.palette[i * 3 + 0];
                u32 g = state.screen_desc.palette[i * 3 + 1];
                u32 b = state.screen_desc.palette[i * 3 + 2];
                palette[i] = Color(r, g, b, 0xff);
            }
        }

        // transparency: only the alpha matters, the rgb of the index is left intact
        if (state.transparent_color_flag)
        {
            palette[state.transparent_color].a = 0;
        }

        return palette;
    }

    // Background color from the GLOBAL color table (the Background Color Index is only
    // meaningful when a Global Color Table is present), as an opaque rgba value.
    u32 background_color(const gif_state& state)
    {
        if (state.screen_desc.color_table_flag())
        {
            const u8* gp = state.screen_desc.palette;
            u32 i = state.screen_desc.background;
            return Color(gp[i * 3 + 0], gp[i * 3 + 1], gp[i * 3 + 2], 0xff);
        }

        return 0;
    }

    std::unique_ptr<u8[]> decode_indices(const u8*& data, const u8* end, const gif_image_descriptor& image_desc)
    {
        int width = image_desc.width;
        int height = image_desc.height;
        int bytes = width * height;

        std::unique_ptr<u8[]> bits(new u8[bytes]);
        const u8* next = lzw_decode(bits.get(), bits.get() + bytes, data, end);
        if (!next)
        {
            // truncated or corrupt lzw stream
            return nullptr;
        }
        data = next;

        // deinterlace
        if (image_desc.interlaced())
        {
            u8* temp = new u8[bytes];
            deinterlace(temp, bits.get(), width, height);
            bits.reset(temp);
        }

        return bits;
    }

    // Single-frame GIFs decode straight into an indexed target (the legacy/demoscene
    // path): we keep the indices and palette intact.
    const u8* read_image_indexed(const u8* data, const u8* end, gif_state& state, Surface& target)
    {
        gif_image_descriptor image_desc;
        data = image_desc.read(data, end);

        Palette palette = build_palette(image_desc, state);

        u8 background = state.screen_desc.background;
        u8 transparent = state.transparent_color;

        std::unique_ptr<u8[]> bits = decode_indices(data, end, image_desc);
        if (!bits)
        {
            return nullptr;
        }

        if (target.palette)
        {
            *target.palette = palette;
        }

        int x = image_desc.left;
        int y = image_desc.top;
        int width = image_desc.width;
        int height = image_desc.height;

        // clear the canvas: transparent index when transparency is in use, else background
        u8 clear = state.transparent_color_flag ? transparent : background;
        for (int sy = 0; sy < target.height; ++sy)
        {
            std::memset(target.address<u8>(0, sy), clear, target.width);
        }

        // NOTE: clipping happens with some image files; don't be too clever and "optimize" this later :)
        for (int sy = 0; sy < height; ++sy)
        {
            int cy = y + sy;
            if (cy < 0 || cy >= target.height)
                continue;

            const u8* s = bits.get() + sy * width;
            u8* d = target.address<u8>(0, cy);

            for (int sx = 0; sx < width; ++sx)
            {
                int cx = x + sx;
                if (cx < 0 || cx >= target.width)
                    continue;

                u8 sample = s[sx];
                if (state.transparent_color_flag && sample == transparent)
                    continue;

                d[cx] = sample;
            }
        }

        return data;
    }

    // Animations decode into a persistent RGBA canvas: each frame is resolved through its
    // own palette and composited with transparency + disposal applied in color space.
    const u8* read_image_rgba(const u8* data, const u8* end, gif_state& state, Surface& target)
    {
        gif_image_descriptor image_desc;
        data = image_desc.read(data, end);

        Palette palette = build_palette(image_desc, state);
        u8 transparent = state.transparent_color;

        const int screen_w = state.screen_desc.width;
        const int screen_h = state.screen_desc.height;
        const int canvas_size = screen_w * screen_h;

        // allocate the persistent canvas (once)
        if (!state.canvas)
        {
            state.canvas.reset(new u32[canvas_size]);
            state.saved.reset(new u32[canvas_size]);
        }

        // restore-to-background uses transparent when this frame has transparency,
        // otherwise the logical screen background color
        u32 clear_color = state.transparent_color_flag ? 0 : background_color(state);

        if (state.first_frame)
        {
            std::fill_n(state.canvas.get(), canvas_size, clear_color);
            state.prev_disposal = 0;
            state.saved_valid = false;
        }
        else
        {
            // Apply the previous frame's disposal before drawing this one.
            switch (state.prev_disposal)
            {
                case 2:
                    // restore to background color
                    canvas_fill_rect(state.canvas.get(), screen_w, screen_h,
                                     state.prev_x, state.prev_y, state.prev_w, state.prev_h,
                                     state.prev_clear_color);
                    break;

                case 3:
                    // restore to previous (the snapshot we took before the previous frame)
                    if (state.saved_valid)
                    {
                        std::memcpy(state.canvas.get(), state.saved.get(), canvas_size * sizeof(u32));
                    }
                    break;

                default:
                    // 0/1: do not dispose, leave the canvas as-is
                    break;
            }
        }

        int x = image_desc.left;
        int y = image_desc.top;
        int width = image_desc.width;
        int height = image_desc.height;

        // Snapshot for disposal method 3. We only ever modify the frame rectangle, so a
        // full-canvas snapshot is equivalent to (and simpler than) saving just the rect.
        if (state.disposal_method == 3)
        {
            std::memcpy(state.saved.get(), state.canvas.get(), canvas_size * sizeof(u32));
            state.saved_valid = true;
        }

        std::unique_ptr<u8[]> bits = decode_indices(data, end, image_desc);
        if (!bits)
        {
            return nullptr;
        }

        // composite this frame into the persistent rgba canvas
        canvas_composite(state.canvas.get(), screen_w, screen_h, bits.get(),
                         x, y, width, height,
                         palette, state.transparent_color_flag != 0, transparent);

        // remember disposal bookkeeping for the next frame
        state.prev_disposal = state.disposal_method;
        state.prev_x = x;
        state.prev_y = y;
        state.prev_w = width;
        state.prev_h = height;
        state.prev_clear_color = clear_color;

        // publish the rgba canvas to the decode target
        int copy_w = std::min(screen_w, target.width);
        int copy_h = std::min(screen_h, target.height);

        for (int sy = 0; sy < copy_h; ++sy)
        {
            u8* dest = target.address<u8>(0, sy);
            std::memcpy(dest, state.canvas.get() + sy * screen_w, copy_w * sizeof(u32));
        }

        return data;
    }

    const u8* read_image(const u8* data, const u8* end, gif_state& state, Surface& target)
    {
        if (target.format.isIndexed())
        {
            return read_image_indexed(data, end, state, target);
        }

        return read_image_rgba(data, end, state, target);
    }

    void read_graphics_control_extension(const u8* p, gif_state& state)
    {
        LittleEndianConstPointer x = p;

        u8 packed = *x++;

        state.disposal_method = (packed >> 2) & 0x07; // 1 - do not dispose, 2 - restore background color, 3 - restore previous
        state.user_input_flag = (packed >> 1) & 0x01; // 0 - user input is not expected, 1 - user input is expected
        state.transparent_color_flag = packed & 0x01; // pixels with this value are not to be touched

        state.delay = x.read16(); // delay between frames in 1/100th of seconds (50 = .5 seconds, 100 = 1.0 seconds, etc)
        state.transparent_color = state.transparent_color_flag ? *x : 0;

        printLine(Print::Info, "      delay: {}, dispose: {}, transparent: {} ({})",
            state.delay,
            state.disposal_method,
            state.transparent_color_flag ? "YES" : "NO",
            state.transparent_color);
    }

    void read_application_extension(const u8* p)
    {
        std::string identifier(reinterpret_cast<const char*>(p), 8);
        MANGO_UNREFERENCED(identifier);
    }

    const u8* read_extension(const u8* p, gif_state& state)
    {
        u8 label = *p++;
        u8 size = *p++;
        printLine(Print::Info, "    label: {:#x}, size: {}", int(label), int(size));

        switch (label)
        {
            case PLAIN_TEXT_EXTENSION:
                break;
            case GRAPHICS_CONTROL_EXTENSION:
                read_graphics_control_extension(p, state);
                break;
            case COMMENT_EXTENSION:
                break;
            case APPLICATION_EXTENSION:
                read_application_extension(p);
                break;
        }

        for ( ; size; )
        {
            p += size;
            size = *p++;
        }

        return p;
    }

    const u8* read_magic(ImageHeader& header, const u8* data, const u8* end)
    {
        if (data + 6 >= end)
        {
            header.setError("[ImageDecoder.GIF] Incorrect header.");
            return nullptr;
        }

        const char* magic = reinterpret_cast<const char*>(data);
        data += 6;

        if (std::strncmp(magic, "GIF87a", 6) && std::strncmp(magic, "GIF89a", 6))
        {
            header.setError("[ImageDecoder.GIF] Header is missing GIF87a or GIF89a identifier.");
            return nullptr;
        }

        return data;
    }

    const u8* read_chunks(const u8* data, const u8* end, gif_state& state, Surface& target)
    {
        // A Graphics Control Extension applies only to the next image, so reset to
        // defaults here; the values are overwritten only if this frame carries a GCE.
        state.delay = 2;
        state.disposal_method = 0;
        state.user_input_flag = 0;
        state.transparent_color_flag = 0;
        state.transparent_color = 0;

        while (data < end)
        {
            u8 chunkID = *data++;
            printLine(Print::Info, "  chunkID: {:#x}", int(chunkID));
            switch (chunkID)
            {
                case GIF_EXTENSION:
                    data = read_extension(data, state);
                    break;

                case GIF_IMAGE:
                    data = read_image(data, end, state, target);
                    return data;

                case GIF_TERMINATE:
                    return nullptr;
            }
        }

        return nullptr;
    }

    // Count the number of image (frame) blocks in the stream. This is a cheap structural
    // walk used to decide the output format up front: single frame -> indexed (keep the
    // indices + palette), multiple frames -> rgba (frames may carry differing palettes
    // and must be composited in color space).
    int count_images(const u8* data, const u8* end)
    {
        const u8* p = data;
        int count = 0;

        while (p < end)
        {
            u8 id = *p++;

            if (id == GIF_EXTENSION)
            {
                if (p >= end)
                    break;
                ++p; // label

                // skip sub-blocks
                while (p < end)
                {
                    u8 size = *p++;
                    if (!size)
                        break;
                    p += size;
                }
            }
            else if (id == GIF_IMAGE)
            {
                if (p + 9 > end)
                    break;

                u8 field = p[8];
                p += 9;

                if (field & 0x80)
                {
                    // local color table
                    p += (1 << ((field & 0x07) + 1)) * 3;
                }

                if (p >= end)
                    break;
                ++p; // lzw minimum code size

                // skip image data sub-blocks
                while (p < end)
                {
                    u8 size = *p++;
                    if (!size)
                        break;
                    p += size;
                }

                ++count;
            }
            else
            {
                // GIF_TERMINATE or anything unexpected
                break;
            }
        }

        return count;
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        gif_state m_state;

        int m_frame_counter = 0;

        const u8* m_start;
        const u8* m_end;
        const u8* m_data;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            m_start = nullptr;
            m_end = m_memory.end();
            m_data = m_memory.address;

            m_data = read_magic(header, m_data, m_end);
            if (header.success)
            {
                m_data = m_state.screen_desc.read(m_data, m_end);

                m_start = m_data;

                // Single-frame GIFs stay indexed (the indices + palette are preserved);
                // animations resolve to rgba because frames may carry differing palettes.
                int frames = count_images(m_start, m_end);
                Format format = frames > 1
                    ? Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8)
                    : IndexedFormat(8);

                header.width   = m_state.screen_desc.width;
                header.height  = m_state.screen_desc.height;
                header.depth   = 0;
                header.levels  = 0;
                header.faces   = 0;
                header.format  = format;
                header.compression = TextureCompression::NONE;
            }
        }

        ~Interface()
        {
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
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

            status.current_frame_index = m_frame_counter;

            if (m_data)
            {
                m_state.first_frame = status.current_frame_index == 0;
                m_data = read_chunks(m_data, m_end, m_state, target);
                m_frame_counter += (m_data != nullptr);
            }

            if (!m_data)
            {
                // out of data - we reached the end of file
                if (m_frame_counter > 1)
                {
                    // there was more than 1 frame so it is animation -> restart animation
                    m_frame_counter = 0;
                    m_data = m_start;
                }
            }

            status.next_frame_index = m_frame_counter;

            status.frame_delay_numerator = m_state.delay;
            status.frame_delay_denominator = 100;

            status.direct = target.isDirect();

            target.resolve();

            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // encoder
    // ------------------------------------------------------------

    struct EncoderState
    {
        u8 data = 0;
        int index = 0;

        int chunkIndex = 0;
        u8 chunk[256];

        void writeBits(LittleEndianStream& s, u32 code, int numbits)
        {
            while (numbits > 0)
            {
                // how many bits still fit into the register
                int size = std::min(numbits, 8 - index);

                // insert the bits into the register
                u32 mask = (1 << size) - 1;
                data |= ((code & mask) << index);

                code >>= size;
                index += size;
                numbits -= size;

                if (index > 7)
                {
                    chunk[chunkIndex++] = data;
                    if (chunkIndex == 255)
                    {
                        flushChunk(s);
                    }

                    data = 0;
                    index = 0;
                }
            }
        }

        void flushChunk(LittleEndianStream& s)
        {
            s.write8(chunkIndex);
            s.write(chunk, chunkIndex);
            chunkIndex = 0;
        }

        void terminate(LittleEndianStream& s)
        {
            if (index)
            {
                chunk[chunkIndex++] = data;
            }

            if (chunkIndex > 0)
            {
                flushChunk(s);
            }
        }
    };

    void gif_encode_image_block(LittleEndianStream& s, int depth, Surface surface)
    {
        const int minCodeSize = depth;
        const u32 clearCode = 1 << depth;

        s.write8(minCodeSize);

        std::vector<u16> codetree(4096 * 256, 0);

        s32 curCode = -1;
        u32 codeSize = u32(minCodeSize + 1);
        u32 maxCode = clearCode + 1;

        EncoderState state;

        state.writeBits(s, clearCode, codeSize); // start with a fresh LZW dictionary

        for (int y = 0; y < surface.height; ++y)
        {
            u8* scan = surface.address(0, y);

            for (int x = 0; x < surface.width; ++x)
            {
                u8 nextValue = scan[x];

                if (curCode < 0)
                {
                    // first value in a new run
                    curCode = nextValue;
                }
                else if (codetree[curCode * 256 + nextValue])
                {
                    // current run already in the dictionary
                    curCode = codetree[curCode * 256 + nextValue];
                }
                else
                {
                    // finish the current run, write a code
                    state.writeBits(s, curCode, codeSize);

                    // insert the new run into the dictionary
                    codetree[curCode * 256 + nextValue] = u16(++maxCode);

                    if (maxCode >= (1ul << codeSize))
                    {
                        // dictionary entry count has broken a size barrier,
                        // we need more bits for codes
                        codeSize++;
                    }

                    if (maxCode == 4095)
                    {
                        // the dictionary is full, clear it out and begin anew
                        state.writeBits(s, clearCode, codeSize); // clear tree
                        
                        std::memset(codetree.data(), 0, 4096 * 256 * sizeof(u16));
                        codeSize = u32(minCodeSize + 1);
                        maxCode = clearCode + 1;
                    }

                    curCode = nextValue;
                }
            }
        }

        // compression footer
        state.writeBits(s, curCode, codeSize);
        state.writeBits(s, clearCode, codeSize);
        state.writeBits(s, clearCode + 1, minCodeSize + 1);
        state.terminate(s);

        s.write8(0); // image block terminator
    }

    void gif_encode_file(Stream& stream, const Surface& surface)
    {
        LittleEndianStream s = stream;

        // identifier
        s.write("GIF89a", 6);

        // screen descriptor
        s.write16(surface.width);
        s.write16(surface.height);

        u8 packed = 0;
        packed |= 0x7; // color table size as log2(size) - 1 (0 -> 2 colors, 7 -> 256 colors)
        packed |= 0x80; // color table present
        packed |= (0x7 << 4); // color resolution as bits - 1 (0 -> 1 bit, 7 -> 8 bits)
        s.write8(packed);

        s.write8(255); // background color
        s.write8(0); // aspect ratio

        // palette
        const Palette& palette = surface;

        for (int i = 0; i < 256; ++i)
        {
            s.write8(palette[i].r);
            s.write8(palette[i].g);
            s.write8(palette[i].b);
        }

        // NOTE: If we ever add support for encoding gif animations this is the section that would write the individual frames.
        {
            // image descriptor
            s.write8(GIF_IMAGE);

            s.write16(0);
            s.write16(0);
            s.write16(surface.width);
            s.write16(surface.height);

            // local palette
            u8 field = 0;
            s.write8(field);

            gif_encode_image_block(s, 8, surface);
        }

        // end of file
        s.write8(GIF_TERMINATE);
    }

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status;

        if (surface.format.isIndexed())
        {
            gif_encode_file(stream, surface);
        }
        else if (surface.format.isLuminance())
        {
            // 8 bit luminance to avoid blitting into indexed format
            TemporaryBitmap temp(surface, LuminanceFormat(8, 0xff, 0));

            // Generate grayscale palette
            Palette palette(256);

            for (int i = 0; i <256; ++i)
            {
                palette[i] = i * 0x00010101 | 0xff000000;
            }

            // Make palette visible to the encoder
            temp.palette = &palette;

            gif_encode_file(stream, temp);
        }
        else
        {
            QuantizedBitmap temp(surface);
            gif_encode_file(stream, temp);
        }

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecGIF()
    {
        registerImageDecoder(createInterface, ".gif");
        registerImageEncoder(imageEncode, ".gif");
    }

} // namespace mango::image

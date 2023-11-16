/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#include <mango/opengl/jpeg_decoder.hpp>
#include <mango/opengl/utilities.hpp>
#include "../jpeg/jpeg.hpp"

#ifdef MANGO_OPENGL_JPEG

// TODO: Huffman decoding in compute shader
// TODO: Different MCU configurations
// TODO: Progressive mode
// TODO: Different color formats (Chroma, Luminance)
// TODO: Arithmetic, CMYK, 12 bit DCT, 16 bit QT table, Lossless -> not supported

namespace
{

using namespace mango;
using namespace mango::jpeg;

static
const char* compute_shader_source = R"(
    #version 430 core
    //#extension GL_EXT_shader_explicit_arithmetic_types : enable

    layout(rgba8, binding = 0) uniform image2D uTexture;

    uniform int uQuantize0[64];
    uniform int uQuantize1[64];
    uniform int uQuantize2[64];

    uniform int u_huffman_dc[10];
    uniform int u_huffman_ac[10];
    uniform int u_huffman_pred[10];

    uniform int u_y0;
    uniform int u_y1;
    uniform int u_dataOffset;
    uniform int u_xmcu;

    layout(std430, binding = 0) buffer CompressedData
    {
        uint input[];
    };

    layout(std430, binding = 1) buffer HuffmanTables
    {
        uint huffman_tables[];
    };

    layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

    uint zigzagTable [] =
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

    int clamp(int x)
    {
        //if (x < 0) x = 0;
        //else if (x > 255) x = 255;
        return x;
    }

    struct IDCT
    {
        int x0, x1, x2, x3;
        int y0, y1, y2, y3;

        void compute(int s0, int s1, int s2, int s3, int s4, int s5, int s6, int s7)
        {
            const int n0 = (s2 + s6) * 2217;
            const int t2 = n0 + s6 * -7567;
            const int t3 = n0 + s2 * 3135;
            const int t0 = (s0 + s4) << 12;
            const int t1 = (s0 - s4) << 12;
            x0 = t0 + t3;
            x3 = t0 - t3;
            x1 = t1 + t2;
            x2 = t1 - t2;

            int p1 = s7 + s1;
            int p2 = s5 + s3;
            int p3 = s7 + s3;
            int p4 = s5 + s1;
            int p5 = (p3 + p4) * 4816;
            p1 = p1 * -3685 + p5;
            p2 = p2 * -10497 + p5;
            p3 = p3 * -8034;
            p4 = p4 * -1597;
            y0 = p1 + p3 + s7 * 1223;
            y1 = p2 + p4 + s5 * 8410;
            y2 = p2 + p3 + s3 * 12586;
            y3 = p1 + p4 + s1 * 6149;
        }
    };

    void idct(out int dest[64], int data[64], int qt[64])
    {
        int temp[64];

        for (int i = 0; i < 8; ++i)
        {
            // dequantize
            const int s0 = data[i + 8 * 0] * qt[i + 8 * 0];
            const int s1 = data[i + 8 * 1] * qt[i + 8 * 1];
            const int s2 = data[i + 8 * 2] * qt[i + 8 * 2];
            const int s3 = data[i + 8 * 3] * qt[i + 8 * 3];
            const int s4 = data[i + 8 * 4] * qt[i + 8 * 4];
            const int s5 = data[i + 8 * 5] * qt[i + 8 * 5];
            const int s6 = data[i + 8 * 6] * qt[i + 8 * 6];
            const int s7 = data[i + 8 * 7] * qt[i + 8 * 7];

            IDCT idct;
            idct.compute(s0, s1, s2, s3, s4, s5, s6, s7);

            const int bias = 0x200;
            idct.x0 += bias;
            idct.x1 += bias;
            idct.x2 += bias;
            idct.x3 += bias;
            temp[i * 8 + 0] = (idct.x0 + idct.y3) >> 10;
            temp[i * 8 + 1] = (idct.x1 + idct.y2) >> 10;
            temp[i * 8 + 2] = (idct.x2 + idct.y1) >> 10;
            temp[i * 8 + 3] = (idct.x3 + idct.y0) >> 10;
            temp[i * 8 + 4] = (idct.x3 - idct.y0) >> 10;
            temp[i * 8 + 5] = (idct.x2 - idct.y1) >> 10;
            temp[i * 8 + 6] = (idct.x1 - idct.y2) >> 10;
            temp[i * 8 + 7] = (idct.x0 - idct.y3) >> 10;
        }

        for (int i = 0; i < 8; ++i)
        {
            IDCT idct;
            idct.compute(temp[i +  0], temp[i +  8], temp[i + 16], temp[i + 24],
                         temp[i + 32], temp[i + 40], temp[i + 48], temp[i + 56]);

            const int bias = 0x10000 + (128 << 17);
            idct.x0 += bias;
            idct.x1 += bias;
            idct.x2 += bias;
            idct.x3 += bias;
            dest[i * 8 + 0] = clamp((idct.x0 + idct.y3) >> 17);
            dest[i * 8 + 1] = clamp((idct.x1 + idct.y2) >> 17);
            dest[i * 8 + 2] = clamp((idct.x2 + idct.y1) >> 17);
            dest[i * 8 + 3] = clamp((idct.x3 + idct.y0) >> 17);
            dest[i * 8 + 4] = clamp((idct.x3 - idct.y0) >> 17);
            dest[i * 8 + 5] = clamp((idct.x2 - idct.y1) >> 17);
            dest[i * 8 + 6] = clamp((idct.x1 - idct.y2) >> 17);
            dest[i * 8 + 7] = clamp((idct.x0 - idct.y3) >> 17);
        }
    }

    vec4 chroma_to_rgb(float y, float cb, float cr)
    {
        float r = y + cr * 1.400;
        float g = y - cb * 0.343 - cr * 0.711;
        float b = y + cb * 1.765;
        return vec4(r, g, b, 1.0);
    }

    struct BitBuffer
    {
        int offset;

        uint mem_data;
        int mem_remain;

        uint data;
        int remain;

        uint getByte()
        {
            if (mem_remain == 0)
            {
                mem_remain = 32;
                mem_data = input[offset++];
            }

            uint x = mem_data & 0xff;
            mem_data >>= 8;
            mem_remain -= 8;

            return x;
        }

        void ensure()
        {
            if (remain < 16)
            {
                remain += 16;
                uint x0 = getByte();
                uint x1 = getByte();
                data = (data << 8) | x0;
                data = (data << 8) | x1;
            }
        }

        int receive(int nbits)
        {
            ensure();
            remain -= nbits;
            uint mask = (1 << nbits) - 1;
            int value = int((data >> remain) & mask);
            return int(value - ((((value + value) >> nbits) - 1) & mask));
        }
    };

    struct HuffmanTable
    {
        uint size[17];
        uint value[256];
        uint maxcode[18];
        uint valueOffset[19];
    };

    /*
    struct Huffman
    {
        HuffmanTable tables[20];
        int last_dc_value[10];
    };
    */

    struct DecodeBlock
    {
        int dc;
        int ac;
        int pred;
    };

    int decode(inout BitBuffer bitbuffer, HuffmanTable tables[4], int index)
    {
        bitbuffer.ensure();

        int symbol;
        int size = 0;

        uint x = (bitbuffer.data << (32 - bitbuffer.remain));
        while (x > tables[index].maxcode[size])
        {
            ++size;
        }

        uint offset = (x >> (32 - size)) + tables[index].valueOffset[size];
        symbol = int(tables[index].value[offset]);

        bitbuffer.remain -= size;

        return symbol;
    }

    void huff_decode_mcu(out int dest[640], inout BitBuffer bitbuffer, inout int last_dc_value[3], DecodeBlock blocks[3], int numBlocks, HuffmanTable huffmanTables[4])
    {
        for (int i = 0; i < numBlocks * 64; ++i)
        {
            dest[i] = 0;
        }

        int offset = 0;

        for (int blk = 0; blk < numBlocks; ++blk)
        {
            int dc = blocks[blk].dc;
            int ac = blocks[blk].ac;
            int pred = blocks[blk].pred;

            // DC
            int s = decode(bitbuffer, huffmanTables, dc);
            if (s != 0)
            {
                s = bitbuffer.receive(s);
            }

            s += last_dc_value[pred];
            last_dc_value[pred] = s;

            dest[offset + 0] = s;

            // AC
            for (int i = 1; i < 64; )
            {
                int s = decode(bitbuffer, huffmanTables, ac);
                int x = s & 15;

                if (x != 0)
                {
                    i += (s >> 4);
                    s = bitbuffer.receive(x);
                    dest[offset + zigzagTable[i++]] = s;
                }
                else
                {
                    if (s < 16)
                        break;
                    i += 16;
                }
            }

            offset += 64;
        }
    }

    void main()
    {
#if 1
        BitBuffer bitbuffer;

        bitbuffer.offset = u_dataOffset;
        bitbuffer.mem_data = 0;
        bitbuffer.mem_remain = 0;
        bitbuffer.data = 0;
        bitbuffer.remain = 0;

        DecodeBlock decodeBlocks[3];

        decodeBlocks[0].dc = 0;
        decodeBlocks[0].ac = 1;
        decodeBlocks[0].pred = 0;

        decodeBlocks[1].dc = 2;
        decodeBlocks[1].ac = 3;
        decodeBlocks[1].pred = 1;

        decodeBlocks[2].dc = 2;
        decodeBlocks[2].ac = 3;
        decodeBlocks[2].pred = 2;

        HuffmanTable huffmanTables[4];

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 17; ++j)
            {
                huffmanTables[i].size[j] = huffman_tables[i * 310 + 0 + j];
            }

            for (int j = 0; j < 256; ++j)
            {
                huffmanTables[i].value[j] = huffman_tables[i * 310 + 17 + j];
            }

            for (int j = 0; j < 18; ++j)
            {
                huffmanTables[i].maxcode[j] = huffman_tables[i * 310 + 17 + 256 + j];
            }

            for (int j = 0; j < 19; ++j)
            {
                huffmanTables[i].valueOffset[j] = huffman_tables[i * 310 + 17 + 256 + 18 + j];
            }
        }

        int last_dc_value[3];

        for (int i = 0; i < 3; ++i)
        {
            last_dc_value[i] = 0;
        }

        for (int mx = 0; mx < u_xmcu; ++mx)
        {
            int temp[640];

            huff_decode_mcu(temp, bitbuffer, last_dc_value, decodeBlocks, 3, huffmanTables);

            int data0[64];
            int data1[64];
            int data2[64];

            for (int i = 0; i < 64; ++i)
            {
                data0[i] = temp[64 * 0 + i];
                data1[i] = temp[64 * 1 + i];
                data2[i] = temp[64 * 2 + i];
            }

            // xxx
            bool clean = true;
            for (int i = 0; i < 64; ++i)
            {
                if (data0[i] != 0) clean = false;
            }

            // inverse DCT

            int lu[64];
            int cb[64];
            int cr[64];

            idct(lu, data0, uQuantize0);
            idct(cb, data1, uQuantize1);
            idct(cr, data2, uQuantize2);

            // resolve color

            //uint idx = gl_GlobalInvocationID.y * gl_NumWorkGroups.x + gl_GlobalInvocationID.x;

            ivec2 blockCoord = ivec2(mx * 8, u_y0 * 8);
            //ivec2 blockCoord = ivec2(gl_GlobalInvocationID.xy * 8);

            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    ivec2 coord = blockCoord + ivec2(x, y);

                    float Y = float(lu[y * 8 + x]) / 255.0;
                    float cb = float(cb[y * 8 + x] - 128) / 255.0;
                    float cr = float(cr[y * 8 + x] - 128) / 255.0;
                    vec4 color = chroma_to_rgb(Y, cb, cr);

                    // xxx
                    if (clean) color = vec4(1.0, 1.0, 0.0, 1.0);

                    imageStore(uTexture, coord, color);
                }
            }
        }

#else

        for (int mx = 0; mx < u_xmcu; ++mx)
        {
            int data0[64];
            int data1[64];
            int data2[64];

            for (int i = 0; i < 64; ++i)
            {
                int base = mx * 64 * 3;
                data0[i] = int(input[base + 64 * 0 + i]);
                data1[i] = int(input[base + 64 * 1 + i]);
                data2[i] = int(input[base + 64 * 2 + i]);
            }

            // inverse DCT

            int lu[64];
            int cb[64];
            int cr[64];

            idct(lu, data0, uQuantize0);
            idct(cb, data1, uQuantize1);
            idct(cr, data2, uQuantize2);

            // resolve color

            //uint idx = gl_GlobalInvocationID.y * gl_NumWorkGroups.x + gl_GlobalInvocationID.x;

            ivec2 blockCoord = ivec2(mx * 8, u_y0 * 8);
            //ivec2 blockCoord = ivec2(gl_GlobalInvocationID.xy * 8);

            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    ivec2 coord = blockCoord + ivec2(x, y);

                    float Y = float(lu[y * 8 + x]) / 255.0;
                    float cb = float(cb[y * 8 + x] - 128) / 255.0;
                    float cr = float(cr[y * 8 + x] - 128) / 255.0;
                    vec4 color = chroma_to_rgb(Y, cb, cr);

                    imageStore(uTexture, coord, color);
                }
            }
        }
#endif
    }
)";

// ---------------------------------------------------------------------------------

struct HuffmanTable
{
    u8 size[17];
    u8 value[256];

    // acceleration tables
    u32 maxcode[18];
    u32 valueOffset[19];

    void configure(const HuffTable& source)
    {
        std::memcpy(size, source.size, 17);
        std::memcpy(value, source.value, 256);

        u8 huffsize[257];
        u32 huffcode[257];

        // Figure C.1: make table of Huffman code length for each symbol
        int p = 0;
        for (int j = 1; j <= 16; ++j)
        {
            int count = int(size[j]);
            while (count-- > 0)
            {
                huffsize[p++] = u8(j);
            }
        }
        huffsize[p] = 0;

        // Figure C.2: generate the codes themselves
        u32 code = 0;
        int si = huffsize[0];
        p = 0;
        while (huffsize[p])
        {
            while ((int(huffsize[p])) == si)
            {
                huffcode[p++] = code;
                code++;
            }
            code <<= 1;
            si++;
        }

        // Figure F.15: generate decoding tables for bit-sequential decoding
        p = 0;
        for (int j = 1; j <= 16; j++)
        {
            if (size[j])
            {
                valueOffset[j] = p - int(huffcode[p]);
                p += size[j];
                maxcode[j] = huffcode[p - 1];
                maxcode[j] <<= (32 - j);
                maxcode[j] |= (1 << (32 - j)) - 1;
            }
            else
            {
                maxcode[j] = 0;
            }
        }
        valueOffset[18] = 0;
        maxcode[17] = 0xffffffff;
    }
};

struct Huffman
{
    HuffmanTable table[2][JPEG_MAX_COMPS_IN_SCAN];
    int last_dc_value[JPEG_MAX_COMPS_IN_SCAN];
};

// ---------------------------------------------------------------------------------

// CPU decoder xD xD

using uint = u32;
static u32* input = nullptr;

struct BitBuffer
{
    int offset;

    uint mem_data;
    int mem_remain;

    uint data;
    int remain;

    uint getByte()
    {
        if (mem_remain == 0)
        {
            mem_remain = 32;
            mem_data = input[offset++];
        }

        uint x = mem_data & 0xff;
        mem_data >>= 8;
        mem_remain -= 8;

        return x;
    }

    void ensure()
    {
        if (remain < 16)
        {
            remain += 16;
            uint x0 = getByte();
            uint x1 = getByte();
            data = (data << 8) | x0;
            data = (data << 8) | x1;
        }
    }

    int receive(int nbits)
    {
        ensure();
        remain -= nbits;
        uint mask = (1 << nbits) - 1;
        int value = int((data >> remain) & mask);
        return int(value - ((((value + value) >> nbits) - 1) & mask));
    }
};

struct DecodeBlock
{
    int dc;
    int ac;
    int pred;
};

int decode(BitBuffer& bitbuffer, HuffmanTable tables[4], int index)
{
    bitbuffer.ensure();

    int symbol;
    int size = 0;

    uint x = (bitbuffer.data << (32 - bitbuffer.remain));
    while (x > tables[index].maxcode[size])
    {
        ++size;
    }

    uint offset = (x >> (32 - size)) + tables[index].valueOffset[size];
    symbol = int(tables[index].value[offset]);

    bitbuffer.remain -= size;

    return symbol;
}

void huff_decode_mcu(int dest[640], DecodeBlock blocks[3], int numBlocks, HuffmanTable huffmanTables[4], int last_dc_value[3], BitBuffer& bitbuffer)
{
    for (int i = 0; i < numBlocks * 64; ++i)
    {
        dest[i] = 0;
    }

    int offset = 0;

    for (int b = 0; b < numBlocks; ++b)
    {
        int dc = blocks[b].dc;
        int ac = blocks[b].ac;

        // DC
        int s = decode(bitbuffer, huffmanTables, dc);
        if (s != 0)
        {
            s = bitbuffer.receive(s);
        }

        int pred = blocks[b].pred;

        s += last_dc_value[pred];
        last_dc_value[pred] = s;

        dest[offset + 0] = s;

        // AC
        for (int i = 1; i < 64; )
        {
            int s = decode(bitbuffer, huffmanTables, ac);
            int x = s & 15;

            if (x != 0)
            {
                i += (s >> 4);
                s = bitbuffer.receive(x);
                dest[offset + zigzagTable[i++]] = s;
            }
            else
            {
                if (s < 16)
                    break;
                i += 16;
            }
        }

        offset += 64;
    }
}

void cpu_decode_interval(int* temp, const Buffer& buffer, const std::vector<int>& huffmanBuffer, int y0, int y1, int xmcu, int dataOffset)
{

    input = (u32*) buffer.data();
    u32* huffman_tables = (u32*) huffmanBuffer.data();

    BitBuffer bitbuffer;

    bitbuffer.offset = dataOffset;
    bitbuffer.mem_data = 0;
    bitbuffer.mem_remain = 0;
    bitbuffer.data = 0;
    bitbuffer.remain = 0;

    DecodeBlock decodeBlocks[3];

    decodeBlocks[0].dc = 0;
    decodeBlocks[0].ac = 1;
    decodeBlocks[0].pred = 0;

    decodeBlocks[1].dc = 2;
    decodeBlocks[1].ac = 3;
    decodeBlocks[1].pred = 1;

    decodeBlocks[2].dc = 2;
    decodeBlocks[2].ac = 3;
    decodeBlocks[2].pred = 2;

    HuffmanTable huffmanTables[4];

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 17; ++j)
        {
            huffmanTables[i].size[j] = huffman_tables[i * 310 + 0 + j];
        }

        for (int j = 0; j < 256; ++j)
        {
            huffmanTables[i].value[j] = huffman_tables[i * 310 + 17 + j];
        }

        for (int j = 0; j < 18; ++j)
        {
            huffmanTables[i].maxcode[j] = huffman_tables[i * 310 + 17 + 256 + j];
        }

        for (int j = 0; j < 19; ++j)
        {
            huffmanTables[i].valueOffset[j] = huffman_tables[i * 310 + 17 + 256 + 18 + j];
        }
    }

    int last_dc_value[3];

    for (int i = 0; i < 3; ++i)
    {
        last_dc_value[i] = 0;
    }

    for (int mx = 0; mx < xmcu; ++mx)
    {
        huff_decode_mcu(temp + 64 * 3 * mx, decodeBlocks, 3, huffmanTables, last_dc_value, bitbuffer);
    }
}

// ---------------------------------------------------------------------------------

struct ComputeDecoderContext : ComputeDecoder
{
    GLuint program = 0;
    GLuint texture = 0;

    void send(const ComputeDecoderInput& input) override
    {
        debugPrint("\n[ComputeDecode]\n");
        debugPrint("  MCU: %d x %d.\n", input.xmcu, input.ymcu);

        Buffer buffer;

        std::vector<u32> sizes;
        std::vector<u32> offsets;

        size_t currentOffset = 0;

        for (auto interval : input.intervals)
        {
            std::vector<u8> temp;

            // sanitize markers and stuff bytes

            const u8* p = interval.memory.address;
            const u8* end = interval.memory.end();

            while (p < end)
            {
                if (p[0] == 0xff)
                {
                    if (p[1] == 0x00)
                    {
                        // literal
                        temp.push_back(p[0]);
                    }
                    p += 2;
                }
                else
                {
                    temp.push_back(*p++);
                }
            }

            size_t padding = align_padding(temp.size(), 4);

            buffer.append(temp.data(), temp.size());
            buffer.append(padding, 0);

            u32 size = u32(temp.size() + padding);

            sizes.push_back(size);
            offsets.push_back(currentOffset);
            currentOffset += size;
        }

        GLuint sbo[2];
        glGenBuffers(2, sbo);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbo[0]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, buffer.size(), reinterpret_cast<GLvoid*>(buffer.data()), GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sbo[0]);

        size_t blocks_in_mcu = input.blocks.size();

        Huffman huffman;

        for (int Tc = 0; Tc <= input.huffman.maxTc; ++Tc)
        {
            for (int Th = 0; Th <= input.huffman.maxTh; ++Th)
            {
                huffman.table[Tc][Th].configure(input.huffman.table[Tc][Th]);
            }
        }

        for (size_t j = 0; j < blocks_in_mcu; ++j)
        {
            huffman.last_dc_value[j] = 0; // not used

            printf("  block %d -> dc: %d, ac: %d, pred: %d\n", 
                int(j), input.blocks[j].dc, input.blocks[j].ac, input.blocks[j].pred);
        }

        std::vector<int> huffmanBuffer(310 * 4);

        for (int i = 0; i < 4; ++i)
        {
            int* dest = huffmanBuffer.data() + 310 * i;

            HuffmanTable& source = huffman.table[i & 1][i >> 1];

            for (int j = 0; j < 17; ++j)
            {
                dest[0 + j] = source.size[j];
            }

            for (int j = 0; j < 256; ++j)
            {
                dest[17 + j] = source.value[j];
            }

            for (int j = 0; j < 18; ++j)
            {
                dest[17 + 256 + j] = source.maxcode[j];
            }

            for (int j = 0; j < 19; ++j)
            {
                dest[17 + 256 + 18 + j] = source.valueOffset[j];
            }
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbo[1]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, huffmanBuffer.size(), reinterpret_cast<GLvoid*>(huffmanBuffer.data()), GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sbo[1]);

        glUseProgram(program);

        glBindImageTexture(0, texture, 0, GL_FALSE, 0,  GL_READ_WRITE, GL_RGBA8);

        glUniform1i(glGetUniformLocation(program, "uTexture"), 0);

        // TODO: configure the source qt

        int quantize[64 * 3];

        for (int i = 0; i < 64; ++i)
        {
            quantize[i + 64 * 0] = input.qt[0][i];
        }

        for (int i = 0; i < 64; ++i)
        {
            quantize[i + 64 * 1] = input.qt[1][i];
        }

        for (int i = 0; i < 64; ++i)
        {
            quantize[i + 64 * 2] = input.qt[2][i];
        }

        glUniform1iv(glGetUniformLocation(program, "uQuantize0"), 64, quantize + 64 * 0);
        glUniform1iv(glGetUniformLocation(program, "uQuantize1"), 64, quantize + 64 * 1);
        glUniform1iv(glGetUniformLocation(program, "uQuantize2"), 64, quantize + 64 * 2);

        int huffman_dc [] =
        {
            input.blocks[0].dc,
            input.blocks[1].dc,
            input.blocks[2].dc,
        };

        int huffman_ac [] =
        {
            input.blocks[0].ac,
            input.blocks[1].ac,
            input.blocks[2].ac,
        };

        int huffman_pred [] =
        {
            input.blocks[0].pred,
            input.blocks[1].pred,
            input.blocks[2].pred,
        };

        glUniform1iv(glGetUniformLocation(program, "u_huffman_dc"), blocks_in_mcu, huffman_dc);
        glUniform1iv(glGetUniformLocation(program, "u_huffman_ac"), blocks_in_mcu, huffman_ac);
        glUniform1iv(glGetUniformLocation(program, "u_huffman_pred"), blocks_in_mcu, huffman_pred);

        glUniform1i(glGetUniformLocation(program, "u_xmcu"), input.xmcu);

        debugPrint("  Compute Segments: %d\n", int(input.intervals.size()));

        for (size_t i = 0; i < input.intervals.size(); ++i)
        {
            int y0 = input.intervals[i].y0;
            int y1 = input.intervals[i].y1;

            u32 offset = offsets[i];

            glUniform1i(glGetUniformLocation(program, "u_y0"), y0);
            glUniform1i(glGetUniformLocation(program, "u_y1"), y1);
            glUniform1i(glGetUniformLocation(program, "u_dataOffset"), offset / 4);

#if 0
            {
                std::vector<int> temp(64 * 3 * input.xmcu);
                cpu_decode_interval(temp.data(), buffer, huffmanBuffer, y0, y1, input.xmcu, offset / 4);

                glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbo[0]);
                glBufferData(GL_SHADER_STORAGE_BUFFER, temp.size() * 4, reinterpret_cast<GLvoid*>(temp.data()), GL_DYNAMIC_COPY);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sbo[0]);

                glDispatchCompute(1, 1, 1);
            }
#endif

            // .. compute ..
            glDispatchCompute(1, 1, 1);
        }

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

        glDeleteBuffers(2, sbo);

#if 0
        size_t total = 0;
        debugPrint("Intervals: %d\n", (int)input.intervals.size());
        for (auto interval : input.intervals)
        {
            total += interval.memory.size;
            debugPrint("  %d KB\n", int(interval.memory.size/1024));
        }
        debugPrint("Total: %d KB\n", int(total/1024));
#endif

        debugPrint("\n");
    }

    void send(const Surface& surface) override
    {
        // TODO: check dimensions match
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surface.width, surface.height, GL_RGBA, GL_UNSIGNED_BYTE, surface.image);
    }
};

} // namespace

namespace mango
{

OpenGLJPEGDecoder::OpenGLJPEGDecoder()
{
    GLuint compute = opengl::createShader(GL_COMPUTE_SHADER, compute_shader_source);
    program = glCreateProgram();

    glAttachShader(program, compute);
    glLinkProgram(program);

    opengl::getLinkStatus(program);
    glUseProgram(program);
}

OpenGLJPEGDecoder::~OpenGLJPEGDecoder()
{
    glDeleteProgram(program);
}

GLuint OpenGLJPEGDecoder::decode(ConstMemory memory)
{
    jpeg::Parser parser(memory);

    int width = parser.header.width;
    int height = parser.header.height;

    ComputeDecoderContext context;

    context.program = program;

    glGenTextures(1, &context.texture);
    glBindTexture(GL_TEXTURE_2D, context.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    u64 time0 = mango::Time::us();

    image::ImageDecodeOptions options;
    image::ImageDecodeStatus status = parser.decode(&context, options);

    u64 time1 = mango::Time::us();
    printf("  compute decode: %d.%d ms\n", int(time1 - time0)/1000,int(time1 - time0)%1000);

    return context.texture;
}

} // namespace mango

#endif // MANGO_OPENGL_JPEG

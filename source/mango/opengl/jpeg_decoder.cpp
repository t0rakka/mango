/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#include <mango/opengl/jpeg_decoder.hpp>
#include <mango/opengl/utilities.hpp>
#include "../jpeg/jpeg.hpp"

#ifdef MANGO_OPENGL_JPEG

// MANGO TODO: Huffman decoding in compute shader
// MANGO TODO: Different MCU configurations
// MANGO TODO: Progressive mode
// MANGO TODO: Different color formats (Chroma, Luminance)
// MANGO TODO: Arithmetic, CMYK, 12 bit DCT, 16 bit QT table, Lossless -> not supported

namespace
{

using namespace mango;
using namespace mango::image;
using namespace mango::image::jpeg;

static
const char* compute_shader_source = R"(
    #version 430 core

    layout(rgba8, binding = 0) uniform image2D u_texture;

    layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

    uniform int u_xmcu;

    uniform int u_quantize[3][64];

    //uniform int u_huffman_dc[10];
    //uniform int u_huffman_ac[10];
    //uniform int u_huffman_pred[10];

    layout(std430, binding = 0) buffer CompressedData
    {
        uint input_data [];
    };

    layout(std430, binding = 1) buffer CompressedDataOffsets
    {
        uint input_offsets [];
    };

    layout(std430, binding = 2) buffer HuffmanTables
    {
        uint huffman_tables [];
    };

    // --------------------------------------------------------------------

    const uint zigzagTable [] =
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

    // --------------------------------------------------------------------

    struct IDCT
    {
        int x0, x1, x2, x3;
        int y0, y1, y2, y3;
    };

    void idct_compute(inout IDCT idct, int s0, int s1, int s2, int s3, int s4, int s5, int s6, int s7)
    {
        const int n0 = (s2 + s6) * 2217;
        const int t2 = n0 + s6 * -7567;
        const int t3 = n0 + s2 * 3135;
        const int t0 = (s0 + s4) << 12;
        const int t1 = (s0 - s4) << 12;

        idct.x0 = t0 + t3;
        idct.x3 = t0 - t3;
        idct.x1 = t1 + t2;
        idct.x2 = t1 - t2;

        int p1 = s7 + s1;
        int p2 = s5 + s3;
        int p3 = s7 + s3;
        int p4 = s5 + s1;
        int p5 = (p3 + p4) * 4816;
        p1 = p1 * -3685 + p5;
        p2 = p2 * -10497 + p5;
        p3 = p3 * -8034;
        p4 = p4 * -1597;

        idct.y0 = p1 + p3 + s7 * 1223;
        idct.y1 = p2 + p4 + s5 * 8410;
        idct.y2 = p2 + p3 + s3 * 12586;
        idct.y3 = p1 + p4 + s1 * 6149;
    }

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
            idct_compute(idct, s0, s1, s2, s3, s4, s5, s6, s7);

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
            idct_compute(idct,
                temp[i +  0], temp[i +  8], temp[i + 16], temp[i + 24],
                temp[i + 32], temp[i + 40], temp[i + 48], temp[i + 56]);

            const int bias = 0x10000 + (128 << 17);
            idct.x0 += bias;
            idct.x1 += bias;
            idct.x2 += bias;
            idct.x3 += bias;

            dest[i * 8 + 0] = (idct.x0 + idct.y3) >> 17;
            dest[i * 8 + 1] = (idct.x1 + idct.y2) >> 17;
            dest[i * 8 + 2] = (idct.x2 + idct.y1) >> 17;
            dest[i * 8 + 3] = (idct.x3 + idct.y0) >> 17;
            dest[i * 8 + 4] = (idct.x3 - idct.y0) >> 17;
            dest[i * 8 + 5] = (idct.x2 - idct.y1) >> 17;
            dest[i * 8 + 6] = (idct.x1 - idct.y2) >> 17;
            dest[i * 8 + 7] = (idct.x0 - idct.y3) >> 17;
        }
    }

    // --------------------------------------------------------------------

    struct BitBuffer
    {
        uint offset;
        uint used;

        uint data;
        uint remain;
    };

    uint getByte(inout BitBuffer bitbuffer)
    {
        if (bitbuffer.used == 32)
        {
            bitbuffer.used = 0;
            ++bitbuffer.offset;
        }

        uint x = (input_data[bitbuffer.offset] >> bitbuffer.used) & 0xff;
        bitbuffer.used += 8;

        return x;
    }

    uint peekBits(inout BitBuffer bitbuffer, uint nbits)
    {
        return (bitbuffer.data >> (bitbuffer.remain - nbits)) & ((1 << nbits) - 1);
    }

    void ensure(inout BitBuffer bitbuffer)
    {
        while (bitbuffer.remain < 16)
        {
            bitbuffer.remain += 8;
            uint x = getByte(bitbuffer);
            if (x == 0xff)
            {
                // skip stuff byte
                getByte(bitbuffer); // filtered out before uploaded
            }
            bitbuffer.data = (bitbuffer.data << 8) | x;
        }
    }

    uint receive(inout BitBuffer bitbuffer, uint nbits)
    {
        ensure(bitbuffer);
        bitbuffer.remain -= nbits;
        uint mask = (1 << nbits) - 1;
        int value = int((bitbuffer.data >> bitbuffer.remain) & mask);
        return uint(value - ((((value + value) >> nbits) - 1) & mask));
    }

    // --------------------------------------------------------------------

    struct DecodeBlock
    {
        int dc;
        int ac;
        int pred;
    };

    struct HuffmanTable
    {
        uint size[17];
        uint value[256];

        uint maxcode[18];
        uint valueOffset[19];
    };

    void huffman_configure(inout HuffmanTable table)
    {
        uint huffsize[257];
        uint huffcode[257];

        // Figure C.1: make table of Huffman code length for each symbol
        uint p = 0;
        for (int j = 1; j <= 16; ++j)
        {
            int count = int(table.size[j]);
            while (count-- > 0)
            {
                huffsize[p++] = uint(j);
            }
        }
        huffsize[p] = 0;

        // Figure C.2: generate the codes themselves
        uint code = 0;
        uint si = huffsize[0];
        p = 0;
        while (huffsize[p] != 0)
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
            if (table.size[j] != 0)
            {
                table.valueOffset[j] = p - int(huffcode[p]);
                p += table.size[j];
                table.maxcode[j] = huffcode[p - 1];
                table.maxcode[j] <<= (32 - j);
                table.maxcode[j] |= (1 << (32 - j)) - 1;
            }
            else
            {
                table.maxcode[j] = 0;
            }
        }
        table.valueOffset[18] = 0;
        table.maxcode[17] = 0xffffffff;
    }

    HuffmanTable g_tables[4];

    uint decode(inout BitBuffer bitbuffer, int tableIndex)
    {
        ensure(bitbuffer);

        uint size = 2;

        uint x = (bitbuffer.data << (32 - bitbuffer.remain));
        while (x > g_tables[tableIndex].maxcode[size])
        {
            ++size;
        }

        uint offset = (x >> (32 - size)) + g_tables[tableIndex].valueOffset[size];
        uint symbol = g_tables[tableIndex].value[offset];

        bitbuffer.remain -= size;

        return symbol;
    }

    // --------------------------------------------------------------------

    vec4 chroma_to_rgb(float y, float cb, float cr)
    {
        float r = y + cr * 1.400;
        float g = y - cb * 0.343 - cr * 0.711;
        float b = y + cb * 1.765;
        return vec4(r, g, b, 1.0);
    }

    // --------------------------------------------------------------------

    void main()
    {
        BitBuffer bitbuffer;

        bitbuffer.offset = input_offsets[gl_GlobalInvocationID.y];
        bitbuffer.used = 0;
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

        for (int i = 0; i < 4; ++i)
        {
            int base = i * 273;

            for (int j = 0; j < 17; ++j)
            {
                g_tables[i].size[j] = huffman_tables[base++];
            }

            for (int j = 0; j < 256; ++j)
            {
                g_tables[i].value[j] = huffman_tables[base++];
            }

            // NOTE: configuring this per interval is expensive but we'll use this for quick start
            huffman_configure(g_tables[i]);
        }

        uint last_dc_value[3];

        for (int i = 0; i < 3; ++i)
        {
            last_dc_value[i] = 0;
        }

        // -------------------------------------------------------------------------------

        for (int mcu_x = 0; mcu_x < u_xmcu; ++mcu_x)
        {
            int result[3][64];

            for (int blk = 0; blk < 3; ++blk)
            {
                int dc = decodeBlocks[blk].dc;
                int ac = decodeBlocks[blk].ac;
                int pred = decodeBlocks[blk].pred;

                int temp[64];

                for (int i = 0; i < 64; ++i)
                {
                    temp[i] = 0;
                }

                // DC
                uint s = decode(bitbuffer, dc);
                if (s != 0)
                {
                    s = receive(bitbuffer, s);
                }

                s += last_dc_value[pred];
                last_dc_value[pred] = s;

                temp[0] = int(s);

                // AC
                for (int i = 1; i < 64; )
                {
                    uint s = decode(bitbuffer, ac);
                    uint x = s & 15;

                    if (x != 0)
                    {
                        i += int(s >> 4);
                        s = receive(bitbuffer, x);
                        temp[zigzagTable[i++]] = int(s);
                    }
                    else
                    {
                        if (s < 16)
                            break;
                        i += 16;
                    }
                }

                // inverse DCT
                idct(result[blk], temp, u_quantize[blk]);
            }

            // resolve color

            ivec2 blockCoord = ivec2(mcu_x * 8, gl_GlobalInvocationID.y * 8);

            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    float Y  = float(result[0][y * 8 + x]) / 255.0;
                    float cb = float(result[1][y * 8 + x] - 128) / 255.0;
                    float cr = float(result[2][y * 8 + x] - 128) / 255.0;
                    vec4 color = chroma_to_rgb(Y, cb, cr);
                    ivec2 coord = blockCoord + ivec2(x, y);
                    imageStore(u_texture, coord, color);
                }
            }
        }
    }
)";

// ---------------------------------------------------------------------------------

struct ComputeDecoderContext : jpeg::ComputeDecoder
{
    GLuint program = 0;
    GLuint texture = 0;

    void send(const ComputeDecoderInput& input) override
    {
        printLine(Print::Info, "\n[ComputeDecode]");
        printLine(Print::Info, "  MCU: {} x {}.", input.xmcu, input.ymcu);

        Buffer buffer;
        std::vector<u32> offsets;

#if 1
        for (auto interval : input.intervals)
        {
            ConstMemory memory = interval.memory;

            offsets.push_back(u32(buffer.size() / 4));

            size_t padding = align_padding(memory.size, 4);

            buffer.append(memory);
            buffer.append(padding, 0);
        }
#else
        for (auto interval : input.intervals)
        {
            std::vector<u8> temp;

            // sanitize markers and stuff bytes

            const u8* p = interval.memory.address;
            const u8* end = interval.memory.end();

            while (p < end)
            {
                u8 s = *p++;
                p += !!(s == 0xff);
                temp.push_back(s);
            }

            offsets.push_back(u32(buffer.size() / 4));

            size_t padding = align_padding(temp.size(), 4);

            buffer.append(temp.data(), temp.size());
            buffer.append(padding, 0);
        }
#endif

        printf("  Buffer: %d bytes\n", u32(buffer.size()));

        //GLsizei blocks_in_mcu = GLsizei(input.blocks.size());

        std::vector<int> huffmanBuffer(273 * 4);

        for (int i = 0; i < 4; ++i)
        {
            int* dest = huffmanBuffer.data() + i * 273;
            const HuffmanTable& source = input.huffman.table[i & 1][i >> 1];

            for (int j = 0; j < 17; ++j)
            {
                *dest++ = source.size[j];
            }

            for (int j = 0; j < 256; ++j)
            {
                *dest++ = source.value[j];
            }
        }

        GLuint sbo[3];
        glGenBuffers(3, sbo);

#if 1
        // upload compressed bitstream
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbo[0]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, buffer.size(), reinterpret_cast<GLvoid*>(buffer.data()), GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sbo[0]);

        // upload offset tables
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbo[1]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, offsets.size() * 4, reinterpret_cast<GLvoid*>(offsets.data()), GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sbo[1]);

        // upload huffman tables
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbo[2]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, huffmanBuffer.size() * 4, reinterpret_cast<GLvoid*>(huffmanBuffer.data()), GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sbo[2]);
#endif

        glUseProgram(program);

        glBindImageTexture(0, texture, 0, GL_FALSE, 0,  GL_READ_WRITE, GL_RGBA8);

        glUniform1i(glGetUniformLocation(program, "u_texture"), 0);

        // MANGO TODO: configure the source qt

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

        glUniform1iv(glGetUniformLocation(program, "u_quantize[0]"), 64, quantize + 64 * 0);
        glUniform1iv(glGetUniformLocation(program, "u_quantize[1]"), 64, quantize + 64 * 1);
        glUniform1iv(glGetUniformLocation(program, "u_quantize[2]"), 64, quantize + 64 * 2);

#if 0
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
#endif

        glUniform1i(glGetUniformLocation(program, "u_xmcu"), input.xmcu);

        printLine(Print::Info, "  Compute Segments: {}", input.intervals.size());

        /*
        for (size_t i = 0; i < input.intervals.size(); ++i)
        {
            int y0 = input.intervals[i].y0;
            int y1 = input.intervals[i].y1;

            u32 offset = offsets[i];

            glUniform1i(glGetUniformLocation(program, "u_y0"), y0);
            glUniform1i(glGetUniformLocation(program, "u_y1"), y1);
            //glUniform1i(glGetUniformLocation(program, "u_data_offset"), offset / 4);

#if 0
            {
                std::vector<int> temp(64 * 3 * input.xmcu);
                cpu_decode_interval(temp.data(), buffer, huffmanBuffer, y0, y1, input.xmcu, offset / 4);

                glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbo[0]);
                glBufferData(GL_SHADER_STORAGE_BUFFER, temp.size() * 4, reinterpret_cast<GLvoid*>(temp.data()), GL_DYNAMIC_COPY);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sbo[0]);
            }
#endif

            // .. compute ..
            glDispatchCompute(1, 1, 1);
        }
        */

        glDispatchCompute(1, GLuint(input.intervals.size()), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

        glDeleteBuffers(3, sbo);

        /*
        size_t total = 0;
        printLine(Print::Info, "Intervals: {}", input.intervals.size());
        for (auto interval : input.intervals)
        {
            total += interval.memory.size;
            printLine(Print::Info, "  {} KB", interval.memory.size / 1024);
        }
        printLine(Print::Info, "Total: {} KB", total/1024);
        */

        printLine(Print::Info, "");
    }

    void send(const Surface& surface) override
    {
        // MANGO TODO: check dimensions match
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

    //Buffer temp(width * height * 4, 128);

    glGenTextures(1, &context.texture);
    glBindTexture(GL_TEXTURE_2D, context.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    u64 time0 = mango::Time::us();

    image::ImageDecodeOptions options;
    image::ImageDecodeStatus status = parser.decode(&context, options);

    u64 time1 = mango::Time::us();
    printLine(Print::Info, "  compute decode: {}.{} ms", (time1 - time0) / 1000, (time1 - time0) % 1000);

    return context.texture;
}

} // namespace mango

#endif // MANGO_OPENGL_JPEG

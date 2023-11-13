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

    layout(std430, binding = 0) buffer input_layout
    {
        int input[];
    };

    layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

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

    void main()
    {
        uint idx = gl_GlobalInvocationID.y * gl_NumWorkGroups.x + gl_GlobalInvocationID.x;
        idx = idx * 3 * 32;

        // convert packed s16 into s32

        int data0[64];
        int data1[64];
        int data2[64];

        for (int i = 0; i < 32; ++i)
        {
            int d0 = input[idx + i + 32 * 0];
            int d1 = input[idx + i + 32 * 1];
            int d2 = input[idx + i + 32 * 2];

            data0[i * 2 + 0] = (d0 << 16) >> 16;
            data0[i * 2 + 1] = d0 >> 16;
            data1[i * 2 + 0] = (d1 << 16) >> 16;
            data1[i * 2 + 1] = d1 >> 16;
            data2[i * 2 + 0] = (d2 << 16) >> 16;
            data2[i * 2 + 1] = d2 >> 16;
        }

        // inverse DCT

        int lu[64];
        int cb[64];
        int cr[64];

        idct(lu, data0, uQuantize0);
        idct(cb, data1, uQuantize1);
        idct(cr, data2, uQuantize2);

        // resolve color

        ivec2 blockCoord = ivec2(gl_GlobalInvocationID.xy * 8);

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
)";

struct ComputeDecoderContext : ComputeDecoder
{
    GLuint program = 0;
    GLuint texture = 0;

    void send(const ComputeDecoderInput& input) override
    {

        Buffer buffer;

        for (auto interval : input.intervals)
        {
            size_t padding = align_padding(interval.memory.size, 4);
            buffer.append(interval.memory);
            buffer.append(padding, 0);
        }


        /*
        size_t blocks_in_mcu = 3; // TODO

        size_t elements = input.ymcu * input.xmcu * blocks_in_mcu * 64;
        size_t bytes = elements * sizeof(s16);

        GLuint sbo;
        glGenBuffers(1, &sbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, bytes, reinterpret_cast<GLvoid*>(input.data), GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sbo);

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

        glDispatchCompute(input.xmcu, input.ymcu, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

        glDeleteBuffers(1, &sbo);
        */

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

        debugPrint("\n[ComputeDecode]\n");
        debugPrint("  MCU: %d x %d.\n\n", input.xmcu, input.ymcu);
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

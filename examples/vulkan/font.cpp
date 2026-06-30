/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.

    Scanline Sweeper glyph rendering prototype (Vulkan).
    https://rookandpossum.com/papers/scanline_sweeper_preprint.pdf
*/
#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <array>
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>

#define MANGO_IMPLEMENT_MAIN
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <mango/core/core.hpp>
#include <mango/math/math.hpp>
#include <mango/filesystem/filesystem.hpp>
#include <mango/vulkan/vulkan.hpp>
#include <mango/vulkan/allocator.hpp>

using namespace mango;
using namespace mango::math;
using namespace mango::vulkan;

// =============================================================================
// mango::font — backend-agnostic glyph preprocessing (no Vulkan)
// =============================================================================

namespace mango::font
{

    using float2 = float32x2;

    struct QuadraticCurve
    {
        float2 p0;
        float2 p1;
        float2 p2;
    };

    struct Contour
    {
        std::vector<QuadraticCurve> curves;
    };

    struct GlyphOutline
    {
        float advance = 0.0f;
        float2 bounds_min { 0.0f, 0.0f };
        float2 bounds_max { 0.0f, 0.0f };
        std::vector<Contour> contours;
    };

    struct GpuContourHeader
    {
        u32 curve_offset = 0;
        u32 curve_count = 0;
    };

    struct GlyphGpuData
    {
        float advance = 0.0f;
        float2 bounds_min { 0.0f, 0.0f };
        float2 bounds_max { 0.0f, 0.0f };
        std::vector<GpuContourHeader> contours;
        std::vector<float> curve_data;
        std::vector<u8> contour_bytes;
        std::vector<u8> curve_bytes;
    };

    static constexpr float kEpsilon = 1e-4f;

    static float2 evaluate_cubic(const float2& p0, const float2& p1, const float2& p2, const float2& p3, float t)
    {
        float s = 1.0f - t;
        float s2 = s * s;
        float s3 = s2 * s;
        float t2 = t * t;
        float t3 = t2 * t;
        return p0 * s3 + p1 * (3.0f * s2 * t) + p2 * (3.0f * s * t2) + p3 * t3;
    }

    static bool is_monotonic_axis(float c0, float c1, float c2)
    {
        float qa = c0 + c2 - 2.0f * c1;
        if (std::fabs(qa) < 1e-3f)
        {
            return true;
        }

        // Extremum parameter; monotonic on [0, 1] when t is outside the open unit interval.
        float t = (c0 - c1) / qa;
        return t <= kEpsilon || t >= 1.0f - kEpsilon;
    }

    static bool is_monotonic(const QuadraticCurve& curve)
    {
        return is_monotonic_axis(curve.p0.x, curve.p1.x, curve.p2.x) &&
               is_monotonic_axis(curve.p0.y, curve.p1.y, curve.p2.y);
    }

    static void subdivide_quadratic(const QuadraticCurve& curve, QuadraticCurve& left, QuadraticCurve& right)
    {
        float2 p01 = (curve.p0 + curve.p1) * 0.5f;
        float2 p12 = (curve.p1 + curve.p2) * 0.5f;
        float2 p012 = (p01 + p12) * 0.5f;

        left = { curve.p0, p01, p012 };
        right = { p012, p12, curve.p2 };
    }

    static void make_monotonic(QuadraticCurve curve, std::vector<QuadraticCurve>& output, int depth = 0)
    {
        if (is_monotonic(curve))
        {
            output.push_back(curve);
            return;
        }

        // Paper §3.1: subdivide until x- and y-monotonic; never emit non-monotonic curves.
        QuadraticCurve left, right;
        subdivide_quadratic(curve, left, right);
        make_monotonic(left, output, depth + 1);
        make_monotonic(right, output, depth + 1);
    }

    static void append_curve(Contour& contour, const QuadraticCurve& curve)
    {
        make_monotonic(curve, contour.curves);
    }

    static QuadraticCurve line_to_quadratic(const float2& p0, const float2& p1)
    {
        return { p0, (p0 + p1) * 0.5f, p1 };
    }

    static bool is_horizontal_line(const float2& p0, const float2& p1)
    {
        return std::fabs(p0.y - p1.y) < kEpsilon && std::fabs(p0.x - p1.x) > kEpsilon;
    }

    static void cubic_to_quadratics(const float2& p0, const float2& p1, const float2& p2, const float2& p3,
                                    std::vector<QuadraticCurve>& output, float tolerance = 0.5f, int depth = 0)
    {
        float2 mid = evaluate_cubic(p0, p1, p2, p3, 0.5f);
        float2 chord_mid = (p0 + p3) * 0.5f;
        float error = length(mid - chord_mid);

        if (error <= tolerance || depth >= 6)
        {
            float2 control = (4.0f * mid - p0 - p3) * 0.5f;
            output.push_back({ p0, control, p3 });
            return;
        }

        float2 p01 = (p0 + p1) * 0.5f;
        float2 p12 = (p1 + p2) * 0.5f;
        float2 p23 = (p2 + p3) * 0.5f;
        float2 p012 = (p01 + p12) * 0.5f;
        float2 p123 = (p12 + p23) * 0.5f;
        float2 p0123 = (p012 + p123) * 0.5f;

        cubic_to_quadratics(p0, p01, p012, p0123, output, tolerance, depth + 1);
        cubic_to_quadratics(p0123, p123, p23, p3, output, tolerance, depth + 1);
    }

    static void pack_for_gpu(const GlyphOutline& outline, GlyphGpuData& gpu)
    {
        gpu.advance = outline.advance;
        gpu.bounds_min = outline.bounds_min;
        gpu.bounds_max = outline.bounds_max;
        gpu.contours.clear();
        gpu.curve_data.clear();
        gpu.contour_bytes.clear();
        gpu.curve_bytes.clear();

        u32 curve_offset = 0;
        for (const Contour& contour : outline.contours)
        {
            GpuContourHeader header;
            header.curve_offset = curve_offset;
            header.curve_count = u32(contour.curves.size());
            gpu.contours.push_back(header);

            for (const QuadraticCurve& curve : contour.curves)
            {
                gpu.curve_data.push_back(curve.p0.x);
                gpu.curve_data.push_back(curve.p0.y);
                gpu.curve_data.push_back(curve.p1.x);
                gpu.curve_data.push_back(curve.p1.y);
                gpu.curve_data.push_back(curve.p2.x);
                gpu.curve_data.push_back(curve.p2.y);
            }

            curve_offset += header.curve_count;
        }

        gpu.contour_bytes.resize(gpu.contours.size() * sizeof(GpuContourHeader));
        if (!gpu.contour_bytes.empty())
        {
            std::memcpy(gpu.contour_bytes.data(), gpu.contours.data(), gpu.contour_bytes.size());
        }

        gpu.curve_bytes.resize(gpu.curve_data.size() * sizeof(float));
        if (!gpu.curve_bytes.empty())
        {
            std::memcpy(gpu.curve_bytes.data(), gpu.curve_data.data(), gpu.curve_bytes.size());
        }
    }

    static void update_bounds(GlyphOutline& outline, const float2& point, bool& initialized)
    {
        if (!initialized)
        {
            outline.bounds_min = point;
            outline.bounds_max = point;
            initialized = true;
            return;
        }

        outline.bounds_min = min(outline.bounds_min, point);
        outline.bounds_max = max(outline.bounds_max, point);
    }

    static void process_shape(const stbtt_vertex* vertices, int count, GlyphOutline& outline)
    {
        Contour contour;
        float2 current { 0.0f, 0.0f };
        bool has_current = false;
        bool bounds_initialized = false;

        auto flush_contour = [&]()
        {
            if (!contour.curves.empty())
            {
                outline.contours.push_back(std::move(contour));
                contour = Contour {};
            }
        };

        for (int i = 0; i < count; ++i)
        {
            const stbtt_vertex& v = vertices[i];
            float2 next(float(v.x), float(v.y));

            switch (v.type)
            {
                case STBTT_vmove:
                    flush_contour();
                    current = next;
                    has_current = true;
                    update_bounds(outline, current, bounds_initialized);
                    break;

                case STBTT_vline:
                {
                    if (!has_current)
                    {
                        break;
                    }

                    if (!is_horizontal_line(current, next))
                    {
                        append_curve(contour, line_to_quadratic(current, next));
                    }

                    current = next;
                    update_bounds(outline, current, bounds_initialized);
                    break;
                }

                case STBTT_vcurve:
                {
                    if (!has_current)
                    {
                        break;
                    }

                    float2 control(float(v.cx), float(v.cy));
                    append_curve(contour, { current, control, next });

                    current = next;
                    update_bounds(outline, current, bounds_initialized);
                    update_bounds(outline, control, bounds_initialized);
                    break;
                }

                case STBTT_vcubic:
                {
                    if (!has_current)
                    {
                        break;
                    }

                    float2 c1(float(v.cx), float(v.cy));
                    float2 c2(float(v.cx1), float(v.cy1));
                    std::vector<QuadraticCurve> quadratics;
                    cubic_to_quadratics(current, c1, c2, next, quadratics);
                    for (const QuadraticCurve& q : quadratics)
                    {
                        append_curve(contour, q);
                    }

                    current = next;
                    update_bounds(outline, current, bounds_initialized);
                    update_bounds(outline, c1, bounds_initialized);
                    update_bounds(outline, c2, bounds_initialized);
                    break;
                }
            }
        }

        flush_contour();
    }

    class Font
    {
    public:
        stbtt_fontinfo info {};
        std::vector<u8> data;
        float scale = 1.0f;
        int ascent = 0;
        int descent = 0;
        int line_gap = 0;

        bool load(const std::string& path, float pixel_height)
        {
            filesystem::File file(path);
            if (!file.size())
            {
                return false;
            }

            const u8* ptr = file.data();
            size_t size = size_t(file.size());
            data.assign(ptr, ptr + size);

            if (!stbtt_InitFont(&info, data.data(), stbtt_GetFontOffsetForIndex(data.data(), 0)))
            {
                return false;
            }

            scale = stbtt_ScaleForPixelHeight(&info, pixel_height);
            stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
            return true;
        }

        void set_pixel_height(float pixel_height)
        {
            scale = stbtt_ScaleForPixelHeight(&info, pixel_height);
        }

        GlyphOutline load_outline(int codepoint) const
        {
            GlyphOutline outline;

            int advance = 0;
            stbtt_GetCodepointHMetrics(&info, codepoint, &advance, nullptr);
            outline.advance = float(advance);

            stbtt_vertex* vertices = nullptr;
            int count = stbtt_GetCodepointShape(&info, codepoint, &vertices);
            if (count > 0 && vertices)
            {
                process_shape(vertices, count, outline);
                stbtt_FreeShape(&info, vertices);
            }

            return outline;
        }

        GlyphGpuData load_gpu_glyph(int codepoint) const
        {
            GlyphOutline outline = load_outline(codepoint);
            GlyphGpuData gpu;
            pack_for_gpu(outline, gpu);
            return gpu;
        }

        float em_per_pixel() const
        {
            return 1.0f / scale;
        }

        float pixel_scale() const
        {
            return scale;
        }
    };

} // namespace mango::font

// =============================================================================
// GLSL shaders
// =============================================================================

namespace
{

const char* g_computeShader = R"(#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(push_constant) uniform PerGlyph
{
    vec4 color;
    vec4 params0; // xy = em-space lower-left of pixel (0,0), zw = em_per_pixel
    vec4 params1; // xy = canvas pixel origin, zw = dispatch size in pixels
    uint contour_count;
} pc;

layout(std430, set = 1, binding = 0) readonly buffer ContourBuffer
{
    uvec2 contours[];
};

layout(std430, set = 1, binding = 1) readonly buffer CurveBuffer
{
    float points[];
};

layout(set = 0, binding = 0, rgba8) uniform image2D canvas;

vec2 read_curve_point(uint curve_index, uint point_index)
{
    uint base = curve_index * 6u + point_index * 2u;
    return vec2(points[base + 0u], points[base + 1u]);
}

// --- Scanline Sweeper appendix 8.1 / 8.2 / 8.3 (GLSL port of HLSL golden model) ---

vec2 evaluate_bezier(vec2 p0, vec2 p1, vec2 p2, float t)
{
    vec2 a = mix(p0, p1, t);
    vec2 b = mix(p1, p2, t);
    return mix(a, b, t);
}

float intersect_monotonic(float qa, float c0, float c1, float c2, float target)
{
    if (abs(qa) < 1e-3)
    {
        return (target - c0) / (c2 - c0);
    }

    float qb = 2.0 * c1 - 2.0 * c0;
    float qc = c0 - target;
    float d = qb * qb - 4.0 * qa * qc;
    float sqrt_d = d < 0.0 ? 0.0 : sqrt(d);
    float inv_2a = 0.5 / qa;
    return -qb * inv_2a + sign(c2 - c0) * sqrt_d * inv_2a;
}

float scanline_sweep(vec2 size, vec2 offset, vec2 p0, vec2 p1, vec2 p2)
{
    if (max(p0.y, p2.y) <= offset.y || min(p0.y, p2.y) >= offset.y + size.y)
    {
        return 0.0;
    }

    vec2 delta = p2 - p0;
    p0 -= offset;
    p1 -= offset;
    p2 -= offset;

    if (p0.x == p1.x && p0.x == p2.x)
    {
        if (p0.x >= size.x)
        {
            return 0.0;
        }

        float t = min(max(p0.y, p2.y), size.y);
        float b = max(min(p0.y, p2.y), 0.0);
        float h = t - b;
        float w = min(size.x, size.x - p0.x);
        return sign(delta.y) * w * h;
    }

    float qa = -2.0 * p1.y + p0.y + p2.y;
    float bt = intersect_monotonic(qa, p0.y, p1.y, p2.y, 0.0);
    float tt = intersect_monotonic(qa, p0.y, p1.y, p2.y, size.y);
    float v_min_t = delta.y > 0.0 ? bt : tt;
    float v_max_t = delta.y > 0.0 ? tt : bt;
    vec2 v_min = evaluate_bezier(p0, p1, p2, clamp(v_min_t, 0.0, 1.0));
    vec2 v_max = evaluate_bezier(p0, p1, p2, clamp(v_max_t, 0.0, 1.0));

    if (max(v_min.x, v_max.x) <= 0.0)
    {
        return (v_max.y - v_min.y) * size.x;
    }

    if (min(v_min.x, v_max.x) >= size.x)
    {
        return 0.0;
    }

    qa = -2.0 * p1.x + p0.x + p2.x;
    float h_min_t;
    float h_max_t;
    vec4 h_check = delta.x > 0.0
        ? vec4(p0.x, p2.x, 0.0, 0.0)
        : vec4(p2.x, p0.x, size.x, 1.0);

    if (h_check.x >= h_check.z)
        h_min_t = h_check.w;
    else if (h_check.y <= h_check.z)
        h_min_t = 1.0 - h_check.w;
    else
        h_min_t = intersect_monotonic(qa, p0.x, p1.x, p2.x, h_check.z);

    h_check.z = size.x - h_check.z;

    if (h_check.x >= h_check.z)
        h_max_t = h_check.w;
    else if (h_check.y <= h_check.z)
        h_max_t = 1.0 - h_check.w;
    else
        h_max_t = intersect_monotonic(qa, p0.x, p1.x, p2.x, h_check.z);

    float min_t = clamp(max(v_min_t, h_min_t), 0.0, 1.0);
    float max_t = clamp(min(v_max_t, h_max_t), 0.0, 1.0);

    vec2 q0 = v_min_t >= h_min_t ? v_min : evaluate_bezier(p0, p1, p2, min_t);
    vec2 q1 = v_max_t <= h_max_t ? v_max : evaluate_bezier(p0, p1, p2, max_t);

    float coverage = 0.0;

    if (min_t > 0.0 && delta.x > 0.0)
    {
        float h = delta.y > 0.0 ? q0.y - max(0.0, p0.y) : min(size.y, p0.y) - q0.y;
        coverage = sign(delta.y) * h * size.x;
    }

    if (max_t < 1.0 && delta.x < 0.0)
    {
        float h = delta.y > 0.0 ? min(size.y, p2.y) - q1.y : q1.y - max(0.0, p2.y);
        coverage += sign(delta.y) * h * size.x;
    }

    float h = q1.y - q0.y;
    float b = -0.5 * (q0.x + q1.x) + size.x;
    coverage += b * h;
    return coverage;
}

// --- implementer glue (section 8 intro; not part of appendix 8.1-8.3) ---

void main()
{
    vec2 pixel_size = pc.params1.zw;
    uvec2 gid = gl_GlobalInvocationID.xy;
    if (gid.x >= uint(pixel_size.x) || gid.y >= uint(pixel_size.y))
    {
        return;
    }

    vec2 em_per_pixel = pc.params0.zw;
    vec2 em_offset = vec2(
        pc.params0.x + float(gid.x) * em_per_pixel.x,
        pc.params0.y - (float(gid.y) + 1.0) * em_per_pixel.y);
    vec2 window_size = em_per_pixel;
    float area = window_size.x * window_size.y;

    float coverage_sum = 0.0;

    for (uint ci = 0u; ci < pc.contour_count; ++ci)
    {
        uint curve_offset = contours[ci].x;
        uint curve_count = contours[ci].y;

        for (uint i = 0u; i < curve_count; ++i)
        {
            uint idx = curve_offset + i;
            vec2 p0 = read_curve_point(idx, 0u);
            vec2 p1 = read_curve_point(idx, 1u);
            vec2 p2 = read_curve_point(idx, 2u);

            // §3.2: cull by scanline and right window edge only. Curves entirely left
            // of the window still contribute rectangular sweep area inside scanline_sweep.
            if (max(p0.y, p2.y) <= em_offset.y || min(p0.y, p2.y) >= em_offset.y + window_size.y)
            {
                continue;
            }
            if (min(p0.x, p2.x) > em_offset.x + window_size.x)
            {
                continue;
            }

            coverage_sum += scanline_sweep(window_size, em_offset, p0, p1, p2);
        }
    }

    float alpha = area > 0.0 ? coverage_sum / area : 0.0;
    alpha *= pc.color.a;

    if (alpha <= 0.0)
    {
        return;
    }

    ivec2 pixel = ivec2(floor(pc.params1.xy)) + ivec2(gid);
    ivec2 canvas_size = imageSize(canvas);
    if (pixel.x < 0 || pixel.y < 0 || pixel.x >= canvas_size.x || pixel.y >= canvas_size.y)
    {
        return;
    }

    vec4 dst = imageLoad(canvas, pixel);
    vec3 out_rgb = mix(dst.rgb, pc.color.rgb, alpha);
    imageStore(canvas, pixel, vec4(out_rgb, 1.0));
}
)";

const char* g_blitVertexShader = R"(#version 450
layout(location = 0) out vec2 vTexcoord;

vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

vec2 texcoords[3] = vec2[](
    vec2(0.0, 0.0),
    vec2(2.0, 0.0),
    vec2(0.0, 2.0)
);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    vTexcoord = texcoords[gl_VertexIndex];
}
)";

const char* g_blitFragmentShader = R"(#version 450
layout(location = 0) in vec2 vTexcoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uTexture;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    ivec2 size = textureSize(uTexture, 0);
    if (pixel.x >= size.x || pixel.y >= size.y)
    {
        discard;
    }

    outColor = texelFetch(uTexture, pixel, 0);
}
)";

} // namespace

// =============================================================================
// mango::vulkan::font — Vulkan backend
// =============================================================================

namespace mango::vulkan::font
{

    struct GlyphGpuBuffers
    {
        vulkan::BufferAllocation contours;
        vulkan::BufferAllocation curves;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };

    struct GlyphPushConstants
    {
        float color[4];
        float params0[4];
        float params1[4];
        u32 contour_count;
    };

    struct GlyphDrawParams
    {
        float screen_x = 0.0f;
        float screen_y = 0.0f;
        float em_window_x0 = 0.0f;
        float em_window_y_top = 0.0f;
        float em_per_pixel = 1.0f;
        u32 pixel_width = 1;
        u32 pixel_height = 1;
        float32x4 color { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    class Renderer
    {
    public:
        Renderer(VkDevice device, VkQueue queue, u32 queueFamily, vulkan::Allocator& allocator)
            : m_device(device)
            , m_queue(queue)
            , m_queueFamily(queueFamily)
            , m_allocator(allocator)
        {
            createDescriptorSetLayouts();
            createPipelines();
            createSampler();
        }

        ~Renderer()
        {
            if (m_device == VK_NULL_HANDLE)
            {
                return;
            }

            vkDeviceWaitIdle(m_device);

            destroyCanvas();

            if (m_sampler)
            {
                vkDestroySampler(m_device, m_sampler, nullptr);
            }

            if (m_blitPipeline)
            {
                vkDestroyPipeline(m_device, m_blitPipeline, nullptr);
            }

            if (m_computePipeline)
            {
                vkDestroyPipeline(m_device, m_computePipeline, nullptr);
            }

            if (m_blitPipelineLayout)
            {
                vkDestroyPipelineLayout(m_device, m_blitPipelineLayout, nullptr);
            }

            if (m_computePipelineLayout)
            {
                vkDestroyPipelineLayout(m_device, m_computePipelineLayout, nullptr);
            }

            if (m_glyphDescriptorPool)
            {
                vkDestroyDescriptorPool(m_device, m_glyphDescriptorPool, nullptr);
            }

            if (m_canvasDescriptorPool)
            {
                vkDestroyDescriptorPool(m_device, m_canvasDescriptorPool, nullptr);
            }

            if (m_blitDescriptorPool)
            {
                vkDestroyDescriptorPool(m_device, m_blitDescriptorPool, nullptr);
            }

            if (m_blitDescriptorSetLayout)
            {
                vkDestroyDescriptorSetLayout(m_device, m_blitDescriptorSetLayout, nullptr);
            }

            if (m_glyphDescriptorSetLayout)
            {
                vkDestroyDescriptorSetLayout(m_device, m_glyphDescriptorSetLayout, nullptr);
            }

            if (m_canvasDescriptorSetLayout)
            {
                vkDestroyDescriptorSetLayout(m_device, m_canvasDescriptorSetLayout, nullptr);
            }

            if (m_computeShader)
            {
                vkDestroyShaderModule(m_device, m_computeShader, nullptr);
            }

            if (m_blitVertexShader)
            {
                vkDestroyShaderModule(m_device, m_blitVertexShader, nullptr);
            }

            if (m_blitFragmentShader)
            {
                vkDestroyShaderModule(m_device, m_blitFragmentShader, nullptr);
            }
        }

        void resize(VkExtent2D extent)
        {
            if (extent.width == 0 || extent.height == 0)
            {
                return;
            }

            if (extent.width == m_extent.width && extent.height == m_extent.height && m_canvasImage)
            {
                return;
            }

            vkDeviceWaitIdle(m_device);
            destroyCanvas();
            m_extent = extent;
            createCanvas();
        }

        void clear_canvas(VkCommandBuffer cmd, float r, float g, float b, float a)
        {
            if (!m_canvasImage)
            {
                return;
            }

            VkImageMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .image = m_canvasImage,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };

            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkClearColorValue clear = {{ r, g, b, a }};
            VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            vkCmdClearColorImage(cmd, m_canvasImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);

            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier);
        }

        void draw_glyph(VkCommandBuffer cmd, const GlyphGpuBuffers& buffers, const mango::font::GlyphGpuData& glyph, const GlyphDrawParams& params)
        {
            if (!m_canvasImage || glyph.curve_bytes.empty())
            {
                return;
            }

            u32 pixel_width = std::max(1u, params.pixel_width);
            u32 pixel_height = std::max(1u, params.pixel_height);

            GlyphPushConstants push {};
            push.color[0] = params.color.x;
            push.color[1] = params.color.y;
            push.color[2] = params.color.z;
            push.color[3] = params.color.w;
            push.params0[0] = params.em_window_x0;
            push.params0[1] = params.em_window_y_top;
            push.params0[2] = params.em_per_pixel;
            push.params0[3] = params.em_per_pixel;
            push.params1[0] = params.screen_x;
            push.params1[1] = params.screen_y;
            push.params1[2] = float(pixel_width);
            push.params1[3] = float(pixel_height);
            push.contour_count = u32(glyph.contours.size());

            VkDescriptorSet sets[] = { m_canvasDescriptorSet, buffers.descriptorSet };

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 2, sets, 0, nullptr);
            vkCmdPushConstants(cmd, m_computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
            vkCmdDispatch(cmd, (pixel_width + 7) / 8, (pixel_height + 7) / 8, 1);

            VkMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            };

            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0, 1, &barrier, 0, nullptr, 0, nullptr);
        }

        void blit_to_swapchain(VkCommandBuffer cmd, VkImageView swapchainView, VkExtent2D extent)
        {
            if (!m_canvasImage)
            {
                return;
            }

            transition_canvas(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            VkImageMemoryBarrier swap_barrier =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .image = VK_NULL_HANDLE,
            };

            VkRenderingAttachmentInfo colorAttachment =
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = swapchainView,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            };

            VkRenderingInfo renderingInfo =
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = { .extent = extent },
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &colorAttachment,
            };

            vkCmdBeginRendering(cmd, &renderingInfo);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_blitPipeline);

            VkViewport viewport =
            {
                .x = 0.0f,
                .y = 0.0f,
                .width = float(extent.width),
                .height = float(extent.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };
            vkCmdSetViewport(cmd, 0, 1, &viewport);

            VkRect2D scissor = { .extent = extent };
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            VkDescriptorSet blitSet = m_blitDescriptorSet;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_blitPipelineLayout, 0, 1, &blitSet, 0, nullptr);
            vkCmdDraw(cmd, 3, 1, 0, 0);
            vkCmdEndRendering(cmd);

            transition_canvas(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

            MANGO_UNREFERENCED(swap_barrier);
        }

        GlyphGpuBuffers upload_glyph(const mango::font::GlyphGpuData& glyph)
        {
            GlyphGpuBuffers buffers;

            VkDeviceSize contour_size = std::max<VkDeviceSize>(glyph.contour_bytes.size(), 16);
            buffers.contours = m_allocator.createBuffer(contour_size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);
            if (glyph.contour_bytes.empty())
            {
                std::memset(buffers.contours.mapped, 0, size_t(contour_size));
            }
            else
            {
                std::memcpy(buffers.contours.mapped, glyph.contour_bytes.data(), glyph.contour_bytes.size());
            }

            VkDeviceSize curve_size = std::max<VkDeviceSize>(glyph.curve_bytes.size(), 16);
            buffers.curves = m_allocator.createBuffer(curve_size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);
            std::memcpy(buffers.curves.mapped, glyph.curve_bytes.data(), glyph.curve_bytes.size());

            VkDescriptorSetAllocateInfo allocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = m_glyphDescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &m_glyphDescriptorSetLayout,
            };

            vkAllocateDescriptorSets(m_device, &allocInfo, &buffers.descriptorSet);

            VkDescriptorBufferInfo contourInfo = { buffers.contours.buffer, 0, VK_WHOLE_SIZE };
            VkDescriptorBufferInfo curveInfo = { buffers.curves.buffer, 0, VK_WHOLE_SIZE };

            VkWriteDescriptorSet writes[2] =
            {
                { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = buffers.descriptorSet, .dstBinding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &contourInfo },
                { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = buffers.descriptorSet, .dstBinding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &curveInfo },
            };

            vkUpdateDescriptorSets(m_device, 2, writes, 0, nullptr);

            return buffers;
        }

        void destroy_glyph_buffers(GlyphGpuBuffers& buffers)
        {
            if (buffers.descriptorSet)
            {
                vkFreeDescriptorSets(m_device, m_glyphDescriptorPool, 1, &buffers.descriptorSet);
                buffers.descriptorSet = VK_NULL_HANDLE;
            }

            if (buffers.contours)
            {
                m_allocator.destroyBuffer(buffers.contours);
            }

            if (buffers.curves)
            {
                m_allocator.destroyBuffer(buffers.curves);
            }
        }

    private:
        VkDevice m_device = VK_NULL_HANDLE;
        VkQueue m_queue = VK_NULL_HANDLE;
        u32 m_queueFamily = 0;
        vulkan::Allocator& m_allocator;

        VkExtent2D m_extent { 0, 0 };

        VkShaderModule m_computeShader = VK_NULL_HANDLE;
        VkShaderModule m_blitVertexShader = VK_NULL_HANDLE;
        VkShaderModule m_blitFragmentShader = VK_NULL_HANDLE;

        VkDescriptorSetLayout m_canvasDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_glyphDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_blitDescriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_blitPipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_computePipeline = VK_NULL_HANDLE;
        VkPipeline m_blitPipeline = VK_NULL_HANDLE;

        VkDescriptorPool m_canvasDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorPool m_glyphDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorPool m_blitDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet m_canvasDescriptorSet = VK_NULL_HANDLE;
        VkDescriptorSet m_blitDescriptorSet = VK_NULL_HANDLE;

        VkSampler m_sampler = VK_NULL_HANDLE;

        vulkan::ImageAllocation m_canvasAllocation;
        VkImage m_canvasImage = VK_NULL_HANDLE;
        VkImageView m_canvasView = VK_NULL_HANDLE;

        void createDescriptorSetLayouts()
        {
            VkDescriptorSetLayoutBinding canvasBinding =
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            };

            VkDescriptorSetLayoutCreateInfo canvasLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 1,
                .pBindings = &canvasBinding,
            };

            vkCreateDescriptorSetLayout(m_device, &canvasLayoutInfo, nullptr, &m_canvasDescriptorSetLayout);

            VkDescriptorSetLayoutBinding glyphBindings[] =
            {
                { .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
                { .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
            };

            VkDescriptorSetLayoutCreateInfo glyphLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 2,
                .pBindings = glyphBindings,
            };

            vkCreateDescriptorSetLayout(m_device, &glyphLayoutInfo, nullptr, &m_glyphDescriptorSetLayout);

            VkDescriptorSetLayoutBinding blitBinding =
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            };

            VkDescriptorSetLayoutCreateInfo blitLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 1,
                .pBindings = &blitBinding,
            };

            vkCreateDescriptorSetLayout(m_device, &blitLayoutInfo, nullptr, &m_blitDescriptorSetLayout);

            VkDescriptorPoolSize canvasPoolSize = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 };
            VkDescriptorPoolCreateInfo canvasPoolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 1,
                .poolSizeCount = 1,
                .pPoolSizes = &canvasPoolSize,
            };

            vkCreateDescriptorPool(m_device, &canvasPoolInfo, nullptr, &m_canvasDescriptorPool);

            VkDescriptorPoolSize glyphPoolSizes[] =
            {
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 128 * 2 },
            };

            VkDescriptorPoolCreateInfo glyphPoolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                .maxSets = 128,
                .poolSizeCount = 1,
                .pPoolSizes = glyphPoolSizes,
            };

            vkCreateDescriptorPool(m_device, &glyphPoolInfo, nullptr, &m_glyphDescriptorPool);

            VkDescriptorPoolSize blitPoolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
            VkDescriptorPoolCreateInfo blitPoolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 1,
                .poolSizeCount = 1,
                .pPoolSizes = &blitPoolSize,
            };

            vkCreateDescriptorPool(m_device, &blitPoolInfo, nullptr, &m_blitDescriptorPool);

            VkDescriptorSetAllocateInfo canvasAllocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = m_canvasDescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &m_canvasDescriptorSetLayout,
            };

            vkAllocateDescriptorSets(m_device, &canvasAllocInfo, &m_canvasDescriptorSet);

            VkDescriptorSetAllocateInfo blitAllocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = m_blitDescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &m_blitDescriptorSetLayout,
            };

            vkAllocateDescriptorSets(m_device, &blitAllocInfo, &m_blitDescriptorSet);
        }

        void createPipelines()
        {
            Compiler compiler;
            Shader compute = compiler.compile(g_computeShader, ShaderStage::Compute);
            Shader blit_vs = compiler.compile(g_blitVertexShader, ShaderStage::Vertex);
            Shader blit_fs = compiler.compile(g_blitFragmentShader, ShaderStage::Fragment);

            if (!compute.valid() || !blit_vs.valid() || !blit_fs.valid())
            {
                printLine(Print::Error, "Font shader compilation failed.");
                if (!compute.log.empty()) printLine(Print::Error, "{}", compute.log);
                return;
            }

            m_computeShader = Compiler::createShaderModule(m_device, compute);
            m_blitVertexShader = Compiler::createShaderModule(m_device, blit_vs);
            m_blitFragmentShader = Compiler::createShaderModule(m_device, blit_fs);

            VkPushConstantRange pushConstantRange =
            {
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .offset = 0,
                .size = sizeof(GlyphPushConstants),
            };

            VkDescriptorSetLayout computeSetLayouts[] = { m_canvasDescriptorSetLayout, m_glyphDescriptorSetLayout };

            VkPipelineLayoutCreateInfo computeLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 2,
                .pSetLayouts = computeSetLayouts,
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = &pushConstantRange,
            };

            vkCreatePipelineLayout(m_device, &computeLayoutInfo, nullptr, &m_computePipelineLayout);

            VkPipelineLayoutCreateInfo blitLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 1,
                .pSetLayouts = &m_blitDescriptorSetLayout,
            };

            vkCreatePipelineLayout(m_device, &blitLayoutInfo, nullptr, &m_blitPipelineLayout);

            VkPipelineShaderStageCreateInfo computeStage =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = m_computeShader,
                .pName = "main",
            };

            VkComputePipelineCreateInfo computePipelineInfo =
            {
                .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                .stage = computeStage,
                .layout = m_computePipelineLayout,
            };

            vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &m_computePipeline);

            VkPipelineShaderStageCreateInfo blitStages[] =
            {
                { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = m_blitVertexShader, .pName = "main" },
                { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = m_blitFragmentShader, .pName = "main" },
            };

            VkPipelineVertexInputStateCreateInfo vertexInputInfo { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
            VkPipelineInputAssemblyStateCreateInfo inputAssembly { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
            VkPipelineViewportStateCreateInfo viewportState { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .scissorCount = 1 };
            VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            VkPipelineDynamicStateCreateInfo dynamicState { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .dynamicStateCount = 2, .pDynamicStates = dynamicStates };
            VkPipelineRasterizationStateCreateInfo rasterizer { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_NONE, .lineWidth = 1.0f };
            VkPipelineMultisampleStateCreateInfo multisampling { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT };
            VkPipelineColorBlendAttachmentState colorBlendAttachment { .colorWriteMask = 0xF };
            VkPipelineColorBlendStateCreateInfo colorBlending { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 1, .pAttachments = &colorBlendAttachment };

            VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
            VkPipelineRenderingCreateInfo renderingCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &colorFormat,
            };

            VkGraphicsPipelineCreateInfo blitPipelineInfo =
            {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &renderingCreateInfo,
                .stageCount = 2,
                .pStages = blitStages,
                .pVertexInputState = &vertexInputInfo,
                .pInputAssemblyState = &inputAssembly,
                .pViewportState = &viewportState,
                .pRasterizationState = &rasterizer,
                .pMultisampleState = &multisampling,
                .pColorBlendState = &colorBlending,
                .pDynamicState = &dynamicState,
                .layout = m_blitPipelineLayout,
            };

            vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &blitPipelineInfo, nullptr, &m_blitPipeline);
        }

        void createSampler()
        {
            VkSamplerCreateInfo samplerInfo =
            {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .magFilter = VK_FILTER_NEAREST,
                .minFilter = VK_FILTER_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler);
        }

        void createCanvas()
        {
            if (m_extent.width == 0 || m_extent.height == 0)
            {
                return;
            }

            VkImageCreateInfo imageInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .extent = { m_extent.width, m_extent.height, 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };

            m_canvasAllocation = m_allocator.createImage(imageInfo, MemoryUsage::GpuOnly);
            m_canvasImage = m_canvasAllocation.image;

            VkImageViewCreateInfo viewInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = m_canvasImage,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };

            vkCreateImageView(m_device, &viewInfo, nullptr, &m_canvasView);

            VkImageMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .image = m_canvasImage,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };

            VkCommandPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = m_queueFamily };
            VkCommandPool pool = VK_NULL_HANDLE;
            vkCreateCommandPool(m_device, &poolInfo, nullptr, &pool);

            VkCommandBufferAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
            VkCommandBuffer cmd = VK_NULL_HANDLE;
            vkAllocateCommandBuffers(m_device, &allocInfo, &cmd);

            VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
            vkBeginCommandBuffer(cmd, &beginInfo);
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            vkEndCommandBuffer(cmd);

            VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd };

            VkFence fence = VK_NULL_HANDLE;
            VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            vkCreateFence(m_device, &fenceInfo, nullptr, &fence);

            vkQueueSubmit(m_queue, 1, &submitInfo, fence);
            vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);
            vkDestroyFence(m_device, fence, nullptr);

            vkFreeCommandBuffers(m_device, pool, 1, &cmd);
            vkDestroyCommandPool(m_device, pool, nullptr);

            update_canvas_descriptor();
            update_blit_descriptor();
        }

        void destroyCanvas()
        {
            if (m_canvasView)
            {
                vkDestroyImageView(m_device, m_canvasView, nullptr);
                m_canvasView = VK_NULL_HANDLE;
            }

            if (m_canvasAllocation)
            {
                m_allocator.destroyImage(m_canvasAllocation);
                m_canvasImage = VK_NULL_HANDLE;
            }
        }

        void update_canvas_descriptor()
        {
            if (!m_canvasView)
            {
                return;
            }

            VkDescriptorImageInfo imageInfo = { VK_NULL_HANDLE, m_canvasView, VK_IMAGE_LAYOUT_GENERAL };
            VkWriteDescriptorSet write =
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_canvasDescriptorSet,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &imageInfo,
            };

            vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
        }

        void update_blit_descriptor()
        {
            if (!m_canvasView)
            {
                return;
            }

            VkDescriptorImageInfo imageInfo = { m_sampler, m_canvasView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            VkWriteDescriptorSet write =
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_blitDescriptorSet,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfo,
            };

            vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
        }

        void transition_canvas(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout,
                               VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                               VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
        {
            VkImageMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = srcAccess,
                .dstAccessMask = dstAccess,
                .oldLayout = oldLayout,
                .newLayout = newLayout,
                .image = m_canvasImage,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };

            vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }
    };

} // namespace mango::vulkan::font

// =============================================================================
// FontWindow — example application
// =============================================================================

class FontWindow : public VulkanWindow
{
protected:
    struct CachedGlyph
    {
        mango::font::GlyphGpuData cpu;
        vulkan::font::GlyphGpuBuffers gpu;
    };

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    u32 m_graphicsQueueFamilyIndex = 0;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    std::unique_ptr<vulkan::Swapchain> m_swapchain;
    std::unique_ptr<vulkan::Allocator> m_allocator;
    std::unique_ptr<vulkan::font::Renderer> m_renderer;
    std::vector<VkCommandBuffer> m_commandBuffers;

    mango::font::Font m_font;
    std::unordered_map<int, CachedGlyph> m_glyphCache;

    float m_fontPixelHeight = 32.0f;
    float m_frameTimeMs = 0.0f;
    static constexpr size_t kFrameTimeHistory = 60;
    std::array<float, kFrameTimeHistory> m_frameTimeHistory {};
    size_t m_frameTimeIndex = 0;
    size_t m_frameTimeCount = 0;
    std::vector<std::string> m_lines =
    {
        "The quick brown fox jumps over the lazy dog. 0123456789",
        "An ancient hero from the waters, from the waves he came while singing.",
        "Ready now I bring my singing, ready now my incantations.",
        "Ilmarinen, smith immortal, forged the wonder-grinder Sampo.",
        "Wainamoinen, old and truthful, sang the birth of ancient wisdom.",
        "Let the dead rest in the darkness, let the living wander light-foot.",
        "Thus began the ancient legends, thus began the old traditions.",
        "Runo after runo he counted, spell on spell he laid in order.",
        "From the forge of the Creator, from the hammer of the Maker.",
        "Never yet was born a singer, never will there be his equal.",
        "Ahti, ruler of the waters, lord of every lake and island.",
    };
    std::string m_fontPath;

public:
    FontWindow(VkInstance instance, int width, int height, u32 flags, const std::string& fontPath)
        : VulkanWindow(instance, width, height, flags)
        , m_fontPath(fontPath)
    {
        m_physicalDevice = selectPhysicalDevice(instance);

        std::vector<VkQueueFamilyProperties> queueFamilies = getPhysicalDeviceQueueFamilyProperties(m_physicalDevice);
        for (size_t i = 0; i < queueFamilies.size(); ++i)
        {
            if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && getPresentationSupport(m_physicalDevice, u32(i)))
            {
                m_graphicsQueueFamilyIndex = u32(i);
                break;
            }
        }

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = m_graphicsQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };

        std::vector<const char*> deviceExtensions = vulkan::requiredDeviceExtensions();

        VkPhysicalDeviceVulkan13Features features13 =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .dynamicRendering = VK_TRUE,
        };

        VkDeviceCreateInfo deviceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &features13,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = u32(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
        };

        vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
        vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);

        VkSurfaceFormatKHR preferredFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        std::vector<VkSurfaceFormatKHR> surfaceFormats = getSurfaceFormats(m_physicalDevice, m_surface);
        VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];
        for (const VkSurfaceFormatKHR& format : surfaceFormats)
        {
            if (format.format == preferredFormat.format && format.colorSpace == preferredFormat.colorSpace)
            {
                selectedFormat = format;
                break;
            }
        }

        m_swapchain = std::make_unique<vulkan::Swapchain>(m_device, m_physicalDevice, m_surface, selectedFormat, m_graphicsQueue, this);
        m_allocator = std::make_unique<vulkan::Allocator>(instance, m_physicalDevice, m_device, VK_API_VERSION_1_3);
        m_renderer = std::make_unique<vulkan::font::Renderer>(m_device, m_graphicsQueue,
            m_graphicsQueueFamilyIndex, *m_allocator);

        VkCommandPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_graphicsQueueFamilyIndex,
        };

        vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);

        m_commandBuffers.resize(m_swapchain->getImageCount());
        VkCommandBufferAllocateInfo cmdAllocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = m_swapchain->getImageCount(),
        };

        vkAllocateCommandBuffers(m_device, &cmdAllocInfo, m_commandBuffers.data());

        if (!m_font.load(m_fontPath, m_fontPixelHeight))
        {
            printLine(Print::Error, "Failed to load font: {}", m_fontPath);
        }
        else
        {
            for (const std::string& line : m_lines)
            {
                cache_string(line);
            }
            cache_string(" fps()");
        }

        VkExtent2D extent = m_swapchain->getExtent();
        m_renderer->resize(extent);
        setVisible(true);
    }

    ~FontWindow()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);

            for (auto& [codepoint, cached] : m_glyphCache)
            {
                MANGO_UNREFERENCED(codepoint);
                m_renderer->destroy_glyph_buffers(cached.gpu);
            }
            m_glyphCache.clear();

            m_renderer.reset();
            m_allocator.reset();
            m_swapchain.reset();
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
            vkDestroyDevice(m_device, nullptr);
        }
    }

    void cache_glyph(int codepoint)
    {
        if (m_glyphCache.count(codepoint))
        {
            return;
        }

        CachedGlyph cached;
        cached.cpu = m_font.load_gpu_glyph(codepoint);
        if (!cached.cpu.curve_bytes.empty())
        {
            cached.gpu = m_renderer->upload_glyph(cached.cpu);
        }

        m_glyphCache[codepoint] = std::move(cached);
    }

    void cache_string(const std::string& text)
    {
        for (unsigned char ch : text)
        {
            cache_glyph(ch);
        }
    }

    const CachedGlyph& get_glyph(int codepoint)
    {
        cache_glyph(codepoint);
        return m_glyphCache.at(codepoint);
    }

    void draw_text(VkCommandBuffer cmd, float pen_x, float baseline_y, std::string_view text, float32x4 color)
    {
        const float pixel_scale = m_font.pixel_scale();
        const float em_per_pixel = m_font.em_per_pixel();

        // One vertical pixel grid for the whole line — do not floor per-glyph bbox tops.
        const float line_screen_y = std::floor(baseline_y - float(m_font.ascent) * pixel_scale);
        const float em_window_y_top = (baseline_y - line_screen_y) * em_per_pixel;

        for (unsigned char ch : text)
        {
            const CachedGlyph& cached = get_glyph(ch);

            if (cached.cpu.curve_bytes.empty())
            {
                pen_x += cached.cpu.advance * pixel_scale;
                continue;
            }

            const float ink_right_em = std::min(cached.cpu.bounds_max.x, cached.cpu.bounds_min.x + cached.cpu.advance);
            const float float_left = pen_x + cached.cpu.bounds_min.x * pixel_scale;
            const float float_right = pen_x + ink_right_em * pixel_scale;
            const float float_bottom = baseline_y - cached.cpu.bounds_min.y * pixel_scale;

            const float screen_x = std::floor(float_left);

            vulkan::font::GlyphDrawParams params;
            params.screen_x = screen_x;
            params.screen_y = line_screen_y;
            params.em_window_x0 = cached.cpu.bounds_min.x + (screen_x - float_left) * em_per_pixel;
            params.em_window_y_top = em_window_y_top;
            params.em_per_pixel = em_per_pixel;
            params.pixel_width = std::max(1u, u32(std::ceil(float_right - screen_x)));
            params.pixel_height = std::max(1u, u32(std::ceil(float_bottom - line_screen_y)));
            params.color = color;

            m_renderer->draw_glyph(cmd, cached.gpu, cached.cpu, params);

            pen_x += cached.cpu.advance * pixel_scale;
        }
    }

    void recordCommandBuffer(VkCommandBuffer cmd, u32 imageIndex, VkExtent2D extent)
    {
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        vkBeginCommandBuffer(cmd, &beginInfo);

        m_renderer->clear_canvas(cmd, 0.1f, 0.14f, 0.23f, 1.0f);

        static constexpr float kHudPixelHeight = 24.0f;
        const float anim_height = m_fontPixelHeight;

        m_font.set_pixel_height(kHudPixelHeight);
        const float hud_scale = m_font.pixel_scale();

        const float fps = m_frameTimeMs > 0.0f ? 1000.0f / m_frameTimeMs : 0.0f;
        char frame_time_text[48];
        std::snprintf(frame_time_text, sizeof(frame_time_text), "%6.3f ms (%3.0f fps)",
            double(m_frameTimeMs), double(fps));

        const float hud_baseline = 8.0f + float(m_font.ascent) * hud_scale;
        draw_text(cmd, 8.0f, hud_baseline, frame_time_text, float32x4(0.75f, 0.9f, 1.0f, 1.0f));

        m_font.set_pixel_height(anim_height);

        const float pixel_scale = m_font.pixel_scale();
        const float line_step = float(m_font.ascent - m_font.descent + m_font.line_gap) * pixel_scale * 1.12f;
        float baseline_y = 100.0f;
        for (const std::string& line : m_lines)
        {
            draw_text(cmd, 40.0f, baseline_y, line, float32x4(1.0f, 1.0f, 1.0f, 1.0f));
            baseline_y += line_step;
        }

        m_swapchain->cmdTransitionImageToColorAttachment(cmd, imageIndex);
        m_renderer->blit_to_swapchain(cmd, m_swapchain->getImageView(imageIndex), extent);
        m_swapchain->cmdTransitionImageToPresent(cmd, imageIndex);

        vkEndCommandBuffer(cmd);
    }

    void render()
    {
        auto frame = m_swapchain->beginFrame();
        if (!frame)
        {
            return;
        }

        VkExtent2D extent = m_swapchain->getExtent();
        if (extent.width > 0 && extent.height > 0)
        {
            m_renderer->resize(extent);
        }

        VkCommandBuffer cmd = m_commandBuffers[frame.imageIndex()];
        recordCommandBuffer(cmd, frame.imageIndex(), extent);
        frame.submitAndPresent(m_graphicsQueue, cmd);
    }

    void onFrame(const FrameInfo& info) override
    {
        constexpr float min_size = 5.0f;
        constexpr float max_size = 64.0f;
        constexpr float cycle_seconds = 10.0f;

        float phase = float(std::fmod(info.time, double(cycle_seconds))) / cycle_seconds;
        float t = 1.0f - std::fabs(phase * 2.0f - 1.0f);
        m_fontPixelHeight = min_size + t * (max_size - min_size);
        m_font.set_pixel_height(m_fontPixelHeight);

        const float frame_ms = float(info.dt * 1000.0);
        m_frameTimeHistory[m_frameTimeIndex] = frame_ms;
        m_frameTimeIndex = (m_frameTimeIndex + 1) % kFrameTimeHistory;
        m_frameTimeCount = std::min(m_frameTimeCount + 1, kFrameTimeHistory);

        float frame_sum = 0.0f;
        for (size_t i = 0; i < m_frameTimeCount; ++i)
        {
            frame_sum += m_frameTimeHistory[i];
        }
        m_frameTimeMs = frame_sum / float(m_frameTimeCount);

        u64 time0 = mango::Time::us();
        render();
        u64 time1 = mango::Time::us();
        setTitle(fmt::format("font: {:.2f} ms", (time1 - time0) / 1000.0));
    }

    void onKeyPress(Keycode code, u32 mask) override
    {
        MANGO_UNREFERENCED(mask);

        if (code == KEYCODE_ESC)
        {
            breakEventLoop();
        }
    }
};

static std::string default_font_path()
{
#if defined(MANGO_PLATFORM_WINDOWS)
    return "C:/Windows/Fonts/arial.ttf";
#elif defined(MANGO_PLATFORM_MACOS)
    return "/System/Library/Fonts/Supplemental/Arial.ttf";
#else
    return "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif
}

int mangoMain(const mango::CommandLine& commands)
{
    std::string fontPath = default_font_path();
    //bool validate_only = false;

    for (size_t i = 1; i < commands.size(); ++i)
    {
        std::string arg = std::string(commands[i]);
        if (arg[0] != '-')
        {
            fontPath = arg;
        }
    }

    std::vector<const char*> enabledExtensions = vulkan::requiredSurfaceExtensions();

    InstanceExtensionProperties instanceExtensionProperties;
    if (instanceExtensionProperties.contains(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME))
    {
        enabledExtensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
    }

    VkApplicationInfo applicationInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "font",
        .applicationVersion = 1,
        .pEngineName = "mango",
        .engineVersion = 1,
        .apiVersion = VK_MAKE_VERSION(1, 3, 0),
    };

    Instance instance(applicationInfo, {}, enabledExtensions);

    FontWindow window(instance, 1280, 720, 0, fontPath);
    window.setTitle("Scanline Sweeper Font (Vulkan)");

    EventLoopConfig config;
    config.mode = FrameMode::Continuous;
    config.waitForFrame = true;
    window.enterEventLoop(config);

    return 0;
}

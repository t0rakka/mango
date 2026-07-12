/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "shaders.hpp"

namespace mango::vulkan::font_shaders
{

const char* computeShader() 
{
    return R"(#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(push_constant) uniform Batch
{
    uvec2 tile_origin;
    uvec2 tile_count;
    uint tile_size;
    uint workgroup_size;
} pc;

layout(std430, set = 1, binding = 0) readonly buffer ContourBuffer
{
    uvec2 contours[];
};

layout(std430, set = 1, binding = 1) readonly buffer CurveBuffer
{
    float points[];
};

layout(std430, set = 1, binding = 2) readonly buffer InstanceBuffer
{
    vec4 instance_data[];
};

layout(std430, set = 1, binding = 3) readonly buffer TileBuffer
{
    uvec2 tiles[];
};

layout(std430, set = 1, binding = 4) readonly buffer TileGlyphBuffer
{
    uint tile_glyphs[];
};

layout(set = 0, binding = 0, rgba8) uniform image2D canvas;

struct GlyphInstance
{
    uint contour_offset;
    uint contour_count;
    vec4 color;
    vec4 params0;
    vec4 params1;
};

GlyphInstance load_instance(uint index)
{
    uint base = index * 4u;
    GlyphInstance inst;
    inst.contour_offset = uint(instance_data[base + 0u].x);
    inst.contour_count = uint(instance_data[base + 0u].y);
    inst.color = instance_data[base + 1u];
    inst.params0 = instance_data[base + 2u];
    inst.params1 = instance_data[base + 3u];
    return inst;
}

vec2 read_curve_point(uint curve_index, uint point_index)
{
    uint base = curve_index * 6u + point_index * 2u;
    return vec2(points[base + 0u], points[base + 1u]);
}

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

float compute_glyph_alpha(GlyphInstance inst, ivec2 pixel)
{
    vec2 pixel_size = inst.params1.zw;
    vec2 gid = vec2(pixel) - inst.params1.xy;
    if (gid.x < 0.0 || gid.y < 0.0 || gid.x >= pixel_size.x || gid.y >= pixel_size.y)
    {
        return 0.0;
    }

    vec2 em_per_pixel = inst.params0.zw;
    vec2 em_offset = vec2(
        inst.params0.x + gid.x * em_per_pixel.x,
        inst.params0.y - gid.y * em_per_pixel.y);
    vec2 window_size = em_per_pixel;
    float area = window_size.x * window_size.y;

    float coverage_sum = 0.0;

    for (uint ci = 0u; ci < inst.contour_count; ++ci)
    {
        uvec2 contour = contours[inst.contour_offset + ci];
        uint curve_offset = contour.x;
        uint curve_count = contour.y;

        for (uint i = 0u; i < curve_count; ++i)
        {
            uint idx = curve_offset + i;
            vec2 p0 = read_curve_point(idx, 0u);
            vec2 p1 = read_curve_point(idx, 1u);
            vec2 p2 = read_curve_point(idx, 2u);

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
    return alpha * inst.color.a;
}

void main()
{
    uvec2 wg = gl_WorkGroupID.xy;
    uint subgroups_per_tile = max(1u, (pc.tile_size + pc.workgroup_size - 1u) / pc.workgroup_size);
    uvec2 tile_idx = wg / subgroups_per_tile;
    if (tile_idx.x >= pc.tile_count.x || tile_idx.y >= pc.tile_count.y)
    {
        return;
    }

    uvec2 sub_wg = wg % subgroups_per_tile;
    ivec2 pixel = ivec2(pc.tile_origin)
        + ivec2(tile_idx) * int(pc.tile_size)
        + ivec2(sub_wg) * int(pc.workgroup_size)
        + ivec2(gl_LocalInvocationID.xy);

    ivec2 canvas_size = imageSize(canvas);
    if (pixel.x < 0 || pixel.y < 0 || pixel.x >= canvas_size.x || pixel.y >= canvas_size.y)
    {
        return;
    }

    uint tile_index = tile_idx.y * pc.tile_count.x + tile_idx.x;
    uvec2 tile = tiles[tile_index];
    uint glyph_start = tile.x;
    uint glyph_count = tile.y;

    if (glyph_count == 0u)
    {
        return;
    }

    vec4 dst = imageLoad(canvas, pixel);
    vec3 rgb = dst.rgb;
    float alpha_out = dst.a;

    for (uint gi = 0u; gi < glyph_count; ++gi)
    {
        uint inst_index = tile_glyphs[glyph_start + gi];
        GlyphInstance inst = load_instance(inst_index);

        float src_a = compute_glyph_alpha(inst, pixel);
        if (src_a <= 0.0)
        {
            continue;
        }

        vec3 src_rgb = inst.color.rgb;
        float out_a = src_a + alpha_out * (1.0 - src_a);
        rgb = (src_rgb * src_a + rgb * alpha_out * (1.0 - src_a)) / max(out_a, 1e-6);
        alpha_out = out_a;
    }

    imageStore(canvas, pixel, vec4(rgb, alpha_out));
}
)";
}

const char* blitVertexShader()
{
    return R"(#version 450
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
}

const char* blitFragmentShader()
{
    return R"(#version 450
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
    if (outColor.a <= 0.0)
    {
        discard;
    }
}
)";
}

} // namespace mango::vulkan::font_shaders

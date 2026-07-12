/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <deque>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>

#include <mango/vulkan/font.hpp>
#include <mango/vulkan/allocator.hpp>
#include <mango/vulkan/compiler.hpp>
#include <mango/core/print.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include <mango/math/math.hpp>

#include "font_internal.hpp"
#include "layout.hpp"
#include "shaders.hpp"

namespace mango::vulkan
{

    namespace
    {
        using namespace mango::math;

        static constexpr u32 kTileSize = 64;
        static constexpr VkDeviceSize kAtlasInitialBytes = 256 * 1024;

        struct GlyphAtlasSlot
        {
            u32 contour_offset = 0;
            u32 contour_count = 0;
        };

        struct GpuGlyphInstance
        {
            float header[4] = {};
            float color[4] = {};
            float params0[4] = {};
            float params1[4] = {};
        };

        struct GpuTileInfo
        {
            u32 glyph_start = 0;
            u32 glyph_count = 0;
        };

        struct BatchPushConstants
        {
            u32 tile_origin_x = 0;
            u32 tile_origin_y = 0;
            u32 tiles_x = 0;
            u32 tiles_y = 0;
            u32 tile_size = kTileSize;
            u32 workgroup_size = 8;
        };

        struct GlyphDrawParams
        {
            float screen_x = 0.0f;
            float screen_y = 0.0f;
            float em_window_x0 = 0.0f;
            float em_window_y0 = 0.0f;
            float em_per_pixel = 1.0f;
            u32 pixel_width = 1;
            u32 pixel_height = 1;
            float32x4 color { 1.0f, 1.0f, 1.0f, 1.0f };
        };

        struct PendingGlyph
        {
            GpuGlyphInstance gpu;
            float rect_x0 = 0.0f;
            float rect_y0 = 0.0f;
            float rect_x1 = 0.0f;
            float rect_y1 = 0.0f;
        };

        struct FaceSlot
        {
            font::Face face;
            float pixelHeight = 32.0f;
            u32 generation = 0;
            bool used = false;
        };

        struct CachedGlyph
        {
            font::GlyphGpuData cpu;
            GlyphAtlasSlot atlas;
        };

        static u64 glyphCacheKey(u32 faceIndex, u32 codepoint, float pixelHeight)
        {
            const u32 height_bits = std::bit_cast<u32>(pixelHeight);
            return (u64(faceIndex) << 48) | (u64(height_bits) << 32) | u64(codepoint);
        }

        static float effectivePixelHeight(const FaceSlot& slot, float stylePixelHeight)
        {
            return stylePixelHeight > 0.0f ? stylePixelHeight : slot.pixelHeight;
        }

    } // namespace

    struct FontRenderer::Impl
    {
        VkDevice device = VK_NULL_HANDLE;
        VkQueue queue = VK_NULL_HANDLE;
        u32 queueFamily = 0;
        Allocator* allocator = nullptr;

        VkExtent2D extent { 0, 0 };
        VkFormat blitColorFormat = VK_FORMAT_B8G8R8A8_UNORM;

        VkShaderModule computeShader = VK_NULL_HANDLE;
        VkShaderModule blitVertexShader = VK_NULL_HANDLE;
        VkShaderModule blitFragmentShader = VK_NULL_HANDLE;

        VkDescriptorSetLayout canvasDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout batchDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout blitDescriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;
        VkPipelineLayout blitPipelineLayout = VK_NULL_HANDLE;
        VkPipeline computePipeline = VK_NULL_HANDLE;
        VkPipeline overlayBlitPipeline = VK_NULL_HANDLE;
        VkPipeline replaceBlitPipeline = VK_NULL_HANDLE;

        VkDescriptorPool canvasDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorPool batchDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorPool blitDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet canvasDescriptorSet = VK_NULL_HANDLE;
        VkDescriptorSet batchDescriptorSet = VK_NULL_HANDLE;
        VkDescriptorSet blitDescriptorSet = VK_NULL_HANDLE;

        VkSampler sampler = VK_NULL_HANDLE;

        ImageAllocation canvasAllocation;
        VkImage canvasImage = VK_NULL_HANDLE;
        VkImageView canvasView = VK_NULL_HANDLE;

        BufferAllocation atlasContours;
        BufferAllocation atlasCurves;
        VkDeviceSize atlasContourCapacity = 0;
        VkDeviceSize atlasCurveCapacity = 0;
        VkDeviceSize atlasContourBytes = 0;
        VkDeviceSize atlasCurveBytes = 0;
        u32 atlasContourCount = 0;
        u32 atlasCurveCount = 0;

        BufferAllocation instanceBuffer;
        BufferAllocation tileBuffer;
        BufferAllocation tileGlyphBuffer;
        VkDeviceSize instanceBufferCapacity = 0;
        VkDeviceSize tileBufferCapacity = 0;
        VkDeviceSize tileGlyphBufferCapacity = 0;

        std::deque<FaceSlot> faces;
        std::unordered_map<u64, CachedGlyph> glyphCache;
        std::vector<PendingGlyph> pendingGlyphs;

        math::float32x4 frameClearColor { 0.0f, 0.0f, 0.0f, 0.0f };
        bool frameOpen = false;

        Impl(VkDevice dev, VkQueue q, u32 family, Allocator& alloc, VkFormat targetFormat)
            : device(dev)
            , queue(q)
            , queueFamily(family)
            , allocator(&alloc)
            , blitColorFormat(targetFormat)
        {
            createDescriptorSetLayouts();
            createPipelines();
            createSampler();
        }

        ~Impl()
        {
            if (device == VK_NULL_HANDLE)
            {
                return;
            }

            vkDeviceWaitIdle(device);

            destroyCanvas();

            if (sampler)
            {
                vkDestroySampler(device, sampler, nullptr);
            }

            if (replaceBlitPipeline)
            {
                vkDestroyPipeline(device, replaceBlitPipeline, nullptr);
            }

            if (overlayBlitPipeline)
            {
                vkDestroyPipeline(device, overlayBlitPipeline, nullptr);
            }

            if (computePipeline)
            {
                vkDestroyPipeline(device, computePipeline, nullptr);
            }

            if (blitPipelineLayout)
            {
                vkDestroyPipelineLayout(device, blitPipelineLayout, nullptr);
            }

            if (computePipelineLayout)
            {
                vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);
            }

            if (batchDescriptorPool)
            {
                vkDestroyDescriptorPool(device, batchDescriptorPool, nullptr);
            }

            destroyAtlas();
            destroyBatchBuffers();

            if (canvasDescriptorPool)
            {
                vkDestroyDescriptorPool(device, canvasDescriptorPool, nullptr);
            }

            if (blitDescriptorPool)
            {
                vkDestroyDescriptorPool(device, blitDescriptorPool, nullptr);
            }

            if (blitDescriptorSetLayout)
            {
                vkDestroyDescriptorSetLayout(device, blitDescriptorSetLayout, nullptr);
            }

            if (batchDescriptorSetLayout)
            {
                vkDestroyDescriptorSetLayout(device, batchDescriptorSetLayout, nullptr);
            }

            if (canvasDescriptorSetLayout)
            {
                vkDestroyDescriptorSetLayout(device, canvasDescriptorSetLayout, nullptr);
            }

            if (computeShader)
            {
                vkDestroyShaderModule(device, computeShader, nullptr);
            }

            if (blitVertexShader)
            {
                vkDestroyShaderModule(device, blitVertexShader, nullptr);
            }

            if (blitFragmentShader)
            {
                vkDestroyShaderModule(device, blitFragmentShader, nullptr);
            }
        }

        FaceSlot* validateHandle(Font font)
        {
            if (!font || font.index >= faces.size())
            {
                return nullptr;
            }

            FaceSlot& slot = faces[font.index];
            if (!slot.used || slot.generation != font.generation)
            {
                return nullptr;
            }

            return &slot;
        }

        const FaceSlot* validateHandle(Font font) const
        {
            if (!font || font.index >= faces.size())
            {
                return nullptr;
            }

            const FaceSlot& slot = faces[font.index];
            if (!slot.used || slot.generation != font.generation)
            {
                return nullptr;
            }

            return &slot;
        }

        u32 allocateFaceSlot()
        {
            for (u32 i = 0; i < u32(faces.size()); ++i)
            {
                if (!faces[i].used)
                {
                    return i;
                }
            }

            faces.emplace_back();
            return u32(faces.size() - 1);
        }

        void removeGlyphCacheForFace(u32 faceIndex)
        {
            for (auto it = glyphCache.begin(); it != glyphCache.end(); )
            {
                if (u32(it->first >> 48) == faceIndex)
                {
                    it = glyphCache.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void withFaceAtHeight(u32 faceIndex, float pixelHeight, auto&& fn)
        {
            FaceSlot& slot = faces[faceIndex];
            const float height = effectivePixelHeight(slot, pixelHeight);
            const float saved = slot.pixelHeight;
            slot.face.setPixelHeight(height);
            fn(slot.face, height);
            slot.face.setPixelHeight(saved);
        }

        void cacheGlyph(u32 faceIndex, u32 codepoint, float pixelHeight)
        {
            FaceSlot& slot = faces[faceIndex];
            const float height = effectivePixelHeight(slot, pixelHeight);
            const u64 key = glyphCacheKey(faceIndex, codepoint, height);
            if (glyphCache.count(key))
            {
                return;
            }

            CachedGlyph cached;
            const float saved = slot.pixelHeight;
            slot.face.setPixelHeight(height);
            cached.cpu = slot.face.loadGpuGlyph(codepoint);
            slot.face.setPixelHeight(saved);

            if (!cached.cpu.curve_bytes.empty())
            {
                cached.atlas = appendGlyphToAtlas(cached.cpu);
            }

            glyphCache[key] = std::move(cached);
        }

        const CachedGlyph& getCachedGlyph(u32 faceIndex, u32 codepoint, float pixelHeight)
        {
            FaceSlot& slot = faces[faceIndex];
            const float height = effectivePixelHeight(slot, pixelHeight);
            cacheGlyph(faceIndex, codepoint, height);
            return glyphCache.at(glyphCacheKey(faceIndex, codepoint, height));
        }

        GlyphAtlasSlot appendGlyphToAtlas(const font::GlyphGpuData& glyph)
        {
            GlyphAtlasSlot slot;
            if (glyph.curve_bytes.empty())
            {
                return slot;
            }

            const u32 contour_count = u32(glyph.contours.size());
            const u32 curve_count = u32(glyph.curve_data.size() / 6);
            const u32 curve_index_base = atlasCurveCount;

            const VkDeviceSize contour_bytes = sizeof(font::GpuContourHeader) * contour_count;
            const VkDeviceSize curve_bytes = glyph.curve_bytes.size();

            ensureAtlasCapacity(atlasContourBytes + contour_bytes, atlasCurveBytes + curve_bytes);

            slot.contour_offset = atlasContourCount;
            slot.contour_count = contour_count;

            auto* contour_dst = reinterpret_cast<font::GpuContourHeader*>(
                static_cast<u8*>(atlasContours.mapped) + atlasContourBytes);

            for (u32 i = 0; i < contour_count; ++i)
            {
                contour_dst[i] = glyph.contours[i];
                contour_dst[i].curve_offset += curve_index_base;
            }

            std::memcpy(static_cast<u8*>(atlasCurves.mapped) + atlasCurveBytes,
                glyph.curve_bytes.data(), glyph.curve_bytes.size());

            atlasContourBytes += contour_bytes;
            atlasCurveBytes += curve_bytes;
            atlasContourCount += contour_count;
            atlasCurveCount += curve_count;

            updateAtlasDescriptors();
            return slot;
        }

        void queueGlyphsForLine(u32 faceIndex, float pen_x, float baseline_y,
                                const std::u32string& codepoints, float32x4 color, float pixelHeight)
        {
            if (codepoints.empty())
            {
                return;
            }

            withFaceAtHeight(faceIndex, pixelHeight, [&](font::Face& face, float height)
            {
                const float pixel_scale = face.pixelScale();
                const float em_per_pixel = face.emPerPixel();

                float line_ink_top = baseline_y;
                float line_ink_bottom = baseline_y;
                bool has_ink = false;

                for (char32_t cp : codepoints)
                {
                    const CachedGlyph& cached = getCachedGlyph(faceIndex, u32(cp), height);
                    if (cached.cpu.curve_bytes.empty())
                    {
                        continue;
                    }

                    has_ink = true;
                    const float ink_top = baseline_y - cached.cpu.bounds_max.y * pixel_scale;
                    const float ink_bottom = baseline_y - cached.cpu.bounds_min.y * pixel_scale;
                    line_ink_top = std::min(line_ink_top, ink_top);
                    line_ink_bottom = std::max(line_ink_bottom, ink_bottom);
                }

                if (!has_ink)
                {
                    return;
                }

                const float line_screen_y = std::floor(line_ink_top);
                const float em_window_y0 = (baseline_y - (line_screen_y + 1.0f)) * em_per_pixel;
                const u32 line_pixel_height = std::max(1u, u32(std::ceil(line_ink_bottom - line_screen_y)));

                for (size_t i = 0; i < codepoints.size(); ++i)
                {
                    const u32 cp = u32(codepoints[i]);
                    const u32 next = (i + 1 < codepoints.size()) ? u32(codepoints[i + 1]) : 0;
                    const CachedGlyph& cached = getCachedGlyph(faceIndex, cp, height);

                    if (cached.cpu.curve_bytes.empty())
                    {
                        pen_x += cached.cpu.advance * pixel_scale;
                        if (next)
                        {
                            pen_x += float(face.kerning(cp, next)) * pixel_scale;
                        }
                        continue;
                    }

                    const float ink_right_em = std::min(cached.cpu.bounds_max.x,
                        cached.cpu.bounds_min.x + cached.cpu.advance);
                    const float float_left = pen_x + cached.cpu.bounds_min.x * pixel_scale;
                    const float float_right = pen_x + ink_right_em * pixel_scale;

                    const float screen_x = std::floor(float_left);

                    GlyphDrawParams params;
                    params.screen_x = screen_x;
                    params.screen_y = line_screen_y;
                    params.em_window_x0 = cached.cpu.bounds_min.x + (screen_x - float_left) * em_per_pixel;
                    params.em_window_y0 = em_window_y0;
                    params.em_per_pixel = em_per_pixel;
                    params.pixel_width = std::max(1u, u32(std::ceil(float_right - screen_x)));
                    params.pixel_height = line_pixel_height;
                    params.color = color;

                    queueGlyph(cached.atlas, params);

                    pen_x += cached.cpu.advance * pixel_scale;
                    if (next)
                    {
                        pen_x += float(face.kerning(cp, next)) * pixel_scale;
                    }
                }
            });
        }

        void queueGlyph(const GlyphAtlasSlot& atlas, const GlyphDrawParams& params)
        {
            if (!atlas.contour_count)
            {
                return;
            }

            PendingGlyph pending {};
            pending.gpu.header[0] = float(atlas.contour_offset);
            pending.gpu.header[1] = float(atlas.contour_count);
            pending.gpu.color[0] = params.color.x;
            pending.gpu.color[1] = params.color.y;
            pending.gpu.color[2] = params.color.z;
            pending.gpu.color[3] = params.color.w;
            pending.gpu.params0[0] = params.em_window_x0;
            pending.gpu.params0[1] = params.em_window_y0;
            pending.gpu.params0[2] = params.em_per_pixel;
            pending.gpu.params0[3] = params.em_per_pixel;
            pending.gpu.params1[0] = params.screen_x;
            pending.gpu.params1[1] = params.screen_y;
            pending.gpu.params1[2] = float(std::max(1u, params.pixel_width));
            pending.gpu.params1[3] = float(std::max(1u, params.pixel_height));
            pending.rect_x0 = params.screen_x;
            pending.rect_y0 = params.screen_y;
            pending.rect_x1 = params.screen_x + float(std::max(1u, params.pixel_width));
            pending.rect_y1 = params.screen_y + float(std::max(1u, params.pixel_height));
            pendingGlyphs.push_back(pending);
        }

        void queueGlyphsForLayoutLine(u32 faceIndex, const font::LayoutLine& line, float32x4 color, float pixelHeight)
        {
            if (line.glyphs.empty())
            {
                return;
            }

            withFaceAtHeight(faceIndex, pixelHeight, [&](font::Face& face, float height)
            {
                const float pixel_scale = face.pixelScale();
                const float em_per_pixel = face.emPerPixel();
                const float baseline_y = line.glyphs[0].y;

                float line_ink_top = baseline_y;
                float line_ink_bottom = baseline_y;
                bool has_ink = false;

                for (const font::PositionedGlyph& pg : line.glyphs)
                {
                    const CachedGlyph& cached = getCachedGlyph(faceIndex, pg.codepoint, height);
                    if (cached.cpu.curve_bytes.empty())
                    {
                        continue;
                    }

                    has_ink = true;
                    const float ink_top = baseline_y - cached.cpu.bounds_max.y * pixel_scale;
                    const float ink_bottom = baseline_y - cached.cpu.bounds_min.y * pixel_scale;
                    line_ink_top = std::min(line_ink_top, ink_top);
                    line_ink_bottom = std::max(line_ink_bottom, ink_bottom);
                }

                if (!has_ink)
                {
                    return;
                }

                const float line_screen_y = std::floor(line_ink_top);
                const float em_window_y0 = (baseline_y - (line_screen_y + 1.0f)) * em_per_pixel;
                const u32 line_pixel_height = std::max(1u, u32(std::ceil(line_ink_bottom - line_screen_y)));

                for (const font::PositionedGlyph& pg : line.glyphs)
                {
                    const CachedGlyph& cached = getCachedGlyph(faceIndex, pg.codepoint, height);

                    if (cached.cpu.curve_bytes.empty())
                    {
                        continue;
                    }

                    const float ink_right_em = std::min(cached.cpu.bounds_max.x,
                        cached.cpu.bounds_min.x + cached.cpu.advance);
                    const float float_left = pg.x + cached.cpu.bounds_min.x * pixel_scale;
                    const float float_right = pg.x + ink_right_em * pixel_scale;

                    const float screen_x = std::floor(float_left);

                    GlyphDrawParams params;
                    params.screen_x = screen_x;
                    params.screen_y = line_screen_y;
                    params.em_window_x0 = cached.cpu.bounds_min.x + (screen_x - float_left) * em_per_pixel;
                    params.em_window_y0 = em_window_y0;
                    params.em_per_pixel = em_per_pixel;
                    params.pixel_width = std::max(1u, u32(std::ceil(float_right - screen_x)));
                    params.pixel_height = line_pixel_height;
                    params.color = color;

                    queueGlyph(cached.atlas, params);
                }
            });
        }

        void resize(VkExtent2D newExtent)
        {
            if (newExtent.width == 0 || newExtent.height == 0)
            {
                return;
            }

            if (newExtent.width == extent.width && newExtent.height == extent.height && canvasImage)
            {
                return;
            }

            vkDeviceWaitIdle(device);
            destroyCanvas();
            extent = newExtent;
            createCanvas();
        }

        void clearCanvas(VkCommandBuffer cmd, float r, float g, float b, float a)
        {
            if (!canvasImage)
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
                .image = canvasImage,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };

            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkClearColorValue clear = {{ r, g, b, a }};
            VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            vkCmdClearColorImage(cmd, canvasImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);

            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier);
        }

        void flushTextBatch(VkCommandBuffer cmd)
        {
            if (!canvasImage || pendingGlyphs.empty())
            {
                return;
            }

            float bbox_x0 = pendingGlyphs[0].rect_x0;
            float bbox_y0 = pendingGlyphs[0].rect_y0;
            float bbox_x1 = pendingGlyphs[0].rect_x1;
            float bbox_y1 = pendingGlyphs[0].rect_y1;

            for (const PendingGlyph& glyph : pendingGlyphs)
            {
                bbox_x0 = std::min(bbox_x0, glyph.rect_x0);
                bbox_y0 = std::min(bbox_y0, glyph.rect_y0);
                bbox_x1 = std::max(bbox_x1, glyph.rect_x1);
                bbox_y1 = std::max(bbox_y1, glyph.rect_y1);
            }

            bbox_x0 = std::floor(bbox_x0);
            bbox_y0 = std::floor(bbox_y0);
            bbox_x1 = std::ceil(bbox_x1);
            bbox_y1 = std::ceil(bbox_y1);

            bbox_x0 = std::max(0.0f, bbox_x0);
            bbox_y0 = std::max(0.0f, bbox_y0);
            bbox_x1 = std::min(float(extent.width), bbox_x1);
            bbox_y1 = std::min(float(extent.height), bbox_y1);

            if (bbox_x1 <= bbox_x0 || bbox_y1 <= bbox_y0)
            {
                return;
            }

            u32 tile_origin_x = u32(bbox_x0) / kTileSize * kTileSize;
            u32 tile_origin_y = u32(bbox_y0) / kTileSize * kTileSize;
            u32 tile_size = kTileSize;
            u32 tiles_x = (u32(std::ceil(bbox_x1)) - tile_origin_x + tile_size - 1) / tile_size;
            u32 tiles_y = (u32(std::ceil(bbox_y1)) - tile_origin_y + tile_size - 1) / tile_size;

            std::vector<GpuTileInfo> tile_infos(tiles_x * tiles_y);
            std::vector<u32> tile_glyphs;
            tile_glyphs.reserve(pendingGlyphs.size() * 2);

            for (u32 ty = 0; ty < tiles_y; ++ty)
            {
                for (u32 tx = 0; tx < tiles_x; ++tx)
                {
                    const float tile_x0 = float(tile_origin_x + tx * tile_size);
                    const float tile_y0 = float(tile_origin_y + ty * tile_size);
                    const float tile_x1 = tile_x0 + float(tile_size);
                    const float tile_y1 = tile_y0 + float(tile_size);

                    GpuTileInfo& tile = tile_infos[ty * tiles_x + tx];
                    tile.glyph_start = u32(tile_glyphs.size());

                    for (u32 i = 0; i < u32(pendingGlyphs.size()); ++i)
                    {
                        const PendingGlyph& glyph = pendingGlyphs[i];
                        if (glyph.rect_x1 <= tile_x0 || glyph.rect_x0 >= tile_x1 ||
                            glyph.rect_y1 <= tile_y0 || glyph.rect_y0 >= tile_y1)
                        {
                            continue;
                        }

                        tile_glyphs.push_back(i);
                    }

                    tile.glyph_count = u32(tile_glyphs.size()) - tile.glyph_start;
                }
            }

            const VkDeviceSize instance_bytes = sizeof(GpuGlyphInstance) * pendingGlyphs.size();
            const VkDeviceSize tile_bytes = sizeof(GpuTileInfo) * tile_infos.size();
            const VkDeviceSize index_bytes = sizeof(u32) * std::max<size_t>(tile_glyphs.size(), 1);

            ensureBatchBuffer(instanceBuffer, instanceBufferCapacity, instance_bytes);
            ensureBatchBuffer(tileBuffer, tileBufferCapacity, tile_bytes);
            ensureBatchBuffer(tileGlyphBuffer, tileGlyphBufferCapacity, index_bytes);

            for (size_t i = 0; i < pendingGlyphs.size(); ++i)
            {
                std::memcpy(static_cast<u8*>(instanceBuffer.mapped) + i * sizeof(GpuGlyphInstance),
                    &pendingGlyphs[i].gpu, sizeof(GpuGlyphInstance));
            }
            std::memcpy(tileBuffer.mapped, tile_infos.data(), size_t(tile_bytes));
            if (tile_glyphs.empty())
            {
                u32 zero = 0;
                std::memcpy(tileGlyphBuffer.mapped, &zero, sizeof(zero));
            }
            else
            {
                std::memcpy(tileGlyphBuffer.mapped, tile_glyphs.data(), size_t(index_bytes));
            }

            BatchPushConstants push {};
            push.tile_origin_x = tile_origin_x;
            push.tile_origin_y = tile_origin_y;
            push.tiles_x = tiles_x;
            push.tiles_y = tiles_y;
            push.tile_size = tile_size;
            push.workgroup_size = 8;

            const u32 subgroups_per_axis = (tile_size + push.workgroup_size - 1) / push.workgroup_size;

            VkDescriptorSet sets[] = { canvasDescriptorSet, batchDescriptorSet };

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 2, sets, 0, nullptr);
            vkCmdPushConstants(cmd, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
            vkCmdDispatch(cmd, tiles_x * subgroups_per_axis, tiles_y * subgroups_per_axis, 1);
        }

        void blitToTarget(VkCommandBuffer cmd, VkImageView targetView, VkExtent2D targetExtent, ResolveMode mode)
        {
            if (!canvasImage)
            {
                return;
            }

            transitionCanvas(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            const bool overlay = (mode == ResolveMode::Overlay);

            VkRenderingAttachmentInfo colorAttachment =
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = targetView,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = overlay ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            };

            VkRenderingInfo renderingInfo =
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = { .extent = targetExtent },
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &colorAttachment,
            };

            vkCmdBeginRendering(cmd, &renderingInfo);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                overlay ? overlayBlitPipeline : replaceBlitPipeline);

            VkViewport viewport =
            {
                .x = 0.0f,
                .y = 0.0f,
                .width = float(targetExtent.width),
                .height = float(targetExtent.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };
            vkCmdSetViewport(cmd, 0, 1, &viewport);

            VkRect2D scissor = { .extent = targetExtent };
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            VkDescriptorSet blitSet = blitDescriptorSet;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blitPipelineLayout, 0, 1, &blitSet, 0, nullptr);
            vkCmdDraw(cmd, 3, 1, 0, 0);
            vkCmdEndRendering(cmd);

            transitionCanvas(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }

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

            vkCreateDescriptorSetLayout(device, &canvasLayoutInfo, nullptr, &canvasDescriptorSetLayout);

            VkDescriptorSetLayoutBinding batchBindings[] =
            {
                { .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
                { .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
                { .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
                { .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
                { .binding = 4, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
            };

            VkDescriptorSetLayoutCreateInfo batchLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 5,
                .pBindings = batchBindings,
            };

            vkCreateDescriptorSetLayout(device, &batchLayoutInfo, nullptr, &batchDescriptorSetLayout);

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

            vkCreateDescriptorSetLayout(device, &blitLayoutInfo, nullptr, &blitDescriptorSetLayout);

            VkDescriptorPoolSize canvasPoolSize = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 };
            VkDescriptorPoolCreateInfo canvasPoolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 1,
                .poolSizeCount = 1,
                .pPoolSizes = &canvasPoolSize,
            };

            vkCreateDescriptorPool(device, &canvasPoolInfo, nullptr, &canvasDescriptorPool);

            VkDescriptorPoolSize batchPoolSizes[] =
            {
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5 },
            };

            VkDescriptorPoolCreateInfo batchPoolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 1,
                .poolSizeCount = 1,
                .pPoolSizes = batchPoolSizes,
            };

            vkCreateDescriptorPool(device, &batchPoolInfo, nullptr, &batchDescriptorPool);

            VkDescriptorPoolSize blitPoolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
            VkDescriptorPoolCreateInfo blitPoolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 1,
                .poolSizeCount = 1,
                .pPoolSizes = &blitPoolSize,
            };

            vkCreateDescriptorPool(device, &blitPoolInfo, nullptr, &blitDescriptorPool);

            VkDescriptorSetAllocateInfo canvasAllocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = canvasDescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &canvasDescriptorSetLayout,
            };

            vkAllocateDescriptorSets(device, &canvasAllocInfo, &canvasDescriptorSet);

            VkDescriptorSetAllocateInfo blitAllocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = blitDescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &blitDescriptorSetLayout,
            };

            vkAllocateDescriptorSets(device, &blitAllocInfo, &blitDescriptorSet);

            VkDescriptorSetAllocateInfo batchAllocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = batchDescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &batchDescriptorSetLayout,
            };

            vkAllocateDescriptorSets(device, &batchAllocInfo, &batchDescriptorSet);

            createAtlasBuffers();
            createBatchBuffers();
        }

        void createPipelines()
        {
            Compiler compiler;
            Shader compute = compiler.compile(font_shaders::computeShader(), ShaderStage::Compute);
            Shader blit_vs = compiler.compile(font_shaders::blitVertexShader(), ShaderStage::Vertex);
            Shader blit_fs = compiler.compile(font_shaders::blitFragmentShader(), ShaderStage::Fragment);

            if (!compute.valid() || !blit_vs.valid() || !blit_fs.valid())
            {
                printLine(Print::Error, "Font shader compilation failed.");
                if (!compute.log.empty()) printLine(Print::Error, "{}", compute.log);
                return;
            }

            computeShader = Compiler::createShaderModule(device, compute);
            blitVertexShader = Compiler::createShaderModule(device, blit_vs);
            blitFragmentShader = Compiler::createShaderModule(device, blit_fs);

            VkPushConstantRange pushConstantRange =
            {
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .offset = 0,
                .size = sizeof(BatchPushConstants),
            };

            VkDescriptorSetLayout computeSetLayouts[] = { canvasDescriptorSetLayout, batchDescriptorSetLayout };

            VkPipelineLayoutCreateInfo computeLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 2,
                .pSetLayouts = computeSetLayouts,
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = &pushConstantRange,
            };

            vkCreatePipelineLayout(device, &computeLayoutInfo, nullptr, &computePipelineLayout);

            VkPipelineLayoutCreateInfo blitLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 1,
                .pSetLayouts = &blitDescriptorSetLayout,
            };

            vkCreatePipelineLayout(device, &blitLayoutInfo, nullptr, &blitPipelineLayout);

            VkPipelineShaderStageCreateInfo computeStage =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = computeShader,
                .pName = "main",
            };

            VkComputePipelineCreateInfo computePipelineInfo =
            {
                .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                .stage = computeStage,
                .layout = computePipelineLayout,
            };

            vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline);

            VkPipelineShaderStageCreateInfo blitStages[] =
            {
                { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = blitVertexShader, .pName = "main" },
                { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = blitFragmentShader, .pName = "main" },
            };

            VkPipelineVertexInputStateCreateInfo vertexInputInfo { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
            VkPipelineInputAssemblyStateCreateInfo inputAssembly { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
            VkPipelineViewportStateCreateInfo viewportState { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .scissorCount = 1 };
            VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            VkPipelineDynamicStateCreateInfo dynamicState { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .dynamicStateCount = 2, .pDynamicStates = dynamicStates };
            VkPipelineRasterizationStateCreateInfo rasterizer { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_NONE, .lineWidth = 1.0f };
            VkPipelineMultisampleStateCreateInfo multisampling { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT };
            VkPipelineColorBlendAttachmentState overlayBlendAttachment
            {
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = 0xF,
            };
            VkPipelineColorBlendAttachmentState replaceBlendAttachment { .colorWriteMask = 0xF };
            VkPipelineColorBlendStateCreateInfo overlayBlending { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 1, .pAttachments = &overlayBlendAttachment };
            VkPipelineColorBlendStateCreateInfo replaceBlending { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 1, .pAttachments = &replaceBlendAttachment };

            VkFormat colorFormat = blitColorFormat;
            VkPipelineRenderingCreateInfo renderingCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &colorFormat,
            };

            VkGraphicsPipelineCreateInfo overlayBlitPipelineInfo =
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
                .pColorBlendState = &overlayBlending,
                .pDynamicState = &dynamicState,
                .layout = blitPipelineLayout,
            };

            vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &overlayBlitPipelineInfo, nullptr, &overlayBlitPipeline);

            VkGraphicsPipelineCreateInfo replaceBlitPipelineInfo =
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
                .pColorBlendState = &replaceBlending,
                .pDynamicState = &dynamicState,
                .layout = blitPipelineLayout,
            };

            vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &replaceBlitPipelineInfo, nullptr, &replaceBlitPipeline);
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

            vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
        }

        void createCanvas()
        {
            if (extent.width == 0 || extent.height == 0)
            {
                return;
            }

            VkImageCreateInfo imageInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .extent = { extent.width, extent.height, 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };

            canvasAllocation = allocator->createImage(imageInfo, MemoryUsage::GpuOnly);
            canvasImage = canvasAllocation.image;

            VkImageViewCreateInfo viewInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = canvasImage,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };

            vkCreateImageView(device, &viewInfo, nullptr, &canvasView);

            VkImageMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .image = canvasImage,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };

            VkCommandPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = queueFamily };
            VkCommandPool pool = VK_NULL_HANDLE;
            vkCreateCommandPool(device, &poolInfo, nullptr, &pool);

            VkCommandBufferAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
            VkCommandBuffer cmd = VK_NULL_HANDLE;
            vkAllocateCommandBuffers(device, &allocInfo, &cmd);

            VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
            vkBeginCommandBuffer(cmd, &beginInfo);
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            vkEndCommandBuffer(cmd);

            VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd };

            VkFence fence = VK_NULL_HANDLE;
            VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            vkCreateFence(device, &fenceInfo, nullptr, &fence);

            vkQueueSubmit(queue, 1, &submitInfo, fence);
            vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
            vkDestroyFence(device, fence, nullptr);

            vkFreeCommandBuffers(device, pool, 1, &cmd);
            vkDestroyCommandPool(device, pool, nullptr);

            updateCanvasDescriptor();
            updateBlitDescriptor();
        }

        void destroyCanvas()
        {
            if (canvasView)
            {
                vkDestroyImageView(device, canvasView, nullptr);
                canvasView = VK_NULL_HANDLE;
            }

            if (canvasAllocation)
            {
                allocator->destroyImage(canvasAllocation);
                canvasImage = VK_NULL_HANDLE;
            }
        }

        void updateCanvasDescriptor()
        {
            if (!canvasView)
            {
                return;
            }

            VkDescriptorImageInfo imageInfo = { VK_NULL_HANDLE, canvasView, VK_IMAGE_LAYOUT_GENERAL };
            VkWriteDescriptorSet write =
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = canvasDescriptorSet,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &imageInfo,
            };

            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }

        void updateBlitDescriptor()
        {
            if (!canvasView)
            {
                return;
            }

            VkDescriptorImageInfo imageInfo = { sampler, canvasView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            VkWriteDescriptorSet write =
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = blitDescriptorSet,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfo,
            };

            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }

        void transitionCanvas(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout,
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
                .image = canvasImage,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };

            vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }

        void createAtlasBuffers()
        {
            atlasContourCapacity = kAtlasInitialBytes;
            atlasCurveCapacity = kAtlasInitialBytes;
            atlasContourBytes = 0;
            atlasCurveBytes = 0;
            atlasContourCount = 0;
            atlasCurveCount = 0;

            atlasContours = allocator->createBuffer(atlasContourCapacity,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);
            atlasCurves = allocator->createBuffer(atlasCurveCapacity,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);
            updateAtlasDescriptors();
        }

        void createBatchBuffers()
        {
            instanceBufferCapacity = 64 * 1024;
            tileBufferCapacity = 16 * 1024;
            tileGlyphBufferCapacity = 64 * 1024;

            instanceBuffer = allocator->createBuffer(instanceBufferCapacity,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);
            tileBuffer = allocator->createBuffer(tileBufferCapacity,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);
            tileGlyphBuffer = allocator->createBuffer(tileGlyphBufferCapacity,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);
            updateBatchDescriptors();
        }

        void destroyAtlas()
        {
            if (atlasContours)
            {
                allocator->destroyBuffer(atlasContours);
            }

            if (atlasCurves)
            {
                allocator->destroyBuffer(atlasCurves);
            }

            atlasContourCapacity = 0;
            atlasCurveCapacity = 0;
            atlasContourBytes = 0;
            atlasCurveBytes = 0;
            atlasContourCount = 0;
            atlasCurveCount = 0;
        }

        void destroyBatchBuffers()
        {
            if (instanceBuffer)
            {
                allocator->destroyBuffer(instanceBuffer);
            }

            if (tileBuffer)
            {
                allocator->destroyBuffer(tileBuffer);
            }

            if (tileGlyphBuffer)
            {
                allocator->destroyBuffer(tileGlyphBuffer);
            }

            instanceBufferCapacity = 0;
            tileBufferCapacity = 0;
            tileGlyphBufferCapacity = 0;
        }

        void ensureAtlasCapacity(VkDeviceSize contour_bytes, VkDeviceSize curve_bytes)
        {
            if (contour_bytes <= atlasContourCapacity && curve_bytes <= atlasCurveCapacity)
            {
                return;
            }

            VkDeviceSize new_contour_capacity = std::max(contour_bytes, atlasContourCapacity * 2);
            VkDeviceSize new_curve_capacity = std::max(curve_bytes, atlasCurveCapacity * 2);
            new_contour_capacity = std::max(new_contour_capacity, kAtlasInitialBytes);
            new_curve_capacity = std::max(new_curve_capacity, kAtlasInitialBytes);

            vkDeviceWaitIdle(device);

            auto new_contours = allocator->createBuffer(new_contour_capacity,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);
            auto new_curves = allocator->createBuffer(new_curve_capacity,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);

            if (atlasContourBytes)
            {
                std::memcpy(new_contours.mapped, atlasContours.mapped, size_t(atlasContourBytes));
            }

            if (atlasCurveBytes)
            {
                std::memcpy(new_curves.mapped, atlasCurves.mapped, size_t(atlasCurveBytes));
            }

            if (atlasContours)
            {
                allocator->destroyBuffer(atlasContours);
            }

            if (atlasCurves)
            {
                allocator->destroyBuffer(atlasCurves);
            }

            atlasContours = new_contours;
            atlasCurves = new_curves;
            atlasContourCapacity = new_contour_capacity;
            atlasCurveCapacity = new_curve_capacity;
            updateAtlasDescriptors();
        }

        void ensureBatchBuffer(BufferAllocation& buffer, VkDeviceSize& capacity, VkDeviceSize required)
        {
            required = std::max<VkDeviceSize>(required, 16);
            if (required <= capacity)
            {
                return;
            }

            VkDeviceSize new_capacity = std::max(required, capacity * 2);
            if (!capacity)
            {
                new_capacity = required;
            }

            vkDeviceWaitIdle(device);

            auto new_buffer = allocator->createBuffer(new_capacity,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);

            if (buffer)
            {
                allocator->destroyBuffer(buffer);
            }

            buffer = new_buffer;
            capacity = new_capacity;
            updateBatchDescriptors();
        }

        void updateAtlasDescriptors()
        {
            VkDescriptorBufferInfo contourInfo = { atlasContours.buffer, 0, VK_WHOLE_SIZE };
            VkDescriptorBufferInfo curveInfo = { atlasCurves.buffer, 0, VK_WHOLE_SIZE };

            VkWriteDescriptorSet writes[2] =
            {
                { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = batchDescriptorSet, .dstBinding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &contourInfo },
                { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = batchDescriptorSet, .dstBinding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &curveInfo },
            };

            vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
        }

        void updateBatchDescriptors()
        {
            VkDescriptorBufferInfo instanceInfo = { instanceBuffer.buffer, 0, VK_WHOLE_SIZE };
            VkDescriptorBufferInfo tileInfo = { tileBuffer.buffer, 0, VK_WHOLE_SIZE };
            VkDescriptorBufferInfo indexInfo = { tileGlyphBuffer.buffer, 0, VK_WHOLE_SIZE };

            VkWriteDescriptorSet writes[3] =
            {
                { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = batchDescriptorSet, .dstBinding = 2, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &instanceInfo },
                { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = batchDescriptorSet, .dstBinding = 3, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &tileInfo },
                { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = batchDescriptorSet, .dstBinding = 4, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &indexInfo },
            };

            vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);
        }
    };

    FontRenderer::FontRenderer(const CreateInfo& info)
    {
        if (info.device == VK_NULL_HANDLE || info.allocator == nullptr)
        {
            MANGO_EXCEPTION("[FontRenderer] Invalid CreateInfo.");
        }

        m_impl = std::make_unique<Impl>(info.device, info.queue, info.queueFamily,
            *info.allocator, info.targetFormat);
    }

    FontRenderer::~FontRenderer()
    {
        if (m_impl && m_impl->device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_impl->device);
        }
    }

    Font FontRenderer::load(ConstMemory memory)
    {
        const u32 index = m_impl->allocateFaceSlot();
        FaceSlot& slot = m_impl->faces[index];

        if (!slot.face.load(memory))
        {
            return {};
        }

        slot.used = true;
        slot.pixelHeight = 32.0f;
        slot.face.setPixelHeight(slot.pixelHeight);

        Font font = { .index = index, .generation = slot.generation };
        return font;
    }

    Font FontRenderer::load(const std::string& path)
    {
        filesystem::File file(path);
        return load(ConstMemory(file));
    }

    Font FontRenderer::load(const filesystem::File& file)
    {
        return load(ConstMemory(file));
    }

    Font FontRenderer::load(const filesystem::Path& path, const std::string& filename)
    {
        filesystem::File file(path, filename);
        return load(ConstMemory(file));
    }

    void FontRenderer::unload(Font font)
    {
        FaceSlot* slot = m_impl->validateHandle(font);
        if (!slot)
        {
            return;
        }

        m_impl->removeGlyphCacheForFace(font.index);
        slot->generation++;
        slot->used = false;
    }

    void FontRenderer::setSize(Font font, float pixelHeight)
    {
        FaceSlot* slot = m_impl->validateHandle(font);
        if (!slot)
        {
            return;
        }

        slot->pixelHeight = pixelHeight;
        slot->face.setPixelHeight(pixelHeight);
        m_impl->removeGlyphCacheForFace(font.index);
    }

    float FontRenderer::size(Font font) const
    {
        const FaceSlot* slot = m_impl->validateHandle(font);
        return slot ? slot->pixelHeight : 0.0f;
    }

    float FontRenderer::lineHeight(Font font, const TextStyle& style) const
    {
        const FaceSlot* slot = m_impl->validateHandle(font);
        if (!slot)
        {
            return 0.0f;
        }

        const float h = effectivePixelHeight(*slot, style.pixelHeight);
        float height = 0.0f;
        const_cast<Impl*>(m_impl.get())->withFaceAtHeight(font.index, h, [&](font::Face& face, float)
        {
            height = float(face.ascent - face.descent + face.line_gap) * face.pixelScale() * style.lineSpacing;
        });
        return height;
    }

    float FontRenderer::ascender(Font font, const TextStyle& style) const
    {
        const FaceSlot* slot = m_impl->validateHandle(font);
        if (!slot)
        {
            return 0.0f;
        }

        const float h = effectivePixelHeight(*slot, style.pixelHeight);
        float asc = 0.0f;
        const_cast<Impl*>(m_impl.get())->withFaceAtHeight(font.index, h, [&](font::Face& face, float)
        {
            asc = float(face.ascent) * face.pixelScale();
        });
        return asc;
    }

    float FontRenderer::descender(Font font, const TextStyle& style) const
    {
        const FaceSlot* slot = m_impl->validateHandle(font);
        if (!slot)
        {
            return 0.0f;
        }

        const float h = effectivePixelHeight(*slot, style.pixelHeight);
        float desc = 0.0f;
        const_cast<Impl*>(m_impl.get())->withFaceAtHeight(font.index, h, [&](font::Face& face, float)
        {
            desc = float(face.descent) * face.pixelScale();
        });
        return desc;
    }

    void FontRenderer::warmup(Font font, std::string_view utf8)
    {
        FaceSlot* slot = m_impl->validateHandle(font);
        if (!slot)
        {
            return;
        }

        const std::u32string codepoints = utf32_from_utf8(utf8);
        for (char32_t cp : codepoints)
        {
            m_impl->cacheGlyph(font.index, u32(cp), slot->pixelHeight);
        }
    }

    TextMetrics FontRenderer::measure(Font font, std::string_view utf8, const TextStyle& style) const
    {
        TextMetrics metrics;
        const FaceSlot* slot = m_impl->validateHandle(font);
        if (!slot)
        {
            return metrics;
        }

        const float h = effectivePixelHeight(*slot, style.pixelHeight);
        const std::u32string codepoints = utf32_from_utf8(utf8);
        const_cast<Impl*>(m_impl.get())->withFaceAtHeight(font.index, h, [&](font::Face& face, float)
        {
            metrics.width = font::measureTextWidth(face, codepoints);
        });
        metrics.height = lineHeight(font, style);
        metrics.lineCount = 1;
        return metrics;
    }

    TextMetrics FontRenderer::measureParagraph(Font font, const math::Rectangle& bounds,
                                       std::string_view utf8, const ParagraphStyle& style) const
    {
        const FaceSlot* slot = m_impl->validateHandle(font);
        if (!slot)
        {
            return {};
        }

        const float h = effectivePixelHeight(*slot, style.pixelHeight);
        const std::u32string codepoints = utf32_from_utf8(utf8);
        TextMetrics metrics;
        const_cast<Impl*>(m_impl.get())->withFaceAtHeight(font.index, h, [&](font::Face& face, float)
        {
            metrics = font::layoutParagraph(face, bounds, codepoints, style).metrics;
        });
        return metrics;
    }

    float FontRenderer::textWidth(Font font, std::string_view utf8, const TextStyle& style) const
    {
        return measure(font, utf8, style).width;
    }

    void FontRenderer::resize(VkExtent2D extent)
    {
        m_impl->resize(extent);
    }

    void FontRenderer::beginFrame(float32x4 clearColor)
    {
        m_impl->frameClearColor = clearColor;
        m_impl->frameOpen = true;
        m_impl->pendingGlyphs.clear();
    }

    TextCursor FontRenderer::cursor(Font font, float x, float baseline_y) const
    {
        MANGO_UNREFERENCED(font);
        return TextCursor { x, baseline_y };
    }

    TextCursor FontRenderer::cursorTopLeft(Font font, float x, float top, const TextStyle& style) const
    {
        return TextCursor { x, top + ascender(font, style) };
    }

    void FontRenderer::draw(Font font, float x, float y, std::string_view utf8, const TextStyle& style)
    {
        FaceSlot* slot = m_impl->validateHandle(font);
        if (!slot)
        {
            return;
        }

        const std::u32string codepoints = utf32_from_utf8(utf8);
        m_impl->queueGlyphsForLine(font.index, x, y, codepoints, style.color, style.pixelHeight);
    }

    void FontRenderer::draw(TextCursor& cursor, Font font, std::string_view utf8, const TextStyle& style)
    {
        draw(font, cursor.x, cursor.y, utf8, style);
    }

    void FontRenderer::newline(TextCursor& cursor, Font font, const TextStyle& style)
    {
        cursor.y += lineHeight(font, style);
    }

    void FontRenderer::drawLine(TextCursor& cursor, Font font, std::string_view utf8, const TextStyle& style)
    {
        draw(cursor, font, utf8, style);
        newline(cursor, font, style);
    }

    void FontRenderer::drawParagraph(Font font, const math::Rectangle& bounds,
                           std::string_view utf8, const ParagraphStyle& style)
    {
        FaceSlot* slot = m_impl->validateHandle(font);
        if (!slot)
        {
            return;
        }

        const float h = effectivePixelHeight(*slot, style.pixelHeight);
        const std::u32string codepoints = utf32_from_utf8(utf8);
        m_impl->withFaceAtHeight(font.index, h, [&](font::Face& face, float height)
        {
            const font::LayoutResult layout = font::layoutParagraph(face, bounds, codepoints, style);
            for (const font::LayoutLine& line : layout.lines)
            {
                m_impl->queueGlyphsForLayoutLine(font.index, line, style.color, height);
            }
        });
    }

    void FontRenderer::drawParagraph(TextCursor& cursor, Font font, float width,
                             std::string_view utf8, const ParagraphStyle& style)
    {
        const float asc = ascender(font, style);
        const float top = cursor.y - asc;
        const float height = float(m_impl->extent.height) - top;
        const math::Rectangle bounds(cursor.x, top, width, std::max(height, 0.0f));

        FaceSlot* slot = m_impl->validateHandle(font);
        if (!slot)
        {
            return;
        }

        const float h = effectivePixelHeight(*slot, style.pixelHeight);
        const std::u32string codepoints = utf32_from_utf8(utf8);
        m_impl->withFaceAtHeight(font.index, h, [&](font::Face& face, float pixelHeight)
        {
            const font::LayoutResult layout = font::layoutParagraph(face, bounds, codepoints, style);
            for (const font::LayoutLine& line : layout.lines)
            {
                m_impl->queueGlyphsForLayoutLine(font.index, line, style.color, pixelHeight);
            }

            if (!layout.lines.empty())
            {
                cursor.y = layout.lines.back().glyphs[0].y + lineHeight(font, style);
            }
        });
    }

    void FontRenderer::encode(VkCommandBuffer cmd, const EncodeTarget& target)
    {
        const float32x4 clear = m_impl->frameClearColor;
        m_impl->clearCanvas(cmd, clear.x, clear.y, clear.z, clear.w);
        m_impl->flushTextBatch(cmd);
        m_impl->blitToTarget(cmd, target.imageView, target.extent, target.mode);
        m_impl->pendingGlyphs.clear();
        m_impl->frameOpen = false;
    }

} // namespace mango::vulkan

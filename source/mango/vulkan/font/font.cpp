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
#include <mango/vulkan/vulkan.hpp>
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
        static constexpr u32 kMaxFramesInFlight = 4;
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
            float stem_darkening = 1.0f;
            float32x4 color { 1.0f, 1.0f, 1.0f, 1.0f };
        };

        struct GlyphRect
        {
            float x0 = 0.0f;
            float y0 = 0.0f;
            float x1 = 0.0f;
            float y1 = 0.0f;
        };

        struct GlyphTileRange
        {
            u32 tx0 = 0;
            u32 ty0 = 0;
            u32 tx1 = 0;
            u32 ty1 = 0;
        };

        struct TextBounds
        {
            u32 x0 = 0;
            u32 y0 = 0;
            u32 x1 = 0;
            u32 y1 = 0;

            bool empty() const
            {
                return x1 <= x0 || y1 <= y0;
            }
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

        static u64 glyphCacheKey(u32 faceIndex, u32 codepoint)
        {
            return (u64(faceIndex) << 32) | u64(codepoint);
        }

        static float effectivePixelHeight(const FaceSlot& slot, float stylePixelHeight)
        {
            return stylePixelHeight > 0.0f ? stylePixelHeight : slot.pixelHeight;
        }

        static GlyphDrawParams makeGlyphDrawParams(float origin_x, float baseline_y,
                                                   const CachedGlyph& cached,
                                                   float pixel_scale, float em_per_pixel,
                                                   float32x4 color, float stemDarkening)
        {
            const float float_left = origin_x + cached.cpu.bounds_min.x * pixel_scale;
            const float float_right = origin_x + cached.cpu.bounds_max.x * pixel_scale;
            const float float_top = baseline_y - cached.cpu.bounds_max.y * pixel_scale;
            const float float_bottom = baseline_y - cached.cpu.bounds_min.y * pixel_scale;

            const float screen_x = std::floor(float_left);
            const float screen_y = std::floor(float_top);

            GlyphDrawParams params;
            params.screen_x = screen_x;
            params.screen_y = screen_y;
            params.em_window_x0 = cached.cpu.bounds_min.x + (screen_x - float_left) * em_per_pixel;
            params.em_window_y0 = (baseline_y - (screen_y + 1.0f)) * em_per_pixel;
            params.em_per_pixel = em_per_pixel;
            params.pixel_width = std::max(1u, u32(std::ceil(float_right - screen_x)));
            params.pixel_height = std::max(1u, u32(std::ceil(float_bottom - screen_y)));
            params.color = color;
            params.stem_darkening = stemDarkening;
            return params;
        }

    } // namespace

    struct FontRenderer::Impl
    {
        VkDevice device = VK_NULL_HANDLE;
        VkQueue queue = VK_NULL_HANDLE;
        u32 queueFamily = 0;
        Allocator* allocator = nullptr;

        VkExtent2D extent { 0, 0 };
        VkFormat targetFormat = VK_FORMAT_B8G8R8A8_UNORM;

        VkShaderModule computeShader = VK_NULL_HANDLE;

        VkDescriptorSetLayout targetDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout batchDescriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;
        VkPipeline computePipeline = VK_NULL_HANDLE;

        VkDescriptorPool targetDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorPool batchDescriptorPool = VK_NULL_HANDLE;
        std::unordered_map<VkImageView, VkDescriptorSet> targetDescriptors;
        VkDescriptorSet batchDescriptorSets[kMaxFramesInFlight] = {};

        BufferAllocation atlasContours;
        BufferAllocation atlasCurves;
        VkDeviceSize atlasContourCapacity = 0;
        VkDeviceSize atlasCurveCapacity = 0;
        VkDeviceSize atlasContourBytes = 0;
        VkDeviceSize atlasCurveBytes = 0;
        u32 atlasContourCount = 0;
        u32 atlasCurveCount = 0;
        bool atlasDescriptorsDirty = false;

        BufferAllocation instanceBuffers[kMaxFramesInFlight];
        BufferAllocation tileBuffers[kMaxFramesInFlight];
        BufferAllocation tileGlyphBuffers[kMaxFramesInFlight];
        VkDeviceSize instanceBufferCapacity[kMaxFramesInFlight] = {};
        VkDeviceSize tileBufferCapacity[kMaxFramesInFlight] = {};
        VkDeviceSize tileGlyphBufferCapacity[kMaxFramesInFlight] = {};
        static constexpr u32 kFramesInFlight = 2;
        u32 framesInFlight = kFramesInFlight;
        u32 batchSlot = 0;

        std::deque<FaceSlot> faces;
        std::unordered_map<u64, CachedGlyph> glyphCache;
        std::vector<GpuGlyphInstance> pendingInstances;
        std::vector<GlyphRect> pendingRects;
        std::u32string utf32Scratch;

        std::vector<GpuTileInfo> batchTileInfos;
        std::vector<u32> batchTileGlyphs;
        std::vector<u32> batchTileFill;
        std::vector<GlyphTileRange> glyphTileRanges;

        Impl(VkDevice dev, VkQueue q, u32 family, Allocator& alloc, VkFormat format)
            : device(dev)
            , queue(q)
            , queueFamily(family)
            , allocator(&alloc)
            , targetFormat(format)
            , framesInFlight(kFramesInFlight)
        {
            createDescriptorSetLayouts();
            createPipelines();
        }

        ~Impl()
        {
            if (device == VK_NULL_HANDLE)
            {
                return;
            }

            vkDeviceWaitIdle(device);

            if (computePipeline)
            {
                vkDestroyPipeline(device, computePipeline, nullptr);
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

            if (targetDescriptorPool)
            {
                vkDestroyDescriptorPool(device, targetDescriptorPool, nullptr);
            }

            if (batchDescriptorSetLayout)
            {
                vkDestroyDescriptorSetLayout(device, batchDescriptorSetLayout, nullptr);
            }

            if (targetDescriptorSetLayout)
            {
                vkDestroyDescriptorSetLayout(device, targetDescriptorSetLayout, nullptr);
            }

            if (computeShader)
            {
                vkDestroyShaderModule(device, computeShader, nullptr);
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
                if (u32(it->first >> 32) == faceIndex)
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

        void cacheGlyph(u32 faceIndex, u32 codepoint)
        {
            const u64 key = glyphCacheKey(faceIndex, codepoint);
            if (glyphCache.find(key) != glyphCache.end())
            {
                return;
            }

            CachedGlyph cached;
            cached.cpu = faces[faceIndex].face.loadGpuGlyph(codepoint);

            if (!cached.cpu.curve_bytes.empty())
            {
                cached.atlas = appendGlyphToAtlas(cached.cpu);
            }

            glyphCache.emplace(key, std::move(cached));
        }

        const CachedGlyph& getCachedGlyph(u32 faceIndex, u32 codepoint)
        {
            const u64 key = glyphCacheKey(faceIndex, codepoint);
            auto it = glyphCache.find(key);
            if (it == glyphCache.end())
            {
                cacheGlyph(faceIndex, codepoint);
                it = glyphCache.find(key);
            }
            return it->second;
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

            atlasDescriptorsDirty = true;
            return slot;
        }

        void queueGlyphsForLine(u32 faceIndex, float pen_x, float baseline_y,
                                const std::u32string& codepoints, float32x4 color, float pixelHeight,
                                float stemDarkening)
        {
            if (codepoints.empty())
            {
                return;
            }

            withFaceAtHeight(faceIndex, pixelHeight, [&](font::Face& face, float)
            {
                const float pixel_scale = face.pixelScale();
                const float em_per_pixel = face.emPerPixel();

                for (size_t i = 0; i < codepoints.size(); ++i)
                {
                    const u32 cp = u32(codepoints[i]);
                    const u32 next = (i + 1 < codepoints.size()) ? u32(codepoints[i + 1]) : 0;
                    const CachedGlyph& cached = getCachedGlyph(faceIndex, cp);

                    if (cached.cpu.curve_bytes.empty())
                    {
                        pen_x += cached.cpu.advance * pixel_scale;
                        if (next)
                        {
                            pen_x += float(face.kerning(cp, next)) * pixel_scale;
                        }
                        continue;
                    }

                    queueGlyph(cached.atlas,
                        makeGlyphDrawParams(pen_x, baseline_y, cached, pixel_scale, em_per_pixel, color, stemDarkening));

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

            const u32 pixel_width = std::max(1u, params.pixel_width);
            const u32 pixel_height = std::max(1u, params.pixel_height);

            GpuGlyphInstance gpu {};
            gpu.header[0] = float(atlas.contour_offset);
            gpu.header[1] = float(atlas.contour_count);
            gpu.header[2] = params.stem_darkening;
            gpu.color[0] = params.color.x;
            gpu.color[1] = params.color.y;
            gpu.color[2] = params.color.z;
            gpu.color[3] = params.color.w;
            gpu.params0[0] = params.em_window_x0;
            gpu.params0[1] = params.em_window_y0;
            gpu.params0[2] = params.em_per_pixel;
            gpu.params0[3] = params.em_per_pixel;
            gpu.params1[0] = params.screen_x;
            gpu.params1[1] = params.screen_y;
            gpu.params1[2] = float(pixel_width);
            gpu.params1[3] = float(pixel_height);

            pendingInstances.push_back(gpu);
            pendingRects.push_back(
            {
                params.screen_x,
                params.screen_y,
                params.screen_x + float(pixel_width),
                params.screen_y + float(pixel_height),
            });
        }

        void queueGlyphsForLayoutLine(u32 faceIndex, const font::LayoutLine& line, float32x4 color,
                                      float pixelHeight, float stemDarkening)
        {
            if (line.glyphs.empty())
            {
                return;
            }

            withFaceAtHeight(faceIndex, pixelHeight, [&](font::Face& face, float)
            {
                const float pixel_scale = face.pixelScale();
                const float em_per_pixel = face.emPerPixel();

                for (const font::PositionedGlyph& pg : line.glyphs)
                {
                    const CachedGlyph& cached = getCachedGlyph(faceIndex, pg.codepoint);

                    if (cached.cpu.curve_bytes.empty())
                    {
                        continue;
                    }

                    queueGlyph(cached.atlas,
                        makeGlyphDrawParams(pg.x, pg.y, cached, pixel_scale, em_per_pixel, color, stemDarkening));
                }
            });
        }

        void resize(VkExtent2D newExtent)
        {
            if (newExtent.width == 0 || newExtent.height == 0)
            {
                return;
            }

            extent = newExtent;
            batchSlot = 0;
            resetTargetDescriptors();
        }

        void resetTargetDescriptors()
        {
            targetDescriptors.clear();
            if (targetDescriptorPool)
            {
                vkResetDescriptorPool(device, targetDescriptorPool, 0);
            }
        }

        VkDescriptorSet allocateTargetDescriptorSet()
        {
            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
            VkDescriptorSetAllocateInfo allocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = targetDescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &targetDescriptorSetLayout,
            };

            VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
            if (result != VK_SUCCESS)
            {
                printLine(Print::Warning, "FontRenderer: target descriptor pool exhausted, resetting");
                resetTargetDescriptors();
                result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
                if (result != VK_SUCCESS)
                {
                    printLine(Print::Error, "FontRenderer: vkAllocateDescriptorSets failed: {}", getString(result));
                    return VK_NULL_HANDLE;
                }
            }

            return descriptorSet;
        }

        void writeTargetDescriptor(VkDescriptorSet descriptorSet, VkImageView imageView)
        {
            VkDescriptorImageInfo imageInfo = { VK_NULL_HANDLE, imageView, VK_IMAGE_LAYOUT_GENERAL };
            VkWriteDescriptorSet write =
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSet,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &imageInfo,
            };

            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }

        VkDescriptorSet getTargetDescriptorSet(VkImageView imageView)
        {
            if (!imageView)
            {
                return VK_NULL_HANDLE;
            }

            const auto found = targetDescriptors.find(imageView);
            if (found != targetDescriptors.end())
            {
                return found->second;
            }

            const VkDescriptorSet descriptorSet = allocateTargetDescriptorSet();
            if (!descriptorSet)
            {
                return VK_NULL_HANDLE;
            }

            writeTargetDescriptor(descriptorSet, imageView);
            targetDescriptors.emplace(imageView, descriptorSet);
            return descriptorSet;
        }

        TextBounds computePendingBounds(VkExtent2D clip_extent) const
        {
            TextBounds bounds;
            if (pendingRects.empty() || clip_extent.width == 0 || clip_extent.height == 0)
            {
                return bounds;
            }

            float x0 = pendingRects[0].x0;
            float y0 = pendingRects[0].y0;
            float x1 = pendingRects[0].x1;
            float y1 = pendingRects[0].y1;

            for (const GlyphRect& rect : pendingRects)
            {
                x0 = std::min(x0, rect.x0);
                y0 = std::min(y0, rect.y0);
                x1 = std::max(x1, rect.x1);
                y1 = std::max(y1, rect.y1);
            }

            bounds.x0 = u32(std::max(0.0f, std::floor(x0)));
            bounds.y0 = u32(std::max(0.0f, std::floor(y0)));
            bounds.x1 = u32(std::min(float(clip_extent.width), std::ceil(x1)));
            bounds.y1 = u32(std::min(float(clip_extent.height), std::ceil(y1)));
            return bounds;
        }

        void flushTextBatch(VkCommandBuffer cmd, const TextBounds& bounds, VkDescriptorSet targetSet, u32 frameSlot)
        {
            if (pendingInstances.empty() || bounds.empty())
            {
                return;
            }

            if (atlasDescriptorsDirty)
            {
                updateAtlasDescriptors();
                atlasDescriptorsDirty = false;
            }

            const u32 tile_origin_x = bounds.x0 / kTileSize * kTileSize;
            const u32 tile_origin_y = bounds.y0 / kTileSize * kTileSize;
            const u32 tile_size = kTileSize;
            const u32 tiles_x = (bounds.x1 - tile_origin_x + tile_size - 1) / tile_size;
            const u32 tiles_y = (bounds.y1 - tile_origin_y + tile_size - 1) / tile_size;

            const u32 tile_count = tiles_x * tiles_y;
            batchTileInfos.assign(tile_count, {});

            const u32 glyph_count = u32(pendingRects.size());
            glyphTileRanges.resize(glyph_count);

            const float origin_x = float(tile_origin_x);
            const float origin_y = float(tile_origin_y);

            for (u32 i = 0; i < glyph_count; ++i)
            {
                const GlyphRect& rect = pendingRects[i];
                GlyphTileRange& range = glyphTileRanges[i];

                range.tx0 = u32(std::max(0.0f, rect.x0 - origin_x)) / tile_size;
                range.ty0 = u32(std::max(0.0f, rect.y0 - origin_y)) / tile_size;
                range.tx1 = std::min(tiles_x - 1, u32(std::max(0.0f, rect.x1 - 1.0f - origin_x)) / tile_size);
                range.ty1 = std::min(tiles_y - 1, u32(std::max(0.0f, rect.y1 - 1.0f - origin_y)) / tile_size);

                for (u32 ty = range.ty0; ty <= range.ty1; ++ty)
                {
                    for (u32 tx = range.tx0; tx <= range.tx1; ++tx)
                    {
                        batchTileInfos[ty * tiles_x + tx].glyph_count++;
                    }
                }
            }

            u32 glyph_index_count = 0;
            for (GpuTileInfo& tile : batchTileInfos)
            {
                tile.glyph_start = glyph_index_count;
                glyph_index_count += tile.glyph_count;
            }

            batchTileGlyphs.resize(std::max(glyph_index_count, 1u));
            batchTileFill.assign(tile_count, 0);

            for (u32 i = 0; i < glyph_count; ++i)
            {
                const GlyphTileRange& range = glyphTileRanges[i];

                for (u32 ty = range.ty0; ty <= range.ty1; ++ty)
                {
                    for (u32 tx = range.tx0; tx <= range.tx1; ++tx)
                    {
                        const u32 tile_index = ty * tiles_x + tx;
                        GpuTileInfo& tile = batchTileInfos[tile_index];
                        batchTileGlyphs[tile.glyph_start + batchTileFill[tile_index]++] = i;
                    }
                }
            }

            const VkDeviceSize instance_bytes = sizeof(GpuGlyphInstance) * pendingInstances.size();
            const VkDeviceSize tile_bytes = sizeof(GpuTileInfo) * batchTileInfos.size();
            const VkDeviceSize index_bytes = sizeof(u32) * batchTileGlyphs.size();

            const u32 slot = frameSlot % framesInFlight;
            BufferAllocation& instanceBuffer = instanceBuffers[slot];
            BufferAllocation& tileBuffer = tileBuffers[slot];
            BufferAllocation& tileGlyphBuffer = tileGlyphBuffers[slot];

            ensureBatchBuffer(slot, instanceBuffer, instanceBufferCapacity[slot], instance_bytes);
            ensureBatchBuffer(slot, tileBuffer, tileBufferCapacity[slot], tile_bytes);
            ensureBatchBuffer(slot, tileGlyphBuffer, tileGlyphBufferCapacity[slot], index_bytes);

            std::memcpy(instanceBuffer.mapped, pendingInstances.data(), size_t(instance_bytes));
            std::memcpy(tileBuffer.mapped, batchTileInfos.data(), size_t(tile_bytes));
            std::memcpy(tileGlyphBuffer.mapped, batchTileGlyphs.data(), size_t(index_bytes));

            VkBufferMemoryBarrier bufferBarriers[3] =
            {
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .buffer = instanceBuffer.buffer,
                    .size = instance_bytes,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .buffer = tileBuffer.buffer,
                    .size = tile_bytes,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .buffer = tileGlyphBuffer.buffer,
                    .size = index_bytes,
                },
            };

            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0, 0, nullptr, 3, bufferBarriers, 0, nullptr);

            BatchPushConstants push {};
            push.tile_origin_x = tile_origin_x;
            push.tile_origin_y = tile_origin_y;
            push.tiles_x = tiles_x;
            push.tiles_y = tiles_y;
            push.tile_size = tile_size;
            push.workgroup_size = 8;

            const u32 subgroups_per_axis = (tile_size + push.workgroup_size - 1) / push.workgroup_size;

            VkDescriptorSet sets[] = { targetSet, batchDescriptorSets[slot] };

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 2, sets, 0, nullptr);
            vkCmdPushConstants(cmd, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
            vkCmdDispatch(cmd, tiles_x * subgroups_per_axis, tiles_y * subgroups_per_axis, 1);
        }

        void createDescriptorSetLayouts()
        {
            VkDescriptorSetLayoutBinding targetBinding =
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            };

            VkDescriptorSetLayoutCreateInfo targetLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 1,
                .pBindings = &targetBinding,
            };

            vkCreateDescriptorSetLayout(device, &targetLayoutInfo, nullptr, &targetDescriptorSetLayout);

            VkDescriptorSetLayoutBinding batchBindings[] =
            {
                { .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
                { .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
                { .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
                { .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
                { .binding = 4, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
            };

            VkDescriptorBindingFlags batchBindingFlags[] =
            {
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
            };

            VkDescriptorSetLayoutBindingFlagsCreateInfo batchBindingFlagsInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
                .bindingCount = 5,
                .pBindingFlags = batchBindingFlags,
            };

            VkDescriptorSetLayoutCreateInfo batchLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = &batchBindingFlagsInfo,
                .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
                .bindingCount = 5,
                .pBindings = batchBindings,
            };

            vkCreateDescriptorSetLayout(device, &batchLayoutInfo, nullptr, &batchDescriptorSetLayout);

            VkDescriptorPoolSize targetPoolSize = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8 };
            VkDescriptorPoolCreateInfo targetPoolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 8,
                .poolSizeCount = 1,
                .pPoolSizes = &targetPoolSize,
            };

            vkCreateDescriptorPool(device, &targetPoolInfo, nullptr, &targetDescriptorPool);

            VkDescriptorPoolSize batchPoolSizes[] =
            {
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5 * framesInFlight },
            };

            VkDescriptorPoolCreateInfo batchPoolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
                .maxSets = framesInFlight,
                .poolSizeCount = 1,
                .pPoolSizes = batchPoolSizes,
            };

            vkCreateDescriptorPool(device, &batchPoolInfo, nullptr, &batchDescriptorPool);

            VkDescriptorSetLayout batchLayouts[kMaxFramesInFlight] = {};
            for (u32 i = 0; i < framesInFlight; ++i)
            {
                batchLayouts[i] = batchDescriptorSetLayout;
            }

            VkDescriptorSetAllocateInfo batchAllocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = batchDescriptorPool,
                .descriptorSetCount = framesInFlight,
                .pSetLayouts = batchLayouts,
            };

            vkAllocateDescriptorSets(device, &batchAllocInfo, batchDescriptorSets);

            createAtlasBuffers();
            createBatchBuffers();
        }

        void createPipelines()
        {
            Compiler compiler;
            Shader compute = compiler.compile(font_shaders::computeShader(), ShaderStage::Compute);

            if (!compute.valid())
            {
                printLine(Print::Error, "Font shader compilation failed.");
                if (!compute.log.empty()) printLine(Print::Error, "{}", compute.log);
                return;
            }

            computeShader = Compiler::createShaderModule(device, compute);

            VkPushConstantRange pushConstantRange =
            {
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .offset = 0,
                .size = sizeof(BatchPushConstants),
            };

            VkDescriptorSetLayout computeSetLayouts[] = { targetDescriptorSetLayout, batchDescriptorSetLayout };

            VkPipelineLayoutCreateInfo computeLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 2,
                .pSetLayouts = computeSetLayouts,
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = &pushConstantRange,
            };

            vkCreatePipelineLayout(device, &computeLayoutInfo, nullptr, &computePipelineLayout);

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
            for (u32 i = 0; i < framesInFlight; ++i)
            {
                instanceBufferCapacity[i] = 64 * 1024;
                tileBufferCapacity[i] = 16 * 1024;
                tileGlyphBufferCapacity[i] = 64 * 1024;

                instanceBuffers[i] = allocator->createBuffer(instanceBufferCapacity[i],
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);
                tileBuffers[i] = allocator->createBuffer(tileBufferCapacity[i],
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);
                tileGlyphBuffers[i] = allocator->createBuffer(tileGlyphBufferCapacity[i],
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::Upload, true);
                updateBatchDescriptors(i);
            }
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
            for (u32 i = 0; i < framesInFlight; ++i)
            {
                if (instanceBuffers[i])
                {
                    allocator->destroyBuffer(instanceBuffers[i]);
                }

                if (tileBuffers[i])
                {
                    allocator->destroyBuffer(tileBuffers[i]);
                }

                if (tileGlyphBuffers[i])
                {
                    allocator->destroyBuffer(tileGlyphBuffers[i]);
                }

                instanceBufferCapacity[i] = 0;
                tileBufferCapacity[i] = 0;
                tileGlyphBufferCapacity[i] = 0;
            }
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

        void ensureBatchBuffer(u32 slot, BufferAllocation& buffer, VkDeviceSize& capacity, VkDeviceSize required)
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
            updateBatchDescriptors(slot);
        }

        void updateAtlasDescriptors()
        {
            VkDescriptorBufferInfo contourInfo = { atlasContours.buffer, 0, VK_WHOLE_SIZE };
            VkDescriptorBufferInfo curveInfo = { atlasCurves.buffer, 0, VK_WHOLE_SIZE };

            VkWriteDescriptorSet writes[kMaxFramesInFlight * 2] = {};
            for (u32 i = 0; i < framesInFlight; ++i)
            {
                writes[i * 2 + 0] =
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = batchDescriptorSets[i],
                    .dstBinding = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &contourInfo,
                };
                writes[i * 2 + 1] =
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = batchDescriptorSets[i],
                    .dstBinding = 1,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &curveInfo,
                };
            }

            vkUpdateDescriptorSets(device, framesInFlight * 2, writes, 0, nullptr);
        }

        void updateBatchDescriptors(u32 slot)
        {
            VkDescriptorBufferInfo instanceInfo = { instanceBuffers[slot].buffer, 0, VK_WHOLE_SIZE };
            VkDescriptorBufferInfo tileInfo = { tileBuffers[slot].buffer, 0, VK_WHOLE_SIZE };
            VkDescriptorBufferInfo indexInfo = { tileGlyphBuffers[slot].buffer, 0, VK_WHOLE_SIZE };

            VkWriteDescriptorSet writes[3] =
            {
                { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = batchDescriptorSets[slot], .dstBinding = 2, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &instanceInfo },
                { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = batchDescriptorSets[slot], .dstBinding = 3, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &tileInfo },
                { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = batchDescriptorSets[slot], .dstBinding = 4, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &indexInfo },
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

        const float rounded = std::max(1.0f, std::round(pixelHeight));
        if (rounded == std::max(1.0f, std::round(slot->pixelHeight)))
        {
            return;
        }

        slot->pixelHeight = pixelHeight;
        slot->face.setPixelHeight(rounded);
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
            m_impl->cacheGlyph(font.index, u32(cp));
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

    void FontRenderer::beginFrame()
    {
        m_impl->pendingInstances.clear();
        m_impl->pendingRects.clear();
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

        m_impl->utf32Scratch = utf32_from_utf8(utf8);
        m_impl->queueGlyphsForLine(font.index, x, y, m_impl->utf32Scratch, style.color, style.pixelHeight, style.stemDarkening);
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

        m_impl->utf32Scratch = utf32_from_utf8(utf8);
        const float h = effectivePixelHeight(*slot, style.pixelHeight);
        m_impl->withFaceAtHeight(font.index, h, [&](font::Face& face, float height)
        {
            const font::LayoutResult layout = font::layoutParagraph(face, bounds, m_impl->utf32Scratch, style);
            for (const font::LayoutLine& line : layout.lines)
            {
                m_impl->queueGlyphsForLayoutLine(font.index, line, style.color, height, style.stemDarkening);
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
        m_impl->utf32Scratch = utf32_from_utf8(utf8);
        m_impl->withFaceAtHeight(font.index, h, [&](font::Face& face, float pixelHeight)
        {
            const font::LayoutResult layout = font::layoutParagraph(face, bounds, m_impl->utf32Scratch, style);
            for (const font::LayoutLine& line : layout.lines)
            {
                m_impl->queueGlyphsForLayoutLine(font.index, line, style.color, pixelHeight, style.stemDarkening);
            }

            if (!layout.lines.empty())
            {
                cursor.y = layout.lines.back().glyphs[0].y + lineHeight(font, style);
            }
        });
    }

    void FontRenderer::encode(VkCommandBuffer cmd, const EncodeTarget& target)
    {
        if (m_impl->pendingInstances.empty())
        {
            return;
        }

        if (!m_impl->computePipeline || !target.imageView)
        {
            return;
        }

        if (target.extent.width > 0 && target.extent.height > 0)
        {
            m_impl->extent = target.extent;
        }

        const TextBounds bounds = m_impl->computePendingBounds(target.extent);
        if (bounds.empty())
        {
            m_impl->pendingInstances.clear();
            m_impl->pendingRects.clear();
            return;
        }

        const VkDescriptorSet targetSet = m_impl->getTargetDescriptorSet(target.imageView);
        if (!targetSet)
        {
            return;
        }

        const u32 batchSlot = m_impl->batchSlot;
        m_impl->flushTextBatch(cmd, bounds, targetSet, batchSlot);
        m_impl->batchSlot = (batchSlot + 1) % m_impl->framesInFlight;
        m_impl->pendingInstances.clear();
        m_impl->pendingRects.clear();
    }

} // namespace mango::vulkan

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <mango/core/configure.hpp>
#include <mango/core/memory.hpp>
#include <mango/image/color.hpp>
#include <mango/vulkan/allocator.hpp>
#include <mango/vulkan/staging_arena.hpp>

namespace mango::vulkan
{

    class VulkanContext;

    // -------------------------------------------------------------------
    // TextureDecoder
    // -------------------------------------------------------------------
    //
    // Decodes any image format mango can read into a sampled VkImage.
    //
    // decode() parses the header, creates the VkImage and returns it right away;
    // the pixels land asynchronously. Baseline/extended Huffman JPEG decodes with
    // Vulkan compute; everything else decodes on the CPU (ImageDecoder) and is
    // uploaded with a copy. Images larger than maxImageDimension2D are downscaled
    // on the CPU so every file produces a texture.
    //
    // The callback fires exactly once per handle from process(), on the thread
    // that calls process(). Status Ready means the image is in
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL and safe to sample — that is the
    // gpujpeg READY moment (texture available), not when a swapchain frame shows
    // it. The callback is also the place to release anything the caller tied to
    // the decode (the compressed file memory, for example). The caller owns the
    // image and destroys it with Allocator::destroyImage() once the callback has
    // fired; after Failed or Cancelled the image holds undefined contents.
    //
    // decode(), cancel() and pending() are thread-safe. process() records and
    // submits GPU work on the queue given at construction, so call it from the
    // thread that owns that queue. The decoder never blocks the caller: the wake
    // hook signals that process() has work (a finished CPU decode or a retired
    // GPU job), so an event-driven loop can sleep until then.
    //
    // The compressed memory passed to decode() must stay valid until the
    // callback fires.

    enum class TextureStatus
    {
        Ready,
        Failed,
        Cancelled,
    };

    enum class TextureDecodePath
    {
        None,       // header failed: no image
        Compute,    // JPEG entropy + IDCT + colour on Vulkan compute
        Cpu,        // ImageDecoder + staging upload
    };

    struct TextureInfo
    {
        u32 width = 0;              // texture dimensions (after downscale)
        u32 height = 0;
        u32 source_width = 0;       // file dimensions
        u32 source_height = 0;

        VkFormat format = VK_FORMAT_UNDEFINED;          // view format to sample with
        VkFormat image_format = VK_FORMAT_UNDEFINED;    // VkImage creation format (mutable to format)

        bool linear = false;        // stored values are linear (format is not an _SRGB view)
        bool needs_tonemap = false; // scene-linear values can exceed display range
        image::ColorInfo color;     // source colour signalling
    };

    // Everything one decode did, for diagnostics and tuning. Times are wall
    // milliseconds measured from the decode() call.
    struct TextureReport
    {
        TextureDecodePath path = TextureDecodePath::None;
        std::string decoder;            // "jpeg-compute", "cpu"
        std::string fallback_reason;    // why a JPEG did not take the compute path
        std::string source_format;      // e.g. "JPEG Baseline DCT 4:2:0", "RGBA16F"

        bool downscaled = false;

        bool icc_present = false;
        std::string icc_description;    // profile 'desc' tag
        std::string icc_handling;       // what was done with the profile

        bool color_transform = false;   // linearize() baked to scene-linear BT.709 fp16
        std::string color_source;       // e.g. "BT.709 sRGB", "BT.2020 PQ"
        bool hdr = false;               // float or HDR transfer content
        bool ultrahdr = false;          // JPEG gain map applied by the CPU decoder

        double header_ms = 0.0;         // decode(): header parse and colour plan
        double image_ms = 0.0;          // decode(): VkImage creation
        double prepare_ms = 0.0;        // compute: classify + entropy extract + host tables
        double decode_ms = 0.0;         // cpu: ImageDecoder::decode
        double convert_ms = 0.0;        // cpu: linearize()
        double downscale_ms = 0.0;      // cpu: downscale
        double staging_ms = 0.0;        // cpu: copy into the staging buffer
        double compile_ms = 0.0;        // compute: pipelines this decode had to build first

        // compute: prepare_ms split
        double prepare_parse_ms = 0.0;      // JpegDecoder header parse + classify
        double prepare_extract_ms = 0.0;    // entropy segment extraction
        double prepare_unstuff_ms = 0.0;    // unstuff + chunk/segment tables
        double prepare_tables_ms = 0.0;     // Huffman tables
        double prepare_pipelines_ms = 0.0;  // pipeline lookup (includes compile_ms)
        double prepare_alloc_ms = 0.0;      // staging buffer allocation
        double prepare_upload_ms = 0.0;     // byteswap + copy into staging

        double submit_ms = 0.0;         // decode() call to vkQueueSubmit (includes waiting for process())
        double gpu_ms = 0.0;            // GPU execution (timestamps, compute only)

        // When the VkImage is sampleable (fence). Comparable to gpujpeg READY:
        // wall time of the GPU work after submit, not when a frame presents it.
        // Many textures can be Ready before any of them is drawn.
        double ready_ms = 0.0;          // vkQueueSubmit → fence

        // When process() delivers the callback. Later than ready_ms when the
        // owner thread was busy (e.g. presenting); not a GPU metric.
        double visible_ms = 0.0;        // decode() call → callback

        size_t peak_host_bytes = 0;     // largest simultaneous CPU allocation for this decode
        size_t staging_bytes = 0;       // host-visible bytes used for the upload
        size_t staging_reserved_bytes = 0;  // compute: staging arena size after this prepare
        size_t image_bytes = 0;         // VkImage memory requirement
    };

    struct TextureResult
    {
        u64 handle = 0;
        TextureStatus status = TextureStatus::Failed;
        ImageAllocation image;
        TextureInfo info;
        TextureReport report;
        std::string error;
    };

    struct TextureDecoderConfig
    {
        // JPEGs below this pixel count decode on the CPU, where they are already fast.
        u64 compute_min_pixels = 1024 * 1024;

        // Set false to force every file through the CPU path.
        bool compute = true;

        // Compute shader variants compile in the background. By default a JPEG
        // whose variant is still compiling takes the CPU path (reported as
        // "compute pipelines compiling"), so no decode waits for the compiler.
        // Set true to wait instead, e.g. for benchmarking the compute path.
        bool wait_for_pipelines = false;

        // Compute decodes recorded but not yet retired. Each owns a slot of device
        // buffers sized to the largest JPEG it has decoded (132 bytes per 8x8
        // block, roughly 75 MB for a 24 MP 4:2:0 frame).
        int max_inflight_compute = 2;

        // Concurrent CPU decodes. Each one is multithreaded internally.
        int max_concurrent_cpu = 2;

        // Host-visible upload memory for compute decodes. initial_chunk is
        // reserved in the background at construction; the arena then stays
        // allocated until the decoder is destroyed or trimStaging(). At the
        // limit, preparation waits for retired decodes instead of growing.
        StagingArenaConfig staging;

        // Called from any thread when process() has new work. Must be thread-safe
        // and cheap (post an event, signal a condition variable).
        std::function<void()> wake;
    };

    class TextureDecoder : private NonCopyable
    {
    public:
        using Handle = u64;
        using Callback = std::function<void(const TextureResult& result)>;

        struct Texture
        {
            Handle handle = 0;      // 0 when the header failed; the callback does not fire
            ImageAllocation image;
            TextureInfo info;
            std::string error;

            explicit operator bool () const
            {
                return handle != 0;
            }
        };

        TextureDecoder(VulkanContext& context, Allocator& allocator,
                       const TextureDecoderConfig& config = TextureDecoderConfig());

        TextureDecoder(VkPhysicalDevice physicalDevice, VkDevice device,
                       VkQueue queue, u32 queueFamilyIndex, Allocator& allocator,
                       const TextureDecoderConfig& config = TextureDecoderConfig());

        // Cancels everything, waits for in-flight work and fires the remaining
        // callbacks with Cancelled.
        ~TextureDecoder();

        // filename selects the decoder by extension (a bare ".jpg" works too).
        Texture decode(ConstMemory memory, const std::string& filename, Callback callback);

        // The callback still fires, with Cancelled. CPU decodes stop early; GPU work
        // already submitted runs to completion before the callback.
        void cancel(Handle handle);

        // Owner thread: submit ready work, retire finished work, fire callbacks.
        void process();

        // Owner thread: process() until every decode has fired its callback.
        void wait();

        // Decodes whose callback has not fired yet.
        size_t pending() const;

        // Compute upload memory: usage and reservation over the decoder lifetime.
        StagingArenaStats stagingStats() const;

        // Frees the staging chunks no decode is using.
        void trimStaging();

    private:
        struct Impl;
        std::unique_ptr<Impl> m;
    };

    // Sampled view in info.format. Use this (or chain VkImageViewUsageCreateInfo
    // with SAMPLED) for compute-decoded images: they carry STORAGE usage on a
    // mutable UNORM image that the _SRGB view format cannot express.
    VkImageView createTextureView(VkDevice device, const ImageAllocation& image, const TextureInfo& info);

    std::string_view getString(TextureStatus status);
    std::string_view getString(TextureDecodePath path);

} // namespace mango::vulkan

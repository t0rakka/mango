/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <mango/image/jpeg.hpp>
#include <mango/vulkan/allocator.hpp>
#include <mango/vulkan/staging_arena.hpp>

namespace mango::vulkan::detail
{

    // Push constants shared by every pipeline; mirrors the GLSL block in the shaders.
    struct JpegComputePush
    {
        s32 nch;
        s32 pass;
        s32 rounds;
        s32 bpm;
        u32 dc_pack;
        u32 ac_pack;
        u32 comp_pack;
        u32 nwords;
        s32 width, height, xmcu, xblock, yblock, components;
        s32 y_hsf, y_vsf, y_qt, y_off;
        s32 cb_hsf, cb_vsf, cb_qt, cb_off;
        s32 cr_hsf, cr_vsf, cr_qt, cr_off;
        s32 mcus_per_group;
        s32 nmcu;
    };

    struct JpegComputePipelines
    {
        VkPipeline sync = VK_NULL_HANDLE;
        VkPipeline prefix = VK_NULL_HANDLE;
        VkPipeline write = VK_NULL_HANDLE;
        VkPipeline dc_local = VK_NULL_HANDLE;
        VkPipeline dc_groups = VK_NULL_HANDLE;
        VkPipeline idct = VK_NULL_HANDLE;
    };

    constexpr int kJpegComputeBindings = 17;

    // One prepared JPEG: host-side tables and the unstuffed scan in a staging
    // buffer, ready to record. Built on a worker thread, recorded on the owner.
    struct JpegComputeJob
    {
        JpegComputePush push {};
        JpegComputePipelines pipelines;

        int chunks = 0;
        int segments = 0;
        int dc_groups = 0;

        StagingRange staging;
        size_t staging_bytes = 0;

        // byte offset and size of each uploaded binding inside staging (size 0 = not uploaded)
        VkDeviceSize src_offset[kJpegComputeBindings] {};
        VkDeviceSize src_size[kJpegComputeBindings] {};

        // device bytes each binding needs
        VkDeviceSize need[kJpegComputeBindings] {};

        double compile_ms = 0.0;

        // prepare() wall time split
        double extract_ms = 0.0;        // extractEntropy
        double unstuff_ms = 0.0;        // unstuff + chunk/segment tables
        double tables_ms = 0.0;         // Huffman tables
        double pipelines_ms = 0.0;      // resolvePipelines (includes compile_ms)
        double alloc_ms = 0.0;          // staging acquire (includes waiting for space)
        double upload_ms = 0.0;         // byteswap + memcpy + flush into staging

        int slot = -1;
    };

    class JpegCompute : private NonCopyable
    {
    public:
        // staging must outlive this object.
        JpegCompute(VkPhysicalDevice physicalDevice, VkDevice device, Allocator& allocator,
                    StagingArena& staging, int slots);
        ~JpegCompute();

        // The device can imageStore into R8G8B8A8_UNORM.
        bool supported() const { return m_supported; }

        // nullptr when the frame fits the compute decoder, else the reason it does not.
        static const char* classify(const image::jpeg::JpegInfo& info);

        // Any thread. Extracts the entropy stream, builds the tables, fills the
        // staging buffer and compiles missing pipelines.
        // wait_for_pipelines = false fails with "compute pipelines compiling"
        // when a variant is still being built.
        bool prepare(image::jpeg::JpegDecoder& decoder, JpegComputeJob& job, bool wait_for_pipelines, std::string& error);

        // Owner thread. Claims a slot; false when all are busy.
        bool acquire(JpegComputeJob& job);

        // Owner thread. Records the decode into image (via its UNORM storage view);
        // leaves the image in SHADER_READ_ONLY_OPTIMAL.
        bool record(VkCommandBuffer cmd, JpegComputeJob& job, VkImage image, VkImageView storageView, std::string& error);

        // Owner thread, after the job's fence signalled.
        double gpuMilliseconds(const JpegComputeJob& job) const;
        void release(JpegComputeJob& job);

        size_t deviceBytes() const;

    private:
        struct Slot
        {
            BufferAllocation buffer[kJpegComputeBindings];
            VkDeviceSize capacity[kJpegComputeBindings] {};
            VkDescriptorSet set = VK_NULL_HANDLE;
            VkQueryPool queries = VK_NULL_HANDLE;
            bool busy = false;
        };

        enum class Resolve
        {
            Try,    // ready or nothing; queue missing variants
            Build,  // compile missing variants inline, skip ones in progress
            Wait,   // compile or wait until available
        };

        struct CompileRequest
        {
            std::string key;
            std::string source;
            std::promise<VkPipeline> promise;
        };

        VkPipeline pipeline(const std::string& key, const std::string& source, Resolve mode,
                            double& compile_ms, std::string& error);
        VkPipeline compilePipeline(const std::string& key, const std::string& source, std::string& error);
        bool resolvePipelines(int ntables, const JpegComputePush& push, JpegComputePipelines& pipelines,
                              Resolve mode, double& compile_ms, std::string& error);

        // Background compiler: requested variants first, then the common ones,
        // so the first JPEGs of a session find their pipelines ready.
        void warmup();
        std::thread m_warmup_thread;
        std::condition_variable m_compile_cv;
        std::deque<CompileRequest> m_compile_queue;
        bool m_stop_warmup = false;

        VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        Allocator& m_allocator;
        StagingArena& m_staging;
        bool m_supported = false;
        float m_tick_ns = 0.0f;

        VkDescriptorSetLayout m_set_layout = VK_NULL_HANDLE;
        VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptor_pool = VK_NULL_HANDLE;
        std::vector<Slot> m_slots;

        std::mutex m_pipeline_mutex;
        std::unordered_map<std::string, std::shared_future<VkPipeline>> m_pipelines;
    };

} // namespace mango::vulkan::detail

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <memory>

#include <mango/core/configure.hpp>
#include <mango/core/memory.hpp>
#include <mango/vulkan/allocator.hpp>

namespace mango::vulkan
{

    struct StagingArenaConfig
    {
        // Allocated by reserve(); the first chunk when nothing was reserved.
        VkDeviceSize initial_chunk = 32ull << 20;

        // Growth doubles the previous chunk up to this size. A single request
        // larger than this gets a chunk of its own size (rounded up).
        VkDeviceSize max_chunk = 128ull << 20;

        // Upper bound on reserved memory. When no chunk fits a request and
        // growing would exceed the limit, acquire() waits for release() instead.
        // A request that cannot fit even in an empty arena replaces the empty
        // chunks with one big enough, so the arena may exceed the limit by
        // the largest single request.
        VkDeviceSize limit = 256ull << 20;

        // Offset alignment of every range (power of two).
        VkDeviceSize alignment = 64;

        // TRANSFER_SRC: fall back to vkCmdCopyBuffer when the chunk is not
        // device-local. STORAGE_BUFFER: ReBAR / host-visible VRAM can be bound
        // directly as a compute storage buffer (no copy).
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                                 | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        // Opt into DEVICE_LOCAL|HOST_VISIBLE staging (ReBAR). Off by default:
        // host unstuff into ReBAR without a cached bounce buffer is much slower
        // than uncached system RAM + a GPU copy on current corpora.
        bool prefer_device_local = false;
    };

    struct StagingRange
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceSize offset = 0;        // of the range inside buffer
        VkDeviceSize size = 0;
        u8* mapped = nullptr;           // host address of offset
        // True when the underlying chunk is DEVICE_LOCAL|HOST_VISIBLE (ReBAR):
        // safe to bind as a storage buffer and skip the host→device copy.
        bool device_local = false;

        // arena bookkeeping
        void* chunk = nullptr;
        u64 handle = 0;

        explicit operator bool () const
        {
            return buffer != VK_NULL_HANDLE;
        }
    };

    struct StagingArenaStats
    {
        VkDeviceSize reserved_bytes = 0;    // sum of chunk sizes
        VkDeviceSize used_bytes = 0;        // live ranges
        VkDeviceSize peak_used_bytes = 0;
        size_t chunks = 0;
        size_t chunk_allocations = 0;       // chunks created over the lifetime
        size_t waits = 0;                   // acquire() calls that waited for release()
        size_t device_local_chunks = 0;     // ReBAR / DEVICE_LOCAL|HOST_VISIBLE chunks
    };

    // Host-visible, persistently mapped upload memory carved out of a few large
    // chunks. Each chunk is one dedicated VkDeviceMemory allocated once and kept
    // until trim() or destruction, so steady-state acquire/release never reach
    // the driver. Ranges of any size are sub-allocated (TLSF) and may be
    // released in any order. Thread-safe.
    class StagingArena : private NonCopyable
    {
    public:
        StagingArena(Allocator& allocator, const StagingArenaConfig& config = StagingArenaConfig());
        ~StagingArena();

        // Grow until at least bytes are reserved (bounded by the limit).
        // Meant for warm-up off the critical path.
        bool reserve(VkDeviceSize bytes);

        // Blocks while the arena is at its limit and other ranges are live.
        // Returns an empty range only when the chunk allocation itself fails.
        StagingRange acquire(VkDeviceSize size);

        // Host writes into range.mapped become visible to the device.
        void flush(const StagingRange& range, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

        // Any thread, after the device has finished reading the range.
        void release(StagingRange& range);

        // Frees the chunks that hold no live range.
        void trim();

        StagingArenaStats stats() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m;
    };

} // namespace mango::vulkan

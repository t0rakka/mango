/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <vector>

#include <vk_mem_alloc.h>

#include <mango/vulkan/staging_arena.hpp>
#include <mango/core/print.hpp>

namespace mango::vulkan
{

namespace
{
    // Oversized requests round up to this so similar large images share a chunk.
    constexpr VkDeviceSize kChunkGranularity = 16ull << 20;

    VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    static_assert(sizeof(VmaVirtualAllocation) <= sizeof(u64));

    u64 toHandle(VmaVirtualAllocation allocation)
    {
        u64 handle = 0;
        std::memcpy(&handle, &allocation, sizeof(allocation));
        return handle;
    }

    VmaVirtualAllocation fromHandle(u64 handle)
    {
        VmaVirtualAllocation allocation = VK_NULL_HANDLE;
        std::memcpy(&allocation, &handle, sizeof(allocation));
        return allocation;
    }

} // namespace

struct StagingArena::Impl
{
    struct Chunk
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaVirtualBlock block = VK_NULL_HANDLE;
        u8* mapped = nullptr;
        VkDeviceSize size = 0;
        size_t live = 0;
        bool device_local = false;
    };

    using ChunkPtr = std::unique_ptr<Chunk>;

    VmaAllocator m_vma;
    StagingArenaConfig m_config;

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<ChunkPtr> m_chunks;
    bool m_growing = false;     // a chunk is being created outside the lock
    VkDeviceSize m_last_chunk = 0;
    StagingArenaStats m_stats;

    Impl(Allocator& allocator, const StagingArenaConfig& config)
        : m_vma(allocator.handle())
        , m_config(config)
    {
        if (!m_config.alignment || (m_config.alignment & (m_config.alignment - 1)))
            m_config.alignment = 64;
        m_config.initial_chunk = std::max<VkDeviceSize>(alignUp(m_config.initial_chunk, m_config.alignment), m_config.alignment);
        m_config.max_chunk = std::max(m_config.max_chunk, m_config.initial_chunk);
        m_config.limit = std::max(m_config.limit, m_config.initial_chunk);
    }

    ~Impl()
    {
        for (ChunkPtr& chunk : m_chunks)
        {
            if (chunk->live)
            {
                printLine(Print::Error, "StagingArena: {} range(s) still live at destruction", chunk->live);
                vmaClearVirtualBlock(chunk->block);
            }
            destroyChunk(chunk);
        }
    }

    ChunkPtr createChunk(VkDeviceSize size)
    {
        VkBufferCreateInfo buffer_info =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = m_config.usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        // AUTO + sequential-write usually lands in uncached system RAM (good for
        // host unstuff bandwidth). ReBAR (DEVICE_LOCAL|HOST_VISIBLE) is opted
        // into only when the config asks for it — raw unstuff into ReBAR over
        // PCIe without a cached bounce buffer collapses throughput.
        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = m_config.prefer_device_local
            ? VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
            : VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
                         | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                         | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        auto chunk = std::make_unique<Chunk>();
        VmaAllocationInfo info = {};
        VkResult result = vmaCreateBuffer(m_vma, &buffer_info, &alloc_info, &chunk->buffer, &chunk->allocation, &info);
        if (result != VK_SUCCESS || !info.pMappedData)
        {
            printLine(Print::Error, "StagingArena: chunk allocation failed ({} bytes)", size);
            if (result == VK_SUCCESS)
                vmaDestroyBuffer(m_vma, chunk->buffer, chunk->allocation);
            return nullptr;
        }

        VmaVirtualBlockCreateInfo block_info = {};
        block_info.size = size;
        if (vmaCreateVirtualBlock(&block_info, &chunk->block) != VK_SUCCESS)
        {
            vmaDestroyBuffer(m_vma, chunk->buffer, chunk->allocation);
            return nullptr;
        }

        chunk->mapped = static_cast<u8*>(info.pMappedData);
        chunk->size = size;
        VkMemoryPropertyFlags mem_flags = 0;
        vmaGetAllocationMemoryProperties(m_vma, chunk->allocation, &mem_flags);
        chunk->device_local = (mem_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0;
        return chunk;
    }

    void destroyChunk(ChunkPtr& chunk)
    {
        vmaDestroyVirtualBlock(chunk->block);
        vmaDestroyBuffer(m_vma, chunk->buffer, chunk->allocation);
        chunk.reset();
    }

    bool tryAllocateLocked(VkDeviceSize size, StagingRange& range)
    {
        VmaVirtualAllocationCreateInfo info = {};
        info.size = size;
        info.alignment = m_config.alignment;

        for (ChunkPtr& chunk : m_chunks)
        {
            VmaVirtualAllocation allocation = VK_NULL_HANDLE;
            VkDeviceSize offset = 0;
            if (vmaVirtualAllocate(chunk->block, &info, &allocation, &offset) != VK_SUCCESS)
                continue;

            ++chunk->live;
            m_stats.used_bytes += size;
            m_stats.peak_used_bytes = std::max(m_stats.peak_used_bytes, m_stats.used_bytes);

            range.buffer = chunk->buffer;
            range.offset = offset;
            range.size = size;
            range.mapped = chunk->mapped + offset;
            range.device_local = chunk->device_local;
            range.chunk = chunk.get();
            range.handle = toHandle(allocation);
            return true;
        }
        return false;
    }

    // Next chunk for a request of size bytes: doubles up to max_chunk, at
    // least the (rounded) request, trimmed to what the limit still allows.
    VkDeviceSize nextChunkSize(VkDeviceSize size) const
    {
        const VkDeviceSize need = size <= m_config.initial_chunk ? alignUp(size, m_config.alignment)
                                                                 : alignUp(size, kChunkGranularity);
        VkDeviceSize desired = m_last_chunk ? std::min(m_last_chunk * 2, m_config.max_chunk) : m_config.initial_chunk;
        if (m_stats.reserved_bytes < m_config.limit)
            desired = std::min(desired, m_config.limit - m_stats.reserved_bytes);
        return std::max(desired, need);
    }

    // Creates one chunk with the lock released; empty chunks in drop are freed first.
    bool growLocked(std::unique_lock<std::mutex>& lock, VkDeviceSize size, std::vector<ChunkPtr> drop)
    {
        m_growing = true;
        lock.unlock();

        for (ChunkPtr& chunk : drop)
            destroyChunk(chunk);
        ChunkPtr chunk = createChunk(size);

        lock.lock();
        m_growing = false;
        const bool created = chunk != nullptr;
        if (created)
        {
            m_stats.reserved_bytes += chunk->size;
            ++m_stats.chunk_allocations;
            m_last_chunk = chunk->size;
            m_chunks.push_back(std::move(chunk));
        }
        m_stats.chunks = m_chunks.size();
        m_cv.notify_all();
        return created;
    }

    // Removes the chunks without live ranges; the caller destroys them.
    std::vector<ChunkPtr> takeEmptyLocked()
    {
        std::vector<ChunkPtr> empty;
        for (auto it = m_chunks.begin(); it != m_chunks.end(); )
        {
            if ((*it)->live == 0)
            {
                m_stats.reserved_bytes -= (*it)->size;
                empty.push_back(std::move(*it));
                it = m_chunks.erase(it);
            }
            else
            {
                ++it;
            }
        }
        m_stats.chunks = m_chunks.size();
        return empty;
    }

    bool reserve(VkDeviceSize bytes)
    {
        std::unique_lock lock(m_mutex);
        bytes = std::min(bytes, m_config.limit);
        while (m_stats.reserved_bytes < bytes)
        {
            if (m_growing)
            {
                m_cv.wait(lock);
                continue;
            }

            const VkDeviceSize size = nextChunkSize(bytes - m_stats.reserved_bytes);
            if (!growLocked(lock, size, {}))
                return false;
        }
        return true;
    }

    StagingRange acquire(VkDeviceSize request)
    {
        StagingRange range;
        if (!request)
            return range;

        const VkDeviceSize size = alignUp(request, m_config.alignment);
        bool waited = false;

        std::unique_lock lock(m_mutex);
        for (;;)
        {
            if (tryAllocateLocked(size, range))
                return range;

            if (m_growing)
            {
                m_cv.wait(lock);
                continue;
            }

            const VkDeviceSize grow = nextChunkSize(size);
            if (m_stats.reserved_bytes + grow <= m_config.limit)
            {
                if (!growLocked(lock, grow, {}))
                    return range;
                continue;
            }

            if (m_stats.used_bytes == 0)
            {
                // Nothing to wait for: swap the (empty) chunks for one that fits.
                const VkDeviceSize big = std::max(alignUp(size, kChunkGranularity),
                                                  std::min(m_config.limit, m_config.max_chunk));
                if (!growLocked(lock, big, takeEmptyLocked()))
                    return range;
                continue;
            }

            if (!waited)
            {
                waited = true;
                ++m_stats.waits;
            }
            m_cv.wait(lock);
        }
    }

    void flush(const StagingRange& range, VkDeviceSize offset, VkDeviceSize size)
    {
        if (!range.chunk)
            return;
        const Chunk* chunk = static_cast<const Chunk*>(range.chunk);
        if (size == VK_WHOLE_SIZE)
            size = range.size - offset;
        vmaFlushAllocation(m_vma, chunk->allocation, range.offset + offset, size);
    }

    void release(StagingRange& range)
    {
        if (!range.chunk)
            return;

        {
            std::lock_guard lock(m_mutex);
            Chunk* chunk = static_cast<Chunk*>(range.chunk);
            vmaVirtualFree(chunk->block, fromHandle(range.handle));
            --chunk->live;
            m_stats.used_bytes -= range.size;
        }
        m_cv.notify_all();
        range = {};
    }

    void trim()
    {
        std::vector<ChunkPtr> empty;
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_growing; });
            empty = takeEmptyLocked();
            if (m_chunks.empty())
                m_last_chunk = 0;
        }
        for (ChunkPtr& chunk : empty)
            destroyChunk(chunk);
    }

    StagingArenaStats stats() const
    {
        std::lock_guard lock(m_mutex);
        StagingArenaStats out = m_stats;
        out.device_local_chunks = 0;
        for (const ChunkPtr& chunk : m_chunks)
            if (chunk->device_local)
                ++out.device_local_chunks;
        return out;
    }
};

StagingArena::StagingArena(Allocator& allocator, const StagingArenaConfig& config)
    : m(std::make_unique<Impl>(allocator, config))
{
}

StagingArena::~StagingArena()
{
}

bool StagingArena::reserve(VkDeviceSize bytes)
{
    return m->reserve(bytes);
}

StagingRange StagingArena::acquire(VkDeviceSize size)
{
    return m->acquire(size);
}

void StagingArena::flush(const StagingRange& range, VkDeviceSize offset, VkDeviceSize size)
{
    m->flush(range, offset, size);
}

void StagingArena::release(StagingRange& range)
{
    m->release(range);
}

void StagingArena::trim()
{
    m->trim();
}

StagingArenaStats StagingArena::stats() const
{
    return m->stats();
}

} // namespace mango::vulkan

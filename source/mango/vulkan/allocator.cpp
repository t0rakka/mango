/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

// VMA pulls in platform headers (windows.h on Win32); keep them lean.
#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

// This is the single translation unit that compiles the VMA implementation. We
// bind to the Vulkan entry points mango already links against (the default static
// binding), so no volk / dynamic function fetch is needed. The implementation is a
// vendored third-party header; silence its (benign) warnings so a -Wall build of
// mango stays clean.
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wnullability-completeness"
    #pragma clang diagnostic ignored "-Wunused-variable"
    #pragma clang diagnostic ignored "-Wunused-parameter"
    #pragma clang diagnostic ignored "-Wmissing-field-initializers"
    #pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#elif defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-variable"
    #pragma GCC diagnostic ignored "-Wunused-parameter"
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#if defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

#include <mango/vulkan/allocator.hpp>
#include <mango/vulkan/vulkan.hpp>
#include <mango/core/print.hpp>

namespace mango::vulkan
{

    namespace
    {
        // All paths use VMA_MEMORY_USAGE_AUTO and let the host-access flags drive
        // whether the allocation lands in host-visible memory. This is the modern,
        // recommended VMA usage and lets it exploit ReBAR (device-local host-visible)
        // when present.
        VmaAllocationCreateInfo makeAllocationCreateInfo(MemoryUsage usage, bool mapped)
        {
            VmaAllocationCreateInfo info = {};
            info.usage = VMA_MEMORY_USAGE_AUTO;

            switch (usage)
            {
                case MemoryUsage::Upload:
                    info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
                    break;
                case MemoryUsage::Readback:
                    info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
                    break;
                case MemoryUsage::GpuOnly:
                case MemoryUsage::Auto:
                    break;
            }

            if (mapped)
            {
                // MAPPED_BIT requires a host-access flag; default to sequential write
                // for a GpuOnly/Auto request that nonetheless asked to be mapped.
                if (!(info.flags & (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                    VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT)))
                {
                    info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
                }

                info.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
            }

            return info;
        }

    } // namespace

    Allocator::Allocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, u32 apiVersion)
        : m_device(device)
    {
        VmaAllocatorCreateInfo createInfo = {};
        createInfo.physicalDevice = physicalDevice;
        createInfo.device = device;
        createInfo.instance = instance;
        createInfo.vulkanApiVersion = apiVersion;

        VkResult result = vmaCreateAllocator(&createInfo, &m_allocator);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vmaCreateAllocator: {}", getString(result));
            m_allocator = VK_NULL_HANDLE;
        }
    }

    Allocator::~Allocator()
    {
        if (m_allocator != VK_NULL_HANDLE)
        {
            vmaDestroyAllocator(m_allocator);
            m_allocator = VK_NULL_HANDLE;
        }
    }

    ImageAllocation Allocator::createImage(const VkImageCreateInfo& imageInfo, MemoryUsage usage)
    {
        ImageAllocation out;
        if (m_allocator == VK_NULL_HANDLE)
        {
            return out;
        }

        VmaAllocationCreateInfo allocInfo = makeAllocationCreateInfo(usage, false);

        VkResult result = vmaCreateImage(m_allocator, &imageInfo, &allocInfo,
                                         &out.image, &out.allocation, nullptr);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vmaCreateImage: {}", getString(result));
            out = {};
        }

        return out;
    }

    BufferAllocation Allocator::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                             MemoryUsage memoryUsage, bool mapped)
    {
        BufferAllocation out;
        if (m_allocator == VK_NULL_HANDLE || size == 0)
        {
            return out;
        }

        VkBufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        VmaAllocationCreateInfo allocInfo = makeAllocationCreateInfo(memoryUsage, mapped);

        VmaAllocationInfo allocationInfo = {};
        VkResult result = vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo,
                                          &out.buffer, &out.allocation, &allocationInfo);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vmaCreateBuffer: {}", getString(result));
            out = {};
            return out;
        }

        if (mapped)
        {
            out.mapped = allocationInfo.pMappedData;
        }

        return out;
    }

    void Allocator::destroyImage(const ImageAllocation& image)
    {
        if (m_allocator != VK_NULL_HANDLE && image.image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_allocator, image.image, image.allocation);
        }
    }

    void Allocator::destroyBuffer(const BufferAllocation& buffer)
    {
        if (m_allocator != VK_NULL_HANDLE && buffer.buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
        }
    }

    void* Allocator::map(VmaAllocation allocation)
    {
        void* data = nullptr;
        if (m_allocator != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
        {
            vmaMapMemory(m_allocator, allocation, &data);
        }
        return data;
    }

    void Allocator::unmap(VmaAllocation allocation)
    {
        if (m_allocator != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
        {
            vmaUnmapMemory(m_allocator, allocation);
        }
    }

    void Allocator::flush(VmaAllocation allocation, VkDeviceSize offset, VkDeviceSize size)
    {
        if (m_allocator != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
        {
            vmaFlushAllocation(m_allocator, allocation, offset, size);
        }
    }

} // namespace mango::vulkan

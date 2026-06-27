/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vulkan/vulkan.h>
#include <mango/core/configure.hpp>
#include <mango/core/memory.hpp>

// Forward declarations of the VMA opaque handles, identical to the ones in
// <vk_mem_alloc.h>. Declaring them here lets this header expose the underlying
// allocator (the escape hatch for fine-grained use) without pulling the ~20k-line
// VMA implementation header into every translation unit that just wants the
// convenience API. The typedefs are identical to VMA's, so include order with
// <vk_mem_alloc.h> does not matter.
VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_HANDLE(VmaAllocation)

namespace mango::vulkan
{

    // How an allocation's memory is accessed. Maps to VMA usage + host-access flags
    // in the implementation; the common cases are named so callers never need VMA.
    enum class MemoryUsage
    {
        GpuOnly,    // device-local, not host visible (sampled images, GPU-side buffers)
        Upload,     // host visible, optimised for sequential CPU writes (staging / dynamic)
        Readback,   // host visible, optimised for CPU reads (GPU -> CPU)
        Auto,       // let VMA decide purely from the buffer/image usage flags
    };

    struct ImageAllocation
    {
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        explicit operator bool () const
        {
            return image != VK_NULL_HANDLE;
        }
    };

    struct BufferAllocation
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        // Non-null when created persistently mapped: write CPU data here directly.
        // For HOST_COHERENT memory no explicit flush is needed; otherwise call flush().
        void* mapped = nullptr;

        explicit operator bool () const
        {
            return buffer != VK_NULL_HANDLE;
        }
    };

    // Thin RAII wrapper over a VmaAllocator. Hides memory-type selection and the
    // vkAllocateMemory/bind boilerplate behind createImage()/createBuffer(), while
    // still exposing the raw VmaAllocator (handle()) for applications that need
    // pools, defragmentation, or other fine-grained control -- those include
    // <vk_mem_alloc.h> directly and drive VMA themselves.
    class Allocator : public NonCopyable
    {
    protected:
        VmaAllocator m_allocator = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;

    public:
        // apiVersion is the VkApplicationInfo::apiVersion the instance was created
        // with (e.g. VK_API_VERSION_1_3); VMA enables version-specific paths from it.
        Allocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, u32 apiVersion);
        ~Allocator();

        // Create and back an image/buffer in one call. On failure a null allocation
        // is returned (test with operator bool). The full VkImageCreateInfo is taken
        // so callers keep complete control of the image; only the memory is automated.
        ImageAllocation createImage(const VkImageCreateInfo& imageInfo, MemoryUsage usage);
        BufferAllocation createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                      MemoryUsage memoryUsage, bool mapped = false);

        void destroyImage(const ImageAllocation& image);
        void destroyBuffer(const BufferAllocation& buffer);

        // Map/unmap for allocations not created persistently mapped (VMA reference
        // counts nested maps). map() returns nullptr on failure.
        void* map(VmaAllocation allocation);
        void unmap(VmaAllocation allocation);

        // Flush a host-visible, non-coherent range to the device (cheap no-op on
        // HOST_COHERENT memory). offset/size are relative to the allocation.
        void flush(VmaAllocation allocation, VkDeviceSize offset, VkDeviceSize size);

        // Escape hatch: the underlying VMA allocator for fine-grained use.
        VmaAllocator handle() const
        {
            return m_allocator;
        }

        operator VmaAllocator () const
        {
            return m_allocator;
        }

        bool valid() const
        {
            return m_allocator != VK_NULL_HANDLE;
        }
    };

} // namespace mango::vulkan

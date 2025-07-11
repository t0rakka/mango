/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/configure.hpp>
#include <mango/core/print.hpp>

#if defined(MANGO_WINDOW_SYSTEM_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
    #include "../window/win32/win32_window.hpp"
#endif

#if defined(MANGO_WINDOW_SYSTEM_XLIB)
    #define VK_USE_PLATFORM_XLIB_KHR
    #include "../window/xlib/xlib_window.hpp"
#endif

#if defined(MANGO_WINDOW_SYSTEM_XCB)
    #define VK_USE_PLATFORM_XCB_KHR
    #include "../window/xcb/xcb_window.hpp"
#endif

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)
    // TODO
    #define VK_USE_PLATFORM_WAYLAND_KHR
    #include "../window/wayland/wayland_window.hpp"
#endif

#include <mango/vulkan/vulkan.hpp>

namespace mango::vulkan
{

    // ------------------------------------------------------------------------------
    // VkSurface functions
    // ------------------------------------------------------------------------------

#if defined(MANGO_WINDOW_SYSTEM_WIN32)

    std::vector<const char*> getSurfaceExtensions()
    {
        return { "VK_KHR_surface", "VK_KHR_win32_surface" };
    }

    static
    VkSurfaceKHR createSurface(VkInstance instance, const WindowHandle& handle)
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = ::GetModuleHandle(NULL),
            .hwnd = handle.hwnd
        };

        VkResult result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkCreateWin32SurfaceKHR : {}", getString(result));
        }

        return surface;
    }

    static
    bool getPresentationSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, const WindowHandle& handle)
    {
        MANGO_UNREFERENCED(handle);
        return vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
    }

#endif

#if defined(MANGO_WINDOW_SYSTEM_XLIB)

    std::vector<const char*> getSurfaceExtensions()
    {
        return { "VK_KHR_surface", "VK_KHR_xlib_surface" };
    }

    static
    VkSurfaceKHR createSurface(VkInstance instance, const WindowHandle& handle)
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkXlibSurfaceCreateInfoKHR surfaceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
            .dpy = handle.display,
            .window = handle.window
        };

        VkResult result = vkCreateXlibSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkCreateXlibSurfaceKHR : {}", getString(result));
        }

        return surface;
    }

    static
    bool getPresentationSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, const WindowHandle& handle)
    {
        return vkGetPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIndex, handle.display, handle.visualid);
    }

#endif

#if defined(MANGO_WINDOW_SYSTEM_XCB)

    std::vector<const char*> getSurfaceExtensions()
    {
        return { "VK_KHR_surface", "VK_KHR_xcb_surface" };
    }

    static
    VkSurfaceKHR createSurface(VkInstance instance, const WindowHandle& handle)
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkXcbSurfaceCreateInfoKHR surfaceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
            .connection = handle.connection,
            .window = handle.window
        };

        VkResult result = vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkCreateXcbSurfaceKHR : {}", getString(result));
        }

        return surface;
    }

    static
    bool getPresentationSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, const WindowHandle& handle)
    {
        return vkGetPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex, handle.connection, handle.visualid);
    }

#endif

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

    std::vector<const char*> getSurfaceExtensions()
    {
        return { "VK_KHR_surface", "VK_KHR_wayland_surface" };
    }

    static
    VkSurfaceKHR createSurface(VkInstance instance, const WindowHandle& handle)
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        // TODO
        /*
        VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .display = handle.display,
            .surface = handle.surface,
        };

        VkResult result = vkCreateWaylandSurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkCreateWaylandSurfaceKHR : {}", getString(result));
        }
        */

        return surface;
    }

    static
    bool getPresentationSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, const WindowHandle& handle)
    {

        return vkGetPhysicalDeviceWaylandPresentationSupportKHR(physicalDevice, queueFamilyIndex, handle.display);
    }

#endif

    // ------------------------------------------------------------------------------
    // Instance
    // ------------------------------------------------------------------------------

    Instance::Instance(const VkApplicationInfo& applicationInfo,
                       const std::vector<const char*> layers,
                       const std::vector<const char*> extensions)
    {
        VkInstanceCreateInfo instanceCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = uint32_t(layers.size()),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount = uint32_t(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };

        VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_handle);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkCreateInstance: {}", getString(result));
            return;
        }
    }

    Instance::~Instance()
    {
        if (m_handle != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_handle, nullptr);
        }
    }

    // ------------------------------------------------------------------------------
    // Device
    // ------------------------------------------------------------------------------

    Device::Device(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& deviceCreateInfo)
    {

        VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &m_handle);
        MANGO_UNREFERENCED(result);
    }

    Device::~Device()
    {
        if (m_handle != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_handle);
            vkDestroyDevice(m_handle, nullptr);
        }
    }

    // ------------------------------------------------------------------------------
    // CommandPool
    // ------------------------------------------------------------------------------

    CommandPool::CommandPool(VkDevice device, uint32_t graphicsQueueFamilyIndex)
        : m_device(device)
    {
        VkCommandPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = graphicsQueueFamilyIndex,
        };

        VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_handle);
        MANGO_UNREFERENCED(result);
    }

    CommandPool::~CommandPool()
    {
        if (m_handle != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
            vkDestroyCommandPool(m_device, m_handle, nullptr);
        }
    }

    // ------------------------------------------------------------------------------
    // VulkanWindow
    // ------------------------------------------------------------------------------

    VulkanWindow::VulkanWindow(VkInstance instance, int width, int height, u32 flags)
        : Window(width, height, flags | Window::API_VULKAN)
        , m_instance(instance)
        , m_surface(VK_NULL_HANDLE)
    {
        m_surface = createSurface(m_instance, *m_window_context);
    }

    VulkanWindow::~VulkanWindow()
    {
        if (m_surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        }
    }

    bool VulkanWindow::getPresentationSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) const
    {
        return vulkan::getPresentationSupport(physicalDevice, queueFamilyIndex, *m_window_context);
        return false;
    }

    void VulkanWindow::toggleFullscreen()
    {
        m_window_context->toggleFullscreen();
    }

    bool VulkanWindow::isFullscreen() const
    {
        return m_window_context->fullscreen;
    }

    // ------------------------------------------------------------------------------
    // enumerateInstanceLayerProperties()
    // ------------------------------------------------------------------------------

    std::vector<VkLayerProperties> enumerateInstanceLayerProperties()
    {
        VkResult result = VK_SUCCESS;

        uint32_t count = 0;

        result = vkEnumerateInstanceLayerProperties(&count, nullptr);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkEnumerateInstanceLayerProperties: {}", getString(result));
            return {};
        }

        std::vector<VkLayerProperties> layerProperties(count);

        result = vkEnumerateInstanceLayerProperties(&count, layerProperties.data());
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkEnumerateInstanceLayerProperties: {}", getString(result));
            return {};
        }

        printLine(Print::Info, "InstanceLayerProperties:");

        for (const auto& property : layerProperties)
        {
            printLine(Print::Info, "  - {}", property.layerName);
        }

        printLine(Print::Info, "");

        return layerProperties;
    }

    // ------------------------------------------------------------------------------
    // enumerateInstanceExtensionProperties()
    // ------------------------------------------------------------------------------

    std::vector<VkExtensionProperties> enumerateInstanceExtensionProperties(const char* layerName)
    {
        VkResult result = VK_SUCCESS;

        uint32_t count = 0;

        result = vkEnumerateInstanceExtensionProperties(layerName, &count, nullptr);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkEnumerateInstanceExtensionProperties: {}", getString(result));
            return {};
        }

        std::vector<VkExtensionProperties> extensionProperties(count);

        result = vkEnumerateInstanceExtensionProperties(layerName, &count, extensionProperties.data());
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkEnumerateInstanceExtensionProperties: {}", getString(result));
            return {};
        }

        printLine(Print::Info, "InstanceExtensionProperties:");

        for (auto property : extensionProperties)
        {
            printLine(Print::Info, "  - {}", property.extensionName);
        }

        printLine(Print::Info, "");

        return extensionProperties;
    }

    // ------------------------------------------------------------------------------
    // enumeratePhysicalDevices()
    // ------------------------------------------------------------------------------

    std::vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance)
    {
        VkResult result = VK_SUCCESS;
        uint32_t count = 0;

        result = vkEnumeratePhysicalDevices(instance, &count, nullptr);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkEnumeratePhysicalDevices: {}", getString(result));
            return {};
        }

        std::vector<VkPhysicalDevice> physicalDevices(count);

        result = vkEnumeratePhysicalDevices(instance, &count, physicalDevices.data());
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkEnumeratePhysicalDevices: {}", getString(result));
            return {};
        }

        // print device properties

        printLine(Print::Info, "PhysicalDevices: {}", physicalDevices.size());

        VkPhysicalDevice selectedPhysicalDevice = 0;
        uint32_t selectedQueueFamilyIndex = 0;

        for (auto physicalDevice : physicalDevices)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);

            printLine(Print::Info, "");
            printLine(Print::Info, "  deviceName: \"{}\"", properties.deviceName);
            printLine(Print::Info, "  deviceType: {}", getString(properties.deviceType));
            printLine(Print::Info, "  apiVersion: {}.{}.{}",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));
            printLine(Print::Info, "  driverVersion: {}.{}.{}",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));
            printLine(Print::Info, "  vendorID: {:08x}", properties.vendorID);
            printLine(Print::Info, "  deviceID: {:08x}", properties.deviceID);
            //properties.limits;
            //properties.sparseProperties;

            //VkPhysicalDeviceFeatures features;
            //vkGetPhysicalDeviceFeatures(physicalDevice, &features);

            VkPhysicalDeviceMemoryProperties memoryProperties;
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

            printLine(Print::Info, "");
            printLine(Print::Info, "  memoryHeapCount: {}", memoryProperties.memoryHeapCount);

            for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i)
            {
                const VkMemoryHeap& memoryHeap = memoryProperties.memoryHeaps[i];

                printLine(Print::Info, "");
                printLine(Print::Info, "    size: {} MB", memoryHeap.size >> 20);
                printLine(Print::Info, "    flags: {:08x}", memoryHeap.flags);

                if (memoryHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                {
                    printLine(Print::Info, "      + VK_MEMORY_HEAP_DEVICE_LOCAL_BIT");
                }

                if (memoryHeap.flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
                {
                    printLine(Print::Info, "      + VK_MEMORY_HEAP_MULTI_INSTANCE_BIT");
                }
            }

            printLine(Print::Info, "");
            printLine(Print::Info, "  memoryTypeCount: {}", memoryProperties.memoryTypeCount);

            for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
            {
                const VkMemoryType& memoryType = memoryProperties.memoryTypes[i];

                printLine(Print::Info, "");
                printLine(Print::Info, "    heapIndex: {}", memoryType.heapIndex);
                printLine(Print::Info, "    propertyFlags: {:08x}", memoryType.propertyFlags);

                if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                {
                    printLine(Print::Info, "      + VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT");
                }

                if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                {
                    printLine(Print::Info, "      + VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT");
                }

                if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                {
                    printLine(Print::Info, "      + VK_MEMORY_PROPERTY_HOST_COHERENT_BIT");
                }

                if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
                {
                    printLine(Print::Info, "      + VK_MEMORY_PROPERTY_HOST_CACHED_BIT");
                }

                if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
                {
                    printLine(Print::Info, "      + VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT");
                }

                if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
                {
                    printLine(Print::Info, "      + VK_MEMORY_PROPERTY_PROTECTED_BIT");
                }

                if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
                {
                    printLine(Print::Info, "      + VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD");
                }

                if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
                {
                    printLine(Print::Info, "      + VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD");
                }

                if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)
                {
                    printLine(Print::Info, "      + VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV");
                }
            }

            // GetPhysicalDeviceQueueFamilyProperties

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

            printLine(Print::Info, "");
            printLine(Print::Info, "  queueFamilyProperties: {}", queueFamilyProperties.size());

            for (uint32_t i = 0; i < queueFamilyCount; ++i)
            {
                const VkQueueFamilyProperties& properties = queueFamilyProperties[i];

                printLine(Print::Info, "");
                printLine(Print::Info, "    queueFlags: {:08x}", properties.queueFlags);

                if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    printLine(Print::Info, "      + VK_QUEUE_GRAPHICS_BIT");
                }

                if (properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    printLine(Print::Info, "      + VK_QUEUE_COMPUTE_BIT");
                }

                if (properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    printLine(Print::Info, "      + VK_QUEUE_TRANSFER_BIT");
                }

                if (properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                {
                    printLine(Print::Info, "      + VK_QUEUE_SPARSE_BINDING_BIT");
                }

                if (properties.queueFlags & VK_QUEUE_PROTECTED_BIT)
                {
                    printLine(Print::Info, "      + VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV");
                }

                if (properties.queueFlags & VK_QUEUE_PROTECTED_BIT)
                {
                    printLine(Print::Info, "      + VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV");
                }

#ifdef VK_ENABLE_BETA_EXTENSIONS
                if (properties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
                {
                    printLine(Print::Info, "      + VK_QUEUE_VIDEO_DECODE_BIT_KHR");
                }

                if (properties.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
                {
                    printLine(Print::Info, "      + VK_QUEUE_VIDEO_ENCODE_BIT_KHR");
                }
#endif

                const VkExtent3D& granularity = properties.minImageTransferGranularity;

                printLine(Print::Info, "    queueCount: {}", properties.queueCount);
                printLine(Print::Info, "    timestampValidBits: {}", properties.timestampValidBits);
                printLine(Print::Info, "    minImageTransferGranularity: {} x {} x {}", granularity.width, granularity.height, granularity.depth);
            }
        }

        printLine(Print::Info, "");

        return physicalDevices;
    }

    // ------------------------------------------------------------------------------
    // selectPhysicalDevice()
    // ------------------------------------------------------------------------------

    struct PhysicalDeviceScore
    {
        VkPhysicalDevice device = VK_NULL_HANDLE;
        size_t deviceIndex = 0;

        u32 typeScore = 0;
        u32 apiScore = 0;
        u32 memoryScore = 0;

        bool operator > (const PhysicalDeviceScore& other) const
        {
            if (typeScore != other.typeScore) return typeScore > other.typeScore;
            if (apiScore != other.apiScore) return apiScore > other.apiScore;
            return memoryScore > other.memoryScore;
        }
    };

    VkPhysicalDevice selectPhysicalDevice(VkInstance instance)
    {
        std::vector<VkPhysicalDevice> physicalDevices = enumeratePhysicalDevices(instance);
        std::vector<PhysicalDeviceScore> scores;

        if (physicalDevices.empty())
        {
            printLine(Print::Warning, "[selectPhysicalDevice] No PhysicalDevices.");
            return VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < physicalDevices.size(); ++i)
        {
            VkPhysicalDevice physicalDevice = physicalDevices[i];

            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);

            VkPhysicalDeviceMemoryProperties memoryProperties;
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

            PhysicalDeviceScore score;

            score.device = physicalDevice;
            score.deviceIndex = i;

            switch (properties.deviceType)
            {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    score.typeScore = 4;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    score.typeScore = 3;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    score.typeScore = 2;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    score.typeScore = 1;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                default:
                    break;
            }

            score.apiScore = VK_VERSION_MAJOR(properties.apiVersion) * 100 +
                             VK_VERSION_MINOR(properties.apiVersion) * 10;

            for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i)
            {
                const VkMemoryHeap& memoryHeap = memoryProperties.memoryHeaps[i];
                if (memoryHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                {
                    score.memoryScore += u32(memoryHeap.size >> 20);
                }
            }

            scores.push_back(score);
        }

        std::sort(scores.begin(), scores.end(), std::greater<>());
        PhysicalDeviceScore bestScore = scores.front();

        printLine(Print::Info, "selectedPhysicalDevice: {}", bestScore.deviceIndex);

        return bestScore.device;
    }

    // ------------------------------------------------------------------------------
    // getPhysicalDeviceQueueFamilyProperties()
    // ------------------------------------------------------------------------------

    std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice)
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queueFamilyProperties.data());

        return queueFamilyProperties;
    }

    // ------------------------------------------------------------------------------
    // getString()
    // ------------------------------------------------------------------------------

    #define XCASE(x) \
        case x: name = #x; break

    const char* getString(VkResult result)
    {
        const char* name = "UNDEFINED";
        switch (result)
        {
            XCASE(VK_SUCCESS);
            XCASE(VK_NOT_READY);
            XCASE(VK_TIMEOUT);
            XCASE(VK_EVENT_SET);
            XCASE(VK_EVENT_RESET);
            XCASE(VK_INCOMPLETE);
            XCASE(VK_ERROR_OUT_OF_HOST_MEMORY);
            XCASE(VK_ERROR_OUT_OF_DEVICE_MEMORY);
            XCASE(VK_ERROR_INITIALIZATION_FAILED);
            XCASE(VK_ERROR_DEVICE_LOST);
            XCASE(VK_ERROR_MEMORY_MAP_FAILED);
            XCASE(VK_ERROR_LAYER_NOT_PRESENT);
            XCASE(VK_ERROR_EXTENSION_NOT_PRESENT);
            XCASE(VK_ERROR_FEATURE_NOT_PRESENT);
            XCASE(VK_ERROR_INCOMPATIBLE_DRIVER);
            XCASE(VK_ERROR_TOO_MANY_OBJECTS);
            XCASE(VK_ERROR_FORMAT_NOT_SUPPORTED);
            XCASE(VK_ERROR_FRAGMENTED_POOL);
            XCASE(VK_ERROR_UNKNOWN);
            XCASE(VK_ERROR_OUT_OF_POOL_MEMORY);
            XCASE(VK_ERROR_INVALID_EXTERNAL_HANDLE);
            XCASE(VK_ERROR_FRAGMENTATION);
            XCASE(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
            XCASE(VK_PIPELINE_COMPILE_REQUIRED);
            XCASE(VK_ERROR_SURFACE_LOST_KHR);
            XCASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
            XCASE(VK_SUBOPTIMAL_KHR);
            XCASE(VK_ERROR_OUT_OF_DATE_KHR);
            XCASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
            XCASE(VK_ERROR_VALIDATION_FAILED_EXT);
            XCASE(VK_ERROR_INVALID_SHADER_NV);
            XCASE(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
            XCASE(VK_ERROR_NOT_PERMITTED_KHR);
            XCASE(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
            XCASE(VK_THREAD_IDLE_KHR);
            XCASE(VK_THREAD_DONE_KHR);
            XCASE(VK_OPERATION_DEFERRED_KHR);
            XCASE(VK_OPERATION_NOT_DEFERRED_KHR);
        }

        return name;
    }

    const char* getString(VkPhysicalDeviceType deviceType)
    {
        const char* name = "UNDEFINED";
        switch (deviceType)
        {
            XCASE(VK_PHYSICAL_DEVICE_TYPE_OTHER);
            XCASE(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
            XCASE(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
            XCASE(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU);
            XCASE(VK_PHYSICAL_DEVICE_TYPE_CPU);
        }

        return name;
    }

    const char* getString(VkFormat format)
    {
        const char* name = "UNDEFINED";
        switch (format)
        {
            // Provided by VK_VERSION_1_0
            XCASE(VK_FORMAT_UNDEFINED);
            XCASE(VK_FORMAT_R4G4_UNORM_PACK8);
            XCASE(VK_FORMAT_R4G4B4A4_UNORM_PACK16);
            XCASE(VK_FORMAT_B4G4R4A4_UNORM_PACK16);
            XCASE(VK_FORMAT_R5G6B5_UNORM_PACK16);
            XCASE(VK_FORMAT_B5G6R5_UNORM_PACK16);
            XCASE(VK_FORMAT_R5G5B5A1_UNORM_PACK16);
            XCASE(VK_FORMAT_B5G5R5A1_UNORM_PACK16);
            XCASE(VK_FORMAT_A1R5G5B5_UNORM_PACK16);
            XCASE(VK_FORMAT_R8_UNORM);
            XCASE(VK_FORMAT_R8_SNORM);
            XCASE(VK_FORMAT_R8_USCALED);
            XCASE(VK_FORMAT_R8_SSCALED);
            XCASE(VK_FORMAT_R8_UINT);
            XCASE(VK_FORMAT_R8_SINT);
            XCASE(VK_FORMAT_R8_SRGB);
            XCASE(VK_FORMAT_R8G8_UNORM);
            XCASE(VK_FORMAT_R8G8_SNORM);
            XCASE(VK_FORMAT_R8G8_USCALED);
            XCASE(VK_FORMAT_R8G8_SSCALED);
            XCASE(VK_FORMAT_R8G8_UINT);
            XCASE(VK_FORMAT_R8G8_SINT);
            XCASE(VK_FORMAT_R8G8_SRGB);
            XCASE(VK_FORMAT_R8G8B8_UNORM);
            XCASE(VK_FORMAT_R8G8B8_SNORM);
            XCASE(VK_FORMAT_R8G8B8_USCALED);
            XCASE(VK_FORMAT_R8G8B8_SSCALED);
            XCASE(VK_FORMAT_R8G8B8_UINT);
            XCASE(VK_FORMAT_R8G8B8_SINT);
            XCASE(VK_FORMAT_R8G8B8_SRGB);
            XCASE(VK_FORMAT_B8G8R8_UNORM);
            XCASE(VK_FORMAT_B8G8R8_SNORM);
            XCASE(VK_FORMAT_B8G8R8_USCALED);
            XCASE(VK_FORMAT_B8G8R8_SSCALED);
            XCASE(VK_FORMAT_B8G8R8_UINT);
            XCASE(VK_FORMAT_B8G8R8_SINT);
            XCASE(VK_FORMAT_B8G8R8_SRGB);
            XCASE(VK_FORMAT_R8G8B8A8_UNORM);
            XCASE(VK_FORMAT_R8G8B8A8_SNORM);
            XCASE(VK_FORMAT_R8G8B8A8_USCALED);
            XCASE(VK_FORMAT_R8G8B8A8_SSCALED);
            XCASE(VK_FORMAT_R8G8B8A8_UINT);
            XCASE(VK_FORMAT_R8G8B8A8_SINT);
            XCASE(VK_FORMAT_R8G8B8A8_SRGB);
            XCASE(VK_FORMAT_B8G8R8A8_UNORM);
            XCASE(VK_FORMAT_B8G8R8A8_SNORM);
            XCASE(VK_FORMAT_B8G8R8A8_USCALED);
            XCASE(VK_FORMAT_B8G8R8A8_SSCALED);
            XCASE(VK_FORMAT_B8G8R8A8_UINT);
            XCASE(VK_FORMAT_B8G8R8A8_SINT);
            XCASE(VK_FORMAT_B8G8R8A8_SRGB);
            XCASE(VK_FORMAT_A8B8G8R8_UNORM_PACK32);
            XCASE(VK_FORMAT_A8B8G8R8_SNORM_PACK32);
            XCASE(VK_FORMAT_A8B8G8R8_USCALED_PACK32);
            XCASE(VK_FORMAT_A8B8G8R8_SSCALED_PACK32);
            XCASE(VK_FORMAT_A8B8G8R8_UINT_PACK32);
            XCASE(VK_FORMAT_A8B8G8R8_SINT_PACK32);
            XCASE(VK_FORMAT_A8B8G8R8_SRGB_PACK32);
            XCASE(VK_FORMAT_A2R10G10B10_UNORM_PACK32);
            XCASE(VK_FORMAT_A2R10G10B10_SNORM_PACK32);
            XCASE(VK_FORMAT_A2R10G10B10_USCALED_PACK32);
            XCASE(VK_FORMAT_A2R10G10B10_SSCALED_PACK32);
            XCASE(VK_FORMAT_A2R10G10B10_UINT_PACK32);
            XCASE(VK_FORMAT_A2R10G10B10_SINT_PACK32);
            XCASE(VK_FORMAT_A2B10G10R10_UNORM_PACK32);
            XCASE(VK_FORMAT_A2B10G10R10_SNORM_PACK32);
            XCASE(VK_FORMAT_A2B10G10R10_USCALED_PACK32);
            XCASE(VK_FORMAT_A2B10G10R10_SSCALED_PACK32);
            XCASE(VK_FORMAT_A2B10G10R10_UINT_PACK32);
            XCASE(VK_FORMAT_A2B10G10R10_SINT_PACK32);
            XCASE(VK_FORMAT_R16_UNORM);
            XCASE(VK_FORMAT_R16_SNORM);
            XCASE(VK_FORMAT_R16_USCALED);
            XCASE(VK_FORMAT_R16_SSCALED);
            XCASE(VK_FORMAT_R16_UINT);
            XCASE(VK_FORMAT_R16_SINT);
            XCASE(VK_FORMAT_R16_SFLOAT);
            XCASE(VK_FORMAT_R16G16_UNORM);
            XCASE(VK_FORMAT_R16G16_SNORM);
            XCASE(VK_FORMAT_R16G16_USCALED);
            XCASE(VK_FORMAT_R16G16_SSCALED);
            XCASE(VK_FORMAT_R16G16_UINT);
            XCASE(VK_FORMAT_R16G16_SINT);
            XCASE(VK_FORMAT_R16G16_SFLOAT);
            XCASE(VK_FORMAT_R16G16B16_UNORM);
            XCASE(VK_FORMAT_R16G16B16_SNORM);
            XCASE(VK_FORMAT_R16G16B16_USCALED);
            XCASE(VK_FORMAT_R16G16B16_SSCALED);
            XCASE(VK_FORMAT_R16G16B16_UINT);
            XCASE(VK_FORMAT_R16G16B16_SINT);
            XCASE(VK_FORMAT_R16G16B16_SFLOAT);
            XCASE(VK_FORMAT_R16G16B16A16_UNORM);
            XCASE(VK_FORMAT_R16G16B16A16_SNORM);
            XCASE(VK_FORMAT_R16G16B16A16_USCALED);
            XCASE(VK_FORMAT_R16G16B16A16_SSCALED);
            XCASE(VK_FORMAT_R16G16B16A16_UINT);
            XCASE(VK_FORMAT_R16G16B16A16_SINT);
            XCASE(VK_FORMAT_R16G16B16A16_SFLOAT);
            XCASE(VK_FORMAT_R32_UINT);
            XCASE(VK_FORMAT_R32_SINT);
            XCASE(VK_FORMAT_R32_SFLOAT);
            XCASE(VK_FORMAT_R32G32_UINT);
            XCASE(VK_FORMAT_R32G32_SINT);
            XCASE(VK_FORMAT_R32G32_SFLOAT);
            XCASE(VK_FORMAT_R32G32B32_UINT);
            XCASE(VK_FORMAT_R32G32B32_SINT);
            XCASE(VK_FORMAT_R32G32B32_SFLOAT);
            XCASE(VK_FORMAT_R32G32B32A32_UINT);
            XCASE(VK_FORMAT_R32G32B32A32_SINT);
            XCASE(VK_FORMAT_R32G32B32A32_SFLOAT);
            XCASE(VK_FORMAT_R64_UINT);
            XCASE(VK_FORMAT_R64_SINT);
            XCASE(VK_FORMAT_R64_SFLOAT);
            XCASE(VK_FORMAT_R64G64_UINT);
            XCASE(VK_FORMAT_R64G64_SINT);
            XCASE(VK_FORMAT_R64G64_SFLOAT);
            XCASE(VK_FORMAT_R64G64B64_UINT);
            XCASE(VK_FORMAT_R64G64B64_SINT);
            XCASE(VK_FORMAT_R64G64B64_SFLOAT);
            XCASE(VK_FORMAT_R64G64B64A64_UINT);
            XCASE(VK_FORMAT_R64G64B64A64_SINT);
            XCASE(VK_FORMAT_R64G64B64A64_SFLOAT);
            XCASE(VK_FORMAT_B10G11R11_UFLOAT_PACK32);
            XCASE(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
            XCASE(VK_FORMAT_D16_UNORM);
            XCASE(VK_FORMAT_X8_D24_UNORM_PACK32);
            XCASE(VK_FORMAT_D32_SFLOAT);
            XCASE(VK_FORMAT_S8_UINT);
            XCASE(VK_FORMAT_D16_UNORM_S8_UINT);
            XCASE(VK_FORMAT_D24_UNORM_S8_UINT);
            XCASE(VK_FORMAT_D32_SFLOAT_S8_UINT);
            XCASE(VK_FORMAT_BC1_RGB_UNORM_BLOCK);
            XCASE(VK_FORMAT_BC1_RGB_SRGB_BLOCK);
            XCASE(VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
            XCASE(VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
            XCASE(VK_FORMAT_BC2_UNORM_BLOCK);
            XCASE(VK_FORMAT_BC2_SRGB_BLOCK);
            XCASE(VK_FORMAT_BC3_UNORM_BLOCK);
            XCASE(VK_FORMAT_BC3_SRGB_BLOCK);
            XCASE(VK_FORMAT_BC4_UNORM_BLOCK);
            XCASE(VK_FORMAT_BC4_SNORM_BLOCK);
            XCASE(VK_FORMAT_BC5_UNORM_BLOCK);
            XCASE(VK_FORMAT_BC5_SNORM_BLOCK);
            XCASE(VK_FORMAT_BC6H_UFLOAT_BLOCK);
            XCASE(VK_FORMAT_BC6H_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_BC7_UNORM_BLOCK);
            XCASE(VK_FORMAT_BC7_SRGB_BLOCK);
            XCASE(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK);
            XCASE(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK);
            XCASE(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK);
            XCASE(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK);
            XCASE(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
            XCASE(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK);
            XCASE(VK_FORMAT_EAC_R11_UNORM_BLOCK);
            XCASE(VK_FORMAT_EAC_R11_SNORM_BLOCK);
            XCASE(VK_FORMAT_EAC_R11G11_UNORM_BLOCK);
            XCASE(VK_FORMAT_EAC_R11G11_SNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_4x4_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_5x4_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_5x4_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_5x5_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_5x5_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_6x5_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_6x5_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_6x6_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_8x5_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_8x5_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_8x6_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_8x6_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_8x8_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_8x8_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_10x5_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_10x5_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_10x6_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_10x6_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_10x8_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_10x8_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_10x10_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_10x10_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_12x10_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_12x10_SRGB_BLOCK);
            XCASE(VK_FORMAT_ASTC_12x12_UNORM_BLOCK);
            XCASE(VK_FORMAT_ASTC_12x12_SRGB_BLOCK);

            // Provided by VK_VERSION_1_1
            XCASE(VK_FORMAT_G8B8G8R8_422_UNORM);
            XCASE(VK_FORMAT_B8G8R8G8_422_UNORM);
            XCASE(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM);
            XCASE(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
            XCASE(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM);
            XCASE(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM);
            XCASE(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM);
            XCASE(VK_FORMAT_R10X6_UNORM_PACK16);
            XCASE(VK_FORMAT_R10X6G10X6_UNORM_2PACK16);
            XCASE(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16);
            XCASE(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16);
            XCASE(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16);
            XCASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16);
            XCASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16);
            XCASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16);
            XCASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16);
            XCASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16);
            XCASE(VK_FORMAT_R12X4_UNORM_PACK16);
            XCASE(VK_FORMAT_R12X4G12X4_UNORM_2PACK16);
            XCASE(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16);
            XCASE(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16);
            XCASE(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16);
            XCASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16);
            XCASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16);
            XCASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16);
            XCASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16);
            XCASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16);
            XCASE(VK_FORMAT_G16B16G16R16_422_UNORM);
            XCASE(VK_FORMAT_B16G16R16G16_422_UNORM);
            XCASE(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM);
            XCASE(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM);
            XCASE(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM);
            XCASE(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM);
            XCASE(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM);

            // Provided by VK_VERSION_1_3
            XCASE(VK_FORMAT_G8_B8R8_2PLANE_444_UNORM);
            XCASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16);
            XCASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16);
            XCASE(VK_FORMAT_G16_B16R16_2PLANE_444_UNORM);
            XCASE(VK_FORMAT_A4R4G4B4_UNORM_PACK16);
            XCASE(VK_FORMAT_A4B4G4R4_UNORM_PACK16);
            XCASE(VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK);
            XCASE(VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK);

            /*
            // Provided by VK_VERSION_1_4
            XCASE(VK_FORMAT_A1B5G5R5_UNORM_PACK16);
            XCASE(VK_FORMAT_A8_UNORM);
            */

            // Provided by VK_IMG_format_pvrtc
            XCASE(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG);
            XCASE(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG);
            XCASE(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG);
            XCASE(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG);
            XCASE(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG);
            XCASE(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG);
            XCASE(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG);
            XCASE(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG);

            /*
            // Provided by VK_ARM_tensors
            XCASE(VK_FORMAT_R8_BOOL_ARM);

            // Provided by VK_NV_optical_flow
            XCASE(VK_FORMAT_R16G16_SFIXED5_NV);

            // Provided by VK_ARM_format_pack
            XCASE(VK_FORMAT_R10X6_UINT_PACK16_ARM);
            XCASE(VK_FORMAT_R10X6G10X6_UINT_2PACK16_ARM);
            XCASE(VK_FORMAT_R10X6G10X6B10X6A10X6_UINT_4PACK16_ARM);
            XCASE(VK_FORMAT_R12X4_UINT_PACK16_ARM);
            XCASE(VK_FORMAT_R12X4G12X4_UINT_2PACK16_ARM);
            XCASE(VK_FORMAT_R12X4G12X4B12X4A12X4_UINT_4PACK16_ARM);
            XCASE(VK_FORMAT_R14X2_UINT_PACK16_ARM);
            XCASE(VK_FORMAT_R14X2G14X2_UINT_2PACK16_ARM);
            XCASE(VK_FORMAT_R14X2G14X2B14X2A14X2_UINT_4PACK16_ARM);
            XCASE(VK_FORMAT_R14X2_UNORM_PACK16_ARM);
            XCASE(VK_FORMAT_R14X2G14X2_UNORM_2PACK16_ARM);
            XCASE(VK_FORMAT_R14X2G14X2B14X2A14X2_UNORM_4PACK16_ARM);
            XCASE(VK_FORMAT_G14X2_B14X2R14X2_2PLANE_420_UNORM_3PACK16_ARM);
            XCASE(VK_FORMAT_G14X2_B14X2R14X2_2PLANE_422_UNORM_3PACK16_ARM);

            // Provided by VK_EXT_texture_compression_astc_hdr
            VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK,
            VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT = VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK,

            // Provided by VK_KHR_sampler_ycbcr_conversion
            VK_FORMAT_G8B8G8R8_422_UNORM_KHR = VK_FORMAT_G8B8G8R8_422_UNORM,
            VK_FORMAT_B8G8R8G8_422_UNORM_KHR = VK_FORMAT_B8G8R8G8_422_UNORM,
            VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR = VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
            VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
            VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR = VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM,
            VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR = VK_FORMAT_G8_B8R8_2PLANE_422_UNORM,
            VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR = VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM,
            VK_FORMAT_R10X6_UNORM_PACK16_KHR = VK_FORMAT_R10X6_UNORM_PACK16,
            VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR = VK_FORMAT_R10X6G10X6_UNORM_2PACK16,
            VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR = VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
            VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR = VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
            VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR = VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
            VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
            VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
            VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
            VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
            VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
            VK_FORMAT_R12X4_UNORM_PACK16_KHR = VK_FORMAT_R12X4_UNORM_PACK16,
            VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR = VK_FORMAT_R12X4G12X4_UNORM_2PACK16,
            VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR = VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
            VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR = VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
            VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR = VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
            VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
            VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
            VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
            VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
            VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
            VK_FORMAT_G16B16G16R16_422_UNORM_KHR = VK_FORMAT_G16B16G16R16_422_UNORM,
            VK_FORMAT_B16G16R16G16_422_UNORM_KHR = VK_FORMAT_B16G16R16G16_422_UNORM,
            VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR = VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM,
            VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR = VK_FORMAT_G16_B16R16_2PLANE_420_UNORM,
            VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR = VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM,
            VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR = VK_FORMAT_G16_B16R16_2PLANE_422_UNORM,
            VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR = VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM,

            // Provided by VK_EXT_ycbcr_2plane_444_formats
            VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT = VK_FORMAT_G8_B8R8_2PLANE_444_UNORM,
            VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16,
            VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16,
            VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT = VK_FORMAT_G16_B16R16_2PLANE_444_UNORM,

            // Provided by VK_EXT_4444_formats
            VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT = VK_FORMAT_A4R4G4B4_UNORM_PACK16,
            VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT = VK_FORMAT_A4B4G4R4_UNORM_PACK16,

            // Provided by VK_NV_optical_flow
            VK_FORMAT_R16G16_S10_5_NV = VK_FORMAT_R16G16_SFIXED5_NV,

            // Provided by VK_KHR_maintenance5
            VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR = VK_FORMAT_A1B5G5R5_UNORM_PACK16,
            VK_FORMAT_A8_UNORM_KHR = VK_FORMAT_A8_UNORM,
            */
        }

        return name;
    }

    const char* getString(VkColorSpaceKHR colorSpace)
    {
        const char* name = "UNDEFINED";
        switch (colorSpace)
        {
            XCASE(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR); // deprecated

            // Provided by VK_EXT_swapchain_colorspace
            XCASE(VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT);
            XCASE(VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT);
            XCASE(VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT); // deprecated
            XCASE(VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT);
            XCASE(VK_COLOR_SPACE_BT709_LINEAR_EXT);
            XCASE(VK_COLOR_SPACE_BT709_NONLINEAR_EXT);
            XCASE(VK_COLOR_SPACE_BT2020_LINEAR_EXT);
            XCASE(VK_COLOR_SPACE_HDR10_ST2084_EXT);
            XCASE(VK_COLOR_SPACE_DOLBYVISION_EXT);
            XCASE(VK_COLOR_SPACE_HDR10_HLG_EXT);
            XCASE(VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT);
            XCASE(VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT);
            XCASE(VK_COLOR_SPACE_PASS_THROUGH_EXT);
            XCASE(VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT);

            // Provided by VK_AMD_display_native_hdr
            XCASE(VK_COLOR_SPACE_DISPLAY_NATIVE_AMD);
        }

        return name;
    }

    #undef XCASE

} // namespace mango::vulkan

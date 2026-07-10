/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vulkan/vulkan.h>

#include <string_view>
#include <unordered_set>
#include <vector>

#include <mango/core/configure.hpp>
#include <mango/window/window.hpp>
#include <mango/vulkan/colorspace.hpp>
#include <mango/vulkan/swapchain.hpp>
#include <mango/vulkan/compiler.hpp>

namespace mango::vulkan
{

    template <typename T>
    class VulkanHandle : public NonCopyable
    {
    protected:
        T m_handle = VK_NULL_HANDLE;

    public:
        operator T () const
        {
            return m_handle;
        }
    };

    class ExtensionProperties : protected NonCopyable
    {
    public:
        bool contains(std::string_view name) const;
        bool contains(const char* name) const;
        void print() const;

        size_t size() const
        {
            return m_properties.size();
        }

        bool empty() const
        {
            return m_properties.empty();
        }

        auto begin()
        {
            return m_properties.begin();
        }

        auto end()
        {
            return m_properties.end();
        }

        auto begin() const
        {
            return m_properties.begin();
        }

        auto end() const
        {
            return m_properties.end();
        }

    protected:
        ExtensionProperties(const std::vector<VkExtensionProperties>& properties);
        ~ExtensionProperties() = default;

        std::vector<VkExtensionProperties> m_properties;
        std::unordered_set<std::string_view> m_names;
    };

    class InstanceExtensionProperties : public ExtensionProperties
    {
    public:
        InstanceExtensionProperties(const char* layerName = nullptr);
        ~InstanceExtensionProperties() = default;
    };

    class DeviceExtensionProperties : public ExtensionProperties
    {
    public:
        DeviceExtensionProperties(VkPhysicalDevice physicalDevice);
        ~DeviceExtensionProperties() = default;
    };

    class Instance : public VulkanHandle<VkInstance>
    {
    public:
        Instance(const VkApplicationInfo& applicationInfo,
                 std::vector<const char*> layers,
                 std::vector<const char*> extensions);
        ~Instance();

    };

    class Device : public VulkanHandle<VkDevice>
    {
    public:
        Device(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& deviceCreateInfo);
        ~Device();
    };

    class CommandPool : public VulkanHandle<VkCommandPool>
    {
    protected:
        VkDevice m_device = VK_NULL_HANDLE;

    public:
        CommandPool(VkDevice device, uint32_t graphicsQueueFamilyIndex);
        ~CommandPool();
    };

    // -------------------------------------------------------------------
    // VulkanDeviceConfig
    // -------------------------------------------------------------------

    enum class SurfaceFormatIntent
    {
        PreferSDR,  // best SDR format; never pick HDR
        PreferHDR,  // best HDR if available, else best SDR
        ForceHDR,   // best HDR only; matchedIntent false when unavailable
    };

    struct SurfaceFormatSelection
    {
        VkSurfaceFormatKHR format {};
        SurfaceFormatIntent requested = SurfaceFormatIntent::PreferSDR;
        bool matchedIntent = false;
        bool isHdr = false;
        OutputTransform outputTransform = OutputTransform::Pass;
    };

    struct VulkanDeviceConfig
    {
        // VK_NULL_HANDLE selects via selectPhysicalDevice().
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

        // Swapchain surface formats in preference order (first supported match wins).
        // When empty, selectSurfaceFormat() uses surfaceFormatIntent and the built-in tier list.
        std::vector<VkSurfaceFormatKHR> preferredFormats;
        SurfaceFormatIntent surfaceFormatIntent = SurfaceFormatIntent::PreferSDR;

        // Optional pNext for VkDeviceCreateInfo (feature structs, etc.).
        // When null, VulkanWindow enables Vulkan 1.3 dynamic rendering by default.
        const void* deviceCreateInfoPNext = nullptr;

        // Appended to requiredDeviceExtensions().
        std::vector<const char*> deviceExtensions;
    };

    // -------------------------------------------------------------------
    // VulkanWindow
    // -------------------------------------------------------------------

    class VulkanWindow : public Window
    {
    protected:
        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;

        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        u32 m_graphicsQueueFamilyIndex = 0;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;

        VkSurfaceFormatKHR m_surfaceFormat {};
        VkExtent2D m_swapchainExtent { 0, 0 };

        std::unique_ptr<Swapchain> m_swapchain;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_commandBuffers;

        bool m_on_device_ready_called = false;

        void initDevice(const VulkanDeviceConfig* config);
        void destroyDevice();
        void allocateCommandBuffers();
        void ensureCommandBuffers();

        u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const;

    public:
        VulkanWindow(VkInstance instance, int width, int height, u32 flags = 0,
                     const VulkanDeviceConfig* config = nullptr);
        ~VulkanWindow();

        operator VkInstance () const
        {
            return m_instance;
        }

        bool isDeviceReady() const
        {
            return m_device != VK_NULL_HANDLE;
        }

        bool getPresentationSupport(VkPhysicalDevice physicalDevice, u32 queueFamilyIndex);

        VkInstance instance() const { return m_instance; }
        VkSurfaceKHR surface() const { return m_surface; }
        VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
        VkDevice device() const { return m_device; }
        VkQueue graphicsQueue() const { return m_graphicsQueue; }
        u32 graphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }
        VkSurfaceFormatKHR surfaceFormat() const { return m_surfaceFormat; }
        VkExtent2D swapchainExtent() const { return m_swapchainExtent; }

        Swapchain& swapchain();
        const Swapchain& swapchain() const;

        VkCommandPool commandPool() const { return m_commandPool; }
        VkCommandBuffer commandBuffer(u32 imageIndex) const;

        // Acquire a swapchain image (handles resize/recreate internally).
        // After a successful return, swapchainExtent() matches the drawable.
        // Calls onSwapchainResize() when the surface extent changes.
        Swapchain::Frame beginDraw();

        void enterEventLoop();
        void enterEventLoop(const EventLoopConfig& config);

        // Called once before the window is shown and the event loop starts.
        virtual void onDeviceReady();

        // Called when the swapchain extent changes (resize / recreate).
        // Rebuild extent-dependent resources (depth, pipelines with fixed sizes, etc.).
        virtual void onSwapchainResize(VkExtent2D extent);
    };

    VkPhysicalDevice selectPhysicalDevice(VkInstance instance);

    // Surface extensions required before any window/surface exists. The no-argument
    // form uses the process-global active window system (Window::getWindowSystem(),
    // resolving it if needed) so the instance and the window agree by construction.
    // The explicit overload is for callers that pin the system themselves.
    std::vector<const char*> requiredSurfaceExtensions();
    std::vector<const char*> requiredSurfaceExtensions(WindowSystem ws);
    std::vector<const char*> requiredDeviceExtensions();

    std::vector<VkLayerProperties>       getInstanceLayerProperties();
    std::vector<VkExtensionProperties>   getInstanceExtensionProperties(const char* layerName = nullptr);
    std::vector<VkPhysicalDevice>        getPhysicalDevices(VkInstance instance);
    std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice);
    std::vector<VkExtensionProperties>   getDeviceExtensionProperties(VkPhysicalDevice physicalDevice);
    std::vector<VkSurfaceFormatKHR>      getSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    // Recommended (format, colorSpace) pairs in preference order for HDR and SDR tiers.
    std::vector<VkSurfaceFormatKHR> getRecommendedSurfaceFormats(SurfaceFormatIntent intent);
    void applyRecommendedSurfaceFormats(VulkanDeviceConfig& config, SurfaceFormatIntent intent);

    SurfaceFormatSelection selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available,
                                               SurfaceFormatIntent intent);
    SurfaceFormatSelection selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available,
                                               bool hdr);
    SurfaceFormatSelection selectSurfaceFormat(VkPhysicalDevice physicalDevice,
                                               VkSurfaceKHR surface,
                                               SurfaceFormatIntent intent);

    void logSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                           const SurfaceFormatSelection* selected = nullptr);

    // Log formats for a live window using its bound swapchain format.
    void logSurfaceFormats(const VulkanWindow& window, SurfaceFormatIntent requested);

    std::string_view getString(VkResult result);
    std::string_view getString(VkPhysicalDeviceType deviceType);
    std::string_view getString(VkFormat format);
    std::string_view getString(VkColorSpaceKHR colorSpace);

} // namespace mango::vulkan

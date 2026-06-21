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

    class VulkanWindow : public Window
    {
    protected:
        VkInstance m_instance;
        VkSurfaceKHR m_surface;

    public:
        VulkanWindow(VkInstance instance, int width, int height, u32 flags);
        ~VulkanWindow();

        operator VkInstance () const
        {
            return m_instance;
        }

        bool getPresentationSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) const;
        void toggleFullscreen();
        bool isFullscreen() const;
    };

    VkPhysicalDevice selectPhysicalDevice(VkInstance instance);

    std::vector<const char*> requiredSurfaceExtensions();
    std::vector<const char*> requiredDeviceExtensions();

    std::vector<VkLayerProperties>       getInstanceLayerProperties();
    std::vector<VkExtensionProperties>   getInstanceExtensionProperties(const char* layerName = nullptr);
    std::vector<VkPhysicalDevice>        getPhysicalDevices(VkInstance instance);
    std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice);
    std::vector<VkExtensionProperties>   getDeviceExtensionProperties(VkPhysicalDevice physicalDevice);
    std::vector<VkSurfaceFormatKHR>      getSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    std::string_view getString(VkResult result);
    std::string_view getString(VkPhysicalDeviceType deviceType);
    std::string_view getString(VkFormat format);
    std::string_view getString(VkColorSpaceKHR colorSpace);

} // namespace mango::vulkan

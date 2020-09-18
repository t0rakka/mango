/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <mango//core/configure.hpp>
#include <mango//core/dynamic_library.hpp>
#include <mango//image/compression.hpp>
#include <mango//window/window.hpp>

// -----------------------------------------------------------------------
// MANGO Vulkan SDK
// -----------------------------------------------------------------------

// TODO: check vulkan_platform.h for supported platforms
// TODO: 32 bit ARM requires armv7a architecture in vulkan headers

#if defined(MANGO_PLATFORM_WINDOWS)

	#define MANGO_ENABLE_VULKAN
	#define MANGO_VULKAN_LIBRARY "vulkan-1.dll"

	#define VK_USE_PLATFORM_WIN32_KHR
	#define VK_NO_PROTOTYPES
	#include "khronos/vulkan.h"

#elif defined(MANGO_PLATFORM_ANDROID)

	// TODO

#elif defined(MANGO_PLATFORM_OSX)

	// NOTE: not supported

#elif defined(MANGO_PLATFORM_UNIX)

	#define MANGO_ENABLE_VULKAN
	#define MANGO_VULKAN_LIBRARY "libvulkan.so"

	#define VK_USE_PLATFORM_XCB_KHR
	#define VK_NO_PROTOTYPES
	#include "khronos/vulkan.h"

#else

	//#error "Unsupported Vulkan implementation."

#endif

#ifdef MANGO_ENABLE_VULKAN

// -----------------------------------------------------------------------
// Vulkan API
// -----------------------------------------------------------------------

// TODO: DEVICE_FUNCs should be in the device

//#define VULKAN_LIBRARY_FUNC(func)  extern PFN_##func func
#define VULKAN_INSTANCE_FUNC(func) extern PFN_##func func
#define VULKAN_DEVICE_FUNC(func)   extern PFN_##func func
#include "func/vk_func.hpp"

namespace mango {
namespace vulkan {

	const char* getResultString(VkResult result);
	const char* getPresentModeString(VkPresentModeKHR presentMode);
	void checkResult(VkResult result, const std::string& function);
	TextureCompression getCompressionFormat(Format& format, VkFormat vk_format);

	class Library
	{
	protected:
		DynamicLibrary m_library;

#define VULKAN_LIBRARY_FUNC(func) PFN_##func func { nullptr }
#include "func/vk_func.hpp"

	public:
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr { nullptr };

		Library();
		~Library();

		std::vector<VkLayerProperties> enumerateInstanceLayerProperties() const;
		std::vector<VkExtensionProperties> enumerateInstanceExtensionProperties() const;
		VkInstance createInstance(const VkInstanceCreateInfo& pCreateInfo);
	};

	class Instance
	{
	protected:
		Library m_library;
		VkInstance m_instance;

	public:
		Instance();
		~Instance();

		VkSurfaceKHR createWindowSurface(WindowHandle* windowHandle) const;
		VkPhysicalDevice selectPhysicalDevice(VkSurfaceKHR presentSurface, uint32_t& graphicsQueueFamilyIndex, uint32_t& presentQueueFamilyIndex) const;

		operator VkInstance () const
		{
			return m_instance;
		}
	};

    class Device
    {
    protected:
        VkPhysicalDevice m_physicalDevice { VK_NULL_HANDLE };
        VkDevice m_device { VK_NULL_HANDLE };

    public:
        Device();
        ~Device();

        operator VkPhysicalDevice () const
        {
            return m_physicalDevice;
        }

        operator VkDevice () const
        {
            return m_device;
        }
    };

	class Swapchain
	{
	protected:
		VkDevice m_device;
		VkSwapchainKHR m_swapchain { VK_NULL_HANDLE };

	public:
		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;
		std::vector<VkCommandBuffer> commandBuffers;

		Swapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, VkCommandPool commandPool, VkQueue queue);
		~Swapchain();

		operator VkSwapchainKHR () const
		{
			return m_swapchain;
		}
	};

	class Context : public Window
	{
	private:
		void initDevice(); // TODO: move into Device

	protected:
		Instance m_instance;

		VkSurfaceKHR m_presentSurface { VK_NULL_HANDLE };
		VkPhysicalDevice m_physicalDevice { VK_NULL_HANDLE };
		VkDevice m_device { VK_NULL_HANDLE };
		VkCommandPool m_commandPool { VK_NULL_HANDLE };
		VkQueue m_queue; // TODO: separate into graphics and present queues
		Swapchain* m_swapchain { nullptr };

	public:
		Context(int width, int height);
		~Context();

		operator VkInstance () const
		{
			return m_instance;
		}
	};

} // namespace vulkan
} // namespace mango

#endif // MANGO_ENABLE_VULKAN

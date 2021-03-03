/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/vulkan/vulkan.hpp>
#include <mango/core/exception.hpp>

#ifdef MANGO_ENABLE_VULKAN


#ifdef VK_KHR_xlib_surface
#include "../window/xlib/xlib_handle.hpp"
#endif

#ifdef VK_KHR_xcb_surface
#include "../window/xcb/xcb_handle.hpp"
#endif

#ifdef VK_KHR_wayland_surface
#include "../window/wayland/wayland_handle.hpp"
#endif

#ifdef VK_KHR_mir_surface
#include "../window/mir/mir_handle.hpp"
#endif

#ifdef VK_KHR_android_surface
#include "../window/android/android_handle.hpp"
#endif

#ifdef VK_KHR_win32_surface
#include "../window/win32/win32_handle.hpp"
#endif


static PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr { nullptr };

// function objects
#define VULKAN_INSTANCE_FUNC(func) PFN_##func func { nullptr };
#define VULKAN_DEVICE_FUNC(func)   PFN_##func func { nullptr };
#include <mango/vulkan/func/vk_func.hpp>

namespace
{
	using namespace mango::vulkan;

	void initInstanceProc(Library& library, VkInstance instance)
	{
        printf("instance procs:\n");
#define VULKAN_INSTANCE_FUNC(func) \
    func = (PFN_##func) library.vkGetInstanceProcAddr(instance, #func); \
    printf("  %d vkGetInstanceProcAddr(%s)\n", int(func != nullptr), #func)
#include <mango/vulkan/func/vk_func.hpp>

		vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr) library.vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
		if (!vkGetDeviceProcAddr)
		{
			MANGO_EXCEPTION("[Vulkan] vkGetDeviceProcAddr not found.");
		}
	}

	void initDeviceProc(VkDevice device)
	{
        printf("device procs:\n");
#define VULKAN_DEVICE_FUNC(func) \
    func = (PFN_##func) vkGetDeviceProcAddr(device, #func); \
    printf("  %d vkGetDeviceProcAddr(%s)\n", int(func != nullptr), #func)
#include <mango/vulkan/func/vk_func.hpp>
	}

} // namespace

namespace mango::vulkan
{
    using namespace mango::image;

	// -----------------------------------------------------------------------
	// Library
	// -----------------------------------------------------------------------

	Library::Library()
		: m_library(MANGO_VULKAN_LIBRARY)
	{
		// initialize exported function
		vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) m_library.address("vkGetInstanceProcAddr");
		if (!vkGetInstanceProcAddr)
		{
			MANGO_EXCEPTION("[Vukan] vkGetInstanceProcAddr not found.");
		}

#define VULKAN_LIBRARY_FUNC(func) \
    func = (PFN_##func) vkGetInstanceProcAddr(nullptr, #func)
#include <mango/vulkan/func/vk_func.hpp>
	}

	Library::~Library()
	{
	}

	std::vector<VkLayerProperties> Library::enumerateInstanceLayerProperties() const
	{
		std::vector<VkLayerProperties> instanceLayerProperties;
		VkResult result;

		uint32_t instanceLayerCount;
		result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		vulkan::checkResult(result, "vkEnumerateInstanceLayerProperties");

		if (instanceLayerCount > 0)
		{
			instanceLayerProperties.resize(instanceLayerCount);
			result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
			vulkan::checkResult(result, "vkEnumerateInstanceLayerProperties");
		}

		return instanceLayerProperties;
	}

	std::vector<VkExtensionProperties> Library::enumerateInstanceExtensionProperties() const
	{
		std::vector<VkExtensionProperties> instanceExtensionProperties;
		VkResult result;

		uint32_t instanceExtensionCount;
		result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
		vulkan::checkResult(result, "vkEnumerateInstanceExtensionProperties");

		if (instanceExtensionCount > 0)
		{
			instanceExtensionProperties.resize(instanceExtensionCount);
			result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensionProperties.data());
			vulkan::checkResult(result, "vkEnumerateInstanceExtensionProperties");
		}

		return instanceExtensionProperties;
	}

	VkInstance Library::createInstance(const VkInstanceCreateInfo& createInfo)
	{
		VkInstance instance;
		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
		vulkan::checkResult(result, "vkCreateInstance");
		return instance;
	}

	// -----------------------------------------------------------------------
	// Instance
	// -----------------------------------------------------------------------

	Instance::Instance()
	{
		// enumerate instance layer properties
		auto instanceLayerProperties = m_library.enumerateInstanceLayerProperties();

		printf("instance layers: %d\n", int(instanceLayerProperties.size()));
		std::vector<const char*> enabledInstanceLayers;

		for (auto layer : instanceLayerProperties)
		{
			printf("  \"%s\"\n", layer.layerName);
		}

		// enumerate instance extension properties
		auto instanceExtensionProperties = m_library.enumerateInstanceExtensionProperties();

		printf("instance extensions: %d\n", int(instanceExtensionProperties.size()));
		std::vector<const char*> enabledInstanceExtensions;

		for (auto extension : instanceExtensionProperties)
		{
			auto name = extension.extensionName;
			printf("  \"%s\"\n", name);

			if (!strcmp(name, VK_KHR_SURFACE_EXTENSION_NAME))
			{
				enabledInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
			}
#ifdef VK_KHR_xlib_surface
			else if (!strcmp(name, VK_KHR_XLIB_SURFACE_EXTENSION_NAME))
			{
				enabledInstanceExtensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
			}
#endif

#ifdef VK_KHR_xcb_surface
			else if (!strcmp(name, VK_KHR_XCB_SURFACE_EXTENSION_NAME))
			{
				enabledInstanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
			}
#endif

#ifdef VK_KHR_wayland_surface
			else if (!strcmp(name, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME))
			{
				enabledInstanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
			}
#endif

#ifdef VK_KHR_mir_surface
			else if (!strcmp(name, VK_KHR_MIR_SURFACE_EXTENSION_NAME))
			{
				enabledInstanceExtensions.push_back(VK_KHR_MIR_SURFACE_EXTENSION_NAME);
			}
#endif

#ifdef VK_KHR_android_surface
			else if (!strcmp(name, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME))
			{
				enabledInstanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
			}
#endif

#ifdef VK_KHR_win32_surface
			else if (!strcmp(name, VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
			{
				enabledInstanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
			}
#endif
		}

		// create instance
		VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceCreateInfo.flags = 0;
		instanceCreateInfo.pApplicationInfo = nullptr;
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledInstanceLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = enabledInstanceLayers.data();
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();

		m_instance = m_library.createInstance(instanceCreateInfo);
		initInstanceProc(m_library, m_instance);
	}

	Instance::~Instance()
	{
		//vkDestroyInstance(m_instance, nullptr);
	}

	VkSurfaceKHR Instance::createWindowSurface(WindowHandle* windowHandle) const
	{
		assert(windowHandle != nullptr);

		VkResult result;
		VkSurfaceKHR surface;

#ifdef VK_KHR_xlib_surface
		const VkXlibSurfaceCreateInfoKHR surfaceCreateInfo = {
			VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
			nullptr,
			0,
			windowHandle->display,
			windowHandle->window
		};

		result = vkCreateXlibSurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &surface);
#endif

#ifdef VK_KHR_xcb_surface
		const VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {
			VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
			nullptr,
			0,
			windowHandle->connection,
			windowHandle->window
		};

		result = vkCreateXcbSurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &surface);
#endif

#ifdef VK_KHR_wayland_surface
		const VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {
			VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
			nullptr,
			0,
			windowHandle->display,
			windowHandle->surface
		};

		result = vkCreateWaylandSurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &surface);
#endif

#ifdef VK_KHR_mir_surface
		const VkMirSurfaceCreateInfoKHR surfaceCreateInfo = {
			VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR,
			nullptr,
			0,
			windowHandle->connection,
			windowHandle->surface
		};

		result = vkCreateMirSurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &surface);
#endif

#ifdef VK_KHR_android_surface
		const VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {
			VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
			nullptr,
			0,
			windowHandle->window
		};

		result = vkCreateAndroidSurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &surface);
#endif

#ifdef VK_KHR_win32_surface
		const VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
			VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			nullptr,
			0,
			GetModuleHandle(NULL),
			windowHandle->hwnd
		};

		result = vkCreateWin32SurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &surface);
#endif

        checkResult(result, "vkCreateSurface");
		return surface;
	}

	VkPhysicalDevice Instance::selectPhysicalDevice(VkSurfaceKHR presentSurface, uint32_t& graphicsQueueFamilyIndex, uint32_t& presentQueueFamilyIndex) const
	{
		VkResult result;

		// selected configuration
		int physicalDeviceScore = 0;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

		graphicsQueueFamilyIndex = UINT32_MAX;
		presentQueueFamilyIndex = UINT32_MAX;

		// enumerate physical devices

		uint32_t physicalDeviceCount;
		result = vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);

		printf("physical devices: %d\n", physicalDeviceCount);
		if (physicalDeviceCount > 0)
		{
			std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
			result = vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

			for (uint32_t i = 0; i < physicalDeviceCount; ++i)
			{
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);
				printf("  name: \"%s\", type: %d\n", properties.deviceName, properties.deviceType);

				// compute device score
				int score = 0;
				switch (properties.deviceType)
				{
					case VK_PHYSICAL_DEVICE_TYPE_OTHER: score = 1; break;
					case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score = 4; break;
					case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score = 5; break;
					case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score = 3; break;
					case VK_PHYSICAL_DEVICE_TYPE_CPU: score = 2; break;
					default: score = 0; break;
				}

				uint32_t queueFamilyPropertyCount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyPropertyCount, nullptr);
				printf("graphics queue families: %d\n", queueFamilyPropertyCount);

				// enumerate queue families for the physical device

				if (queueFamilyPropertyCount > 0)
				{
					std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
					vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyPropertyCount, queueFamilyProperties.data());

					uint32_t graphicsIndex = UINT32_MAX;
					uint32_t presentIndex = UINT32_MAX;
					uint32_t graphicsAndPresentIndex = UINT32_MAX;

					for (uint32_t familyIndex = 0; familyIndex < queueFamilyPropertyCount; ++familyIndex)
					{
						const VkQueueFamilyProperties& properties = queueFamilyProperties[familyIndex];

						// print properties
						printf("  queue family %d: [", familyIndex);
						if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) printf("G");
						if (properties.queueFlags & VK_QUEUE_COMPUTE_BIT) printf("C");
						if (properties.queueFlags & VK_QUEUE_TRANSFER_BIT) printf("T");
						if (properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) printf("S");
						printf("], count: %d\n", properties.queueCount);

						// check graphics support
						bool graphicsSupport = (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
						if (graphicsSupport)
						{
							graphicsIndex = familyIndex;
						}

						// check presentation support
						VkBool32 presentSupport = VK_FALSE;
						result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], familyIndex, presentSurface, &presentSupport);
						if (presentSupport)
						{
							presentIndex = familyIndex;
							if (graphicsSupport)
							{
								// queue supports both present and graphics -> perfect
								graphicsAndPresentIndex = familyIndex;
							}
						}
					}

					// TODO: graphicsAndPresentIndex is superior and should be chosen
					(void)graphicsAndPresentIndex;

					if (graphicsIndex != UINT32_MAX && presentIndex != UINT32_MAX)
					{
						// select device with the highest score
						if (score > physicalDeviceScore)
						{
							physicalDeviceScore = score;
							physicalDevice = physicalDevices[i];
							presentQueueFamilyIndex = presentIndex;
							graphicsQueueFamilyIndex = graphicsIndex;
						}
					}
				}
			}
		}

		MANGO_UNREFERENCED(result);
		return physicalDevice;
	}

	// -----------------------------------------------------------------------
	// Device
	// -----------------------------------------------------------------------

	Device::Device()
	{
	}

	Device::~Device()
	{
	}

	// -----------------------------------------------------------------------
	// Swapchain
	// -----------------------------------------------------------------------

	Swapchain::Swapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, VkCommandPool commandPool, VkQueue queue)
	: m_device(device)
	{
		VkResult result;

		// enumerate surface formats

		VkFormat colorFormat;
		VkColorSpaceKHR colorSpace;

		uint32_t surfaceFormatsCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, nullptr);
		printf("surface formats: %d\n", surfaceFormatsCount);

		if (surfaceFormatsCount > 0)
		{
			std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatsCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, surfaceFormats.data());

			// choose first format
			colorSpace = surfaceFormats[0].colorSpace;
			colorFormat = surfaceFormats[0].format;

			if (colorFormat == VK_FORMAT_UNDEFINED)
				colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
		}
		else
		{
			throw "no surface formats!";
		}

		// choose present mode

		VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
		printf("present modes: %d\n", presentModeCount);

		if (presentModeCount > 0)
		{
			std::vector<VkPresentModeKHR> presentModes(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

			for (size_t i = 0; i < presentModeCount; i++)
			{
				printf("  %s\n", getPresentModeString(presentModes[i]));

				if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}

				if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
				{
					swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				}
			}
		}

		printf("selected present mode: %s\n", getPresentModeString(swapchainPresentMode));

		// create swapchain

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

		if (surfaceCapabilities.currentExtent.width == 0xffffffff)
			throw "Undefined surface capabilities.";

		auto swapchainExtent = surfaceCapabilities.currentExtent;
		printf("swapChainExtent: %d x %d\n", swapchainExtent.width, swapchainExtent.height);

		// Determine the number of images
		uint32_t desiredImageCount = surfaceCapabilities.minImageCount + 1;

		if ((surfaceCapabilities.maxImageCount > 0) && (desiredImageCount > surfaceCapabilities.maxImageCount))
		{
			desiredImageCount = surfaceCapabilities.maxImageCount;
		}

		VkSurfaceTransformFlagsKHR swapchain_pre_transform = surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ?
			VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : surfaceCapabilities.currentTransform;

		VkSwapchainCreateInfoKHR swapchainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		swapchainCreateInfo.flags = 0;
		swapchainCreateInfo.surface = surface;
		swapchainCreateInfo.minImageCount = desiredImageCount;
		swapchainCreateInfo.imageFormat = colorFormat;
		swapchainCreateInfo.imageColorSpace = colorSpace;
		swapchainCreateInfo.imageExtent = swapchainExtent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
		swapchainCreateInfo.preTransform = (VkSurfaceTransformFlagBitsKHR)swapchain_pre_transform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = swapchainPresentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		result = vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain);
		vulkan::checkResult(result, "vkCreateSwapchainKHR");

		// get swapchain images

		uint32_t swapchainImageCount = 0;
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, nullptr);
		printf("swapchain images: %d\n", swapchainImageCount);

		if (swapchainImageCount < 1)
		{
			// TODO: error
		}

		images.resize(swapchainImageCount);
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, images.data());

		imageViews.resize(swapchainImageCount);

		// create setup command buffer

		VkCommandBuffer setupCommandBuffer;

		VkCommandBufferAllocateInfo commandBufferAllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		commandBufferAllocateInfo.commandPool = commandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = 1;

		result = vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &setupCommandBuffer);
		vulkan::checkResult(result, "vkAllocateCommandBuffers");

		VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;

		result = vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo);
		vulkan::checkResult(result, "vkBeginCommandBuffer");

		for (uint32_t i = 0; i < swapchainImageCount; i++)
		{
			// set image layout - barrier from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.image = images[i];
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

			VkImageViewCreateInfo swapchainColorAttachmentView = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			swapchainColorAttachmentView.flags = 0;
			swapchainColorAttachmentView.image = images[i];
			swapchainColorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			swapchainColorAttachmentView.format = colorFormat;
			swapchainColorAttachmentView.components.r = VK_COMPONENT_SWIZZLE_R;
			swapchainColorAttachmentView.components.g = VK_COMPONENT_SWIZZLE_G;
			swapchainColorAttachmentView.components.b = VK_COMPONENT_SWIZZLE_B;
			swapchainColorAttachmentView.components.a = VK_COMPONENT_SWIZZLE_A;
			swapchainColorAttachmentView.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCreateImageView(m_device, &swapchainColorAttachmentView, nullptr, &imageViews[i]);
		}

		vkEndCommandBuffer(setupCommandBuffer);

		VkSubmitInfo commandBufferSubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		commandBufferSubmitInfo.waitSemaphoreCount = 0;
		commandBufferSubmitInfo.pWaitSemaphores = nullptr;
		commandBufferSubmitInfo.pWaitDstStageMask = nullptr;
		commandBufferSubmitInfo.commandBufferCount = 1;
		commandBufferSubmitInfo.pCommandBuffers = &setupCommandBuffer;
		commandBufferSubmitInfo.signalSemaphoreCount = 0;
		commandBufferSubmitInfo.pSignalSemaphores = nullptr;

		result = vkQueueSubmit(queue, 1, &commandBufferSubmitInfo, VK_NULL_HANDLE);
		vulkan::checkResult(result, "vkQueueSubmit");
	}

	Swapchain::~Swapchain()
	{
		if (m_swapchain != VK_NULL_HANDLE)
			vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	}

    // -----------------------------------------------------------------------
    // Context
    // -----------------------------------------------------------------------

    Context::Context(int width, int height)
    : Window(width, height)
    {
		m_presentSurface = m_instance.createWindowSurface(m_handle);
		initDevice();
		m_swapchain = new Swapchain(m_physicalDevice, m_device, m_presentSurface, m_commandPool, m_queue);
	}

    Context::~Context()
    {
		if (m_device != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_device);

			delete m_swapchain;

			if (m_commandPool != VK_NULL_HANDLE)
				vkDestroyCommandPool(m_device, m_commandPool, nullptr);

			vkDestroyDevice(m_device, nullptr);
		}

		if (m_presentSurface != VK_NULL_HANDLE)
			vkDestroySurfaceKHR(m_instance, m_presentSurface, nullptr);
    }

	void Context::initDevice()
	{
		VkResult result;

		// configure physical device
		uint32_t graphicsQueueFamilyIndex;
		uint32_t presentQueueFamilyIndex;
		VkPhysicalDevice physicalDevice = m_instance.selectPhysicalDevice(m_presentSurface, graphicsQueueFamilyIndex, presentQueueFamilyIndex);

		if (physicalDevice == VK_NULL_HANDLE)
		{
			throw "PhysicalDevices with graphics queue and presentation support not detected.";
		}

		m_physicalDevice = physicalDevice;

		// enumerate device layers

		std::vector<const char*> enabledDeviceLayers;

		uint32_t deviceLayerCount;
		result = vkEnumerateDeviceLayerProperties(physicalDevice, &deviceLayerCount, nullptr);

		printf("device layers: %d\n", deviceLayerCount);
		if (deviceLayerCount > 0)
		{
			std::vector<VkLayerProperties> deviceLayers(deviceLayerCount);
			result = vkEnumerateDeviceLayerProperties(physicalDevice, &deviceLayerCount, deviceLayers.data());

			for (auto layer : deviceLayers)
			{
				printf("  %s\n", layer.layerName);
			}
		}

		// enumerate device extensions

		std::vector<const char*> enabledDeviceExtensions;

		uint32_t deviceExtensionCount;
		result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);

		printf("device extensions: %d\n", deviceExtensionCount);
		if (deviceExtensionCount > 0)
		{
			std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
			result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data());

			for (uint32_t i = 0; i < deviceExtensionCount; ++i)
			{
				const char* extension = deviceExtensions[i].extensionName;
				printf("  %s\n", extension);

				if (!strcmp(extension, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
				{
					enabledDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
				}
			}
		}

		// create device

		const float queuePriorities[]
		{
			0.0f
		};

		VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queueCreateInfo.flags = 0;
		queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = queuePriorities;

		VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledDeviceLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = enabledDeviceLayers.data();
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();
		deviceCreateInfo.pEnabledFeatures = nullptr;

		result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &m_device);
		vulkan::checkResult(result, "vkCreateDevice");

		initDeviceProc(m_device);

		// get device queue handle
		vkGetDeviceQueue(m_device, graphicsQueueFamilyIndex, 0, &m_queue);

		// create command pool
		VkCommandPoolCreateInfo commandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

		result = vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_commandPool);
		vulkan::checkResult(result, "vkCreateCommandPool");
	}

	// -----------------------------------------------------------------------
	// functions
	// -----------------------------------------------------------------------

	const char* getResultString(VkResult result)
	{
#define CASE(x) case x: name = #x; break
		const char* name;
		switch (result)
		{
			CASE(VK_SUCCESS);
			CASE(VK_NOT_READY);
			CASE(VK_TIMEOUT);
			CASE(VK_EVENT_SET);
			CASE(VK_EVENT_RESET);
			CASE(VK_INCOMPLETE);
			CASE(VK_ERROR_OUT_OF_HOST_MEMORY);
			CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY);
			CASE(VK_ERROR_INITIALIZATION_FAILED);
			CASE(VK_ERROR_DEVICE_LOST);
			CASE(VK_ERROR_MEMORY_MAP_FAILED);
			CASE(VK_ERROR_LAYER_NOT_PRESENT);
			CASE(VK_ERROR_EXTENSION_NOT_PRESENT);
			CASE(VK_ERROR_FEATURE_NOT_PRESENT);
			CASE(VK_ERROR_INCOMPATIBLE_DRIVER);
			CASE(VK_ERROR_TOO_MANY_OBJECTS);
			CASE(VK_ERROR_FORMAT_NOT_SUPPORTED);
			CASE(VK_ERROR_SURFACE_LOST_KHR);
			CASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
			CASE(VK_SUBOPTIMAL_KHR);
			CASE(VK_ERROR_OUT_OF_DATE_KHR);
			CASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
			CASE(VK_ERROR_VALIDATION_FAILED_EXT);
			default: name = "UNKNOWN"; break;
		}
		return name;
#undef CASE
	}

	const char* getPresentModeString(VkPresentModeKHR presentMode)
	{
		const char* name = "UNKNOWN";
		switch (presentMode)
		{
			case VK_PRESENT_MODE_IMMEDIATE_KHR: name = "IMMEDIATE"; break;
			case VK_PRESENT_MODE_MAILBOX_KHR: name = "MAILBOX"; break;
			case VK_PRESENT_MODE_FIFO_KHR: name = "FIFO"; break;
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR: name = "FIFO_RELAXED"; break;
			default: name = "UNKNOWN"; break;
		}
		return name;
	}

	void checkResult(VkResult result, const std::string& function)
	{
		if (result != VK_SUCCESS)
		{
			std::string error = makeString("[Vulkan] %s -> %s",
				function.c_str(),
				getResultString(result));
			const char* str = error.c_str();

			if (result < 0)
			{
				// error
				MANGO_EXCEPTION(str);
			}
			else
			{
				// information
				printf("%s\n", str);
			}
		}
	}

	TextureCompression getCompressionFormat(Format& format, VkFormat vk_format)
	{
		struct FormatDesc
		{
			Format format;
			TextureCompression compression;
		};

		const FormatDesc table[] =
		{
			{ Format(), TextureCompression::NONE },

			{ Format(8, Format::UNORM, Format::RG, 4, 4, 0, 0), TextureCompression::NONE },

			{ Format(16, Format::UNORM, Format::RGBA, 4, 4, 4, 4), TextureCompression::NONE },
			{ Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4), TextureCompression::NONE },
			{ Format(16, Format::UNORM, Format::RGB, 5, 6, 5, 0), TextureCompression::NONE },
			{ Format(16, Format::UNORM, Format::BGR, 5, 6, 5, 0), TextureCompression::NONE },
			{ Format(16, Format::UNORM, Format::RGBA, 5, 5, 5, 1), TextureCompression::NONE },
			{ Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 1), TextureCompression::NONE },
			{ Format(16, Format::UNORM, Format::ARGB, 1, 5, 5, 5), TextureCompression::NONE },

			{ Format(8, Format::UNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE },
			{ Format(8, Format::SNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE },
			{ Format(8, Format::UNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE },
			{ Format(8, Format::SNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE },
			{ Format(8, Format::UINT, Format::R, 8, 0, 0, 0), TextureCompression::NONE },
			{ Format(8, Format::SINT, Format::R, 8, 0, 0, 0), TextureCompression::NONE },
			{ Format(8, Format::UNORM, Format::R, 8, 0, 0, 0), TextureCompression::NONE }, // srgb

			{ Format(16, Format::UNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE },
			{ Format(16, Format::SNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE },
			{ Format(16, Format::UNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE },
			{ Format(16, Format::SNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE },
			{ Format(16, Format::UINT, Format::RG, 8, 8, 0, 0), TextureCompression::NONE },
			{ Format(16, Format::SINT, Format::RG, 8, 8, 0, 0), TextureCompression::NONE },
			{ Format(16, Format::UNORM, Format::RG, 8, 8, 0, 0), TextureCompression::NONE }, // srgb

			{ Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE },
			{ Format(24, Format::SNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE },
			{ Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE },
			{ Format(24, Format::SNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE },
			{ Format(24, Format::UINT, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE },
			{ Format(24, Format::SINT, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE },
			{ Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0), TextureCompression::NONE }, // srgb

			{ Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE },
			{ Format(24, Format::SNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE },
			{ Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE },
			{ Format(24, Format::SNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE },
			{ Format(24, Format::UINT, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE },
			{ Format(24, Format::SINT, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE },
			{ Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0), TextureCompression::NONE }, // srgb

			{ Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::SNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::SNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::UINT, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::SINT, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), TextureCompression::NONE }, // srgb

			{ Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::SNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::SNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::UINT, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::SINT, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), TextureCompression::NONE }, // srgb

			{ Format(32, Format::UNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::SNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::UNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::SNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::UINT, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::SINT, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE },
			{ Format(32, Format::UNORM, Format::ABGR, 8, 8, 8, 8), TextureCompression::NONE }, // srgb

			{ Format(32, Format::UNORM, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE },
			{ Format(32, Format::SNORM, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE },
			{ Format(32, Format::UNORM, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE },
			{ Format(32, Format::SNORM, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE },
			{ Format(32, Format::UINT, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE },
			{ Format(32, Format::SINT, Format::ARGB, 2, 10, 10, 10), TextureCompression::NONE },

			{ Format(32, Format::UNORM, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE },
			{ Format(32, Format::SNORM, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE },
			{ Format(32, Format::UNORM, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE },
			{ Format(32, Format::SNORM, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE },
			{ Format(32, Format::UINT, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE },
			{ Format(32, Format::SINT, Format::ABGR, 2, 10, 10, 10), TextureCompression::NONE },

			{ Format(16, Format::UNORM, Format::R, 16, 0, 0, 0), TextureCompression::NONE },
			{ Format(16, Format::SNORM, Format::R, 16, 0, 0, 0), TextureCompression::NONE },
			{ Format(16, Format::UNORM, Format::R, 16, 0, 0, 0), TextureCompression::NONE },
			{ Format(16, Format::SNORM, Format::R, 16, 0, 0, 0), TextureCompression::NONE },
			{ Format(16, Format::UINT, Format::R, 16, 0, 0, 0), TextureCompression::NONE },
			{ Format(16, Format::SINT, Format::R, 16, 0, 0, 0), TextureCompression::NONE },
			{ Format(16, Format::FLOAT16, Format::R, 16, 0, 0, 0), TextureCompression::NONE },

			{ Format(32, Format::UNORM, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },
			{ Format(32, Format::SNORM, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },
			{ Format(32, Format::UNORM, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },
			{ Format(32, Format::SNORM, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },
			{ Format(32, Format::UINT, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },
			{ Format(32, Format::SINT, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },
			{ Format(32, Format::FLOAT16, Format::RG, 16, 16, 0, 0), TextureCompression::NONE },

			{ Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },
			{ Format(48, Format::SNORM, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },
			{ Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },
			{ Format(48, Format::SNORM, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },
			{ Format(48, Format::UINT, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },
			{ Format(48, Format::SINT, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },
			{ Format(48, Format::FLOAT16, Format::RGB, 16, 16, 16, 0), TextureCompression::NONE },

			{ Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },
			{ Format(64, Format::SNORM, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },
			{ Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },
			{ Format(64, Format::SNORM, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },
			{ Format(64, Format::UINT, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },
			{ Format(64, Format::SINT, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },
			{ Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16), TextureCompression::NONE },

			{ Format(32, Format::UINT, Format::R, 32, 0, 0, 0), TextureCompression::NONE },
			{ Format(32, Format::SINT, Format::R, 32, 0, 0, 0), TextureCompression::NONE },
			{ Format(32, Format::FLOAT32, Format::R, 32, 0, 0, 0), TextureCompression::NONE },

			{ Format(64, Format::UINT, Format::RG, 32, 32, 0, 0), TextureCompression::NONE },
			{ Format(64, Format::SINT, Format::RG, 32, 32, 0, 0), TextureCompression::NONE },
			{ Format(64, Format::FLOAT32, Format::RG, 32, 32, 0, 0), TextureCompression::NONE },

			{ Format(96, Format::UINT, Format::RGB, 32, 32, 32, 0), TextureCompression::NONE },
			{ Format(96, Format::SINT, Format::RGB, 32, 32, 32, 0), TextureCompression::NONE },
			{ Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32, 0), TextureCompression::NONE },

			{ Format(128, Format::UINT, Format::RGBA, 32, 32, 32, 32), TextureCompression::NONE },
			{ Format(128, Format::SINT, Format::RGBA, 32, 32, 32, 32), TextureCompression::NONE },
			{ Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32), TextureCompression::NONE },

			{ Format(64, Format::UINT, Format::R, 64, 0, 0, 0), TextureCompression::NONE },
			{ Format(64, Format::SINT, Format::R, 64, 0, 0, 0), TextureCompression::NONE },
			{ Format(64, Format::FLOAT64, Format::R, 64, 0, 0, 0), TextureCompression::NONE },

			{ Format(128, Format::UINT, Format::RG, 64, 64, 0, 0), TextureCompression::NONE },
			{ Format(128, Format::SINT, Format::RG, 64, 64, 0, 0), TextureCompression::NONE },
			{ Format(128, Format::FLOAT64, Format::RG, 64, 64, 0, 0), TextureCompression::NONE },

			{ Format(192, Format::UINT, Format::RGB, 64, 64, 64, 0), TextureCompression::NONE },
			{ Format(192, Format::SINT, Format::RGB, 64, 64, 64, 0), TextureCompression::NONE },
			{ Format(192, Format::FLOAT64, Format::RGB, 64, 64, 64, 0), TextureCompression::NONE },

			{ Format(256, Format::UINT, Format::RGBA, 64, 64, 64, 64), TextureCompression::NONE },
			{ Format(256, Format::SINT, Format::RGBA, 64, 64, 64, 64), TextureCompression::NONE },
			{ Format(256, Format::FLOAT64, Format::RGBA, 64, 64, 64, 64), TextureCompression::NONE },

			{ Format(), TextureCompression::R11F_G11F_B10F },
			{ Format(), TextureCompression::RGB9_E5 },

			{ Format(), TextureCompression::NONE }, // VK_FORMAT_D16_UNORM
			{ Format(), TextureCompression::NONE }, // VK_FORMAT_X8_D24_UNORM_PACK32
			{ Format(), TextureCompression::NONE }, // VK_FORMAT_D32_SFLOAT
			{ Format(), TextureCompression::NONE }, // VK_FORMAT_S8_UINT
			{ Format(), TextureCompression::NONE }, // VK_FORMAT_D16_UNORM_S8_UINT
			{ Format(), TextureCompression::NONE }, // VK_FORMAT_D24_UNORM_S8_UINT
			{ Format(), TextureCompression::NONE }, // VK_FORMAT_D32_SFLOAT_S8_UINT

			{ Format(), TextureCompression::BC1_UNORM },
			{ Format(), TextureCompression::BC1_UNORM_SRGB },
			{ Format(), TextureCompression::BC1_UNORM_ALPHA },
			{ Format(), TextureCompression::BC1_UNORM_ALPHA_SRGB },
			{ Format(), TextureCompression::BC2_UNORM },
			{ Format(), TextureCompression::BC2_UNORM_SRGB },
			{ Format(), TextureCompression::BC3_UNORM },
			{ Format(), TextureCompression::BC3_UNORM_SRGB },
			{ Format(), TextureCompression::BC4_UNORM },
			{ Format(), TextureCompression::BC4_SNORM },
			{ Format(), TextureCompression::BC5_UNORM },
			{ Format(), TextureCompression::BC5_SNORM },
			{ Format(), TextureCompression::BC6H_UF16 },
			{ Format(), TextureCompression::BC6H_SF16 },
			{ Format(), TextureCompression::BC7_UNORM },
			{ Format(), TextureCompression::BC7_UNORM_SRGB },
			{ Format(), TextureCompression::ETC2_RGB },
			{ Format(), TextureCompression::ETC2_SRGB },
			{ Format(), TextureCompression::ETC2_RGB_ALPHA1 },
			{ Format(), TextureCompression::ETC2_SRGB_ALPHA1 },
			{ Format(), TextureCompression::ETC2_RGBA },
			{ Format(), TextureCompression::ETC2_SRGB_ALPHA8 },
			{ Format(), TextureCompression::EAC_R11 },
			{ Format(), TextureCompression::EAC_SIGNED_R11 },
			{ Format(), TextureCompression::EAC_RG11 },
			{ Format(), TextureCompression::EAC_SIGNED_RG11 },
			{ Format(), TextureCompression::ASTC_RGBA_4x4 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_4x4 },
			{ Format(), TextureCompression::ASTC_RGBA_5x4 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_5x4 },
			{ Format(), TextureCompression::ASTC_RGBA_5x5 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_5x5 },
			{ Format(), TextureCompression::ASTC_RGBA_6x5 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_6x5 },
			{ Format(), TextureCompression::ASTC_RGBA_6x6 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_6x6 },
			{ Format(), TextureCompression::ASTC_RGBA_8x5 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_8x5 },
			{ Format(), TextureCompression::ASTC_RGBA_8x6 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_8x6 },
			{ Format(), TextureCompression::ASTC_RGBA_8x8 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_8x8 },
			{ Format(), TextureCompression::ASTC_RGBA_10x5 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_10x5 },
			{ Format(), TextureCompression::ASTC_RGBA_10x6 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_10x6 },
			{ Format(), TextureCompression::ASTC_RGBA_10x8 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_10x8 },
			{ Format(), TextureCompression::ASTC_RGBA_10x10 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_10x10 },
			{ Format(), TextureCompression::ASTC_RGBA_12x10 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_12x10 },
			{ Format(), TextureCompression::ASTC_RGBA_12x12 },
			{ Format(), TextureCompression::ASTC_SRGB_ALPHA_12x12 },
		};

		const int maxTableIndex = int((sizeof(table) / sizeof(table[0])) - 1);

		int index = int(vk_format);
		if (index < 0 || index > maxTableIndex)
		{
			// select undefined format
			index = 0;
		}

		format = table[index].format;
		return table[index].compression;
	}

} // namespace mango::vulkan

#else

    void dummy_vulkan_function_to_satisfy_linker()
    {
        // NOTE: this translation unit shouldn't be compiled on platforms w/o vulkan support
    }

#endif // MANGO_ENABLE_VULKAN

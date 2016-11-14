/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

// ---------------------------------------------------------------------------------
// library
// ---------------------------------------------------------------------------------

#ifdef VULKAN_LIBRARY_FUNC

VULKAN_LIBRARY_FUNC(vkCreateInstance);
VULKAN_LIBRARY_FUNC(vkEnumerateInstanceExtensionProperties);
VULKAN_LIBRARY_FUNC(vkEnumerateInstanceLayerProperties);

#undef VULKAN_LIBRARY_FUNC

#endif

// ---------------------------------------------------------------------------------
// instance
// ---------------------------------------------------------------------------------

#ifdef VULKAN_INSTANCE_FUNC

VULKAN_INSTANCE_FUNC(vkDestroyInstance);
VULKAN_INSTANCE_FUNC(vkEnumeratePhysicalDevices);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceFeatures);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceFormatProperties);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceImageFormatProperties);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceProperties);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceMemoryProperties);
VULKAN_INSTANCE_FUNC(vkCreateDevice);
VULKAN_INSTANCE_FUNC(vkEnumerateDeviceExtensionProperties);
VULKAN_INSTANCE_FUNC(vkEnumerateDeviceLayerProperties);

#ifdef VK_KHR_surface
VULKAN_INSTANCE_FUNC(vkDestroySurfaceKHR);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
#endif

#ifdef VK_KHR_display
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceDisplayPropertiesKHR);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceDisplayPlanePropertiesKHR);
VULKAN_INSTANCE_FUNC(vkGetDisplayPlaneSupportedDisplaysKHR);
VULKAN_INSTANCE_FUNC(vkGetDisplayModePropertiesKHR);
VULKAN_INSTANCE_FUNC(vkCreateDisplayModeKHR);
VULKAN_INSTANCE_FUNC(vkGetDisplayPlaneCapabilitiesKHR);
VULKAN_INSTANCE_FUNC(vkCreateDisplayPlaneSurfaceKHR);
#endif

#ifdef VK_KHR_xlib_surface
VULKAN_INSTANCE_FUNC(vkCreateXlibSurfaceKHR);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceXlibPresentationSupportKHR);
#endif

#ifdef VK_KHR_xcb_surface
VULKAN_INSTANCE_FUNC(vkCreateXcbSurfaceKHR);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceXcbPresentationSupportKHR);
#endif

#ifdef VK_KHR_wayland_surface
VULKAN_INSTANCE_FUNC(vkCreateWaylandSurfaceKHR);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceWaylandPresentationSupportKHR);
#endif

#ifdef VK_KHR_mir_surface
VULKAN_INSTANCE_FUNC(vkCreateMirSurfaceKHR);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceMirPresentationSupportKHR);
#endif

#ifdef VK_KHR_android_surface
VULKAN_INSTANCE_FUNC(vkCreateAndroidSurfaceKHR);
#endif

#ifdef VK_KHR_win32_surface
VULKAN_INSTANCE_FUNC(vkCreateWin32SurfaceKHR);
VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceWin32PresentationSupportKHR);
#endif

#ifdef VK_EXT_debug_report
VULKAN_INSTANCE_FUNC(vkCreateDebugReportCallbackEXT);
VULKAN_INSTANCE_FUNC(vkDestroyDebugReportCallbackEXT);
VULKAN_INSTANCE_FUNC(vkDebugReportMessageEXT);
#endif

#undef VULKAN_INSTANCE_FUNC

#endif

// ---------------------------------------------------------------------------------
// device
// ---------------------------------------------------------------------------------

#ifdef VULKAN_DEVICE_FUNC

VULKAN_DEVICE_FUNC(vkDestroyDevice);
VULKAN_DEVICE_FUNC(vkGetDeviceQueue);
VULKAN_DEVICE_FUNC(vkQueueSubmit);
VULKAN_DEVICE_FUNC(vkQueueWaitIdle);
VULKAN_DEVICE_FUNC(vkDeviceWaitIdle);
VULKAN_DEVICE_FUNC(vkAllocateMemory);
VULKAN_DEVICE_FUNC(vkFreeMemory);
VULKAN_DEVICE_FUNC(vkMapMemory);
VULKAN_DEVICE_FUNC(vkUnmapMemory);
VULKAN_DEVICE_FUNC(vkFlushMappedMemoryRanges);
VULKAN_DEVICE_FUNC(vkInvalidateMappedMemoryRanges);
VULKAN_DEVICE_FUNC(vkGetDeviceMemoryCommitment);
VULKAN_DEVICE_FUNC(vkBindBufferMemory);
VULKAN_DEVICE_FUNC(vkBindImageMemory);
VULKAN_DEVICE_FUNC(vkGetBufferMemoryRequirements);
VULKAN_DEVICE_FUNC(vkGetImageMemoryRequirements);
VULKAN_DEVICE_FUNC(vkGetImageSparseMemoryRequirements);
VULKAN_DEVICE_FUNC(vkGetPhysicalDeviceSparseImageFormatProperties);
VULKAN_DEVICE_FUNC(vkQueueBindSparse);
VULKAN_DEVICE_FUNC(vkCreateFence);
VULKAN_DEVICE_FUNC(vkDestroyFence);
VULKAN_DEVICE_FUNC(vkResetFences);
VULKAN_DEVICE_FUNC(vkGetFenceStatus);
VULKAN_DEVICE_FUNC(vkWaitForFences);
VULKAN_DEVICE_FUNC(vkCreateSemaphore);
VULKAN_DEVICE_FUNC(vkDestroySemaphore);
VULKAN_DEVICE_FUNC(vkCreateEvent);
VULKAN_DEVICE_FUNC(vkDestroyEvent);
VULKAN_DEVICE_FUNC(vkGetEventStatus);
VULKAN_DEVICE_FUNC(vkSetEvent);
VULKAN_DEVICE_FUNC(vkResetEvent);
VULKAN_DEVICE_FUNC(vkCreateQueryPool);
VULKAN_DEVICE_FUNC(vkDestroyQueryPool);
VULKAN_DEVICE_FUNC(vkGetQueryPoolResults);
VULKAN_DEVICE_FUNC(vkCreateBuffer);
VULKAN_DEVICE_FUNC(vkDestroyBuffer);
VULKAN_DEVICE_FUNC(vkCreateBufferView);
VULKAN_DEVICE_FUNC(vkDestroyBufferView);
VULKAN_DEVICE_FUNC(vkCreateImage);
VULKAN_DEVICE_FUNC(vkDestroyImage);
VULKAN_DEVICE_FUNC(vkGetImageSubresourceLayout);
VULKAN_DEVICE_FUNC(vkCreateImageView);
VULKAN_DEVICE_FUNC(vkDestroyImageView);
VULKAN_DEVICE_FUNC(vkCreateShaderModule);
VULKAN_DEVICE_FUNC(vkDestroyShaderModule);
VULKAN_DEVICE_FUNC(vkCreatePipelineCache);
VULKAN_DEVICE_FUNC(vkDestroyPipelineCache);
VULKAN_DEVICE_FUNC(vkGetPipelineCacheData);
VULKAN_DEVICE_FUNC(vkMergePipelineCaches);
VULKAN_DEVICE_FUNC(vkCreateGraphicsPipelines);
VULKAN_DEVICE_FUNC(vkCreateComputePipelines);
VULKAN_DEVICE_FUNC(vkDestroyPipeline);
VULKAN_DEVICE_FUNC(vkCreatePipelineLayout);
VULKAN_DEVICE_FUNC(vkDestroyPipelineLayout);
VULKAN_DEVICE_FUNC(vkCreateSampler);
VULKAN_DEVICE_FUNC(vkDestroySampler);
VULKAN_DEVICE_FUNC(vkCreateDescriptorSetLayout);
VULKAN_DEVICE_FUNC(vkDestroyDescriptorSetLayout);
VULKAN_DEVICE_FUNC(vkCreateDescriptorPool);
VULKAN_DEVICE_FUNC(vkDestroyDescriptorPool);
VULKAN_DEVICE_FUNC(vkResetDescriptorPool);
VULKAN_DEVICE_FUNC(vkAllocateDescriptorSets);
VULKAN_DEVICE_FUNC(vkFreeDescriptorSets);
VULKAN_DEVICE_FUNC(vkUpdateDescriptorSets);
VULKAN_DEVICE_FUNC(vkCreateFramebuffer);
VULKAN_DEVICE_FUNC(vkDestroyFramebuffer);
VULKAN_DEVICE_FUNC(vkCreateRenderPass);
VULKAN_DEVICE_FUNC(vkDestroyRenderPass);
VULKAN_DEVICE_FUNC(vkGetRenderAreaGranularity);
VULKAN_DEVICE_FUNC(vkCreateCommandPool);
VULKAN_DEVICE_FUNC(vkDestroyCommandPool);
VULKAN_DEVICE_FUNC(vkResetCommandPool);
VULKAN_DEVICE_FUNC(vkAllocateCommandBuffers);
VULKAN_DEVICE_FUNC(vkFreeCommandBuffers);
VULKAN_DEVICE_FUNC(vkBeginCommandBuffer);
VULKAN_DEVICE_FUNC(vkEndCommandBuffer);
VULKAN_DEVICE_FUNC(vkResetCommandBuffer);
VULKAN_DEVICE_FUNC(vkCmdBindPipeline);
VULKAN_DEVICE_FUNC(vkCmdSetViewport);
VULKAN_DEVICE_FUNC(vkCmdSetScissor);
VULKAN_DEVICE_FUNC(vkCmdSetLineWidth);
VULKAN_DEVICE_FUNC(vkCmdSetDepthBias);
VULKAN_DEVICE_FUNC(vkCmdSetBlendConstants);
VULKAN_DEVICE_FUNC(vkCmdSetDepthBounds);
VULKAN_DEVICE_FUNC(vkCmdSetStencilCompareMask);
VULKAN_DEVICE_FUNC(vkCmdSetStencilWriteMask);
VULKAN_DEVICE_FUNC(vkCmdSetStencilReference);
VULKAN_DEVICE_FUNC(vkCmdBindDescriptorSets);
VULKAN_DEVICE_FUNC(vkCmdBindIndexBuffer);
VULKAN_DEVICE_FUNC(vkCmdBindVertexBuffers);
VULKAN_DEVICE_FUNC(vkCmdDraw);
VULKAN_DEVICE_FUNC(vkCmdDrawIndexed);
VULKAN_DEVICE_FUNC(vkCmdDrawIndirect);
VULKAN_DEVICE_FUNC(vkCmdDrawIndexedIndirect);
VULKAN_DEVICE_FUNC(vkCmdDispatch);
VULKAN_DEVICE_FUNC(vkCmdDispatchIndirect);
VULKAN_DEVICE_FUNC(vkCmdCopyBuffer);
VULKAN_DEVICE_FUNC(vkCmdCopyImage);
VULKAN_DEVICE_FUNC(vkCmdBlitImage);
VULKAN_DEVICE_FUNC(vkCmdCopyBufferToImage);
VULKAN_DEVICE_FUNC(vkCmdCopyImageToBuffer);
VULKAN_DEVICE_FUNC(vkCmdUpdateBuffer);
VULKAN_DEVICE_FUNC(vkCmdFillBuffer);
VULKAN_DEVICE_FUNC(vkCmdClearColorImage);
VULKAN_DEVICE_FUNC(vkCmdClearDepthStencilImage);
VULKAN_DEVICE_FUNC(vkCmdClearAttachments);
VULKAN_DEVICE_FUNC(vkCmdResolveImage);
VULKAN_DEVICE_FUNC(vkCmdSetEvent);
VULKAN_DEVICE_FUNC(vkCmdResetEvent);
VULKAN_DEVICE_FUNC(vkCmdWaitEvents);
VULKAN_DEVICE_FUNC(vkCmdPipelineBarrier);
VULKAN_DEVICE_FUNC(vkCmdBeginQuery);
VULKAN_DEVICE_FUNC(vkCmdEndQuery);
VULKAN_DEVICE_FUNC(vkCmdResetQueryPool);
VULKAN_DEVICE_FUNC(vkCmdWriteTimestamp);
VULKAN_DEVICE_FUNC(vkCmdCopyQueryPoolResults);
VULKAN_DEVICE_FUNC(vkCmdPushConstants);
VULKAN_DEVICE_FUNC(vkCmdBeginRenderPass);
VULKAN_DEVICE_FUNC(vkCmdNextSubpass);
VULKAN_DEVICE_FUNC(vkCmdEndRenderPass);
VULKAN_DEVICE_FUNC(vkCmdExecuteCommands);

#ifdef VK_KHR_swapchain
VULKAN_DEVICE_FUNC(vkCreateSwapchainKHR);
VULKAN_DEVICE_FUNC(vkDestroySwapchainKHR);
VULKAN_DEVICE_FUNC(vkGetSwapchainImagesKHR);
VULKAN_DEVICE_FUNC(vkAcquireNextImageKHR);
VULKAN_DEVICE_FUNC(vkQueuePresentKHR);
#endif

#ifdef VK_KHR_display_swapchain
VULKAN_DEVICE_FUNC(vkCreateSharedSwapchainsKHR);
#endif

#undef VULKAN_DEVICE_FUNC

#endif

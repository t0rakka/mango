/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace mango::vulkan
{

    enum class InputColorSpace
    {
        BT709_LINEAR,
        BT709_NONLINEAR,
    };

    enum class TonemapMode
    {
        None,
        Reinhard,
        Aces,
        Agx,
    };

    struct OutputTransformOptions
    {
        InputColorSpace input = InputColorSpace::BT709_LINEAR;
        float sdrWhiteNits = 203.0f;
        float peakNits = 10000.0f;
        TonemapMode tonemap = TonemapMode::None;
        float exposure = 1.0f;
    };

    bool isSRGB(VkFormat format);
    bool isLinear(VkColorSpaceKHR colorSpace);
    bool isHDR(VkSurfaceFormatKHR surfaceFormat);
    bool isSDR(VkSurfaceFormatKHR surfaceFormat);

    OutputTransformOptions defaultOutputOptions(VkSurfaceFormatKHR surfaceFormat,
                                                float exposure = 1.0f);

    // Display-referred sRGB input (vertex colors, 8-bit textures): no tonemap or grading.
    OutputTransformOptions defaultDisplayOutputOptions(VkSurfaceFormatKHR surfaceFormat);

    // Returns GLSL helpers plus encodeOutput(vec4) for the given swapchain surface format.
    // Constants such as exposure and nits are baked into the generated source.
    std::string getOutputTransformGLSL(VkSurfaceFormatKHR surfaceFormat);
    std::string getOutputTransformGLSL(VkSurfaceFormatKHR surfaceFormat, const OutputTransformOptions& options);

    std::string getDisplayOutputTransformGLSL(VkSurfaceFormatKHR surfaceFormat);

} // namespace mango::vulkan

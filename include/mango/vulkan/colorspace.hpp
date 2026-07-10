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
    };

    // Resolve-stage output transform selected from the swapchain surface format.
    enum class OutputTransform : int
    {
        Pass = 0,                   // sRGB surface: texture decode is sufficient
        SrgbEncode = 1,             // float/UNORM SDR: linear BT.709 -> sRGB gamma
        SdrToHdrPQ = 2,             // HDR10 ST2084: BT.709 -> BT.2020 -> PQ
        SdrToHdrHLG = 3,            // HDR10 HLG: BT.709 -> BT.2020 -> BT.2408 gain -> HLG OETF
        SdrToAdobeRgb = 4,          // Adobe RGB nonlinear: BT.709 -> Adobe RGB -> Adobe OETF
        SdrToBt709Nonlinear = 5,    // BT.709 nonlinear: SMPTE 170M / ITU OETF
        SdrToDciP3Nonlinear = 6,    // DCI-P3 nonlinear: BT.709 -> P3 -> gamma 2.6
        SdrToExtendedSrgbLinear = 7,// EXTENDED_SRGB_LINEAR + sRGB RT: pre-compensate encode
        SdrToLinearSurface = 8,     // linear color space + UNORM/float: linear passthrough
        SdrToDisplayP3Linear = 9,   // Display P3 linear float: gamut map + linear out
        SdrToBt2020Linear = 10,     // BT.2020 linear float: gamut map + linear out
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
    bool isLinearSdrColorSpace(VkColorSpaceKHR colorSpace);
    bool isHDR(VkSurfaceFormatKHR surfaceFormat);
    bool isSDR(VkSurfaceFormatKHR surfaceFormat);

    OutputTransform selectOutputTransform(VkSurfaceFormatKHR surfaceFormat);

    // SDR diffuse white in nits. PQ: linear * nits / peakNits. HLG: BT.2408 gain * (nits / 203).
    float defaultSdrWhiteNits(VkColorSpaceKHR colorSpace);

    OutputTransformOptions defaultOutputOptions(VkSurfaceFormatKHR surfaceFormat,
                                                float exposure = 1.0f);

    // Returns GLSL helpers plus encodeOutput(vec4) for the given swapchain surface format.
    // Constants such as exposure and nits are baked into the generated source.
    std::string getOutputTransformGLSL(VkSurfaceFormatKHR surfaceFormat);
    std::string getOutputTransformGLSL(VkSurfaceFormatKHR surfaceFormat,
                                       const OutputTransformOptions& options);

} // namespace mango::vulkan

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/vulkan/colorspace.hpp>

#include <fmt/format.h>
#include <string>

namespace mango::vulkan
{

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

    namespace
    {

        const char* g_linear_to_srgb = R"(
vec3 linearToSrgb(vec3 linear)
{
    vec3 lo = linear * 12.92;
    vec3 hi = pow(max(linear, vec3(0.0031308)), vec3(1.0 / 2.4)) * 1.055 - 0.055;
    return mix(lo, hi, step(vec3(0.0031308), linear));
}
)";

        const char* g_srgb_to_linear = R"(
vec3 srgbToLinear(vec3 encoded)
{
    vec3 lo = encoded / 12.92;
    vec3 hi = pow(max((encoded + 0.055) / 1.055, vec3(0.0)), vec3(2.4));
    return mix(lo, hi, step(vec3(0.04045), encoded));
}
)";

        const char* g_linear_to_pq = R"(
vec3 linearToPQ(vec3 linear)
{
    const float m1 = 0.1593017578125;
    const float m2 = 78.84375;
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;

    vec3 Lm = pow(max(linear, vec3(0.0)), vec3(m1));
    vec3 N = (c1 + c2 * Lm) / (1.0 + c3 * Lm);
    return pow(N, vec3(m2));
}
)";

        const char* g_linear_to_hlg = R"(
vec3 linearToHLG(vec3 linear)
{
    const float a = 0.17883277;
    const float b = 0.28466892;
    const float c = 0.55991073;
    vec3 x = max(linear, vec3(0.0));
    vec3 lo = sqrt(3.0 * x);
    vec3 hi = a * log(max(12.0 * x - b, 1e-6)) + c;
    return mix(lo, hi, step(vec3(1.0 / 12.0), x));
}
)";

        const char* g_linear_to_smpte170m = R"(
vec3 linearToSmpte170m(vec3 linear)
{
    const float alpha = 1.099;
    const float beta = 0.018;
    vec3 x = max(linear, vec3(0.0));
    vec3 lo = x * 4.5;
    vec3 hi = alpha * pow(x, vec3(0.45)) - (alpha - 1.0);
    return mix(lo, hi, step(vec3(beta), x));
}
)";

        const char* g_linear_to_adobe_rgb = R"(
vec3 linearToAdobeRgb(vec3 linear)
{
    return pow(max(linear, vec3(0.0)), vec3(256.0 / 563.0));
}
)";

        const char* g_linear_to_gamma26 = R"(
vec3 linearToGamma26(vec3 linear)
{
    return pow(max(linear, vec3(0.0)), vec3(1.0 / 2.6));
}
)";

        const char* g_bt709_to_display_p3 = R"(
vec3 bt709ToDisplayP3(vec3 linear)
{
    const mat3 m = mat3(
        vec3(0.8224619287, 0.0335730637, 0.0170815977),
        vec3(0.1775380529, 0.9666420745, 0.0723970842),
        vec3(0.0000000000, 0.0000000000, 0.9105199182)
    );
    return m * linear;
}
)";

        const char* g_bt709_to_bt2020 = R"(
vec3 bt709ToBt2020(vec3 linear)
{
    const mat3 m = mat3(
        vec3(0.6274040, 0.0690970, 0.0163916),
        vec3(0.3292820, 0.9195400, 0.0880132),
        vec3(0.0433136, 0.0113612, 0.8955950)
    );
    return m * linear;
}
)";

        const char* g_bt709_to_adobe_rgb = R"(
vec3 bt709ToAdobeRgb(vec3 linear)
{
    const mat3 m = mat3(
        vec3(0.7151626, 0.0000000, 0.0000000),
        vec3(0.2848372, 1.0000000, 0.0411705),
        vec3(0.0000000, 0.0000000, 0.9588295)
    );
    return m * linear;
}
)";

        std::string formatGlslFloat(float value)
        {
            return fmt::format("{:.5f}", value);
        }

        struct OutputRecipe
        {
            std::string helpers;
            std::string body;
        };

        void appendHelper(std::string& helpers, const char* snippet)
        {
            if (helpers.find(snippet) != std::string::npos)
            {
                return;
            }

            helpers += snippet;
        }

        OutputRecipe buildRecipe(OutputTransform transform, const OutputTransformOptions& options)
        {
            OutputRecipe recipe;

            switch (transform)
            {
                case OutputTransform::Pass:
                    recipe.body = "// sRGB surface: texture decode is sufficient.";
                    break;

                case OutputTransform::SrgbEncode:
                    appendHelper(recipe.helpers, g_linear_to_srgb);
                    recipe.body = "color.rgb = linearToSrgb(color.rgb);";
                    break;

                case OutputTransform::SdrToHdrPQ:
                {
                    appendHelper(recipe.helpers, g_bt709_to_bt2020);
                    appendHelper(recipe.helpers, g_linear_to_pq);
                    const std::string pqScale = formatGlslFloat(options.sdrWhiteNits / options.peakNits);
                    recipe.body =
                        "vec3 linear = bt709ToBt2020(color.rgb);\n"
                        "    color.rgb = linearToPQ(linear * " + pqScale + ");";
                    break;
                }

                case OutputTransform::SdrToHdrHLG:
                {
                    appendHelper(recipe.helpers, g_bt709_to_bt2020);
                    appendHelper(recipe.helpers, g_linear_to_hlg);
                    constexpr float kBt2408HlgGain = 0.265f;
                    constexpr float kBt2408HlgReferenceNits = 203.0f;
                    const std::string hlgScale = formatGlslFloat(
                        kBt2408HlgGain * (options.sdrWhiteNits / kBt2408HlgReferenceNits));
                    recipe.body =
                        "vec3 linear = bt709ToBt2020(color.rgb);\n"
                        "    color.rgb = linearToHLG(linear * " + hlgScale + ");";
                    break;
                }

                case OutputTransform::SdrToAdobeRgb:
                    appendHelper(recipe.helpers, g_bt709_to_adobe_rgb);
                    appendHelper(recipe.helpers, g_linear_to_adobe_rgb);
                    recipe.body = "color.rgb = linearToAdobeRgb(bt709ToAdobeRgb(color.rgb));";
                    break;

                case OutputTransform::SdrToBt709Nonlinear:
                    appendHelper(recipe.helpers, g_linear_to_smpte170m);
                    recipe.body = "color.rgb = linearToSmpte170m(color.rgb);";
                    break;

                case OutputTransform::SdrToDciP3Nonlinear:
                    appendHelper(recipe.helpers, g_bt709_to_display_p3);
                    appendHelper(recipe.helpers, g_linear_to_gamma26);
                    recipe.body = "color.rgb = linearToGamma26(bt709ToDisplayP3(color.rgb));";
                    break;

                case OutputTransform::SdrToExtendedSrgbLinear:
                    appendHelper(recipe.helpers, g_srgb_to_linear);
                    recipe.body = "color.rgb = srgbToLinear(color.rgb);";
                    break;

                case OutputTransform::SdrToLinearSurface:
                    recipe.body = "// Linear surface: scene-linear passthrough.";
                    break;

                case OutputTransform::SdrToDisplayP3Linear:
                    appendHelper(recipe.helpers, g_bt709_to_display_p3);
                    recipe.body = "color.rgb = bt709ToDisplayP3(color.rgb);";
                    break;

                case OutputTransform::SdrToBt2020Linear:
                    appendHelper(recipe.helpers, g_bt709_to_bt2020);
                    recipe.body = "color.rgb = bt709ToBt2020(color.rgb);";
                    break;
            }

            if (options.input == InputColorSpace::BT709_NONLINEAR)
            {
                switch (transform)
                {
                    case OutputTransform::Pass:
                        recipe.helpers.clear();
                        appendHelper(recipe.helpers, g_srgb_to_linear);
                        recipe.body = "color.rgb = srgbToLinear(color.rgb);";
                        break;

                    case OutputTransform::SrgbEncode:
                        recipe.helpers.clear();
                        recipe.body = "// display-referred sRGB passthrough.";
                        break;

                    default:
                        break;
                }
            }

            return recipe;
        }

        std::string buildTonemapGlsl(TonemapMode mode, float exposure)
        {
            if (mode == TonemapMode::None)
            {
                return {};
            }

            const std::string exposureLiteral = formatGlslFloat(exposure);

            if (mode == TonemapMode::Aces)
            {
                return std::string(R"(
vec3 tonemapAces(vec3 color)
{
    const float kExposure = )") + exposureLiteral + R"(;
    color = max(color, vec3(0.0)) * kExposure;

    const mat3 acesInput = mat3(
        0.59719, 0.07600, 0.02840,
        0.35458, 0.90834, 0.13383,
        0.04823, 0.01566, 0.83777
    );
    const mat3 acesOutput = mat3(
        1.60475, -0.10208, -0.00327,
       -0.53108,  1.10813, -0.07276,
       -0.07367, -0.00605,  1.07602
    );

    vec3 v = acesInput * color;
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return acesOutput * (a / b);
}
)";
            }

            if (mode == TonemapMode::Agx)
            {
                return std::string(R"(
vec3 agxDefaultContrastApprox(vec3 x)
{
    vec3 x2 = x * x;
    vec3 x4 = x2 * x2;
    return + 15.5     * x4 * x2
           - 40.14    * x4 * x
           + 31.96    * x4
           - 6.868    * x2 * x
           + 0.4298   * x2
           + 0.1191   * x
           - 0.00232;
}

vec3 tonemapAgx(vec3 color)
{
    const float kExposure = )") + exposureLiteral + R"(;
    const float kMinEv = -12.47393;
    const float kMaxEv = 4.026069;

    color = max(color, vec3(0.0)) * kExposure;

    const mat3 linearSrgbToLinearRec2020 = mat3(
        0.6274, 0.0691, 0.0164,
        0.3293, 0.9195, 0.0880,
        0.0433, 0.0113, 0.8956
    );
    const mat3 agxInset = mat3(
        0.856627153315983, 0.137318972929847, 0.11189821299995,
        0.0951212405381588, 0.761241990602591, 0.0767994186031903,
        0.0482516061458583, 0.101439036467562, 0.811302368396859
    );
    const mat3 agxOutset = mat3(
        1.1271005818144368, -0.1413297634984383, -0.14132976349843826,
       -0.11060664309660323, 1.157823702216272, -0.11060664309660294,
       -0.016493938717834573, -0.016493938717834257, 1.2519364065950405
    );
    const mat3 linearRec2020ToLinearSrgb = mat3(
        1.6605, -0.1246, -0.0182,
       -0.5876, 1.1329, -0.1006,
       -0.0728, -0.0083, 1.1187
    );

    color = linearSrgbToLinearRec2020 * color;
    color = agxInset * max(color, vec3(1e-10));
    color = clamp(log2(color), kMinEv, kMaxEv);
    color = (color - kMinEv) / (kMaxEv - kMinEv);
    color = clamp(agxDefaultContrastApprox(color), 0.0, 1.0);
    color = agxOutset * color;
    color = linearRec2020ToLinearSrgb * max(color, vec3(0.0));
    return clamp(color, 0.0, 1.0);
}
)";
            }

            return std::string(R"(
vec3 tonemapReinhard(vec3 color)
{
    const float kExposure = )") + exposureLiteral + R"(;
    color = max(color, vec3(0.0)) * kExposure;
    return color / (1.0 + color);
}
)";
        }

        const char* tonemapFunctionName(TonemapMode mode)
        {
            switch (mode)
            {
                case TonemapMode::Reinhard:
                    return "tonemapReinhard";
                case TonemapMode::Aces:
                    return "tonemapAces";
                case TonemapMode::Agx:
                    return "tonemapAgx";
                default:
                    return nullptr;
            }
        }

    } // namespace

    bool isSRGB(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_R8_SRGB:
            case VK_FORMAT_R8G8_SRGB:
            case VK_FORMAT_R8G8B8_SRGB:
            case VK_FORMAT_B8G8R8_SRGB:
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_SRGB:
            case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
            case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            case VK_FORMAT_BC2_SRGB_BLOCK:
            case VK_FORMAT_BC3_SRGB_BLOCK:
            case VK_FORMAT_BC7_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
            case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
            case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
            case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
            case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
            case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
            case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
            case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
            case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
            case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
            /*
            case VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT:
            case VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT:
            case VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT:
            case VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT:
            case VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT:
            case VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT:
            case VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT:
            case VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT:
            case VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT:
            case VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT:
            */
                return true;
            default:
                return false;
        }
    }

    bool isLinear(VkColorSpaceKHR colorSpace)
    {
        switch (colorSpace)
        {
            case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
            case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:
            case VK_COLOR_SPACE_BT709_LINEAR_EXT:
            case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
            case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
                return true;
            default:
                return false;
        }
    }

    bool isHDR(VkSurfaceFormatKHR surfaceFormat)
    {
        switch (surfaceFormat.colorSpace)
        {
            case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
            case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
            case VK_COLOR_SPACE_HDR10_ST2084_EXT:
            case VK_COLOR_SPACE_HDR10_HLG_EXT:
                return true;
            default:
                return false;
        }
    }

    bool isSDR(VkSurfaceFormatKHR surfaceFormat)
    {
        if (isHDR(surfaceFormat))
        {
            return false;
        }

        switch (surfaceFormat.colorSpace)
        {
            case VK_COLOR_SPACE_BT709_LINEAR_EXT:
            case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
            case VK_COLOR_SPACE_PASS_THROUGH_EXT:
            case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD:
                return false;
            case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
                return isSRGB(surfaceFormat.format);
            default:
                return true;
        }
    }

    static
    OutputTransform selectOutputTransform(VkSurfaceFormatKHR surfaceFormat)
    {
        if (surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
        {
            return OutputTransform::SdrToHdrPQ;
        }

        if (surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_HLG_EXT)
        {
            return OutputTransform::SdrToHdrHLG;
        }

        if (surfaceFormat.colorSpace == VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT)
        {
            return OutputTransform::SdrToAdobeRgb;
        }

        if (surfaceFormat.colorSpace == VK_COLOR_SPACE_BT709_NONLINEAR_EXT)
        {
            return OutputTransform::SdrToBt709Nonlinear;
        }

        if (surfaceFormat.colorSpace == VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT ||
            surfaceFormat.colorSpace == VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT)
        {
            return OutputTransform::SdrToDciP3Nonlinear;
        }

        if (isLinear(surfaceFormat.colorSpace))
        {
            if (surfaceFormat.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT &&
                isSRGB(surfaceFormat.format))
            {
                return OutputTransform::SdrToExtendedSrgbLinear;
            }

            if (surfaceFormat.colorSpace == VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT)
            {
                return OutputTransform::SdrToDisplayP3Linear;
            }

            if (surfaceFormat.colorSpace == VK_COLOR_SPACE_BT2020_LINEAR_EXT)
            {
                return OutputTransform::SdrToBt2020Linear;
            }

            return OutputTransform::SdrToLinearSurface;
        }

        if (!isSRGB(surfaceFormat.format))
        {
            return OutputTransform::SrgbEncode;
        }

        return OutputTransform::Pass;
    }

    static
    float defaultSdrWhiteNits(VkColorSpaceKHR colorSpace)
    {
        switch (colorSpace)
        {
            case VK_COLOR_SPACE_HDR10_ST2084_EXT:
            case VK_COLOR_SPACE_HDR10_HLG_EXT:
                return 203.0f;
            default:
                return 100.0f;
        }
    }

    OutputTransformOptions defaultOutputOptions(VkSurfaceFormatKHR surfaceFormat, float exposure)
    {
        OutputTransformOptions options;
        options.exposure = exposure;
        options.sdrWhiteNits = defaultSdrWhiteNits(surfaceFormat.colorSpace);

        if (isHDR(surfaceFormat))
        {
            options.tonemap = TonemapMode::None;
        }
        else
        {
            options.tonemap = TonemapMode::Reinhard;
        }

        return options;
    }

    OutputTransformOptions defaultDisplayOutputOptions(VkSurfaceFormatKHR surfaceFormat)
    {
        OutputTransformOptions options;
        options.input = InputColorSpace::BT709_NONLINEAR;
        options.tonemap = TonemapMode::None;
        options.sdrWhiteNits = defaultSdrWhiteNits(surfaceFormat.colorSpace);
        return options;
    }

    std::string getDisplayOutputTransformGLSL(VkSurfaceFormatKHR surfaceFormat)
    {
        return getOutputTransformGLSL(surfaceFormat, defaultDisplayOutputOptions(surfaceFormat));
    }

    std::string getOutputTransformGLSL(VkSurfaceFormatKHR surfaceFormat)
    {
        return getOutputTransformGLSL(surfaceFormat, defaultOutputOptions(surfaceFormat));
    }

    std::string getOutputTransformGLSL(VkSurfaceFormatKHR surfaceFormat,
                                       const OutputTransformOptions& options)
    {
        OutputTransform transform = selectOutputTransform(surfaceFormat);
        OutputRecipe recipe = buildRecipe(transform, options);

        std::string source = buildTonemapGlsl(options.tonemap, options.exposure);
        source += recipe.helpers;

        source += R"(
vec4 encodeOutput(vec4 color)
{
    color.rgb = max(color.rgb, vec3(0.0));
)";

        if (options.tonemap != TonemapMode::None)
        {
            if (const char* tonemapFn = tonemapFunctionName(options.tonemap))
            {
                source += "    color.rgb = ";
                source += tonemapFn;
                source += "(color.rgb);\n";
            }
        }

        source += "    ";
        source += recipe.body;
        source += R"(
    return color;
}
)";

        return source;
    }

} // namespace mango::vulkan

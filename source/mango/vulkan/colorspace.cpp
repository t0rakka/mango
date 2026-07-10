/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/vulkan/colorspace.hpp>

#include <fmt/format.h>
#include <string>

namespace mango::vulkan
{

    namespace
    {

        std::string formatGlslFloat(float value)
        {
            return fmt::format("{:.5f}", value);
        }

        std::string formatGlslRatio(float numerator, float denominator)
        {
            return formatGlslFloat(numerator / denominator);
        }

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
                    const std::string pqScale = formatGlslRatio(options.sdrWhiteNits, options.peakNits);
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

            return recipe;
        }

        std::string buildTonemapGlsl(TonemapMode mode, float exposure)
        {
            if (mode == TonemapMode::None)
            {
                return {};
            }

            const std::string exposureLiteral = formatGlslFloat(exposure);

            return std::string(R"(
vec3 tonemapSdr(vec3 color)
{
    const float kExposure = )") + exposureLiteral + R"(;
    color = max(color, vec3(0.0)) * kExposure;
    return color / (1.0 + color);
}
)";
        }

    } // namespace

    bool isSRGB(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_SRGB:
            case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
                return true;
            default:
                return false;
        }
    }

    bool isLinearSdrColorSpace(VkColorSpaceKHR colorSpace)
    {
        switch (colorSpace)
        {
            case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
            case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:
            case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
                return true;
            default:
                return false;
        }
    }

    bool isHDR(VkSurfaceFormatKHR surfaceFormat)
    {
        switch (surfaceFormat.colorSpace)
        {
            case VK_COLOR_SPACE_HDR10_ST2084_EXT:
            case VK_COLOR_SPACE_HDR10_HLG_EXT:
            case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
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

        if (isLinearSdrColorSpace(surfaceFormat.colorSpace))
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

    std::string getOutputTransformGLSL(VkSurfaceFormatKHR surfaceFormat)
    {
        return getOutputTransformGLSL(surfaceFormat, defaultOutputOptions(surfaceFormat));
    }

    std::string getOutputTransformGLSL(VkSurfaceFormatKHR surfaceFormat,
                                       const OutputTransformOptions& options)
    {
        OutputTransform transform = selectOutputTransform(surfaceFormat);
        OutputRecipe recipe = buildRecipe(transform, options);

        if (options.input == InputColorSpace::BT709_NONLINEAR &&
            transform != OutputTransform::SdrToExtendedSrgbLinear)
        {
            recipe.helpers.clear();
            recipe.body = "// BT709_NONLINEAR input passthrough.";
        }

        std::string source = buildTonemapGlsl(options.tonemap, options.exposure);
        source += recipe.helpers;

        source += R"(
vec4 encodeOutput(vec4 color)
{
    color.rgb = max(color.rgb, vec3(0.0));
)";

        if (options.tonemap != TonemapMode::None)
        {
            source += "    color.rgb = tonemapSdr(color.rgb);\n";
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

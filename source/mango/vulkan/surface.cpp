/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/print.hpp>
#include <mango/vulkan/vulkan.hpp>

namespace mango::vulkan
{

    namespace
    {

        bool surfaceFormatEquals(const VkSurfaceFormatKHR& a, const VkSurfaceFormatKHR& b)
        {
            return a.format == b.format && a.colorSpace == b.colorSpace;
        }

        bool isSupportedSurfaceFormat(VkSurfaceFormatKHR surfaceFormat)
        {
            switch (surfaceFormat.colorSpace)
            {
                case VK_COLOR_SPACE_PASS_THROUGH_EXT:
                case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD:
                case VK_COLOR_SPACE_DOLBYVISION_EXT:
                case VK_COLOR_SPACE_BT709_LINEAR_EXT:
                case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
                    return false;
                default:
                    break;
            }

            return true;
        }

        // HDR tiers: PQ before HLG; fp16 before 10-bit UNORM within each transfer function.
        constexpr VkSurfaceFormatKHR g_hdrSurfaceFormats[] =
        {
            { VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_HDR10_ST2084_EXT },
            { VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_HDR10_ST2084_EXT },
            { VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_COLOR_SPACE_HDR10_ST2084_EXT },

            { VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_HDR10_HLG_EXT },
            { VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_HDR10_HLG_EXT },

            { VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_BT2020_LINEAR_EXT },
            { VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_BT2020_LINEAR_EXT },
        };

        // Default SDR: write display-referred values directly (VGA-style), compositor treats as sRGB.
        constexpr VkSurfaceFormatKHR g_sdrSurfaceFormats[] =
        {
            { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
            { VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
            { VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
        };

        constexpr VkSurfaceFormatKHR g_defaultSurfaceFormat =
        {
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        };

        const VkSurfaceFormatKHR* findAvailable(const std::vector<VkSurfaceFormatKHR>& available,
                                              const VkSurfaceFormatKHR& preferred)
        {
            if (!isSupportedSurfaceFormat(preferred))
            {
                return nullptr;
            }

            for (const VkSurfaceFormatKHR& format : available)
            {
                if (surfaceFormatEquals(format, preferred))
                {
                    return &format;
                }
            }

            return nullptr;
        }

        const VkSurfaceFormatKHR* selectFromList(const std::vector<VkSurfaceFormatKHR>& available,
                                               const VkSurfaceFormatKHR* list,
                                               size_t count)
        {
            for (size_t i = 0; i < count; ++i)
            {
                if (const VkSurfaceFormatKHR* match = findAvailable(available, list[i]))
                {
                    return match;
                }
            }

            return nullptr;
        }

        VkSurfaceFormatKHR fallbackSdrFormat(const std::vector<VkSurfaceFormatKHR>& available)
        {
            if (const VkSurfaceFormatKHR* match = findAvailable(available, g_defaultSurfaceFormat))
            {
                return *match;
            }

            for (const VkSurfaceFormatKHR& format : available)
            {
                if (!isHDR(format) && isSupportedSurfaceFormat(format))
                {
                    return format;
                }
            }

            return g_defaultSurfaceFormat;
        }

        SurfaceFormatSelection makeSelection(VkSurfaceFormatKHR format,
                                             SurfaceFormatIntent requested,
                                             bool matchedIntent)
        {
            SurfaceFormatSelection selection;
            selection.format = format;
            selection.requested = requested;
            selection.matchedIntent = matchedIntent;
            selection.isHdr = isHDR(format);
            return selection;
        }

    } // namespace

    std::vector<VkSurfaceFormatKHR> getRecommendedSurfaceFormats(SurfaceFormatIntent intent)
    {
        std::vector<VkSurfaceFormatKHR> formats;

        if (intent == SurfaceFormatIntent::HDR)
        {
            formats.insert(formats.end(),
                std::begin(g_hdrSurfaceFormats), std::end(g_hdrSurfaceFormats));
        }

        formats.insert(formats.end(),
            std::begin(g_sdrSurfaceFormats), std::end(g_sdrSurfaceFormats));

        return formats;
    }

    void applyRecommendedSurfaceFormats(VulkanDeviceConfig& config, SurfaceFormatIntent intent)
    {
        config.surfaceFormatIntent = intent;
        config.preferredFormats = getRecommendedSurfaceFormats(intent);
    }

    SurfaceFormatSelection selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available,
                                               SurfaceFormatIntent intent)
    {
        if (available.empty())
        {
            return makeSelection(g_defaultSurfaceFormat, intent, false);
        }

        if (intent == SurfaceFormatIntent::HDR)
        {
            if (const VkSurfaceFormatKHR* match = selectFromList(available,
                    g_hdrSurfaceFormats, std::size(g_hdrSurfaceFormats)))
            {
                return makeSelection(*match, intent, true);
            }
        }

        if (const VkSurfaceFormatKHR* match = selectFromList(available,
                g_sdrSurfaceFormats, std::size(g_sdrSurfaceFormats)))
        {
            const bool matchedIntent = intent == SurfaceFormatIntent::SDR;
            return makeSelection(*match, intent, matchedIntent);
        }

        if (intent == SurfaceFormatIntent::SDR)
        {
            return makeSelection(fallbackSdrFormat(available), intent, false);
        }

        return makeSelection(fallbackSdrFormat(available), intent, false);
    }

    SurfaceFormatSelection selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available,
                                               bool hdr)
    {
        return selectSurfaceFormat(available, hdr ? SurfaceFormatIntent::HDR
                                                  : SurfaceFormatIntent::SDR);
    }

    SurfaceFormatSelection selectSurfaceFormat(VkPhysicalDevice physicalDevice,
                                               VkSurfaceKHR surface,
                                               SurfaceFormatIntent intent)
    {
        return selectSurfaceFormat(getSurfaceFormats(physicalDevice, surface), intent);
    }

    void logSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                           const SurfaceFormatSelection* selected)
    {
        const std::vector<VkSurfaceFormatKHR> surfaceFormats = getSurfaceFormats(physicalDevice, surface);

        printLine(Print::Info, "PhysicalDeviceSurfaceFormats:");

        for (const VkSurfaceFormatKHR& format : surfaceFormats)
        {
            const bool is_selected = selected &&
                surfaceFormatEquals(format, selected->format);
            printLine(Print::Info, "  {} {} | {}",
                is_selected ? ">" : " ",
                getString(format.format),
                getString(format.colorSpace));
        }

        if (selected)
        {
            printLine(Print::Info, "SurfaceFormat {} | {} (HDR: {})",
                getString(selected->format.format),
                getString(selected->format.colorSpace),
                selected->isHdr ? "yes" : "no");
        }
    }

    void logSurfaceFormats(const VulkanWindow& window, SurfaceFormatIntent requested)
    {
        SurfaceFormatSelection selection;
        selection.format = window.surfaceFormat();
        selection.requested = requested;
        selection.isHdr = isHDR(selection.format);

        logSurfaceFormats(window.physicalDevice(), window.surface(), &selection);
    }

} // namespace mango::vulkan

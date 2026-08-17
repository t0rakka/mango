/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/mango.hpp>
#include <vector>

namespace mango::demo
{

    struct VulkanDemoArgs
    {
        bool info = false;
        bool validate = false;
    };

    inline void configureVulkanDemoParser(CommandLineParser& parser, VulkanDemoArgs& args)
    {
        parser.usage("[options]");

        parser.flag("--info", "enable verbose info output",
            [&]()
            {
                args.info = true;
            });

        parser.flag("--validate", "enable Khronos validation layer",
            [&]()
            {
                args.validate = true;
            });
    }

    inline std::vector<const char*> vulkanEnabledLayers(const VulkanDemoArgs& args)
    {
        std::vector<const char*> enabledLayers;

        if (args.validate)
        {
            enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
        }

        return enabledLayers;
    }

    inline bool applyVulkanDemoArgs(const VulkanDemoArgs& args)
    {
        if (args.info)
        {
            printEnable(Print::Info, true);
        }

        return true;
    }

} // namespace mango::demo

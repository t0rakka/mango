/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>

namespace mango::vulkan
{

    class VulkanWindow : public Window
    {
    public:
        VulkanWindow();
        ~VulkanWindow();
    };

} // namespace mango::vulkan

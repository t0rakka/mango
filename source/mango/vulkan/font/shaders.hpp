/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

namespace mango::vulkan::font_shaders
{

    // Format-agnostic storage image: GL_EXT_shader_image_load_formatted at shader
    // compile time; shaderStorageImage*WithoutFormat on the logical device at
    // runtime. The format is determined by the bound EncodeTarget.imageView.
    const char* computeShader();

} // namespace mango::vulkan::font_shaders

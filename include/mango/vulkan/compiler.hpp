/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.h>
#include <mango/core/configure.hpp>

namespace mango::vulkan
{

    enum class ShaderStage
    {
        Vertex,
        Fragment,
        Compute,
    };

    enum class ShaderVariableKind
    {
        Attribute,
        Uniform,
        Varying,
    };

    struct ShaderVariable
    {
        ShaderVariableKind kind;
        std::string name;
        std::string type;
        s32 set = -1;
        s32 binding = -1;
        s32 location = -1;
    };

    struct Shader
    {
        std::vector<u32> spirv;
        std::string log;
        std::vector<ShaderVariable> variables;

        bool valid() const
        {
            return !spirv.empty();
        }

        void print() const;
    };

    class Compiler
    {
    public:
        Compiler();
        ~Compiler();

        Compiler(const Compiler&) = delete;
        Compiler& operator=(const Compiler&) = delete;

        Shader compile(std::string_view source, ShaderStage stage);
        static Shader loadSPIRV(const u32* data, size_t wordCount);

        static VkShaderModule createShaderModule(VkDevice device, const Shader& shader);
    };

} // namespace mango::vulkan

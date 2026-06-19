/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        TessellationControl,
        TessellationEvaluation,
        Geometry,
        Fragment,
        Compute,
        Task,
        Mesh,
        RayGen,
        AnyHit,
        ClosestHit,
        Miss,
        Intersection,
        Callable,
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
        s32 glType = 0;
        std::string typeName;
        s32 set = -1;
        s32 binding = -1;
        s32 location = -1;
    };

    struct Shader
    {
        VkShaderStageFlagBits stage = {};
        std::vector<u32> spirv;
        std::string log;
        std::vector<ShaderVariable> variables;

        bool valid() const;
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

        static VkShaderModule createShaderModule(VkDevice device, const std::vector<u32>& spirv);
        static VkShaderModule createShaderModule(VkDevice device, const Shader& shader);
    };

    VkShaderStageFlagBits toVkShaderStage(ShaderStage stage);

    std::string_view getString(ShaderStage stage);
    std::string_view getString(VkShaderStageFlagBits stage);

    std::string_view getString(ShaderVariableKind kind);
    std::string_view getGlDefineTypeString(s32 glDefineType);

    std::string_view getTypeString(const ShaderVariable& variable);

} // namespace mango::vulkan

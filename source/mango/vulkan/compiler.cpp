/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/configure.hpp>
#include <mango/core/print.hpp>
#include <mango/vulkan/compiler.hpp>
#include <mango/vulkan/vulkan.hpp>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>

namespace mango::vulkan
{

    namespace
    {
        EShLanguage toGlslangStage(ShaderStage stage)
        {
            switch (stage)
            {
                case ShaderStage::Vertex:   return EShLangVertex;
                case ShaderStage::Fragment: return EShLangFragment;
                case ShaderStage::Compute:  return EShLangCompute;
            }

            return EShLangCount;
        }

        const char* getString(ShaderVariableKind kind)
        {
            switch (kind)
            {
                case ShaderVariableKind::Attribute: return "attribute";
                case ShaderVariableKind::Uniform:   return "uniform";
                case ShaderVariableKind::Varying:   return "varying";
            }

            return "unknown";
        }

        void collectReflection(Shader& shader, glslang::TProgram& program, EShLanguage stage)
        {
            if (!program.buildReflection())
            {
                return;
            }

            if (stage == EShLangVertex)
            {
                for (int i = 0; i < program.getNumLiveAttributes(); ++i)
                {
                    const char* name = program.getAttributeName(i);
                    const glslang::TObjectReflection& attribute = program.getPipeInput(i);

                    ShaderVariable variable;
                    variable.kind = ShaderVariableKind::Attribute;
                    variable.name = name ? name : "";
                    variable.type = fmt::format("glsl_type({})", program.getAttributeType(i));
                    variable.location = s32(attribute.layoutLocation());
                    shader.variables.push_back(variable);
                }
            }

            for (int i = 0; i < program.getNumLiveUniformVariables(); ++i)
            {
                const char* name = program.getUniformName(i);

                ShaderVariable variable;
                variable.kind = ShaderVariableKind::Uniform;
                variable.name = name ? name : "";
                variable.type = fmt::format("glsl_type({})", program.getUniformType(i));
                variable.binding = program.getUniformBinding(i);
                shader.variables.push_back(variable);
            }
        }

    } // namespace

    // ------------------------------------------------------------------------------
    // Shader
    // ------------------------------------------------------------------------------

    void Shader::print() const
    {
        if (!log.empty())
        {
            printLine(Print::Info, "Shader log:\n{}", log);
        }

        if (variables.empty())
        {
            printLine(Print::Info, "Shader reflection: (none)");
            return;
        }

        printLine(Print::Info, "Shader reflection:");

        for (const ShaderVariable& variable : variables)
        {
            std::string bindings;

            if (variable.location >= 0)
            {
                bindings += fmt::format(" location={}", variable.location);
            }

            if (variable.set >= 0)
            {
                bindings += fmt::format(" set={}", variable.set);
            }

            if (variable.binding >= 0)
            {
                bindings += fmt::format(" binding={}", variable.binding);
            }

            printLine(Print::Info, "  {} \"{}\" : {}{}",
                getString(variable.kind),
                variable.name,
                variable.type,
                bindings);
        }
    }

    // ------------------------------------------------------------------------------
    // Compiler
    // ------------------------------------------------------------------------------

    Compiler::Compiler()
    {
        glslang::InitializeProcess();
    }

    Compiler::~Compiler()
    {
        glslang::FinalizeProcess();
    }

    Shader Compiler::compile(std::string_view source, ShaderStage stage)
    {
        Shader shader;
        const EShLanguage eshStage = toGlslangStage(stage);

        glslang::TShader glslShader(eshStage);
        std::string sourceStorage(source);
        const char* sourceCString = sourceStorage.c_str();
        glslShader.setStrings(&sourceCString, 1);
        glslShader.setEntryPoint("main");
        glslShader.setEnvInput(glslang::EShSourceGlsl, eshStage, glslang::EShClientVulkan, 100);
        glslShader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
        glslShader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

        const EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);
        const TBuiltInResource& resources = *GetDefaultResources();

        if (!glslShader.parse(&resources, 450, ECoreProfile, false, false, messages))
        {
            shader.log = std::string("Parse failed:\n") + glslShader.getInfoLog() + "\n" + glslShader.getInfoDebugLog();
            return shader;
        }

        glslang::TProgram program;
        program.addShader(&glslShader);

        if (!program.link(messages))
        {
            shader.log = std::string("Link failed:\n") + program.getInfoLog() + "\n" + program.getInfoDebugLog();
            return shader;
        }

        const glslang::TIntermediate* intermediate = program.getIntermediate(eshStage);
        if (!intermediate)
        {
            shader.log = "No SPIR-V intermediate after linking.";
            return shader;
        }

        collectReflection(shader, program, eshStage);

        glslang::SpvOptions spvOptions {};
        spvOptions.generateDebugInfo = false;
        spvOptions.stripDebugInfo = true;
        spvOptions.disableOptimizer = false;
        spvOptions.optimizeSize = false;

        std::vector<unsigned int> spirv;
        glslang::GlslangToSpv(*intermediate, spirv, &spvOptions);
        shader.spirv.assign(spirv.begin(), spirv.end());

        return shader;
    }

    Shader Compiler::loadSPIRV(const u32* data, size_t wordCount)
    {
        Shader shader;

        if (!data || wordCount == 0)
        {
            shader.log = "Empty SPIR-V input.";
            return shader;
        }

        shader.spirv.assign(data, data + wordCount);
        return shader;
    }

    VkShaderModule Compiler::createShaderModule(VkDevice device, const Shader& shader)
    {
        if (!shader.valid())
        {
            return VK_NULL_HANDLE;
        }

        VkShaderModuleCreateInfo createInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = shader.spirv.size() * sizeof(u32),
            .pCode = shader.spirv.data(),
        };

        VkShaderModule module = VK_NULL_HANDLE;
        VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &module);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkCreateShaderModule: {}", getString(result));
            return VK_NULL_HANDLE;
        }

        return module;
    }

} // namespace mango::vulkan

/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/configure.hpp>
#include <mango/core/print.hpp>
#include <mango/vulkan/vulkan.hpp>

#if 0

#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>

class GLSlangInitializer
{
public:
    GLSlangInitializer()
    {
        glslang::InitializeProcess();
    }

    ~GLSlangInitializer()
    {
        glslang::FinalizeProcess();
    }
};

// TODO: alternatively, make a compiler class and re-use it to compile programs
//       Compiler (initialize/deinitialize) -> Program (reflection, spirv binary)
static GLSlangInitializer g_glslang_initializer;

namespace glslang
{

struct TBuiltInResource {
    int maxLights;
    int maxClipPlanes;
    int maxTextureUnits;
    int maxTextureCoords;
    int maxVertexAttribs;
    int maxVertexUniformComponents;
    int maxVaryingFloats;
    int maxVertexTextureImageUnits;
    int maxCombinedTextureImageUnits;
    int maxTextureImageUnits;
    int maxFragmentUniformComponents;
    int maxDrawBuffers;
    int maxVertexUniformVectors;
    int maxVaryingVectors;
    int maxFragmentUniformVectors;
    int maxVertexOutputVectors;
    int maxFragmentInputVectors;
    int minProgramTexelOffset;
    int maxProgramTexelOffset;
    int maxClipDistances;
    int maxComputeWorkGroupCountX;
    int maxComputeWorkGroupCountY;
    int maxComputeWorkGroupCountZ;
    int maxComputeWorkGroupSizeX;
    int maxComputeWorkGroupSizeY;
    int maxComputeWorkGroupSizeZ;
    int maxComputeUniformComponents;
    int maxComputeTextureImageUnits;
    int maxComputeImageUniforms;
    int maxComputeAtomicCounters;
    int maxComputeAtomicCounterBuffers;
    int maxVaryingComponents;
    int maxVertexOutputComponents;
    int maxGeometryInputComponents;
    int maxGeometryOutputComponents;
    int maxFragmentInputComponents;
    int maxImageUnits;
    int maxCombinedImageUnitsAndFragmentOutputs;
    int maxCombinedShaderOutputResources;
    int maxImageSamples;
    int maxVertexImageUniforms;
    int maxTessControlImageUniforms;
    int maxTessEvaluationImageUniforms;
    int maxGeometryImageUniforms;
    int maxFragmentImageUniforms;
    int maxCombinedImageUniforms;
    int maxGeometryTextureImageUnits;
    int maxGeometryOutputVertices;
    int maxGeometryTotalOutputComponents;
    int maxGeometryUniformComponents;
    int maxGeometryVaryingComponents;
    int maxTessControlInputComponents;
    int maxTessControlOutputComponents;
    int maxTessControlTextureImageUnits;
    int maxTessControlUniformComponents;
    int maxTessControlTotalOutputComponents;
    int maxTessEvaluationInputComponents;
    int maxTessEvaluationOutputComponents;
    int maxTessEvaluationTextureImageUnits;
    int maxTessEvaluationUniformComponents;
    int maxTessPatchComponents;
    int maxPatchVertices;
    int maxTessGenLevel;
    int maxViewports;
    int maxVertexAtomicCounters;
    int maxTessControlAtomicCounters;
    int maxTessEvaluationAtomicCounters;
    int maxGeometryAtomicCounters;
    int maxFragmentAtomicCounters;
    int maxCombinedAtomicCounters;
    int maxAtomicCounterBindings;
    int maxVertexAtomicCounterBuffers;
    int maxTessControlAtomicCounterBuffers;
    int maxTessEvaluationAtomicCounterBuffers;
    int maxGeometryAtomicCounterBuffers;
    int maxFragmentAtomicCounterBuffers;
    int maxCombinedAtomicCounterBuffers;
    int maxAtomicCounterBufferSize;
    int maxTransformFeedbackBuffers;
    int maxTransformFeedbackInterleavedComponents;
    int maxCullDistances;
    int maxCombinedClipAndCullDistances;
    int maxSamples;
    int maxMeshOutputVerticesNV;
    int maxMeshOutputPrimitivesNV;
    int maxMeshWorkGroupSizeX_NV;
    int maxMeshWorkGroupSizeY_NV;
    int maxMeshWorkGroupSizeZ_NV;
    int maxTaskWorkGroupSizeX_NV;
    int maxTaskWorkGroupSizeY_NV;
    int maxTaskWorkGroupSizeZ_NV;
    int maxMeshViewCountNV;
    int maxMeshOutputVerticesEXT;
    int maxMeshOutputPrimitivesEXT;
    int maxMeshWorkGroupSizeX_EXT;
    int maxMeshWorkGroupSizeY_EXT;
    int maxMeshWorkGroupSizeZ_EXT;
    int maxTaskWorkGroupSizeX_EXT;
    int maxTaskWorkGroupSizeY_EXT;
    int maxTaskWorkGroupSizeZ_EXT;
    int maxMeshViewCountEXT;
    int maxDualSourceDrawBuffersEXT;

    TLimits limits;
};

const TBuiltInResource DefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxMeshOutputVerticesEXT = */ 256,
    /* .maxMeshOutputPrimitivesEXT = */ 256,
    /* .maxMeshWorkGroupSizeX_EXT = */ 128,
    /* .maxMeshWorkGroupSizeY_EXT = */ 128,
    /* .maxMeshWorkGroupSizeZ_EXT = */ 128,
    /* .maxTaskWorkGroupSizeX_EXT = */ 128,
    /* .maxTaskWorkGroupSizeY_EXT = */ 128,
    /* .maxTaskWorkGroupSizeZ_EXT = */ 128,
    /* .maxMeshViewCountEXT = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,

    /* .limits = */ {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }};

} // namespace

namespace mango::vulkan
{

    // ------------------------------------------------------------------------------
    // Compiler
    // ------------------------------------------------------------------------------

    std::vector<uint32_t> compileGLSLtoSPIRV(const std::string& source, EShLanguage stage) {
        glslang::TShader shader(stage);
        const char* src = source.c_str();
        shader.setStrings(&src, 1);
        shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

        //if (!shader.parse(&glslang::DefaultTBuiltInResource, 100, false, EShMessages::EShMsgDefault)) {
        //    throw std::runtime_error(shader.getInfoLog());
        //}

        glslang::TProgram program;
        program.addShader(&shader);

        if (!program.link(EShMsgDefault)) {
            throw std::runtime_error(program.getInfoLog());
        }

        /*
        for (int i = 0; i < program.getNumLiveUniformVariables(); ++i) {
            int id = program.getUniformIndex(i);
            const char* name = program.getUniformName(id);

            const glslang::TType* type = program.getUniformTType(id);
            int binding = -1, set = -1;
            
            if (type->getQualifier().hasBinding())
                binding = type->getQualifier().layoutBinding;

            if (type->getQualifier().hasSet())
                set = type->getQualifier().layoutSet;

            std::cout << "Uniform: " << name
                    << " | Type: " << type->getCompleteString()
                    << " | Binding: " << binding
                    << " | Set: " << set
                    << std::endl;
        }
        */

        std::vector<uint32_t> spirv;
        glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
        return spirv;
    }

    void test()
    {
        std::string vs = R"(
            #version 450
            layout(location = 0) in vec3 inPos;
            void main() {
                gl_Position = vec4(inPos, 1.0);
            }
        )";

        auto spirv = glslang::compileGLSLtoSPIRV(vs, EShLangVertex);

        //std::cout << "Compiled SPIR-V size: " << spirv.size() * sizeof(uint32_t) << " bytes\n";
    }

} // namespace mango::vulkan

#endif // 0

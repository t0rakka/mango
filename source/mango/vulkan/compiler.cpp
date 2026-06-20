/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
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

    // glslang reflection type ids (glslang/MachineIndependent/gl_types.h)
    enum GlDefineType : s32
    {
        GL_FLOAT                                        = 0x1406,
        GL_FLOAT_VEC2                                   = 0x8B50,
        GL_FLOAT_VEC3                                   = 0x8B51,
        GL_FLOAT_VEC4                                   = 0x8B52,
        GL_DOUBLE                                       = 0x140A,
        GL_DOUBLE_VEC2                                  = 0x8FFC,
        GL_DOUBLE_VEC3                                  = 0x8FFD,
        GL_DOUBLE_VEC4                                  = 0x8FFE,
        GL_INT                                          = 0x1404,
        GL_INT_VEC2                                     = 0x8B53,
        GL_INT_VEC3                                     = 0x8B54,
        GL_INT_VEC4                                     = 0x8B55,
        GL_UNSIGNED_INT                                 = 0x1405,
        GL_UNSIGNED_INT_VEC2                            = 0x8DC6,
        GL_UNSIGNED_INT_VEC3                            = 0x8DC7,
        GL_UNSIGNED_INT_VEC4                            = 0x8DC8,
        GL_INT64                                        = 0x140E,
        GL_INT64_VEC2                                   = 0x8FE9,
        GL_INT64_VEC3                                   = 0x8FEA,
        GL_INT64_VEC4                                   = 0x8FEB,
        GL_UNSIGNED_INT64                               = 0x140F,
        GL_UNSIGNED_INT64_VEC2                          = 0x8FF5,
        GL_UNSIGNED_INT64_VEC3                          = 0x8FF6,
        GL_UNSIGNED_INT64_VEC4                          = 0x8FF7,
        GL_UNSIGNED_INT16_VEC2                          = 0x8FF1,
        GL_UNSIGNED_INT16_VEC3                          = 0x8FF2,
        GL_UNSIGNED_INT16_VEC4                          = 0x8FF3,
        GL_INT16                                        = 0x8FE4,
        GL_INT16_VEC2                                   = 0x8FE5,
        GL_INT16_VEC3                                   = 0x8FE6,
        GL_INT16_VEC4                                   = 0x8FE7,
        GL_BOOL                                         = 0x8B56,
        GL_BOOL_VEC2                                    = 0x8B57,
        GL_BOOL_VEC3                                    = 0x8B58,
        GL_BOOL_VEC4                                    = 0x8B59,
        GL_FLOAT_MAT2                                   = 0x8B5A,
        GL_FLOAT_MAT3                                   = 0x8B5B,
        GL_FLOAT_MAT4                                   = 0x8B5C,
        GL_FLOAT_MAT2x3                                 = 0x8B65,
        GL_FLOAT_MAT2x4                                 = 0x8B66,
        GL_FLOAT_MAT3x2                                 = 0x8B67,
        GL_FLOAT_MAT3x4                                 = 0x8B68,
        GL_FLOAT_MAT4x2                                 = 0x8B69,
        GL_FLOAT_MAT4x3                                 = 0x8B6A,
        GL_DOUBLE_MAT2                                  = 0x8F46,
        GL_DOUBLE_MAT3                                  = 0x8F47,
        GL_DOUBLE_MAT4                                  = 0x8F48,
        GL_DOUBLE_MAT2x3                                = 0x8F49,
        GL_DOUBLE_MAT2x4                                = 0x8F4A,
        GL_DOUBLE_MAT3x2                                = 0x8F4B,
        GL_DOUBLE_MAT3x4                                = 0x8F4C,
        GL_DOUBLE_MAT4x2                                = 0x8F4D,
        GL_DOUBLE_MAT4x3                                = 0x8F4E,
        GL_FLOAT16                                      = 0x8FF8,
        GL_FLOAT16_VEC2                                 = 0x8FF9,
        GL_FLOAT16_VEC3                                 = 0x8FFA,
        GL_FLOAT16_VEC4                                 = 0x8FFB,
        GL_FLOAT16_MAT2                                 = 0x91C5,
        GL_FLOAT16_MAT3                                 = 0x91C6,
        GL_FLOAT16_MAT4                                 = 0x91C7,
        GL_FLOAT16_MAT2x3                               = 0x91C8,
        GL_FLOAT16_MAT2x4                               = 0x91C9,
        GL_FLOAT16_MAT3x2                               = 0x91CA,
        GL_FLOAT16_MAT3x4                               = 0x91CB,
        GL_FLOAT16_MAT4x2                               = 0x91CC,
        GL_FLOAT16_MAT4x3                               = 0x91CD,
        GL_SAMPLER_1D                                   = 0x8B5D,
        GL_SAMPLER_2D                                   = 0x8B5E,
        GL_SAMPLER_3D                                   = 0x8B5F,
        GL_SAMPLER_CUBE                                 = 0x8B60,
        GL_SAMPLER_BUFFER                               = 0x8DC2,
        GL_SAMPLER_1D_ARRAY                             = 0x8DC0,
        GL_SAMPLER_2D_ARRAY                             = 0x8DC1,
        GL_SAMPLER_1D_ARRAY_SHADOW                      = 0x8DC3,
        GL_SAMPLER_2D_ARRAY_SHADOW                      = 0x8DC4,
        GL_SAMPLER_CUBE_SHADOW                          = 0x8DC5,
        GL_SAMPLER_1D_SHADOW                            = 0x8B61,
        GL_SAMPLER_2D_SHADOW                            = 0x8B62,
        GL_SAMPLER_2D_RECT                              = 0x8B63,
        GL_SAMPLER_2D_RECT_SHADOW                       = 0x8B64,
        GL_SAMPLER_2D_MULTISAMPLE                       = 0x9108,
        GL_SAMPLER_2D_MULTISAMPLE_ARRAY                 = 0x910B,
        GL_SAMPLER_CUBE_MAP_ARRAY                       = 0x900C,
        GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW                = 0x900D,
        GL_FLOAT16_SAMPLER_1D                           = 0x91CE,
        GL_FLOAT16_SAMPLER_2D                           = 0x91CF,
        GL_FLOAT16_SAMPLER_3D                           = 0x91D0,
        GL_FLOAT16_SAMPLER_CUBE                         = 0x91D1,
        GL_FLOAT16_SAMPLER_2D_RECT                      = 0x91D2,
        GL_FLOAT16_SAMPLER_1D_ARRAY                     = 0x91D3,
        GL_FLOAT16_SAMPLER_2D_ARRAY                     = 0x91D4,
        GL_FLOAT16_SAMPLER_CUBE_MAP_ARRAY               = 0x91D5,
        GL_FLOAT16_SAMPLER_BUFFER                       = 0x91D6,
        GL_FLOAT16_SAMPLER_2D_MULTISAMPLE               = 0x91D7,
        GL_FLOAT16_SAMPLER_2D_MULTISAMPLE_ARRAY         = 0x91D8,
        GL_FLOAT16_SAMPLER_1D_SHADOW                    = 0x91D9,
        GL_FLOAT16_SAMPLER_2D_SHADOW                    = 0x91DA,
        GL_FLOAT16_SAMPLER_2D_RECT_SHADOW               = 0x91DB,
        GL_FLOAT16_SAMPLER_1D_ARRAY_SHADOW              = 0x91DC,
        GL_FLOAT16_SAMPLER_2D_ARRAY_SHADOW              = 0x91DD,
        GL_FLOAT16_SAMPLER_CUBE_SHADOW                  = 0x91DE,
        GL_FLOAT16_SAMPLER_CUBE_MAP_ARRAY_SHADOW        = 0x91DF,
        GL_FLOAT16_IMAGE_1D                             = 0x91E0,
        GL_FLOAT16_IMAGE_2D                             = 0x91E1,
        GL_FLOAT16_IMAGE_3D                             = 0x91E2,
        GL_FLOAT16_IMAGE_2D_RECT                        = 0x91E3,
        GL_FLOAT16_IMAGE_CUBE                           = 0x91E4,
        GL_FLOAT16_IMAGE_1D_ARRAY                       = 0x91E5,
        GL_FLOAT16_IMAGE_2D_ARRAY                       = 0x91E6,
        GL_FLOAT16_IMAGE_CUBE_MAP_ARRAY                 = 0x91E7,
        GL_FLOAT16_IMAGE_BUFFER                         = 0x91E8,
        GL_FLOAT16_IMAGE_2D_MULTISAMPLE                 = 0x91E9,
        GL_FLOAT16_IMAGE_2D_MULTISAMPLE_ARRAY           = 0x91EA,
        GL_INT_SAMPLER_1D                               = 0x8DC9,
        GL_INT_SAMPLER_2D                               = 0x8DCA,
        GL_INT_SAMPLER_3D                               = 0x8DCB,
        GL_INT_SAMPLER_CUBE                             = 0x8DCC,
        GL_INT_SAMPLER_1D_ARRAY                         = 0x8DCE,
        GL_INT_SAMPLER_2D_ARRAY                         = 0x8DCF,
        GL_INT_SAMPLER_2D_RECT                          = 0x8DCD,
        GL_INT_SAMPLER_BUFFER                           = 0x8DD0,
        GL_INT_SAMPLER_2D_MULTISAMPLE                   = 0x9109,
        GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY             = 0x910C,
        GL_INT_SAMPLER_CUBE_MAP_ARRAY                   = 0x900E,
        GL_UNSIGNED_INT_SAMPLER_1D                      = 0x8DD1,
        GL_UNSIGNED_INT_SAMPLER_2D                      = 0x8DD2,
        GL_UNSIGNED_INT_SAMPLER_3D                      = 0x8DD3,
        GL_UNSIGNED_INT_SAMPLER_CUBE                    = 0x8DD4,
        GL_UNSIGNED_INT_SAMPLER_1D_ARRAY                = 0x8DD6,
        GL_UNSIGNED_INT_SAMPLER_2D_ARRAY                = 0x8DD7,
        GL_UNSIGNED_INT_SAMPLER_2D_RECT                 = 0x8DD5,
        GL_UNSIGNED_INT_SAMPLER_BUFFER                  = 0x8DD8,
        GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY    = 0x910D,
        GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY          = 0x900F,
        GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE          = 0x910A,
        GL_IMAGE_1D                                     = 0x904C,
        GL_IMAGE_2D                                     = 0x904D,
        GL_IMAGE_3D                                     = 0x904E,
        GL_IMAGE_2D_RECT                                = 0x904F,
        GL_IMAGE_CUBE                                   = 0x9050,
        GL_IMAGE_BUFFER                                 = 0x9051,
        GL_IMAGE_1D_ARRAY                               = 0x9052,
        GL_IMAGE_2D_ARRAY                               = 0x9053,
        GL_IMAGE_CUBE_MAP_ARRAY                         = 0x9054,
        GL_IMAGE_2D_MULTISAMPLE                         = 0x9055,
        GL_IMAGE_2D_MULTISAMPLE_ARRAY                   = 0x9056,
        GL_INT_IMAGE_1D                                 = 0x9057,
        GL_INT_IMAGE_2D                                 = 0x9058,
        GL_INT_IMAGE_3D                                 = 0x9059,
        GL_INT_IMAGE_2D_RECT                            = 0x905A,
        GL_INT_IMAGE_CUBE                               = 0x905B,
        GL_INT_IMAGE_BUFFER                             = 0x905C,
        GL_INT_IMAGE_1D_ARRAY                           = 0x905D,
        GL_INT_IMAGE_2D_ARRAY                           = 0x905E,
        GL_INT_IMAGE_CUBE_MAP_ARRAY                     = 0x905F,
        GL_INT_IMAGE_2D_MULTISAMPLE                     = 0x9060,
        GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY               = 0x9061,
        GL_UNSIGNED_INT_IMAGE_1D                        = 0x9062,
        GL_UNSIGNED_INT_IMAGE_2D                        = 0x9063,
        GL_UNSIGNED_INT_IMAGE_3D                        = 0x9064,
        GL_UNSIGNED_INT_IMAGE_2D_RECT                   = 0x9065,
        GL_UNSIGNED_INT_IMAGE_CUBE                      = 0x9066,
        GL_UNSIGNED_INT_IMAGE_BUFFER                    = 0x9067,
        GL_UNSIGNED_INT_IMAGE_1D_ARRAY                  = 0x9068,
        GL_UNSIGNED_INT_IMAGE_2D_ARRAY                  = 0x9069,
        GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY            = 0x906A,
        GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE            = 0x906B,
        GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY      = 0x906C,
        GL_UNSIGNED_INT_ATOMIC_COUNTER                  = 0x92DB,
    };

    VkShaderStageFlagBits toVkShaderStage(ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage::Vertex:                 return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderStage::TessellationControl:    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            case ShaderStage::TessellationEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            case ShaderStage::Geometry:               return VK_SHADER_STAGE_GEOMETRY_BIT;
            case ShaderStage::Fragment:               return VK_SHADER_STAGE_FRAGMENT_BIT;
            case ShaderStage::Compute:                return VK_SHADER_STAGE_COMPUTE_BIT;
            case ShaderStage::Task:                   return VK_SHADER_STAGE_TASK_BIT_EXT;
            case ShaderStage::Mesh:                   return VK_SHADER_STAGE_MESH_BIT_EXT;
            case ShaderStage::RayGen:                 return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            case ShaderStage::AnyHit:                 return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            case ShaderStage::ClosestHit:             return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            case ShaderStage::Miss:                   return VK_SHADER_STAGE_MISS_BIT_KHR;
            case ShaderStage::Intersection:           return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            case ShaderStage::Callable:               return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
        }

        return {};
    }

    std::string_view getString(VkShaderStageFlagBits stage)
    {
        switch (stage)
        {
            case VK_SHADER_STAGE_VERTEX_BIT:                  return "vertex";
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:    return "tessellation control";
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return "tessellation evaluation";
            case VK_SHADER_STAGE_GEOMETRY_BIT:                return "geometry";
            case VK_SHADER_STAGE_FRAGMENT_BIT:                return "fragment";
            case VK_SHADER_STAGE_COMPUTE_BIT:                 return "compute";
            case VK_SHADER_STAGE_TASK_BIT_EXT:                return "task";
            case VK_SHADER_STAGE_MESH_BIT_EXT:                return "mesh";
            case VK_SHADER_STAGE_RAYGEN_BIT_KHR:              return "raygen";
            case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:             return "any hit";
            case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:         return "closest hit";
            case VK_SHADER_STAGE_MISS_BIT_KHR:                return "miss";
            case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:        return "intersection";
            case VK_SHADER_STAGE_CALLABLE_BIT_KHR:            return "callable";
            case VK_SHADER_STAGE_ALL_GRAPHICS:                return "all graphics";
            case VK_SHADER_STAGE_ALL:                         return "all";
            case VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI:  return "subpass shading";
            case VK_SHADER_STAGE_CLUSTER_CULLING_BIT_HUAWEI:  return "cluster culling";
        }

        return "unknown";
    }

    std::string_view getString(ShaderStage stage)
    {
        return getString(toVkShaderStage(stage));
    }

    std::string_view getString(ShaderVariableKind kind)
    {
        switch (kind)
        {
            case ShaderVariableKind::Attribute: return "attribute";
            case ShaderVariableKind::Uniform:   return "uniform";
            case ShaderVariableKind::Varying:   return "varying";
        }

        return "unknown";
    }

    std::string_view getTypeString(const ShaderVariable& variable)
    {
        if (variable.glType != 0)
        {
            return getGlDefineTypeString(variable.glType);
        }

        if (!variable.typeName.empty())
        {
            return variable.typeName;
        }

        return "unknown";
    }

    #define GL_CASE(x) case x: return #x

    std::string_view getGlDefineTypeString(s32 glDefineType)
    {
        switch (glDefineType)
        {
            GL_CASE(GL_FLOAT);
            GL_CASE(GL_FLOAT_VEC2);
            GL_CASE(GL_FLOAT_VEC3);
            GL_CASE(GL_FLOAT_VEC4);
            GL_CASE(GL_DOUBLE);
            GL_CASE(GL_DOUBLE_VEC2);
            GL_CASE(GL_DOUBLE_VEC3);
            GL_CASE(GL_DOUBLE_VEC4);
            GL_CASE(GL_INT);
            GL_CASE(GL_INT_VEC2);
            GL_CASE(GL_INT_VEC3);
            GL_CASE(GL_INT_VEC4);
            GL_CASE(GL_UNSIGNED_INT);
            GL_CASE(GL_UNSIGNED_INT_VEC2);
            GL_CASE(GL_UNSIGNED_INT_VEC3);
            GL_CASE(GL_UNSIGNED_INT_VEC4);
            GL_CASE(GL_INT64);
            GL_CASE(GL_INT64_VEC2);
            GL_CASE(GL_INT64_VEC3);
            GL_CASE(GL_INT64_VEC4);
            GL_CASE(GL_UNSIGNED_INT64);
            GL_CASE(GL_UNSIGNED_INT64_VEC2);
            GL_CASE(GL_UNSIGNED_INT64_VEC3);
            GL_CASE(GL_UNSIGNED_INT64_VEC4);
            GL_CASE(GL_UNSIGNED_INT16_VEC2);
            GL_CASE(GL_UNSIGNED_INT16_VEC3);
            GL_CASE(GL_UNSIGNED_INT16_VEC4);
            GL_CASE(GL_INT16);
            GL_CASE(GL_INT16_VEC2);
            GL_CASE(GL_INT16_VEC3);
            GL_CASE(GL_INT16_VEC4);
            GL_CASE(GL_BOOL);
            GL_CASE(GL_BOOL_VEC2);
            GL_CASE(GL_BOOL_VEC3);
            GL_CASE(GL_BOOL_VEC4);
            GL_CASE(GL_FLOAT_MAT2);
            GL_CASE(GL_FLOAT_MAT3);
            GL_CASE(GL_FLOAT_MAT4);
            GL_CASE(GL_FLOAT_MAT2x3);
            GL_CASE(GL_FLOAT_MAT2x4);
            GL_CASE(GL_FLOAT_MAT3x2);
            GL_CASE(GL_FLOAT_MAT3x4);
            GL_CASE(GL_FLOAT_MAT4x2);
            GL_CASE(GL_FLOAT_MAT4x3);
            GL_CASE(GL_DOUBLE_MAT2);
            GL_CASE(GL_DOUBLE_MAT3);
            GL_CASE(GL_DOUBLE_MAT4);
            GL_CASE(GL_DOUBLE_MAT2x3);
            GL_CASE(GL_DOUBLE_MAT2x4);
            GL_CASE(GL_DOUBLE_MAT3x2);
            GL_CASE(GL_DOUBLE_MAT3x4);
            GL_CASE(GL_DOUBLE_MAT4x2);
            GL_CASE(GL_DOUBLE_MAT4x3);
            GL_CASE(GL_FLOAT16);
            GL_CASE(GL_FLOAT16_VEC2);
            GL_CASE(GL_FLOAT16_VEC3);
            GL_CASE(GL_FLOAT16_VEC4);
            GL_CASE(GL_FLOAT16_MAT2);
            GL_CASE(GL_FLOAT16_MAT3);
            GL_CASE(GL_FLOAT16_MAT4);
            GL_CASE(GL_FLOAT16_MAT2x3);
            GL_CASE(GL_FLOAT16_MAT2x4);
            GL_CASE(GL_FLOAT16_MAT3x2);
            GL_CASE(GL_FLOAT16_MAT3x4);
            GL_CASE(GL_FLOAT16_MAT4x2);
            GL_CASE(GL_FLOAT16_MAT4x3);
            GL_CASE(GL_SAMPLER_1D);
            GL_CASE(GL_SAMPLER_2D);
            GL_CASE(GL_SAMPLER_3D);
            GL_CASE(GL_SAMPLER_CUBE);
            GL_CASE(GL_SAMPLER_BUFFER);
            GL_CASE(GL_SAMPLER_1D_ARRAY);
            GL_CASE(GL_SAMPLER_2D_ARRAY);
            GL_CASE(GL_SAMPLER_1D_ARRAY_SHADOW);
            GL_CASE(GL_SAMPLER_2D_ARRAY_SHADOW);
            GL_CASE(GL_SAMPLER_CUBE_SHADOW);
            GL_CASE(GL_SAMPLER_1D_SHADOW);
            GL_CASE(GL_SAMPLER_2D_SHADOW);
            GL_CASE(GL_SAMPLER_2D_RECT);
            GL_CASE(GL_SAMPLER_2D_RECT_SHADOW);
            GL_CASE(GL_SAMPLER_2D_MULTISAMPLE);
            GL_CASE(GL_SAMPLER_2D_MULTISAMPLE_ARRAY);
            GL_CASE(GL_SAMPLER_CUBE_MAP_ARRAY);
            GL_CASE(GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW);
            GL_CASE(GL_FLOAT16_SAMPLER_1D);
            GL_CASE(GL_FLOAT16_SAMPLER_2D);
            GL_CASE(GL_FLOAT16_SAMPLER_3D);
            GL_CASE(GL_FLOAT16_SAMPLER_CUBE);
            GL_CASE(GL_FLOAT16_SAMPLER_2D_RECT);
            GL_CASE(GL_FLOAT16_SAMPLER_1D_ARRAY);
            GL_CASE(GL_FLOAT16_SAMPLER_2D_ARRAY);
            GL_CASE(GL_FLOAT16_SAMPLER_CUBE_MAP_ARRAY);
            GL_CASE(GL_FLOAT16_SAMPLER_BUFFER);
            GL_CASE(GL_FLOAT16_SAMPLER_2D_MULTISAMPLE);
            GL_CASE(GL_FLOAT16_SAMPLER_2D_MULTISAMPLE_ARRAY);
            GL_CASE(GL_FLOAT16_SAMPLER_1D_SHADOW);
            GL_CASE(GL_FLOAT16_SAMPLER_2D_SHADOW);
            GL_CASE(GL_FLOAT16_SAMPLER_2D_RECT_SHADOW);
            GL_CASE(GL_FLOAT16_SAMPLER_1D_ARRAY_SHADOW);
            GL_CASE(GL_FLOAT16_SAMPLER_2D_ARRAY_SHADOW);
            GL_CASE(GL_FLOAT16_SAMPLER_CUBE_SHADOW);
            GL_CASE(GL_FLOAT16_SAMPLER_CUBE_MAP_ARRAY_SHADOW);
            GL_CASE(GL_FLOAT16_IMAGE_1D);
            GL_CASE(GL_FLOAT16_IMAGE_2D);
            GL_CASE(GL_FLOAT16_IMAGE_3D);
            GL_CASE(GL_FLOAT16_IMAGE_2D_RECT);
            GL_CASE(GL_FLOAT16_IMAGE_CUBE);
            GL_CASE(GL_FLOAT16_IMAGE_1D_ARRAY);
            GL_CASE(GL_FLOAT16_IMAGE_2D_ARRAY);
            GL_CASE(GL_FLOAT16_IMAGE_CUBE_MAP_ARRAY);
            GL_CASE(GL_FLOAT16_IMAGE_BUFFER);
            GL_CASE(GL_FLOAT16_IMAGE_2D_MULTISAMPLE);
            GL_CASE(GL_FLOAT16_IMAGE_2D_MULTISAMPLE_ARRAY);
            GL_CASE(GL_INT_SAMPLER_1D);
            GL_CASE(GL_INT_SAMPLER_2D);
            GL_CASE(GL_INT_SAMPLER_3D);
            GL_CASE(GL_INT_SAMPLER_CUBE);
            GL_CASE(GL_INT_SAMPLER_1D_ARRAY);
            GL_CASE(GL_INT_SAMPLER_2D_ARRAY);
            GL_CASE(GL_INT_SAMPLER_2D_RECT);
            GL_CASE(GL_INT_SAMPLER_BUFFER);
            GL_CASE(GL_INT_SAMPLER_2D_MULTISAMPLE);
            GL_CASE(GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY);
            GL_CASE(GL_INT_SAMPLER_CUBE_MAP_ARRAY);
            GL_CASE(GL_UNSIGNED_INT_SAMPLER_1D);
            GL_CASE(GL_UNSIGNED_INT_SAMPLER_2D);
            GL_CASE(GL_UNSIGNED_INT_SAMPLER_3D);
            GL_CASE(GL_UNSIGNED_INT_SAMPLER_CUBE);
            GL_CASE(GL_UNSIGNED_INT_SAMPLER_1D_ARRAY);
            GL_CASE(GL_UNSIGNED_INT_SAMPLER_2D_ARRAY);
            GL_CASE(GL_UNSIGNED_INT_SAMPLER_2D_RECT);
            GL_CASE(GL_UNSIGNED_INT_SAMPLER_BUFFER);
            GL_CASE(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY);
            GL_CASE(GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY);
            GL_CASE(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE);
            GL_CASE(GL_IMAGE_1D);
            GL_CASE(GL_IMAGE_2D);
            GL_CASE(GL_IMAGE_3D);
            GL_CASE(GL_IMAGE_2D_RECT);
            GL_CASE(GL_IMAGE_CUBE);
            GL_CASE(GL_IMAGE_BUFFER);
            GL_CASE(GL_IMAGE_1D_ARRAY);
            GL_CASE(GL_IMAGE_2D_ARRAY);
            GL_CASE(GL_IMAGE_CUBE_MAP_ARRAY);
            GL_CASE(GL_IMAGE_2D_MULTISAMPLE);
            GL_CASE(GL_IMAGE_2D_MULTISAMPLE_ARRAY);
            GL_CASE(GL_INT_IMAGE_1D);
            GL_CASE(GL_INT_IMAGE_2D);
            GL_CASE(GL_INT_IMAGE_3D);
            GL_CASE(GL_INT_IMAGE_2D_RECT);
            GL_CASE(GL_INT_IMAGE_CUBE);
            GL_CASE(GL_INT_IMAGE_BUFFER);
            GL_CASE(GL_INT_IMAGE_1D_ARRAY);
            GL_CASE(GL_INT_IMAGE_2D_ARRAY);
            GL_CASE(GL_INT_IMAGE_CUBE_MAP_ARRAY);
            GL_CASE(GL_INT_IMAGE_2D_MULTISAMPLE);
            GL_CASE(GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY);
            GL_CASE(GL_UNSIGNED_INT_IMAGE_1D);
            GL_CASE(GL_UNSIGNED_INT_IMAGE_2D);
            GL_CASE(GL_UNSIGNED_INT_IMAGE_3D);
            GL_CASE(GL_UNSIGNED_INT_IMAGE_2D_RECT);
            GL_CASE(GL_UNSIGNED_INT_IMAGE_CUBE);
            GL_CASE(GL_UNSIGNED_INT_IMAGE_BUFFER);
            GL_CASE(GL_UNSIGNED_INT_IMAGE_1D_ARRAY);
            GL_CASE(GL_UNSIGNED_INT_IMAGE_2D_ARRAY);
            GL_CASE(GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY);
            GL_CASE(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE);
            GL_CASE(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY);
            GL_CASE(GL_UNSIGNED_INT_ATOMIC_COUNTER);
        }

        return "unknown";
    }

    #undef GL_CASE

    namespace
    {
        EShLanguage toGlslangStage(ShaderStage stage)
        {
            switch (stage)
            {
                case ShaderStage::Vertex:                 return EShLangVertex;
                case ShaderStage::TessellationControl:    return EShLangTessControl;
                case ShaderStage::TessellationEvaluation: return EShLangTessEvaluation;
                case ShaderStage::Geometry:               return EShLangGeometry;
                case ShaderStage::Fragment:               return EShLangFragment;
                case ShaderStage::Compute:                return EShLangCompute;
                case ShaderStage::Task:                   return EShLangTaskNV;
                case ShaderStage::Mesh:                   return EShLangMeshNV;
                case ShaderStage::RayGen:                 return EShLangRayGen;
                case ShaderStage::AnyHit:                 return EShLangAnyHit;
                case ShaderStage::ClosestHit:             return EShLangClosestHit;
                case ShaderStage::Miss:                   return EShLangMiss;
                case ShaderStage::Intersection:           return EShLangIntersect;
                case ShaderStage::Callable:               return EShLangCallable;
            }

            return EShLangCount;
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
                    variable.glType = attribute.glDefineType;
                    variable.location = s32(attribute.layoutLocation());
                    shader.variables.push_back(variable);
                }
            }

            for (int i = 0; i < program.getNumLiveUniformVariables(); ++i)
            {
                const char* name = program.getUniformName(i);

                const glslang::TObjectReflection& uniform = program.getUniform(i);

                ShaderVariable variable;
                variable.kind = ShaderVariableKind::Uniform;
                variable.name = name ? name : "";
                variable.glType = uniform.glDefineType;

                const int blockIndex = program.getUniformBlockIndex(i);
                if (blockIndex >= 0)
                {
                    variable.typeName = program.getUniformBlockName(blockIndex);
                }

                variable.binding = program.getUniformBinding(i);
                shader.variables.push_back(variable);
            }
        }

    } // namespace

    // ------------------------------------------------------------------------------
    // Shader
    // ------------------------------------------------------------------------------

    bool Shader::valid() const
    {
        return !spirv.empty();
    }

    void Shader::print() const
    {
        printLine(Print::Info, "[{} shader]", getString(stage));

        if (!log.empty())
        {
            printLine(Print::Info, "Shader log:\n{}", log);
        }

        if (variables.empty())
        {
            printLine(Print::Info, 2, "Reflection: (none)");
            return;
        }

        printLine(Print::Info, 2, "Reflection:");

        for (const ShaderVariable& variable : variables)
        {
            std::string bindings;

            if (variable.location >= 0)
            {
                bindings += fmt::format(", location = {}", variable.location);
            }

            if (variable.set >= 0)
            {
                bindings += fmt::format(", set = {}", variable.set);
            }

            if (variable.binding >= 0)
            {
                bindings += fmt::format(", binding = {}", variable.binding);
            }

            printLine(Print::Info, 4, "  {}: \"{}\", {}{}",
                getString(variable.kind),
                variable.name,
                getTypeString(variable),
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
        shader.stage = toVkShaderStage(stage);
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

    VkShaderModule Compiler::createShaderModule(VkDevice device, const std::vector<u32>& spirv)
    {
        if (spirv.empty())
        {
            return VK_NULL_HANDLE;
        }

        VkShaderModuleCreateInfo createInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = spirv.size() * sizeof(u32),
            .pCode = spirv.data(),
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

    VkShaderModule Compiler::createShaderModule(VkDevice device, const Shader& shader)
    {
        if (!shader.valid())
        {
            return VK_NULL_HANDLE;
        }

        return createShaderModule(device, shader.spirv);
    }

} // namespace mango::vulkan
